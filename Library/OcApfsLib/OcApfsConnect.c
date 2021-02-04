/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "OcApfsInternal.h"
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcApfsLib.h>
#include <Library/OcPeCoffExtLib.h>
#include <Library/OcAppleSecureBootLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcDriverConnectionLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/OcVariable.h>
#include <Protocol/ApfsUnsupportedBds.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

LIST_ENTRY               mApfsPrivateDataList = INITIALIZE_LIST_HEAD_VARIABLE (mApfsPrivateDataList);
STATIC UINT64            mApfsMinimalVersion = OC_APFS_VERSION_DEFAULT;
STATIC UINT32            mApfsMinimalDate    = OC_APFS_DATE_DEFAULT;
STATIC UINT32            mOcScanPolicy;
STATIC BOOLEAN           mIgnoreVerbose;
STATIC BOOLEAN           mGlobalConnect;
STATIC BOOLEAN           mDisconnectHandles;
STATIC EFI_SYSTEM_TABLE  *mNullSystemTable;

//
// There seems to exist a driver with a very large version, which is treated by
// apfs kernel extension to have 0 version. Follow suit.
//
STATIC UINT64 mApfsBlacklistedVersions[] = {
  4294966999999999999ULL
};

STATIC
EFI_STATUS
ApfsCheckOpenCoreScanPolicy (
  IN EFI_HANDLE  Handle
  )
{
  UINT32      ScanPolicy;

  //
  // If filesystem limitations are set and APFS is not allowed,
  // report failure.
  //
  if ((mOcScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) != 0
    && (mOcScanPolicy & OC_SCAN_ALLOW_FS_APFS) == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // If device type locking is set and this device is not allowed,
  // report failure.
  //
  if ((mOcScanPolicy & OC_SCAN_DEVICE_LOCK) != 0) {
    ScanPolicy = OcGetDevicePolicyType (Handle, NULL);
    if ((ScanPolicy & mOcScanPolicy) == 0) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ApfsVerifyDriverVersion (
  IN APFS_PRIVATE_DATA  *PrivateData,
  IN VOID               *DriverBuffer,
  IN UINT32             DriverSize
  )
{
  EFI_STATUS            Status;
  APFS_DRIVER_VERSION   *DriverVersion;
  UINT64                RealVersion;
  UINT32                RealDate;
  UINTN                 Index;
  BOOLEAN               HasLegitVersion;

  Status = PeCoffGetApfsDriverVersion (
    DriverBuffer,
    DriverSize,
    &DriverVersion
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "OCJS: No APFS driver version found for %g - %r\n",
      &PrivateData->LocationInfo.ContainerUuid,
      Status
      ));

    RealVersion = 22; ///< From apfs kernel extension.
    RealDate    = 0;
  } else {
    RealVersion = DriverVersion->Version;
    RealDate    = 0;

    //
    // Parse YYYY/MM/DD date.
    //
    for (Index = 0; Index < 10; ++Index) {
      if ((Index == 4 || Index == 7)) {
        if (DriverVersion->Date[Index] != '/') {
          RealDate = 0;
          break;
        }
        continue;
      }

      if (DriverVersion->Date[Index] < '0' || DriverVersion->Date[Index] > '9') {
        RealDate = 0;
        break;
      }

      RealDate *= 10;
      RealDate += DriverVersion->Date[Index] - '0';
    }

    if (RealDate == 0) {
      DEBUG ((
        DEBUG_WARN,
        "OCJS: APFS driver date is invalid for %g\n",
        &PrivateData->LocationInfo.ContainerUuid
        ));
    }
  }

  for (Index = 0; Index < ARRAY_SIZE (mApfsBlacklistedVersions); ++Index) {
    if (RealVersion == mApfsBlacklistedVersions[Index]) {
      DEBUG ((
        DEBUG_WARN,
        "OCJS: APFS driver version %Lu is blacklisted for %g, treating as 0\n",
        DriverVersion->Version,
        &PrivateData->LocationInfo.ContainerUuid
        ));
      RealVersion = 0;
      break;
    }
  }

  HasLegitVersion = (mApfsMinimalVersion == 0 || mApfsMinimalVersion <= RealVersion)
    && (mApfsMinimalDate == 0 || mApfsMinimalDate <= RealDate);

  DEBUG ((
    DEBUG_INFO,
    "OCJS: APFS driver %Lu/%u found for %g, required >= %Lu/%u, %a\n",
    RealVersion,
    RealDate,
    &PrivateData->LocationInfo.ContainerUuid,
    mApfsMinimalVersion,
    mApfsMinimalDate,
    HasLegitVersion ? "allow" : "prohibited"
    ));

  if (HasLegitVersion) {
    return EFI_SUCCESS;
  }

  return EFI_SECURITY_VIOLATION;
}

STATIC
EFI_STATUS
ApfsRegisterPartition (
  IN  EFI_HANDLE             Handle,
  IN  EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN  APFS_NX_SUPERBLOCK     *SuperBlock,
  OUT APFS_PRIVATE_DATA      **PrivateDataPointer
  )
{
  EFI_STATUS           Status;
  APFS_PRIVATE_DATA    *PrivateData;

  PrivateData = AllocateZeroPool (sizeof (*PrivateData));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // File private data fields.
  //
  PrivateData->Signature = APFS_PRIVATE_DATA_SIGNATURE;
  PrivateData->LocationInfo.ControllerHandle = Handle;
  CopyGuid (&PrivateData->LocationInfo.ContainerUuid, &SuperBlock->Uuid);
  PrivateData->BlockIo = BlockIo;
  PrivateData->ApfsBlockSize = SuperBlock->BlockSize;
  PrivateData->LbaMultiplier = PrivateData->ApfsBlockSize / PrivateData->BlockIo->Media->BlockSize;
  PrivateData->EfiJumpStart  = SuperBlock->EfiJumpStart;
  InternalApfsInitFusionData (SuperBlock, PrivateData);

  //
  // Install boot record information.
  // This guarantees us that we never register twice.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
    &PrivateData->LocationInfo.ControllerHandle,
    &gApfsEfiBootRecordInfoProtocolGuid,
    &PrivateData->LocationInfo,
    NULL
    );
  if (EFI_ERROR (Status)) {
    FreePool (PrivateData);
    return Status;
  }

  //
  // Save into the list and return.
  //
  InsertTailList (&mApfsPrivateDataList, &PrivateData->Link);
  *PrivateDataPointer = PrivateData;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ApfsStartDriver (
  IN APFS_PRIVATE_DATA  *PrivateData,
  IN VOID               *DriverBuffer,
  IN UINT32             DriverSize
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_HANDLE                 ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_IMAGE_LOAD             LoadImage;
  APPLE_SECURE_BOOT_PROTOCOL *SecureBoot;
  UINT8                      Policy;

  Status = PeCoffVerifyAppleSignature (
    DriverBuffer,
    &DriverSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Failed to verify signature %g - %r\n",
      &PrivateData->LocationInfo.ContainerUuid,
      Status
      ));
    return Status;
  }

  Status = ApfsVerifyDriverVersion (
    PrivateData,
    DriverBuffer,
    DriverSize
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
    PrivateData->LocationInfo.ControllerHandle,
    &gEfiDevicePathProtocolGuid,
    (VOID **) &DevicePath
    );
  if (EFI_ERROR (Status)) {
    DevicePath = NULL;
  }

  SecureBoot = OcAppleSecureBootGetProtocol ();
  ASSERT (SecureBoot != NULL);
  Status = SecureBoot->GetPolicy (
    SecureBoot,
    &Policy
    );
  //
  // Load directly when we have Apple Secure Boot.
  // - Either normal.
  // - Or during DMG loading.
  //
  if ((!EFI_ERROR (Status) && Policy != AppleImg4SbModeDisabled)
    || (OcAppleSecureBootGetDmgLoading (&Policy) && Policy != AppleImg4SbModeDisabled)) {
    LoadImage = OcImageLoaderLoad;
  } else {
    LoadImage = gBS->LoadImage;
  }

  ImageHandle = NULL;
  Status = LoadImage (
    FALSE,
    gImageHandle,
    DevicePath,
    DriverBuffer,
    DriverSize,
    &ImageHandle
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Failed to load %g - %r\n",
      &PrivateData->LocationInfo.ContainerUuid,
      Status
      ));
    return Status;
  }

  //
  // Disable verbose mode on request.
  // Note, we cannot deallocate null text output table once we allocate it.
  //
  if (mIgnoreVerbose) {
    Status = gBS->HandleProtocol (
      ImageHandle,
      &gEfiLoadedImageProtocolGuid,
      (VOID *) &LoadedImage
      );
    if (!EFI_ERROR (Status)) {
      if (mNullSystemTable == NULL) {
        mNullSystemTable = AllocateNullTextOutSystemTable (gST);
      }
      if (mNullSystemTable != NULL) {
        LoadedImage->SystemTable = mNullSystemTable;
      }
    }
  }

  Status = gBS->StartImage (
    ImageHandle,
    NULL,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Failed to start %g - %r\n",
      &PrivateData->LocationInfo.ContainerUuid,
      Status
      ));

    gBS->UnloadImage (ImageHandle);
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCJS: Connecting %a%a APFS driver on handle %p\n",
    mGlobalConnect ? "globally" : "normally",
    mDisconnectHandles ? " with disconnection" : "",
    PrivateData->LocationInfo.ControllerHandle
    ));

  if (mDisconnectHandles) {
    //
    // Unblock handles as some types of firmware, such as that on the HP EliteBook 840 G2,
    // may automatically lock all volumes without filesystem drivers upon
    // any attempt to connect them.
    // REF: https://github.com/acidanthera/bugtracker/issues/1128
    //
    OcDisconnectDriversOnHandle (PrivateData->LocationInfo.ControllerHandle);
  }

  if (mGlobalConnect) {
    //
    // Connect all devices when implicitly requested. This is a workaround
    // for some older HP laptops, which for some reason fail to connect by both
    // drive and partition handles.
    // REF: https://github.com/acidanthera/bugtracker/issues/960
    //
    OcConnectDrivers ();
  } else {
    //
    // Recursively connect controller to get apfs.efi loaded.
    // We cannot use apfs.efi handle as it apparently creates new handles.
    // This follows ApfsJumpStart driver implementation.
    //
    gBS->ConnectController (PrivateData->LocationInfo.ControllerHandle, NULL, NULL, TRUE);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ApfsConnectDevice (
  IN EFI_HANDLE             Handle,
  IN EFI_BLOCK_IO_PROTOCOL  *BlockIo
  )
{
  EFI_STATUS           Status;
  APFS_NX_SUPERBLOCK   *SuperBlock;
  APFS_PRIVATE_DATA    *PrivateData;
  VOID                 *DriverBuffer;
  UINT32               DriverSize;

  //
  // This may still be not APFS but some other file system.
  //
  Status = InternalApfsReadSuperBlock (BlockIo, &SuperBlock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCJS: Got APFS super block for %g\n", &SuperBlock->Uuid));

  //
  // We no longer need super block once we register ourselves.
  //
  Status = ApfsRegisterPartition (Handle, BlockIo, SuperBlock, &PrivateData);
  FreePool (SuperBlock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // We cannot load drivers if we have no fusion drive pair as they are not
  // guaranteed to be located on each drive.
  //
  if (!PrivateData->CanLoadDriver) {
    return EFI_NOT_READY;
  }

  Status = InternalApfsReadDriver (PrivateData, &DriverSize, &DriverBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ApfsStartDriver (PrivateData, DriverBuffer, DriverSize);
  FreePool (DriverBuffer);
  return Status;
}

VOID
OcApfsConfigure (
  IN UINT64   MinVersion,
  IN UINT32   MinDate,
  IN UINT32   ScanPolicy,
  IN BOOLEAN  GlobalConnect,
  IN BOOLEAN  DisconnectHandles,
  IN BOOLEAN  IgnoreVerbose
  )
{
  //
  // Translate special values like 0 and -1.
  //
  if (MinVersion == OC_APFS_VERSION_ANY) {
    mApfsMinimalVersion = 0;
  } else if (MinVersion == OC_APFS_VERSION_AUTO) {
    mApfsMinimalVersion = OC_APFS_VERSION_DEFAULT;
  } else {
    mApfsMinimalVersion = MinVersion;
  }

  if (MinDate == OC_APFS_DATE_ANY) {
    mApfsMinimalDate = 0;
  } else if (MinDate == OC_APFS_DATE_AUTO) {
    mApfsMinimalDate = OC_APFS_DATE_DEFAULT;
  } else {
    mApfsMinimalDate = MinDate;
  }

  mOcScanPolicy      = ScanPolicy;
  mIgnoreVerbose     = IgnoreVerbose;
  mGlobalConnect     = GlobalConnect;
  mDisconnectHandles = DisconnectHandles;
}

EFI_STATUS
OcApfsConnectHandle (
  IN EFI_HANDLE  Handle,
  IN BOOLEAN     VerifyPolicy
  )
{
  EFI_STATUS             Status;
  VOID                   *TempProtocol;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;

  //
  // In the end of successful APFS or some other driver connection
  // we have a filesystem driver.
  // We have nothing to do if the device is already connected.
  //
  Status = gBS->HandleProtocol (
    Handle,
    &gEfiSimpleFileSystemProtocolGuid,
    &TempProtocol
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "OCJS: FS already connected\n"));
    return EFI_ALREADY_STARTED;
  }

  //
  // Obtain Block I/O.
  // We do not need to care about 2nd revision, as apfs.efi does not use it.
  //
  Status = gBS->HandleProtocol (
    Handle,
    &gEfiBlockIoProtocolGuid,
    (VOID **) &BlockIo
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCJS: Cannot connect, BlockIo error - %r\n", Status));
    return EFI_UNSUPPORTED;
  }

  //
  // Filter out handles:
  // - Without media.
  // - Which are not partitions (APFS containers).
  // - Which have non-POT block size.
  //
  if (BlockIo->Media == NULL
    || !BlockIo->Media->LogicalPartition) {
    return EFI_UNSUPPORTED;
  }

  if (BlockIo->Media->BlockSize == 0
    || (BlockIo->Media->BlockSize & (BlockIo->Media->BlockSize - 1)) != 0) {
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Cannot connect, BlockIo malformed: %d %u\n",
      BlockIo->Media->LogicalPartition,
      BlockIo->Media->BlockSize
      ));
    return EFI_UNSUPPORTED;
  }

  //
  // Filter out handles, which do not respect OpenCore policy.
  //
  if (VerifyPolicy) {
    Status = ApfsCheckOpenCoreScanPolicy (Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCJS: Cannot connect, Policy error - %r\n", Status));
      return Status;
    }
  }

  //
  // If the handle is marked as unsupported, we should respect this.
  // TODO: Install this protocol on failure (not in ApfsJumpStart)?
  //
  Status = gBS->HandleProtocol (
    Handle,
    &gApfsUnsupportedBdsProtocolGuid,
    &TempProtocol
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCJS: Cannot connect, unsupported BDS\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // If we installed APFS EFI boot record, then this handle is already
  // handled, though potentially not connected.
  //
  Status = gBS->HandleProtocol (
    Handle,
    &gApfsEfiBootRecordInfoProtocolGuid,
    &TempProtocol
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCJS: Cannot connect, already handled\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // This is possibly APFS, try connecting.
  //
  return ApfsConnectDevice (Handle, BlockIo);
}

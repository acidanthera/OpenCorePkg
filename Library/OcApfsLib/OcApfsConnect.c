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
#include <Library/OcAppleImageVerificationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/OcVariables.h>
#include <Protocol/ApfsUnsupportedBds.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

LIST_ENTRY               mApfsPrivateDataList;
STATIC EFI_SYSTEM_TABLE  *mNullSystemTable;

STATIC
EFI_STATUS
ApfsCheckOpenCoreScanPolicy (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  UINT32      ScanPolicy;
  UINT32      OcScanPolicy;
  UINTN       OcScanPolicySize;

  //
  // If scan policy is missing, just ignore.
  // FIXME: This should be passed directly.
  //
  OcScanPolicy = 0;
  OcScanPolicySize = sizeof (OcScanPolicy);
  Status = gRT->GetVariable (
    OC_SCAN_POLICY_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    NULL,
    &OcScanPolicySize,
    &OcScanPolicy
    );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // If filesystem limitations are set and APFS is not allowed,
  // report failure.
  //
  if ((OcScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) != 0
    && (OcScanPolicy & OC_SCAN_ALLOW_FS_APFS) == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // If device type locking is set and this device is not allowed,
  // report failure.
  //
  if ((OcScanPolicy & OC_SCAN_DEVICE_LOCK) != 0) {
    ScanPolicy = OcGetDevicePolicyType (Handle, NULL);
    if ((ScanPolicy & OcScanPolicy) == 0) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
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
  // Lazy-initialise private data list.
  // Private data list must be valid as we try to register partition.
  //
  if (mApfsPrivateDataList.ForwardLink == NULL) {
    InitializeListHead (&mApfsPrivateDataList);
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
  IN UINTN              DriverSize
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_HANDLE                 ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  Status = VerifyApplePeImageSignature (
    DriverBuffer,
    &DriverSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Failed to verify signature %g - %r\n",
      PrivateData->LocationInfo.ContainerUuid,
      Status
      ));
    return Status;
  }

  //
  // TODO: Verify driver timestamp here.
  //

  Status = gBS->HandleProtocol (
    PrivateData->LocationInfo.ControllerHandle,
    &gEfiDevicePathProtocolGuid,
    (VOID **) &DevicePath
    );
  if (EFI_ERROR (Status)) {
    DevicePath = NULL;
  }

  ImageHandle = NULL;
  Status = gBS->LoadImage (
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
      PrivateData->LocationInfo.ContainerUuid,
      Status
      ));
    return Status;
  }

  //
  // Disable verbose mode.
  // FIXME: This should be configurable.
  // Note, we cannot deallocate null text output table once we allocate it.
  //

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

  Status = gBS->StartImage (
    ImageHandle,
    NULL,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCJS: Failed to start %g - %r\n",
      PrivateData->LocationInfo.ContainerUuid,
      Status
      ));

    gBS->UnloadImage (ImageHandle);
    return Status;
  }

  //
  // Connect loaded apfs.efi to controller from which we retrieve it
  //
  gBS->ConnectController (PrivateData->LocationInfo.ControllerHandle, NULL, NULL, TRUE);
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
  UINTN                DriverSize;

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

  Status = InternalApfsReadJumpStartDriver (PrivateData, &DriverSize, &DriverBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ApfsStartDriver (PrivateData, DriverBuffer, DriverSize);
  FreePool (DriverBuffer);
  return Status;
}

EFI_STATUS
OcApfsConnectDevice (
  IN EFI_HANDLE  Handle
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
    &gEfiSimpleFileSystemProtocolGuid,
    Handle,
    &TempProtocol
    );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Obtain Block I/O.
  // We do not need to care about 2nd revision, as apfs.efi does not use it.
  //
  Status = gBS->HandleProtocol (
    &gEfiBlockIoProtocolGuid,
    Handle,
    (VOID **) &BlockIo
    );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Filter out handles:
  // - Without media.
  // - Which are not partitions (APFS containers).
  // - Which have non-POT block size.
  //
  if (BlockIo->Media == NULL
    || !BlockIo->Media->LogicalPartition
    || BlockIo->Media->BlockSize == 0
    || (BlockIo->Media->BlockSize & (BlockIo->Media->BlockSize - 1)) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Filter out handles, which do not respect OpenCore policy.
  //
  Status = ApfsCheckOpenCoreScanPolicy (Handle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If the handle is marked as unsupported, we should respect this.
  // TODO: Install this protocol on failure (not in ApfsJumpStart)?
  //
  Status = gBS->HandleProtocol (
    &gApfsUnsupportedBdsProtocolGuid,
    Handle,
    &TempProtocol
    );
  if (!EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // If we installed APFS EFI boot record, then this handle is already
  // handled, though potentially not connected.
  //
  Status = gBS->HandleProtocol (
    &gApfsEfiBootRecordInfoProtocolGuid,
    Handle,
    &TempProtocol
    );
  if (!EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // This is possibly APFS, try connecting.
  //
  return ApfsConnectDevice (Handle, BlockIo);
}

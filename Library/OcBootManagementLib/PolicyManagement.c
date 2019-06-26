/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BootManagementInternal.h"

#include <Guid/AppleApfsInfo.h>
#include <Guid/AppleBless.h>
#include <Guid/AppleHfsInfo.h>
#include <Guid/AppleVariable.h>
#include <Guid/Gpt.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/OcVariables.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/ApfsEfiBootRecordInfo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/LoadedImage.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

UINT32
OcGetDevicePolicyType (
  IN  EFI_HANDLE   Handle,
  OUT BOOLEAN      *External  OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT8                     SubType;

  if (External != NULL) {
    *External = FALSE;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **) &DevicePath);
  if (EFI_ERROR (Status)) {
    return 0;
  }

  //
  // FIXME: This code does not take care of RamDisk exception as specified per
  // https://github.com/acidanthera/bugtracker/issues/335.
  // Currently we do not need it, but in future we may.
  //

  while (!IsDevicePathEnd (DevicePath)) {
    if (DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) {
      SubType = DevicePathSubType (DevicePath);
      switch (SubType) {
        case MSG_SATA_DP:
          return OC_SCAN_ALLOW_DEVICE_SATA;
        case MSG_SASEX_DP:
          return OC_SCAN_ALLOW_DEVICE_SASEX;
        case MSG_SCSI_DP:
          return OC_SCAN_ALLOW_DEVICE_SCSI;
        case MSG_NVME_NAMESPACE_DP:
          return OC_SCAN_ALLOW_DEVICE_NVME;
        case MSG_ATAPI_DP:
          if (External != NULL) {
            *External = TRUE;
          }
          return OC_SCAN_ALLOW_DEVICE_ATAPI;
        case MSG_USB_DP:
          if (External != NULL) {
            *External = TRUE;
          }
          return OC_SCAN_ALLOW_DEVICE_USB;
        case MSG_1394_DP:
          if (External != NULL) {
            *External = TRUE;
          }
          return OC_SCAN_ALLOW_DEVICE_FIREWIRE;
        case MSG_SD_DP:
        case MSG_EMMC_DP:
          if (External != NULL) {
            *External = TRUE;
          }
          return OC_SCAN_ALLOW_DEVICE_SDCARD;
      }

      //
      // We do not have good protection against device tunneling.
      // These things must be considered:
      // - Thunderbolt 2 PCI-e pass-through
      // - Thunderbolt 3 PCI-e pass-through (Type-C, may be different from 2)
      // - FireWire devices
      // For now we hope that first messaging type protects us, and all
      // subsequent messaging types are tunneled.
      //

      break;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  return 0;
}

UINT32
OcGetFileSystemPolicyType (
  IN  EFI_HANDLE   Handle
  )
{
  CONST EFI_PARTITION_ENTRY  *PartitionEntry;

  PartitionEntry = OcGetGptPartitionEntry (Handle);

  if (PartitionEntry == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Missing partition info for %p\n", Handle));
    return 0;
  }

  if (CompareGuid (&PartitionEntry->PartitionTypeGUID, &gAppleApfsPartitionTypeGuid)) {
    return OC_SCAN_ALLOW_FS_APFS;
  }

  if (CompareGuid (&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeSystemPartGuid)) {
    return OC_SCAN_ALLOW_FS_ESP;
  }

  //
  // Unsure whether these two should be separate, likely not.
  //
  if (CompareGuid (&PartitionEntry->PartitionTypeGUID, &gAppleHfsPartitionTypeGuid)
    || CompareGuid (&PartitionEntry->PartitionTypeGUID, &gAppleHfsBootPartitionTypeGuid)) {
    return OC_SCAN_ALLOW_FS_HFS;
  }

  return 0;
}

RETURN_STATUS
InternalCheckScanPolicy (
  IN  EFI_HANDLE                       Handle,
  IN  UINT32                           Policy,
  OUT BOOLEAN                          *External OPTIONAL
  )
{
  UINT32                     DevicePolicy;
  UINT32                     FileSystemPolicy;

  //
  // Always request policy type due to external checks.
  //
  DevicePolicy = OcGetDevicePolicyType (Handle, External);
  if ((Policy & OC_SCAN_DEVICE_LOCK) != 0 && (Policy & DevicePolicy) == 0) {
    DEBUG ((DEBUG_INFO, "OCB: Invalid device policy (%x/%x) for %p\n", DevicePolicy, Policy, Handle));
    return EFI_SECURITY_VIOLATION;
  }

  if ((Policy & OC_SCAN_FILE_SYSTEM_LOCK) != 0) {
    FileSystemPolicy = OcGetFileSystemPolicyType (Handle);

    if ((Policy & FileSystemPolicy) == 0) {
      DEBUG ((DEBUG_INFO, "OCB: Invalid file system policy (%x/%x) for %p\n", FileSystemPolicy, Policy, Handle));
      return EFI_SECURITY_VIOLATION;
    }
  }

  return RETURN_SUCCESS;
}

EFI_LOADED_IMAGE_PROTOCOL *
OcGetAppleBootLoadedImage (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL    *CurrNode;
  FILEPATH_DEVICE_PATH        *LastNode;
  BOOLEAN                     IsMacOS;
  UINTN                       PathLen;
  UINTN                       Index;

  IsMacOS = FALSE;

  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);

  if (!EFI_ERROR (Status) && LoadedImage->FilePath) {
    LastNode = NULL;

    for (CurrNode = LoadedImage->FilePath; !IsDevicePathEnd (CurrNode); CurrNode = NextDevicePathNode (CurrNode)) {
      if (DevicePathType (CurrNode) == MEDIA_DEVICE_PATH && DevicePathSubType (CurrNode) == MEDIA_FILEPATH_DP) {
        LastNode = (FILEPATH_DEVICE_PATH *) CurrNode;
      }
    }

    if (LastNode != NULL) {
      //
      // Detect macOS by boot.efi in the bootloader name.
      //
      PathLen = OcFileDevicePathNameLen (LastNode);
      if (PathLen >= L_STR_LEN ("boot.efi")) {
        Index = PathLen - L_STR_LEN ("boot.efi");
        IsMacOS = (Index == 0 || LastNode->PathName[Index - 1] == L'\\')
          && CompareMem (&LastNode->PathName[Index], L"boot.efi", L_STR_SIZE (L"boot.efi")) == 0;
      }
    }
  }

  return IsMacOS ? LoadedImage : NULL;
}

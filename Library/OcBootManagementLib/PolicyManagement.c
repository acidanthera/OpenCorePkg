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
#include <Guid/OcVariable.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/ApfsEfiBootRecordInfo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/LoadedImage.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
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
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  ACPI_HID_DEVICE_PATH      *Acpi;
  HARDDRIVE_DEVICE_PATH     *HardDrive;
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

  DevicePathWalker = DevicePath;
  while (!IsDevicePathEnd (DevicePathWalker)) {
    if (DevicePathType (DevicePathWalker) == MESSAGING_DEVICE_PATH) {
      SubType = DevicePathSubType (DevicePathWalker);
      switch (SubType) {
        case MSG_SATA_DP:
          return OC_SCAN_ALLOW_DEVICE_SATA;
        case MSG_SASEX_DP:
          return OC_SCAN_ALLOW_DEVICE_SASEX;
        case MSG_SCSI_DP:
          return OC_SCAN_ALLOW_DEVICE_SCSI;
        case MSG_APPLE_NVME_NAMESPACE_DP:
        case MSG_NVME_NAMESPACE_DP:
          return OC_SCAN_ALLOW_DEVICE_NVME;
        case MSG_ATAPI_DP:
          //
          // Check if this ATA Bus has HDD connected.
          // DVD will have NO_DISK_SIGNATURE at least for our DuetPkg.
          //
          DevicePathWalker = NextDevicePathNode (DevicePathWalker);
          HardDrive = (HARDDRIVE_DEVICE_PATH *) DevicePathWalker;
          if (!IsDevicePathEnd (DevicePathWalker)
            && DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH
            && DevicePathSubType (DevicePathWalker) == MEDIA_HARDDRIVE_DP
            && (HardDrive->SignatureType == SIGNATURE_TYPE_MBR
              || HardDrive->SignatureType == SIGNATURE_TYPE_GUID)) {
            return OC_SCAN_ALLOW_DEVICE_ATAPI;
          }

          //
          // Assume this is DVD/CD.
          //
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

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }

  DevicePathWalker = DevicePath;
  while (!IsDevicePathEnd (DevicePathWalker)) {
    if (DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH) {
      return OC_SCAN_ALLOW_DEVICE_PCI;
    }

    if (DevicePathType (DevicePathWalker) == ACPI_DEVICE_PATH) {
      Acpi = (ACPI_HID_DEVICE_PATH *) DevicePathWalker;
      if ((Acpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST
        && EISA_ID_TO_NUM (Acpi->HID) == 0x0A03) {
        //
        // Allow PciRoot.
        //
        DevicePathWalker = NextDevicePathNode (DevicePathWalker);
        continue;
      }
    } else if (DevicePathType (DevicePathWalker) == HARDWARE_DEVICE_PATH
      && DevicePathSubType (DevicePathWalker) == HW_PCI_DP) {
      //
      // Allow Pci.
      //
      DevicePathWalker = NextDevicePathNode (DevicePathWalker);
      continue;
    }

    //
    // Forbid everything else.
    //
    break;
  }

  return 0;
}

/**
  Microsoft partitions.
  https://docs.microsoft.com/ru-ru/windows/win32/api/vds/ns-vds-create_partition_parameters
**/
EFI_GUID mMsftBasicDataPartitionTypeGuid = {
  0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}
};

EFI_GUID mMsftReservedPartitionTypeGuid = {
  0xE3C9E316, 0x0B5C, 0x4DB8, {0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE}
};

EFI_GUID mMsftRecoveryPartitionTypeGuid = {
  0xDE94BBA4, 0x06D1, 0x4D40, {0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC}
};

/**
  Linux partitions.
  https://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_type_GUIDs
**/
EFI_GUID mLinuxRootX86PartitionTypeGuid = {
  0x44479540, 0xF297, 0x41B2, {0x9A, 0xF7, 0xD1, 0x31, 0xD5, 0xF0, 0x45, 0x8A}
};

EFI_GUID mLinuxRootX8664PartitionTypeGuid = {
  0x4F68BCE3, 0xE8CD, 0x4DB1, {0x96, 0xE7, 0xFB, 0xCA, 0xF9, 0x84, 0xB7, 0x09}
};

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

  if (CompareGuid (&PartitionEntry->PartitionTypeGUID, &mMsftBasicDataPartitionTypeGuid)) {
    return OC_SCAN_ALLOW_FS_NTFS;
  }

  if (CompareGuid (&PartitionEntry->PartitionTypeGUID, &mLinuxRootX86PartitionTypeGuid)
    || CompareGuid (&PartitionEntry->PartitionTypeGUID, &mLinuxRootX8664PartitionTypeGuid)) {
    return OC_SCAN_ALLOW_FS_EXT;
  }

  return 0;
}

EFI_STATUS
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

  return EFI_SUCCESS;
}

OC_BOOT_ENTRY_TYPE
OcGetBootDevicePathType (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT BOOLEAN                   *IsFolder   OPTIONAL,
  OUT BOOLEAN                   *IsGeneric  OPTIONAL
  )
{
  CHAR16                      *Path;
  UINTN                       PathLen;
  UINTN                       RestLen;
  UINTN                       Index;

  Path = NULL;

  if (IsGeneric != NULL) {
    *IsGeneric = FALSE;
  }

  if (IsFolder != NULL) {
    *IsFolder = FALSE;
  }

  Path = OcCopyDevicePathFullName (DevicePath, NULL);
  if (Path == NULL) {
    return OC_BOOT_UNKNOWN;
  }

  PathLen = StrLen (Path);

  //
  // Use the trailing character to determine folder.
  //
  if (IsFolder != NULL && Path[PathLen - 1] == L'\\') {
    *IsFolder = TRUE;
  }

  //
  // Detect macOS recovery by com.apple.recovery.boot in the bootloader path.
  //
  if (OcStrStrLength (Path, PathLen, L"com.apple.recovery.boot", L_STR_LEN (L"com.apple.recovery.boot")) != NULL) {
    FreePool (Path);
    return OC_BOOT_APPLE_RECOVERY;
  }

  if (OcStrStrLength (Path, PathLen, L"EFI\\APPLE", L_STR_LEN (L"EFI\\APPLE")) != NULL) {
    FreePool (Path);
    return OC_BOOT_APPLE_FW_UPDATE;
  }

  //
  // Detect macOS by boot.efi in the bootloader name.
  // Detect macOS Time Machine by tmbootpicker.efi in the bootloader name.
  //
  STATIC CONST CHAR16 *Bootloaders[] = {
    L"boot.efi",
    L"tmbootpicker.efi",
    L"bootmgfw.efi"
  };

  STATIC CONST UINTN BootloaderLengths[] = {
    L_STR_LEN (L"boot.efi"),
    L_STR_LEN (L"tmbootpicker.efi"),
    L_STR_LEN (L"bootmgfw.efi")
  };

  STATIC CONST OC_BOOT_ENTRY_TYPE BootloaderTypes[] = {
    OC_BOOT_APPLE_OS,
    OC_BOOT_APPLE_TIME_MACHINE,
    OC_BOOT_WINDOWS
  };

  for (Index = 0; Index < ARRAY_SIZE (Bootloaders); ++Index) {
    if (PathLen < BootloaderLengths[Index]) {
      continue;
    }

    RestLen = PathLen - BootloaderLengths[Index];
    if ((RestLen == 0 || Path[RestLen - 1] == L'\\')
      && OcStrniCmp (&Path[RestLen], Bootloaders[Index], BootloaderLengths[Index]) == 0) {
      FreePool (Path);
      return BootloaderTypes[Index];
    }
  }

  CONST CHAR16 *GenericBootloader      = &EFI_REMOVABLE_MEDIA_FILE_NAME[L_STR_LEN (L"\\EFI\\BOOT\\")];
  CONST UINTN  GenericBootloaderLength = L_STR_LEN (EFI_REMOVABLE_MEDIA_FILE_NAME) - L_STR_LEN (L"\\EFI\\BOOT\\");

  if (IsGeneric != NULL && PathLen >= GenericBootloaderLength) {
    RestLen = PathLen - GenericBootloaderLength;
    if ((RestLen == 0 || Path[RestLen - 1] == L'\\')
      && OcStrniCmp (&Path[RestLen], GenericBootloader, GenericBootloaderLength) == 0) {
      *IsGeneric = TRUE;
    }
  }

  FreePool (Path);
  return OC_BOOT_UNKNOWN;
}

EFI_LOADED_IMAGE_PROTOCOL *
OcGetAppleBootLoadedImage (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;

  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **)&LoadedImage
    );

  if (!EFI_ERROR (Status)
    && LoadedImage->FilePath != NULL
    && (OcGetBootDevicePathType (LoadedImage->FilePath, NULL, NULL) & OC_BOOT_APPLE_ANY) != 0) {
    return LoadedImage;
  }

  return NULL;
}

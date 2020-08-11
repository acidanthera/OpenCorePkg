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

#include <Guid/AppleBless.h>
#include <IndustryStandard/AppleDiskLabel.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcXmlLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

CHAR16 *
InternalGetAppleDiskLabel (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName,
  IN  CONST CHAR16                     *LabelFilename
  )
{
  CHAR16   *DiskLabelPath;
  UINTN    DiskLabelPathSize;
  CHAR8    *AsciiDiskLabel;
  CHAR16   *UnicodeDiskLabel;
  UINT32   DiskLabelLength;

  DiskLabelPathSize = StrSize (BootDirectoryName) + StrSize (LabelFilename) - sizeof (CHAR16);
  DiskLabelPath     = AllocatePool (DiskLabelPathSize);

  if (DiskLabelPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (DiskLabelPath, DiskLabelPathSize, L"%s%s", BootDirectoryName, LabelFilename);
  DEBUG ((DEBUG_INFO, "OCB: Trying to get label from %s\n", DiskLabelPath));

  AsciiDiskLabel = (CHAR8 *) ReadFile (FileSystem, DiskLabelPath, &DiskLabelLength, OC_MAX_VOLUME_LABEL_SIZE);
  FreePool (DiskLabelPath);

  if (AsciiDiskLabel != NULL) {
    UnicodeDiskLabel = AsciiStrCopyToUnicode (AsciiDiskLabel, DiskLabelLength);
    if (UnicodeDiskLabel != NULL) {
      UnicodeFilterString (UnicodeDiskLabel, TRUE);
    }
    FreePool (AsciiDiskLabel);
  } else {
    UnicodeDiskLabel = NULL;
  }

  return UnicodeDiskLabel;
}

EFI_STATUS
InternalGetAppleImage (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *DirectoryName,
  IN  CONST CHAR16                     *LabelFilename,
  OUT VOID                             **ImageData,
  OUT UINT32                           *DataSize
  )
{
  CHAR16   *ImagePath;
  UINTN    ImagePathSize;

  ImagePathSize = StrSize (DirectoryName) + StrSize (LabelFilename) - sizeof (CHAR16);
  ImagePath     = AllocatePool (ImagePathSize);

  if (ImagePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeSPrint (ImagePath, ImagePathSize, L"%s%s", DirectoryName, LabelFilename);
  DEBUG ((DEBUG_INFO, "OCB: Trying to get image from %s\n", ImagePath));

  *ImageData = ReadFile (FileSystem, ImagePath, DataSize, BASE_16MB);

  FreePool (ImagePath);

  if (*ImageData == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Whether it is disk label or .icns, disk label is always smaller.
  // Early abort on obviously small images.
  //
  if (*DataSize <= sizeof (APPLE_DISK_LABEL)) {
    FreePool (*ImageData);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
CHAR16 *
GetAppleRecoveryNameFromPlist (
  IN CHAR8   *SystemVersionData,
  IN UINT32  SystemVersionDataSize
  )
{
  XML_DOCUMENT        *Document;
  XML_NODE            *RootDict;
  UINT32              DictSize;
  UINT32              Index;
  CONST CHAR8         *CurrentKey;
  XML_NODE            *CurrentValue;
  CONST CHAR8         *Version;
  CHAR16              *RecoveryName;
  UINTN               RecoveryNameSize;

  Document = XmlDocumentParse (SystemVersionData, SystemVersionDataSize, FALSE);

  if (Document == NULL) {
    return NULL;
  }

  RootDict = PlistNodeCast (PlistDocumentRoot (Document), PLIST_NODE_TYPE_DICT);

  if (RootDict == NULL) {
    XmlDocumentFree (Document);
    return NULL;
  }

  RecoveryName = NULL;

  DictSize = PlistDictChildren (RootDict);
  for (Index = 0; Index < DictSize; Index++) {
    CurrentKey = PlistKeyValue (PlistDictChild (RootDict, Index, &CurrentValue));

    if (CurrentKey == NULL || AsciiStrCmp (CurrentKey, "ProductUserVisibleVersion") != 0) {
      continue;
    }

    if (PlistNodeCast (CurrentValue, PLIST_NODE_TYPE_STRING) != NULL) {
      Version = XmlNodeContent (CurrentValue);
      if (Version != NULL) {
        RecoveryNameSize = L_STR_SIZE(L"Recovery ") + AsciiStrLen (Version) * sizeof (CHAR16);
        RecoveryName = AllocatePool (RecoveryNameSize);
        if (RecoveryName != NULL) {
          UnicodeSPrint (RecoveryName, RecoveryNameSize, L"Recovery %a", Version);
          UnicodeFilterString (RecoveryName, TRUE);
        }
      }
    }

    break;
  }

  XmlDocumentFree (Document);
  return RecoveryName;
}

CHAR16 *
InternalGetAppleRecoveryName (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName
  )
{
  CHAR16   *SystemVersionPath;
  UINTN    SystemVersionPathSize;
  CHAR8    *SystemVersionData;
  CHAR16   *UnicodeDiskLabel;
  UINT32   SystemVersionDataSize;

  SystemVersionPathSize = StrSize (BootDirectoryName) + L_STR_SIZE_NT (L"SystemVersion.plist");
  SystemVersionPath     = AllocatePool (SystemVersionPathSize);

  if (SystemVersionPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (SystemVersionPath, SystemVersionPathSize, L"%sSystemVersion.plist", BootDirectoryName);
  DEBUG ((DEBUG_INFO, "OCB: Trying to get recovery from %s\n", SystemVersionPath));
  SystemVersionData = (CHAR8 *) ReadFile (FileSystem, SystemVersionPath, &SystemVersionDataSize, BASE_1MB);
  FreePool (SystemVersionPath);

  if (SystemVersionData != NULL) {
    UnicodeDiskLabel = GetAppleRecoveryNameFromPlist (SystemVersionData, SystemVersionDataSize);
    FreePool (SystemVersionData);
  } else {
    UnicodeDiskLabel = NULL;
  }

  return UnicodeDiskLabel;
}

EFI_STATUS
InternalGetRecoveryOsBooter (
  IN  EFI_HANDLE                Device,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  IN  BOOLEAN                   Basic
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *Root;
  EFI_FILE_PROTOCOL                *Recovery;
  UINTN                            FilePathSize;
  EFI_DEVICE_PATH_PROTOCOL         *TmpPath;
  UINTN                            TmpPathSize;
  CHAR16                           *DevicePathText;

  Status = gBS->HandleProtocol (
    Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FilePathSize = 0;

  if (!Basic) {
    *FilePath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
                  Root,
                  &gAppleBlessedOsxFolderInfoGuid,
                  sizeof (EFI_DEVICE_PATH_PROTOCOL),
                  &FilePathSize
                  );
  } else {
    //
    // Requested basic recovery support, i.e. only com.apple.recovery.boot folder check.
    // This is useful for locating empty USB sticks with just a dmg in them.
    //
    *FilePath = NULL;
  }

  if (*FilePath != NULL) {
    if (IsDevicePathValid (*FilePath, FilePathSize)) {
      //
      // We skip alternate entry when current one is the same.
      // This is to prevent recovery and volume duplicates on HFS+ systems.
      //

      TmpPath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
        Root,
        &gAppleBlessedSystemFolderInfoGuid,
        sizeof (EFI_DEVICE_PATH_PROTOCOL),
        &TmpPathSize
        );

      if (TmpPath != NULL) {
        if (IsDevicePathValid (TmpPath, TmpPathSize)
          && IsDevicePathEqual (TmpPath, *FilePath)) {
          DEBUG ((DEBUG_INFO, "Skipping equal alternate device path %p\n", Device));
          Status = EFI_ALREADY_STARTED;
          FreePool (*FilePath);
          *FilePath = NULL;
        }
        FreePool (TmpPath);
      }

      if (!EFI_ERROR (Status)) {
        //
        // This entry should point to a folder with recovery.
        // Apple never adds trailing slashes to blessed folder paths.
        // However, we do rely on trailing slashes in folder paths and add them here.
        //
        TmpPath = TrailedBooterDevicePath (*FilePath);
        if (TmpPath != NULL) {
          FreePool (*FilePath);
          *FilePath = TmpPath;
        }
      }
    } else {
      FreePool (*FilePath);
      *FilePath = NULL;
      Status = EFI_NOT_FOUND;
    }
  } else {
    //
    // Ok, this one can still be FileVault 2 HFS+ recovery or just a hardcoded basic recovery.
    // Apple does add its path to so called "Alternate OS blessed file/folder", but this
    // path is not accessible from HFSPlus.efi driver. Just why???
    // Their SlingShot.efi app just bruteforces com.apple.recovery.boot directory existence,
    // and we have to copy.
    //
    Status = SafeFileOpen (Root, &Recovery, L"\\com.apple.recovery.boot", EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR (Status)) {
      //
      // Do not do any extra checks for simplicity, as they will be done later either way.
      //
      Root->Close (Recovery);
      Status    = EFI_NOT_FOUND;
      TmpPath   = DevicePathFromHandle (Device);

      if (TmpPath != NULL) {
        *FilePath = AppendFileNameDevicePath (TmpPath, L"\\com.apple.recovery.boot\\");
        if (*FilePath != NULL) {
          DEBUG_CODE_BEGIN ();
          DevicePathText = ConvertDevicePathToText (*FilePath, FALSE, FALSE);
          if (DevicePathText != NULL) {
            DEBUG ((DEBUG_INFO, "OCB: Got recovery dp %s\n", DevicePathText));
            FreePool (DevicePathText);
          }
          DEBUG_CODE_END ();
          Status = EFI_SUCCESS;
        }
      }
    } else {
      Status = EFI_NOT_FOUND;
    }
  }

  Root->Close (Root);

  return Status;
}

EFI_STATUS
EFIAPI
OcGetBootEntryLabelImage (
  IN  OC_PICKER_CONTEXT          *Context,
  IN  OC_BOOT_ENTRY              *BootEntry,
  IN  UINT8                      Scale,
  OUT VOID                       **ImageData,
  OUT UINT32                     *DataLength
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *BootDirectoryName;
  EFI_HANDLE                       Device;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  *ImageData = NULL;
  *DataLength = 0;

  if (BootEntry->Type == OC_BOOT_EXTERNAL_TOOL || BootEntry->Type == OC_BOOT_RESET_NVRAM) {
    ASSERT (Context->CustomDescribe != NULL);

    Status = Context->CustomDescribe (
      Context->CustomEntryContext,
      BootEntry,
      Scale,
      NULL,
      NULL,
      ImageData,
      DataLength
      );

    DEBUG ((DEBUG_INFO, "OCB: Get custom label %s - %r\n", BootEntry->Name, Status));
    return Status;
  }

  ASSERT (BootEntry->DevicePath != NULL);

  Status = OcBootPolicyDevicePathToDirPath (
    BootEntry->DevicePath,
    &BootDirectoryName,
    &Device
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
    Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );

  if (EFI_ERROR (Status)) {
    FreePool (BootDirectoryName);
    return Status;
  }

  Status = InternalGetAppleImage (
    FileSystem,
    BootDirectoryName,
    Scale == 2 ? L".disk_label_2x" : L".disk_label",
    ImageData,
    DataLength
    );

  DEBUG ((DEBUG_INFO, "OCB: Get normal label %s - %r\n", BootEntry->Name, Status));
  FreePool (BootDirectoryName);

  return Status;
}

EFI_STATUS
EFIAPI
OcGetBootEntryIcon (
  IN  OC_PICKER_CONTEXT          *Context,
  IN  OC_BOOT_ENTRY              *BootEntry,
  OUT VOID                       **ImageData,
  OUT UINT32                     *DataLength
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *BootDirectoryName;
  EFI_HANDLE                       Device;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  *ImageData = NULL;
  *DataLength = 0;

  if (BootEntry->Type == OC_BOOT_EXTERNAL_TOOL || BootEntry->Type == OC_BOOT_RESET_NVRAM) {
    ASSERT (Context->CustomDescribe != NULL);

    Status = Context->CustomDescribe (
      Context->CustomEntryContext,
      BootEntry,
      0,
      ImageData,
      DataLength,
      NULL,
      NULL
      );

    DEBUG ((DEBUG_INFO, "OCB: OcGetBootEntryIcon - %s (tool) - %r\n", BootEntry->Name, Status));
    return Status;
  }

  ASSERT (BootEntry->DevicePath != NULL);

  Status = OcBootPolicyDevicePathToDirPath (
    BootEntry->DevicePath,
    &BootDirectoryName,
    &Device
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
    Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );

  if (EFI_ERROR (Status)) {
    FreePool (BootDirectoryName);
    return Status;
  }

  if (BootEntry->Type == OC_BOOT_EXTERNAL_OS && BootEntry->PathName != NULL) {
    //
    // Try to load the icon from the same path with appended .icns extension.
    //
    Status = InternalGetAppleImage (
      FileSystem,
      BootEntry->PathName,
      L".icns",
      ImageData,
      DataLength
      );
    
    DEBUG ((DEBUG_INFO, "OCB: OcGetBootEntryIcon - %s (custom entry) - %r\n", BootEntry->Name, Status));
    
    //
    // Return early if custom icon was loaded successfully.
    //
    if(!EFI_ERROR (Status)) {
      FreePool (BootDirectoryName);
      return Status;
    }
  }

  Status = InternalGetAppleImage (
    FileSystem,
    L"",
    L".VolumeIcon.icns",
    ImageData,
    DataLength
    );

  DEBUG ((DEBUG_INFO, "OCB: OcGetBootEntryIcon - %s (volume icon) - %r\n", BootEntry->Name, Status));

  FreePool (BootDirectoryName);

  return Status;
}

EFI_STATUS
InternalDescribeBootEntry (
  IN OUT OC_BOOT_ENTRY  *BootEntry
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *BootDirectoryName;
  CHAR16                           *RecoveryBootName;
  EFI_HANDLE                       Device;
  UINT32                           BcdSize;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  //
  // Custom entries need no special description.
  //
  if (BootEntry->Type == OC_BOOT_EXTERNAL_OS || BootEntry->Type == OC_BOOT_EXTERNAL_TOOL) {
    return EFI_SUCCESS;
  }

  Status = OcBootPolicyDevicePathToDirPath (
    BootEntry->DevicePath,
    &BootDirectoryName,
    &Device
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
    Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );

  if (EFI_ERROR (Status)) {
    FreePool (BootDirectoryName);
    return Status;
  }

  //
  // Try to use APFS-style label or legacy HFS one.
  //
  BootEntry->Name = InternalGetAppleDiskLabel (FileSystem, BootDirectoryName, L".contentDetails");
  if (BootEntry->Name == NULL) {
    BootEntry->Name = InternalGetAppleDiskLabel (FileSystem, BootDirectoryName, L".disk_label.contentDetails");
  }

  //
  // With FV2 encryption on HFS+ the actual boot happens from "Recovery HD/S/L/CoreServices".
  // For some reason "Recovery HD/S/L/CoreServices/.disk_label" may not get updated immediately,
  // and will contain "Recovery HD" despite actually pointing to "Macintosh HD".
  // This also spontaneously happens with renamed APFS volumes. The workaround is to manually
  // edit the file or sometimes choose the boot volume once more in preferences.
  //
  // TODO: Bugreport this to Apple, as this is clearly their bug, which should be reproducible
  // on original hardware.
  //
  // There exists .root_uuid, which contains real partition UUID in ASCII, however, Apple
  // BootPicker only uses it for entry deduplication, and we cannot figure out the name
  // on an encrypted volume anyway.
  //

  //
  // Windows boot entry may have a custom name, so ensure OC_BOOT_WINDOWS is set correctly.
  //
  if (BootEntry->Type == OC_BOOT_UNKNOWN && BootEntry->IsGeneric) {
    DEBUG ((DEBUG_INFO, "OCB: Trying to detect Microsoft BCD\n"));
    Status = ReadFileSize (FileSystem, L"\\EFI\\Microsoft\\Boot\\BCD", &BcdSize);
    if (!EFI_ERROR (Status)) {
      BootEntry->Type = OC_BOOT_WINDOWS;
    }
  }

  if (BootEntry->Type == OC_BOOT_WINDOWS && BootEntry->Name == NULL) {
    BootEntry->Name = AllocateCopyPool (sizeof (L"Windows"), L"Windows");
  }

  if (BootEntry->Name == NULL) {
    BootEntry->Name = GetVolumeLabel (FileSystem);
    if (BootEntry->Name != NULL
      && (!StrCmp (BootEntry->Name, L"Recovery HD")
       || !StrCmp (BootEntry->Name, L"Recovery"))) {
      if (BootEntry->Type == OC_BOOT_UNKNOWN || BootEntry->Type == OC_BOOT_APPLE_OS) {
        BootEntry->Type = OC_BOOT_APPLE_RECOVERY;
      }
      RecoveryBootName = InternalGetAppleRecoveryName (FileSystem, BootDirectoryName);
      if (RecoveryBootName != NULL) {
        FreePool (BootEntry->Name);
        BootEntry->Name = RecoveryBootName;
      }
    }
  }

  if (BootEntry->Name == NULL) {
    FreePool (BootDirectoryName);
    return EFI_NOT_FOUND;
  }

  BootEntry->PathName = BootDirectoryName;

  return EFI_SUCCESS;
}

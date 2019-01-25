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

#include <AppleMacEfi.h>

#include <Guid/AppleApfsInfo.h>
#include <Guid/AppleBless.h>
#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/AppleBootPolicy.h>

#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcXmlLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC
CHAR16 *
GetAppleDiskLabel (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName,
  IN  CONST CHAR16                     *LabelFilename
  )
{
  CHAR16   *DiskLabelPath;
  UINTN    DiskLabelPathSize;
  CHAR8    *AsciiDiskLabel;
  CHAR16   *UnicodeDiskLabel;
  UINTN    DiskLabelLength;

  DiskLabelPathSize = StrSize (BootDirectoryName) + StrLen (LabelFilename) * sizeof (CHAR16);
  DiskLabelPath = AllocatePool (DiskLabelPathSize);

  if (DiskLabelPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (DiskLabelPath, DiskLabelPathSize, L"%s%s", BootDirectoryName, LabelFilename);
  AsciiDiskLabel = (CHAR8 *) ReadFile (FileSystem, DiskLabelPath, &DiskLabelLength);
  FreePool (DiskLabelPath);

  if (AsciiDiskLabel != NULL) {
    UnicodeDiskLabel = OcAsciiStrToUnicode (AsciiDiskLabel, DiskLabelLength);
    FreePool (AsciiDiskLabel);
  } else {
    UnicodeDiskLabel = NULL;
  }

  return UnicodeDiskLabel;
}

STATIC
CHAR16 *
GetAppleRecoveryNameFromPlist (
  IN CHAR8   *SystemVersionData,
  IN UINTN   SystemVersionDataSize
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

  Document = XmlParseDocument (SystemVersionData, SystemVersionDataSize);

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
          UnicodeSPrint(RecoveryName, RecoveryNameSize, L"Recovery %a", Version);
        }
      }
    }

    break;
  }

  XmlDocumentFree (Document);
  return RecoveryName;
}

STATIC
CHAR16 *
GetAppleRecoveryName (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName
  )
{
  CHAR16   *SystemVersionPath;
  UINTN    SystemVersionPathSize;
  CHAR8    *SystemVersionData;
  CHAR16   *UnicodeDiskLabel;
  UINTN    SystemVersionDataSize;

  SystemVersionPathSize = StrSize (BootDirectoryName) + L_STR_SIZE_NT (L"SystemVersion.plist");
  SystemVersionPath = AllocatePool (SystemVersionPathSize);

  if (SystemVersionPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (SystemVersionPath, SystemVersionPathSize, L"%sSystemVersion.plist", BootDirectoryName);
  SystemVersionData = (CHAR8 *) ReadFile (FileSystem, SystemVersionPath, &SystemVersionDataSize);
  FreePool (SystemVersionPath);

  if (SystemVersionData != NULL) {
    UnicodeDiskLabel = GetAppleRecoveryNameFromPlist (SystemVersionData, SystemVersionDataSize);
    FreePool (SystemVersionData);
  } else {
    UnicodeDiskLabel = NULL;
  }

  return UnicodeDiskLabel;
}

STATIC
EFI_STATUS
GetAlternateOsBooter (
  IN  EFI_HANDLE                Device,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *Root;
  UINTN                            FilePathSize;

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

  *FilePath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
                Root,
                &gAppleBlessedAlternateOsInfoGuid,
                sizeof (EFI_DEVICE_PATH_PROTOCOL),
                &FilePathSize
                );

  if (*FilePath != NULL) {
    if (!IsDevicePathValid(*FilePath, FilePathSize)) {
      FreePool (*FilePath);
      *FilePath = NULL;
      Status = EFI_NOT_FOUND;
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  Root->Close (Root);

  return Status;
}

//
// TODO: This should be less hardcoded.
//
STATIC
BOOLEAN
IsFolderBootEntry (
  IN     EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  FILEPATH_DEVICE_PATH      *FolderDevicePath;
  BOOLEAN                   IsFolder;

  IsFolder = FALSE;

  DevicePathWalker = DevicePath;
  while (!IsDevicePathEnd (DevicePathWalker)) {
    if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)) {
      FolderDevicePath = (FILEPATH_DEVICE_PATH *) DevicePathWalker;
      IsFolder = FolderDevicePath->PathName[StrLen (FolderDevicePath->PathName) - 1] == L'\\';
    } else {
      IsFolder = FALSE;
    }

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }
  return IsFolder;
}

EFI_STATUS
OcDescribeBootEntry (
  IN     APPLE_BOOT_POLICY_PROTOCOL *BootPolicy,
  IN     EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN OUT CHAR16                     **BootEntryName OPTIONAL,
  IN OUT CHAR16                     **BootPathName OPTIONAL
  )
{
  EFI_STATUS     Status;

  CHAR16                           *BootDirectoryName;
  CHAR16                           *RecoveryBootName;
  EFI_HANDLE                       Device;
  EFI_HANDLE                       ApfsVolumeHandle;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  Status = BootPolicy->GetBootInfo (
    DevicePath,
    &BootDirectoryName,
    &Device,
    &ApfsVolumeHandle
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

  if (BootEntryName != NULL) { 
    //
    // Try to use APFS-style label or legacy HFS one.
    //
    *BootEntryName = GetAppleDiskLabel (FileSystem, BootDirectoryName, L".contentDetails");
    if (*BootEntryName == NULL) {
      *BootEntryName = GetAppleDiskLabel (FileSystem, BootDirectoryName, L".disk_label.contentDetails");
    }
    if (*BootEntryName == NULL) {
      *BootEntryName = GetVolumeLabel (FileSystem);
      if (*BootEntryName != NULL && (!StrCmp (*BootEntryName, L"Recovery HD") || !StrCmp (*BootEntryName, L"Recovery"))) {
        RecoveryBootName = GetAppleRecoveryName (FileSystem, BootDirectoryName);
        if (RecoveryBootName != NULL) {
          FreePool (*BootEntryName);
          *BootEntryName = RecoveryBootName;
        }
      }
    }

    if (*BootEntryName == NULL) {
      FreePool (BootDirectoryName);
      return EFI_NOT_FOUND;
    }
  }

  if (BootPathName != NULL) {
    *BootPathName = BootDirectoryName;
  } else {
    FreePool (BootDirectoryName);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcScanForBootEntries (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Mode,
  OUT OC_BOOT_ENTRY               **BootEntries,
  OUT UINTN                       *Count
  )
{
  EFI_STATUS                Status;
  UINTN                     NoHandles;
  EFI_HANDLE                *Handles;
  UINTN                     Index;
  OC_BOOT_ENTRY             *Entries;
  UINTN                     EntryIndex;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *RecoveryPath;
  VOID                      *Reserved;
  EFI_FILE_PROTOCOL         *RecoveryRoot;
  EFI_HANDLE                RecoveryDeviceHandle;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NoHandles,
                  &Handles
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (NoHandles == 0) {
    FreePool (Handles);
    return EFI_NOT_FOUND;
  }

  Entries = AllocateZeroPool (NoHandles * 2 * sizeof (OC_BOOT_ENTRY));
  if (Entries == NULL) {
    FreePool (Handles);
    return EFI_OUT_OF_RESOURCES;
  }

  EntryIndex = 0;

  for (Index = 0; Index < NoHandles; ++Index) {
    Status = BootPolicy->GetBootFileEx (
      Handles[Index],
      APPLE_BOOT_POLICY_MODE_1,
      &DevicePath
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Entries[EntryIndex].DevicePath = DevicePath;
    Entries[EntryIndex].PrefersDmgBoot = IsFolderBootEntry (DevicePath);

    ++EntryIndex;

    Status = BootPolicy->GetPathNameOnApfsRecovery (
      DevicePath,
      L"\\",
      &RecoveryPath,
      &Reserved,
      &RecoveryRoot,
      &RecoveryDeviceHandle
      );

    if (!EFI_ERROR (Status)) {
      DevicePath = FileDevicePath (RecoveryDeviceHandle, RecoveryPath);
      FreePool (RecoveryPath);
      RecoveryRoot->Close (RecoveryRoot);
    } else {
      Status = GetAlternateOsBooter (Handles[Index], &DevicePath);
      if (EFI_ERROR (Status)) {
        continue;
      }
    }

    Entries[EntryIndex].DevicePath     = DevicePath;
    Entries[EntryIndex].PrefersDmgBoot = IsFolderBootEntry (DevicePath);

    ++EntryIndex;
  }

  FreePool (Handles);

  *BootEntries = Entries;
  *Count = EntryIndex;

  return EFI_SUCCESS;
}

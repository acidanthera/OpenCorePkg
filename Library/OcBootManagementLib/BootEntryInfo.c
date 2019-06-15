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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
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
  DEBUG ((DEBUG_INFO, "Trying to get label from %s\n", DiskLabelPath));

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
  DEBUG ((DEBUG_INFO, "Trying to get recovery from %s\n", SystemVersionPath));
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

    Status = Root->Open (Root, &Recovery, L"\\com.apple.recovery.boot\\", EFI_FILE_MODE_READ, 0);
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

//
// TODO: This should be less hardcoded.
//
VOID
InternalSetBootEntryFlags (
  IN OUT OC_BOOT_ENTRY   *BootEntry
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  FILEPATH_DEVICE_PATH      *FilePath;
  UINTN                     Len;
  UINTN                     Index;
  BOOLEAN                   Result;
  INTN                      CmpResult;

  BootEntry->IsFolder   = FALSE;
  BootEntry->IsRecovery = FALSE;
  BootEntry->IsWindows  = FALSE;

  DevicePathWalker = BootEntry->DevicePath;

  if (DevicePathWalker == NULL) {
    return;
  }

  while (!IsDevicePathEnd (DevicePathWalker)) {
    if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)) {
      FilePath = (FILEPATH_DEVICE_PATH *)DevicePathWalker;
      Len      = OcFileDevicePathNameLen (FilePath);
      if (Len > 0) {
        //
        // Only the trailer of the last (non-empty) FilePath node matters.
        //
        BootEntry->IsFolder = (FilePath->PathName[Len - 1] == L'\\');

        if (!BootEntry->IsRecovery) {
          Result = OcOverflowSubUN (
                     Len,
                     L_STR_LEN (L"com.apple.recovery.boot"),
                     &Len
                     );
          if (!Result) {
            for (Index = 0; Index < Len; ++Index) {
              CmpResult = CompareMem (
                            &FilePath->PathName[Index],
                            L"com.apple.recovery.boot",
                            L_STR_SIZE_NT (L"com.apple.recovery.boot")
                            );
              if (CmpResult == 0) {
                BootEntry->IsRecovery = TRUE;
                break;
              }
            }
          }
        }
      }
    } else {
      BootEntry->IsFolder = FALSE;
    }

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }
}

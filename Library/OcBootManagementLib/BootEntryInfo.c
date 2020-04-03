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

EFI_STATUS
InternalGetAppleDiskLabelImage (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName,
  IN  CONST CHAR16                     *LabelFilename,
  OUT VOID                             **ImageData,
  OUT UINT32                           *DataSize
  )
{
  CHAR16   *DiskLabelPath;
  UINTN    DiskLabelPathSize;

  DiskLabelPathSize = StrSize (BootDirectoryName) + StrSize (LabelFilename) - sizeof (CHAR16);
  DiskLabelPath     = AllocatePool (DiskLabelPathSize);

  if (DiskLabelPath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeSPrint (DiskLabelPath, DiskLabelPathSize, L"%s%s", BootDirectoryName, LabelFilename);
  DEBUG ((DEBUG_WARN, "Trying to get image from %s\n", DiskLabelPath));

  *ImageData = ReadFile (FileSystem, DiskLabelPath, DataSize, 10485760);

  if (*ImageData != NULL && *DataSize > 5) {
    FreePool (DiskLabelPath);
    return EFI_SUCCESS;
  }

  DEBUG((DEBUG_WARN, "File %s not found\n", DiskLabelPath));
  FreePool (DiskLabelPath);
  return EFI_NOT_FOUND;
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
            DEBUG ((DEBUG_INFO, "OCBM: Got recovery dp %s\n", DevicePathText));
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
InternalPrepareScanInfo (
  IN     APPLE_BOOT_POLICY_PROTOCOL       *BootPolicy,
  IN     OC_PICKER_CONTEXT                *Context,
  IN     EFI_HANDLE                       *Handles,
  IN     UINTN                            Index,
  IN OUT INTERNAL_DEV_PATH_SCAN_INFO      *DevPathScanInfo
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs;
  EFI_FILE_PROTOCOL                *Root;
  CHAR16                           *VolumeLabel;

  DevPathScanInfo->Device         = Handles[Index];
  DevPathScanInfo->BootDevicePath = NULL;

  Status = gBS->HandleProtocol (
    DevPathScanInfo->Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &SimpleFs
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalCheckScanPolicy (
    DevPathScanInfo->Device,
    Context->ScanPolicy,
    &DevPathScanInfo->IsExternal
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Skipping handle %p due to scan policy %x\n",
      DevPathScanInfo->Device,
      Context->ScanPolicy
      ));
    return Status;
  }

  //
  // Do not do normal scanning on load handle.
  // We only allow recovery there.
  //
  if (Context->ExcludeHandle != DevPathScanInfo->Device) {
    Status = EFI_NOT_FOUND;

    if (Context->NumCustomBootPaths > 0) {
      Status = SimpleFs->OpenVolume (SimpleFs, &Root);
      if (!EFI_ERROR (Status)) {
        Status = OcGetBooterFromPredefinedNameList (
          DevPathScanInfo->Device,
          Root,
          (CONST CHAR16 **)Context->CustomBootPaths,
          Context->NumCustomBootPaths,
          &DevPathScanInfo->BootDevicePath,
          NULL
          );

        Root->Close (Root);
      }
    }

    if (EFI_ERROR (Status)) {
      Status = BootPolicy->GetBootFileEx (
        DevPathScanInfo->Device,
        BootPolicyOk,
        &DevPathScanInfo->BootDevicePath
        );
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  //
  // Do not deal with recovery when hiding auxiliary.
  //
  DevPathScanInfo->SkipRecovery = Context->HideAuxiliary;

  //
  // This volume may still be a recovery volume.
  //
  if (EFI_ERROR (Status) && !DevPathScanInfo->SkipRecovery) {
    Status = InternalGetRecoveryOsBooter (
      DevPathScanInfo->Device,
      &DevPathScanInfo->BootDevicePath,
      TRUE
      );
    if (!EFI_ERROR (Status)) {
      DevPathScanInfo->SkipRecovery = TRUE;
    }
  }

  if (!EFI_ERROR (Status)) {
    ASSERT (DevPathScanInfo->BootDevicePath != NULL);

    DevPathScanInfo->NumBootInstances = OcGetNumDevicePathInstances (
      DevPathScanInfo->BootDevicePath
      );

    Status = gBS->HandleProtocol (
      DevPathScanInfo->Device,
      &gEfiDevicePathProtocolGuid,
      (VOID **)&DevPathScanInfo->HdDevicePath
      );
    if (EFI_ERROR (Status)) {
      FreePool (DevPathScanInfo->BootDevicePath);
      DevPathScanInfo->BootDevicePath = NULL;
    }

    DevPathScanInfo->HdPrefixSize = GetDevicePathSize (
      DevPathScanInfo->HdDevicePath
      ) - END_DEVICE_PATH_LENGTH;

  }

  DEBUG_CODE_BEGIN ();
  VolumeLabel = GetVolumeLabel (SimpleFs);
  DEBUG ((
    DEBUG_INFO,
    "OCB: Filesystem %u (%p) named %s (%r) has %u entries\n",
    (UINT32) Index,
    DevPathScanInfo->Device,
    VolumeLabel != NULL ? VolumeLabel : L"<Null>",
    Status,
    DevPathScanInfo->BootDevicePath == NULL ? 0 : (UINT32) DevPathScanInfo->NumBootInstances
    ));
  if (VolumeLabel != NULL) {
    FreePool (VolumeLabel);
  }
  DEBUG_CODE_END ();

  return Status;
}

UINTN
InternalFillValidBootEntries (
  IN     APPLE_BOOT_POLICY_PROTOCOL   *BootPolicy,
  IN     OC_PICKER_CONTEXT            *Context,
  IN     INTERNAL_DEV_PATH_SCAN_INFO  *DevPathScanInfo,
  IN     EFI_DEVICE_PATH_PROTOCOL     *DevicePathWalker,
  IN OUT OC_BOOT_ENTRY                *Entries,
  IN     UINTN                        EntryIndex
  )
{
  EFI_STATUS           Status;
  EFI_DEVICE_PATH      *DevicePath;
  UINTN                DevPathSize;
  INTN                 CmpResult;
  CHAR16               *RecoveryPath;
  VOID                 *Reserved;
  EFI_FILE_PROTOCOL    *RecoveryRoot;
  EFI_HANDLE           RecoveryDeviceHandle;

  while (TRUE) {
    DevicePath = GetNextDevicePathInstance (&DevicePathWalker, &DevPathSize);
    if (DevicePath == NULL) {
      break;
    }

    if ((DevPathSize - END_DEVICE_PATH_LENGTH) < DevPathScanInfo->HdPrefixSize) {
      FreePool (DevicePath);
      continue;
    }

    if ((Context->ScanPolicy & OC_SCAN_SELF_TRUST_LOCK) != 0) {
      CmpResult = CompareMem (
        DevicePath,
        DevPathScanInfo->HdDevicePath,
        DevPathScanInfo->HdPrefixSize
        );
      if (CmpResult != 0) {
        DEBUG ((
          DEBUG_INFO,
          "OCB: Skipping handle %p instance due to self trust violation",
          DevPathScanInfo->Device
          ));

        DebugPrintDevicePath (
          DEBUG_INFO,
          "  Disk DP",
          DevPathScanInfo->HdDevicePath
          );
        DebugPrintDevicePath (
          DEBUG_INFO,
          "  Instance DP",
          DevicePath
          );

        FreePool (DevicePath);
        continue;
      }
    }

    Entries[EntryIndex].DevicePath = DevicePath;
    Entries[EntryIndex].IsExternal = DevPathScanInfo->IsExternal;
    Entries[EntryIndex].Type = OcGetBootDevicePathType (
      Entries[EntryIndex].DevicePath,
      &Entries[EntryIndex].IsFolder
      );

    //
    // This entry can still be legacy HFS non-dmg recovery or Time Machine, ensure that it is not.
    //
    if (Context->HideAuxiliary
      && (Entries[EntryIndex].Type & (OC_BOOT_APPLE_RECOVERY | OC_BOOT_APPLE_TIME_MACHINE)) != 0) {
      ZeroMem (&Entries[EntryIndex], sizeof (Entries[EntryIndex]));
      FreePool (DevicePath);
      continue;
    }

    DEBUG ((
      DEBUG_BULK_INFO,
      "OCB: Adding entry %u, external - %d, skip recovery - %d\n",
      (UINT32) EntryIndex,
      DevPathScanInfo->IsExternal,
      DevPathScanInfo->SkipRecovery
      ));
    DebugPrintDevicePath (DEBUG_BULK_INFO, "DevicePath", DevicePath);

    ++EntryIndex;

    if (DevPathScanInfo->SkipRecovery) {
      continue;
    }

    RecoveryPath = NULL;
    Status = BootPolicy->GetApfsRecoveryFilePath (
      DevicePath,
      L"\\",
      &RecoveryPath,
      &Reserved,
      &RecoveryRoot,
      &RecoveryDeviceHandle
      );

    DEBUG ((
      DEBUG_BULK_INFO,
      "OCB: Adding entry %u, external - %u, recovery (%s) - %r\n",
      (UINT32) EntryIndex,
      DevPathScanInfo->IsExternal,
      RecoveryPath != NULL ? RecoveryPath : L"<null>",
      Status
      ));

    if (!EFI_ERROR (Status)) {
      DevicePath = FileDevicePath (RecoveryDeviceHandle, RecoveryPath);
      FreePool (RecoveryPath);
      RecoveryRoot->Close (RecoveryRoot);
    } else {
      Status = InternalGetRecoveryOsBooter (
        DevPathScanInfo->Device,
        &DevicePath,
        FALSE
        );
      if (EFI_ERROR (Status)) {
        continue;
      }
    }

    Entries[EntryIndex].DevicePath = DevicePath;
    Entries[EntryIndex].IsExternal = DevPathScanInfo->IsExternal;
    Entries[EntryIndex].Type = OcGetBootDevicePathType (
      Entries[EntryIndex].DevicePath,
      &Entries[EntryIndex].IsFolder
      );
    ++EntryIndex;
  }

  return EntryIndex;
}

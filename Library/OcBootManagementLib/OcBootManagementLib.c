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
#include <Uefi.h>

#include <Guid/AppleApfsInfo.h>
#include <Guid/AppleBless.h>
#include <Guid/FileInfo.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/OcAppleChunklistLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcXmlLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/FileHandleLib.h>

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  OC_APPLE_DISK_IMAGE_CONTEXT    DmgContext;
  EFI_HANDLE                     BlockIoHandle;
} INTERNAL_DMG_LOAD_CONTEXT;

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
    UnicodeFilterString (UnicodeDiskLabel, TRUE);
    FreePool (AsciiDiskLabel);
  } else {
    UnicodeDiskLabel = NULL;
  }

  return UnicodeDiskLabel;
}

STATIC
CHAR16 *
GetAppleRootedName (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName
  )
{
  EFI_STATUS  Status;
  CHAR16      *RootedDiskPath;
  UINTN       RootedDiskPathSize;
  EFI_GUID    RootUuid;
  CHAR8       *AsciiRootUuid;
  CHAR16      *UnicodeRootUuid;
  CHAR16      *UnicodeRootedName;
  UINT32      AsciiRootUuidLength;

  RootedDiskPathSize = StrSize (BootDirectoryName) + L_STR_SIZE_NT (L".root_uuid");
  RootedDiskPath     = AllocatePool (RootedDiskPathSize);

  if (RootedDiskPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (RootedDiskPath, RootedDiskPathSize, L"%s%a", BootDirectoryName, ".root_uuid");
  DEBUG ((DEBUG_INFO, "Trying to get root from %s\n", RootedDiskPath));
  //
  // Permit a few new lines afterwards, though I doubt Apple has them anywhere.
  //
  AsciiRootUuid = (CHAR8 *) ReadFile (FileSystem, RootedDiskPath, &AsciiRootUuidLength, GUID_STRING_LENGTH + 4);
  FreePool (RootedDiskPath);

  if (AsciiRootUuid == NULL) {
    DEBUG ((DEBUG_INFO, "Failed!\n"));
    return NULL;
  }

  if (AsciiRootUuidLength >= GUID_STRING_LENGTH) {
    UnicodeRootUuid = AsciiStrCopyToUnicode (AsciiRootUuid, GUID_STRING_LENGTH);
  }

  FreePool (AsciiRootUuid);

  if (AsciiRootUuidLength < GUID_STRING_LENGTH || UnicodeRootUuid == NULL) {
    return NULL;
  }

  Status = StrToGuid (UnicodeRootUuid, &RootUuid);
  FreePool (UnicodeRootUuid);  
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // FIXME: Obtain actual partition label once Apple Partition Info is ready.
  //
  UnicodeRootedName = AllocatePool (128);
  if (UnicodeRootedName != NULL) {
    UnicodeSPrint (UnicodeRootedName, 128, L"Blessed %g", &RootUuid);
  }

  return UnicodeRootedName;
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

STATIC
EFI_STATUS
GetRecoveryOsBooter (
  IN  EFI_HANDLE                Device,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
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
  *FilePath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
                Root,
                &gAppleBlessedOsxFolderInfoGuid,
                sizeof (EFI_DEVICE_PATH_PROTOCOL),
                &FilePathSize
                );

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
    // Ok, this one can still be FileVault 2 HFS+ recovery.
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
STATIC
VOID
SetBootEntryFlags (
  IN OUT OC_BOOT_ENTRY   *BootEntry
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  CONST CHAR16              *Path;

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
      Path   = ((FILEPATH_DEVICE_PATH *) DevicePathWalker)->PathName;
      if (Path[StrLen (Path) - 1] == L'\\') {
        BootEntry->IsFolder = TRUE;
      }
      if (StrStr (Path, L"com.apple.recovery.boot") != NULL) {
        BootEntry->IsRecovery = TRUE;
      }
    } else {
      BootEntry->IsFolder = FALSE;
    }

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }
}

EFI_STATUS
OcDescribeBootEntry (
  IN     APPLE_BOOT_POLICY_PROTOCOL *BootPolicy,
  IN OUT OC_BOOT_ENTRY              *BootEntry
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *BootDirectoryName;
  CHAR16                           *RecoveryBootName;
  EFI_HANDLE                       Device;
  EFI_HANDLE                       ApfsVolumeHandle;
  UINT32                           BcdSize;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  Status = BootPolicy->GetBootInfo (
    BootEntry->DevicePath,
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

  //
  // Try to use APFS-style label or legacy HFS one.
  //
  BootEntry->Name = GetAppleDiskLabel (FileSystem, BootDirectoryName, L".contentDetails");
  if (BootEntry->Name == NULL) {
    BootEntry->Name = GetAppleDiskLabel (FileSystem, BootDirectoryName, L".disk_label.contentDetails");
  }

  //
  // With FV2 encryption on HFS+ the actual boot happens from "Recovery HD/S/L/CoreServices".
  // For some reason "Recovery HD/S/L/CoreServices/.disk_label" may not get updated immediately,
  // and will contain "Recovery HD" despite actually pointing to "Macintosh HD".
  // In this case we should prioritise .root_uuid, which contains real partition UUID in ASCII.
  // TODO: I *think* later we can use this by default, but only when GetAppleRootedName
  // starts to return proper volume label.
  //
  if (BootEntry->Name != NULL && !BootEntry->IsRecovery && StrCmp (BootEntry->Name, L"Recovery HD") == 0) {
    RecoveryBootName = GetAppleRootedName (FileSystem, BootDirectoryName);
    if (RecoveryBootName != NULL) {
      FreePool (BootEntry->Name);
      BootEntry->Name = RecoveryBootName;
    }
  }

  //
  // Windows boot entry may have a custom name, so ensure IsWindows is set correctly.
  //
  DEBUG ((DEBUG_INFO, "Trying to detect Microsoft BCD\n"));
  Status = ReadFileSize (FileSystem, L"\\EFI\\Microsoft\\Boot\\BCD", &BcdSize);
  if (!EFI_ERROR (Status)) {
    BootEntry->IsWindows = TRUE;
    if (BootEntry->Name == NULL) {
      BootEntry->Name      = AllocateCopyPool(sizeof (L"BOOTCAMP Windows"), L"BOOTCAMP Windows");
    }
  }

  if (BootEntry->Name == NULL) {
    BootEntry->Name = GetVolumeLabel (FileSystem);
    if (BootEntry->Name != NULL 
      && (!StrCmp (BootEntry->Name, L"Recovery HD")
       || !StrCmp (BootEntry->Name, L"Recovery"))) {
      BootEntry->IsRecovery = TRUE;
      RecoveryBootName = GetAppleRecoveryName (FileSystem, BootDirectoryName);
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

VOID
OcResetBootEntry (
  IN OUT OC_BOOT_ENTRY              *BootEntry
  )
{
  if (BootEntry->DevicePath != NULL) {
    FreePool (BootEntry->DevicePath);
    BootEntry->DevicePath = NULL;
  }

  if (BootEntry->Name != NULL) {
    FreePool (BootEntry->Name);
    BootEntry->Name = NULL;
  }

  if (BootEntry->PathName != NULL) {
    FreePool (BootEntry->PathName);
    BootEntry->PathName = NULL;
  }
}

VOID
OcFreeBootEntries (
  IN OUT OC_BOOT_ENTRY              *BootEntries,
  IN     UINTN                      Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; ++Index) {
    OcResetBootEntry (&BootEntries[Index]);
  }

  FreePool (BootEntries);
}

UINTN
OcFillBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  IN  EFI_HANDLE                  Handle,
  OUT OC_BOOT_ENTRY               *BootEntry,
  OUT OC_BOOT_ENTRY               *AlternateBootEntry OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *RecoveryPath;
  VOID                      *Reserved;
  EFI_FILE_PROTOCOL         *RecoveryRoot;
  EFI_HANDLE                RecoveryDeviceHandle;

  Status = BootPolicy->GetBootFileEx (
    Handle,
    APPLE_BOOT_POLICY_MODE_1,
    &DevicePath
    );

  if (EFI_ERROR (Status)) {
    return 0;
  }

  BootEntry->DevicePath = DevicePath;
  SetBootEntryFlags (BootEntry);

  if (AlternateBootEntry != NULL) {
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
      Status = GetRecoveryOsBooter (Handle, &DevicePath);
      if (EFI_ERROR (Status)) {
        return 1;
      }
    }

    AlternateBootEntry->DevicePath = DevicePath;
    SetBootEntryFlags (AlternateBootEntry);
    return 2;
  }

  return 1;
}

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetFirstDeviceBootFilePath (
  IN APPLE_BOOT_POLICY_PROTOCOL      *BootPolicy,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DmgDevicePath,
  IN UINTN                           DmgDevicePathSize
  )
{
  EFI_DEVICE_PATH_PROTOCOL       *BootDevicePath;

  EFI_STATUS                     Status;
  INTN                           CmpResult;

  CONST EFI_DEVICE_PATH_PROTOCOL *FsDevicePath;
  UINTN                          FsDevicePathSize;

  UINTN                          NumHandles;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          Index;

  ASSERT (BootPolicy != NULL);
  ASSERT (DmgDevicePath != NULL);
  ASSERT (DmgDevicePathSize >= END_DEVICE_PATH_LENGTH);

  BootDevicePath = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < NumHandles; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&FsDevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    FsDevicePathSize = GetDevicePathSize (FsDevicePath);

    if (FsDevicePathSize < DmgDevicePathSize) {
      continue;
    }

    CmpResult = CompareMem (
                  FsDevicePath,
                  DmgDevicePath,
                  (DmgDevicePathSize - END_DEVICE_PATH_LENGTH)
                  );
    if (CmpResult != 0) {
      continue;
    }

    Status = BootPolicy->GetBootFileEx (
                           HandleBuffer[Index],
                           APPLE_BOOT_POLICY_MODE_1,
                           &BootDevicePath
                           );
    if (!EFI_ERROR (Status)) {
      break;
    }

    BootDevicePath = NULL;
  }

  FreePool (HandleBuffer);

  return BootDevicePath;
}

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetDiskImageBootFile (
  OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  IN  VOID                        *DmgBuffer,
  IN  UINTN                       DmgBufferSize,
  IN  VOID                        *ChunklistBuffer OPTIONAL,
  IN  UINT32                      ChunklistBufferSize OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL       *DevPath;

  BOOLEAN                        Result;
  OC_APPLE_CHUNKLIST_CONTEXT     ChunklistContext;

  CONST EFI_DEVICE_PATH_PROTOCOL *DmgDevicePath;
  UINTN                          DmgDevicePathSize;

  ASSERT (Context != NULL);
  ASSERT (BootPolicy != NULL);
  ASSERT (DmgBuffer != NULL);
  ASSERT (DmgBufferSize > 0);

  Result = OcAppleDiskImageInitializeContext (
             &Context->DmgContext,
             DmgBuffer,
             DmgBufferSize,
             FALSE
             );
  if (!Result) {
    return NULL;
  }

  if (ChunklistBuffer == NULL) {
    if ((Policy & OC_LOAD_REQUIRE_APPLE_SIGN) != 0) {
      OcAppleDiskImageFreeContext (&Context->DmgContext);
      return NULL;
    }
  } else if ((Policy & (OC_LOAD_VERIFY_APPLE_SIGN | OC_LOAD_REQUIRE_TRUSTED_KEY)) != 0) {
    ASSERT (ChunklistBufferSize > 0);

    Result = OcAppleChunklistInitializeContext (
                &ChunklistContext,
                ChunklistBuffer,
                ChunklistBufferSize
                );
    if (!Result) {
      OcAppleDiskImageFreeContext (&Context->DmgContext);
      return NULL;
    }

    if ((Policy & OC_LOAD_REQUIRE_TRUSTED_KEY) != 0) {
      Result = FALSE;
      //
      // FIXME: Properly abstract OcAppleKeysLib.
      //
      if ((Policy & OC_LOAD_TRUST_APPLE_V1_KEY) != 0) {
        Result = OcAppleChunklistVerifySignature (
                   &ChunklistContext,
                   (RSA_PUBLIC_KEY *)&PkDataBase[0].PublicKey
                   );
      }

      if (!Result && ((Policy & OC_LOAD_TRUST_APPLE_V2_KEY) != 0)) {
        Result = OcAppleChunklistVerifySignature (
                   &ChunklistContext,
                   (RSA_PUBLIC_KEY *)&PkDataBase[1].PublicKey
                   );
      }

      if (!Result) {
        OcAppleDiskImageFreeContext (&Context->DmgContext);
        return NULL;
      }
    }

    Result = OcAppleDiskImageVerifyData (
               &Context->DmgContext,
               &ChunklistContext
               );
    if (!Result) {
      //
      // FIXME: Warn user instead of aborting when OC_LOAD_REQUIRE_TRUSTED_KEY
      //        is not set.
      //
      OcAppleDiskImageFreeContext (&Context->DmgContext);
      return NULL;
    }
  }

  Context->BlockIoHandle = OcAppleDiskImageInstallBlockIo (
                             &Context->DmgContext,
                             &DmgDevicePath,
                             &DmgDevicePathSize
                             );
  if (Context->BlockIoHandle == NULL) {
    OcAppleDiskImageFreeContext (&Context->DmgContext);
    return NULL;
  }

  DevPath = InternalGetFirstDeviceBootFilePath (
              BootPolicy,
              DmgDevicePath,
              DmgDevicePathSize
              );
  if (DevPath != NULL) {
    return DevPath;
  }

  OcAppleDiskImageUninstallBlockIo (
    &Context->DmgContext,
    Context->BlockIoHandle
    );
  OcAppleDiskImageFreeContext (&Context->DmgContext);

  return NULL;
}

STATIC
EFI_FILE_INFO *
InternalFindFirstDmgFileName (
  IN  EFI_FILE_PROTOCOL  *Directory,
  OUT UINTN              *FileNameLen
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *FileInfo;
  BOOLEAN       NoFile;
  UINTN         ExtOffset;
  INTN          Result;

  ASSERT (Directory != NULL);

  for (
    Status = FileHandleFindFirstFile (Directory, &FileInfo), NoFile = FALSE;
    (!EFI_ERROR (Status) && !NoFile);
    Status = FileHandleFindNextFile (Directory, FileInfo, &NoFile)
    ) {
    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
      continue;
    }

    ExtOffset = (StrLen (FileInfo->FileName) - L_STR_LEN (L".dmg"));
    Result    = StrCmp (&FileInfo->FileName[ExtOffset], L".dmg");
    if (Result == 0) {
      if (FileNameLen != NULL) {
        *FileNameLen = ExtOffset;
      }

      return FileInfo;
    }
  }

  return NULL;
}

STATIC
EFI_FILE_INFO *
InternalFindDmgChunklist (
  IN EFI_FILE_PROTOCOL  *Directory,
  IN CONST CHAR16       *DmgFileName,
  IN UINTN              DmgFileNameLen
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *FileInfo;
  BOOLEAN       NoFile;
  UINTN         NameLen;
  INTN          Result;
  UINTN         ChunklistFileNameLen;

  ChunklistFileNameLen = (DmgFileNameLen + L_STR_LEN (".chunklist"));

  for (
    Status = FileHandleFindFirstFile (Directory, &FileInfo), NoFile = FALSE;
    (!EFI_ERROR (Status) && !NoFile);
    Status = FileHandleFindNextFile (Directory, FileInfo, &NoFile)
    ) {
    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
      continue;
    }

    NameLen = StrLen (FileInfo->FileName);

    if (NameLen != ChunklistFileNameLen) {
      continue;
    }

    Result = StrnCmp (FileInfo->FileName, DmgFileName, DmgFileNameLen);
    if (Result != 0) {
      continue;
    }

    Result = StrCmp (&FileInfo->FileName[DmgFileNameLen], L".chunklist");
    if (Result == 0) {
      return FileInfo;
    }
  }

  return NULL;
}

/**
  FIXME: Remove this once EfiOpenFileByDevicePath lands into UDK.
  https://github.com/tianocore/edk2/commit/768b611136d0f2b99a99e446c089d1a30c3fa5d5
**/
EFI_STATUS
EFIAPI
OcOpenFileByDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  OUT    EFI_FILE_PROTOCOL         **File,
  IN     UINT64                    OpenMode,
  IN     UINT64                    Attributes
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      FileSystemHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *LastFile;
  FILEPATH_DEVICE_PATH            *FilePathNode;
  CHAR16                          *AlignedPathName;
  CHAR16                          *PathName;
  EFI_FILE_PROTOCOL               *NextFile;

  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *File = NULL;

  if (FilePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Look up the filesystem.
  //
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  FilePath,
                  &FileSystemHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = gBS->OpenProtocol (
                  FileSystemHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the root directory of the filesystem. After this operation succeeds,
  // we have to release LastFile on error.
  //
  Status = FileSystem->OpenVolume (FileSystem, &LastFile);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Traverse the device path nodes relative to the filesystem.
  //
  while (!IsDevicePathEnd (*FilePath)) {
    if (DevicePathType (*FilePath) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (*FilePath) != MEDIA_FILEPATH_DP) {
      Status = EFI_INVALID_PARAMETER;
      goto CloseLastFile;
    }
    FilePathNode = (FILEPATH_DEVICE_PATH *)*FilePath;

    //
    // FilePathNode->PathName may be unaligned, and the UEFI specification
    // requires pointers that are passed to protocol member functions to be
    // aligned. Create an aligned copy of the pathname if necessary.
    //
    if ((UINTN)FilePathNode->PathName % sizeof *FilePathNode->PathName == 0) {
      AlignedPathName = NULL;
      PathName = FilePathNode->PathName;
    } else {
      AlignedPathName = AllocateCopyPool (
                          (DevicePathNodeLength (FilePathNode) -
                           SIZE_OF_FILEPATH_DEVICE_PATH),
                          FilePathNode->PathName
                          );
      if (AlignedPathName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto CloseLastFile;
      }
      PathName = AlignedPathName;
    }

    //
    // Open or create the file corresponding to the next pathname fragment.
    //
    Status = LastFile->Open (
                         LastFile,
                         &NextFile,
                         PathName,
                         OpenMode,
                         Attributes
                         );

    //
    // Release any AlignedPathName on both error and success paths; PathName is
    // no longer needed.
    //
    if (AlignedPathName != NULL) {
      FreePool (AlignedPathName);
    }
    if (EFI_ERROR (Status)) {
      goto CloseLastFile;
    }

    //
    // Advance to the next device path node.
    //
    LastFile->Close (LastFile);
    LastFile = NextFile;
    *FilePath = NextDevicePathNode (FilePathNode);
  }

  *File = LastFile;
  return EFI_SUCCESS;

CloseLastFile:
  LastFile->Close (LastFile);

  //
  // We are on the error path; we must have set an error Status for returning
  // to the caller.
  //
  ASSERT (EFI_ERROR (Status));
  return Status;
}

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalLoadDmg (
  IN OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN     APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN     UINT32                      Policy
  )
{
  EFI_DEVICE_PATH_PROTOCOL *DevPath;

  EFI_STATUS               Status;

  EFI_FILE_PROTOCOL        *DmgDir;

  UINTN                    DmgFileNameLen;
  EFI_FILE_INFO            *DmgFileInfo;
  EFI_FILE_PROTOCOL        *DmgFile;
  UINT32                   DmgFileSize;
  VOID                     *DmgBuffer;

  EFI_FILE_INFO            *ChunklistFileInfo;
  EFI_FILE_PROTOCOL        *ChunklistFile;
  UINT32                   ChunklistFileSize;
  VOID                     *ChunklistBuffer;

  ASSERT (Context != NULL);
  ASSERT (BootPolicy != NULL);

  Status = OcOpenFileByDevicePath (
             &Context->DevicePath,
             &DmgDir,
             EFI_FILE_MODE_READ,
             EFI_FILE_DIRECTORY
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DmgFileInfo = InternalFindFirstDmgFileName (DmgDir, &DmgFileNameLen);
  if (DmgFileInfo == NULL) {
    DmgDir->Close (DmgDir);
    return NULL;
  }

  Status = DmgDir->Open (
                     DmgDir,
                     &DmgFile,
                     DmgFileInfo->FileName,
                     EFI_FILE_MODE_READ,
                     0
                     );
  if (EFI_ERROR (Status)) {
    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    return NULL;
  }

  DmgFileSize = 0;
  Status = GetFileSize (DmgFile, &DmgFileSize);

  if (Status != EFI_SUCCESS) {
    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    DmgFile->Close (DmgFile);
    return NULL;
  }

  DmgBuffer = OcAppleDiskImageAllocateBuffer (DmgFileSize);
  if (DmgBuffer == NULL) {
    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    DmgFile->Close (DmgFile);
    return NULL;
  }

  Status = GetFileData (DmgFile, 0, DmgFileSize, DmgBuffer);

  DmgFile->Close (DmgFile);

  if (EFI_ERROR (Status)) {
    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    OcAppleDiskImageFreeBuffer (DmgBuffer, DmgFileSize);
    return NULL;
  }

  ChunklistBuffer   = NULL;
  ChunklistFileSize = 0;

  ChunklistFileInfo = InternalFindDmgChunklist (
                        DmgDir,
                        DmgFileInfo->FileName,
                        DmgFileNameLen
                        );
  if (ChunklistFileInfo != NULL) {
    Status = DmgDir->Open (
                       DmgDir,
                       &ChunklistFile,
                       ChunklistFileInfo->FileName,
                       EFI_FILE_MODE_READ,
                       0
                       );
    if (!EFI_ERROR (Status)) {
      Status = GetFileSize (ChunklistFile, &ChunklistFileSize);
      if (Status == EFI_SUCCESS) {
        ChunklistBuffer = AllocatePool (ChunklistFileSize);

        if (ChunklistBuffer == NULL) {
          ChunklistFileSize = 0;
        } else {
          Status = GetFileData (ChunklistFile, 0, ChunklistFileSize, ChunklistBuffer);
          if (EFI_ERROR (Status)) {
            FreePool (ChunklistBuffer);
            ChunklistBuffer   = NULL;
            ChunklistFileSize = 0;
          }
        }
      }

      ChunklistFile->Close (ChunklistFile);
    }

    FreePool (ChunklistFileInfo);
  }

  FreePool (DmgFileInfo);

  DmgDir->Close (DmgDir);

  DevPath = InternalGetDiskImageBootFile (
              Context,
              BootPolicy,
              Policy,
              DmgBuffer,
              DmgFileSize,
              ChunklistBuffer,
              ChunklistFileSize
              );
  Context->DevicePath = DevPath;

  if (DevPath == NULL) {
    OcAppleDiskImageFreeBuffer (DmgBuffer, DmgFileSize);
  }

  if (ChunklistBuffer != NULL) {
    FreePool (ChunklistBuffer);
  }

  return DevPath;
}

VOID
InternalUnloadDmg (
  IN INTERNAL_DMG_LOAD_CONTEXT  *DmgLoadContext
  )
{
  FreePool (DmgLoadContext->DevicePath);
  OcAppleDiskImageUninstallBlockIo (
    &DmgLoadContext->DmgContext,
    DmgLoadContext->BlockIoHandle
    );
  OcAppleDiskImageFreeContextAndBuffer (&DmgLoadContext->DmgContext);
}

EFI_STATUS
OcScanForBootEntries (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  OUT OC_BOOT_ENTRY               **BootEntries,
  OUT UINTN                       *Count,
  OUT UINTN                       *AllocCount OPTIONAL,
  IN  EFI_HANDLE                  LoadHandle  OPTIONAL,
  IN  BOOLEAN                     Describe
  )
{
  EFI_STATUS                Status;
  UINTN                     NoHandles;
  EFI_HANDLE                *Handles;
  UINTN                     Index;
  OC_BOOT_ENTRY             *Entries;
  UINTN                     EntryIndex;
  CHAR16                    *DevicePath;
  UINTN                     EntryCount;

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
    EntryCount = OcFillBootEntry (
      BootPolicy,
      Policy,
      Handles[Index],
      &Entries[EntryIndex],
      &Entries[EntryIndex+1]
      );

    if (LoadHandle == Handles[Index] && EntryCount > 0) {
      DEBUG ((DEBUG_INFO, "Skipping self load handle entry %p\n", LoadHandle));
      --EntryCount;
      CopyMem (
        &Entries[EntryIndex+1],
        &Entries[EntryIndex],
        sizeof (Entries[EntryIndex]) * EntryCount
        );
    }

    EntryIndex += EntryCount;
  }

  FreePool (Handles);

  if (Describe) {
    DEBUG ((DEBUG_INFO, "Scanning got %u entries\n", (UINT32) EntryIndex));

    for (Index = 0; Index < EntryIndex; ++Index) {
      Status = OcDescribeBootEntry (BootPolicy, &Entries[Index]);
      if (EFI_ERROR (Status)) {
        break;
      }

      DEBUG ((
        DEBUG_INFO,
        "Entry %u is %s at %s (W:%d|R:%d|F:%d)\n",
        (UINT32) Index,
        Entries[Index].Name,
        Entries[Index].PathName,
        Entries[Index].IsWindows,
        Entries[Index].IsRecovery,
        Entries[Index].IsFolder
        ));

      DevicePath = ConvertDevicePathToText(Entries[Index].DevicePath, FALSE, FALSE);
      if (DevicePath != NULL) {
        DEBUG ((
          DEBUG_INFO,
          "Entry %u is %s at dp %s\n",
          (UINT32) Index,
          Entries[Index].Name,
          DevicePath
          ));
        FreePool (DevicePath);
      }
    }

    if (EFI_ERROR (Status)) {
      OcFreeBootEntries (Entries, EntryIndex);
      return Status;
    }
  }

  *BootEntries = Entries;
  *Count       = EntryIndex;

  if (AllocCount != NULL) {
    *AllocCount = NoHandles * 2;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcShowSimpleBootMenu (
  IN OC_BOOT_ENTRY                *BootEntries,
  IN UINTN                        Count,
  IN UINTN                        DefaultEntry,
  IN UINTN                        TimeOutSeconds,
  OUT OC_BOOT_ENTRY               **ChosenBootEntry
  )
{
  UINTN   Index;
  INTN    KeyIndex;
  CHAR16  Code[2];

  Code[1] = '\0';

  while (TRUE) {
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->OutputString (gST->ConOut, L"OpenCore Boot Menu\r\n\r\n");

    for (Index = 0; Index < MIN (Count, OC_INPUT_MAX); ++Index) {
      Code[0] = OC_INPUT_STR[Index];
      gST->ConOut->OutputString (gST->ConOut, DefaultEntry == Index && TimeOutSeconds > 0 ? L"* " : L"  ");
      gST->ConOut->OutputString (gST->ConOut, Code);
      gST->ConOut->OutputString (gST->ConOut, L". ");
      gST->ConOut->OutputString (gST->ConOut, BootEntries[Index].Name);
      if (BootEntries[Index].IsFolder) {
        gST->ConOut->OutputString (gST->ConOut, L" (dmg)");
      }
      gST->ConOut->OutputString (gST->ConOut, L"\r\n");
    }

    if (Index < Count) {
      gST->ConOut->OutputString (gST->ConOut, L"WARN: Some entries were skipped!\r\n");
    }

    gST->ConOut->OutputString (gST->ConOut, L"\r\nChoose boot entry: ");

    while (TRUE) {
      KeyIndex = WaitForKeyIndex (TimeOutSeconds);
      if (KeyIndex == OC_INPUT_TIMEOUT) {
        *ChosenBootEntry = &BootEntries[DefaultEntry];
        gST->ConOut->OutputString (gST->ConOut, L"Timeout\r\n");
        return EFI_SUCCESS;
      } else if (KeyIndex == OC_INPUT_ABORTED) {
        gST->ConOut->OutputString (gST->ConOut, L"Aborted\r\n");
        return EFI_ABORTED;
      } else if (KeyIndex != OC_INPUT_INVALID && (UINTN)KeyIndex < Count) {
	    ASSERT (KeyIndex >= 0);
        *ChosenBootEntry = &BootEntries[KeyIndex];
        Code[0] = OC_INPUT_STR[KeyIndex];
        gST->ConOut->OutputString (gST->ConOut, Code);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        return EFI_SUCCESS;
      }

      if (TimeOutSeconds > 0) {
        TimeOutSeconds = 0;
        break;
      }
    }
  }

  ASSERT (FALSE);
}

EFI_STATUS
OcLoadBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  UINT32                      Policy,
  IN  EFI_HANDLE                  ParentHandle,
  OUT EFI_HANDLE                  *EntryHandle
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  INTERNAL_DMG_LOAD_CONTEXT  DmgLoadContext;
  CHAR16                     *UnicodeDevicePath;

  //
  // TODO: support Apple loaded image, policy, and dmg boot.
  //

  if (BootEntry->IsFolder) {
    if ((Policy & OC_LOAD_ALLOW_DMG_BOOT) == 0) {
      return EFI_SECURITY_VIOLATION;
    }

    DmgLoadContext.DevicePath = BootEntry->DevicePath;
    DevicePath = InternalLoadDmg (
                   &DmgLoadContext,
                   BootPolicy,
                   Policy
                   );
    if (DevicePath == NULL) {
      return EFI_UNSUPPORTED;
    }

    UnicodeDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
    if (UnicodeDevicePath != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "Dmg boot %s to dp %s\n",
        BootEntry->Name,
        UnicodeDevicePath
        ));
      FreePool (UnicodeDevicePath);
    }

  } else {
    DevicePath = BootEntry->DevicePath;
  }

  Status = gBS->LoadImage (FALSE, ParentHandle, DevicePath, NULL, 0, EntryHandle);

  //
  // Please note that you cannot unload dmg after loading, as we still are booting from it.
  //

  return Status;
}

EFI_STATUS
OcRunSimpleBootPicker (
  IN  UINT32           LookupPolicy,
  IN  UINT32           BootPolicy,
  IN  UINT32           TimeoutSeconds,
  IN  OC_IMAGE_START   StartImage,
  IN  BOOLEAN          ShowPicker,
  IN  EFI_HANDLE       LoadHandle  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  APPLE_BOOT_POLICY_PROTOCOL  *AppleBootPolicy;
  OC_BOOT_ENTRY               *Chosen;
  OC_BOOT_ENTRY               *Entries;
  UINTN                       EntryCount;
  EFI_HANDLE                  BooterHandle;
  UINT32                      DefaultEntry;

  AppleBootPolicy = OcAppleBootPolicyInstallProtocol (FALSE);
  if (AppleBootPolicy == NULL) {
    DEBUG ((DEBUG_ERROR, "AppleBootPolicy locate failure\n"));
    return EFI_NOT_FOUND;
  }

  while (TRUE) {
    DEBUG ((DEBUG_INFO, "Performing OcScanForBootEntries...\n"));

    Status = OcScanForBootEntries (
      AppleBootPolicy,
      LookupPolicy,
      &Entries,
      &EntryCount,
      NULL,
      LoadHandle,
      TRUE
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OcScanForBootEntries failure - %r\n", Status));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "Performing OcShowSimpleBootMenu...\n"));

    //
    // TODO: obtain default entry!
    //
    DefaultEntry = 0;

    if (ShowPicker) {
      Status = OcShowSimpleBootMenu (
        Entries,
        EntryCount,
        DefaultEntry,
        TimeoutSeconds,
        &Chosen
        );
    } else {
      Chosen = &Entries[DefaultEntry];
      Status = EFI_SUCCESS;
    }

    if (EFI_ERROR (Status) && Status != EFI_ABORTED) {
      DEBUG ((DEBUG_ERROR, "OcShowSimpleBootMenu failed - %r\n", Status));
      OcFreeBootEntries (Entries, EntryCount);
      return Status;
    }

    TimeoutSeconds = 0;

    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "Should boot from %s (W:%d|R:%d|F:%d)\n",
        Chosen->Name,
        Chosen->IsWindows,
        Chosen->IsRecovery,
        Chosen->IsFolder
        ));
    }

    if (!EFI_ERROR (Status)) {
      Status = OcLoadBootEntry (AppleBootPolicy, Chosen, BootPolicy, gImageHandle, &BooterHandle);
      if (!EFI_ERROR (Status)) {
        Status = StartImage (Chosen, BooterHandle, NULL, NULL);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "StartImage failed - %r\n", Status));
        }
      } else {
        DEBUG ((DEBUG_ERROR, "LoadImage failed - %r\n", Status));
      }

      gBS->Stall (5000000);
    }

    OcFreeBootEntries (Entries, EntryCount);
  }
}

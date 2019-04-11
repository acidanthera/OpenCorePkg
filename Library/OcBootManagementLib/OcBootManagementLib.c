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
  AsciiDiskLabel = (CHAR8 *) ReadFile (FileSystem, DiskLabelPath, &DiskLabelLength);
  FreePool (DiskLabelPath);

  if (AsciiDiskLabel != NULL) {
    UnicodeDiskLabel = AsciiStrCopyToUnicode (AsciiDiskLabel, DiskLabelLength);
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
  AsciiRootUuid = (CHAR8 *) ReadFile (FileSystem, RootedDiskPath, &AsciiRootUuidLength);
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
  UINT32   SystemVersionDataSize;

  SystemVersionPathSize = StrSize (BootDirectoryName) + L_STR_SIZE_NT (L"SystemVersion.plist");
  SystemVersionPath     = AllocatePool (SystemVersionPathSize);

  if (SystemVersionPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (SystemVersionPath, SystemVersionPathSize, L"%sSystemVersion.plist", BootDirectoryName);
  DEBUG ((DEBUG_INFO, "Trying to get recovery from %s\n", SystemVersionPath));
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
      // This entry should point to a folder with recovery.
      // Apple never adds trailing slashes to blessed folder paths.
      // However, we do rely on trailing slashes in folder paths and add them here.
      //
      TmpPath = TrailedBooterDevicePath (*FilePath);
      if (TmpPath != NULL) {
        FreePool (*FilePath);
        *FilePath = TmpPath;
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

  if (BootEntry->Name == NULL) {
    DEBUG ((DEBUG_INFO, "Trying to detect Microsoft BCD\n"));
    Status = ReadFileSize (FileSystem, L"\\EFI\\Microsoft\\Boot\\BCD", &BcdSize);
    if (!EFI_ERROR (Status)) {
      BootEntry->IsWindows = TRUE;
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

  //
  // We skip alternate entry when current one is actually a recovery.
  // This is to prevent recovery duplicates on HFS+ systems.
  //
  if (AlternateBootEntry != NULL && !BootEntry->IsRecovery) {
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
BOOLEAN
InternalBootAppleDiskImage (
  IN APPLE_BOOT_POLICY_PROTOCOL      *BootPolicy,
  IN UINT32                          Policy,
  IN OC_IMAGE_START                  StartImage,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DmgDevicePath,
  IN UINTN                           DmgDevicePathSize
  )
{
  BOOLEAN                        Result;

  EFI_STATUS                     Status;
  INTN                           CmpResult;

  UINTN                          NumBootEntries;
  OC_BOOT_ENTRY                  BootEntry;
  EFI_HANDLE                     BooterHandle;

  CONST EFI_DEVICE_PATH_PROTOCOL *FsDevicePath;
  UINTN                          FsDevicePathSize;

  UINTN                          NumHandles;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          Index;

  Result = FALSE;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  for (Index = 0; Index < NumHandles; ++Index) {
    Status = gBS->HandleProtocol (
                    &HandleBuffer[Index],
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

    NumBootEntries = OcFillBootEntry (
                       BootPolicy,
                       Policy,
                       &HandleBuffer[Index],
                       &BootEntry,
                       NULL
                       );
    if (NumBootEntries == 0) {
      continue;
    }

    Status = OcLoadBootEntry (
               &BootEntry,
               Policy,
               gImageHandle,
               &BooterHandle
               );
    if (!EFI_ERROR (Status)) {
      Status = StartImage (&BootEntry, BooterHandle, NULL, NULL);
      if (!EFI_ERROR (Status)) {
        Result = TRUE;
      } else {
        DEBUG ((DEBUG_ERROR, "StartImage failed - %r\n", Status));
      }
    } else {
      DEBUG ((DEBUG_ERROR, "LoadImage failed - %r\n", Status));
    }

    break;
  }

  FreePool (HandleBuffer);

  return Result;
}

BOOLEAN
OcLoadAppleDiskImage (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  IN  OC_IMAGE_START              StartImage,
  IN  VOID                        *DmgBuffer,
  IN  UINTN                       DmgBufferSize,
  IN  VOID                        *ChunklistBuffer OPTIONAL,
  IN  UINT32                      ChunklistBufferSize OPTIONAL
  )
{
  BOOLEAN                        Result;

  OC_APPLE_DISK_IMAGE_CONTEXT    DmgContext;
  OC_APPLE_CHUNKLIST_CONTEXT     ChunklistContext;
  EFI_HANDLE                     BlockIoHandle;

  CONST EFI_DEVICE_PATH_PROTOCOL *DmgDevicePath;
  UINTN                          DmgDevicePathSize;

  ASSERT (BootPolicy != NULL);
  ASSERT (StartImage != NULL);
  ASSERT (DmgBuffer != NULL);
  ASSERT (DmgBufferSize > 0);

  if ((Policy & OC_LOAD_ALLOW_DMG_BOOT) == 0) {
    return FALSE;
  }

  Result = OcAppleDiskImageInitializeContext (
             &DmgContext,
             DmgBuffer,
             DmgBufferSize,
             FALSE
             );
  if (!Result) {
    return FALSE;
  }

  if (ChunklistBuffer == NULL) {
    if ((Policy & OC_LOAD_REQUIRE_APPLE_SIGN) != 0) {
      Result = FALSE;
      goto Done;
    }
  } else if ((Policy & (OC_LOAD_VERIFY_APPLE_SIGN | OC_LOAD_REQUIRE_TRUSTED_KEY)) != 0) {
    ASSERT (ChunklistBufferSize > 0);

    Result = OcAppleChunklistInitializeContext (
                &ChunklistContext,
                ChunklistBuffer,
                ChunklistBufferSize
                );
    if (!Result) {
      goto Done;
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
        goto Done;
      }
    }

    Result = OcAppleDiskImageVerifyData (&DmgContext, &ChunklistContext);
    if (!Result) {
      //
      // FIXME: Warn user instead of aborting when OC_LOAD_REQUIRE_TRUSTED_KEY
      //        is not set.
      //
      goto Done;
    }
  }

  BlockIoHandle = OcAppleDiskImageInstallBlockIo (
                    &DmgContext,
                    &DmgDevicePath,
                    &DmgDevicePathSize
                    );
  if (BlockIoHandle == NULL) {
    Result = FALSE;
    goto Done;
  }

  Result = InternalBootAppleDiskImage (
             BootPolicy,
             Policy,
             StartImage,
             DmgDevicePath,
             DmgDevicePathSize
             );

  //
  // If we reach here, booting from the DMG has been unsuccessful.
  //

  OcAppleDiskImageUninstallBlockIo (&DmgContext, BlockIoHandle);

Done:
  OcAppleDiskImageFreeContext (&DmgContext);

  return Result;
}

EFI_STATUS
OcScanForBootEntries (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  OUT OC_BOOT_ENTRY               **BootEntries,
  OUT UINTN                       *Count,
  OUT UINTN                       *AllocCount OPTIONAL,
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
    EntryIndex += OcFillBootEntry (
      BootPolicy,
      Policy,
      Handles[Index],
      &Entries[EntryIndex],
      &Entries[EntryIndex+1]
      );
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
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  UINT32                      Policy,
  IN  EFI_HANDLE                  ParentHandle,
  OUT EFI_HANDLE                  *EntryHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  //
  // TODO: support Apple loaded image, policy, and dmg boot.
  //

  if (BootEntry->IsFolder) {
    DevicePath = AppendFileNameDevicePath (BootEntry->DevicePath, L"boot.efi");
    DEBUG ((DEBUG_WARN, "HACK! HACK! HACK! Using boot.efi for dmg boot!\n"));
  } else {
    DevicePath = DuplicateDevicePath (BootEntry->DevicePath);
  }

  Status = gBS->LoadImage (FALSE, ParentHandle, DevicePath, NULL, 0, EntryHandle);

  FreePool (DevicePath);

  return Status;
}

EFI_STATUS
OcRunSimpleBootMenu (
  IN  UINT32           LookupPolicy,
  IN  UINT32           BootPolicy,
  IN  UINT32           TimeoutSeconds,
  IN  OC_IMAGE_START   StartImage
  )
{
  EFI_STATUS                  Status;
  APPLE_BOOT_POLICY_PROTOCOL  *AppleBootPolicy;
  OC_BOOT_ENTRY               *Chosen;
  OC_BOOT_ENTRY               *Entries;
  UINTN                       EntryCount;
  EFI_HANDLE                  BooterHandle;

  Status = OcAppleBootPolicyInstallProtocol (gImageHandle, gST);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleBootPolicy install failure - %r\n", Status));
  }

  Status = gBS->LocateProtocol (
    &gAppleBootPolicyProtocolGuid,
    NULL,
    (VOID **) &AppleBootPolicy
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AppleBootPolicy locate failure - %r\n", Status));
    return Status;
  }

  while (TRUE) {
    DEBUG ((DEBUG_INFO, "Performing OcScanForBootEntries...\n"));

    Status = OcScanForBootEntries (
      AppleBootPolicy,
      LookupPolicy,
      &Entries,
      &EntryCount,
      NULL,
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
    Status = OcShowSimpleBootMenu (
      Entries,
      EntryCount,
      0,
      TimeoutSeconds,
      &Chosen
      );

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
      Status = OcLoadBootEntry (Chosen, BootPolicy, gImageHandle, &BooterHandle);
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

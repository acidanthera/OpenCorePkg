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

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#include <Guid/AppleVariable.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/OcVariable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

/*
  Expands DevicePath from short-form to full-form.
  The only valid expansions are full Device Paths refering to a file or a 
  volume root. Latter type may be used with custom policies to determine a
  bootable file.

  @param[in]  BootContext   Context of filesystems.
  @param[in]  DevicePath    The Device Path to expand.
  @param[in]  LazyScan      Lazy filesystem scanning.
  @param[out] FileSystem    Resulting filesystem.
  @param[out] IsRoot        Whether DevicePath refers to the root of a volume.

  @returns DevicePath expansion or NULL on failure.
*/
STATIC
EFI_DEVICE_PATH_PROTOCOL *
ExpandShortFormBootPath (
  IN  OC_BOOT_CONTEXT           *BootContext,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  BOOLEAN                   LazyScan,
  OUT OC_BOOT_FILESYSTEM        **FileSystem,
  OUT BOOLEAN                   *IsRoot
  )
{
  EFI_STATUS               Status;

  EFI_DEVICE_PATH_PROTOCOL *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *PrevDevicePath;

  EFI_HANDLE               FileSystemHandle;
  EFI_FILE_PROTOCOL        *File;
  EFI_FILE_INFO            *FileInfo;
  BOOLEAN                  IsRootPath;
  BOOLEAN                  IsDirectory;

  ASSERT (BootContext != NULL);
  ASSERT (DevicePath != NULL);
  ASSERT (FileSystem != NULL);
  ASSERT (IsRoot != NULL);

  //
  // Iteratively expand the short-form Device Path to its possible full forms.
  // A valid Device Path will either refer to a valid file or to a valid root
  // volume.
  //
  PrevDevicePath = NULL;
  IsDirectory = FALSE;
  do {
    FullDevicePath = OcGetNextLoadOptionDevicePath (
      DevicePath,
      PrevDevicePath
      );

    if (PrevDevicePath != NULL) {
      FreePool (PrevDevicePath);
    }

    //
    // When no more full representations can be built, the Device Path is
    // not bootable.
    //
    if (FullDevicePath == NULL) {
      DEBUG ((DEBUG_INFO, "OCB: Short-form DP could not be expanded\n"));
      return NULL;
    }

    PrevDevicePath = FullDevicePath;

    DebugPrintDevicePath (
      DEBUG_INFO,
      "OCB: Expanded DP",
      FullDevicePath
      );

  	//
    // Retrieve the filesystem handle.
    //
    RemainingDevicePath = FullDevicePath;
    Status = gBS->LocateDevicePath (
      &gEfiSimpleFileSystemProtocolGuid,
      &RemainingDevicePath,
      &FileSystemHandle
      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DebugPrintDevicePath (
      DEBUG_INFO,
      "OCB: Expanded DP remainder",
      RemainingDevicePath
      );

    //
    // Check whether we are allowed to boot from this filesystem.
    //
    *FileSystem = InternalFileSystemForHandle (BootContext, FileSystemHandle, LazyScan);
    if (*FileSystem == NULL) {
      continue;
    }

    //
    // Check whether the Device Path refers to a valid file handle.
    //
    Status = OcOpenFileByRemainingDevicePath (
      FileSystemHandle,
      RemainingDevicePath,
      &File,
      EFI_FILE_MODE_READ,
      0
      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Retrieve file info to determine potentially bootable state.
    //
    FileInfo = GetFileInfo (
      File,
      &gEfiFileInfoGuid,
      sizeof (EFI_FILE_INFO),
      NULL
      );
    //
    // When File Info cannot be retrieved, assume the worst case but don't
    // skip the Device Path expansion as it is valid.
    //
    IsDirectory = TRUE;
    if (FileInfo != NULL) {
      IsDirectory = (FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0;
      FreePool (FileInfo);
    }

    File->Close (File);

    //
    // Return only Device Paths that either refer to a file or a volume root.
    // Root Device Paths may be expanded by custom policies (such as Apple Boot
    // Policy) later.
    //
    IsRootPath = IsDevicePathEnd (RemainingDevicePath);
    if (IsRootPath || !IsDirectory) {
      ASSERT (FullDevicePath != NULL);
      ASSERT (*FileSystem != NULL);

      *IsRoot = IsDirectory;
      return FullDevicePath;
    }

    //
    // Request a new device path expansion.
    //
  } while (TRUE);
}

/**
  Check whether device path points to OpenCore bootloader.

  @param[in]  DevicePath   Device path of the entry.

  @retval TRUE   Entry represents OpenCore bootloader.
  @retval FALSE  Entry is not necessarily OpenCore bootloader.
**/
STATIC
BOOLEAN
IsOpenCoreBootloader (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  STATIC CONST UINT32 OpenCoreMagicOffset = 0x40;
  STATIC CONST UINT8  OpenCoreMagic[] = {
    0x0E, 0x1F, 0xBA, 0x10, 0x00, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x0F, 0x0B,
    0x4F, 0x70, 0x65, 0x6E, 0x43, 0x6F, 0x72, 0x65, 0x20, 0x42, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61,
    0x64, 0x65, 0x72, 0x20, 0x28, 0x63, 0x29, 0x20, 0x41, 0x63, 0x69, 0x64, 0x61, 0x6E, 0x74, 0x68,
    0x65, 0x72, 0x61, 0x20, 0x52, 0x65, 0x73, 0x65, 0x61, 0x72, 0x63, 0x68, 0x0D, 0x0A, 0x24, 0x00 
  };

  EFI_STATUS        Status;

  EFI_FILE_PROTOCOL *File;
  UINT8             FileReadMagic[sizeof (OpenCoreMagic)];

  Status = OcOpenFileByDevicePath (
    &DevicePath,
    &File,
    EFI_FILE_MODE_READ,
    0
    );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = GetFileData (
    File,
    OpenCoreMagicOffset,
    sizeof (FileReadMagic),
    FileReadMagic
    );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return CompareMem (FileReadMagic, OpenCoreMagic, sizeof (OpenCoreMagic)) == 0;
}

/**
  Register bootable entry on the filesystem.

  @param[in,out] BootContext   Context of filesystems.
  @param[in,out] FileSystem    Filesystem for creation.
  @param[in]     BootEntry     Entry to register.
**/
STATIC
VOID
RegisterBootOption (
  IN OUT OC_BOOT_CONTEXT           *BootContext,
  IN OUT OC_BOOT_FILESYSTEM        *FileSystem,
  IN     OC_BOOT_ENTRY             *BootEntry
  )
{
  CHAR16  *TextDevicePath;

  DEBUG_CODE_BEGIN ();

  if (BootEntry->DevicePath != NULL) {
    TextDevicePath = ConvertDevicePathToText (BootEntry->DevicePath, TRUE, FALSE);
  } else {
    TextDevicePath = NULL;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Registering entry %s (T:%d|F:%d|G:%d|E:%d) - %s\n",
    BootEntry->Name,
    BootEntry->Type,
    BootEntry->IsFolder,
    BootEntry->IsGeneric,
    BootEntry->IsExternal,
    OC_HUMAN_STRING (TextDevicePath)
    ));

  if (TextDevicePath != NULL) {
    FreePool (TextDevicePath);
  }

  DEBUG_CODE_END ();

  //
  // Register boot entry.
  // Not using RecoveryFs is intended for correct order.
  //
  InsertTailList (&FileSystem->BootEntries, &BootEntry->Link);
  ++BootContext->BootEntryCount;

  //
  // For tools and system options we are done.
  //
  if ((BootEntry->Type & (OC_BOOT_SYSTEM | OC_BOOT_EXTERNAL_TOOL)) != 0) {
    return;
  }

  //
  // If no options were previously found this is the default one.
  //
  if (BootContext->DefaultEntry == NULL) {
    BootContext->DefaultEntry = BootEntry;
  }

  //
  // Set override picker commands.
  //
  if (BootContext->PickerContext->PickerCommand == OcPickerBootApple) {
    if (BootContext->DefaultEntry->Type != OC_BOOT_APPLE_OS
      && BootEntry->Type == OC_BOOT_APPLE_OS) {
      BootContext->DefaultEntry = BootEntry;
    }
  } else if (BootContext->PickerContext->PickerCommand == OcPickerBootAppleRecovery) {
    if (BootContext->DefaultEntry->Type != OC_BOOT_APPLE_RECOVERY
      && BootEntry->Type == OC_BOOT_APPLE_RECOVERY) {
      BootContext->DefaultEntry = BootEntry;
    }
  }
}

/**
  Create single bootable entry from device path.

  @param[in,out] BootContext   Context of filesystems.
  @param[in,out] FileSystem    Filesystem for creation.
  @param[in]     DevicePath    Device path of the entry.
  @param[in]     RecoveryPart  Device path is on recovery partition.
  @param[in]     Deduplicate   Ensure that duplicated entries are not added. 

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
AddBootEntryOnFileSystem (
  IN OUT OC_BOOT_CONTEXT           *BootContext,
  IN OUT OC_BOOT_FILESYSTEM        *FileSystem,
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN     BOOLEAN                   RecoveryPart,
  IN     BOOLEAN                   Deduplicate
  )
{
  EFI_STATUS          Status;
  OC_BOOT_ENTRY       *BootEntry;
  OC_BOOT_ENTRY_TYPE  EntryType;
  LIST_ENTRY          *Link;
  OC_BOOT_ENTRY       *ExistingEntry;
  CHAR16              *TextDevicePath;
  BOOLEAN             IsFolder;
  BOOLEAN             IsGeneric;
  BOOLEAN             IsReallocated;

  EntryType = OcGetBootDevicePathType (DevicePath, &IsFolder, &IsGeneric);

  if (IsFolder && BootContext->PickerContext->DmgLoading == OcDmgLoadingDisabled) {
    DevicePath    = AppendFileNameDevicePath (DevicePath, L"boot.efi");
    IsFolder      = FALSE;
    IsReallocated = TRUE;
    DEBUG ((DEBUG_INFO, "OCB: Switching DMG boot path to boot.efi due to policy\n"));
    if (DevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    IsReallocated = FALSE;
  }

  DEBUG_CODE_BEGIN ();

  TextDevicePath = ConvertDevicePathToText (DevicePath, TRUE, FALSE);

  DEBUG ((
    DEBUG_INFO,
    "OCB: Adding entry type (T:%u|F:%d|G:%d) - %s\n",
    EntryType,
    IsFolder,
    IsGeneric,
    OC_HUMAN_STRING (TextDevicePath)
    ));

  if (TextDevicePath != NULL) {
    FreePool (TextDevicePath);
  }

  DEBUG_CODE_END ();

  //
  // Mark self recovery presence.
  //
  if (!RecoveryPart && EntryType == OC_BOOT_APPLE_RECOVERY) {
    FileSystem->HasSelfRecovery = TRUE;
  }

  //
  // Do not add recoveries when not requested (e.g. can be HFS+ recovery).
  //
  if (BootContext->PickerContext->HideAuxiliary && EntryType == OC_BOOT_APPLE_RECOVERY) {
    DEBUG ((DEBUG_INFO, "OCB: Discarding recovery entry due to auxiliary\n"));
    if (IsReallocated) {
      FreePool (DevicePath);
    }
    return EFI_UNSUPPORTED;
  }

  //
  // Do not add Time Machine when not requested.
  //
  if (BootContext->PickerContext->HideAuxiliary && EntryType == OC_BOOT_APPLE_TIME_MACHINE) {
    DEBUG ((DEBUG_INFO, "OCB: Discarding time machine entry due to auxiliary\n"));
    if (IsReallocated) {
      FreePool (DevicePath);
    }
    return EFI_UNSUPPORTED;
  }

  //
  // Skip OpenCore bootloaders on own entry.
  // We do not waste time doing this for other entries.
  //
  if (RecoveryPart ? FileSystem->RecoveryFs->LoaderFs : FileSystem->LoaderFs
    && IsOpenCoreBootloader (DevicePath)) {
    DEBUG ((DEBUG_INFO, "OCB: Discarding discovered OpenCore bootloader\n"));
    if (IsReallocated) {
      FreePool (DevicePath);
    }
    return EFI_UNSUPPORTED;
  }

  //
  // Skip duplicated entries, which may happen in BootOrder.
  // For example, macOS during hibernation may leave Boot0082 in BootNext and Boot0080 in BootOrder,
  // and they will have exactly the same boot entry.
  //
  if (Deduplicate) {
    for (
      Link = GetFirstNode (&FileSystem->BootEntries);
      !IsNull (&FileSystem->BootEntries, Link);
      Link = GetNextNode (&FileSystem->BootEntries, Link)) {
      ExistingEntry = BASE_CR (Link, OC_BOOT_ENTRY, Link);
      //
      // All non-custom entries have DPs.
      //
      ASSERT (ExistingEntry->DevicePath != NULL);
      if (IsDevicePathEqual (ExistingEntry->DevicePath, DevicePath)) {
        DEBUG ((DEBUG_INFO, "OCB: Discarding already present DP\n"));
        //
        // We may have more than one macOS installation in APFS container.
        // Boot policy returns them in a defined (constant) order, and we want
        // to preserve this order regardless of the BootOrder.
        //
        // When an operating system is present in BootOrder it will be put to
        // the front of FileSystem boot entries. As a result instead of:
        // [OS1], [REC1], [OS2], [REC2] we may get [OS2], [OS1], [REC1], [REC2].
        // The latter happens because after [REC1] discovered [OS1] is skipped
        // due to being already present. The code below moves [OS2] to the end
        // of list at [REC1] stage to fix the order.
        //
        // This change assumes that only one operating system from the container
        // can be present as a boot option. For now this appears to be true.
        //
        RemoveEntryList (Link);
        InsertTailList (&FileSystem->BootEntries, Link);
        if (IsReallocated) {
          FreePool (DevicePath);
        }
        return EFI_ALREADY_STARTED;
      }
    }
  }

  //
  // Allocate, initialise, and describe boot entry.
  //
  BootEntry = AllocateZeroPool (sizeof (*BootEntry));
  if (BootEntry == NULL) {
    if (IsReallocated) {
      FreePool (DevicePath);
    }
    return EFI_OUT_OF_RESOURCES;
  }

  BootEntry->DevicePath = DevicePath;
  BootEntry->Type       = EntryType;
  BootEntry->IsFolder   = IsFolder;
  BootEntry->IsGeneric  = IsGeneric;
  BootEntry->IsExternal = RecoveryPart ? FileSystem->RecoveryFs->External : FileSystem->External;

  Status = InternalDescribeBootEntry (BootEntry);
  if (EFI_ERROR (Status)) {
    FreePool (BootEntry);
    if (IsReallocated) {
      FreePool (DevicePath);
    }
    return Status;
  }

  RegisterBootOption (
    BootContext,
    FileSystem,
    BootEntry
    );

  return EFI_SUCCESS;
}

/**
  Create bootable entry from custom entry.

  @param[in,out] BootContext   Context of filesystems.
  @param[in,out] FileSystem    Filesystem to add custom entry.
  @param[in]     CustomEntry   Custom entry.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
AddBootEntryFromCustomEntry (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN OUT OC_BOOT_FILESYSTEM  *FileSystem,
  IN     OC_PICKER_ENTRY     *CustomEntry
  )
{
  OC_BOOT_ENTRY         *BootEntry;
  CHAR16                *PathName;
  FILEPATH_DEVICE_PATH  *FilePath;

  if (CustomEntry->Auxiliary && BootContext->PickerContext->HideAuxiliary) {
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate, initialise, and describe boot entry.
  //
  BootEntry = AllocateZeroPool (sizeof (*BootEntry));
  if (BootEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BootEntry->Name = AsciiStrCopyToUnicode (CustomEntry->Name, 0);
  if (BootEntry->Name == NULL) {
    FreePool (BootEntry);
    return EFI_OUT_OF_RESOURCES;
  }

  PathName = AsciiStrCopyToUnicode (CustomEntry->Path, 0);
  if (PathName == NULL) {
    FreePool (BootEntry->Name);
    FreePool (BootEntry);
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Adding custom entry %s (%a) -> %a\n",
    BootEntry->Name,
    CustomEntry->Tool ? "tool" : "os",
    CustomEntry->Path
    ));

  if (CustomEntry->Tool) {
    BootEntry->Type = OC_BOOT_EXTERNAL_TOOL;
    UnicodeUefiSlashes (PathName);
    BootEntry->PathName = PathName;
  } else {
    BootEntry->Type = OC_BOOT_EXTERNAL_OS;

    BootEntry->DevicePath = ConvertTextToDevicePath (PathName);
    FreePool (PathName);
    if (BootEntry->DevicePath == NULL) {
      FreePool (BootEntry->Name);
      FreePool (BootEntry);
      return EFI_OUT_OF_RESOURCES;
    }

    FilePath = (FILEPATH_DEVICE_PATH *) (
        FindDevicePathNodeWithType (
          BootEntry->DevicePath,
          MEDIA_DEVICE_PATH,
          MEDIA_FILEPATH_DP
        )
      );
    if (FilePath == NULL) {
      FreePool (BootEntry->Name);
      FreePool (BootEntry->DevicePath);
      FreePool (BootEntry);
      return EFI_UNSUPPORTED;
    }

    BootEntry->PathName = AllocateCopyPool (
      OcFileDevicePathNameSize (FilePath),
      FilePath->PathName
      );
    if (BootEntry->PathName == NULL) {
      FreePool (BootEntry->Name);
      FreePool (BootEntry->DevicePath);
      FreePool (BootEntry);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  BootEntry->LoadOptionsSize = (UINT32) AsciiStrLen (CustomEntry->Arguments);
  if (BootEntry->LoadOptionsSize > 0) {
    BootEntry->LoadOptions = AllocateCopyPool (
      BootEntry->LoadOptionsSize + 1,
      CustomEntry->Arguments
      );
    if (BootEntry->LoadOptions == NULL) {
      BootEntry->LoadOptionsSize = 0;
    }
  }

  RegisterBootOption (
    BootContext,
    FileSystem,
    BootEntry
    );

  return EFI_SUCCESS;
}

/**
  Create bootable entry from system entry.

  @param[in,out] BootContext   Context of filesystems.
  @param[in,out] FileSystem    Filesystem to add custom entry.
  @param[in]     Name          System entry name.
  @param[in]     Action        System entry action.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
AddBootEntryFromSystemEntry (
  IN OUT OC_BOOT_CONTEXT        *BootContext,
  IN OUT OC_BOOT_FILESYSTEM     *FileSystem,
  IN     CONST CHAR16           *Name,
  IN     OC_BOOT_SYSTEM_ACTION  Action
  )
{
  OC_BOOT_ENTRY         *BootEntry;

  if (BootContext->PickerContext->HideAuxiliary) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "OCB: Adding system entry %s\n", Name));

  //
  // Allocate, initialise, and describe boot entry.
  //
  BootEntry = AllocateZeroPool (sizeof (*BootEntry));
  if (BootEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BootEntry->Name = AllocateCopyPool (StrSize (Name), Name);
  if (BootEntry->Name == NULL) {
    FreePool (BootEntry);
    return EFI_OUT_OF_RESOURCES;
  }

  BootEntry->Type         = OC_BOOT_RESET_NVRAM;
  BootEntry->SystemAction = Action;

  RegisterBootOption (
    BootContext,
    FileSystem,
    BootEntry
    );

  return EFI_SUCCESS;
}

/**
  Create bootable entries from bless policy.
  This function may create more than one entry, and for APFS
  it will likely produce a sequence of 'OS, RECOVERY' entry pairs.

  @param[in,out] BootContext         Context of filesystems.
  @param[in,out] FileSystem          Filesystem to scan for bless.
  @param[in]     PredefinedPaths     The predefined boot file locations to scan.
  @param[in]     NumPredefinedPaths  The number of elements in PredefinedPaths.
  @param[in]     LazyScan            Lazy filesystem scanning.
  @param[in]     Deduplicate         Ensure that duplicated entries are not added. 

  @retval EFI_STATUS for last created option.
**/
STATIC
EFI_STATUS
AddBootEntryFromBless (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN OUT OC_BOOT_FILESYSTEM  *FileSystem,
  IN     CONST CHAR16        **PredefinedPaths,
  IN     UINTN               NumPredefinedPaths,
  IN     BOOLEAN             LazyScan,
  IN     BOOLEAN             Deduplicate
  )
{
  EFI_STATUS                       Status;
  EFI_STATUS                       PrimaryStatus;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePathWalker;
  EFI_DEVICE_PATH_PROTOCOL         *NewDevicePath;
  UINTN                            NewDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL         *HdDevicePath;
  UINTN                            HdPrefixSize;
  INTN                             CmpResult;
  EFI_FILE_PROTOCOL                *Root;
  CHAR16                           *RecoveryPath;
  EFI_FILE_PROTOCOL                *RecoveryRoot;
  EFI_HANDLE                       RecoveryDeviceHandle;

  //
  // We need to ensure that blessed device paths are on the same filesystem.
  // Read the prefix path.
  //
  Status = gBS->HandleProtocol (
    FileSystem->Handle,
    &gEfiDevicePathProtocolGuid,
    (VOID **) &HdDevicePath
    );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  DebugPrintDevicePath (DEBUG_INFO, "OCB: Adding bless entry on disk", HdDevicePath);

  HdPrefixSize = GetDevicePathSize (HdDevicePath) - END_DEVICE_PATH_LENGTH;

  //
  // Custom bless paths have the priority, try to look them up first.
  //
  if (BootContext->PickerContext->NumCustomBootPaths > 0) {
    Status = gBS->HandleProtocol (
      FileSystem->Handle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID **) &SimpleFs
      );

    if (!EFI_ERROR (Status)) {
      Status = SimpleFs->OpenVolume (SimpleFs, &Root);
      if (!EFI_ERROR (Status)) {
        Status = OcGetBooterFromPredefinedPathList (
          FileSystem->Handle,
          Root,
          (CONST CHAR16 **) BootContext->PickerContext->CustomBootPaths,
          BootContext->PickerContext->NumCustomBootPaths,
          &DevicePath,
          NULL
          );

        Root->Close (Root);
      }
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  //
  // On failure obtain normal bless paths.
  //
  if (EFI_ERROR (Status)) {
    Status = OcBootPolicyGetBootFileEx (
      FileSystem->Handle,
      PredefinedPaths,
      NumPredefinedPaths,
      &DevicePath
      );
  }

  //
  // If both custom and normal found nothing, then nothing is blessed.
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Since blessed paths can be multiple (e.g. when more than one macOS is present in the container).
  //
  Status = EFI_NOT_FOUND;
  DevicePathWalker = DevicePath;
  while (TRUE) {
    NewDevicePath = GetNextDevicePathInstance (&DevicePathWalker, &NewDevicePathSize);
    if (NewDevicePath == NULL) {
      break;
    }

    //
    // Blessed path is obviously too short.
    //
    if (NewDevicePathSize - END_DEVICE_PATH_LENGTH < HdPrefixSize) {
      FreePool (NewDevicePath);
      continue;
    }

    //
    // Blessed path does not prefix filesystem path.
    //
    CmpResult = CompareMem (
      NewDevicePath,
      HdDevicePath,
      HdPrefixSize
      );
    if (CmpResult != 0) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Skipping handle %p instance due to self trust violation\n",
        FileSystem->Handle
        ));

      DebugPrintDevicePath (
        DEBUG_INFO,
        "OCB: Disk DP",
        HdDevicePath
        );
      DebugPrintDevicePath (
        DEBUG_INFO,
        "OCB: Instance DP",
        NewDevicePath
        );

      FreePool (NewDevicePath);
      continue;
    }

    //
    // Add blessed device path.
    //
    PrimaryStatus = AddBootEntryOnFileSystem (
      BootContext,
      FileSystem,
      NewDevicePath,
      FALSE,
      Deduplicate
      );
    //
    // Cannot free the failed device path now as it may have recovery.
    //

    //
    // If the partition contains recovery on itself or recoveries are not requested,
    // proceed to next entry.
    //
    // First part means that APFS recovery is irrelevant, these recoveries are actually
    // on a different partition, but can only be pointed from Preboot.
    // This way we will show any 'com.apple.recovery.boot' recovery physically present
    // on the partition no more than once.
    //
    if (FileSystem->HasSelfRecovery || BootContext->PickerContext->HideAuxiliary) {
      if (EFI_ERROR (PrimaryStatus)) {
        FreePool (NewDevicePath);
      }
      Status = PrimaryStatus;
      continue;
    }

    //
    // Now add APFS recovery (from Recovery partition) right afterwards if present.
    //
    Status = OcBootPolicyGetApfsRecoveryFilePath (
      NewDevicePath,
      L"\\",
      PredefinedPaths,
      NumPredefinedPaths,
      &RecoveryPath,
      &RecoveryRoot,
      &RecoveryDeviceHandle
      );

    //
    // Can free the failed primary device path now.
    //
    if (EFI_ERROR (PrimaryStatus)) {
      FreePool (NewDevicePath);
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCB: APFS recovery is not present - %r\n", Status));
      continue;
    }

    RecoveryRoot->Close (RecoveryRoot);

    //
    // Obtain recovery file system and ensure scan policy if it was not done before.
    //
    if (FileSystem->RecoveryFs == NULL) {
      FileSystem->RecoveryFs = InternalFileSystemForHandle (BootContext, RecoveryDeviceHandle, LazyScan);
    }

    //
    // If new recovery is not on the same volume or not allowed, then something went wrong, skip it.
    // This is technically also a performance optimisation allowing us not to lookup recovery fs every time.
    //
    if (FileSystem->RecoveryFs == NULL || FileSystem->RecoveryFs->Handle != RecoveryDeviceHandle) {
      FreePool (RecoveryPath);
      continue;
    }

    NewDevicePath = FileDevicePath (RecoveryDeviceHandle, RecoveryPath);
    FreePool (RecoveryPath);
    if (NewDevicePath == NULL) {
      continue;
    }

    //
    // Add blessed device path.
    //
    Status = AddBootEntryOnFileSystem (
      BootContext,
      FileSystem,
      NewDevicePath,
      TRUE,
      Deduplicate
      );
    if (EFI_ERROR (Status)) {
      FreePool (NewDevicePath);
    }
  }

  FreePool (DevicePath);

  return Status;
}

/**
  Create bootable entries from recovery files (com.apple.boot.recovery) on the volume.

  @param[in,out] BootContext   Context of filesystems.
  @param[in,out] FileSystem    Filesystem to scan for recovery.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
AddBootEntryFromSelfRecovery (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN OUT OC_BOOT_FILESYSTEM  *FileSystem
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;

  //
  // If there is already one recovery (it may not be registered due to HideAuxiliary)
  // or if there is HideAuxiliary, do not add recoveries at all.
  //
  if (FileSystem->HasSelfRecovery || BootContext->PickerContext->HideAuxiliary) {
    return EFI_UNSUPPORTED;
  }

  Status = InternalGetRecoveryOsBooter (
    FileSystem->Handle,
    &DevicePath,
    FALSE
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Returned device path is always on the same partition, thus no scan check.
  //
  Status = AddBootEntryOnFileSystem (
    BootContext,
    FileSystem,
    DevicePath,
    FALSE,
    FALSE
    );

  if (EFI_ERROR (Status)) {
    FreePool (DevicePath);
  }

  return Status;
}

/**
  Create bootable entries from boot options.

  @param[in,out] BootContext   Context of filesystems.
  @param[in]     BootOption    Boot option number.
  @param[in]     LazyScan      Lazy filesystem scanning.

  @retval EFI_SUCCESS if at least one option was added.
**/
STATIC
EFI_STATUS
AddBootEntryFromBootOption (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN     UINT16              BootOption,
  IN     BOOLEAN             LazyScan
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *ExpandedDevicePath;
  EFI_HANDLE                 FileSystemHandle;
  OC_BOOT_FILESYSTEM         *FileSystem;
  UINTN                      DevicePathSize;
  INTN                       NumPatchedNodes;
  BOOLEAN                    IsAppleLegacy;
  BOOLEAN                    IsRoot;

  DEBUG ((DEBUG_INFO, "OCB: Building entry from Boot%04x\n", BootOption));

  //
  // Obtain original device path.
  // Discard load options for security reasons.
  // Also discard boot name to avoid confusion.
  //
  DevicePath = InternalGetBootOptionData (
    BootOption,
    BootContext->BootVariableGuid,
    NULL,
    NULL,
    NULL
    );
  if (DevicePath == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Get BootCamp device path stored in special variable.
  // BootCamp device path will point to disk instead of partition.
  //
  IsAppleLegacy = InternalIsAppleLegacyLoadApp (DevicePath);
  if (IsAppleLegacy) {
    FreePool (DevicePath);
    Status = GetVariable2 (
      APPLE_BOOT_CAMP_HD_VARIABLE_NAME,
      &gAppleBootVariableGuid,
      (VOID **) &DevicePath,
      &DevicePathSize
      );

    if (EFI_ERROR (Status) || !IsDevicePathValid (DevicePath, DevicePathSize)) {
      DEBUG ((DEBUG_INFO, "OCB: Legacy DP invalid - %r\n", Status));
      if (!EFI_ERROR (Status)) {
        FreePool (DevicePath);
      }
      return EFI_NOT_FOUND;
    } else {
      DebugPrintDevicePath (DEBUG_INFO, "OCB: Solved legacy DP", DevicePath);
    }
  }

  FileSystem = NULL;
  IsRoot     = FALSE;

  //
  // Fixup device path if necessary.
  // WARN: DevicePath must be allocated from pool as it may be reallocated.
  //
  NumPatchedNodes = OcFixAppleBootDevicePath (
    &DevicePath,
    &RemainingDevicePath
    );
  if (NumPatchedNodes > 0) {
    //
    // DevicePath size may be different on successful update.
    //
    DevicePathSize = GetDevicePathSize (DevicePath);
    DebugPrintDevicePath (DEBUG_INFO, "OCB: Fixed DP", DevicePath);
  }

  //
  // Expand BootCamp device path to EFI partition device path.
  //
  if (IsAppleLegacy) {
    //
    // BootCampHD always refers to a full Device Path. Failure to patch
    // indicates an invalid Device Path.
    //
    if (NumPatchedNodes == -1) {
      DEBUG ((DEBUG_INFO, "OCB: Ignoring broken legacy DP\n"));
      FreePool (DevicePath);
      return EFI_NOT_FOUND;
    }

    RemainingDevicePath = DevicePath;
    DevicePath = OcDiskFindSystemPartitionPath (
      DevicePath,
      &DevicePathSize,
      &FileSystemHandle
      );

    FreePool (RemainingDevicePath);

    //
    // This is obviously always a Root Device Path.
    //
    IsRoot = TRUE;

    //
    // Ensure that we are allowed to boot from this filesystem.
    //
    if (DevicePath != NULL) {
      FileSystem = InternalFileSystemForHandle (BootContext, FileSystemHandle, LazyScan);
      if (FileSystem == NULL) {
        DevicePath = NULL;
      }
    }

    //
    // The Device Path returned by OcDiskFindSystemPartitionPath() is a pointer
    // to an installed protocol. Duplicate it so we own the memory.
    //
    if (DevicePath != NULL) {
      DevicePath = AllocateCopyPool (DevicePathSize, DevicePath);
    }

    if (DevicePath == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // The Device Path must be entirely locatable (and hence full-form) as
    // OcDiskFindSystemPartitionPath() guarantees to only return valid paths.
    //
    ASSERT (DevicePathSize > END_DEVICE_PATH_LENGTH);
    DevicePathSize -= END_DEVICE_PATH_LENGTH;
    RemainingDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) ((UINTN) DevicePath + DevicePathSize);
  } else if (DevicePath == RemainingDevicePath) {
    //
    // OcFixAppleBootDevicePath() did not advance the Device Path node, hence
    // it cannot be located at all and may be a short-form Device Path.
    // DevicePath has not been changed no matter success or failure.
    //
    DEBUG ((DEBUG_INFO, "OCB: Assuming DP is short-form (prefix)\n"));

    //
    // Expand and on failure fix the Device Path till both yields no new result.
    //
    do {
      //
      // Expand the short-form Device Path.
      //
      ExpandedDevicePath = ExpandShortFormBootPath (
        BootContext,
        DevicePath,
        LazyScan,
        &FileSystem,
        &IsRoot
        );
      if (ExpandedDevicePath != NULL) {
        break;
      }

      //
      // If short-form expansion failed, try to fix the short-form and re-try.
      // WARN: DevicePath must be allocated from pool here.
      //
      NumPatchedNodes = OcFixAppleBootDevicePathNode (
        &DevicePath,
        &RemainingDevicePath,
        NULL
        );
    } while (NumPatchedNodes > 0);

    FreePool (DevicePath);
    DevicePath = ExpandedDevicePath;

    if (DevicePath == NULL) {
      return EFI_NOT_FOUND;
    }
  } else if (NumPatchedNodes == -1) {
    //
    // OcFixAppleBootDevicePath() advanced the Device Path node and yet failed
    // to locate the path, it is invalid.
    //
    DEBUG ((DEBUG_INFO, "OCB: Ignoring broken normal DP\n"));
    FreePool (DevicePath);
    return EFI_NOT_FOUND;
  } else {
    //
    // OcFixAppleBootDevicePath() advanced the Device Path node and succeeded
    // to locate the path, but it may still be a shot-form Device Path (lacking
    // a suffix rather than prefix).
    //
    DEBUG ((DEBUG_INFO, "OCB: Assuming DP is full-form or lacks suffix\n"));

    RemainingDevicePath = DevicePath;
    DevicePath = ExpandShortFormBootPath (
      BootContext,
      RemainingDevicePath,
      LazyScan,
      &FileSystem,
      &IsRoot
      );

    FreePool (RemainingDevicePath);

    if (DevicePath == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  //
  // If we reached here we have a filesystem and device path.
  //
  ASSERT (FileSystem != NULL);
  ASSERT (DevicePath != NULL);

  //
  // We have a complete device path, just add this entry.
  //
  if (!IsRoot) {
    Status = AddBootEntryOnFileSystem (
      BootContext,
      FileSystem,
      DevicePath,
      FALSE,
      TRUE
      );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    FreePool (DevicePath);
  }

  //
  // We may have a Boot#### entry pointing to macOS with full DP (up to boot.efi),
  // so IsRoot will be true. However, if this is APFS, we may still have:
  // - Recovery for this macOS.
  // - Another macOS installation.
  // We can only detect them with bless, so we invoke bless in deduplication mode.
  // We also detect only the Core Apple Boot Policy predefined booter paths to
  // avoid detection of e.g. generic booters (such as BOOTx64) to avoid
  // duplicates.
  //
  // The amount of paths depends on the kind of the entry.
  // - If this is a root entry (i.e. it points to the partition)
  //   we invoke full bless, as it may be Windows entry created by legacy NVRAM script.
  // - If this is a full entry (i.e. it points to the bootloader)
  //   we invoke partial bless, which ignores BOOTx64.efi.
  //   Ignoring BOOTx64.efi is important as we may already have bootmgfw.efi as our entry,
  //   and we do not want to see Windows added twice.
  //
  Status = AddBootEntryFromBless (
    BootContext,
    FileSystem,
    gAppleBootPolicyPredefinedPaths,
    IsRoot ? gAppleBootPolicyNumPredefinedPaths : gAppleBootPolicyCoreNumPredefinedPaths,
    LazyScan,
    TRUE
    );

  return Status;
}

/**
  Release boot entry contents allocated from pool.

  @param[in,out]  BootEntry      Located boot entry.
**/
STATIC
VOID
FreeBootEntry (
  IN OC_BOOT_ENTRY        *BootEntry
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

  if (BootEntry->LoadOptions != NULL) {
    FreePool (BootEntry->LoadOptions);
    BootEntry->LoadOptions     = NULL;
    BootEntry->LoadOptionsSize = 0;
  }

  FreePool (BootEntry);
}

/**
  Allocate a new filesystem entry in boot entries
  in case it can be used according to current ScanPolicy.

  @param[in,out] BootContext       Context of filesystems.
  @param[in]     FileSystemHandle  Filesystem handle.
  @param[in]     FileSystemEntry   Resulting filesystem, optional.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
AddFileSystemEntry (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN     EFI_HANDLE          FileSystemHandle,
     OUT OC_BOOT_FILESYSTEM  **FileSystemEntry  OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                TmpStatus;
  BOOLEAN                   IsExternal;
  BOOLEAN                   LoaderFs;
  OC_BOOT_FILESYSTEM        *Entry;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *TextDevicePath;

  Status = InternalCheckScanPolicy (
    FileSystemHandle,
    BootContext->PickerContext->ScanPolicy,
    &IsExternal
    );

  LoaderFs = BootContext->PickerContext->LoaderHandle == FileSystemHandle;

  DEBUG_CODE_BEGIN ();

  TmpStatus = gBS->HandleProtocol (
    FileSystemHandle,
    &gEfiDevicePathProtocolGuid,
    (VOID **) &DevicePath
    );
  if (!EFI_ERROR (TmpStatus)) {
    TextDevicePath = ConvertDevicePathToText (DevicePath, TRUE, FALSE);
  } else {
    TextDevicePath = NULL;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Adding fs %p (E:%d|L:%d|P:%r) - %s\n",
    FileSystemHandle,
    IsExternal,
    LoaderFs,
    Status,
    OC_HUMAN_STRING (TextDevicePath)
    ));

  if (TextDevicePath != NULL) {
    FreePool (TextDevicePath);
  }

  DEBUG_CODE_END ();

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Entry = AllocatePool (sizeof (*Entry));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Handle = FileSystemHandle;
  InitializeListHead (&Entry->BootEntries);
  Entry->RecoveryFs      = NULL;
  Entry->External        = IsExternal;
  Entry->LoaderFs        = LoaderFs;
  Entry->HasSelfRecovery = FALSE;
  InsertTailList (&BootContext->FileSystems, &Entry->Link);
  ++BootContext->FileSystemCount;

  if (FileSystemEntry != NULL) {
    *FileSystemEntry = Entry;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
AddFileSystemEntryForCustom (
  IN OUT OC_BOOT_CONTEXT     *BootContext
  )
{
  EFI_STATUS          Status;
  OC_BOOT_FILESYSTEM  *FileSystem;
  UINTN               Index;

  //
  // When there are no custom entries and NVRAM reset is hidden
  // we have no work to do.
  //
  if (BootContext->PickerContext->AllCustomEntryCount == 0
    && (!BootContext->PickerContext->ShowNvramReset
      || BootContext->PickerContext->HideAuxiliary)) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Adding fs %p for %u custom entries%a%a\n",
    OC_CUSTOM_FS_HANDLE,
    BootContext->PickerContext->AllCustomEntryCount,
    BootContext->PickerContext->ShowNvramReset ? " and nvram reset" : "",
    BootContext->PickerContext->HideAuxiliary ? " (aux hidden)" : " (aux shown)"
    ));

  FileSystem = AllocateZeroPool (sizeof (*FileSystem));
  if (FileSystem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FileSystem->Handle = OC_CUSTOM_FS_HANDLE;
  InitializeListHead (&FileSystem->BootEntries);
  InsertTailList (&BootContext->FileSystems, &FileSystem->Link);
  ++BootContext->FileSystemCount;

  Status = EFI_NOT_FOUND;
  for (Index = 0; Index < BootContext->PickerContext->AllCustomEntryCount; ++Index) {
    Status = AddBootEntryFromCustomEntry (
      BootContext,
      FileSystem,
      &BootContext->PickerContext->CustomEntries[Index]
      );
  }

  if (BootContext->PickerContext->ShowNvramReset) {
    Status = AddBootEntryFromSystemEntry (
      BootContext,
      FileSystem,
      OC_MENU_RESET_NVRAM_ENTRY,
      InternalSystemActionResetNvram
      );
  }

  return Status;
}

STATIC
VOID
FreeFileSystemEntry (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN     OC_BOOT_FILESYSTEM  *FileSystemEntry
  )
{
  LIST_ENTRY     *Link;
  OC_BOOT_ENTRY  *BootEntry;

  RemoveEntryList (&FileSystemEntry->Link);
  --BootContext->FileSystemCount;

  while (!IsListEmpty (&FileSystemEntry->BootEntries)) {
    Link = GetFirstNode (&FileSystemEntry->BootEntries);
    BootEntry = BASE_CR (Link, OC_BOOT_ENTRY, Link);
    RemoveEntryList (Link);
    FreeBootEntry (BootEntry);
  }

  FreePool (FileSystemEntry);
}

OC_BOOT_FILESYSTEM *
InternalFileSystemForHandle (
  IN OC_BOOT_CONTEXT  *BootContext,
  IN EFI_HANDLE       FileSystemHandle,
  IN BOOLEAN          LazyScan
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Link;
  OC_BOOT_FILESYSTEM  *FileSystem;  

  for (
    Link = GetFirstNode (&BootContext->FileSystems);
    !IsNull (&BootContext->FileSystems, Link);
    Link = GetNextNode (&BootContext->FileSystems, Link)) {
    FileSystem = BASE_CR (Link, OC_BOOT_FILESYSTEM, Link);

    if (FileSystem->Handle == FileSystemHandle) {
      DEBUG ((DEBUG_INFO, "OCB: Matched fs %p%a\n", FileSystemHandle, LazyScan ? " (lazy)" : ""));
      return FileSystem;
    }
  }

  //
  // Lazily check filesystem scan policy and add it in case it is ok. 
  //
  if (!LazyScan) {
    DEBUG ((DEBUG_INFO, "OCB: Restricted fs %p access\n", FileSystemHandle));
    return NULL;
  }

  Status = AddFileSystemEntry (BootContext, FileSystemHandle, &FileSystem);
  if (!EFI_ERROR (Status)) {
    return FileSystem;
  }

  return NULL;
}

STATIC
OC_BOOT_CONTEXT *
BuildFileSystemList (
  IN OC_PICKER_CONTEXT  *Context,
  IN BOOLEAN            Empty
  )
{
  OC_BOOT_CONTEXT  *BootContext;
  EFI_STATUS       Status;
  UINTN            NoHandles;
  EFI_HANDLE       *Handles;
  UINTN            Index;

  BootContext = AllocatePool (sizeof (*Context));
  if (BootContext == NULL) {
    return NULL;
  }

  BootContext->BootEntryCount  = 0;
  BootContext->FileSystemCount = 0;
  InitializeListHead (&BootContext->FileSystems);
  if (Context->CustomBootGuid) {
    BootContext->BootVariableGuid = &gOcVendorVariableGuid;
  } else {
    BootContext->BootVariableGuid = &gEfiGlobalVariableGuid;
  }
  BootContext->DefaultEntry  = NULL;
  BootContext->PickerContext = Context;

  if (Empty) {
    return BootContext;
  }

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    &NoHandles,
    &Handles
    );
  if (EFI_ERROR (Status)) {
    return BootContext;
  }

  for (Index = 0; Index < NoHandles; ++Index) {
    AddFileSystemEntry (
      BootContext,
      Handles[Index],
      NULL
      );
  }

  FreePool (Handles);
  return BootContext;
}

VOID
OcFreeBootContext (
  IN OUT OC_BOOT_CONTEXT  *Context
  )
{
  LIST_ENTRY          *Link;
  OC_BOOT_FILESYSTEM  *FileSystem;

  while (!IsListEmpty (&Context->FileSystems)) {
    Link = GetFirstNode (&Context->FileSystems);
    FileSystem = BASE_CR (Link, OC_BOOT_FILESYSTEM, Link);
    FreeFileSystemEntry (Context, FileSystem);
  }

  FreePool (Context);
}

OC_BOOT_CONTEXT *
OcScanForBootEntries (
  IN  OC_PICKER_CONTEXT  *Context
  )
{
  OC_BOOT_CONTEXT                  *BootContext;
  UINTN                            Index;
  LIST_ENTRY                       *Link;
  OC_BOOT_FILESYSTEM               *FileSystem;

  //
  // Obtain the list of filesystems filtered by scan policy.
  //
  BootContext = BuildFileSystemList (
    Context,
    FALSE
    );
  if (BootContext == NULL) {
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "OCB: Found %u potentially bootable filesystems\n", (UINT32) BootContext->FileSystemCount));

  //
  // Create primary boot options from BootOrder.
  //
  if (Context->BootOrder == NULL) {
    Context->BootOrder = InternalGetBootOrderForBooting (
      BootContext->BootVariableGuid,
      &Context->BootOrderCount
      );
  }

  if (Context->BootOrder != NULL) {
    for (Index = 0; Index < Context->BootOrderCount; ++Index) {
      AddBootEntryFromBootOption (BootContext, Context->BootOrder[Index], FALSE);
    }
  }

  DEBUG ((DEBUG_INFO, "OCB: Processing blessed list\n"));

  //
  // Create primary boot options on filesystems without options
  // and alternate boot options on all filesystems.
  //
  for (
    Link = GetFirstNode (&BootContext->FileSystems);
    !IsNull (&BootContext->FileSystems, Link);
    Link = GetNextNode (&BootContext->FileSystems, Link)) {
    FileSystem = BASE_CR (Link, OC_BOOT_FILESYSTEM, Link);

    //
    // No entries, so we process this directory with Apple Bless.
    //
    if (IsListEmpty (&FileSystem->BootEntries)) {
      AddBootEntryFromBless (
        BootContext,
        FileSystem,
        gAppleBootPolicyPredefinedPaths,
        gAppleBootPolicyNumPredefinedPaths,
        FALSE,
        FALSE
        );
    }

    //
    // Record predefined recoveries.
    //
    AddBootEntryFromSelfRecovery (BootContext, FileSystem);
  }

  //
  // Build custom and system options.
  //
  AddFileSystemEntryForCustom (BootContext);

  if (BootContext->BootEntryCount == 0) {
    OcFreeBootContext (BootContext);
    return NULL;
  }

  return BootContext;
}

OC_BOOT_CONTEXT *
OcScanForDefaultBootEntry (
  IN  OC_PICKER_CONTEXT  *Context
  )
{
  OC_BOOT_CONTEXT                  *BootContext;
  UINTN                            Index;
  OC_BOOT_FILESYSTEM               *FileSystem;
  EFI_STATUS                       Status;
  UINTN                            NoHandles;
  EFI_HANDLE                       *Handles;

  //
  // Obtain empty list of filesystems.
  //
  BootContext = BuildFileSystemList (Context, TRUE);
  if (BootContext == NULL) {
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "OCB: Looking up for default entry\n"));

  //
  // Create primary boot options from BootOrder.
  //
  if (Context->BootOrder == NULL) {
    Context->BootOrder = InternalGetBootOrderForBooting (
      BootContext->BootVariableGuid,
      &Context->BootOrderCount
      );
  }

  if (Context->BootOrder != NULL) {
    for (Index = 0; Index < Context->BootOrderCount; ++Index) {
      AddBootEntryFromBootOption (BootContext, Context->BootOrder[Index], TRUE);

      //
      // Return as long as we are good.
      //
      if (BootContext->DefaultEntry != NULL) {
        return BootContext;
      }
    }
  }

  //
  // Obtain filesystems and try processing the remainings.
  //
  NoHandles = 0;
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    &NoHandles,
    &Handles
    );

  DEBUG ((DEBUG_INFO, "OCB: Processing %u blessed list - %r\n", (UINT32) NoHandles, Status));

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NoHandles; ++Index) {
      //
      // Do not add filesystems twice.
      //
      if (InternalFileSystemForHandle (BootContext, Handles[Index], FALSE) != NULL) {
        continue;
      }

      Status = AddFileSystemEntry (
        BootContext,
        Handles[Index],
        &FileSystem
        );
      if (EFI_ERROR (Status)) {
        continue;
      }

      AddBootEntryFromBless (
        BootContext,
        FileSystem,
        gAppleBootPolicyPredefinedPaths,
        gAppleBootPolicyNumPredefinedPaths,
        FALSE,
        FALSE
        );
      if (BootContext->DefaultEntry != NULL) {
        FreePool (Handles);
        return BootContext;
      }

      AddBootEntryFromSelfRecovery (BootContext, FileSystem);
      if (BootContext->DefaultEntry != NULL) {
        FreePool (Handles);
        return BootContext;
      }
    }

    FreePool (Handles);
  }

  //
  // Build custom and system options.
  //
  AddFileSystemEntryForCustom (BootContext);

  if (BootContext->DefaultEntry == NULL) {
    OcFreeBootContext (BootContext);
    return NULL;
  }

  ASSERT (BootContext->BootEntryCount > 0);

  return BootContext;
}

OC_BOOT_ENTRY  **
OcEnumerateEntries (
  IN  OC_BOOT_CONTEXT  *BootContext
  )
{
  OC_BOOT_ENTRY       **Entries;
  UINT32              EntryIndex;
  LIST_ENTRY          *FsLink;
  OC_BOOT_FILESYSTEM  *FileSystem;
  LIST_ENTRY          *EnLink;
  OC_BOOT_ENTRY       *BootEntry;

  Entries = AllocatePool (sizeof (*Entries) * BootContext->BootEntryCount);
  if (Entries == NULL) {
    return NULL;
  }

  EntryIndex = 0;
  for (
    FsLink = GetFirstNode (&BootContext->FileSystems);
    !IsNull (&BootContext->FileSystems, FsLink);
    FsLink = GetNextNode (&BootContext->FileSystems, FsLink)) {
    FileSystem = BASE_CR (FsLink, OC_BOOT_FILESYSTEM, Link);

    for (
      EnLink = GetFirstNode (&FileSystem->BootEntries);
      !IsNull (&FileSystem->BootEntries, EnLink);
      EnLink = GetNextNode (&FileSystem->BootEntries, EnLink)) {
      BootEntry = BASE_CR (EnLink, OC_BOOT_ENTRY, Link);

      ASSERT (EntryIndex < BootContext->BootEntryCount);
      Entries[EntryIndex] = BootEntry;
      BootEntry->EntryIndex = ++EntryIndex;
    }
  }

  ASSERT (EntryIndex == BootContext->BootEntryCount);
  ASSERT (BootContext->DefaultEntry == NULL || BootContext->DefaultEntry->EntryIndex > 0);
  return Entries;
}

EFI_STATUS
OcLoadBootEntry (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  OC_BOOT_ENTRY      *BootEntry,
  IN  EFI_HANDLE         ParentHandle
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 EntryHandle;
  INTERNAL_DMG_LOAD_CONTEXT  DmgLoadContext;

  if ((BootEntry->Type & OC_BOOT_SYSTEM) != 0) {
    ASSERT (BootEntry->SystemAction != NULL);
    return BootEntry->SystemAction ();
  }

  Status = InternalLoadBootEntry (
    Context,
    BootEntry,
    ParentHandle,
    &EntryHandle,
    &DmgLoadContext
    );
  if (!EFI_ERROR (Status)) {
    Status = Context->StartImage (BootEntry, EntryHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCB: StartImage failed - %r\n", Status));
      //
      // Unload dmg if any.
      //
      InternalUnloadDmg (&DmgLoadContext);
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCB: LoadImage failed - %r\n", Status));
  }

  return Status;
}

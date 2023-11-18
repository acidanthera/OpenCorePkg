/** @file
  Copyright (C) 2019-2022, vit9696, mikebeaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "BootManagementInternal.h"

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#include <IndustryStandard/AppleCsrConfig.h>

#include <Guid/AppleVariable.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/Gpt.h>
#include <Guid/OcVariable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVariableLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

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
  EFI_STATUS  Status;

  EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *PrevDevicePath;

  EFI_HANDLE         FileSystemHandle;
  EFI_FILE_PROTOCOL  *File;
  EFI_FILE_INFO      *FileInfo;
  BOOLEAN            IsRootPath;
  BOOLEAN            IsDirectory;

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
  IsDirectory    = FALSE;
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
    Status              = gBS->LocateDevicePath (
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
    *FileSystem = InternalFileSystemForHandle (BootContext, FileSystemHandle, LazyScan, NULL);
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
    FileInfo = OcGetFileInfo (
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
  Check boot entry visibility by device path.

  @param[in]  Context      Picker context.
  @param[in]  DevicePath   Device path of the entry.

  @return Entry visibility
**/
STATIC
INTERNAL_ENTRY_VISIBILITY
ReadEntryVisibility (
  IN OC_PICKER_CONTEXT         *Context,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS  Status;
  CHAR8       *Visibility;
  CHAR8       *VisibilityCommand;
  CHAR8       *Walker;
  UINTN       IdentifierLength;

  //
  // Allow root location as well as leaf, because this is a non-Apple file which will
  // get deleted at update if placed next to boot.efi in macOS.
  // Leaf (next to bootloader) is recommended location for non-macOS.
  //
  Status = OcGetBootEntryFileFromDevicePath (
             DevicePath,
             L".contentVisibility",
             "visibility",
             OC_MAX_CONTENT_VISIBILITY_SIZE,
             0,
             (VOID **)&Visibility,
             NULL,
             TRUE,
             TRUE
             );

  if (EFI_ERROR (Status)) {
    return BootEntryNormal;
  }

  //
  // Allow for terminating new line, but be strict about it -
  // after removing this, things must match exactly.
  //
  Walker = AsciiStrStr (Visibility, "\r");
  if (Walker != NULL) {
    *Walker = '\0';
  }

  Walker = AsciiStrStr (Visibility, "\n");
  if (Walker != NULL) {
    *Walker = '\0';
  }

  Walker = AsciiStrStr (Visibility, ":");
  if (Walker == NULL) {
    VisibilityCommand = Visibility;
  } else {
    if (*(Context->InstanceIdentifier) == '\0') {
      DEBUG ((DEBUG_INFO, "OCB: No InstanceIdentifier, ignoring qualified visibility\n"));
      FreePool (Visibility);
      return BootEntryNormal;
    }

    *Walker++         = '\0';
    VisibilityCommand = Walker;
    Walker            = Visibility;

    IdentifierLength = AsciiStrLen (Context->InstanceIdentifier);
    Status           = EFI_NOT_FOUND;
    do {
      if (  (AsciiStrnCmp (Walker, Context->InstanceIdentifier, IdentifierLength) == 0)
         && ((Walker[IdentifierLength] == '\0') || (Walker[IdentifierLength] == ',')))
      {
        Status = EFI_SUCCESS;
        break;
      }

      Walker = AsciiStrStr (Walker, ",");
      if (Walker != NULL) {
        ++Walker;
      }
    } while (Walker != NULL);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCB: \"%a\" not present in \"%a\" ignoring visibility\n", Context->InstanceIdentifier, Visibility));
      FreePool (Visibility);
      return BootEntryNormal;
    }
  }

  if (AsciiStrCmp (VisibilityCommand, "Disabled") == 0) {
    FreePool (Visibility);
    return BootEntryDisabled;
  }

  if (AsciiStrCmp (VisibilityCommand, "Auxiliary") == 0) {
    FreePool (Visibility);
    return BootEntryAuxiliary;
  }

  DEBUG ((DEBUG_INFO, "OCB: Discovered unsupported visibility \"%a\"\n", VisibilityCommand));

  FreePool (Visibility);
  return BootEntryNormal;
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
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN OUT OC_BOOT_FILESYSTEM  *FileSystem,
  IN     OC_BOOT_ENTRY       *BootEntry
  )
{
  CHAR16  *TextDevicePath;

  DEBUG_CODE_BEGIN ();

  if (BootEntry->DevicePath != NULL) {
    TextDevicePath = ConvertDevicePathToText (BootEntry->DevicePath, FALSE, FALSE);
  } else {
    TextDevicePath = NULL;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Registering entry %s [%a] (T:%d|F:%d|G:%d|E:%d|B:%d) - %s\n",
    BootEntry->Name,
    BootEntry->Flavour,
    BootEntry->Type,
    BootEntry->IsFolder,
    BootEntry->IsGeneric,
    BootEntry->IsExternal,
    BootEntry->IsBootEntryProtocol,
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
  // If no options were previously found and this entry type
  // is allowed in this context then this is the default one.
  //
  if (  (BootContext->DefaultEntry == NULL)
     && ((BootEntry->Type & OC_BOOT_EXTERNAL_TOOL) == 0)
     && (  ((BootEntry->Type & OC_BOOT_SYSTEM) == 0)
        || (BootContext->PickerContext->PickerCommand == OcPickerProtocolHotKey)
           )
        )
  {
    BootContext->DefaultEntry = BootEntry;
  }

  //
  // For tools and system options we are done.
  //
  if ((BootEntry->Type & (OC_BOOT_EXTERNAL_TOOL | OC_BOOT_SYSTEM)) != 0) {
    return;
  }

  //
  // Set override picker commands.
  //
  if (BootContext->PickerContext->PickerCommand == OcPickerBootApple) {
    if (  (BootContext->DefaultEntry->Type != OC_BOOT_APPLE_OS)
       && (BootEntry->Type == OC_BOOT_APPLE_OS))
    {
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
  EFI_STATUS                 Status;
  OC_BOOT_ENTRY              *BootEntry;
  OC_BOOT_ENTRY_TYPE         EntryType;
  LIST_ENTRY                 *Link;
  OC_BOOT_ENTRY              *ExistingEntry;
  CHAR16                     *TextDevicePath;
  INTERNAL_ENTRY_VISIBILITY  Visibility;
  BOOLEAN                    IsFolder;
  BOOLEAN                    IsGeneric;
  BOOLEAN                    IsReallocated;

  EntryType = OcGetBootDevicePathType (DevicePath, &IsFolder, &IsGeneric);

  if (IsFolder && (BootContext->PickerContext->DmgLoading == OcDmgLoadingDisabled)) {
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

  TextDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);

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
  if (!RecoveryPart && (EntryType == OC_BOOT_APPLE_RECOVERY)) {
    FileSystem->HasSelfRecovery = TRUE;
  }

  //
  // Do not add recoveries when not requested (e.g. can be HFS+ recovery).
  //
  if (BootContext->PickerContext->HideAuxiliary && (EntryType == OC_BOOT_APPLE_RECOVERY)) {
    DEBUG ((DEBUG_INFO, "OCB: Discarding recovery entry due to auxiliary\n"));
    if (IsReallocated) {
      FreePool (DevicePath);
    }

    return EFI_UNSUPPORTED;
  }

  //
  // Do not add Time Machine when not requested.
  //
  if (BootContext->PickerContext->HideAuxiliary && (EntryType == OC_BOOT_APPLE_TIME_MACHINE)) {
    DEBUG ((DEBUG_INFO, "OCB: Discarding time machine entry due to auxiliary\n"));
    if (IsReallocated) {
      FreePool (DevicePath);
    }

    return EFI_UNSUPPORTED;
  }

  //
  // Skip disabled entries, like OpenCore bootloader.
  //
  Visibility = ReadEntryVisibility (BootContext->PickerContext, DevicePath);
  if (Visibility == BootEntryDisabled) {
    DEBUG ((DEBUG_INFO, "OCB: Discarding disabled entry by visibility\n"));
    if (IsReallocated) {
      FreePool (DevicePath);
    }

    return EFI_UNSUPPORTED;
  }

  //
  // Skip custom auxiliary entries.
  //
  if ((Visibility == BootEntryAuxiliary) && BootContext->PickerContext->HideAuxiliary) {
    DEBUG ((DEBUG_INFO, "OCB: Discarding auxiliary entry by visibility\n"));
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
         Link = GetNextNode (&FileSystem->BootEntries, Link))
    {
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

  Status = InternalDescribeBootEntry (BootContext, BootEntry);
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
  Release boot entry contents allocated from pool.

  @param[in,out]  BootEntry      Located boot entry.
**/
STATIC
VOID
FreeBootEntry (
  IN OC_BOOT_ENTRY  *BootEntry
  )
{
  if (BootEntry->DevicePath != NULL) {
    FreePool (BootEntry->DevicePath);
    BootEntry->DevicePath = NULL;
  }

  if (BootEntry->Id != NULL) {
    FreePool (BootEntry->Id);
    BootEntry->Id = NULL;
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

  if (BootEntry->Flavour != NULL) {
    FreePool (BootEntry->Flavour);
    BootEntry->Flavour = NULL;
  }

  FreePool (BootEntry);
}

/**
  Create bootable entry from custom entry.

  @param[in,out] BootContext   Context of filesystems.
  @param[in,out] FileSystem    Filesystem to add custom entry.
  @param[in]     CustomEntry   Custom entry.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
InternalAddBootEntryFromCustomEntry (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN OUT OC_BOOT_FILESYSTEM  *FileSystem,
  IN     OC_PICKER_ENTRY     *CustomEntry,
  IN     BOOLEAN             IsBootEntryProtocol
  )
{
  EFI_STATUS                       Status;
  OC_BOOT_ENTRY                    *BootEntry;
  CHAR16                           *PathName;
  FILEPATH_DEVICE_PATH             *FilePath;
  CHAR8                            *ContentFlavour;
  CHAR16                           *BootDirectoryName;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFileSystem;
  CONST EFI_PARTITION_ENTRY        *PartitionEntry;

  if (CustomEntry->Auxiliary && BootContext->PickerContext->HideAuxiliary) {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Not adding hidden auxiliary entry %a (%a|B:%d) -> %a\n",
      CustomEntry->Name,
      CustomEntry->Tool ? "tool" : "os",
      IsBootEntryProtocol,
      CustomEntry->Path
      ));
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate, initialise, and describe boot entry.
  //
  BootEntry = AllocateZeroPool (sizeof (*BootEntry));
  if (BootEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BootEntry->IsExternal = FileSystem->External;

  if (CustomEntry->Id != NULL) {
    BootEntry->Id = AsciiStrCopyToUnicode (CustomEntry->Id, 0);
    if (BootEntry->Id == NULL) {
      FreeBootEntry (BootEntry);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  ASSERT (CustomEntry->Name != NULL);
  BootEntry->Name = AsciiStrCopyToUnicode (CustomEntry->Name, 0);
  if (BootEntry->Name == NULL) {
    FreeBootEntry (BootEntry);
    return EFI_OUT_OF_RESOURCES;
  }

  if (!CustomEntry->ExternalSystemAction && !CustomEntry->SystemAction) {
    ASSERT (CustomEntry->Path != NULL);
    PathName = AsciiStrCopyToUnicode (CustomEntry->Path, 0);
    if (PathName == NULL) {
      FreeBootEntry (BootEntry);
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    ASSERT (CustomEntry->Path == NULL);
    PathName = NULL;
  }

  ASSERT (CustomEntry->Flavour != NULL);
  BootEntry->Flavour = AllocateCopyPool (AsciiStrSize (CustomEntry->Flavour), CustomEntry->Flavour);
  if (BootEntry->Flavour == NULL) {
    FreeBootEntry (BootEntry);
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Adding custom entry %s (%a|B:%d) -> %a\n",
    BootEntry->Name,
    CustomEntry->ExternalSystemAction != NULL ? "ext-action" : (CustomEntry->SystemAction != NULL ? "action" : (CustomEntry->Tool ? "tool" : "os")),
    IsBootEntryProtocol,
    CustomEntry->Path
    ));

  if (CustomEntry->ExternalSystemAction) {
    BootEntry->Type                        = OC_BOOT_EXTERNAL_SYSTEM;
    BootEntry->ExternalSystemAction        = CustomEntry->ExternalSystemAction;
    BootEntry->ExternalSystemGetDevicePath = CustomEntry->ExternalSystemGetDevicePath;
    BootEntry->AudioBasePath               = CustomEntry->AudioBasePath;
    BootEntry->AudioBaseType               = CustomEntry->AudioBaseType;
    BootEntry->IsExternal                  = CustomEntry->External;
    BootEntry->DevicePath                  = DuplicateDevicePath (CustomEntry->ExternalSystemDevicePath);

    if (BootEntry->DevicePath == NULL) {
      FreeBootEntry (BootEntry);
      return EFI_OUT_OF_RESOURCES;
    }
  } else if (CustomEntry->SystemAction) {
    BootEntry->Type          = OC_BOOT_SYSTEM;
    BootEntry->SystemAction  = CustomEntry->SystemAction;
    BootEntry->AudioBasePath = CustomEntry->AudioBasePath;
    BootEntry->AudioBaseType = CustomEntry->AudioBaseType;
  } else if (CustomEntry->Tool) {
    BootEntry->Type = OC_BOOT_EXTERNAL_TOOL;
    UnicodeUefiSlashes (PathName);
    BootEntry->PathName = PathName;
  } else {
    BootEntry->Type = OC_BOOT_EXTERNAL_OS;

    //
    // For boot entry protocol path is relative to device root,
    // for user entry path is absolute device path.
    //
    if (IsBootEntryProtocol) {
      UnicodeUefiSlashes (PathName);
      BootEntry->DevicePath = FileDevicePath (FileSystem->Handle, PathName);
    } else {
      BootEntry->DevicePath = ConvertTextToDevicePath (PathName);
    }

    FreePool (PathName);
    if (BootEntry->DevicePath == NULL) {
      FreeBootEntry (BootEntry);
      return EFI_OUT_OF_RESOURCES;
    }

    FilePath = (FILEPATH_DEVICE_PATH *)(
                                        FindDevicePathNodeWithType (
                                          BootEntry->DevicePath,
                                          MEDIA_DEVICE_PATH,
                                          MEDIA_FILEPATH_DP
                                          )
                                        );
    if (FilePath == NULL) {
      DEBUG ((
        DEBUG_WARN,
        "OCB: Invalid device path, not adding entry %a\n",
        CustomEntry->Name
        ));
      FreeBootEntry (BootEntry);
      return EFI_UNSUPPORTED;
    }

    BootEntry->PathName = AllocateCopyPool (
                            OcFileDevicePathNameSize (FilePath),
                            FilePath->PathName
                            );
    if (BootEntry->PathName == NULL) {
      FreeBootEntry (BootEntry);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // NOTE: It is not currently necessary/useful to apply .contentDetails around here because:
    //  a) Entries have user-specified names already.
    //  b) OpenLinuxBoot needs to read the label file early, when allowed by picker attributes,
    //     so it can be used for pretty name with kernel version appended when required.
    // If any future boot entry protocol drivers do want .contentDetails applied for them, we will need
    // to pass back an entry flag indicating whether .contentDetails has already been applied or not.
    //

    //
    // Try to get content flavour from file.
    // If enabled and present, .contentFlavour always overrides flavour from boot entry protocol,
    // but is only applied to Entries if they have flavour Auto.
    //
    if (  ((BootContext->PickerContext->PickerAttributes & OC_ATTR_USE_FLAVOUR_ICON) != 0)
       && (IsBootEntryProtocol || (AsciiStrCmp (BootEntry->Flavour, OC_FLAVOUR_AUTO) == 0)))
    {
      Status = OcBootPolicyDevicePathToDirPath (
                 BootEntry->DevicePath,
                 &BootDirectoryName,
                 &SimpleFileSystem
                 );

      if (!EFI_ERROR (Status)) {
        ContentFlavour = InternalGetContentFlavour (SimpleFileSystem, BootDirectoryName);

        if (ContentFlavour != NULL) {
          //
          // 'Auto' read from file means do not override.
          //
          if (AsciiStrCmp (ContentFlavour, OC_FLAVOUR_AUTO) == 0) {
            FreePool (ContentFlavour);
          } else {
            if (BootEntry->Flavour != NULL) {
              FreePool (BootEntry->Flavour);
            }

            BootEntry->Flavour = ContentFlavour;
          }
        }

        //
        // There is no need for the additional flavour fixup from BootEntryInfo.c, since type
        // OC_BOOT_EXTERNAL_OS does not need fixing up, and already determines our voiceover.
        //

        FreePool (BootDirectoryName);
      }
    }
  }

  BootEntry->LaunchInText     = CustomEntry->TextMode;
  BootEntry->ExposeDevicePath = CustomEntry->RealPath;
  BootEntry->FullNvramAccess  = CustomEntry->FullNvramAccess;

  if ((BootEntry->ExternalSystemAction != NULL) || (BootEntry->SystemAction != NULL)) {
    ASSERT (CustomEntry->Arguments == NULL);
  } else {
    ASSERT (CustomEntry->Arguments != NULL);
    BootEntry->LoadOptionsSize = (UINT32)AsciiStrLen (CustomEntry->Arguments);
    if (BootEntry->LoadOptionsSize > 0) {
      BootEntry->LoadOptions = AllocateCopyPool (
                                 BootEntry->LoadOptionsSize + 1,
                                 CustomEntry->Arguments
                                 );
      if (BootEntry->LoadOptions == NULL) {
        BootEntry->LoadOptionsSize = 0;
      }
    }
  }

  BootEntry->IsCustom            = TRUE;
  BootEntry->IsBootEntryProtocol = IsBootEntryProtocol;
  if (IsBootEntryProtocol && (BootEntry->ExternalSystemAction == NULL) && (BootEntry->SystemAction == NULL)) {
    PartitionEntry = OcGetGptPartitionEntry (FileSystem->Handle);
    if (PartitionEntry == NULL) {
      CopyGuid (&BootEntry->UniquePartitionGUID, &gEfiPartTypeUnusedGuid);
    } else {
      CopyGuid (&BootEntry->UniquePartitionGUID, &PartitionEntry->UniquePartitionGUID);
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
                  (VOID **)&HdDevicePath
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
                    (VOID **)&SimpleFs
                    );

    if (!EFI_ERROR (Status)) {
      Status = SimpleFs->OpenVolume (SimpleFs, &Root);
      if (!EFI_ERROR (Status)) {
        Status = OcGetBooterFromPredefinedPathList (
                   FileSystem->Handle,
                   Root,
                   (CONST CHAR16 **)BootContext->PickerContext->CustomBootPaths,
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
  Status           = EFI_NOT_FOUND;
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
      FileSystem->RecoveryFs = InternalFileSystemForHandle (BootContext, RecoveryDeviceHandle, LazyScan, NULL);
    }

    //
    // If new recovery is not on the same volume or not allowed, then something went wrong, skip it.
    // This is technically also a performance optimisation allowing us not to lookup recovery fs every time.
    //
    if ((FileSystem->RecoveryFs == NULL) || (FileSystem->RecoveryFs->Handle != RecoveryDeviceHandle)) {
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
  Create bootable entries from recovery files (com.apple.recovery.boot) on the volume.

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
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

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

  @param[in,out] BootContext                Context of filesystems.
  @param[in]     BootOption                 Boot option number.
  @param[in]     LazyScan                   Lazy filesystem scanning.
  @param[in,out] CustomFileSystem           File system on which to add user defined custom option.
                                            If non-NULL still searching for first (normally only) OC
                                            custom entry, either user defined or entry protocol.
  @param[out]    CustomIndex                Index of custom user defined entry, if matched.
  @param[in]     EntryProtocolHandles       Installed Boot Entry Protocol handles.
  @param[in]     EntryProtocolHandleCount   Installed Boot Entry Protocol handle count.
  @param[out]    EntryProtocolPartuuid      Unique partition UUID of parition with entry protocol
                                            custom entry, if matched.
  @param[out]    EntryProtocolId            Id of entry protocol custom entry, if matched.

  @retval EFI_SUCCESS if at least one option was added.
**/
STATIC
EFI_STATUS
AddBootEntryFromBootOption (
  IN OUT OC_BOOT_CONTEXT *BootContext,
  IN     UINT16 BootOption,
  IN     BOOLEAN LazyScan,
  IN OUT OC_BOOT_FILESYSTEM *CustomFileSystem,
  OUT UINT32 *CustomIndex, OPTIONAL
  IN     EFI_HANDLE          *EntryProtocolHandles,
  IN     UINTN               EntryProtocolHandleCount,
  OUT EFI_GUID            *EntryProtocolPartuuid, OPTIONAL
  OUT CHAR16              **EntryProtocolId          OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *ExpandedDevicePath;
  EFI_HANDLE                FileSystemHandle;
  OC_BOOT_FILESYSTEM        *FileSystem;
  UINTN                     DevicePathSize;
  CHAR16                    *TextDevicePath;
  INTN                      NumPatchedNodes;
  BOOLEAN                   IsAppleLegacy;
  BOOLEAN                   IsAppleLegacyHandled;
  BOOLEAN                   IsRoot;
  EFI_LOAD_OPTION           *LoadOption;
  UINTN                     LoadOptionSize;
  UINT32                    Index;
  INTN                      CmpResult;
  UINTN                     NoHandles;
  EFI_HANDLE                *Handles;

  CONST EFI_PARTITION_ENTRY            *PartitionEntry;
  CONST OC_CUSTOM_BOOT_DEVICE_PATH     *CustomDevPath;
  CONST OC_ENTRY_PROTOCOL_DEVICE_PATH  *EntryProtocolDevPath;

  DEBUG ((DEBUG_INFO, "OCB: Building entry from Boot%04x\n", BootOption));

  //
  // Obtain original device path.
  // Discard load options for security reasons.
  // Also discard boot name to avoid confusion.
  //
  LoadOption = OcGetBootOptionData (
                 &LoadOptionSize,
                 BootOption,
                 BootContext->BootVariableGuid
                 );
  if (LoadOption == NULL) {
    return EFI_NOT_FOUND;
  }

  DevicePath = InternalGetBootOptionPath (
                 LoadOption,
                 LoadOptionSize
                 );
  if (DevicePath == NULL) {
    FreePool (LoadOption);
    return EFI_NOT_FOUND;
  }

  //
  // Re-use the Load Option buffer for the Device Path.
  //
  CopyMem (LoadOption, DevicePath, LoadOption->FilePathListLength);
  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)LoadOption;

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
               (VOID **)&DevicePath,
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
  IsAppleLegacyHandled = FALSE;
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

    //
    // Attempt to handle detected legacy OS via Apple legacy interface.
    //
    RemainingDevicePath = DevicePath;
    DevicePath          = OcDiskFindActiveMbrPartitionPath (
                            DevicePath,
                            &DevicePathSize,
                            &FileSystemHandle
                            );

    //
    // Disk with MBR or hybrid MBR was detected.
    //
    if (DevicePath != NULL) {
      TextDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
      if (TextDevicePath != NULL) {
        //
        // Add entry from externally provided legacy interface.
        // Boot entry ID must be active partition Device Path.
        //
        Status = OcAddEntriesFromBootEntryProtocol (
                   BootContext,
                   CustomFileSystem,
                   EntryProtocolHandles,
                   EntryProtocolHandleCount,
                   TextDevicePath,
                   TRUE,
                   FALSE
                   );
        if (!EFI_ERROR (Status)) {
          if (EntryProtocolId != NULL) {
            *EntryProtocolId = TextDevicePath;
          }

          FileSystem           = CustomFileSystem;
          IsAppleLegacyHandled = TRUE;
        } else {
          FreePool (TextDevicePath);
        }
      }
    }

    if (!IsAppleLegacyHandled) {
      //
      // Boot option was set to Apple legacy interface incorrectly by macOS.
      // This will occur on Macs that normally boot Windows in legacy mode,
      // but have Windows installed in UEFI mode.
      //
      // Locate the ESP from the BootCampHD Device Path instead.
      //
      DevicePath = OcDiskFindSystemPartitionPath (
                     RemainingDevicePath,
                     &DevicePathSize,
                     &FileSystemHandle
                     );

      //
      // Ensure that we are allowed to boot from this filesystem.
      //
      if (DevicePath != NULL) {
        FileSystem = InternalFileSystemForHandle (BootContext, FileSystemHandle, LazyScan, NULL);
        if (FileSystem == NULL) {
          DevicePath = NULL;
        }
      }
    }

    FreePool (RemainingDevicePath);

    //
    // This is obviously always a Root Device Path.
    //
    IsRoot = TRUE;

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
    DevicePathSize     -= END_DEVICE_PATH_LENGTH;
    RemainingDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)DevicePath + DevicePathSize);
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

    if ((ExpandedDevicePath == NULL) && (CustomFileSystem != NULL)) {
      //
      // If non-standard device path, attempt to pre-construct a user config
      // custom entry found in BOOT#### so it can be set as default.
      //
      ASSERT (CustomIndex == NULL || *CustomIndex == MAX_UINT32);

      CustomDevPath = InternalGetOcCustomDevPath (DevicePath);

      if (CustomDevPath != NULL) {
        for (Index = 0; Index < BootContext->PickerContext->AllCustomEntryCount; ++Index) {
          CmpResult = MixedStrCmp (
                        CustomDevPath->EntryName.PathName,
                        BootContext->PickerContext->CustomEntries[Index].Name
                        );
          if (CmpResult == 0) {
            if (CustomIndex != NULL) {
              *CustomIndex = Index;
            }

            InternalAddBootEntryFromCustomEntry (
              BootContext,
              CustomFileSystem,
              &BootContext->PickerContext->CustomEntries[Index],
              FALSE
              );
            break;
          }
        }
      } else {
        //
        // If still unknown device path, attempt to pre-construct an entry protocol
        // entry found in BOOT#### so it can be set as default.
        //
        ASSERT (EntryProtocolId == NULL || *EntryProtocolId == NULL);
        ASSERT ((EntryProtocolPartuuid == NULL) == (EntryProtocolId == NULL));

        EntryProtocolDevPath = InternalGetOcEntryProtocolDevPath (DevicePath);

        if (EntryProtocolDevPath != NULL) {
          //
          // Search for ID on matching device only.
          // Note that on, e.g., OVMF, devices do not have PartitionEntry, therefore
          // the first matching entry protocol ID on any filesystem will match.
          //
          NoHandles = 0;
          Status    = gBS->LocateHandleBuffer (
                             ByProtocol,
                             &gEfiSimpleFileSystemProtocolGuid,
                             NULL,
                             &NoHandles,
                             &Handles
                             );

          if (!EFI_ERROR (Status)) {
            for (Index = 0; Index < NoHandles; ++Index) {
              PartitionEntry = OcGetGptPartitionEntry (Handles[Index]);

              if (CompareGuid (
                    (PartitionEntry == NULL) ? &gEfiPartTypeUnusedGuid : &PartitionEntry->UniquePartitionGUID,
                    &EntryProtocolDevPath->Partuuid
                    )
                  )
              {
                FileSystem = InternalFileSystemForHandle (BootContext, Handles[Index], TRUE, NULL);
                if (FileSystem == NULL) {
                  continue;
                }

                Status = OcAddEntriesFromBootEntryProtocol (
                           BootContext,
                           FileSystem,
                           EntryProtocolHandles,
                           EntryProtocolHandleCount,
                           EntryProtocolDevPath->EntryName.PathName,
                           TRUE,
                           FALSE
                           );

                if (!EFI_ERROR (Status)) {
                  if (EntryProtocolPartuuid != NULL) {
                    if (PartitionEntry == NULL) {
                      CopyGuid (EntryProtocolPartuuid, &gEfiPartTypeUnusedGuid);
                    } else {
                      CopyGuid (EntryProtocolPartuuid, &PartitionEntry->UniquePartitionGUID);
                    }
                  }

                  if (EntryProtocolId != NULL) {
                    *EntryProtocolId = AllocateCopyPool (StrSize (EntryProtocolDevPath->EntryName.PathName), EntryProtocolDevPath->EntryName.PathName);
                    //
                    // If NULL allocated, just continue as if we had not matched.
                    //
                  }

                  break;
                }
              }
            }

            FreePool (Handles);
          }
        }
      }
    }

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
    DevicePath          = ExpandShortFormBootPath (
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

  if (IsAppleLegacyHandled) {
    return EFI_SUCCESS;
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
  IN OUT OC_BOOT_CONTEXT  *BootContext,
  IN     EFI_HANDLE       FileSystemHandle,
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
                     (VOID **)&DevicePath
                     );
  if (!EFI_ERROR (TmpStatus)) {
    TextDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
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
OC_BOOT_FILESYSTEM *
CreateFileSystemForCustom (
  IN OUT CONST OC_BOOT_CONTEXT  *BootContext
  )
{
  OC_BOOT_FILESYSTEM  *FileSystem;

  FileSystem = AllocateZeroPool (sizeof (*FileSystem));
  if (FileSystem == NULL) {
    return NULL;
  }

  FileSystem->Handle = OC_CUSTOM_FS_HANDLE;
  InitializeListHead (&FileSystem->BootEntries);

  DEBUG ((
    DEBUG_INFO,
    "OCB: Adding fs %p for %u custom entries and BEP%a\n",
    OC_CUSTOM_FS_HANDLE,
    BootContext->PickerContext->AllCustomEntryCount,
    BootContext->PickerContext->HideAuxiliary ? " (aux hidden)" : " (aux shown)"
    ));

  return FileSystem;
}

//
// @retval EFI_SUCCESS           One or more entries added.
// @retval EFI_NOT_FOUND         No entries added.
//
STATIC
EFI_STATUS
AddFileSystemEntryForCustom (
  IN OUT OC_BOOT_CONTEXT     *BootContext,
  IN OUT OC_BOOT_FILESYSTEM  *FileSystem,
  IN     UINT32              PrecreatedCustomIndex
  )
{
  EFI_STATUS  ReturnStatus;
  EFI_STATUS  Status;
  UINTN       Index;

  ReturnStatus = EFI_NOT_FOUND;

  for (Index = 0; Index < BootContext->PickerContext->AllCustomEntryCount; ++Index) {
    //
    // Skip the custom boot entry that has already been created.
    //
    if (Index == PrecreatedCustomIndex) {
      continue;
    }

    Status = InternalAddBootEntryFromCustomEntry (
               BootContext,
               FileSystem,
               &BootContext->PickerContext->CustomEntries[Index],
               FALSE
               );

    if (!EFI_ERROR (Status)) {
      ReturnStatus = EFI_SUCCESS;
    }
  }

  return ReturnStatus;
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
    Link      = GetFirstNode (&FileSystemEntry->BootEntries);
    BootEntry = BASE_CR (Link, OC_BOOT_ENTRY, Link);
    RemoveEntryList (Link);
    FreeBootEntry (BootEntry);
  }

  FreePool (FileSystemEntry);
}

OC_BOOT_FILESYSTEM *
InternalFileSystemForHandle (
  IN  OC_BOOT_CONTEXT  *BootContext,
  IN  EFI_HANDLE       FileSystemHandle,
  IN  BOOLEAN          LazyScan,
  OUT BOOLEAN          *AlreadySeen         OPTIONAL
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Link;
  OC_BOOT_FILESYSTEM  *FileSystem;

  if (AlreadySeen != NULL) {
    *AlreadySeen = FALSE;
  }

  for (
       Link = GetFirstNode (&BootContext->FileSystems);
       !IsNull (&BootContext->FileSystems, Link);
       Link = GetNextNode (&BootContext->FileSystems, Link))
  {
    FileSystem = BASE_CR (Link, OC_BOOT_FILESYSTEM, Link);

    if (FileSystem->Handle == FileSystemHandle) {
      DEBUG ((DEBUG_INFO, "OCB: Matched fs %p%a\n", FileSystemHandle, LazyScan ? " (lazy)" : ""));
      if (AlreadySeen != NULL) {
        *AlreadySeen = TRUE;
      }

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

  BootContext = AllocatePool (sizeof (*BootContext));
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
    Link       = GetFirstNode (&Context->FileSystems);
    FileSystem = BASE_CR (Link, OC_BOOT_FILESYSTEM, Link);
    FreeFileSystemEntry (Context, FileSystem);
  }

  FreePool (Context);
}

EFI_STATUS
OcSetDefaultBootRecovery (
  IN OUT OC_BOOT_CONTEXT  *BootContext
  )
{
  LIST_ENTRY          *FsLink;
  OC_BOOT_FILESYSTEM  *FileSystem;
  LIST_ENTRY          *EnLink;
  OC_BOOT_ENTRY       *BootEntry;
  OC_BOOT_ENTRY       *FirstRecovery;
  OC_BOOT_ENTRY       *RecoveryInitiator;
  BOOLEAN             UseInitiator;

  FirstRecovery = NULL;
  UseInitiator  = BootContext->PickerContext->RecoveryInitiator != NULL;

  //
  // This could technically use AppleBootPolicy recovery getting function,
  // but it will do extra disk i/o and will not work with HFS+ recovery.
  //
  for (
       FsLink = GetFirstNode (&BootContext->FileSystems);
       !IsNull (&BootContext->FileSystems, FsLink);
       FsLink = GetNextNode (&BootContext->FileSystems, FsLink))
  {
    FileSystem = BASE_CR (FsLink, OC_BOOT_FILESYSTEM, Link);

    RecoveryInitiator = NULL;

    for (
         EnLink = GetFirstNode (&FileSystem->BootEntries);
         !IsNull (&FileSystem->BootEntries, EnLink);
         EnLink = GetNextNode (&FileSystem->BootEntries, EnLink))
    {
      BootEntry = BASE_CR (EnLink, OC_BOOT_ENTRY, Link);

      //
      // Record first found recovery in case we find nothing.
      //
      if ((FirstRecovery == NULL) && (BootEntry->Type == OC_BOOT_APPLE_RECOVERY)) {
        FirstRecovery = BootEntry;
        ASSERT (BootEntry->DevicePath != NULL);

        if (!UseInitiator) {
          DebugPrintDevicePath (DEBUG_INFO, "OCB: Using first recovery path", BootEntry->DevicePath);
          BootContext->DefaultEntry = FirstRecovery;
          return EFI_SUCCESS;
        } else {
          DebugPrintDevicePath (DEBUG_INFO, "OCB: Storing first recovery path", BootEntry->DevicePath);
        }
      }

      if ((RecoveryInitiator != NULL) && (BootEntry->Type == OC_BOOT_APPLE_RECOVERY)) {
        DebugPrintDevicePath (DEBUG_INFO, "OCB: Using initiator recovery path", BootEntry->DevicePath);
        BootContext->DefaultEntry = BootEntry;
        return EFI_SUCCESS;
      }

      if (  (BootEntry->Type == OC_BOOT_APPLE_OS)
         && UseInitiator
         && IsDevicePathEqual (
              BootContext->PickerContext->RecoveryInitiator,
              BootEntry->DevicePath
              ))
      {
        DebugPrintDevicePath (DEBUG_INFO, "OCB: Found initiator", BootEntry->DevicePath);
        RecoveryInitiator = BootEntry;
      }
    }

    if (RecoveryInitiator != NULL) {
      if (FirstRecovery != NULL) {
        DEBUG ((DEBUG_INFO, "OCB: Using first recovery path for no initiator"));
        BootContext->DefaultEntry = FirstRecovery;
        return EFI_SUCCESS;
      }

      DEBUG ((DEBUG_INFO, "OCB: Looking for any first recovery due to no initiator"));
      UseInitiator = FALSE;
    }
  }

  return EFI_NOT_FOUND;
}

OC_BOOT_CONTEXT *
OcScanForBootEntries (
  IN  OC_PICKER_CONTEXT  *Context
  )
{
  OC_BOOT_CONTEXT            *BootContext;
  UINTN                      Index;
  LIST_ENTRY                 *Link;
  OC_BOOT_FILESYSTEM         *FileSystem;
  OC_BOOT_FILESYSTEM         *CustomFileSystem;
  OC_BOOT_FILESYSTEM         *CustomFileSystemDefault;
  UINT32                     DefaultCustomIndex;    ///< Index if Tools or Entries item is pre-created
  CHAR16                     *DefaultEntryId;       ///< ID if boot entry protocol item is pre-created
  EFI_GUID                   DefaultEntryPartuuid;  ///< PARTUUID for pre-created boot entry protocol item
  BOOLEAN                    IsDefaultEntryProtocolPartition;
  EFI_HANDLE                 *EntryProtocolHandles;
  UINTN                      EntryProtocolHandleCount;
  CONST EFI_PARTITION_ENTRY  *PartitionEntry;

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

  DEBUG ((DEBUG_INFO, "OCB: Found %u potentially bootable filesystems\n", (UINT32)BootContext->FileSystemCount));

  //
  // Locate loaded boot entry protocol drivers.
  //
  OcLocateBootEntryProtocolHandles (&EntryProtocolHandles, &EntryProtocolHandleCount);

  //
  // Create primary boot options from BootOrder.
  //
  if (Context->BootOrder == NULL) {
    Context->BootOrder = InternalGetBootOrderForBooting (
                           BootContext->BootVariableGuid,
                           Context->BlacklistAppleUpdate,
                           &Context->BootOrderCount,
                           FALSE
                           );
  }

  CustomFileSystem = CreateFileSystemForCustom (BootContext);

  //
  // Delay CustomFileSystem insertion to have custom entries at the end.
  //

  DefaultCustomIndex = MAX_UINT32;
  DefaultEntryId     = NULL;

  if (Context->BootOrder != NULL) {
    CustomFileSystemDefault = CustomFileSystem;

    for (Index = 0; Index < Context->BootOrderCount; ++Index) {
      AddBootEntryFromBootOption (
        BootContext,
        Context->BootOrder[Index],
        FALSE,
        CustomFileSystemDefault,
        &DefaultCustomIndex,
        EntryProtocolHandles,
        EntryProtocolHandleCount,
        &DefaultEntryPartuuid,
        &DefaultEntryId
        );

      //
      // Pre-create at most one custom entry. Under normal circumstances, no
      // more than one should exist as a boot option anyway.
      //
      if ((DefaultCustomIndex != MAX_UINT32) || (DefaultEntryId != NULL)) {
        CustomFileSystemDefault = NULL;
      }
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
       Link = GetNextNode (&BootContext->FileSystems, Link))
  {
    FileSystem = BASE_CR (Link, OC_BOOT_FILESYSTEM, Link);

    PartitionEntry                  = OcGetGptPartitionEntry (FileSystem->Handle);
    IsDefaultEntryProtocolPartition = (
                                         (DefaultEntryId != NULL)
                                      && CompareGuid (
                                           &DefaultEntryPartuuid,
                                           (PartitionEntry == NULL) ? &gEfiPartTypeUnusedGuid : &PartitionEntry->UniquePartitionGUID
                                           )
                                         );

    //
    // No entries, or only entry pre-created from boot entry protocol,
    // so process this directory with Apple Bless.
    //
    if (IsDefaultEntryProtocolPartition || IsListEmpty (&FileSystem->BootEntries)) {
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
    // Try boot entry protocol.
    // Entry protocol entries are added regardless of bless; e.g. user might well
    // have /loader/entries in ESP, in addition to normal blessed files.
    // Skip any entry already created from boot options.
    //
    OcAddEntriesFromBootEntryProtocol (
      BootContext,
      FileSystem,
      EntryProtocolHandles,
      EntryProtocolHandleCount,
      IsDefaultEntryProtocolPartition ? DefaultEntryId : NULL,
      FALSE,
      FALSE
      );

    //
    // Record predefined recoveries.
    //
    AddBootEntryFromSelfRecovery (BootContext, FileSystem);
  }

  if (CustomFileSystem != NULL) {
    //
    // Insert the custom file system last for entry order.
    //
    InsertTailList (&BootContext->FileSystems, &CustomFileSystem->Link);
    ++BootContext->FileSystemCount;

    //
    // Build custom and system options.
    //
    AddFileSystemEntryForCustom (BootContext, CustomFileSystem, DefaultCustomIndex);

    //
    // Boot entry protocol also supports custom and system entries.
    //
    OcAddEntriesFromBootEntryProtocol (
      BootContext,
      CustomFileSystem,
      EntryProtocolHandles,
      EntryProtocolHandleCount,
      DefaultEntryId,
      FALSE,
      FALSE
      );
  }

  if (DefaultEntryId != NULL) {
    FreePool (DefaultEntryId);
    DefaultEntryId = NULL;
  }

  OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);

  if (BootContext->BootEntryCount == 0) {
    OcFreeBootContext (BootContext);
    return NULL;
  }

  //
  // Find recovery.
  //
  if (BootContext->PickerContext->PickerCommand == OcPickerBootAppleRecovery) {
    OcSetDefaultBootRecovery (BootContext);
  }

  return BootContext;
}

OC_BOOT_CONTEXT *
OcScanForDefaultBootEntry (
  IN  OC_PICKER_CONTEXT  *Context,
  IN  BOOLEAN            UseBootNextOnly
  )
{
  OC_BOOT_CONTEXT     *BootContext;
  UINTN               Index;
  OC_BOOT_FILESYSTEM  *FileSystem;
  BOOLEAN             AlreadySeen;
  EFI_STATUS          Status;
  UINTN               NoHandles;
  EFI_HANDLE          *Handles;
  OC_BOOT_FILESYSTEM  *CustomFileSystem;
  EFI_HANDLE          *EntryProtocolHandles;
  UINTN               EntryProtocolHandleCount;

  //
  // Obtain empty list of filesystems.
  //
  BootContext = BuildFileSystemList (Context, TRUE);
  if (BootContext == NULL) {
    return NULL;
  }

  CustomFileSystem = CreateFileSystemForCustom (BootContext);
  if (CustomFileSystem != NULL) {
    //
    // The entry order does not matter, UI will not be shown.
    //
    InsertTailList (&BootContext->FileSystems, &CustomFileSystem->Link);
    ++BootContext->FileSystemCount;
  }

  DEBUG ((DEBUG_INFO, "OCB: Looking for default entry (%d:%a)\n", Context->PickerCommand, Context->HotKeyEntryId));

  if (Context->PickerCommand != OcPickerProtocolHotKey) {
    //
    // Locate loaded boot entry protocol drivers.
    //
    OcLocateBootEntryProtocolHandles (&EntryProtocolHandles, &EntryProtocolHandleCount);

    //
    // Create primary boot options from BootOrder.
    //
    if (Context->BootOrder == NULL) {
      Context->BootOrder = InternalGetBootOrderForBooting (
                             BootContext->BootVariableGuid,
                             Context->BlacklistAppleUpdate,
                             &Context->BootOrderCount,
                             UseBootNextOnly
                             );
    }

    if (Context->BootOrder != NULL) {
      for (Index = 0; Index < Context->BootOrderCount; ++Index) {
        //
        // Returned default entry values not required, as no other
        // entries will be created after a match here.
        //
        AddBootEntryFromBootOption (
          BootContext,
          Context->BootOrder[Index],
          TRUE,
          CustomFileSystem,
          NULL,
          EntryProtocolHandles,
          EntryProtocolHandleCount,
          NULL,
          NULL
          );

        //
        // Return as long as we are good.
        //
        if (BootContext->DefaultEntry != NULL) {
          OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);
          return BootContext;
        }
      }
    }

    if (UseBootNextOnly) {
      OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);
      OcFreeBootContext (BootContext);
      return NULL;
    }

    //
    // Obtain filesystems and try processing those remaining.
    //
    NoHandles = 0;
    Status    = gBS->LocateHandleBuffer (
                       ByProtocol,
                       &gEfiSimpleFileSystemProtocolGuid,
                       NULL,
                       &NoHandles,
                       &Handles
                       );

    DEBUG ((DEBUG_INFO, "OCB: Processing %u blessed list - %r\n", (UINT32)NoHandles, Status));

    if (!EFI_ERROR (Status)) {
      for (Index = 0; Index < NoHandles; ++Index) {
        //
        // If file system has been seen during BOOT#### entry processing then
        // bless has already been processed (and failed or we would not be here).
        //
        FileSystem = InternalFileSystemForHandle (BootContext, Handles[Index], TRUE, &AlreadySeen);
        if (FileSystem == NULL) {
          continue;
        }

        if (!AlreadySeen) {
          AddBootEntryFromBless (
            BootContext,
            FileSystem,
            gAppleBootPolicyPredefinedPaths,
            gAppleBootPolicyNumPredefinedPaths,
            FALSE,
            FALSE
            );
          if (BootContext->DefaultEntry != NULL) {
            OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);
            FreePool (Handles);
            return BootContext;
          }
        }

        //
        // Try boot entry protocol. No need to deduplicate as won't reach
        // here if default entry from BOOT#### was successfully created.
        //
        OcAddEntriesFromBootEntryProtocol (
          BootContext,
          FileSystem,
          EntryProtocolHandles,
          EntryProtocolHandleCount,
          NULL,
          FALSE,
          FALSE
          );
        if (BootContext->DefaultEntry != NULL) {
          OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);
          FreePool (Handles);
          return BootContext;
        }

        AddBootEntryFromSelfRecovery (BootContext, FileSystem);
        if (BootContext->DefaultEntry != NULL) {
          OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);
          FreePool (Handles);
          return BootContext;
        }
      }

      FreePool (Handles);
    }

    if (CustomFileSystem != NULL) {
      //
      // Build custom and system options. Do not try to deduplicate custom options
      // as the list is never shown.
      //
      AddFileSystemEntryForCustom (BootContext, CustomFileSystem, MAX_UINT32);
      if (BootContext->DefaultEntry != NULL) {
        OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);
        return BootContext;
      }

      //
      // Boot entry protocol for custom and system entries.
      //
      OcAddEntriesFromBootEntryProtocol (
        BootContext,
        CustomFileSystem,
        EntryProtocolHandles,
        EntryProtocolHandleCount,
        NULL,
        FALSE,
        FALSE
        );
    }

    OcFreeBootEntryProtocolHandles (&EntryProtocolHandles);
  } else {
    //
    // Filter boot entry protocol entries from selected protocol instance only for hotkey entry.
    //
    Status = OcAddEntriesFromBootEntryProtocol (
               BootContext,
               CustomFileSystem,
               &Context->HotKeyProtocolHandle,
               1,
               Context->HotKeyEntryId,
               TRUE,
               TRUE
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCB: Missing boot entry protocol entry for hotkey %a - %r\n", Context->HotKeyEntryId, Status));
    }
  }

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
       FsLink = GetNextNode (&BootContext->FileSystems, FsLink))
  {
    FileSystem = BASE_CR (FsLink, OC_BOOT_FILESYSTEM, Link);

    for (
         EnLink = GetFirstNode (&FileSystem->BootEntries);
         !IsNull (&FileSystem->BootEntries, EnLink);
         EnLink = GetNextNode (&FileSystem->BootEntries, EnLink))
    {
      BootEntry = BASE_CR (EnLink, OC_BOOT_ENTRY, Link);

      ASSERT (EntryIndex < BootContext->BootEntryCount);
      Entries[EntryIndex]   = BootEntry;
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

  if ((BootEntry->Type & OC_BOOT_EXTERNAL_SYSTEM) != 0) {
    ASSERT (BootEntry->ExternalSystemAction != NULL);
    return BootEntry->ExternalSystemAction (Context, BootEntry->DevicePath);
  }

  if ((BootEntry->Type & OC_BOOT_SYSTEM) != 0) {
    ASSERT (BootEntry->SystemAction != NULL);
    return BootEntry->SystemAction (Context);
  }

  Status = InternalLoadBootEntry (
             Context,
             BootEntry,
             ParentHandle,
             &EntryHandle,
             &DmgLoadContext
             );
  if (!EFI_ERROR (Status)) {
    //
    // This does nothing unless emulated NVRAM is present. A hack, basically, to allow us
    // to switch back to the normal macOS boot entry after booting a macOS Installer once,
    // because we have nothing available to correctly update the emulated NVRAM file while
    // the macOS installer is running and rebooting. This strategy is correct, often, and
    // better then the alternative (continuing to create an installer entry when it no longer
    // exists) in any event. See OpenVariableRuntimeDxe documentation for more details.
    //
    if (BootEntry->IsAppleInstaller) {
      OcSwitchToFallbackLegacyNvram ();
    }

    Status = Context->StartImage (BootEntry, EntryHandle, NULL, NULL, BootEntry->LaunchInText);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCB: StartImage failed - %r\n", Status));
      //
      // Unload dmg if any.
      //
      InternalUnloadDmg (&DmgLoadContext);
      //
      // Unload image.
      //
      gBS->UnloadImage (EntryHandle);
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCB: LoadImage failed - %r\n", Status));
  }

  return Status;
}

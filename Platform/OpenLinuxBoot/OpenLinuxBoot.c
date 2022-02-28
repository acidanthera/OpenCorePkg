/** @file
  Linux boot driver, supporting Boot Loader Specification, GRUB2 blscfg, and autodetect.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LinuxBootInternal.h"

#include <Uefi.h>
#include <Guid/Gpt.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/OcBootEntry.h>

UINTN gLinuxBootFlags = LINUX_BOOT_ALL & ~(LINUX_BOOT_ADD_DEBUG_INFO | LINUX_BOOT_LOG_VERBOSE | LINUX_BOOT_ADD_RW);

OC_FLEX_ARRAY     *mParsedLoadOptions;

OC_PICKER_CONTEXT *gPickerContext;
OC_FLEX_ARRAY     *gLoaderEntries;
EFI_GUID          gPartuuid;
CHAR8             *gFileSystemType;

VOID
InternalFreePickerEntry (
  IN   OC_PICKER_ENTRY          *Entry
  )
{
  ASSERT (Entry != NULL);

  if (Entry == NULL) {
    return;
  }

  //
  // TODO: Is this un-CONST casting okay?
  // (Are they CONST because they are not supposed to be freed when used as before?)
  //
  if (Entry->Id != NULL) {
    FreePool ((CHAR8 *)Entry->Id);
  }
  if (Entry->Name != NULL) {
    FreePool ((CHAR8 *)Entry->Name);
  }
  if (Entry->Path != NULL) {
    FreePool ((CHAR8 *)Entry->Path);
  }
  if (Entry->Arguments != NULL) {
    FreePool ((CHAR8 *)Entry->Arguments);
  }
  if (Entry->Flavour != NULL) {
    FreePool ((CHAR8 *)Entry->Flavour);
  }
}

STATIC
VOID
EFIAPI
OcFreeLinuxBootEntries (
  IN   OC_PICKER_ENTRY          **Entries,
  IN   UINTN                    NumEntries
  )
{
  UINTN Index;

  ASSERT (Entries   != NULL);
  ASSERT (*Entries  != NULL);
  if (Entries == NULL || *Entries == NULL) {
    return;
  }

  for (Index = 0; Index < NumEntries; Index++) {
    InternalFreePickerEntry (&(*Entries)[Index]);
  }

  FreePool (*Entries);
  *Entries = NULL;
}

STATIC
EFI_STATUS
EFIAPI
OcGetLinuxBootEntries (
  IN           OC_PICKER_CONTEXT        *PickerContext,
  IN     CONST EFI_HANDLE               Device,
     OUT       OC_PICKER_ENTRY          **Entries,
     OUT       UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *RootDirectory;
  UINT32                          FileSystemPolicy;
  CONST EFI_PARTITION_ENTRY       *PartitionEntry;

  ASSERT (PickerContext != NULL);
  ASSERT (Entries     != NULL);
  ASSERT (NumEntries  != NULL);

  gPickerContext  = PickerContext;
  *Entries        = NULL;
  *NumEntries     = 0;

  //
  // No custom entries.
  //
  if (Device == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Open partition file system.
  //
  Status = gBS->HandleProtocol (
    Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LNX: Missing filesystem - %r\n", Status));
    return Status;
  }

  //
  // Get handle to partiton root directory.
  //
  Status = FileSystem->OpenVolume (
    FileSystem,
    &RootDirectory
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LNX: Invalid root volume - %r\n", Status));
    return Status;
  }

  gFileSystemType   = NULL;

  FileSystemPolicy = OcGetFileSystemPolicyType (Device);

  //
  // Disallow Apple filesystems, mainly to avoid needlessly
  // scanning multiple APFS partitions.
  //
  if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_APFS) != 0) {
    gFileSystemType = "APFS";
    DEBUG ((DEBUG_INFO, "LNX: %a - not scanning\n", gFileSystemType));
    Status = EFI_NOT_FOUND;
  } else if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_HFS) != 0) {
    gFileSystemType = "HFS";
    DEBUG ((DEBUG_INFO, "LNX: %a - not scanning\n", gFileSystemType));
    Status = EFI_NOT_FOUND;
  } else if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_ESP) != 0) {
    gFileSystemType = "ESP";
    if ((gLinuxBootFlags & LINUX_BOOT_SCAN_ESP) == 0) {
      DEBUG ((DEBUG_INFO, "LNX: %a - requested not to scan\n", gFileSystemType));
      Status = EFI_NOT_FOUND;
    }
  } else if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_XBOOTLDR) != 0) {
    gFileSystemType = "XBOOTLDR";
    if ((gLinuxBootFlags & LINUX_BOOT_SCAN_XBOOTLDR) == 0) {
      DEBUG ((DEBUG_INFO, "LNX: %a - requested not to scan\n", gFileSystemType));
      Status = EFI_NOT_FOUND;
    }
  } else if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_LINUX_ROOT) != 0) {
    gFileSystemType = "LNX-R";
    if ((gLinuxBootFlags & LINUX_BOOT_SCAN_LINUX_ROOT) == 0) {
      DEBUG ((DEBUG_INFO, "LNX: %a - requested not to scan\n", gFileSystemType));
      Status = EFI_NOT_FOUND;
    }
  } else if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_LINUX_DATA) != 0) {
    gFileSystemType = "LNX-D";
    if ((gLinuxBootFlags & LINUX_BOOT_SCAN_LINUX_DATA) == 0) {
      DEBUG ((DEBUG_INFO, "LNX: %a - requested not to scan\n", gFileSystemType));
      Status = EFI_NOT_FOUND;
    }
  } else {
    if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_NTFS) != 0) {
      //
      // This is not just NTFS, Msft Basic Data part type GUID is used for non-ESP FAT too.
      //  
      gFileSystemType = "NTFS/FAT";
    } else {
      gFileSystemType = "OTHER";
    }

    if ((gLinuxBootFlags & LINUX_BOOT_SCAN_OTHER) == 0) {
      DEBUG ((DEBUG_INFO, "LNX: %a - requested not to scan\n", gFileSystemType));
      Status = EFI_NOT_FOUND;
    }
  }

  if (EFI_ERROR (Status)) {
    RootDirectory->Close (RootDirectory);
    return Status;
  }

  //
  // Save PARTUUID for autodetect.
  //
  PartitionEntry = OcGetGptPartitionEntry (Device);
  if (PartitionEntry == NULL) {
    CopyGuid (&gPartuuid, &gEfiPartTypeUnusedGuid);
  } else {
    CopyGuid (&gPartuuid, &PartitionEntry->UniquePartitionGUID);
  }

  //
  // Log TypeGUID and PARTUUID of the drive we're in.
  //
  DEBUG ((
    DEBUG_INFO,
    "LNX: TypeGUID: %g (%a) PARTUUID: %g\n",
    (PartitionEntry == NULL) ? &gEfiPartTypeUnusedGuid : &PartitionEntry->PartitionTypeGUID,
    gFileSystemType,
    &gPartuuid
    ));

  //
  // Scan for boot loader spec & blscfg entries (Fedora-like).
  //
  Status = InternalScanLoaderEntries (
    RootDirectory,
    Entries,
    NumEntries
    );

  //
  // Note: As currently structured, will fall through to autodetect
  // if no /loader/entries/*.conf files are present, but also if there
  // are only unusable files in there.
  //
  if (EFI_ERROR (Status)
    && (gLinuxBootFlags & LINUX_BOOT_ALLOW_AUTODETECT) != 0) {
    //
    // Auto-detect vmlinuz and initrd files on own root filesystem (Debian-like).
    //
    Status = InternalAutodetectLinux (
      RootDirectory,
      Entries,
      NumEntries
      );
  }

  RootDirectory->Close (RootDirectory);

  return Status;
}

STATIC
OC_BOOT_ENTRY_PROTOCOL
mLinuxBootEntryProtocol = {
  OC_BOOT_ENTRY_PROTOCOL_REVISION,
  OcGetLinuxBootEntries,
  OcFreeLinuxBootEntries
};

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  UINTN                             AddBootFlags;
  UINTN                             RemoveBootFlags;

  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Keep mParsedLoadOptions kicking around, as all found options link into its memory.
  //
  Status = OcParseLoadOptions (LoadedImage, &mParsedLoadOptions);
  if (!EFI_ERROR (Status)) {
    AddBootFlags = 0;
    RemoveBootFlags = 0;
    OcParsedVarsGetInt (mParsedLoadOptions, L"flags",  &gLinuxBootFlags, TRUE);
    OcParsedVarsGetInt (mParsedLoadOptions, L"flags+", &AddBootFlags,    TRUE);
    OcParsedVarsGetInt (mParsedLoadOptions, L"flags-", &RemoveBootFlags, TRUE);
    gLinuxBootFlags |= AddBootFlags;
    gLinuxBootFlags &= ~RemoveBootFlags;
  } else {
    if (Status != EFI_NOT_FOUND) {
      return Status;
    }
    ASSERT (mParsedLoadOptions == NULL);
  }

  if ((gLinuxBootFlags & LINUX_BOOT_ALLOW_AUTODETECT) != 0) {
    Status = InternalPreloadAutoOpts (mParsedLoadOptions);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
    &ImageHandle,
    &gOcBootEntryProtocolGuid,
    &mLinuxBootEntryProtocol,
    NULL
    );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

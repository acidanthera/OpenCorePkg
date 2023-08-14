/** @file
  Legacy boot driver.

  Copyright (c) 2023, Goldfish64. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LegacyBootInternal.h"

#include <Protocol/OcBootEntry.h>

STATIC EFI_HANDLE  mImageHandle;
STATIC BOOLEAN     mIsAppleInterfaceSupported;

VOID
InternalFreePickerEntry (
  IN   OC_PICKER_ENTRY  *Entry
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
EFI_STATUS
ExternalSystemActionDoLegacyBoot (
  IN OUT          OC_PICKER_CONTEXT         *PickerContext,
  IN              EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 LoadedImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  //
  // Load and start legacy OS.
  //
  // On Macs, use the Apple legacy interface.
  // On other systems, use the Legacy8259 protocol.
  //
  DebugPrintDevicePath (DEBUG_INFO, "LEG: Legacy device path", DevicePath);
  if (mIsAppleInterfaceSupported) {
    Status = InternalLoadAppleLegacyInterface (
               mImageHandle,
               DevicePath,
               &LoadedImageHandle
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "LEG: Failure while loading Apple legacy interface - %r\n", Status));
      return Status;
    }

    Status = gBS->HandleProtocol (
                    LoadedImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "LEG: Failure while loading Apple legacy interface - %r\n", Status));
      return Status;
    }

    LoadedImage->LoadOptionsSize = 0;
    LoadedImage->LoadOptions     = NULL;

    CONST CHAR8  *Args;

    Args = AllocateCopyPool (L_STR_SIZE ("HD"), "HD");

    OcAppendArgumentsToLoadedImage (LoadedImage, &Args, 1, TRUE);
    DEBUG ((
      DEBUG_INFO,
      "OCB: Args <%s>\n",
      LoadedImage->LoadOptions
      ));
    Status = gBS->StartImage (LoadedImageHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "LEG: Failure while starting Apple legacy interface - %r\n", Status));
      return Status;
    }
  } else {
    InternalLoadLegacyPbr (DevicePath, OcPartitionGetPartitionHandle (DevicePath));
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcGetLegacyBootEntries (
  IN OUT         OC_PICKER_CONTEXT  *PickerContext,
  IN     CONST EFI_HANDLE           Device OPTIONAL,
  OUT       OC_PICKER_ENTRY         **Entries,
  OUT       UINTN                   *NumEntries
  )
{
  // EFI_STATUS                       Status;
  // EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  // EFI_FILE_PROTOCOL                *RootDirectory;
  // UINT32                           FileSystemPolicy;

  OC_LEGACY_OS_TYPE  LegacyType;
  OC_PICKER_ENTRY    *PickerEntry;

  //
  // No custom entries.
  //
  if (Device == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Open partition file system.
  //

  /* Status = gBS->HandleProtocol (
                   Device,
                   &gEfiSimpleFileSystemProtocolGuid,
                   (VOID **)&FileSystem
                   );
   if (EFI_ERROR (Status)) {
     DEBUG ((DEBUG_WARN, "LEG: Missing filesystem - %r\n", Status));
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
     DEBUG ((DEBUG_WARN, "LEG: Invalid root volume - %r\n", Status));
     return Status;
   }

   FileSystemPolicy = OcGetFileSystemPolicyType (Device);*/

  //
  // Disallow all but NTFS filesystems.
  //
  // if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_NTFS) != 0) {
  //   DEBUG ((DEBUG_INFO, "LEG: Not scanning non-NTFS filesystem\n"));
  //  Status = EFI_NOT_FOUND;
  // }

  //
  // Scan for boot entry (only one entry per filesystem).
  //
  LegacyType = InternalGetPartitionLegacyOsType (Device);
  // RootDirectory->Close (RootDirectory);
  if (LegacyType == OcLegacyOsTypeNone) {
    return EFI_NOT_FOUND;
  }

  CHAR16  *str2 = ConvertDevicePathToText (DevicePathFromHandle (Device), FALSE, FALSE);
  CHAR8   *str  = AllocateZeroPool (StrLen (str2) + 1);

  UnicodeStrToAsciiStrS (str2, str, StrLen (str2) + 1);

  PickerEntry                       = AllocateZeroPool (sizeof (*PickerEntry));
  PickerEntry->Id                   = str;
  PickerEntry->Name                 = "Windows (legacy)";
  PickerEntry->Path                 = NULL;
  PickerEntry->Arguments            = NULL;
  PickerEntry->Flavour              = OC_FLAVOUR_WINDOWS;
  PickerEntry->Tool                 = FALSE;
  PickerEntry->TextMode             = TRUE;
  PickerEntry->RealPath             = FALSE;
  PickerEntry->ExternalSystemAction = ExternalSystemActionDoLegacyBoot;

  *Entries    = PickerEntry;
  *NumEntries = 1;

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
OcFreeLegacyBootEntries (
  IN   OC_PICKER_ENTRY  **Entries,
  IN   UINTN            NumEntries
  )
{
  return;
  UINTN  Index;

  ASSERT (Entries   != NULL);
  ASSERT (*Entries  != NULL);
  if ((Entries == NULL) || (*Entries == NULL)) {
    return;
  }

  for (Index = 0; Index < NumEntries; Index++) {
    InternalFreePickerEntry (&(*Entries)[Index]);
  }

  FreePool (*Entries);
  *Entries = NULL;
}

STATIC
OC_BOOT_ENTRY_PROTOCOL
  mLegacyBootEntryProtocol = {
  OC_BOOT_ENTRY_PROTOCOL_REVISION,
  OcGetLegacyBootEntries,
  OcFreeLegacyBootEntries,
  NULL
};

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mImageHandle = ImageHandle;

  Status = InternalIsLegacyInterfaceSupported (&mIsAppleInterfaceSupported);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LEG: Legacy boot interface is supported on this system\n"));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "LEG: Apple legacy interface: %d\n", mIsAppleInterfaceSupported));

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gOcBootEntryProtocolGuid,
                  &mLegacyBootEntryProtocol,
                  NULL
                  );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

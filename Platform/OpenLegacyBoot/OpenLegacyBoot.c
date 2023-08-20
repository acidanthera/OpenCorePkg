/** @file
  Legacy boot driver.

  Copyright (c) 2023, Goldfish64. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LegacyBootInternal.h"

#include <Protocol/OcBootEntry.h>

STATIC EFI_HANDLE  mImageHandle;
STATIC BOOLEAN     mIsAppleInterfaceSupported;

STATIC
CHAR8 *
GetLegacyEntryName (
  OC_LEGACY_OS_TYPE  LegacyOsType
  )
{
  return AllocateCopyPool (L_STR_SIZE ("Windows (legacy)"), "Windows (legacy)");
}

STATIC
CHAR8 *
GetLegacyEntryFlavour (
  OC_LEGACY_OS_TYPE  LegacyOsType
  )
{
  return AllocateCopyPool (L_STR_SIZE (OC_FLAVOUR_WINDOWS), OC_FLAVOUR_WINDOWS);
}

STATIC
VOID
FreePickerEntry (
  IN   OC_PICKER_ENTRY  *Entry
  )
{
  ASSERT (Entry != NULL);

  if (Entry == NULL) {
    return;
  }

  if (Entry->Id != NULL) {
    FreePool ((CHAR8 *)Entry->Id);
  }

  if (Entry->Name != NULL) {
    FreePool ((CHAR8 *)Entry->Name);
  }

  if (Entry->Flavour != NULL) {
    FreePool ((CHAR8 *)Entry->Flavour);
  }
}

STATIC
EFI_STATUS
ExternalSystemActionDoLegacyBoot (
  IN OUT  OC_PICKER_CONTEXT         *PickerContext,
  IN      EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 DiskHandle;
  EFI_HANDLE                 LoadedImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  BOOLEAN                    IsExternal;
  CONST CHAR8                *AppleBootArg;

  //
  // Load and start legacy OS.
  //
  // On Macs, use the Apple legacy interface.
  // On other systems, use the Legacy8259 protocol.
  //
  DebugPrintDevicePath (DEBUG_INFO, "OLB: Legacy device path", DevicePath);
  if (mIsAppleInterfaceSupported) {
    DiskHandle = OcPartitionGetPartitionHandle (DevicePath);
    if (DiskHandle == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Status = InternalLoadAppleLegacyInterface (
               mImageHandle,
               DevicePath,
               &LoadedImageHandle
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failure while loading Apple legacy interface - %r\n", Status));
      return Status;
    }

    //
    // Specify boot device type argument on loaded image.
    //
    Status = gBS->HandleProtocol (
                    LoadedImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failure while loading Apple legacy interface - %r\n", Status));
      gBS->UnloadImage (LoadedImageHandle);
      return Status;
    }

    LoadedImage->LoadOptionsSize = 0;
    LoadedImage->LoadOptions     = NULL;

    OcGetDevicePolicyType (DiskHandle, &IsExternal);

    if (OcIsDiskCdRom (DevicePath)) {
      AppleBootArg = "CD";
    } else if (IsExternal) {
      AppleBootArg = "USB";
    } else {
      AppleBootArg = "HD";
    }

    OcAppendArgumentsToLoadedImage (
      LoadedImage,
      &AppleBootArg,
      1,
      TRUE
      );
    DEBUG ((DEBUG_INFO, "OLB: Apple legacy interface args <%s>\n", LoadedImage->LoadOptions));

    Status = gBS->StartImage (LoadedImageHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failure while starting Apple legacy interface - %r\n", Status));
      gBS->UnloadImage (LoadedImageHandle);
      return Status;
    }
  } else {
    Status = InternalLoadLegacyPbr (DevicePath);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failure while starting legacy PBR interface - %r\n", Status));
      return Status;
    }
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
  EFI_STATUS  Status;
  UINTN       NoHandles;
  EFI_HANDLE  *Handles;
  UINTN       HandleIndex;
  UINT32      ScanPolicy;
  BOOLEAN     IsExternal;

  CHAR16  *UnicodeDevicePath;
  CHAR8   *AsciiDevicePath;
  UINTN   AsciiDevicePathSize;

  EFI_HANDLE                BlockDeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *BlockDevicePath;
  OC_LEGACY_OS_TYPE         LegacyOsType;
  OC_FLEX_ARRAY             *FlexPickerEntries;
  OC_PICKER_ENTRY           *PickerEntry;

  ASSERT (PickerContext != NULL);
  ASSERT (Entries     != NULL);
  ASSERT (NumEntries  != NULL);

  //
  // Custom entries only.
  //
  if (Device != NULL) {
    return EFI_NOT_FOUND;
  }

  FlexPickerEntries = OcFlexArrayInit (sizeof (OC_PICKER_ENTRY), (OC_FLEX_ARRAY_FREE_ITEM)FreePickerEntry);
  if (FlexPickerEntries == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get all Block I/O handles.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &NoHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OLB: Failed to get Block I/O handles - %r\n", Status));
    OcFlexArrayFree (&FlexPickerEntries);
    return Status;
  }

  for (HandleIndex = 0; HandleIndex < NoHandles; HandleIndex++) {
    BlockDeviceHandle = Handles[HandleIndex];

    //
    // If device type locking is set and this device is not allowed,
    // skip device.
    //
    ScanPolicy = OcGetDevicePolicyType (BlockDeviceHandle, &IsExternal);
    if (((PickerContext->ScanPolicy & OC_SCAN_DEVICE_LOCK) != 0) && ((ScanPolicy & PickerContext->ScanPolicy) == 0)) {
      continue;
    }

    //
    // Detect legacy OS type.
    //
    LegacyOsType = InternalGetPartitionLegacyOsType (BlockDeviceHandle, mIsAppleInterfaceSupported);
    if (LegacyOsType == OcLegacyOsTypeNone) {
      continue;
    }

    BlockDevicePath = DevicePathFromHandle (BlockDeviceHandle);
    if (BlockDevicePath == NULL) {
      DEBUG ((DEBUG_INFO, "OLB: Could not find Device Path for block device\n"));
      continue;
    }

    //
    // Create and add picker entry to entries.
    //
    PickerEntry = OcFlexArrayAddItem (FlexPickerEntries);
    if (PickerEntry == NULL) {
      OcFlexArrayFree (&FlexPickerEntries);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Device Path will be used as ID and is required for default entry matching.
    //
    UnicodeDevicePath = ConvertDevicePathToText (BlockDevicePath, FALSE, FALSE);
    if (UnicodeDevicePath == NULL) {
      OcFlexArrayFree (&FlexPickerEntries);
      return EFI_OUT_OF_RESOURCES;
    }

    AsciiDevicePathSize = (StrLen (UnicodeDevicePath) + 1) * sizeof (CHAR8);
    AsciiDevicePath     = AllocatePool (AsciiDevicePathSize);
    if (AsciiDevicePath == NULL) {
      FreePool (UnicodeDevicePath);
      OcFlexArrayFree (&FlexPickerEntries);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = UnicodeStrToAsciiStrS (UnicodeDevicePath, AsciiDevicePath, AsciiDevicePathSize);
    FreePool (UnicodeDevicePath);
    if (EFI_ERROR (Status)) {
      FreePool (AsciiDevicePath);
      OcFlexArrayFree (&FlexPickerEntries);
      return Status;
    }

    PickerEntry->Id                       = AsciiDevicePath;
    PickerEntry->Name                     = GetLegacyEntryName (LegacyOsType);
    PickerEntry->Path                     = NULL;
    PickerEntry->Arguments                = NULL;
    PickerEntry->Flavour                  = GetLegacyEntryFlavour (LegacyOsType);
    PickerEntry->Tool                     = FALSE;
    PickerEntry->TextMode                 = FALSE;
    PickerEntry->RealPath                 = FALSE;
    PickerEntry->External                 = IsExternal;
    PickerEntry->ExternalSystemAction     = ExternalSystemActionDoLegacyBoot;
    PickerEntry->ExternalSystemDevicePath = BlockDevicePath;

    if ((PickerEntry->Name == NULL) || (PickerEntry->Flavour == NULL)) {
      OcFlexArrayFree (&FlexPickerEntries);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  OcFlexArrayFreeContainer (&FlexPickerEntries, (VOID **)Entries, NumEntries);

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
  UINTN  Index;

  ASSERT (Entries   != NULL);
  ASSERT (*Entries  != NULL);
  if ((Entries == NULL) || (*Entries == NULL)) {
    return;
  }

  for (Index = 0; Index < NumEntries; Index++) {
    FreePickerEntry (&(*Entries)[Index]);
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
    DEBUG ((DEBUG_WARN, "OLB: Legacy boot interface is supported on this system\n"));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OLB: Apple legacy interface: %d\n", mIsAppleInterfaceSupported));

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

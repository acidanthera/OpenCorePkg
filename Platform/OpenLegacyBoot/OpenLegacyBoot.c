/** @file
  Legacy boot driver.

  Copyright (c) 2023, Goldfish64. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LegacyBootInternal.h"

#include <Protocol/OcBootEntry.h>

STATIC EFI_HANDLE  mImageHandle;
STATIC BOOLEAN     mIsAppleInterfaceSupported;

STATIC OC_FLEX_ARRAY  *mHiddenDevicePaths;

//
// Fallback PIWG firmware media device path for Apple legacy interface.
// MemoryMapped(0xB,0xFFE00000,0xFFF9FFFF)/FvFile(2B0585EB-D8B8-49A9-8B8C-E21B01AEF2B7)
//
STATIC CONST UINT8                     AppleLegacyInterfaceFallbackDevicePathData[] = {
  0x01, 0x03, 0x18, 0x00, 0x0B, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xE0, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xF9, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x06, 0x14, 0x00, 0xEB, 0x85, 0x05, 0x2B,
  0xB8, 0xD8, 0xA9, 0x49, 0x8B, 0x8C, 0xE2, 0x1B,
  0x01, 0xAE, 0xF2, 0xB7, 0x7F, 0xFF, 0x04, 0x00
};
STATIC CONST EFI_DEVICE_PATH_PROTOCOL  *AppleLegacyInterfaceFallbackDevicePathPath = (EFI_DEVICE_PATH_PROTOCOL *)AppleLegacyInterfaceFallbackDevicePathData;

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
CHAR8 *
LoadAppleDiskLabel (
  IN OUT  OC_PICKER_CONTEXT  *PickerContext,
  IN      EFI_HANDLE         DiskHandle
  )
{
  EFI_STATUS                       Status;
  CHAR8                            *DiskLabel;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  DiskLabel = NULL;
  if ((PickerContext->PickerAttributes & OC_ATTR_USE_DISK_LABEL_FILE) != 0) {
    Status = gBS->HandleProtocol (
                    DiskHandle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)&FileSystem
                    );
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    DiskLabel = OcReadFile (FileSystem, L".contentDetails", NULL, 0);
    if (DiskLabel == NULL) {
      DiskLabel = OcReadFile (FileSystem, L".disk_label.contentDetails", NULL, 0);
    }

    if (DiskLabel == NULL) {
      DEBUG ((DEBUG_INFO, "OLB: %s %s not present\n", L".contentDetails", L".disk_label.contentDetails"));
    } else {
      DEBUG ((DEBUG_INFO, "OLB: Found disk label '%a'\n", DiskLabel));
    }
  }

  return DiskLabel;
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
  EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  BOOLEAN                    IsExternal;
  CONST CHAR8                *AppleBootArg;

  //
  // Set BootCampHD to desired disk Device Path for non-CD devices.
  //
  if (!OcIsDiskCdRom (DevicePath)) {
    Status = InternalSetBootCampHDPath (DevicePath);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failure while setting BootCampHD variable - %r\n", Status));
      return Status;
    }
  }

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
               &LoadedImageDevicePath,
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
ExternalSystemGetDevicePath (
  IN OUT  OC_PICKER_CONTEXT         *PickerContext,
  IN OUT  EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                LoadedImageHandle;
  EFI_DEVICE_PATH_PROTOCOL  *LoadedImageDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *AppleLoaderDevicePath;

  //
  // Set BootCampHD to desired disk Device Path for non-CD devices.
  //
  if (!OcIsDiskCdRom (*DevicePath)) {
    Status = InternalSetBootCampHDPath (*DevicePath);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failure while setting BootCampHD variable - %r\n", Status));
      return Status;
    }
  }

  //
  // Locate Apple legacy interface loader.
  //
  // On Macs, use the Apple legacy interface and set BootCampHD.
  // On other systems, use a default placeholder path.
  //
  DebugPrintDevicePath (DEBUG_INFO, "OLB: Legacy device path", *DevicePath);
  if (mIsAppleInterfaceSupported) {
    Status = InternalLoadAppleLegacyInterface (
               mImageHandle,
               &LoadedImageDevicePath,
               &LoadedImageHandle
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failure while loading Apple legacy interface - %r\n", Status));
      return Status;
    }

    gBS->UnloadImage (LoadedImageHandle);

    AppleLoaderDevicePath = DuplicateDevicePath (LoadedImageDevicePath);
  } else {
    AppleLoaderDevicePath = DuplicateDevicePath (AppleLegacyInterfaceFallbackDevicePathPath);
  }

  if (AppleLoaderDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *DevicePath = AppleLoaderDevicePath;
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
  UINTN       DevicePathIndex;
  BOOLEAN     SkipHiddenDevice;
  UINT32      ScanPolicy;
  BOOLEAN     IsExternal;

  CHAR16  *UnicodeDevicePath;
  CHAR8   *AsciiDevicePath;
  UINTN   AsciiDevicePathSize;
  CHAR16  **HiddenUnicodeDevicePath;

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

    BlockDevicePath = DevicePathFromHandle (BlockDeviceHandle);
    if (BlockDevicePath == NULL) {
      DEBUG ((DEBUG_INFO, "OLB: Could not find Device Path for block device\n"));
      continue;
    }

    //
    // Device Path will be used as ID and is required for default entry matching.
    //
    UnicodeDevicePath = ConvertDevicePathToText (BlockDevicePath, FALSE, FALSE);
    if (UnicodeDevicePath == NULL) {
      OcFlexArrayFree (&FlexPickerEntries);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Skip device if on hidden list.
    //
    if (mHiddenDevicePaths != NULL) {
      SkipHiddenDevice = FALSE;
      for (DevicePathIndex = 0; DevicePathIndex < mHiddenDevicePaths->Count; DevicePathIndex++) {
        HiddenUnicodeDevicePath = (CHAR16 **)OcFlexArrayItemAt (mHiddenDevicePaths, DevicePathIndex);
        ASSERT (HiddenUnicodeDevicePath != NULL);

        if (StrCmp (UnicodeDevicePath, *HiddenUnicodeDevicePath) == 0) {
          DEBUG ((DEBUG_INFO, "OLB: Skipping hidden device %s\n", *HiddenUnicodeDevicePath));
          SkipHiddenDevice = TRUE;
          break;
        }
      }

      if (SkipHiddenDevice) {
        FreePool (UnicodeDevicePath);
        continue;
      }
    }

    //
    // Detect legacy OS type.
    //
    LegacyOsType = InternalGetPartitionLegacyOsType (BlockDeviceHandle, mIsAppleInterfaceSupported);
    if (LegacyOsType == OcLegacyOsTypeNone) {
      FreePool (UnicodeDevicePath);
      continue;
    }

    //
    // Create and add picker entry to entries.
    //
    PickerEntry = OcFlexArrayAddItem (FlexPickerEntries);
    if (PickerEntry == NULL) {
      FreePool (UnicodeDevicePath);
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

    PickerEntry->Name = LoadAppleDiskLabel (PickerContext, BlockDeviceHandle);
    if (PickerEntry->Name == NULL) {
      PickerEntry->Name = GetLegacyEntryName (LegacyOsType);
    }

    PickerEntry->Id                          = AsciiDevicePath;
    PickerEntry->Path                        = NULL;
    PickerEntry->Arguments                   = NULL;
    PickerEntry->Flavour                     = GetLegacyEntryFlavour (LegacyOsType);
    PickerEntry->Tool                        = FALSE;
    PickerEntry->TextMode                    = FALSE;
    PickerEntry->RealPath                    = FALSE;
    PickerEntry->External                    = IsExternal;
    PickerEntry->ExternalSystemAction        = ExternalSystemActionDoLegacyBoot;
    PickerEntry->ExternalSystemGetDevicePath = ExternalSystemGetDevicePath;
    PickerEntry->ExternalSystemDevicePath    = BlockDevicePath;

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
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     *DevicePathNames;
  OC_FLEX_ARRAY              *ParsedLoadOptions;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mImageHandle       = ImageHandle;
  mHiddenDevicePaths = NULL;

  Status = OcParseLoadOptions (LoadedImage, &ParsedLoadOptions);
  if (!EFI_ERROR (Status)) {
    OcParsedVarsGetUnicodeStr (ParsedLoadOptions, L"--hide-devices", &DevicePathNames);
    if (DevicePathNames != NULL) {
      mHiddenDevicePaths = OcStringSplit (DevicePathNames, L';', OcStringFormatUnicode);
      if (mHiddenDevicePaths == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }
  } else {
    if (Status != EFI_NOT_FOUND) {
      return Status;
    }
  }

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

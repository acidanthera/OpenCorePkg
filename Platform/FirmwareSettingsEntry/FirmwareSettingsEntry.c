/** @file
  Boot entry protocol implementation of firmware settings boot picker entry.

  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Guid/AppleVariable.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/OcBootEntry.h>

#define OC_MENU_FIRMWARE_SETTINGS_ID     "firmware_settings"
#define OC_MENU_FIRMWARE_SETTINGS_ENTRY  "Firmware Settings"

STATIC
EFI_STATUS
SystemActionFirmwareSettings (
  IN OUT          OC_PICKER_CONTEXT  *PickerContext
  )
{
  return OcResetSystem (L"firmware");
}

STATIC OC_PICKER_ENTRY  mFirmwareSettingsBootEntries[1] = {
  {
    .Id            = OC_MENU_FIRMWARE_SETTINGS_ID,
    .Name          = OC_MENU_FIRMWARE_SETTINGS_ENTRY,
    .Path          = NULL,
    .Arguments     = NULL,
    .Flavour       = OC_FLAVOUR_FIRMWARE_SETTINGS,
    .Auxiliary     = TRUE,
    .Tool          = FALSE,
    .TextMode      = FALSE,
    .RealPath      = FALSE,
    .SystemAction  = SystemActionFirmwareSettings,
    .AudioBasePath = OC_VOICE_OVER_AUDIO_FILE_FIRMWARE_SETTINGS,
    .AudioBaseType = OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE
  }
};

STATIC
EFI_STATUS
EFIAPI
FirmwareSettingsGetBootEntries (
  IN OUT          OC_PICKER_CONTEXT  *PickerContext,
  IN     CONST EFI_HANDLE            Device OPTIONAL,
  OUT       OC_PICKER_ENTRY          **Entries,
  OUT       UINTN                    *NumEntries
  )
{
  //
  // We provide non-device-specific entries only.
  //
  if (Device != NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "BEP: Firmware settings entry\n"));

  *Entries    = mFirmwareSettingsBootEntries;
  *NumEntries = ARRAY_SIZE (mFirmwareSettingsBootEntries);

  return EFI_SUCCESS;
}

STATIC
OC_BOOT_ENTRY_PROTOCOL
  mFirmwareSettingsBootEntryProtocol = {
  OC_BOOT_ENTRY_PROTOCOL_REVISION,
  FirmwareSettingsGetBootEntries,
  NULL,
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

  //
  // Will log relevant warnings if unsupported.
  //
  Status = OcResetToFirmwareSettingsSupported ();
  if (EFI_ERROR (Status)) {
    //
    // We must succeed or driver halts load.
    //
    return EFI_SUCCESS;
  }

  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gOcBootEntryProtocolGuid,
                &mFirmwareSettingsBootEntryProtocol,
                NULL
                );
}

/** @file
  Boot entry protocol implementation of Toggle SIP boot picker entry.

  Copyright (c) 2022, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Guid/AppleVariable.h>
#include <IndustryStandard/AppleCsrConfig.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/OcBootEntry.h>

#define OC_MENU_SIP_IS_DISABLED  "Toggle SIP (Disabled)"
#define OC_MENU_SIP_IS_ENABLED   "Toggle SIP (Enabled)"

STATIC UINT32  mCsrUserConfig;
STATIC UINT32  mCsrNextConfig;
STATIC UINT32  mAttributes;

STATIC
EFI_STATUS
SystemActionSetSip (
  IN OUT          OC_PICKER_CONTEXT  *Context
  )
{
  return OcSetSip (&mCsrNextConfig, mAttributes);
}

STATIC OC_PICKER_ENTRY  mToggleSipBootEntries[1] = {
  {
    .Id            = "toggle_sip",
    .Name          = NULL, ///< overridden in set up
    .Path          = NULL,
    .Arguments     = NULL,
    .Flavour       = NULL, ///< overridden in set up
    .Auxiliary     = TRUE,
    .Tool          = FALSE,
    .TextMode      = FALSE,
    .RealPath      = FALSE,
    .SystemAction  = SystemActionSetSip,
    .AudioBasePath = NULL, ///< overridden in set up
    .AudioBaseType = OC_VOICE_OVER_AUDIO_BASE_TYPE_OPEN_CORE
  }
};

STATIC
EFI_STATUS
EFIAPI
ToggleSipGetBootEntries (
  IN OUT          OC_PICKER_CONTEXT  *PickerContext,
  IN     CONST EFI_HANDLE            Device OPTIONAL,
  OUT       OC_PICKER_ENTRY          **Entries,
  OUT       UINTN                    *NumEntries
  )
{
  EFI_STATUS  Status;
  UINT32      CsrActiveConfig;
  BOOLEAN     IsEnabled;

  //
  // Custom entries only.
  //
  if (Device != NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Configure current action.
  //
  Status = OcGetSip (&CsrActiveConfig, &mAttributes);
  if (!EFI_ERROR (Status) || (Status == EFI_NOT_FOUND)) {
    IsEnabled = OcIsSipEnabled (Status, CsrActiveConfig);
  } else {
    DEBUG ((DEBUG_WARN, "BEP: ToggleSip failed to read csr-active-config, aborting! - %r\n", Status));
    return Status;
  }

  if (IsEnabled) {
    mToggleSipBootEntries[0].Name          = OC_MENU_SIP_IS_ENABLED;
    mToggleSipBootEntries[0].Flavour       = OC_FLAVOUR_TOGGLE_SIP_ENABLED;
    mToggleSipBootEntries[0].AudioBasePath = OC_VOICE_OVER_AUDIO_FILE_SIP_IS_ENABLED;
    mCsrNextConfig                         = mCsrUserConfig;
  } else {
    mToggleSipBootEntries[0].Name          = OC_MENU_SIP_IS_DISABLED;
    mToggleSipBootEntries[0].Flavour       = OC_FLAVOUR_TOGGLE_SIP_DISABLED;
    mToggleSipBootEntries[0].AudioBasePath = OC_VOICE_OVER_AUDIO_FILE_SIP_IS_DISABLED;
    mCsrNextConfig                         = 0;
  }

  DEBUG ((DEBUG_INFO, "BEP: Adding Toggle SIP entry, currently enabled %d, will change 0x%X->0x%X\n", IsEnabled, CsrActiveConfig, mCsrNextConfig));

  *Entries    = mToggleSipBootEntries;
  *NumEntries = ARRAY_SIZE (mToggleSipBootEntries);

  return EFI_SUCCESS;
}

STATIC
OC_BOOT_ENTRY_PROTOCOL
  mToggleSipBootEntryProtocol = {
  OC_BOOT_ENTRY_PROTOCOL_REVISION,
  ToggleSipGetBootEntries,
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
  UINTN                      Data;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  if (OcHasLoadOptions (LoadedImage->LoadOptionsSize, LoadedImage->LoadOptions)) {
    if (OcUnicodeStartsWith (LoadedImage->LoadOptions, L"0x", TRUE)) {
      Status = StrHexToUintnS (LoadedImage->LoadOptions, NULL, &Data);
    } else {
      Status = StrDecimalToUintnS (LoadedImage->LoadOptions, NULL, &Data);
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "BEP: ToggleSip cannot parse %s - %r\n", LoadedImage->LoadOptions, Status));
    }
  }

  if (!EFI_ERROR (Status)) {
    mCsrUserConfig = (UINT32)Data;
    if (OcIsSipEnabled (EFI_SUCCESS, mCsrUserConfig)) {
      DEBUG ((DEBUG_WARN, "BEP: Specified value 0x%X will not disable SIP!\n", mCsrUserConfig));
    }
  } else {
    mCsrUserConfig = OC_CSR_DISABLE_FLAGS; ///< Defaults.
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gOcBootEntryProtocolGuid,
                  &mToggleSipBootEntryProtocol,
                  NULL
                  );
  return Status;
}

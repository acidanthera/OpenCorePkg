/** @file
  Boot entry protocol implementation of Toggle SIP boot picker entry.

  Copyright (c) 2022, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Guid/AppleVariable.h>
#include <IndustryStandard/AppleCsrConfig.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/OcBootEntry.h>

#define OC_MENU_TOGGLE_SIP           "Toggle SIP (%a)"
#define OC_MENU_TOGGLE_SIP_SHOW_CSR  "Toggle SIP (0x%X:%a)"
#define OC_MENU_TOGGLE_SIP_MAX_SIZE  (\
    sizeof(OC_MENU_TOGGLE_SIP_SHOW_CSR) \
  + sizeof(UINT32) * 2 * sizeof (CHAR8) - L_STR_LEN("%X") \
  + L_STR_LEN("Disabled") - L_STR_LEN("%a"))

STATIC UINT32   mCsrUserConfig;
STATIC UINT32   mCsrNextConfig;
STATIC UINT32   mAttributes;
STATIC BOOLEAN  mShowCsr;

STATIC
EFI_STATUS
SystemActionSetSip (
  IN OUT          OC_PICKER_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsEnabled;
  UINT32      CsrActiveConfig;

  Status = OcSetSip (&mCsrNextConfig, mAttributes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "BEP: OcSetSip (0x%X, 0x%X) failed! - %r\n", mCsrNextConfig, mAttributes, Status));
    return Status;
  }

  Status = OcGetSip (&CsrActiveConfig, &mAttributes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "BEP: OcGetSip (0x%X, 0x%X) failed! - %r\n", mCsrNextConfig, mAttributes, Status));
    return Status;
  }

  IsEnabled = OcIsSipEnabled (Status, CsrActiveConfig);

  DEBUG ((
    DEBUG_INFO,
    "BEP: SystemActionSetSip csr-active-config=0x%X (SIP %a) - %r\n",
    CsrActiveConfig,
    (IsEnabled ? "enabled" : "disabled"),
    Status
    ));

  return Status;
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
  CHAR8       *Name;

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

    //
    // Same logic as in CsrUtil.efi to determine attributes to use
    // (specifically keep same V or NV setting as existing value, if any).
    //
    if (Status == EFI_NOT_FOUND) {
      //
      // TODO: We may want to upgrade Boot Entry Protocol again, so that
      // this line is able to access and respect OC WriteFlash setting?
      //
      mAttributes     = CSR_APPLE_SIP_NVRAM_NV_ATTR;
      CsrActiveConfig = 0;
    } else {
      //
      // We are finding other bits set on Apl, specifically 0x80000000,
      // so only consider relevant bits.
      //
      mAttributes &= CSR_APPLE_SIP_NVRAM_NV_ATTR;
    }
  } else {
    DEBUG ((DEBUG_WARN, "BEP: ToggleSip failed to read csr-active-config, aborting! - %r\n", Status));
    return Status;
  }

  Name = AllocatePool (OC_MENU_TOGGLE_SIP_MAX_SIZE);
  if (Name == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (mShowCsr) {
    Status = OcAsciiSafeSPrint (
               Name,
               OC_MENU_TOGGLE_SIP_MAX_SIZE,
               OC_MENU_TOGGLE_SIP_SHOW_CSR,
               CsrActiveConfig,
               IsEnabled ? "Enabled" : "Disabled"
               );
  } else {
    Status = OcAsciiSafeSPrint (
               Name,
               OC_MENU_TOGGLE_SIP_MAX_SIZE,
               OC_MENU_TOGGLE_SIP,
               IsEnabled ? "Enabled" : "Disabled"
               );
  }

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    FreePool (Name);
    return EFI_ABORTED;
  }

  mToggleSipBootEntries[0].Name = Name;
  if (IsEnabled) {
    mToggleSipBootEntries[0].Flavour       = OC_FLAVOUR_TOGGLE_SIP_ENABLED;
    mToggleSipBootEntries[0].AudioBasePath = OC_VOICE_OVER_AUDIO_FILE_SIP_IS_ENABLED;
    mCsrNextConfig                         = mCsrUserConfig;
  } else {
    mToggleSipBootEntries[0].Flavour       = OC_FLAVOUR_TOGGLE_SIP_DISABLED;
    mToggleSipBootEntries[0].AudioBasePath = OC_VOICE_OVER_AUDIO_FILE_SIP_IS_DISABLED;
    mCsrNextConfig                         = 0;
  }

  DEBUG ((
    DEBUG_INFO,
    "BEP: Toggle SIP entry, currently %a, will change 0x%X->0x%X%a\n",
    (IsEnabled ? "enabled" : "disabled"),
    CsrActiveConfig,
    mCsrNextConfig,
    ((mAttributes & EFI_VARIABLE_NON_VOLATILE) == 0 ? " (volatile)" : "")
    ));

  *Entries    = mToggleSipBootEntries;
  *NumEntries = ARRAY_SIZE (mToggleSipBootEntries);

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
ToggleSipFreeBootEntries (
  IN           OC_PICKER_ENTRY  **Entries,
  IN           UINTN            NumEntries
  )
{
  UINTN  Index;

  if (NumEntries == 0) {
    return;
  }

  ASSERT (NumEntries == 1);
  ASSERT (Entries != NULL);

  for (Index = 0; Index < NumEntries; Index++) {
    if (Entries[Index]->Name != NULL) {
      FreePool ((VOID *)Entries[Index]->Name);  ///< Discard const
    }
  }
}

STATIC
OC_BOOT_ENTRY_PROTOCOL
  mToggleSipBootEntryProtocol = {
  OC_BOOT_ENTRY_PROTOCOL_REVISION,
  ToggleSipGetBootEntries,
  ToggleSipFreeBootEntries
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
  OC_FLEX_ARRAY              *ParsedLoadOptions;
  CHAR16                     *Option;
  UINTN                      Index;
  UINTN                      Data;
  BOOLEAN                    HasUserCsr;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status     = OcParseLoadOptions (LoadedImage, &ParsedLoadOptions);
  HasUserCsr = FALSE;
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < ParsedLoadOptions->Count; Index++) {
      Option = OcParsedVarsItemAt (ParsedLoadOptions, Index)->Unicode.Name;
      if (Option[0] != L'-') {
        if (OcUnicodeStartsWith (Option, L"0x", TRUE)) {
          Status = StrHexToUintnS (Option, NULL, &Data);
        } else {
          Status = StrDecimalToUintnS (Option, NULL, &Data);
        }

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN, "BEP: ToggleSip cannot parse %s - %r\n", Option, Status));
          HasUserCsr = FALSE;
        } else {
          HasUserCsr = TRUE;
        }
      }
    }

    mShowCsr = OcHasParsedVar (ParsedLoadOptions, L"--show-csr", OcStringFormatUnicode);

    OcFlexArrayFree (&ParsedLoadOptions);
  } else {
    ASSERT (ParsedLoadOptions == NULL);

    if (Status != EFI_NOT_FOUND) {
      return Status;
    }

    mShowCsr = FALSE;
  }

  if (HasUserCsr) {
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

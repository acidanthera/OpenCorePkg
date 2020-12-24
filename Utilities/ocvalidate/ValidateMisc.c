/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "ocvalidate.h"
#include "OcValidateLib.h"

#include <Library/OcBootManagementLib.h>
#include <Library/OcConfigurationLib.h>
#include <Protocol/OcLog.h>

/**
  Validate if SecureBootModel has allowed value.

  @param[in]  SecureBootModel  SecureBootModel retrieved from user config.

  @retval     TRUE             If SecureBootModel is valid.
**/
STATIC
BOOLEAN
ValidateSecureBootModel (
  IN  CONST CHAR8  *SecureBootModel
  )
{
  UINT32   Index;
  CONST CHAR8 *AllowedSecureBootModel[] = {
    "Default", "Disabled",
    "j137",  "j680",  "j132",  "j174",  "j140k",
    "j780",  "j213",  "j140a", "j152f", "j160",
    "j230k", "j214k", "j223",  "j215",  "j185", "j185f",
    "x86legacy"
  };

  for (Index = 0; Index < ARRAY_SIZE (AllowedSecureBootModel); ++Index) {
    if (AsciiStrCmp (SecureBootModel, AllowedSecureBootModel[Index]) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

UINT32
CheckMisc (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32          ErrorCount;
  UINT32          Index;
  OC_MISC_CONFIG  *UserMisc;
  OC_UEFI_CONFIG  *UserUefi;
  UINT32          ConsoleAttributes;
  CONST CHAR8     *HibernateMode;
  UINT32          PickerAttributes;
  CONST CHAR8     *PickerMode;
  UINT64          DisplayLevel;
  UINT64          AllowedDisplayLevel;
  UINT64          HaltLevel;
  UINT64          AllowedHaltLevel;
  UINT32          Target;
  CONST CHAR8     *BootProtect;
  BOOLEAN         IsRequestBootVarRoutingEnabled;
  CONST CHAR8     *AsciiDmgLoading;
  UINT32          ExposeSensitiveData;
  CONST CHAR8     *AsciiVault;
  UINT32          ScanPolicy;
  UINT32          AllowedScanPolicy;
  CONST CHAR8     *SecureBootModel;
  CONST CHAR8     *Arguments;
  CONST CHAR8     *Comment;
  CONST CHAR8     *AsciiName;
  CONST CHAR16    *UnicodeName;
  CONST CHAR8     *Path;

  DEBUG ((DEBUG_VERBOSE, "config loaded into Misc checker!\n"));

  ErrorCount                     = 0;
  UserMisc                       = &Config->Misc;
  UserUefi                       = &Config->Uefi;
  ConsoleAttributes              = UserMisc->Boot.ConsoleAttributes;
  HibernateMode                  = OC_BLOB_GET (&UserMisc->Boot.HibernateMode);
  PickerAttributes               = UserMisc->Boot.PickerAttributes;
  PickerMode                     = OC_BLOB_GET (&UserMisc->Boot.PickerMode);
  DisplayLevel                   = UserMisc->Debug.DisplayLevel;
  AllowedDisplayLevel            = DEBUG_WARN | DEBUG_INFO | DEBUG_VERBOSE | DEBUG_ERROR;
  HaltLevel                      = DisplayLevel;
  AllowedHaltLevel               = AllowedDisplayLevel;
  Target                         = UserMisc->Debug.Target;
  BootProtect                    = OC_BLOB_GET (&UserMisc->Security.BootProtect);
  IsRequestBootVarRoutingEnabled = UserUefi->Quirks.RequestBootVarRouting;
  AsciiDmgLoading                = OC_BLOB_GET (&UserMisc->Security.DmgLoading);
  ExposeSensitiveData            = UserMisc->Security.ExposeSensitiveData;
  AsciiVault                     = OC_BLOB_GET (&UserMisc->Security.Vault);
  ScanPolicy                     = UserMisc->Security.ScanPolicy;
  AllowedScanPolicy              = OC_SCAN_FILE_SYSTEM_LOCK | OC_SCAN_DEVICE_LOCK | OC_SCAN_DEVICE_BITS | OC_SCAN_FILE_SYSTEM_BITS;
  SecureBootModel                = OC_BLOB_GET (&UserMisc->Security.SecureBootModel);

  if ((ConsoleAttributes & ~0x7FU) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->ConsoleAttributes has unknown bits set!\n"));
    ++ErrorCount;
  }

  if (AsciiStrCmp (HibernateMode, "None") != 0
    && AsciiStrCmp (HibernateMode, "Auto") != 0
    && AsciiStrCmp (HibernateMode, "RTC") != 0
    && AsciiStrCmp (HibernateMode, "NVRAM") != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->HibernateMode is borked (Can only be None, Auto, RTC, or NVRAM)!\n"));
    ++ErrorCount;
  }

  if ((PickerAttributes & ~OC_ATTR_ALL_BITS) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->PickerAttributes is has unknown bits set!\n"));
    ++ErrorCount;
  }

  //
  // FIXME: Is OpenCanopy.efi mandatory if set to External? Or is this just a suggestion?
  //
  if (AsciiStrCmp (PickerMode, "Builtin") != 0
    && AsciiStrCmp (PickerMode, "External") != 0
    && AsciiStrCmp (PickerMode, "Apple") != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->PickerMode is borked (Can only be Builtin, External, or Apple)!\n"));
    ++ErrorCount;
  }

  //
  // FIXME: Check whether DisplayLevel only supports values within AllowedDisplayLevel, or all possible levels in DebugLib.h?
  //
  if ((DisplayLevel & ~AllowedDisplayLevel) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Debug->DisplayLevel is has unknown bits set!\n"));
    ++ErrorCount;
  }
  if ((HaltLevel & ~AllowedHaltLevel) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->HaltLevel has unknown bits set!\n"));
    ++ErrorCount;
  }

  if ((Target & ~OC_LOG_ALL_BITS) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Debug->Target has unknown bits set!\n"));
    ++ErrorCount;
  }

  //
  // TODO: Check requirements of Security->AuthRestart.
  //

  if (AsciiStrCmp (BootProtect, "None") != 0
    && AsciiStrCmp (BootProtect, "Bootstrap") != 0
    && AsciiStrCmp (BootProtect, "BootstrapShort") != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->BootProtect is borked (Can only be None, Bootstrap, or BootstrapShort)!\n"));
    ++ErrorCount;
  } else if (AsciiStrCmp (BootProtect, "Bootstrap") == 0
    || AsciiStrCmp (BootProtect, "BootstrapShort") == 0) {
    if (!IsRequestBootVarRoutingEnabled) {
      DEBUG ((DEBUG_WARN, "Misc->Security->BootProtect is set to %a which requires UEFI->Quirks->RequestBootVarRouting to be enabled!\n", BootProtect));
      ++ErrorCount;
    }
    //
    // NOTE: RequestBootVarRouting requires OpenRuntime.efi, which will be checked in UEFI checker.
    //
  }

  if (AsciiStrCmp (AsciiDmgLoading, "Disabled") != 0
    && AsciiStrCmp (AsciiDmgLoading, "Signed") != 0
    && AsciiStrCmp (AsciiDmgLoading, "Any") != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->DmgLoading is borked (Can only be Disabled, Signed, or Any)!\n"));
    ++ErrorCount;
  }

  if ((ExposeSensitiveData & ~OCS_EXPOSE_ALL_BITS) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->ExposeSensitiveData has unknown bits set!\n"));
    ++ErrorCount;
  }

  if (AsciiStrCmp (AsciiVault, "Optional") != 0
    && AsciiStrCmp (AsciiVault, "Basic") != 0
    && AsciiStrCmp (AsciiVault, "Secure") != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->Vault is borked (Can only be Optional, Basic, or Secure)!\n"));
    ++ErrorCount;
  }

  //
  // ScanPolicy can be zero (failsafe value).
  //
  if (ScanPolicy != 0) {
    if ((ScanPolicy & ~AllowedScanPolicy) != 0) { 
      DEBUG ((DEBUG_WARN, "Misc->Security->ScanPolicy has unknown bits set!\n"));
      ++ErrorCount;
    }

    if ((ScanPolicy & OC_SCAN_FILE_SYSTEM_BITS) != 0 && (ScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) == 0) {
      DEBUG ((DEBUG_WARN, "Misc->Security->ScanPolicy requests scanning filesystem, but OC_SCAN_FILE_SYSTEM_LOCK (bit 0) is not set!\n"));
      ++ErrorCount;
    }

    if ((ScanPolicy & OC_SCAN_DEVICE_BITS) != 0 && (ScanPolicy & OC_SCAN_DEVICE_LOCK) == 0) {
      DEBUG ((DEBUG_WARN, "Misc->Security->ScanPolicy requests scanning devices, but OC_SCAN_DEVICE_LOCK (bit 1) is not set!\n"));
      ++ErrorCount;
    }
  }

  //
  // Validate SecureBootModel.
  //
  if (!ValidateSecureBootModel (SecureBootModel)) {
    DEBUG ((DEBUG_WARN, "Misc->Security->SecureBootModel is borked!\n"));
    ++ErrorCount;
  }

  for (Index = 0; Index < UserMisc->Entries.Count; ++Index) {
    Arguments    = OC_BLOB_GET (&UserMisc->Entries.Values[Index]->Arguments);
    Comment      = OC_BLOB_GET (&UserMisc->Entries.Values[Index]->Comment);
    AsciiName    = OC_BLOB_GET (&UserMisc->Entries.Values[Index]->Name);
    Path         = OC_BLOB_GET (&UserMisc->Entries.Values[Index]->Path);

    //
    // Sanitise strings.
    //
    // NOTE: As Arguments takes identical requirements of Comment,
    //       we use Comment sanitiser here.
    //
    if (!AsciiCommentIsLegal (Arguments)) {
      DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Arguments contains illegal character!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }
    
    UnicodeName = AsciiStrCopyToUnicode (AsciiName, 0);
    if (UnicodeName != NULL) {
      if (!UnicodeIsFilteredString (UnicodeName, TRUE)) {
        DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Name contains illegal character!\n", Index));
        ++ErrorCount;
      }

      FreePool ((VOID *) UnicodeName);
    }

    //
    // FIXME: Properly sanitise Path.
    //
    if (!AsciiCommentIsLegal (Path)) {
      DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Path contains illegal character!\n", Index));
      ++ErrorCount;
    }
  }

  return ReportError (__func__, ErrorCount);
}

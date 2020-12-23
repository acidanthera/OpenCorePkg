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

UINT32
CheckMisc (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32          ErrorCount;
  OC_MISC_CONFIG  *UserMisc;
  OC_UEFI_CONFIG  *UserUefi;
  UINT32          ConsoleAttributes;
  CONST CHAR8     *HibernateMode;
  UINT32          PickerAttributes;
  UINT32          AllowedPickerAttributes;
  CONST CHAR8     *PickerMode;
  UINT64          DisplayLevel;
  UINT64          AllowedDisplayLevel;
  UINT64          HaltLevel;
  UINT64          AllowedHaltLevel;
  UINT32          Target;
  UINT32          AllowedTarget;
  CONST CHAR8     *BootProtect;
  BOOLEAN         IsRequestBootVarRoutingEnabled;
  CONST CHAR8     *AsciiDmgLoading;
  UINT32          ExposeSensitiveData;
  UINT32          AllowedExposeSensitiveData;
  CONST CHAR8     *AsciiVault;

  DEBUG ((DEBUG_VERBOSE, "config loaded into Misc checker!\n"));

  ErrorCount                     = 0;
  UserMisc                       = &Config->Misc;
  UserUefi                       = &Config->Uefi;
  ConsoleAttributes              = UserMisc->Boot.ConsoleAttributes;
  HibernateMode                  = OC_BLOB_GET (&UserMisc->Boot.HibernateMode);
  PickerAttributes               = UserMisc->Boot.PickerAttributes;
  AllowedPickerAttributes        = OC_ATTR_USE_VOLUME_ICON | OC_ATTR_USE_DISK_LABEL_FILE | OC_ATTR_USE_GENERIC_LABEL_IMAGE | OC_ATTR_USE_ALTERNATE_ICONS | OC_ATTR_USE_POINTER_CONTROL;
  PickerMode                     = OC_BLOB_GET (&UserMisc->Boot.PickerMode);
  DisplayLevel                   = UserMisc->Debug.DisplayLevel;
  AllowedDisplayLevel            = DEBUG_WARN | DEBUG_INFO | DEBUG_VERBOSE | DEBUG_ERROR;
  HaltLevel                      = DisplayLevel;
  AllowedHaltLevel               = AllowedDisplayLevel;
  Target                         = UserMisc->Debug.Target;
  AllowedTarget                  = OC_LOG_ENABLE | OC_LOG_CONSOLE | OC_LOG_DATA_HUB | OC_LOG_SERIAL | OC_LOG_VARIABLE | OC_LOG_NONVOLATILE | OC_LOG_FILE;
  BootProtect                    = OC_BLOB_GET (&UserMisc->Security.BootProtect);
  IsRequestBootVarRoutingEnabled = UserUefi->Quirks.RequestBootVarRouting;
  AsciiDmgLoading                = OC_BLOB_GET (&UserMisc->Boot.PickerMode);
  ExposeSensitiveData            = UserMisc->Security.ExposeSensitiveData;
  AllowedExposeSensitiveData     = OCS_EXPOSE_BOOT_PATH | OCS_EXPOSE_VERSION_VAR | OCS_EXPOSE_VERSION_UI | OCS_EXPOSE_OEM_INFO;
  AsciiVault                     = OC_BLOB_GET (&UserMisc->Security.Vault);

  if ((ConsoleAttributes & ~0x7FU) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->ConsoleAttributes is borked!\n"));
    ++ErrorCount;
  }

  if (AsciiStrCmp (HibernateMode, "None") != 0
    && AsciiStrCmp (HibernateMode, "Auto") != 0
    && AsciiStrCmp (HibernateMode, "RTC") != 0
    && AsciiStrCmp (HibernateMode, "NVRAM") != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->HibernateMode is borked (Can only be None, Auto, RTC, or NVRAM)!\n"));
    ++ErrorCount;
  }

  if ((PickerAttributes & ~AllowedPickerAttributes) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->PickerAttributes is borked!\n"));
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
    DEBUG ((DEBUG_WARN, "Misc->Debug->DisplayLevel is borked!\n"));
    ++ErrorCount;
  }
  if ((HaltLevel & ~AllowedHaltLevel) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->HaltLevel is borked!\n"));
    ++ErrorCount;
  }

  if ((Target & ~AllowedTarget) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Debug->Target is borked!\n"));
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

  if ((ExposeSensitiveData & ~AllowedExposeSensitiveData) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->ExposeSensitiveData is borked!\n"));
    ++ErrorCount;
  }

  if (AsciiStrCmp (AsciiVault, "Optional") != 0
    && AsciiStrCmp (AsciiVault, "Basic") != 0
    && AsciiStrCmp (AsciiVault, "Secure") != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->Vault is borked (Can only be Optional, Basic, or Secure)!\n"));
    ++ErrorCount;
  }

  return ReportError (__func__, ErrorCount);
}

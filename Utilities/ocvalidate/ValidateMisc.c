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

UINT32
CheckMisc (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32          ErrorCount;
  OC_MISC_CONFIG  *UserMisc;
  // OC_UEFI_CONFIG  *UserUefi;
  UINT32          ConsoleAttributes;
  CONST CHAR8     *HibernateMode;
  UINT32          PickerAttributes;
  UINT32          AllowedPickerAttributes;
  CONST CHAR8     *PickerMode;
  UINT64          DisplayLevel;
  UINT64          AllowedDisplayLevel;

  DEBUG ((DEBUG_VERBOSE, "config loaded into Misc checker!\n"));

  ErrorCount              = 0;
  UserMisc                = &Config->Misc;
  // UserUefi                = &Config->Uefi;
  ConsoleAttributes       = UserMisc->Boot.ConsoleAttributes;
  HibernateMode           = OC_BLOB_GET (&UserMisc->Boot.HibernateMode);
  PickerAttributes        = UserMisc->Boot.PickerAttributes;
  AllowedPickerAttributes = OC_ATTR_USE_VOLUME_ICON | OC_ATTR_USE_DISK_LABEL_FILE | OC_ATTR_USE_GENERIC_LABEL_IMAGE | OC_ATTR_USE_ALTERNATE_ICONS | OC_ATTR_USE_POINTER_CONTROL;
  PickerMode              = OC_BLOB_GET (&UserMisc->Boot.PickerMode);
  DisplayLevel            = UserMisc->Debug.DisplayLevel;
  AllowedDisplayLevel     = DEBUG_WARN | DEBUG_INFO | DEBUG_VERBOSE | DEBUG_ERROR;

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

  return ReportError (__func__, ErrorCount);
}

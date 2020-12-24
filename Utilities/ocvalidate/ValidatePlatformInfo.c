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

#include <Library/OcMacInfoLib.h>

//
// NOTE: Only PlatformInfo->Generic is checked here. The rest is ignored.
//
UINT32
CheckPlatformInfo (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32              ErrorCount;
  EFI_STATUS          Status;
  OC_PLATFORM_CONFIG  *UserPlatformInfo;
  BOOLEAN             IsAutomaticEnabled;
  CONST CHAR8         *UpdateSMBIOSMode;
  CONST CHAR8         *SystemProductName;
  CONST CHAR8         *SystemMemoryStatus;
  CONST CHAR8         *AsciiSystemUUID;
  GUID                Guid;

  DEBUG ((DEBUG_VERBOSE, "config loaded into PlatformInfo checker!\n"));

  ErrorCount         = 0;
  UserPlatformInfo   = &Config->PlatformInfo;
  IsAutomaticEnabled = UserPlatformInfo->Automatic;
  UpdateSMBIOSMode   = OC_BLOB_GET (&UserPlatformInfo->UpdateSmbiosMode);
  SystemProductName  = OC_BLOB_GET (&UserPlatformInfo->Generic.SystemProductName);
  SystemMemoryStatus = OC_BLOB_GET (&UserPlatformInfo->Generic.SystemMemoryStatus);
  AsciiSystemUUID    = OC_BLOB_GET (&UserPlatformInfo->Generic.SystemUuid);

  if (AsciiStrCmp (UpdateSMBIOSMode, "TryOverwrite") != 0
    && AsciiStrCmp (UpdateSMBIOSMode, "Create") != 0
    && AsciiStrCmp (UpdateSMBIOSMode, "Overwrite") != 0
    && AsciiStrCmp (UpdateSMBIOSMode, "Custom") != 0) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->UpdateSMBIOSMode is borked (Can only be TryOverwrite, Create, Overwrite, or Custom)!\n"));
    ++ErrorCount;
  }

  if (!IsAutomaticEnabled) {
    //
    // This is not an error, but we need to stop checking further.
    //
    return ReportError (__func__, ErrorCount);
  }

  if (!HasMacInfo (SystemProductName)) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Generic->SystemProductName has unknown model set!\n"));
    ++ErrorCount;
  }

  if (AsciiStrCmp (SystemMemoryStatus, "Auto") != 0
    && AsciiStrCmp (SystemMemoryStatus, "Upgradable") != 0
    && AsciiStrCmp (SystemMemoryStatus, "Soldered") != 0) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Generic->SystemMemoryStatus is borked (Can only be Auto, Upgradable, or Soldered)!\n"));
    ++ErrorCount;
  }

  Status = AsciiStrToGuid (AsciiSystemUUID, &Guid);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Generic->SystemUUID is borked!\n"));
    ++ErrorCount;
  }

  //
  // TODO: Sanitise MLB, ProcessorType, and SystemSerialNumber if possible...
  //

  return ReportError (__func__, ErrorCount);
}

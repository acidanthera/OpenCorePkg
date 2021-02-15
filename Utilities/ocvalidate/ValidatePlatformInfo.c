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
#include <IndustryStandard/AppleSmBios.h>

STATIC
BOOLEAN
ValidateProcessorType (
  IN  UINT16  ProcessorType
  )
{
  UINTN               Index;
  STATIC CONST UINT8  AllowedProcessorType[] = {
    AppleProcessorMajorCore,
    AppleProcessorMajorCore2,
    AppleProcessorMajorXeonPenryn,
    AppleProcessorMajorXeonNehalem,
    AppleProcessorMajorI5,
    AppleProcessorMajorI7,
    AppleProcessorMajorI3,
    AppleProcessorMajorI9,
    AppleProcessorMajorXeonE5,
    AppleProcessorMajorM,
    AppleProcessorMajorM3,
    AppleProcessorMajorM5,
    AppleProcessorMajorM7,
    AppleProcessorMajorXeonW
  };

  //
  // 0 is allowed.
  //
  if (ProcessorType == 0U) {
    return TRUE;
  }

  for (Index = 0; Index < ARRAY_SIZE (AllowedProcessorType); ++Index) {
    if ((ProcessorType >> 8U) == AllowedProcessorType[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

//
// NOTE: Only PlatformInfo->Generic is checked here. The rest is ignored.
//

STATIC
UINT32
CheckPlatformInfoGeneric (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32              ErrorCount;
  OC_PLATFORM_CONFIG  *UserPlatformInfo;
  CONST CHAR8         *SystemProductName;
  CONST CHAR8         *SystemMemoryStatus;
  CONST CHAR8         *AsciiSystemUUID;
  UINT16              ProcessorType;

  ErrorCount          = 0;
  UserPlatformInfo    = &Config->PlatformInfo;

  SystemProductName   = OC_BLOB_GET (&UserPlatformInfo->Generic.SystemProductName);
  if (!HasMacInfo (SystemProductName)) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Generic->SystemProductName has unknown model set!\n"));
    ++ErrorCount;
  }

  SystemMemoryStatus  = OC_BLOB_GET (&UserPlatformInfo->Generic.SystemMemoryStatus);
  if (AsciiStrCmp (SystemMemoryStatus, "Auto") != 0
    && AsciiStrCmp (SystemMemoryStatus, "Upgradable") != 0
    && AsciiStrCmp (SystemMemoryStatus, "Soldered") != 0) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Generic->SystemMemoryStatus is borked (Can only be Auto, Upgradable, or Soldered)!\n"));
    ++ErrorCount;
  }

  AsciiSystemUUID     = OC_BLOB_GET (&UserPlatformInfo->Generic.SystemUuid);
  if (AsciiSystemUUID[0] != '\0'
    && AsciiStrCmp (AsciiSystemUUID, "OEM") != 0
    && !AsciiGuidIsLegal (AsciiSystemUUID)) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Generic->SystemUUID is borked (Can only be empty, special string OEM or valid UUID)!\n"));
    ++ErrorCount;
  }

  ProcessorType       = UserPlatformInfo->Generic.ProcessorType;
  if (!ValidateProcessorType (ProcessorType)) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Generic->ProcessorType is borked!\n"));
    ++ErrorCount;
  }

  //
  // TODO: Sanitise MLB, ProcessorType, and SystemSerialNumber if possible...
  //

  return ErrorCount;
}

UINT32
CheckPlatformInfo (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32               ErrorCount;
  OC_PLATFORM_CONFIG   *UserPlatformInfo;
  BOOLEAN              IsAutomaticEnabled;
  CONST CHAR8          *UpdateSMBIOSMode;
  UINTN                Index;
  STATIC CONFIG_CHECK  PlatformInfoCheckers[] = {
    &CheckPlatformInfoGeneric
  };

  DEBUG ((DEBUG_VERBOSE, "config loaded into %a!\n", __func__));

  ErrorCount          = 0;
  UserPlatformInfo    = &Config->PlatformInfo;
  
  UpdateSMBIOSMode    = OC_BLOB_GET (&UserPlatformInfo->UpdateSmbiosMode);
  if (AsciiStrCmp (UpdateSMBIOSMode, "TryOverwrite") != 0
    && AsciiStrCmp (UpdateSMBIOSMode, "Create") != 0
    && AsciiStrCmp (UpdateSMBIOSMode, "Overwrite") != 0
    && AsciiStrCmp (UpdateSMBIOSMode, "Custom") != 0) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->UpdateSMBIOSMode is borked (Can only be TryOverwrite, Create, Overwrite, or Custom)!\n"));
    ++ErrorCount;
  }
  
  IsAutomaticEnabled = UserPlatformInfo->Automatic;
  if (!IsAutomaticEnabled) {
    //
    // This is not an error, but we need to stop checking further.
    //
    return ReportError (__func__, ErrorCount);
  }

  for (Index = 0; Index < ARRAY_SIZE (PlatformInfoCheckers); ++Index) {
    ErrorCount += PlatformInfoCheckers[Index] (Config);
  }

  return ReportError (__func__, ErrorCount);
}

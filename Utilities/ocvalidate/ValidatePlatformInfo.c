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
  OC_PLATFORM_CONFIG  *UserPlatformInfo;
  BOOLEAN             IsAutomaticEnabled;

  DEBUG ((DEBUG_VERBOSE, "config loaded into PlatformInfo checker!\n"));

  ErrorCount         = 0;
  UserPlatformInfo   = &Config->PlatformInfo;
  IsAutomaticEnabled = UserPlatformInfo->Automatic;

  if (!IsAutomaticEnabled) {
    DEBUG ((DEBUG_WARN, "PlatformInfo->Automatic is not enabled!\n"));
    ++ErrorCount;
  }

  //
  // TODO: Check properties with OcMacInfoLib.
  //

  return ReportError (__func__, ErrorCount);
}

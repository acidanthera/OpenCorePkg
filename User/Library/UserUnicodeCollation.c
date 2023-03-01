/** @file
  Copyright (c) 2023, Savva Mitrofanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserPcd.h>
#include <UserUnicodeCollation.h>

UINT8  _gPcd_FixedAtBuild_PcdUefiVariableDefaultLang[4]         = { 101, 110, 103, 0 };
UINT8  _gPcd_FixedAtBuild_PcdUefiVariableDefaultPlatformLang[6] = { 101, 110, 45, 85, 83, 0 };

VOID
UserUnicodeCollationInstallProtocol (
  OUT EFI_UNICODE_COLLATION_PROTOCOL  **Interface
  )
{
  OcUnicodeCollationInitializeMappingTables ();

  *Interface = &gInternalUnicode2Eng;
}

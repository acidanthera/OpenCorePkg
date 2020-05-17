/** @file
  BIOS ID library definitions.

  This library provides functions to get BIOS ID, ROM Info, VERSION and DATE

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
Portions Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef BIOS_ID_LIB_H
#define BIOS_ID_LIB_H

// GetBiosVersionDateTime
/** This function returns the Version & Release Date by getting and converting
    BIOS ID.

  @param[out] BiosVersion      The Bios Version out of the conversion.
  @param[out] BiosReleaseDate  The Bios Release Date out of the conversion.
**/
VOID
GetBiosVersionDateTime (
  OUT CHAR8  **BiosVersion,
  OUT CHAR8  **BiosReleaseDate
  );

// GetRomInfo
/** This function returns ROM Info by searching FV.

  @param[out] RomInfo  ROM Info
**/
VOID
GetRomInfo (
  OUT APPLE_ROM_INFO_STRING  *RomInfo
  );

#endif // BIOS_ID_LIB_H

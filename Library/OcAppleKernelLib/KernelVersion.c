/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>

#include "PrelinkedInternal.h"

UINT32
OcParseDarwinVersion (
  IN  CONST CHAR8         *String
  )
{
  UINT32  Version;
  UINT32  VersionPart;
  UINT32  Index;
  UINT32  Index2;

  if (*String == '\0' || *String < '0' || *String > '9') {
    return 0;
  }

  Version = 0;

  for (Index = 0; Index < 3; ++Index) {
    Version *= 100;

    VersionPart = 0;

    for (Index2 = 0; Index2 < 2; ++Index2) {
      //
      // Handle single digit parts, i.e. parse 1.2.3 as 010203.
      //
      if (*String != '.' && *String != '\0') {
        VersionPart *= 10;
      }

      if (*String >= '0' && *String <= '9') {
        VersionPart += *String++ - '0';
      } else if (*String != '.' && *String != '\0') {
        return 0;
      }
    }

    Version += VersionPart;

    if (*String == '.') {
      ++String;
    }
  }

  return Version;
}

BOOLEAN
OcMatchDarwinVersion (
  IN  UINT32  CurrentVersion  OPTIONAL,
  IN  UINT32  MinVersion      OPTIONAL,
  IN  UINT32  MaxVersion      OPTIONAL
  )
{
  //
  // Check against min <= curr <= max.
  // curr=0 -> curr=inf, max=0  -> max=inf
  //

  //
  // Replace max inf with max known version.
  //
  if (MaxVersion == 0) {
    MaxVersion = CurrentVersion;
  }

  //
  // Handle curr=inf <= max=inf(?) case.
  //
  if (CurrentVersion == 0) {
    return MaxVersion == 0;
  }

  //
  // Handle curr=num > max=num case.
  //
  if (CurrentVersion > MaxVersion) {
    return FALSE;
  }

  //
  // Handle min=num > curr=num case.
  //
  if (CurrentVersion < MinVersion) {
    return FALSE;
  }

  return TRUE;
}

UINT32
OcKernelReadDarwinVersion (
  IN  CONST UINT8   *Kernel,
  IN  UINT32        KernelSize
  )
{
  BOOLEAN Exists;
  UINT32  Offset;
  UINT32  Index;
  CHAR8   DarwinVersion[32];
  UINT32  DarwinVersionInteger;

  Offset = 0;
  Exists = FindPattern (
    (CONST UINT8 *) "Darwin Kernel Version ",
    NULL,
    L_STR_LEN ("Darwin Kernel Version "),
    Kernel,
    KernelSize,
    &Offset
    );

  if (!Exists) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to determine kernel version\n"));
    return 0;
  }

  Offset += L_STR_LEN ("Darwin Kernel Version ");

  for (Index = 0; Index < ARRAY_SIZE (DarwinVersion) - 1; ++Index, ++Offset) {
    if (Offset >= KernelSize || Kernel[Offset] == ':') {
      break;
    }
    DarwinVersion[Index] = (CHAR8) Kernel[Offset];
  }
  DarwinVersion[Index] = '\0';
  DarwinVersionInteger = OcParseDarwinVersion (DarwinVersion);

  DEBUG ((
    DEBUG_INFO,
    "OCAK: Read kernel version %a (%u)\n",
    DarwinVersion,
    DarwinVersionInteger
    ));

  return DarwinVersionInteger;
}

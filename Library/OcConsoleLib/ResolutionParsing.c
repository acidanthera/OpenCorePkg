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

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcGuardLib.h>

/**
  Parse resolution string.

  @param[in]   String   Resolution in WxH@Bpp or WxH format.
  @param[out]  Width    Parsed width or 0.
  @param[out]  Height   Parsed height or 0.
  @param[out]  Bpp      Parsed Bpp or 0, optional to force WxH format.
  @param[out]  Max      Set to TRUE when String equals to Max.
**/
STATIC
VOID
ParseResolution (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT UINT32              *Bpp OPTIONAL,
  OUT BOOLEAN             *Max
  )
{
  UINT32  TmpWidth;
  UINT32  TmpHeight;

  *Width  = 0;
  *Height = 0;
  *Max    = FALSE;

  if (Bpp != NULL) {
    *Bpp    = 0;
  }

  if (AsciiStrCmp (String, "Max") == 0) {
    *Max = TRUE;
    return;
  }

  if (*String == '\0' || *String < '0' || *String > '9') {
    return;
  }

  TmpWidth = TmpHeight = 0;

  while (*String >= '0' && *String <= '9') {
    if (OcOverflowMulAddU32 (TmpWidth, 10, *String++ - '0', &TmpWidth)) {
      return;
    }
  }

  if (*String++ != 'x' || *String < '0' || *String > '9') {
    return;
  }

  while (*String >= '0' && *String <= '9') {
    if (OcOverflowMulAddU32 (TmpHeight, 10, *String++ - '0', &TmpHeight)) {
      return;
    }
  }

  if (*String != '\0' && (*String != '@' || Bpp == NULL)) {
    return;
  }

  *Width  = TmpWidth;
  *Height = TmpHeight;

  if (*String == '\0') {
    return;
  }

  TmpWidth = 0;
  while (*String >= '0' && *String <= '9') {
    if (OcOverflowMulAddU32 (TmpWidth, 10, *String++ - '0', &TmpWidth)) {
      return;
    }
  }

  if (*String != '\0') {
    return;
  }

  *Bpp = TmpWidth;
}

VOID
OcParseScreenResolution (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT UINT32              *Bpp,
  OUT BOOLEAN             *Max
  )
{
  ASSERT (String != NULL);
  ASSERT (Width != NULL);
  ASSERT (Height != NULL);
  ASSERT (Bpp != NULL);
  ASSERT (Max != NULL);

  ParseResolution (String, Width, Height, Bpp, Max);
}

VOID
OcParseConsoleMode (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT BOOLEAN             *Max
  )
{
  ASSERT (String != NULL);
  ASSERT (Width != NULL);
  ASSERT (Height != NULL);
  ASSERT (Max != NULL);

  ParseResolution (String, Width, Height, NULL, Max);
}

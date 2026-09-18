/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  Debug.c

Abstract:

Revision History:

--*/
#include "EfiLdr.h"
#include "Debug.h"

UINT8  *mCursor;
UINT8  mHeaderIndex = 10;

VOID
PrintHeader (
  CHAR8  Char
  )
{
  *(UINT8 *)(UINTN)(0x000b8000 + mHeaderIndex) = Char;
  mHeaderIndex                                += 2;
}

VOID
ClearScreen (
  VOID
  )
{
  UINT32  Index;

  mCursor = (UINT8 *)(UINTN)(0x000b8000 + 160);
  for (Index = 0; Index < 80 * 49; Index++) {
    *mCursor = ' ';
    mCursor += 2;
  }

  mCursor = (UINT8 *)(UINTN)(0x000b8000 + 160);
}

VOID
EFIAPI
PrintString (
  IN CONST CHAR8  *FormatString,
  ...
  )
{
  UINTN    Index;
  CHAR8    PrintBuffer[256];
  VA_LIST  Marker;

  VA_START (Marker, FormatString);
  AsciiVSPrint (PrintBuffer, sizeof (PrintBuffer), FormatString, Marker);
  VA_END (Marker);

  for (Index = 0; PrintBuffer[Index] != 0; Index++) {
    if (PrintBuffer[Index] == '\n') {
      mCursor = (UINT8 *)(UINTN)(0xb8000 + (((((UINTN)mCursor - 0xb8000) + 160) / 160) * 160));
    } else {
      *mCursor = (UINT8)PrintBuffer[Index];
      mCursor += 2;
    }
  }

  //
  // All information also output to serial port.
  //
  SerialPortWrite ((UINT8 *)PrintBuffer, Index);
}

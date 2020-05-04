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

#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

VOID
DebugPrintDevicePath (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  DEBUG_CODE_BEGIN();

  CHAR16 *TextDevicePath;

  TextDevicePath = ConvertDevicePathToText (DevicePath, TRUE, FALSE);
  DEBUG ((ErrorLevel, "%a - %s\n", Message, OC_HUMAN_STRING (TextDevicePath)));
  if (TextDevicePath != NULL) {
    FreePool (TextDevicePath);
  }

  DEBUG_CODE_END();
}

VOID
DebugPrintHexDump (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN UINT8                     *Bytes,
  IN UINTN                     Size
  )
{
  DEBUG_CODE_BEGIN();

  UINTN  Index;
  UINTN  Index2;
  UINTN  Count;
  UINTN  SizeLeft;
  UINTN  MaxPerLine;
  CHAR8  *HexLine;
  CHAR8  *HexLineCurrent;

  MaxPerLine = 64;
  Count      = (Size + MaxPerLine - 1) / MaxPerLine;

  HexLine = AllocatePool (MaxPerLine * 3 + 1);
  if (HexLine == NULL) {
    return;
  }

  SizeLeft = Size;

  for (Index = 0; Index < Count; ++Index) {
    HexLineCurrent = HexLine;

    for (Index2 = 0; SizeLeft > 0 && Index2 < MaxPerLine; ++Index2) {
      if (Index2 > 0) {
        *HexLineCurrent++ = ' ';
      }

      *HexLineCurrent++ = OC_HEX_UPPER (*Bytes);
      *HexLineCurrent++ = OC_HEX_LOWER (*Bytes);
      --SizeLeft;
      ++Bytes;
    }

    *HexLineCurrent++ = '\0';

    DEBUG ((
      ErrorLevel,
      "%a (%u/%u %u) - %a\n",
      Message,
      (UINT32) Index + 1,
      (UINT32) Count,
      (UINT32) Size,
      HexLine
      ));
  }

  FreePool (HexLine);

  DEBUG_CODE_END();
}

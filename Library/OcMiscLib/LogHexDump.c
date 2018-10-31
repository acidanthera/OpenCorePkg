/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <Library/DebugLib.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include <Library/OcStringLib.h>

//
//
//

EFI_STATUS
LogHexDump (
  IN  VOID      *Address,
  IN  VOID      *Address2,
  IN  UINTN     Length,
  IN  UINTN     LineSize,
  IN  BOOLEAN   DisplayAscii
  )
{
  CHAR8   *HexString;
  CHAR8   *AsciiString;
  UINT8   *Base;
  UINTN   DisplayAddress;
  UINTN   Index;
  UINTN   LineLength;

  if (Length == 0 || LineSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  HexString = AllocateZeroPool (LineSize * 4 + sizeof(CHAR8));
  AsciiString = AllocateZeroPool (LineSize + sizeof(CHAR8));

  if (HexString == NULL || AsciiString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Base = (UINT8 *)Address;

  DisplayAddress = (UINTN)Address2;

  // Iterate

  do {

    LineLength = MIN (LineSize, Length);

    for (Index = 0; Index < LineLength; Index++) {
      AsciiSPrint (&HexString[Index * 3], 4, "%02X ", (UINT8)(Base[Index]));
      AsciiSPrint (&AsciiString[Index], 2, "%c", IsAsciiPrint (Base[Index]) ? Base[Index] : '.');
    }

    if (DisplayAscii == TRUE) {

      DEBUG ((DEBUG_INFO, "0x%0*X %-*a%-*a\n",
                        4,
                        DisplayAddress,
                        LineSize * 3,
                        HexString,
                        LineSize,
                        AsciiString));
    } else {

      DEBUG ((DEBUG_INFO, "0x%0*X %-*a\n",
                        4,
                        DisplayAddress,
                        LineSize * 3,
                        HexString,
                        LineSize));
    }

    DisplayAddress += LineLength;

    Base += LineLength;
    Length -= LineLength;

  } while (Length > 0);

  //FreePool (AsciiString);
  FreePool (HexString);

  return EFI_SUCCESS;
}

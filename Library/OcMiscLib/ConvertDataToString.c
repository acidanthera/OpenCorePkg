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

#include <IndustryStandard/Pci.h>

#include <Guid/ApplePlatformInfo.h>

#include <Protocol/DataHub.h>
#include <Protocol/LegacyRegion.h>
#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcPrintLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Macros.h>

// ConvertDataToString
/** Attempt to convert the data into an ascii string.

  @param[in] Data      A pointer to the data to convert.
  @param[in] DataSize  The length of data to convert.

  @retval  An ascii string representing the data.
**/
CHAR8 *
ConvertDataToString (
  IN VOID   *Data,
  IN UINTN  DataSize
  )
{
  CHAR8   *OutputBuffer;

  CHAR8   *String;
  UINT8   *VariableData;
  UINT8   *Base;
  UINTN   VariableSize;
  UINTN   OutputLength;
  UINTN   Index;
  BOOLEAN IsAscii;
  //BOOLEAN IsNullTerminated;
  BOOLEAN IsUnicode;

  OutputBuffer = NULL;

  if ((Data != NULL) && (DataSize != 0) && (DataSize <= 256)) {
    IsAscii          = FALSE;
    //IsNullTerminated = FALSE;
    IsUnicode        = FALSE;
    VariableData     = Data;
    VariableSize     = DataSize;

    // Detect Unicode String
    if ((IS_ASCII (*(CHAR16 *)VariableData) > 0) && (DataSize > sizeof (CHAR16))) {
      IsUnicode = TRUE;

      do {
        if (!IsAsciiPrint ((CHAR8)(*(CHAR16 *)VariableData))) {
          IsUnicode = FALSE;
        }

        VariableData += sizeof (CHAR16);
        VariableSize -= sizeof (CHAR16);
      } while (VariableSize > sizeof (CHAR16));

      // Check Last Unicode Character

      if ((*(CHAR16 *)VariableData == 0) && IsUnicode) {
        //IsNullTerminated = TRUE;
      } else if (!IsAsciiPrint ((CHAR8)*(CHAR16 *)VariableData)) {
        IsUnicode = FALSE;
      }

    } else if ((IS_ASCII (*(CHAR8 *)VariableData) > 0) && (DataSize > sizeof (CHAR8))) {
      IsAscii = TRUE;

      // Detect Ascii String
      do {
        if (!IsAsciiPrint (*(CHAR8 *)VariableData)) {
          IsAscii = FALSE;
        }

        ++VariableData;
        --VariableSize;
      } while (VariableSize > sizeof (CHAR8));

      // Check Last Ascii Character

      if ((*(CHAR8 *)VariableData == 0) && IsAscii) {
        //IsNullTerminated = TRUE;
      } else if (!IsAsciiPrint (*(CHAR8 *)VariableData)) {
        IsAscii = FALSE;
      }
    }

    // Support Max 128 Chars
    OutputLength = MIN (128, DataSize);

    // Create Buffer Adding Space for Quotes and Null Terminator
    OutputBuffer = AllocateZeroPool ((UINTN)MultU64x32 (OutputLength, 4));

    if (OutputBuffer != NULL) {
      Base   = Data;
      String = OutputBuffer;

      if (IsAscii) {
        // Create "AsciiString" Output

        *(String++) = '\"';

        do {

          if (*Base == '\0' || DataSize == 0) {
            break;
          }

          *(String++) = *(Base++);

        } while ((--OutputLength > 0) && (--DataSize > 0));

        *(String++) = '\"';
        *(String++) = '\0';

      } else if (IsUnicode ) {
        // Create "UnicodeString" Output

        *(String++) = '\"';

        do {
          if (*(CHAR16 *)Base == L'\0' || DataSize == 0) {
            break;
          }

          *(String++) = (CHAR8)*(CHAR16 *)Base;

          Base     += sizeof (CHAR16);
          DataSize -= sizeof (CHAR16);

        } while ((--OutputLength > 0) && (DataSize > 0));

        *(String++) = '\"';
        *(String++) = '\0';

      } else {
        // Create Hex String Output
        for (Index = 0; Index < MIN (32, OutputLength); Index++) {
          OcSPrint (&OutputBuffer[Index * 3], 4, OUTPUT_ASCII, "%02X ", Base[Index]);
        }
      }
    }
  }

  return OutputBuffer;
}

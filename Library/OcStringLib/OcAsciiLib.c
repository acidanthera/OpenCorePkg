/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcStringLib.h>

// IsAsciiPrint
/** Check if character is printable

  @param[in] Char  The ascii character to check if is printable.

  @retval  TRUE, if character is printable.
**/
BOOLEAN
IsAsciiPrint (
  IN CHAR8  Char
  )
{
  return ((Char >= ' ') && (Char < '~'));
}

// IsAsciiSpace
/** Check if character is a white space character

  @param[in] Char  The ascii character to check if is white space.

  @retval  TRUE, if character is a white space character
**/
INTN
IsAsciiSpace (
  IN CHAR8  Char
  )
{
  return ((Char == ' ')
       || (Char == '\t')
       || (Char == '\v')
       || (Char == '\f')
       || (Char == '\r')
       || (Char == '\n'));
}

/** Convert null terminated ascii string to unicode.

  @param[in]  String1  A pointer to the ascii string to convert to unicode.
  @param[in]  Length   Length or 0 to calculate the length of the ascii string to convert.

  @retval  A pointer to the converted unicode string allocated from pool.
**/
CHAR16 *
AsciiStrCopyToUnicode (
  IN  CONST CHAR8   *AsciiString,
  IN  UINTN         Length
  )
{
  CHAR16  *UnicodeString;
  CHAR16  *UnicodeStringWalker;
  UINTN   UnicodeStringSize;

  ASSERT (AsciiString != NULL);

  if (Length == 0) {
    Length = AsciiStrLen (AsciiString);
  }

  UnicodeStringSize = (Length + 1) * sizeof (CHAR16);
  UnicodeString = AllocatePool (UnicodeStringSize);

  if (UnicodeString != NULL) {
    UnicodeStringWalker = UnicodeString;
    while (*AsciiString != '\0' && Length--) {
      *(UnicodeStringWalker++) = *(AsciiString++);
    }
    *UnicodeStringWalker = L'\0';
  }

  return UnicodeString;
}

BOOLEAN
AsciiUint64ToLowerHex (
  OUT CHAR8   *Buffer,
  IN  UINT32  BufferSize,
  IN  UINT64  Value
  )
{
  CONST UINT32  MaxShifts = (sizeof (UINT64) * 8) - 4;
  UINT32        Index;
  BOOLEAN       Printed;
  UINT8         Curr;

  if (BufferSize < 4) {
    return FALSE;
  }

  *Buffer++   = '0';
  *Buffer++   = 'x';

  if (Value > 0) {
    BufferSize -= 2;
    for (Printed = FALSE, Index = MaxShifts; Index <= MaxShifts; Index -= 4) {
      Curr     = (UINT8) (RShiftU64 (Value, Index) & 0xFU);
      Printed |= Curr > 0;
      if (Printed) {
        *Buffer++ = "0123456789abcdef"[Curr];
        if (--BufferSize == 0) {
          return FALSE;
        }
      }
    }
  } else {
    *Buffer++ = '0';
  }

  *Buffer++ = '\0';
  return TRUE;
}

EFI_STATUS
EFIAPI
OcAsciiSafeSPrint (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Marker;
  VA_LIST     Marker2;
  UINTN       NumberOfPrinted;

  ASSERT (StartOfBuffer != NULL);
  ASSERT (BufferSize > 0);
  ASSERT (FormatString != NULL);

  VA_START (Marker, FormatString);

  VA_COPY (Marker2, Marker);
  NumberOfPrinted = SPrintLengthAsciiFormat (FormatString, Marker2);
  VA_END (Marker2);

  if (BufferSize - 1 >= NumberOfPrinted) {
    AsciiVSPrint (StartOfBuffer, BufferSize, FormatString, Marker);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_OUT_OF_RESOURCES;
  }

  VA_END (Marker);

  return Status;
}

BOOLEAN
EFIAPI
OcAsciiEndsWith (
  IN CONST CHAR8      *String,
  IN CONST CHAR8      *SearchString
  )
{
  UINTN   StringLength;
  UINTN   SearchStringLength;

  ASSERT (String != NULL);
  ASSERT (SearchString != NULL);

  StringLength        = AsciiStrLen (String);
  SearchStringLength  = AsciiStrLen (SearchString);

  return StringLength >= SearchStringLength
    && AsciiStrnCmp (&String[StringLength - SearchStringLength], SearchString, SearchStringLength) == 0;
}

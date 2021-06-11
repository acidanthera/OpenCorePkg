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
#include <Library/OcGuardLib.h>
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

// IsAsciiAlpha
/** Check if character is alphabetical.

  @param[in] Char  The ascii character to check if is alphabetical.

  @retval  TRUE, if character is alphabetical.
**/
INTN
IsAsciiAlpha (
  IN CHAR8  Char
  )
{
  return ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z'));
}

// IsAsciiSpace
/** Check if character is a white space character.

  @param[in] Char  The ascii character to check if is white space.

  @retval  TRUE, if character is a white space character.
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

BOOLEAN
IsAsciiNumber (
  IN CHAR8  Char
  )
{
  return Char >= L'0' && Char <= L'9';
}

VOID
AsciiUefiSlashes (
  IN OUT CHAR8    *String
  )
{
  CHAR8  *Needle;

  Needle = String;
  while ((Needle = AsciiStrStr (Needle, "/")) != NULL) {
    *Needle++ = '\\';
  }
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

INTN
EFIAPI
OcAsciiStrniCmp (
  IN CONST CHAR8   *FirstString,
  IN CONST CHAR8   *SecondString,
  IN UINTN         Length
  )
{
  CHAR8  UpperFirstString;
  CHAR8  UpperSecondString;

  if (Length == 0) {
    return 0;
  }

  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength.
  // Length tests are performed inside AsciiStrLen().
  //
  ASSERT (AsciiStrSize (FirstString) != 0);
  ASSERT (AsciiStrSize (SecondString) != 0);

  UpperFirstString  = AsciiCharToUpper (*FirstString);
  UpperSecondString = AsciiCharToUpper (*SecondString);
  while ((*FirstString != '\0') &&
         (*SecondString != '\0') &&
         (UpperFirstString == UpperSecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    UpperFirstString  = AsciiCharToUpper (*FirstString);
    UpperSecondString = AsciiCharToUpper (*SecondString);
    Length--;
  }

  return UpperFirstString - UpperSecondString;
}

BOOLEAN
EFIAPI
OcAsciiEndsWith (
  IN CONST CHAR8      *String,
  IN CONST CHAR8      *SearchString,
  IN BOOLEAN          CaseInsensitiveMatch
  )
{
  UINTN   StringLength;
  UINTN   SearchStringLength;

  ASSERT (String != NULL);
  ASSERT (SearchString != NULL);

  StringLength        = AsciiStrLen (String);
  SearchStringLength  = AsciiStrLen (SearchString);

  if (CaseInsensitiveMatch) {
    return StringLength >= SearchStringLength
      && OcAsciiStrniCmp (&String[StringLength - SearchStringLength], SearchString, SearchStringLength) == 0;
  }
  return StringLength >= SearchStringLength
    && AsciiStrnCmp (&String[StringLength - SearchStringLength], SearchString, SearchStringLength) == 0;
}

BOOLEAN
EFIAPI
OcAsciiStartsWith (
  IN CONST CHAR8      *String,
  IN CONST CHAR8      *SearchString,
  IN BOOLEAN          CaseInsensitiveMatch
  )
{
  CHAR8   First;
  CHAR8   Second;

  ASSERT (String != NULL);
  ASSERT (SearchString != NULL);

  while (TRUE) {
    First = *String++;
    Second = *SearchString++;
    if (Second == '\0') {
      return TRUE;
    }
    if (First == '\0') {
      return FALSE;
    }
    if (CaseInsensitiveMatch) {
      First  = AsciiCharToUpper (First);
      Second = AsciiCharToUpper (Second);
    }
    if (First != Second) {
      return FALSE;
    }
  }
}

CHAR8 *
EFIAPI
OcAsciiStriStr (
  IN      CONST CHAR8              *String,
  IN      CONST CHAR8              *SearchString
  )
{
  CONST CHAR8 *FirstMatch;
  CONST CHAR8 *SearchStringTmp;

  ASSERT (AsciiStrSize (String) != 0);
  ASSERT (AsciiStrSize (SearchString) != 0);

  if (*SearchString == '\0') {
    return (CHAR8 *) String;
  }

  while (*String != '\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;

    while ((AsciiCharToUpper (*String) == AsciiCharToUpper (*SearchStringTmp))
            && (*String != '\0')) {
      String++;
      SearchStringTmp++;
    }

    if (*SearchStringTmp == '\0') {
      return (CHAR8 *) FirstMatch;
    }

    if (*String == '\0') {
      return NULL;
    }

    String = FirstMatch + 1;
  }

  return NULL;
}

CHAR8 *
EFIAPI
OcAsciiStrChr (
  IN      CONST CHAR8              *String,
  IN            CHAR8              Char
  )
{
  ASSERT (AsciiStrSize (String) != 0);

  while (*String != '\0') {
    //
    // Return immediately when matching first occurrence of Char.
    //
    if (*String == Char) {
      return (CHAR8 *) String;
    }

    ++String;
  }

  return NULL;
}

CHAR8 *
EFIAPI
OcAsciiStrrChr (
  IN      CONST CHAR8              *String,
  IN            CHAR8              Char
  )
{
  CHAR8 *Save;

  ASSERT (AsciiStrSize (String) != 0);

  Save = NULL;

  while (*String != '\0') {
    //
    // Record the last occurrence of Char.
    //
    if (*String == Char) {
      Save = (CHAR8 *) String;
    }

    ++String;
  }

  return Save;
}

BOOLEAN
OcAsciiStringNPrintable (
  IN  CONST CHAR8  *String,
  IN  UINTN        Number
  )
{
  UINTN  Index;

  for (Index = 0; Index < Number; ++Index) {
    //
    // Terminate search if non-printable character is found.
    // The next IFs determine the return value.
    //
    if (!IsAsciiPrint (String[Index])) {
      break;
    }
  }

  //
  // If the loop above can be terminated without errors, Index should equal to ValueSize.
  // And this is all good.
  //
  if (Index == Number) {
    return TRUE;
  }

  //
  // If the last character is not ASCII-printable but is '\0', then it is fine.
  //
  if (Index == Number - 1) {
    return String[Index] == '\0';
  }

  //
  // Otherwise, the string is broken.
  //
  return FALSE;
}

EFI_STATUS
EFIAPI
OcAsciiStrToRawGuid (
  IN  CONST CHAR8        *String,
  OUT GUID               *Guid
  )
{
  EFI_STATUS  Status;

  Status = AsciiStrToGuid (String, Guid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Swap little endian numbers to their big endian counter parts.
  //
  Guid->Data1 = SwapBytes32 (Guid->Data1);
  Guid->Data2 = SwapBytes16 (Guid->Data2);
  Guid->Data3 = SwapBytes16 (Guid->Data3);
  return EFI_SUCCESS;
}

VOID
AsciiFilterString (
  IN OUT CHAR8    *String,
  IN     BOOLEAN  SingleLine
  )
{
  while (*String != L'\0') {
    if ((*String & 0x7F) != *String) {
      //
      // Remove all unicode characters.
      //
      *String = '_';
    } else if (SingleLine && (*String == '\r' || *String == '\n')) {
      //
      // Stop after printing one line.
      //
      *String = '\0';
      break;
    } else if (*String < 0x20 || *String == 0x7F) {
      //
      // Drop all unprintable spaces but space including tabs.
      //
      *String = '_';
    }

    ++String;
  }
}

VOID
EFIAPI
OcAsciiPrintBuffer (
  IN OUT CHAR8        **AsciiBuffer,
  IN OUT UINTN        *AsciiBufferSize,
  IN     CONST CHAR8  *FormatString,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Marker;
  CHAR8       Tmp[256];
  CHAR8       *NewBuffer;
  UINTN       NewBufferSize;

  if (*AsciiBuffer == NULL) {
    return;
  }

  VA_START (Marker, FormatString);
  AsciiVSPrint (Tmp, sizeof (Tmp), FormatString, Marker);
  VA_END (Marker);

  Status = AsciiStrCatS (*AsciiBuffer, *AsciiBufferSize, Tmp);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    if (OcOverflowMulUN (*AsciiBufferSize, 2, &NewBufferSize)) {
      return;
    }
    NewBuffer = ReallocatePool (*AsciiBufferSize, NewBufferSize, *AsciiBuffer);
    if (NewBuffer == NULL) {
      FreePool (*AsciiBuffer);

      *AsciiBuffer     = NULL;
      *AsciiBufferSize = 0;

      return;
    }

    *AsciiBuffer      = NewBuffer;
    *AsciiBufferSize  = NewBufferSize;

    AsciiStrCatS (*AsciiBuffer, *AsciiBufferSize, Tmp);
  }
}

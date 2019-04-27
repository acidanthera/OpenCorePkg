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

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcStringLib.h>
#include <Library/PcdLib.h>

/**
  Convert a Unicode character to upper case only if
  it maps to a valid small-case ASCII character.

  This internal function only deal with Unicode character
  which maps to a valid small-case ASII character, i.e.
  L'a' to L'z'. For other Unicode character, the input character
  is returned directly.


  @param  Char  The character to convert.

  @retval LowerCharacter   If the Char is with range L'a' to L'z'.
  @retval Unchanged        Otherwise.

**/
STATIC
CHAR16
InternalCharToUpper (
  IN      CHAR16                    Char
  )
{
  //
  // FIXME: Drop this function once new UDK lands and brings native CharToUpper.
  //
  if (Char >= L'a' && Char <= L'z') {
    return (CHAR16) (Char - (L'a' - L'A'));
  }

  return Char;
}

/**
  Performs a case insensitive comparison of two Null-terminated Unicode strings,
  and returns the difference between the first mismatched Unicode characters.

  This function performs a case insensitive comparison of the Null-terminated
  Unicode string FirstString to the Null-terminated Unicode string
  SecondString. If FirstString is identical to SecondString, then 0 is
  returned. Otherwise, the value returned is the first mismatched upper case
  Unicode character in SecondString subtracted from the first mismatched upper
  case Unicode character in FirstString.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  FirstString   A pointer to a Null-terminated Unicode string.
  @param  SecondString  A pointer to a Null-terminated Unicode string.

  @retval ==0    FirstString is identical to SecondString using case
                 insensitiv comparisons.
  @retval !=0    FirstString is not identical to SecondString using case
                 insensitive comparisons.

**/
INTN
EFIAPI
StriCmp (
  IN CHAR16  *FirstString,
  IN CHAR16  *SecondString
  )
{
  CHAR16  UpperFirstString;
  CHAR16  UpperSecondString;

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength
  //
  ASSERT (StrSize (FirstString) != 0);
  ASSERT (StrSize (SecondString) != 0);

  UpperFirstString  = InternalCharToUpper (*FirstString);
  UpperSecondString = InternalCharToUpper (*SecondString);
  while ((*FirstString != '\0') && (*SecondString != '\0') && (UpperFirstString == UpperSecondString)) {
    FirstString++;
    SecondString++;
    UpperFirstString  = InternalCharToUpper (*FirstString);
    UpperSecondString = InternalCharToUpper (*SecondString);
  }

  return UpperFirstString - UpperSecondString;
}

/**
  Compares up to a specified length the contents of two Null-terminated Unicode
  strings using case insensitive comparisons, and returns the difference
  between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString using case insensitive
  comparisons.  At most, Length Unicode characters will be compared. If Length
  is 0, then 0 is returned. If FirstString is identical to SecondString, then 0
  is returned. Otherwise, the value returned is the first mismatched upper case
  Unicode character in SecondString subtracted from the first mismatched upper
  case Unicode character in FirstString.

  If Length > 0 and FirstString is NULL, then ASSERT().
  If Length > 0 and FirstString is not aligned on a 16-bit boundary, then
  ASSERT().
  If Length > 0 and SecondString is NULL, then ASSERT().
  If Length > 0 and SecondString is not aligned on a 16-bit boundary, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Length is greater than
  PcdMaximumUnicodeStringLength, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  FirstString   A pointer to a Null-terminated Unicode string.
  @param  SecondString  A pointer to a Null-terminated Unicode string.
  @param  Length        The maximum number of Unicode characters to compare.

  @retval ==0    FirstString is identical to SecondString using case
                 insensitive comparisons.
  @retval others FirstString is not identical to SecondString using case
                 insensitive comparisons.

**/
INTN
EFIAPI
StrniCmp (
  IN CONST CHAR16  *FirstString,
  IN CONST CHAR16  *SecondString,
  IN UINTN         Length
  )
{
  CHAR16  UpperFirstString;
  CHAR16  UpperSecondString;

  if (Length == 0) {
    return 0;
  }

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (FirstString) != 0);
  ASSERT (StrSize (SecondString) != 0);

  if (PcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
    ASSERT (Length <= PcdGet32 (PcdMaximumUnicodeStringLength));
  }

  UpperFirstString  = InternalCharToUpper (*FirstString);
  UpperSecondString = InternalCharToUpper (*SecondString);
  while ((*FirstString != L'\0') &&
         (*SecondString != L'\0') &&
         (UpperFirstString == UpperSecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    UpperFirstString  = InternalCharToUpper (*FirstString);
    UpperSecondString = InternalCharToUpper (*SecondString);
    Length--;
  }

  return UpperFirstString - UpperSecondString;
}

VOID
UnicodeUefiSlashes (
  IN OUT CHAR16  *String
  )
{
  CHAR16  *Needle;

  while ((Needle = StrStr (String, L"/")) != NULL) {
    *Needle = L'\\';
  }
}

VOID
UnicodeFilterString (
  IN OUT CHAR16   *String,
  IN     BOOLEAN  SingleLine
  )
{
  while (*String != L'\0') {
    if ((*String & 0x7FU) != *String) {
      //
      // Remove all unicode characters.
      //
      *String = L'_';
    } else if (SingleLine && (*String == L'\r' || *String == L'\n')) {
      //
      // Stop after printing one line.
      //
      *String = L'\0';
      break;
    } else if (*String < 0x20 || *String == 0x7F) {
      //
      // Drop all unprintable spaces but space including tabs.
      //
      *String = L'_';
    }

    ++String;
  }
}

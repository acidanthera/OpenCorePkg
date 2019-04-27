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

#ifndef OC_STRING_LIB_H_
#define OC_STRING_LIB_H_

/**
  Returns the length of a Null-terminated string literal.

  @param[in] String  The Null-terminated string literal.

**/
#define L_STR_LEN(String) (ARRAY_SIZE (String) - 1)

/**
  Returns the size of a Null-terminated string literal in bytes, including the
  Null terminator.

  @param[in] String  The Null-terminated string literal.

**/
#define L_STR_SIZE(String) (sizeof (String))

/**
  Returns the size of a Null-terminated string literal in bytes, excluding the
  Null terminator.

  @param[in] String  The Null-terminated string literal.

**/
#define L_STR_SIZE_NT(String) (sizeof (String) - sizeof (*(String)))

/** Check if character is printable

  @param[in] Char  The ascii character to check if is printable.

  @retval  TRUE, if character is printable.
**/
BOOLEAN
IsAsciiPrint (
  IN CHAR8  Char
  );

/** Check if character is a white space character

  @param[in] Char  The ascii character to check if is white space.

  @retval  TRUE, if character is a white space character
**/
INTN
IsAsciiSpace (
  IN CHAR8  Char
  );

/** Convert null terminated ascii string to unicode.

  @param[in]  String1  A pointer to the ascii string to convert to unicode.
  @param[in]  Length   Length or 0 to calculate the length of the ascii string to convert.

  @retval  A pointer to the converted unicode string allocated from pool.
**/
CHAR16 *
AsciiStrCopyToUnicode (
  IN  CONST CHAR8   *String,
  IN  UINTN         Length
  );

/**
  Convert 64-bit unsigned integer to a nul-termianted hex string.

  @param[out]  Buffer      Destination buffer.
  @param[in]   BufferSize  Destination buffer size in bytes.
  @param[in]   Value       Value to convert.
**/
BOOLEAN
AsciiUint64ToLowerHex (
  OUT CHAR8   *Buffer,
  IN  UINT32  BufferSize,
  IN  UINT64  Value
  );

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
  );

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
  );

/**
  Convert path with mixed slashes to UEFI slashes (\\).

  @param[in,out]  String      Path.
**/
VOID
UnicodeUefiSlashes (
  IN OUT CHAR16  *String
  );

/**
  Filter string from unprintable characters.

  @param[in,out]  String      String to filter.
  @param[in]      SingleLine  Enforce only one line.
**/
VOID
UnicodeFilterString (
  IN OUT CHAR16   *String,
  IN     BOOLEAN  SingleLine
  );

#endif // OC_STRING_LIB_H_

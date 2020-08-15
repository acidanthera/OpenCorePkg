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

#ifndef OC_STRING_LIB_H
#define OC_STRING_LIB_H

#include <Uefi.h>

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

  @param[in]  String  A pointer to the ascii string to convert to unicode.
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

  @retval TRUE on fit
**/
BOOLEAN
AsciiUint64ToLowerHex (
  OUT CHAR8   *Buffer,
  IN  UINT32  BufferSize,
  IN  UINT64  Value
  );

/**
  Alternative to AsciiSPrint, which checks that the buffer can contain all the characters.

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated ASCII format string.
  @param  ...             Variable argument list whose contents are accessed based on the
                          format string specified by FormatString.

  @retval EFI_SUCCESS           When data was printed to supplied buffer.
  @retval EFI_OUT_OF_RESOURCES  When supplied buffer cannot contain all the characters.
**/
EFI_STATUS
EFIAPI
OcAsciiSafeSPrint (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  ...
  );

/** Check if ASCII string ends with another ASCII string.

  @param[in]  String        A pointer to a Null-terminated ASCII string.
  @param[in]  SearchString  A pointer to a Null-terminated ASCII string
                            to compare against String.

  @retval  TRUE if String ends with SearchString.
**/
BOOLEAN
EFIAPI
OcAsciiEndsWith (
  IN CONST CHAR8      *String,
  IN CONST CHAR8      *SearchString
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
OcStriCmp (
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
OcStrniCmp (
  IN CONST CHAR16  *FirstString,
  IN CONST CHAR16  *SecondString,
  IN UINTN         Length
  );

/**
  Returns the first occurrence of a Null-terminated Unicode sub-string
  in a Null-terminated Unicode string through a case insensitive comparison.

  This function scans the contents of the Null-terminated Unicode string
  specified by String and returns the first occurrence of SearchString.
  If SearchString is not found in String, then NULL is returned.  If
  the length of SearchString is zero, then String is returned.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If SearchString is NULL, then ASSERT().
  If SearchString is not aligned on a 16-bit boundary, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and SearchString
  or String contains more than PcdMaximumUnicodeStringLength Unicode
  characters, not including the Null-terminator, then ASSERT().

  @param  String          The pointer to a Null-terminated Unicode string.
  @param  SearchString    The pointer to a Null-terminated Unicode string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @return others          If there is a match.

**/
CHAR16 *
EFIAPI
OcStriStr (
  IN      CONST CHAR16              *String,
  IN      CONST CHAR16              *SearchString
  );

/**
  Search substring in string.

  @param[in]  String               Search string.
  @param[in]  StringLength         Search string length.
  @param[in]  SearchString         String to search.
  @param[in]  SearchStringLength   String to search length.

  @retval NULL    If the SearchString does not appear in String.
  @retval others  If there is a match.
**/
CONST CHAR16 *
OcStrStrLength (
  IN CONST CHAR16  *String,
  IN UINTN         StringLength,
  IN CONST CHAR16  *SearchString,
  IN UINTN         SearchStringLength
  );

/**
  Alternative to UnicodeSPrint, which checks that the buffer can contain all the characters.

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated Unicode format string.
  @param  ...             Variable argument list whose contents are accessed based on the
                          format string specified by FormatString.

  @retval EFI_SUCCESS           When data was printed to supplied buffer.
  @retval EFI_OUT_OF_RESOURCES  When supplied buffer cannot contain all the characters.
**/
EFI_STATUS
EFIAPI
OcUnicodeSafeSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  ...
  );

/** Check if Unicode string ends with another Unicode string.

  @param[in]  String        A pointer to a Null-terminated Unicode string.
  @param[in]  SearchString  A pointer to a Null-terminated Unicode string
                            to compare against String.

  @retval  TRUE if String ends with SearchString.
**/
BOOLEAN
EFIAPI
OcUnicodeEndsWith (
  IN CONST CHAR16     *String,
  IN CONST CHAR16     *SearchString
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

#endif // OC_STRING_LIB_H

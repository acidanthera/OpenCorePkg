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

// IsAsciiDecimalDigitCharacter
/** Check if character is a decimal digit

  @param[in] Char  The ascii character to check if is printable.

  @retval  TRUE, if character is a decimal digit.
**/
BOOLEAN
IsAsciiDecimalDigitCharacter (
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

// AsciiHexCharToUintn
/** Convert ascii hexadecimal character to unsigned integer.

  @param[in] Char  The ascii character to convert to integer.

  @retval  Integer value of character representation.
**/
UINTN
AsciiHexCharToUintn (
  IN CHAR8  Char
  );

/**

  @param[in] String1  A pointer to a buffer containing the first ascii string to compare.
  @param[in] String2  A pointer to a buffer containing the second ascii string to compare.
  @param[in] Length   The number of characters to compare.

  @retval  Integer value of character representation.
**/
INTN
AsciiStrnIntegerCmp (
  IN CHAR8  *String1,
  IN CHAR8  *String2,
  IN UINTN  Length
  );

/** Convert ascii string to unsigned integer.

  @param[in] Char  The null terminated ascii string to convert to integer.

  @retval  Integer value of the ascii string.
**/
INTN
AsciiStrToInteger (
  IN CHAR8  *Start
  );

/** Convert ascii character to upper case

  @param[in] Char  The ascii character to convert to upperase.

  @retval  Upper case representation of the ascii character.
**/
CHAR8
AsciiToUpperChar (
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

/** Remove leading and trailing spaces in the string

  @param[in] Start  A pointer to the ascii string

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
AsciiTrimWhiteSpace (
  IN  CHAR8   *String
  );

/** Return the filename element of a pathname

  @param[in] FullPath  A pointer to a ascii path

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
AsciiBaseName (
  IN CHAR8  *FullPath
  );

/** Return the folder element of a pathname

  @param[in] FullPath  A pointer to a ascii path

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
AsciiDirName (
  IN CHAR8  *FullPath
  );

/** Check if character is printable

  @param[in] Char  The unicode character to check if is printable.

  @retval  TRUE, if character is printable.
**/
BOOLEAN
IsPrint (
  IN CHAR16  Char
  );

/** Check if character is a decimal digit

  @param[in] Char  The unicode character to check if is printable.

  @retval  TRUE, if character is a decimal digit.
**/
BOOLEAN
IsDecimalDigitCharacter (
  IN CHAR16  Char
  );

/** Check if character is a white space character

  @param[in] Char  The unicode character to check if is white space.

  @retval  TRUE, if character is a white space character
**/
INTN
IsSpace (
    IN CHAR16  Char
  );

/** Convert unicode hexadecimal character to unsigned integer.

  @param[in] Char  The unicode character to convert to integer.

  @retval  Integer value of character representation.
**/
UINTN
HexCharToUintn (
  IN CHAR16  Char
  );

/**

  @param[in] String1  A pointer to a buffer containing the first unicode string to compare.
  @param[in] String2  A pointer to a buffer containing the second unicode string to compare.
  @param[in] Length   The number of characters to compare.

  @retval  Integer value of character representation.
**/
INTN
StrnIntegerCmp (
  IN CHAR16  *String1,
  IN CHAR16  *String2,
  IN UINTN   Length
  );

/** Convert unicode string to unsigned integer.

  @param[in] Char  The null terminated unicode string to convert to integer.

  @retval  Integer value of the unicode string.
**/
INTN
StrToInteger (
  IN CHAR16  *Start
  );

/** Convert unicode character to upper case

  @param[in] Char  The unicode character to convert to upperase.

  @retval  Upper case representation of the unicode character.
**/
CHAR16
ToUpperChar (
  IN CHAR16  Char
  );

/** Convert null terminated unicode string to ascii.

  @param[in]  String1  A pointer to the unicode string to convert to ascii.
  @param[out] String2  A pointer to the converted ascii string.

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
OcStrToAscii (
  IN CHAR16  *String1,
  IN CHAR8   *String2
  );

/** Compare unicode string with ascii string ignoring case

  @param[in] String1  A pointer to a unicode string to compare.
  @param[in] String2  A pointer to a ascii string to compare.

  @retval  A pointer to the converted ascii string.
**/
UINTN
StrCmpiAscii (
  IN CHAR16  *String1,
  IN CHAR8   *String2
  );

/** Compare unicode strings ignoring case

  @param[in] String1  A pointer to a unicode string to compare.
  @param[in] String2  A pointer to a unicode string to compare.

  @retval  A pointer to the converted ascii string.
**/
UINTN
StrCmpiBasic (
  IN CHAR16  *String1,
  IN CHAR16  *String2
  );

/** Return the filename element of a pathname

  @param[in] FullPath  A pointer to a unicode path

  @retval  A pointer to the converted ascii string.
**/
CHAR16 *
UnicodeBaseName (
  IN CHAR16  *FullPath
  );

/** Return the folder element of a pathname

  @param[in] FullPath  A pointer to a unicode path

  @retval  A pointer to the converted ascii string.
**/
CHAR16 *
UnicodeDirName (
  IN CHAR16  *FullPath
  );

/**

  @param[in] String    A pointer to a unicode string
  @param[in] Variable  A pointer to a unicode string variable name to parse for

  @retval  A pointer to the variable value
**/
CHAR16 *
UnicodeParseString (
  IN  CHAR16    *String,
  IN  CHAR16    *Variable
  );

/** Remove leading and trailing spaces in the string

  @param[in] Start  A pointer to the unicode string

  @retval  A pointer to the converted unicode string.
**/
CHAR16 *
TrimWhiteSpace (
  IN  CHAR16   *String
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
  Convert path with mixed slashes to UEFI slashes (\\).

  @param[in,out]  String      Path.
**/
VOID
UnicodeUefiSlashes (
  IN OUT CHAR16  *String
  );

#endif // OC_STRING_LIB_H_

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
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
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

// IsAsciiDecimalDigitCharacter
/** Check if character is a decimal digit

  @param[in] Char  The ascii character to check if is printable.

  @retval  TRUE, if character is a decimal digit.
**/
BOOLEAN
IsAsciiDecimalDigitCharacter (
  IN  CHAR8 Char
  )
{
  return ((Char >= '0') && (Char <= '9'));
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

// AsciiHexCharToUintn
/** Convert ascii hexadecimal character to unsigned integer.

  @param[in] Char  The ascii character to convert to integer.

  @retval  Integer value of character representation.
**/
UINTN
AsciiHexCharToUintn (
  IN CHAR8  Char
  )
{
  UINTN Value;

  if (IsAsciiDecimalDigitCharacter (Char)) {
    Value = (UINTN)(Char - L'0');
  } else {
    Value = (UINTN)(10 + AsciiToUpperChar (Char) - L'A');
  }

  return Value;
}

// AsciiStrnIntegerCmp
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
  )
{
  INTN  Result;

  UINTN Index;

  Result = 0;

  for (Index = 0; Index < Length; ++Index) {
    if (String1[Index] != String2[Index]) {
      Result = (String1[Index] - String2[Index]);

      break;
    }

    if (String1[Index] == '\0') {
      break;
    }
  }

  return Result;
}

// AsciiStrToInteger
/** Convert ascii string to unsigned integer.

  @param[in] Char  The null terminated ascii string to convert to integer.

  @retval  Integer value of the ascii string.
**/
INTN
AsciiStrToInteger (
  IN CHAR8  *Start
  )
{
  UINTN   Base;
  INTN    Sum;

  BOOLEAN Negative;

  Sum = 0;

  if (Start != NULL) {
    while (IsAsciiSpace (*Start) && (*Start != '\0')) {
      ++Start;
    }

    Base     = 10;
    Negative = FALSE;

    if (*Start == '-') {
      Negative = TRUE;
      ++Start;
    } else if (*Start == '0') {
      ++Start;

      if ((*Start == 'x') || (*Start == 'X')) {
        Base = 16;
        ++Start;
      }
    } else if (*Start == '#') {
      Base = 16;
      ++Start;
    }

    if (Base == 10) {
      while ((*Start >= '0') && (*Start <= '9')) {
        Sum *= 10;
        Sum += (*Start - '0');

        ++Start;
      }

      if (Negative) {
        Sum = -Sum;
      }
    } else if (Base == 16) {
      Sum = (INTN)AsciiStrHexToUint64 (Start);
    }
  }

  return Sum;
}

// AsciiToUpperChar
/** Convert ascii character to upper case

  @param[in] Char  The ascii character to convert to upperase.

  @retval  Upper case representation of the ascii character.
**/
CHAR8
AsciiToUpperChar (
  IN CHAR8  Char
  )
{
  if ((Char >= 'a') && (Char <= 'z')) {
    Char -= ('a' - 'A');
  }

  return Char;
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

// UnicodeBaseName
/** Return the filename element of a pathname

  @param[in] FullPath  A pointer to a ascii path

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
AsciiBaseName (
  IN CHAR8  *FullPath
  )
{
  CHAR8 *BaseName;

  BaseName = NULL;

  if (FullPath != NULL) {
    BaseName = ((FullPath + AsciiStrLen (FullPath)) - 1);

    while ((BaseName > FullPath) && (BaseName[-1] != '\\') && (BaseName[-1] != '/')) {
      --BaseName;
    }
  }

  return BaseName;
}

// AsciiDirName
/** Return the folder element of a pathname

  @param[in] FullPath  A pointer to a ascii path

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
AsciiDirName (
  IN CHAR8  *FullPath
)
{
  CHAR8 *DirName;

  if (FullPath != NULL) {
    DirName = FullPath + AsciiStrLen (FullPath) - 1;

    if ((*(DirName) != '\\') && (*(DirName) != '/')) {
      while ((DirName > FullPath) && (DirName[-1] != '\\') && (DirName[-1] != '/')) {
        --DirName;
      }

      DirName[-1] = '\0';
    }
  }

  return FullPath;
}

// AsciiTrimWhiteSpace
/** Remove any leading or trailing space in the string

  @param[in] Start  A pointer to the ascii string

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
AsciiTrimWhiteSpace (
  IN  CHAR8   *String
  )
{
  CHAR8 *End;

  if (String != NULL) {

    // Trim leading white space
    while (IsAsciiSpace (*String)) {
      String++;
    }

    // Trim trailing white space
    End = String + AsciiStrLen (String);

    while (End != String && IsAsciiSpace (*--End));

    // Write new null terminator
    *(++End) = '\0';
  }

  return String;
}


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
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcStringLib.h>

// IsPrint
/** Check if character is printable

  @param[in] Char  The unicode character to check if is printable.

  @retval  TRUE, if character is printable.
**/
BOOLEAN
IsPrint (
  IN CHAR16  Char
  )
{
  return ((Char >= L' ') && (Char < L'~'));
}

// IsDecimalDigitCharacter
/** Check if character is a decimal digit

  @param[in] Char  The unicode character to check if is printable.

  @retval  TRUE, if character is a decimal digit.
**/
BOOLEAN
IsDecimalDigitCharacter (
  IN CHAR16  Char
  )
{
  return ((Char >= L'0') && (Char <= L'9'));
}

// IsSpace
/** Check if character is a white space character

  @param[in] Char  The unicode character to check if is white space.

  @retval  TRUE, if character is a white space character
**/
INTN
IsSpace (
  IN CHAR16  Char
  )
{
  return ((Char == L' ')
       || (Char == L'\t')
       || (Char == L'\v')
       || (Char == L'\f')
       || (Char == L'\r'));
}

// HexCharToUintn
/** Convert unicode hexadecimal character to unsigned integer.

  @param[in] Char  The unicode character to convert to integer.

  @retval  Integer value of character representation.
**/
UINTN
HexCharToUintn (
  IN CHAR16  Char
  )
{
  UINTN Result;

  if (IsDecimalDigitCharacter (Char)) {
    Result = (Char - L'0');
  } else {
    Result = (UINTN)((10 + ToUpperChar (Char)) - L'A');
  }

  return Result;
}

// StrnIntegerCmp
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

    if (String1[Index] == L'\0') {
      break;
    }
  }

  return Result;
}

// StrToInteger
/** Convert unicode string to unsigned integer.

  @param[in] Char  The null terminated unicode string to convert to integer.

  @retval  Integer value of the unicode string.
**/
INTN
StrToInteger (
  IN CHAR16  *Start
  )
{
  UINTN   Base;
  INTN    Sum;

  BOOLEAN Negative;

  Sum = 0;

  if (Start != NULL) {
    while (IsSpace ((CHAR8)*Start) && (*Start != L'\0')) {
      ++Start;
    }

    Base     = 10;
    Negative = FALSE;

    if (*Start == '-') {
      Negative = TRUE;
      ++Start;
    } else if (*Start == L'0') {
      ++Start;

      if ((*Start == L'x') || (*Start == L'X')) {
        Base = 16;
        ++Start;
      }
    } else if (*Start == L'#') {
      Base = 16;
      ++Start;
    }

    if (Base == 10) {
      while ((*Start >= L'0') && (*Start <= L'9')) {
        Sum *= 10;
        Sum += (*Start - L'0');

        ++Start;
      }

      if (Negative) {
        Sum = -Sum;
      }
    } else if (Base == 16) {
      Sum = (INTN)StrHexToUint64 (Start);
    }
  }

  return Sum;
}

// ToUpperChar
/** Convert unicode character to upper case

  @param[in] Char  The unicode character to convert to upperase.

  @retval  Upper case representation of the unicode character.
**/
CHAR16
ToUpperChar (
  IN CHAR16  Char
  )
{
  if ((Char <= MAX_UINT8) && ((Char >= 'a') && (Char <= 'z'))) {
    Char -= ('a' - 'A');
  }

  return Char;
}

// OcStrToAscii
/** Convert null terminated unicode string to ascii.

  @param[in] String1   A pointer to the unicode string to convert to ascii.
  @param[out] String2  A pointer to the converted ascii string.

  @retval  A pointer to the converted ascii string.
**/
CHAR8 *
OcStrToAscii (
  IN CHAR16  *String1,
  IN CHAR8   *String2
  )
{
  CHAR8 *Start;

  Start = NULL;

  if ((String1 != NULL) && (String2 != NULL)) {
    Start = String2;

    while (*String1 != L'\0') {
      *String2 = (CHAR8)*String1;

      ++String1;
      ++String2;
    }

    *String2 = '\0';
  }

  return Start;
}

// StrCmpiAscii
/** Compare unicode string with ascii string ignoring case

  @param[in] String1  A pointer to a unicode string to compare.
  @param[in] String2  A pointer to a ascii string to compare.

  @retval  A pointer to the converted ascii string.
**/
UINTN
StrCmpiAscii (
  IN CHAR16  *String1,
  IN CHAR8   *String2
  )
{
  UINTN  Result;

  CHAR16 Chr1;
  CHAR16 Chr2;

  if ((String1 == NULL) || (String2 == NULL)) {
    Result = 1;
  } else if ((*String1 == L'\0') && (*String2 == '\0')) {
    Result = 0;
  } else if ((*String1 == L'\0') || (*String2 == '\0')) {
    Result = 1;
  } else {
    do {
      Chr1 = ToUpperChar (*String1);
      Chr2 = ToUpperChar ((CHAR16)*String2);

      ++String1;
      ++String2;
    } while ((String1[-1] != L'\0') && (Chr1 == Chr2));

    Result = (Chr1 - Chr2);
  }

  return Result;
}

// StrCmpiBasic
/** Compare unicode strings ignoring case

  @param[in] String1  A pointer to a unicode string to compare.
  @param[in] String2  A pointer to a unicode string to compare.

  @retval  A pointer to the converted ascii string.
**/
UINTN
StrCmpiBasic (
  IN CHAR16  *String1,
  IN CHAR16  *String2
  )
{
  UINTN  Result;

  CHAR16 Chr1;
  CHAR16 Chr2;

  if ((String1 == NULL) || (String2 == NULL)) {
    Result = 1;
  } else if ((*String1 == L'\0') && (*String2 == L'\0')) {
    Result = 0;
  } else if ((*String1 == L'\0') || (*String2 == L'\0')) {
    Result = 1;
  } else {
    do {
      Chr1 = ToUpperChar (*String1);
      Chr2 = ToUpperChar (*String2);

      ++String1;
      ++String2;
    } while ((String1[-1] != L'\0') && (Chr1 == Chr2));

    Result = (Chr1 - Chr2);
  }

  return Result;
}

// UnicodeBaseName
/** Return the filename element of a pathname

  @param[in] FullPath  A pointer to a unicode path

  @retval  A pointer to the converted ascii string.
**/
CHAR16 *
UnicodeBaseName (
  IN CHAR16  *FullPath
  )
{
  CHAR16 *BaseName;

  BaseName = NULL;

  if ((FullPath != NULL) && (*FullPath != L'\0')) {
    BaseName = ((FullPath + StrLen (FullPath)) - 1);

    while ((BaseName > FullPath) && (BaseName[-1] != L'\\') && (BaseName[-1] != L'/')) {
      --BaseName;
    }
  }

  return BaseName;
}

// UnicodeDirName
/** Return the folder element of a pathname

  @param[in] FullPath  A pointer to a unicode path

  @retval  A pointer to the converted ascii string.
**/
CHAR16 *
UnicodeDirName (
  IN CHAR16  *FullPath
)
{
  CHAR16 *DirName;

  if ((FullPath != NULL) && (*FullPath != L'\0')) {
    DirName = FullPath + StrLen (FullPath) - 1;

    if ((*(DirName) != L'\\') && (*(DirName) != L'/')) {
      while ((DirName > FullPath) && (DirName[-1] != L'\\') && (DirName[-1] != L'/')) {
        --DirName;
      }

      DirName[-1] = L'\0';
    }
  }

  return FullPath;
}

// UnicodeParseString
/**

  @param[in] String    A pointer to a unicode string
  @param[in] Variable  A pointer to a unicode string variable name to parse for

  @retval  A pointer to the variable value
**/
CHAR16 *
UnicodeParseString (
  IN  CHAR16    *String,
  IN  CHAR16    *Variable
  )
{
  CHAR16  *Start;
  CHAR16  *Data;
  CHAR16  *Value;

  if (String == NULL || Variable == NULL) {
    return NULL;
  }

  // Check string contains variable
  Start = StrStr (String, Variable);

  if (Start == NULL) {
    return NULL;
  }

  // locate Variable= in string
  while (*Start != L'\0') {
    if (*Start == L'=') {
      break;
    }
    Start++;
  }

  // Extra sanity check
  if (*Start++ != L'=') {
    return NULL;
  }

  Data = AllocateZeroPool (StrSize (Start));

  if (Data == NULL) {
    return NULL;
  }

  Value = Data;

  // copy filtering any " from data
  while (*Start != L'\0') {
    if (*Start != L'"') {
      *(Data++) = *Start;
    }
    Start++;
  }

  return Value;
}

// TrimWhiteSpace
/** Remove any leading or trailing space in the string

  @param[in] Start  A pointer to the unicode string

  @retval  A pointer to the converted unicode string.
**/
CHAR16 *
TrimWhiteSpace (
  IN  CHAR16   *String
  )
{
  CHAR16 *End;

  if ((String != NULL) && (*String != L'\0')) {

    // Trim leading white space
    while (IsSpace (*String)) {
      String++;
    }

    // Trim trailing white space
    End = String + StrLen (String);

    while (End != String && IsSpace (*--End));

    // Write new null terminator
    *(++End) = L'\0';
  }

  return String;
}

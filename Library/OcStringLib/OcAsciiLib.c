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

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Macros.h>

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
       || (Char == '\r'));
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

// OcAsciiStrToGuid
/** Convert correctly formatted string into a GUID.

  @param[in] StringGuid  A pointer to a buffer containing the ascii string.
  @param[in] Guid        A pointer to location to store the converted GUID.

  @retval EFI_SUCCESS  The conversion completed successfully.
**/
EFI_STATUS
OcAsciiStrToGuid (
  IN     CONST CHAR8  *StringGuid,
  IN OUT EFI_GUID     *Guid
  )
{
  EFI_STATUS Status;

  Status = EFI_INVALID_PARAMETER;

  if ((StringGuid != NULL) && (Guid != NULL)) {
    ZeroMem (Guid, sizeof (*Guid));

    if ((StringGuid[8] == '-')
     && (StringGuid[13] == '-')
     && (StringGuid[18] == '-')
     && (StringGuid[23] == '-')) {
      Guid->Data1 = (UINT32)AsciiStrHexToUint64 (StringGuid);
      Guid->Data2 = (UINT16)AsciiStrHexToUint64 (StringGuid + 9);
      Guid->Data3 = (UINT16)AsciiStrHexToUint64 (StringGuid + 14);

      Guid->Data4[0] = (UINT8)(AsciiHexCharToUintn (StringGuid[19]) * 16 + AsciiHexCharToUintn (StringGuid[20]));
      Guid->Data4[1] = (UINT8)(AsciiHexCharToUintn (StringGuid[21]) * 16 + AsciiHexCharToUintn (StringGuid[22]));
      Guid->Data4[2] = (UINT8)(AsciiHexCharToUintn (StringGuid[24]) * 16 + AsciiHexCharToUintn (StringGuid[25]));
      Guid->Data4[3] = (UINT8)(AsciiHexCharToUintn (StringGuid[26]) * 16 + AsciiHexCharToUintn (StringGuid[27]));
      Guid->Data4[4] = (UINT8)(AsciiHexCharToUintn (StringGuid[28]) * 16 + AsciiHexCharToUintn (StringGuid[29]));
      Guid->Data4[5] = (UINT8)(AsciiHexCharToUintn (StringGuid[30]) * 16 + AsciiHexCharToUintn (StringGuid[31]));
      Guid->Data4[6] = (UINT8)(AsciiHexCharToUintn (StringGuid[32]) * 16 + AsciiHexCharToUintn (StringGuid[33]));
      Guid->Data4[7] = (UINT8)(AsciiHexCharToUintn (StringGuid[34]) * 16 + AsciiHexCharToUintn (StringGuid[35]));

      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

// OcAsciiStrToUnicode
/** Convert null terminated ascii string to unicode.

  @param[in]  String1  A pointer to the ascii string to convert to unicode.
  @param[out] String2  A pointer to the converted unicode string.
  @param[in]  Length   Length or 0 to calculate the length of the ascii string to convert.

  @retval  A pointer to the converted unicode string.
**/
CHAR16 *
OcAsciiStrToUnicode (
  IN  CHAR8   *String1,
  IN  CHAR16  *String2,
  IN  UINTN   Length
  )
{
  CHAR16 *Start;

  Start = NULL;

  if ((String1 != NULL) && (String2 != NULL)) {

    if (Length == 0) {
      Length = AsciiStrLen (String1);
    }

    Start = String2;

    while (*String1 != '\0' && Length--) {
      *(String2++) = *(String1++);
    }

    *String2 = L'\0';
  }

  return Start;
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

// OcAsciiStrnCmp
/**
  Compares two Null-terminated ASCII strings with maximum lengths, and returns
  the difference between the first mismatched ASCII characters.

  @param[in]  FirstString   A pointer to a Null-terminated ASCII string.
  @param[in]  SecondString  A pointer to a Null-terminated ASCII string.
  @param[in]  Length        The maximum number of ASCII characters for compare.

  @retval ==0               FirstString is identical to SecondString.
  @retval !=0               FirstString is not identical to SecondString.

**/
INTN
OcAsciiStrnCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString,
  IN      UINTN                     Length
  )
{
  if (Length == 0) {
    return 0;
  }

  while ((*FirstString != '\0') &&
         (*SecondString != '\0') &&
         (*FirstString == *SecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    Length--;
  }
  return *FirstString - *SecondString;
}

// OcAsciiStrStr
/**
  Returns the first occurrence of a Null-terminated ASCII sub-string
  in a Null-terminated ASCII string.

  @param[in]  String1       A pointer to a Null-terminated ASCII string.
  @param[in]  SearchString  A pointer to a Null-terminated ASCII string to search for.

  @retval NULL              If the SearchString does not appear in String.

**/
CHAR8 *
OcAsciiStrStr (
  IN      CONST CHAR8               *String,
  IN      CONST CHAR8               *SearchString
  )
{
  CONST CHAR8 *FirstMatch;
  CONST CHAR8 *SearchStringTmp;

  if (*SearchString == '\0') {
    return (CHAR8 *) String;
  }

  while (*String != '\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;
    
    while ((*String == *SearchStringTmp) 
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

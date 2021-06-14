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
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>

INTN
EFIAPI
OcStriCmp (
  IN CONST CHAR16  *FirstString,
  IN CONST CHAR16  *SecondString
  )
{
  CHAR16  UpperFirstString;
  CHAR16  UpperSecondString;

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength
  //
  ASSERT (StrSize (FirstString) != 0);
  ASSERT (StrSize (SecondString) != 0);

  UpperFirstString  = CharToUpper (*FirstString);
  UpperSecondString = CharToUpper (*SecondString);
  while ((*FirstString != '\0') && (*SecondString != '\0') && (UpperFirstString == UpperSecondString)) {
    FirstString++;
    SecondString++;
    UpperFirstString  = CharToUpper (*FirstString);
    UpperSecondString = CharToUpper (*SecondString);
  }

  return UpperFirstString - UpperSecondString;
}

INTN
EFIAPI
OcStrniCmp (
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
  
  UpperFirstString  = CharToUpper (*FirstString);
  UpperSecondString = CharToUpper (*SecondString);
  while ((*FirstString != L'\0') &&
         (*SecondString != L'\0') &&
         (UpperFirstString == UpperSecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    UpperFirstString  = CharToUpper (*FirstString);
    UpperSecondString = CharToUpper (*SecondString);
    Length--;
  }

  return UpperFirstString - UpperSecondString;
}

CHAR16 *
EFIAPI
OcStriStr (
  IN      CONST CHAR16              *String,
  IN      CONST CHAR16              *SearchString
  )
{
  CONST CHAR16 *FirstMatch;
  CONST CHAR16 *SearchStringTmp;

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (String) != 0);
  ASSERT (StrSize (SearchString) != 0);

  if (*SearchString == L'\0') {
    return (CHAR16 *) String;
  }

  while (*String != L'\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;

    while ((CharToUpper (*String) == CharToUpper (*SearchStringTmp))
            && (*String != L'\0')) {
      String++;
      SearchStringTmp++;
    }

    if (*SearchStringTmp == L'\0') {
      return (CHAR16 *) FirstMatch;
    }

    if (*String == L'\0') {
      return NULL;
    }

    String = FirstMatch + 1;
  }

  return NULL;
}

CONST CHAR16 *
OcStrStrLength (
  CONST CHAR16  *String,
  UINTN         StringLength,
  CONST CHAR16  *SearchString,
  UINTN         SearchStringLength
  )
{
  UINTN         Index;
  UINTN         Index2;
  UINTN         Index3;
  INTN          CmpResult;
  CONST CHAR16  *Y;
  CONST CHAR16  *X;

  //
  // REF: http://www-igm.univ-mlv.fr/~lecroq/string/node13.html#SECTION00130
  //

  if (SearchStringLength > StringLength
    || SearchStringLength == 0
    || StringLength == 0) {
    return NULL;
  }

  if (SearchStringLength > 1) {
    Index = 0;

    Y = (CONST CHAR16 *) String;
    X = (CONST CHAR16 *) SearchString;

    if (X[0] == X[1]) {
      Index2 = 2;
      Index3 = 1;
    } else {
      Index2 = 1;
      Index3 = 2;
    }

    while (Index <= StringLength - SearchStringLength) {
      if (X[1] != Y[Index+1]) {
        Index += Index2;
      } else {
        CmpResult = CompareMem (X+2, Y+Index+2, (SearchStringLength - 2) * sizeof (*SearchString));
        if (CmpResult == 0 && X[0] == Y[Index]) {
          return &Y[Index];
        }

        Index += Index3;
      }
    }
  } else {
    return ScanMem16 (String, StringLength * sizeof (*SearchString), *SearchString);
  }

  return NULL;
}

CHAR16 *
EFIAPI
OcStrChr (
  IN      CONST CHAR16              *String,
  IN            CHAR16              Char
  )
{
  ASSERT (StrSize (String) != 0);

  while (*String != '\0') {
    //
    // Return immediately when matching first occurrence of Char.
    //
    if (*String == Char) {
      return (CHAR16 *) String;
    }

    ++String;
  }

  return NULL;
}

CHAR16 *
EFIAPI
OcStrrChr (
  IN      CONST CHAR16              *String,
  IN            CHAR16              Char
  )
{
  CHAR16 *Save;

  ASSERT (StrSize (String) != 0);

  Save = NULL;

  while (*String != '\0') {
    //
    // Record the last occurrence of Char.
    //
    if (*String == Char) {
      Save = (CHAR16 *) String;
    }

    ++String;
  }

  return Save;
}

VOID
UnicodeUefiSlashes (
  IN OUT CHAR16  *String
  )
{
  CHAR16  *Needle;

  Needle = String;
  while ((Needle = StrStr (Needle, L"/")) != NULL) {
    *Needle++ = L'\\';
  }
}

BOOLEAN
UnicodeGetParentDirectory (
  IN OUT CHAR16  *String
  )
{
  UINTN  Length;

  Length = StrLen (String);

  //
  // Drop starting slash (as it cannot be passed to some drivers).
  //
  if (String[0] == '\\' || String[0] == '/') {
    CopyMem (&String[0], &String[1], Length * sizeof (String[0]));
    --Length;
  }

  //
  // Empty paths have no root directory.
  //
  if (Length == 0) {
    return FALSE;
  }

  //
  // Drop trailing slash when getting a directory.
  //
  if (String[Length - 1] == '\\' || String[Length - 1] == '/') {
    --Length;
    //
    // Paths with just one slash have no root directory (e.g. \\/).
    //
    if (Length == 0) {
      return FALSE;
    }
  }

  //
  // Find slash in the path.
  //
  while (String[Length - 1] != '\\' && String[Length - 1] != '/') {
    --Length;

    //
    // Paths containing just a filename get normalised.
    //
    if (Length == 0) {
      *String = '\0';
      return TRUE;
    }
  }

  //
  // Path containing some other directory get its path.
  //
  String[Length - 1] = '\0';
  return TRUE;
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

BOOLEAN
UnicodeIsFilteredString (
  IN CONST CHAR16   *String,
  IN       BOOLEAN  SingleLine
  )
{
  while (*String != L'\0') {
    if ((*String & 0x7FU) != *String) {
      return FALSE;
    }

    if (SingleLine && (*String == L'\r' || *String == L'\n')) {
      return FALSE;
    }

    if (*String < 0x20 || *String == 0x7F) {
      return FALSE;
    }

    ++String;
  }

  return TRUE;
}

EFI_STATUS
EFIAPI
OcUnicodeSafeSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
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
  NumberOfPrinted = SPrintLength (FormatString, Marker2);
  VA_END (Marker2);

  if (BufferSize - 1 >= NumberOfPrinted) {
    UnicodeVSPrint (StartOfBuffer, BufferSize, FormatString, Marker);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_OUT_OF_RESOURCES;
  }

  VA_END (Marker);

  return Status;
}

BOOLEAN
EFIAPI
OcUnicodeEndsWith (
  IN CONST CHAR16     *String,
  IN CONST CHAR16     *SearchString,
  IN BOOLEAN          CaseInsensitiveMatch
  )
{
  UINTN   StringLength;
  UINTN   SearchStringLength;

  ASSERT (String != NULL);
  ASSERT (SearchString != NULL);

  StringLength        = StrLen (String);
  SearchStringLength  = StrLen (SearchString);

  if (CaseInsensitiveMatch) {
    return StringLength >= SearchStringLength
      && OcStrniCmp (&String[StringLength - SearchStringLength], SearchString, SearchStringLength) == 0;
  }
  return StringLength >= SearchStringLength
    && StrnCmp (&String[StringLength - SearchStringLength], SearchString, SearchStringLength) == 0;
}

BOOLEAN
EFIAPI
OcUnicodeStartsWith (
  IN CONST CHAR16     *String,
  IN CONST CHAR16     *SearchString,
  IN BOOLEAN          CaseInsensitiveMatch
  )
{
  CHAR16  First;
  CHAR16  Second;

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
      First  = CharToUpper (First);
      Second = CharToUpper (Second);
    }
    if (First != Second) {
      return FALSE;
    }
  }
}

BOOLEAN
HasValidGuidStringPrefix (
  IN CONST CHAR16  *String
  )
{
  UINTN  Length;
  UINTN  Index;
  UINTN  GuidLength = GUID_STRING_LENGTH;

  Length = StrLen (String);
  if (Length < GuidLength) {
    return FALSE;
  }

  for (Index = 0; Index < GuidLength; ++Index) {
    if (Index == 8 || Index == 13 || Index == 18 || Index == 23) {
      if (String[Index] != '-') {
        return FALSE;
      }
    } else if (!(String[Index] >= L'0' && String[Index] <= L'9')
      && !(String[Index] >= L'A' && String[Index] <= L'F')
      && !(String[Index] >= L'a' && String[Index] <= L'f')) {
      return FALSE;
    }
  }

  return TRUE;
}

INTN
MixedStrCmp (
  IN CONST CHAR16  *FirstString,
  IN CONST CHAR8   *SecondString
  )
{
  while (*FirstString != '\0' && *FirstString == *SecondString) {
    ++FirstString;
    ++SecondString;
  }

  return *FirstString - *SecondString;
}

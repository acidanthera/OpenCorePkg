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

  if (PcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
    ASSERT (Length <= PcdGet32 (PcdMaximumUnicodeStringLength));
  }

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
  IN CONST CHAR16     *SearchString
  )
{
  UINTN   StringLength;
  UINTN   SearchStringLength;

  ASSERT (String != NULL);
  ASSERT (SearchString != NULL);

  StringLength        = StrLen (String);
  SearchStringLength  = StrLen (SearchString);

  return StringLength >= SearchStringLength
    && StrnCmp (&String[StringLength - SearchStringLength], SearchString, SearchStringLength) == 0;
}

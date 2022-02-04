/** @file
  Linux version compare.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LinuxBootInternal.h"

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define IS_SECTION_BREAK(Ch) ((Ch) == '-' || (Ch) == '~' || (Ch) == '\0')

// TODO: (?) Make this one OC library function, and
// confirm nothing similar already in OC or EDK-II.
STATIC
INTN
BoundedAsciiStrCmp (
  CONST CHAR8               *FirstString,
  CONST CHAR8               *FirstStringEnd,
  CONST CHAR8               *SecondString,
  CONST CHAR8               *SecondStringEnd
  )
{
  ASSERT (FirstString != NULL);
  ASSERT (SecondString != NULL);
  ASSERT (FirstStringEnd >= FirstString);
  ASSERT (SecondStringEnd >= SecondString);

  while ((FirstString != FirstStringEnd) &&
    (SecondString != SecondStringEnd) &&
    (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }
  if (FirstString == FirstStringEnd) {
    if (SecondString == SecondStringEnd) {
      return 0;
    } else {
      return - *SecondString;
    }
  }
  if (SecondString == SecondStringEnd) {
    return *FirstString;
  }
  return *FirstString - *SecondString;
}

STATIC
VOID
GetNextFragment (
  IN     CONST CHAR8    **Pos,
     OUT CONST CHAR8    **FragmentStart,
     OUT CONST CHAR8    **FragmentEnd,
     OUT       BOOLEAN  *IsAlphaFragment,
     OUT       BOOLEAN  *IsSectionBreak,
     OUT       CHAR8    *SectionChar
  )
{
  CHAR8         Ch;

  ASSERT (Pos             != NULL);
  ASSERT (*Pos            != NULL);
  ASSERT (FragmentStart   != NULL);
  ASSERT (FragmentEnd     != NULL);
  ASSERT (IsAlphaFragment != NULL);
  ASSERT (IsSectionBreak  != NULL);
  ASSERT (SectionChar     != NULL);

  Ch = **Pos;
  if (IS_SECTION_BREAK (Ch)) {
    *IsSectionBreak = TRUE;
    *SectionChar = Ch;
    if (Ch != '\0') {
      ++(*Pos);
    }
    return;
  }

  *IsSectionBreak = FALSE;
  *FragmentStart = *Pos;

  if (IS_ALPHA (Ch) || IS_DIGIT (Ch)) {
    *IsAlphaFragment = IS_ALPHA (Ch);
    do {
      ++(*Pos);
      Ch = **Pos;
    }
    while (*IsAlphaFragment ? IS_ALPHA (Ch) : IS_DIGIT (Ch));
  } else {
    *IsAlphaFragment = TRUE;
  }

  *FragmentEnd = *Pos;
  if (!(IS_ALPHA (Ch) || IS_DIGIT (Ch) || IS_SECTION_BREAK (Ch))) {
    ++(*Pos);
  }

  return;
}

//
// Return +1 if Version1 is higher than Version2.
// Return -1 if Version2 is higher than Version1.
// Return 0 if they sort equally.
// Non-identical strings may sort as the same (e.g. v000 and v0).
//
// Useful info on how GRUB sorts versions at https://askubuntu.com/a/1277440/693497 .
//
// This is not a replication of that logic, but does something reasonable and
// behaviourally similar (hopefully the same, for reasonable version strings).
//
//  - Versions come in sections a.b.c.d... .
//  - If matched so far, having 'more' version fragments in the same section sorts higher.
//  - - & ~ are section breaks.
//  - Any non-alpha non-digit (not just '.') is a fragment separator.
//  - Changing from alpha to digit or vice-versa is also a fragment seprator.
//  - If matched up to a section break, having a ~ section next is lower than
//    anything else (means pre-release), and having a - section next is higher
//    (just means 'more' in the version).
//
STATIC
INTN
DoVersionCompare (
  IN CONST CHAR8          *Version1,
  IN CONST CHAR8          *Version2
 )
{
  EFI_STATUS    Status;
  CONST CHAR8   *Pos1;
  CONST CHAR8   *Pos2;
  CONST CHAR8   *FragmentStart1;
  CONST CHAR8   *FragmentEnd1;
  CONST CHAR8   *FragmentStart2;
  CONST CHAR8   *FragmentEnd2;
  UINTN         VersionFragment1;
  UINTN         VersionFragment2;
  BOOLEAN       IsAlphaFragment1;
  BOOLEAN       IsAlphaFragment2;
  BOOLEAN       IsSectionBreak1;
  BOOLEAN       IsSectionBreak2;
  CHAR8         SectionChar1;
  CHAR8         SectionChar2;
  BOOLEAN       IsRescue1;
  BOOLEAN       IsRescue2;
  INTN          Compare;
  CHAR8         ChSave;

  ASSERT (Version1 != NULL);
  ASSERT (Version2 != NULL);
  if (Version1 == NULL || Version2 == NULL) {
    return 0;
  }

  //
  // Match some of what GRUB script does, c.f. askubuntu link above.
  // We can't used EndsWith as we're not as clean at getting the version
  // out without the machine-id tacked on afterwards.
  //
  IsRescue1 = AsciiStrStr (Version1, "rescue") != NULL;
  IsRescue2 = AsciiStrStr (Version2, "rescue") != NULL;

  if (IsRescue1 != IsRescue2) {
    return IsRescue2 ? +1 : -1;
  }

  Pos1 = Version1;
  Pos2 = Version2;

  while (TRUE) {
    GetNextFragment (&Pos1, &FragmentStart1, &FragmentEnd1, &IsAlphaFragment1, &IsSectionBreak1, &SectionChar1);
    GetNextFragment (&Pos2, &FragmentStart2, &FragmentEnd2, &IsAlphaFragment2, &IsSectionBreak2, &SectionChar2);

    if (IsSectionBreak1 != IsSectionBreak2) {
      return IsSectionBreak2 ? +1 : -1;
    } else if (IsSectionBreak1) {
      if (SectionChar1 != SectionChar2) {
        if (SectionChar1 == '-' || SectionChar2 == '~') {
          return +1;
        }
        if (SectionChar1 == '~' || SectionChar2 == '-') {
          return -1;
        }
        ASSERT (FALSE);
        return 0;
      }

      if (SectionChar1 == '\0') {
        return 0;
      }
    } else {
      if (IsAlphaFragment1 == IsAlphaFragment2 && !IsAlphaFragment1) {
        ChSave = *FragmentEnd1;
        *((CHAR8 *) FragmentEnd1) = '\0';
        Status = AsciiStrDecimalToUintnS (FragmentStart1, NULL, &VersionFragment1);
        *((CHAR8 *) FragmentEnd1) = ChSave;
        ASSERT (!EFI_ERROR(Status));

        ChSave = *FragmentEnd2;
        *((CHAR8 *) FragmentEnd2) = '\0';
        Status = AsciiStrDecimalToUintnS (FragmentStart2, NULL, &VersionFragment2);
        *((CHAR8 *) FragmentEnd2) = ChSave;
        ASSERT (!EFI_ERROR(Status));

        Compare = VersionFragment1 - VersionFragment2;
      } else {
        Compare = BoundedAsciiStrCmp (FragmentStart1, FragmentEnd1, FragmentStart2, FragmentEnd2);
      }
      if (Compare != 0) {
        return Compare;
      }
    }
  }
}

INTN
EFIAPI
InternalVersionCompare (
  IN CONST VOID           *Version1,
  IN CONST VOID           *Version2
 )
{
  return DoVersionCompare (*((CONST CHAR8 **) Version1), *((CONST CHAR8 **) Version2));
}

INTN
EFIAPI
InternalReverseVersionCompare (
  IN CONST VOID           *Version1,
  IN CONST VOID           *Version2
 )
{
  return -DoVersionCompare (*((CONST CHAR8 **) Version1), *((CONST CHAR8 **) Version2));
}

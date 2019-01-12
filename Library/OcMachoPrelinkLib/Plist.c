/** @file
  PLIST helper functions.
  
Copyright (c) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcStringLib.h>

#include "OcMachoPrelinkInternal.h"

CONST CHAR8 *
InternalPlistStrStrSameLevelUp (
  IN CONST CHAR8  *AnchorString,
  IN CONST CHAR8  *FindString,
  IN UINTN        FindStringLen
  )
{
  CONST CHAR8 *Walker;
  UINTN       DictLevel;
  UINTN       ShortestMatch;

  ASSERT (AnchorString != NULL);
  ASSERT (FindString != NULL);
  ASSERT (AsciiStrLen (FindString) == FindStringLen);

  ShortestMatch = MIN (FindStringLen, L_STR_LEN ("<dict>"));

  for (
    Walker = (AnchorString - ShortestMatch);
    AsciiStrnCmp (Walker, FindString, FindStringLen) != 0;
    --Walker
    ) {
    for (DictLevel = 1; TRUE; --Walker) {
      if (AsciiStrnCmp (Walker, STR_N_HELPER ("</dict>")) == 0) {
        ++DictLevel;
        Walker -= (ShortestMatch - 1);
        continue;
      }

      if (AsciiStrnCmp (Walker, STR_N_HELPER ("<dict>")) == 0) {
        --DictLevel;
        if (DictLevel == 0) {
          return NULL;
        }
      }

      if (DictLevel == 1) {
        break;
      }
    }
  }

  return Walker;
}

CONST CHAR8 *
InternalPlistStrStrSameLevelDown (
  IN CONST CHAR8  *AnchorString,
  IN CONST CHAR8  *FindString,
  IN UINTN        FindStringLen
  )
{
  CONST CHAR8 *Walker;
  UINTN       DictLevel;

  ASSERT (AnchorString != NULL);
  ASSERT (FindString != NULL);
  ASSERT (AsciiStrLen (FindString) == FindStringLen);

  for (
    Walker = AnchorString;
    AsciiStrnCmp (Walker, FindString, FindStringLen) != 0;
    ++Walker
    ) {
    for (DictLevel = 1; TRUE; ++Walker) {
      if (AsciiStrnCmp (Walker, STR_N_HELPER ("<dict>")) == 0) {
        ++DictLevel;
        Walker += (L_STR_LEN ("<dict>") - 1);
        continue;
      }

      if (AsciiStrnCmp (Walker, STR_N_HELPER ("</dict>")) == 0) {
        --DictLevel;
        if (DictLevel == 0) {
          return NULL;
        }
      }

      if (DictLevel == 1) {
        break;
      }
    }
  }

  return Walker;
}

CONST CHAR8 *
InternalPlistStrStrSameLevel (
  IN CONST CHAR8  *AnchorString,
  IN CONST CHAR8  *FindString,
  IN UINTN        FindStringLen,
  IN UINTN        DownwardsOffset
  )
{
  CONST CHAR8 *Result;

  Result = InternalPlistStrStrSameLevelUp (
            AnchorString,
            FindString,
            FindStringLen
            );
  if (Result == NULL) {
    Result = InternalPlistStrStrSameLevelDown (
               (AnchorString + DownwardsOffset),
               FindString,
               FindStringLen
               );
  }

  return Result;
}

UINT64
InternalPlistGetIntValue (
  IN CONST CHAR8  **Tag
  )
{
  UINT64      Value;

  CONST CHAR8 *Walker;
  CHAR8       *Walker2;

  ASSERT (Tag != NULL);
  ASSERT (*Tag != NULL);
  ASSERT (AsciiStrnCmp (*Tag, STR_N_HELPER ("<integer")) == 0);

  Walker = (*Tag + L_STR_LEN ("<integer"));
  //
  // Skip "size" and any other attributes.
  //
  Walker = AsciiStrStr (Walker, ">");
  ASSERT (Walker != NULL);
  ++Walker;
  //
  // Temporarily terminate after the integer definition.
  //
  Walker2 = AsciiStrStr (Walker, "<");
  ASSERT (Walker2 != NULL);
  *Walker2 = '\0';
  //
  // Assert that the prelinked PLIST always uses hexadecimal integers.
  //
  while ((*Walker == ' ') || (*Walker == '\t')) {
    ++Walker;
  }
  ASSERT ((Walker[1] == 'x') || (Walker[1] == 'X'));
  Value = AsciiStrHexToUint64 (Walker);
  //
  // Restore the previously replaced opening brace.
  //
  *Walker2 = '<';
  //
  // Return the first character after the integer tag.
  //
  *Tag = (Walker2 + L_STR_LEN ("</integer>"));

  return Value;
}

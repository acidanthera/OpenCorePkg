/** @file
  Flexible string tokens.

  Copyright (c) 2022, Mike Beaton, PMheart. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFlexArrayLib.h>

OC_FLEX_ARRAY *
OcStringSplit (
  IN OUT  VOID           *String,
  IN      CONST CHAR16   Delim,
  IN      CONST BOOLEAN  IsUnicode
  )
{
  OC_FLEX_ARRAY  *Tokens;
  VOID           **Pointer;
  VOID           *NextToken;
  CHAR16         Ch;

  ASSERT (String != NULL);

  if (IsUnicode ? ((CHAR16 *) String)[0] == CHAR_NULL : ((CHAR8 *) String)[0] == '\0') {
    return NULL;
  }

  Tokens = OcFlexArrayInit (sizeof (String), OcFlexArrayFreePointerItem);
  if (Tokens == NULL) {
    return NULL;
  }

  NextToken = NULL;
  do {
    Ch = IsUnicode ? *((CHAR16 *) String) : *((CHAR8 *) String);

    if (Ch == Delim || OcIsSpace (Ch) || Ch == CHAR_NULL) {
      if (NextToken != NULL) {
        if (IsUnicode) {
          *((CHAR16 *) String) = CHAR_NULL;
        } else {
          *((CHAR8 *) String) = '\0';
        }

        Pointer = OcFlexArrayAddItem (Tokens);
        if (Pointer == NULL) {
          OcFlexArrayFree (&Tokens);
          return NULL;
        }

        *Pointer = NextToken;
        NextToken = NULL;
      }
    } else {
      if (NextToken == NULL) {
        NextToken = String;
      }
    }

    String = (UINT8 *) String + (IsUnicode ? sizeof (CHAR16) : sizeof (CHAR8));
  } while (Ch != CHAR_NULL);

  return Tokens;
}

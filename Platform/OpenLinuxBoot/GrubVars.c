/** @file
  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LinuxBootInternal.h"

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcStringLib.h>

#include <Protocol/OcBootEntry.h>

STATIC OC_FLEX_ARRAY *mGrubVars = NULL;

EFI_STATUS
InternalInitGrubVars (
  VOID
  )
{
  mGrubVars = OcFlexArrayInit (sizeof (GRUB_VAR), NULL);
  if (mGrubVars == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}

VOID
InternalFreeGrubVars (
  VOID
  )
{
  if (mGrubVars != NULL) {
    OcFlexArrayFree (&mGrubVars);
  }
}

EFI_STATUS
InternalSetGrubVar (
  CHAR8 *Key,
  CHAR8 *Value,
  UINTN Errors
  )
{
  GRUB_VAR    *Var;
  UINTN       Index;
  UINTN       WereErrors;

  ASSERT (mGrubVars != NULL);
  ASSERT (Key[0] != '\0');

  for (Index = 0; Index < mGrubVars->Count; ++Index) {
    Var = OcFlexArrayItemAt (mGrubVars, Index);
    if (AsciiStrCmp (Var->Key, Key) == 0) {
      break;
    }
  }

  if (Index < mGrubVars->Count) {
    Var->Value = Value;
    WereErrors = Var->Errors;

    //
    // Probably not worth the two lines of code (because unlikely to
    // occur), but: allow later non-indented no-vars value to overwrite
    // earlier non-indented has-vars value and thereby become usable.
    //
    if ((Errors & VAR_ERR_INDENTED) == 0) {
      Var->Errors &= ~VAR_ERR_HAS_VARS;
    }

    //
    // Indentation err stays set because, even if grub.cfg code layout is
    // reasonable as we are assuming, we are not parsing enough to tell
    // which order (or none) indented vars are set in.
    //
    Var->Errors |= Errors;

    DEBUG ((OC_TRACE_GRUB_VARS,
      "LNX: Repeated %a=%a (0x%x->0x%x)\n",
      Key,
      Value,
      WereErrors,
      Var->Errors
      ));
  } else {
    Var = OcFlexArrayAddItem (mGrubVars);
    if (Var == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Var->Key    = Key;
    Var->Value  = Value;
    Var->Errors = Errors;

    DEBUG ((OC_TRACE_GRUB_VARS,
      "LNX: Added %a=%a (0x%x)\n",
      Key,
      Value,
      Errors
      ));
  }

  return EFI_SUCCESS;
}

//
// Compatible with Grub2 blscfg's simple definition of what gets replaced,
// cf InternalExpandGrubVars. (If there was more complex logic, it would
// probably make most sense just not to have this pre-check.)
//
BOOLEAN
InternalHasGrubVars (
  CHAR8 *Value
  )
{
  return OcAsciiStrChr (Value, '$') != NULL;
}

GRUB_VAR *
InternalGetGrubVar (
  IN     CONST CHAR8 *Key
  )
{
  UINTN     Index;
  GRUB_VAR  *Var;

  for (Index = 0; Index < mGrubVars->Count; Index++) {
    Var = OcFlexArrayItemAt (mGrubVars, Index);
    if (AsciiStrCmp (Var->Key, Key) == 0) {
      return Var;
    }
  }

  return NULL;
}

EFI_STATUS
InternalExpandGrubVarsForArray (
  IN OUT       OC_FLEX_ARRAY *Options
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  CHAR8       **Value;
  CHAR8       *Result;

  for (Index = 0; Index < Options->Count; Index++) {
    Value = OcFlexArrayItemAt (Options, Index);
    if (InternalHasGrubVars (*Value)) {
      Status = InternalExpandGrubVars (*Value, &Result);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      FreePool (*Value);
      *Value = Result;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalExpandGrubVars (
  IN     CONST CHAR8 *Value,
  IN OUT       CHAR8 **Result
  )
{
  EFI_STATUS          Status;
  UINTN               Pos;
  UINTN               LastPos;
  BOOLEAN             InVar;
  BOOLEAN             Retake;
  GRUB_VAR            *Var;
  CHAR8               Ch;
  UINTN               VarLength;
  OC_STRING_BUFFER    *StringBuffer;

  ASSERT (Value  != NULL);
  ASSERT (Result != NULL);

  *Result = NULL;

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringBuffer = OcAsciiStringBufferInit ();
  if (StringBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Pos     = 0;
  LastPos = 0;
  InVar   = FALSE;
  Status  = EFI_SUCCESS;
  Retake  = FALSE;

  //
  // These simple checks for what counts as a var to replace (including
  // the fact that there is no escape for '$') match Grub2 blscfg module.
  //
  do {
    Ch = Value[Pos];
    if (!InVar) {
      if (Ch == '$' || Ch == '\0') {
        Status = OcAsciiStringBufferAppendN (StringBuffer, &Value[LastPos], Pos - LastPos);

        InVar = TRUE;
        LastPos = Pos + 1;
      }
    } else if (!(Ch == '_' || IS_DIGIT (Ch) || IS_ALPHA (Ch))) {
      ((CHAR8 *)Value)[Pos] = '\0';
      Var = InternalGetGrubVar (&Value[LastPos]);
      if (Var == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: Missing required grub var $%a\n", &Value[LastPos]));
        Status = EFI_INVALID_PARAMETER;
      } else if (Var->Errors != 0) {
        DEBUG ((DEBUG_WARN, "LNX: Unusable grub var $%a - 0x%x\n", &Value[LastPos], Var->Errors));
        Status = EFI_INVALID_PARAMETER;
      }
      ((CHAR8 *)Value)[Pos] = Ch;

      //
      // Blscfg always appends a space to each expanded token (and the var values
      // are often set up to end with a space, too), then later GRUB tokenization
      // of the options as grub booter options (which we do not currently do - may
      // be needed in some escaped cases?) cleans it up. Here, we try to combine
      // obvious doubled up spaces right away.
      //
      if (!EFI_ERROR (Status)) {
        if (Var->Value != NULL) {
          VarLength = AsciiStrLen (Var->Value);
          if (VarLength > 0 && Var->Value[VarLength - 1] == ' ') {
            --VarLength;
          }
        }
        Status = OcAsciiStringBufferAppendN (StringBuffer, Var->Value, VarLength);
      }

      if (!EFI_ERROR (Status) && !(Ch == ' ' || Ch == '\0')) {
        Status = OcAsciiStringBufferAppend (StringBuffer, " ");
      }
        
      InVar = FALSE;
      Retake = TRUE;
      LastPos = Pos;
    }

    if (Retake) {
      Retake = FALSE;
    } else {
      ++Pos;
    }
  }
  while (Ch != '\0' && !EFI_ERROR (Status));

  *Result = OcAsciiStringBufferFreeContainer (&StringBuffer);

  if (EFI_ERROR (Status)) {
    if (*Result != NULL) {
      FreePool (*Result);
      *Result = NULL;
    }
  }

  DEBUG ((OC_TRACE_GRUB_VARS, "LNX: Expanding '%a' => '%a' - %r\n", Value, *Result, Status));

  return Status;
}

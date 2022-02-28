/** @file
  GRUB environment block parser.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/BaseLib.h>
#include <Library/OcDebugLogLib.h>
#include "LinuxBootInternal.h"

/*
  grubenv processing states.
*/
typedef enum GRUBENV_STATE_ {
  GRUBENV_NEXT_LINE,
  GRUBENV_KEY,
  GRUBENV_VAR,
  GRUBENV_COMMENT
} GRUBENV_STATE;

EFI_STATUS
InternalProcessGrubEnv (
  IN OUT       CHAR8              *Content,
  IN     CONST UINTN              Length
  )
{
  EFI_STATUS      Status;
  UINTN           Pos;
  UINTN           KeyStart;
  UINTN           VarStart;
  GRUBENV_STATE   State;

  State = GRUBENV_NEXT_LINE;

  //
  // In a valid grubenv block the last comment, if present, is not
  // \n terminated, but all var lines must be.
  //
  for (Pos = 0; Pos < Length && Content[Pos] != '\0'; Pos++) {
    switch (State) {
      case GRUBENV_NEXT_LINE:
        if (Content[Pos] == '#') {
          State = GRUBENV_COMMENT;
        } else {
          KeyStart = Pos;
          State = GRUBENV_KEY;
        }
        break;
        
      case GRUBENV_COMMENT:
        if (Content[Pos] == '\n') {
          State = GRUBENV_NEXT_LINE;
        }
        break;
        
      case GRUBENV_KEY:
        if (Content[Pos] == '=') {
          Content[Pos] = '\0';
          VarStart = Pos + 1;
          State = GRUBENV_VAR;
        }
        break;
        
      case GRUBENV_VAR:
        if (Content[Pos] == '\n') {
          Content[Pos] = '\0';
          Status = InternalSetGrubVar (&Content[KeyStart], &Content[VarStart], VAR_ERR_NONE);
          if (EFI_ERROR (Status)) {
            return Status;
          }
          State = GRUBENV_NEXT_LINE;
        }
        break;

      default:
        ASSERT (FALSE);
        break;
    }
  }

  ASSERT (State == GRUBENV_COMMENT || State == GRUBENV_NEXT_LINE);

  return EFI_SUCCESS;
}

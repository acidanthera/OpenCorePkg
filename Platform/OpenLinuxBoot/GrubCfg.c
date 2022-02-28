/** @file
  Naive GRUB config parser.

  Attemps to respect GRUB escape and line continuation syntax, and
  then to extract GRUB set commands for some basic processing.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LinuxBootInternal.h"

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>

/*
  grub.cfg processing states.
*/
typedef enum GRUB_PARSE_STATE_ {
  GRUB_LEADING_SPACE,
  GRUB_COMMENT,
  GRUB_TOKEN,
  GRUB_SINGLE_QUOTE,
  GRUB_DOUBLE_QUOTE
} GRUB_PARSE_STATE;

/*
  grub.cfg $var processing states.
*/
typedef enum GRUB_VAR_STATE_ {
  GRUB_VAR_NONE,
  GRUB_VAR_START,
  GRUB_VAR_END,
  GRUB_VAR_CHAR
} GRUB_VAR_STATE;

#define GRUB_LINE "in grub.cfg at line"

/*
  grub.cfg $var processing flags.
*/
#define VAR_FLAGS_NONE      (0)
#define VAR_FLAGS_BRACE     BIT0
#define VAR_FLAGS_NUMERIC   BIT1

#define SHIFT_TOKEN(offset) do { \
  CopyMem (*Token + (offset), *Token, &Content[*Pos] - *Token); \
  *Token += (offset); \
} while (0)

STATIC
EFI_STATUS
GrubNextToken (
  CHAR8     *Content,
  UINTN     *Pos,
  UINTN     *Line,
  CHAR8     **Token,
  BOOLEAN   *IsIndented,
  BOOLEAN   *ContainsVars
  )
{
  GRUB_PARSE_STATE    GrubState;
  GRUB_VAR_STATE      VarState;
  UINTN               GrubVarFlags;

  BOOLEAN             Escaped;
  BOOLEAN             TokenCompleted;
  BOOLEAN             Retake;

  CHAR8               Ch;
  CHAR8               Ch2;

  UINTN               Add;

  *Token          = NULL;
  *IsIndented     = FALSE;
  *ContainsVars   = FALSE;

  GrubState       = GRUB_LEADING_SPACE;
  VarState        = GRUB_VAR_NONE;

  Escaped         = FALSE;
  TokenCompleted  = FALSE;
  Retake          = FALSE;

  do {
    Ch = Content[*Pos];

    if (Ch == '\n') {
      *Line += 1;
    } else if (!(Ch == '\0' || Ch == '\t' || (Ch >= 32 && Ch <= 127))) {
      DEBUG ((DEBUG_WARN, "LNX: Invalid char 0x%x %a %u\n", Ch, GRUB_LINE, Line));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Deal with escape char and line continuation.
    //
    if (Ch == '\\') {
      Ch2 = Content[(*Pos) + 1];

      if (Ch2 == '\0' || Ch2 == '\n') {
        //
        // Line continuation.
        //
        if (VarState != GRUB_VAR_NONE) {
          //
          // We could handle this fine (just remove this check), but GRUB doesn't:
          // https://www.gnu.org/software/grub/manual/grub/html_node/grub_fot.html#FOOT7
          //
          DEBUG ((DEBUG_WARN, "LNX: Illegal line continuation within variable name %a %u\n", GRUB_LINE, Line));
          return EFI_INVALID_PARAMETER;
        }

        //
        // '\n' go to char afterwards, '\0' go to '\0'.
        //
        Add = (Ch2 == '\n' ? 2 : 1);
        SHIFT_TOKEN (Add);
        *Pos += Add;
        Ch = Content[*Pos];
        *Line += 1;
      } else {
        //
        // Escapes.
        //
        switch (GrubState) {
          //
          // No escapes in single quote.
          //
          case GRUB_SINGLE_QUOTE:
            break;

          //
          // Only these escapes in double quote.
          //
          case GRUB_DOUBLE_QUOTE:
            if (Ch2 == '$' || Ch2 == '"') {
              Escaped = TRUE;
            }
            break;

          //
          // Anything can be escaped.
          //
          default:
            Escaped = TRUE;
            break;
        }

        if (Escaped) {
          SHIFT_TOKEN (1);
          ++(*Pos);
          Ch = Ch2;
          Escaped = FALSE;
        }
      }
    }

    //
    // Grub var is a special state which can be entered within other states.
    // Allowed: $?, $@, $#, $nnn, $alphanumeric
    //
    if (VarState != GRUB_VAR_NONE) {
      ASSERT (GrubState == GRUB_TOKEN || GrubState == GRUB_SINGLE_QUOTE || GrubState == GRUB_DOUBLE_QUOTE);

      switch (VarState) {
        case GRUB_VAR_START:
          //
          // The fact that a token contains a var reference means we cannot use it;
          // we are looking for tokens which define vars, not ones which use them.
          //
          *ContainsVars = TRUE;

          if (Ch == '{') {
            if ((GrubVarFlags & VAR_FLAGS_BRACE) != 0) {
              DEBUG ((DEBUG_WARN, "LNX: Illegal character in variable name %a %u\n", GRUB_LINE, Line));
              return EFI_INVALID_PARAMETER;
            }
            GrubVarFlags |= VAR_FLAGS_BRACE;
          } else if (Ch == '}') {
            //
            // Empty var name is valid.
            //
            if ((GrubVarFlags & VAR_FLAGS_BRACE) == 0) {
              DEBUG ((DEBUG_WARN, "LNX: Illegal character in variable name %a %u\n", GRUB_LINE, Line));
              return EFI_INVALID_PARAMETER;
            }
            VarState = GRUB_VAR_NONE;
          } else if (Ch == '@' || Ch == '?' || Ch == '#') {
            VarState = GRUB_VAR_END;
          } else if (IS_DIGIT (Ch)) {
            GrubVarFlags |= VAR_FLAGS_NUMERIC;
            VarState = GRUB_VAR_CHAR;
          } else if (Ch == '_' || IS_ALPHA (Ch)) {
            VarState = GRUB_VAR_CHAR;
          } else {
            DEBUG ((DEBUG_WARN, "LNX: Illegal character in variable name %a %u\n", GRUB_LINE, Line));
            return EFI_INVALID_PARAMETER;
          }

          break;
          
        case GRUB_VAR_END:
          if ((GrubVarFlags & VAR_FLAGS_BRACE) != 0) {
            if (Ch != '}') {
              DEBUG ((DEBUG_WARN, "LNX: Illegal character in variable name %a %u\n", GRUB_LINE, Line));
              return EFI_INVALID_PARAMETER;
            }
          } else {
            Retake = TRUE;
          }
          VarState = GRUB_VAR_NONE;
          break;
          
        case GRUB_VAR_CHAR:
          if (!(IS_DIGIT (Ch) ||
            ((GrubVarFlags & VAR_FLAGS_NUMERIC) == 0
              && (Ch == '_' || IS_ALPHA (Ch))))) {
            VarState = GRUB_VAR_END;
            Retake = TRUE;
          }
          break;

        default:
          ASSERT (FALSE);
          break;
      }
    } else {
      switch (GrubState) {
        case GRUB_LEADING_SPACE:
          if (Ch == '\0' || Ch == '\n' || (!Escaped && Ch == ';')) {
            TokenCompleted = TRUE;
            Retake = TRUE;
          } else if (Ch == ' ' || Ch == '\t') {
            *IsIndented = TRUE;
          } else if (Ch == '#') {
            GrubState = GRUB_COMMENT;
          } else {
            *Token = &Content[*Pos];
            GrubState = GRUB_TOKEN;
            Retake = TRUE;
          }
          break;

        case GRUB_COMMENT:
          if (Ch == '\n' || Ch == '\0') {
            TokenCompleted = TRUE;
            Retake = TRUE;
          }
          break;

        case GRUB_TOKEN:
          if (Ch == '\n' || Ch == '\0' || (!Escaped && (Ch == ';' || Ch == ' ' || Ch == '\t'))) {
            TokenCompleted = TRUE;
            Retake = TRUE;
          } else if (!Escaped && Ch == '\'') {
            SHIFT_TOKEN (1);
            GrubState = GRUB_SINGLE_QUOTE;
          } else if (!Escaped && Ch == '"') {
            SHIFT_TOKEN (1);
            GrubState = GRUB_DOUBLE_QUOTE;
          } else if (!Escaped && Ch == '$') {
            VarState      = GRUB_VAR_START;
            GrubVarFlags  = VAR_FLAGS_NONE;
          }
          break;

        case GRUB_SINGLE_QUOTE:
          if (Ch == '\'') {
            SHIFT_TOKEN (1);
            GrubState = GRUB_TOKEN;
          } else if (Ch == '$') {
            VarState      = GRUB_VAR_START;
            GrubVarFlags  = VAR_FLAGS_NONE;
          }
          break;

        case GRUB_DOUBLE_QUOTE:
          if (!Escaped && Ch == '"') {
            SHIFT_TOKEN (1);
            GrubState = GRUB_TOKEN;
          } else if (!Escaped && Ch == '$') {
            VarState      = GRUB_VAR_START;
            GrubVarFlags  = VAR_FLAGS_NONE;
          }
          break;
      }
    }

    if (Retake) {
      Retake = FALSE;
    } else if (Ch != '\0') {
      ++(*Pos);
    }
  }
  while (Ch != '\0' && !TokenCompleted);

  if (!TokenCompleted && GrubState != GRUB_LEADING_SPACE) {
    DEBUG ((DEBUG_WARN, "LNX: Syntax error (state=%u) %a %u\n", GrubState, GRUB_LINE, Line));
    return EFI_INVALID_PARAMETER;
  }
  
  return EFI_SUCCESS;
}

STATIC
BOOLEAN
GrubNextLine (
  CHAR8     Ch,
  UINTN     *Pos
  )
{
  if (Ch == '\0') {
    return TRUE;
  }

  if (Ch == ';' || Ch == '\n') {
    (*Pos)++;
    return TRUE;
  }

  ASSERT (Ch == ' ' || Ch == '\t');

  (*Pos)++;
  return FALSE;
}

STATIC
EFI_STATUS
SetVar (
  UINTN   Line,
  CHAR8   *Token,
  BOOLEAN IsIndented,
  BOOLEAN ContainsVars
  )
{
  EFI_STATUS  Status;
  CHAR8       *Equals;
  CHAR8       *Dollar;
  UINTN       VarStatus;

  //
  // Note: It is correct grub2 parsing to treat these tokens with = in (whether after set or not) as one token.
  //
  Equals = OcAsciiStrChr (Token, '=');
  if (Equals == NULL) {
    DEBUG ((DEBUG_WARN, "LNX: Invalid set command %a %u\n", GRUB_LINE, Line));
    return EFI_INVALID_PARAMETER;
  }
  *Equals = '\0';

  Dollar = OcAsciiStrChr (Token, '$');
  if (Dollar != NULL) {
    //
    // Non-typical but valid GRUB syntax to use variable replacements within
    // variable name; we don't know what the name is (and are probably pretty
    // unlikely to find the required values in valid, non-indented variables
    // which we do know), so we ignore it.
    //
    DEBUG ((DEBUG_WARN, "LNX: Ignoring tokenised %a %a %a %u\n", "variable name", Token, GRUB_LINE, Line));
    return EFI_SUCCESS;
  }

  VarStatus = 0;
  if (IsIndented) {
    VarStatus |= VAR_ERR_INDENTED;
  }
  if (ContainsVars) {
    VarStatus |= VAR_ERR_HAS_VARS;
  }
  Status = InternalSetGrubVar (Token, Equals + 1, VarStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalProcessGrubCfg (
  IN OUT        CHAR8              *Content
  )
{
  EFI_STATUS  Status;
  UINTN       Pos;
  UINTN       Line;
  UINTN       NextLine;
  UINTN       TokenIndex;
  CHAR8       *Dollar;
  CHAR8       *Equals;
  CHAR8       *Token;
  CHAR8       LastChar;
  BOOLEAN     IsIndented;
  BOOLEAN     ContainsVars;
  BOOLEAN     SetCommand;
  BOOLEAN     SetIsIndented;

  Pos   = 0;
  Line  = 1;
  
  do {
    TokenIndex  = 0;
    SetCommand  = FALSE;

    do {
      //
      // Save new line number until we've finished any messages.
      //
      NextLine = Line;
      Status = GrubNextToken (Content, &Pos, &NextLine, &Token, &IsIndented, &ContainsVars);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Terminate token and remember terminator char.
      //
      LastChar = Content[Pos];
      if (Token != NULL) {
        Content[Pos] = '\0';

        if (TokenIndex == 0) {
          //
          // Warn on pretty obscure - though valid - syntax of building the command name from variables;
          // do not warn for direct setting of grub internal values with no set command, i.e. just name=value,
          // where the $ is only in the value.
          //
          Dollar = OcAsciiStrChr (Token, '$');
          if (Dollar != NULL) {
            Equals = OcAsciiStrChr (Token, '=');
            if (Equals == NULL || Dollar < Equals) {
              DEBUG ((DEBUG_WARN, "LNX: Ignoring tokenised %a %a %a %u\n", "command", Token, GRUB_LINE, Line));
            }
          } else {
            //
            // No non-indented variables after non-indented blscfg command can be used.
            //
            if (AsciiStrCmp("blscfg", Token) == 0) {
              return EFI_SUCCESS;
            }

            //
            // We could process grub unset command similarly to set, but we probably don't need it.
            //
            if (AsciiStrCmp("set", Token) == 0) {
              SetCommand = TRUE;
              SetIsIndented = IsIndented;
            }
          }
        } else if (TokenIndex == 1 && SetCommand) {
          Status = SetVar (Line, Token, SetIsIndented, ContainsVars);
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }
        ++TokenIndex;
      }
      Line = NextLine;
    } while (!GrubNextLine (LastChar, &Pos));
  } while (LastChar != '\0');

  //
  // Possibly allow through on flag?
  //
  DEBUG ((DEBUG_WARN, "LNX: blscfg command not found in grub.cfg\n"));
  return EFI_NOT_FOUND;
}

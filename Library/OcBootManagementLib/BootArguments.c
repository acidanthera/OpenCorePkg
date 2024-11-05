/** @file
  Copyright (C) 2019-2024, vit9696, mikebeaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Guid/AppleVariable.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "BootManagementInternal.h"

/*
  Shell var and load options processing states.
*/
typedef enum PARSE_VARS_STATE_ {
  PARSE_VARS_WHITE_SPACE,
  PARSE_VARS_COMMENT,
  PARSE_VARS_NAME,
  PARSE_VARS_VALUE,
  PARSE_VARS_QUOTED_VALUE,
  PARSE_VARS_SHELL_EXPANSION
} PARSE_VARS_STATE;

//
// Shift memory from token start to current position forwards by offset bytes
// and update token to point to shifted start (thereby discarding offset bytes
// from the token ending at current position).
//
#define SHIFT_TOKEN(pos, token, offset)  do {\
  CopyMem ((UINT8 *)(token) + (offset), (token), (UINT8 *)(pos) - (UINT8 *)(token)); \
  (token) = (UINT8 *)(token) + (offset); \
} while (0)

VOID
OcParseBootArgs (
  OUT OC_BOOT_ARGUMENTS  *Arguments,
  IN  VOID               *BootArgs
  )
{
  BootArgs1  *BA1 = (BootArgs1 *)BootArgs;
  BootArgs2  *BA2 = (BootArgs2 *)BootArgs;

  ZeroMem (Arguments, sizeof (*Arguments));

  if (BA1->Version == kBootArgsVersion1) {
    //
    // Pre Lion
    //
    Arguments->MemoryMap                  = &BA1->MemoryMap;
    Arguments->MemoryMapSize              = &BA1->MemoryMapSize;
    Arguments->MemoryMapDescriptorSize    = &BA1->MemoryMapDescriptorSize;
    Arguments->MemoryMapDescriptorVersion = &BA1->MemoryMapDescriptorVersion;

    Arguments->CommandLine = &BA1->CommandLine[0];

    Arguments->KernelAddrP       = &BA1->kaddr;
    Arguments->SystemTableP      = &BA1->efiSystemTable;
    Arguments->RuntimeServicesPG = &BA1->efiRuntimeServicesPageStart;
    Arguments->RuntimeServicesV  = &BA1->efiRuntimeServicesVirtualPageStart;
    Arguments->DeviceTreeP       = &BA1->deviceTreeP;
    Arguments->DeviceTreeLength  = &BA1->deviceTreeLength;
    Arguments->SystemTable       = (EFI_SYSTEM_TABLE *)(UINTN)BA1->efiSystemTable;
  } else {
    //
    // Lion and newer
    //
    Arguments->MemoryMap                  = &BA2->MemoryMap;
    Arguments->MemoryMapSize              = &BA2->MemoryMapSize;
    Arguments->MemoryMapDescriptorSize    = &BA2->MemoryMapDescriptorSize;
    Arguments->MemoryMapDescriptorVersion = &BA2->MemoryMapDescriptorVersion;

    Arguments->CommandLine = &BA2->CommandLine[0];

    Arguments->KernelAddrP       = &BA2->kaddr;
    Arguments->SystemTableP      = &BA2->efiSystemTable;
    Arguments->RuntimeServicesPG = &BA2->efiRuntimeServicesPageStart;
    Arguments->RuntimeServicesV  = &BA2->efiRuntimeServicesVirtualPageStart;
    Arguments->DeviceTreeP       = &BA2->deviceTreeP;
    Arguments->DeviceTreeLength  = &BA2->deviceTreeLength;
    Arguments->SystemTable       = (EFI_SYSTEM_TABLE *)(UINTN)BA2->efiSystemTable;

    if (BA2->flags & kBootArgsFlagCSRActiveConfig) {
      Arguments->CsrActiveConfig = &BA2->csrActiveConfig;
    }
  }
}

CONST CHAR8 *
OcGetArgumentFromCmd (
  IN  CONST CHAR8  *CommandLine,
  IN  CONST CHAR8  *Argument,
  IN  CONST UINTN  ArgumentLength,
  OUT UINTN        *ValueLength OPTIONAL
  )
{
  CHAR8  *Str;
  CHAR8  *StrEnd;

  Str = AsciiStrStr (CommandLine, Argument);

  //
  // Invalidate found boot arg if:
  // - it is neither the beginning of Cmd, nor has space prefix            -> boot arg is a suffix of another arg
  // - it has neither space suffix, nor \0 suffix, and does not end with = -> boot arg is a prefix of another arg
  //
  if (!Str || ((Str != CommandLine) && (*(Str - 1) != ' ')) ||
      ((Str[ArgumentLength] != ' ') && (Str[ArgumentLength] != '\0') &&
       (Str[ArgumentLength - 1] != '=')))
  {
    return NULL;
  }

  Str += ArgumentLength;

  if (ValueLength != NULL) {
    StrEnd = AsciiStrStr (Str, " ");
    if (StrEnd == NULL) {
      *ValueLength = AsciiStrLen (Str);
    } else {
      *ValueLength = StrEnd - Str;
    }
  }

  return Str;
}

VOID
OcRemoveArgumentFromCmd (
  IN OUT CHAR8        *CommandLine,
  IN     CONST CHAR8  *Argument
  )
{
  CHAR8  *Match;
  CHAR8  *Updated;
  UINTN  ArgumentLength;

  ArgumentLength = AsciiStrLen (Argument);
  Match          = CommandLine;

  do {
    Match = AsciiStrStr (Match, Argument);
    if (  (Match != NULL) && ((Match == CommandLine) || (*(Match - 1) == ' '))
       && (  (Match[ArgumentLength - 1] == '=')
          || (Match[ArgumentLength] == ' ')
          || (Match[ArgumentLength] == '\0')))
    {
      while (*Match != ' ' && *Match != '\0') {
        *Match++ = ' ';
      }
    } else if (Match != NULL) {
      ++Match;
    }
  } while (Match != NULL);

  //
  // Write zeroes to reduce data leak
  //
  Updated = CommandLine;

  while (CommandLine[0] == ' ') {
    CommandLine++;
  }

  while (CommandLine[0] != '\0') {
    while (CommandLine[0] == ' ' && CommandLine[1] == ' ') {
      CommandLine++;
    }

    *Updated++ = *CommandLine++;
  }

  ZeroMem (Updated, CommandLine - Updated);
}

BOOLEAN
OcAppendArgumentToCmd (
  IN OUT OC_PICKER_CONTEXT  *Context OPTIONAL,
  IN OUT CHAR8              *CommandLine,
  IN     CONST CHAR8        *Argument,
  IN     CONST UINTN        ArgumentLength
  )
{
  EFI_STATUS  Status;
  UINTN       Len = AsciiStrLen (CommandLine);

  if (Context != NULL) {
    Status = InternalRunRequestPrivilege (Context, OcPrivilegeAuthorized);
    if (EFI_ERROR (Status)) {
      if (Status != EFI_ABORTED) {
        ASSERT (FALSE);
        return FALSE;
      }

      return TRUE;
    }
  }

  //
  // Account for extra space.
  //
  if (Len + ((Len > 0) ? 1 : 0) + ArgumentLength >= BOOT_LINE_LENGTH) {
    DEBUG ((DEBUG_INFO, "OCB: boot-args are invalid, ignoring\n"));
    return FALSE;
  }

  if (Len > 0) {
    CommandLine   += Len;
    *CommandLine++ = ' ';
  }

  AsciiStrnCpyS (CommandLine, ArgumentLength + 1, Argument, ArgumentLength + 1);
  return TRUE;
}

BOOLEAN
OcAppendArgumentsToLoadedImage (
  IN OUT EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN     CONST CHAR8                **Arguments,
  IN     UINT32                     ArgumentCount,
  IN     BOOLEAN                    Replace
  )
{
  UINT32  Index;
  UINTN   ArgumentLength;
  UINTN   TotalLength;
  CHAR16  *NewArguments;
  CHAR16  *Walker;

  ASSERT (LoadedImage != NULL);
  ASSERT (Arguments != NULL);
  ASSERT (ArgumentCount > 0);

  TotalLength = 0;

  //
  // Count length including spaces between or '\0' for the last argument.
  //
  for (Index = 0; Index < ArgumentCount; ++Index) {
    ArgumentLength = AsciiStrSize (Arguments[Index]);
    if (BaseOverflowAddUN (TotalLength, ArgumentLength, &TotalLength)) {
      return FALSE;
    }
  }

  if (BaseOverflowMulUN (TotalLength, sizeof (CHAR16), &TotalLength)) {
    return FALSE;
  }

  Replace |= LoadedImage->LoadOptionsSize < sizeof (CHAR16);
  if (  !Replace
     && BaseOverflowTriAddUN (TotalLength, sizeof (CHAR16), LoadedImage->LoadOptionsSize, &TotalLength))
  {
    return FALSE;
  }

  NewArguments = AllocatePool (TotalLength);
  if (NewArguments == NULL) {
    return FALSE;
  }

  Walker = NewArguments;
  for (Index = 0; Index < ArgumentCount; ++Index) {
    if (Index != 0) {
      *Walker++ = ' ';
    }

    ArgumentLength = AsciiStrLen (Arguments[Index]);
    AsciiStrToUnicodeStrS (
      Arguments[Index],
      Walker,
      ArgumentLength + 1
      );
    Walker += ArgumentLength;
  }

  if (!Replace) {
    *Walker++ = ' ';
    CopyMem (Walker, LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize);
    Walker   += LoadedImage->LoadOptionsSize / sizeof (CHAR16);
    *Walker++ = '\0';
  }

  LoadedImage->LoadOptions     = NewArguments;
  LoadedImage->LoadOptionsSize = (UINT32)TotalLength;

  return TRUE;
}

BOOLEAN
OcCheckArgumentFromEnv (
  IN     EFI_LOADED_IMAGE  *LoadedImage OPTIONAL,
  IN     EFI_GET_VARIABLE  GetVariable OPTIONAL,
  IN     CONST CHAR8       *Argument,
  IN     CONST UINTN       ArgumentLength,
  IN OUT CHAR8             **Value OPTIONAL
  )
{
  CHAR16       *Options;
  UINTN        OptionsSize;
  CHAR8        BootArgsVar[BOOT_LINE_LENGTH];
  UINTN        BootArgsVarLen;
  EFI_STATUS   Status;
  UINTN        LastIndex;
  CHAR16       Last;
  BOOLEAN      HasArgument;
  CONST CHAR8  *ArgValue;
  UINTN        ArgValueLength;

  HasArgument = FALSE;

  if (LoadedImage != NULL) {
    Options     = (CHAR16 *)LoadedImage->LoadOptions;
    OptionsSize = LoadedImage->LoadOptionsSize / sizeof (CHAR16);

    if ((Options != NULL) && (OptionsSize > 0)) {
      //
      // Just in case we do not have 0-termination.
      // This may cut some data with unexpected options, but it is not like we care.
      //
      LastIndex          = OptionsSize - 1;
      Last               = Options[LastIndex];
      Options[LastIndex] = '\0';

      UnicodeStrToAsciiStrS (Options, BootArgsVar, BOOT_LINE_LENGTH);

      ArgValue = OcGetArgumentFromCmd (BootArgsVar, Argument, ArgumentLength, &ArgValueLength);
      if (ArgValue != NULL) {
        HasArgument = TRUE;
      }

      //
      // Options do not belong to us, restore the changed value.
      //
      Options[LastIndex] = Last;
    }
  }

  if (!HasArgument) {
    //
    // Important to avoid triggering boot-args wrapper too early if we have any.
    //
    BootArgsVarLen = sizeof (BootArgsVar);
    Status         = (GetVariable != NULL ? GetVariable : gRT->GetVariable)(
  L"boot-args",
  &gAppleBootVariableGuid,
  NULL,
  &BootArgsVarLen,
  BootArgsVar
  );

    if (!EFI_ERROR (Status) && (BootArgsVarLen > 0)) {
      //
      // Just in case we do not have 0-termination
      //
      BootArgsVar[BootArgsVarLen-1] = '\0';

      ArgValue = OcGetArgumentFromCmd (BootArgsVar, Argument, ArgumentLength, &ArgValueLength);
      if (ArgValue != NULL) {
        HasArgument = TRUE;
      }
    }
  }

  if (HasArgument && (Value != NULL)) {
    *Value = AllocateCopyPool (AsciiStrnSizeS (ArgValue, ArgValueLength), ArgValue);
    if (*Value == NULL) {
      return FALSE;
    }
  }

  return HasArgument;
}

BOOLEAN
EFIAPI
OcValidLoadOptions (
  IN        UINT32  LoadOptionsSize,
  IN CONST  VOID    *LoadOptions
  )
{
  return (
            (((LoadOptions) == NULL) == ((LoadOptionsSize) == 0))
         && ((LoadOptionsSize) % sizeof (CHAR16) == 0)
         && ((LoadOptionsSize) <= MAX_LOAD_OPTIONS_SIZE)
         && (
               ((LoadOptions) == NULL)
            || (((CHAR16 *)(LoadOptions))[((LoadOptionsSize) / 2) - 1] == CHAR_NULL)
               )
            );
}

BOOLEAN
EFIAPI
OcHasLoadOptions (
  IN        UINT32  LoadOptionsSize,
  IN CONST  VOID    *LoadOptions
  )
{
  return (
            ((LoadOptions) != NULL)
         && ((LoadOptionsSize) >= sizeof (CHAR16))
         && (((LoadOptionsSize) % sizeof (CHAR16)) == 0)
         && (((CHAR16 *)(LoadOptions))[((LoadOptionsSize) / 2) - 1] == CHAR_NULL)
            );
}

EFI_STATUS
OcParseLoadOptions (
  IN     CONST EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  OUT       OC_FLEX_ARRAY                 **ParsedVars
  )
{
  EFI_STATUS  Status;

  ASSERT (LoadedImage != NULL);
  ASSERT (ParsedVars != NULL);
  *ParsedVars = NULL;

  if (!OcValidLoadOptions (LoadedImage->LoadOptionsSize, LoadedImage->LoadOptions)) {
    DEBUG ((DEBUG_ERROR, "OCB: Invalid LoadOptions (%p:%u)\n", LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize));
    return EFI_INVALID_PARAMETER;
  }

  if (!OcHasLoadOptions (LoadedImage->LoadOptionsSize, LoadedImage->LoadOptions)) {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No LoadOptions (%p:%u)\n", LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize));
    return EFI_NOT_FOUND;
  }

  Status = OcParseVars (LoadedImage->LoadOptions, ParsedVars, OcStringFormatUnicode, FALSE);

  if (Status == EFI_INVALID_PARAMETER) {
    DEBUG ((DEBUG_ERROR, "OCB: Failed to parse LoadOptions (%p:%u)\n", LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize));
  } else if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_WARN, "OCB: Empty LoadOptions\n"));
  }

  return Status;
}

EFI_STATUS
OcParseVars (
  IN           VOID              *StrVars,
  OUT       OC_FLEX_ARRAY        **ParsedVars,
  IN     CONST OC_STRING_FORMAT  StringFormat,
  IN     CONST BOOLEAN           TokensOnly
  )
{
  VOID              *Pos;
  VOID              *NewPos;
  PARSE_VARS_STATE  State;
  PARSE_VARS_STATE  PushState;
  BOOLEAN           Retake;
  CHAR16            Ch;
  CHAR16            NewCh;
  CHAR16            QuoteChar;
  VOID              *Name;
  VOID              *Value;
  VOID              *OriginalValue;
  OC_PARSED_VAR     *Option;

  if ((StrVars == NULL) || ((StringFormat == OcStringFormatUnicode) ? (((CHAR16 *)StrVars)[0] == CHAR_NULL) : (((CHAR8 *)StrVars)[0] == '\0'))) {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No vars (%p)\n", StrVars));
    return EFI_NOT_FOUND;
  }

  *ParsedVars = OcFlexArrayInit (sizeof (OC_PARSED_VAR), NULL);
  if (*ParsedVars == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Pos       = StrVars;
  State     = PARSE_VARS_WHITE_SPACE;
  PushState = PARSE_VARS_WHITE_SPACE;
  Retake    = FALSE;
  QuoteChar = CHAR_NULL;

  do {
    Ch = (StringFormat == OcStringFormatUnicode) ? *((CHAR16 *)Pos) : *((CHAR8 *)Pos);
    switch (State) {
      case PARSE_VARS_WHITE_SPACE:
        if (Ch == '#') {
          State = PARSE_VARS_COMMENT;
        } else if (!OcIsSpaceOrNull (Ch)) {
          if (TokensOnly) {
            Option = OcFlexArrayAddItem (*ParsedVars);
            if (Option == NULL) {
              OcFlexArrayFree (ParsedVars);
              return EFI_OUT_OF_RESOURCES;
            }

            DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Value-only token\n"));

            State         = PARSE_VARS_VALUE;
            Value         = Pos;
            OriginalValue = Value;
            Retake        = TRUE;
          } else {
            State = PARSE_VARS_NAME;
            Name  = Pos;
          }
        }

        break;

      case PARSE_VARS_COMMENT:
        if (Ch == '\n') {
          State = PARSE_VARS_WHITE_SPACE;
        }

        break;

      case PARSE_VARS_NAME:
        if ((Ch == L'=') || OcIsSpaceOrNull (Ch)) {
          if (StringFormat == OcStringFormatUnicode) {
            *((CHAR16 *)Pos) = CHAR_NULL;
          } else {
            *((CHAR8 *)Pos) = '\0';
          }

          if (Ch == L'=') {
            State         = PARSE_VARS_VALUE;
            Value         = (UINT8 *)Pos + ((StringFormat == OcStringFormatUnicode) ? sizeof (CHAR16) : sizeof (CHAR8));
            OriginalValue = Value;
          } else {
            State = PARSE_VARS_WHITE_SPACE;
          }

          Option = OcFlexArrayAddItem (*ParsedVars);
          if (Option == NULL) {
            OcFlexArrayFree (ParsedVars);
            return EFI_OUT_OF_RESOURCES;
          }

          if (StringFormat == OcStringFormatUnicode) {
            Option->Unicode.Name = Name;
            DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Name=\"%s\"\n", Name));
          } else {
            Option->Ascii.Name = Name;
            DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Name=\"%a\"\n", Name));
          }

          if (State == PARSE_VARS_WHITE_SPACE) {
            DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No value %u\n", 1));
          }

          Name = NULL;
        }

        break;

      case PARSE_VARS_SHELL_EXPANSION:
        if (Ch == '`') {
          ASSERT (PushState != PARSE_VARS_WHITE_SPACE);
          State = PushState;
        }

        break;

      //
      // In token value (but not name) we handle sh and grub quoting and string concatenation, e.g. 'abc\'"'\""def becomes abc\'"def.
      //
      case PARSE_VARS_VALUE:
      case PARSE_VARS_QUOTED_VALUE:
        if ((State != PARSE_VARS_QUOTED_VALUE) && ((Ch == L'\'') || (Ch == L'"'))) {
          QuoteChar = Ch;
          SHIFT_TOKEN (Pos, Value, (StringFormat == OcStringFormatUnicode) ? sizeof (CHAR16) : sizeof (CHAR8));
          State = PARSE_VARS_QUOTED_VALUE;
        } else if ((State == PARSE_VARS_QUOTED_VALUE) && (Ch == QuoteChar)) {
          SHIFT_TOKEN (Pos, Value, (StringFormat == OcStringFormatUnicode) ? sizeof (CHAR16) : sizeof (CHAR8));
          QuoteChar = CHAR_NULL;
          State     = PARSE_VARS_VALUE;
        } else if (((State != PARSE_VARS_QUOTED_VALUE) || (QuoteChar == L'"')) && (Ch == L'\\')) {
          NewPos = (UINT8 *)Pos + ((StringFormat == OcStringFormatUnicode) ? sizeof (CHAR16) : sizeof (CHAR8));
          NewCh  = (StringFormat == OcStringFormatUnicode) ? *((CHAR16 *)Pos) : *((CHAR8 *)Pos);
          //
          // https://www.gnu.org/software/bash/manual/html_node/Double-Quotes.html
          //
          if ((State != PARSE_VARS_QUOTED_VALUE) || (NewCh == '"') || (NewCh == '\\') || (NewCh == '$') || (NewCh == '`')) {
            SHIFT_TOKEN (Pos, Value, (StringFormat == OcStringFormatUnicode) ? sizeof (CHAR16) : sizeof (CHAR8));
            Pos = NewPos;
            Ch  = NewCh;
          }
        } else if (Ch == L'`') {
          PushState = State;
          State     = PARSE_VARS_SHELL_EXPANSION;
        } else if ((State == PARSE_VARS_VALUE) && OcIsSpaceOrNull (Ch)) {
          //
          // Explicitly quoted empty string (e.g. `var=""`) is stored detectably differently from missing value (i.e. `var=`, or just `var`).
          //
          if (Pos == OriginalValue) {
            ASSERT (!TokensOnly);
            DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No value %u\n", 2));
          } else {
            if (PushState != PARSE_VARS_WHITE_SPACE) {
              DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Found shell expansion, cancelling value\n"));
              PushState = PARSE_VARS_WHITE_SPACE;
            } else {
              if (StringFormat == OcStringFormatUnicode) {
                *((CHAR16 *)Pos)      = CHAR_NULL;
                Option->Unicode.Value = Value;
                DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Value=\"%s\"\n", Value));
              } else {
                *((CHAR8 *)Pos)     = '\0';
                Option->Ascii.Value = Value;
                DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Value=\"%a\"\n", Value));
              }
            }
          }

          Value         = NULL;
          OriginalValue = NULL;
          Option        = NULL;
          State         = PARSE_VARS_WHITE_SPACE;
        }

        break;

      default:
        ASSERT (FALSE);
        break;
    }

    if (Retake) {
      Retake = FALSE;
    } else {
      Pos = (UINT8 *)Pos + ((StringFormat == OcStringFormatUnicode) ? sizeof (CHAR16) : sizeof (CHAR8));
    }
  } while (Ch != CHAR_NULL);

  if ((State != PARSE_VARS_WHITE_SPACE) || (PushState != PARSE_VARS_WHITE_SPACE)) {
    //
    // E.g. for GRUB config files this may potentially be caused by a file
    // neither we nor the user directly controls, so better warn than error.
    //
    DEBUG ((DEBUG_WARN, "OCB: Invalid vars (%u)\n", State));
    OcFlexArrayFree (ParsedVars);
    return EFI_INVALID_PARAMETER;
  }

  if ((*ParsedVars)->Items == NULL) {
    OcFlexArrayFree (ParsedVars);
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

OC_PARSED_VAR *
OcParsedVarsItemAt (
  IN     CONST OC_FLEX_ARRAY  *ParsedVars,
  IN     CONST UINTN          Index
  )
{
  OC_PARSED_VAR  *Option;

  Option = OcFlexArrayItemAt (ParsedVars, Index);
  return Option;
}

BOOLEAN
OcParsedVarsGetStr (
  IN     CONST OC_FLEX_ARRAY     *ParsedVars,
  IN     CONST VOID              *Name,
  OUT       VOID                 **Value,
  IN     CONST OC_STRING_FORMAT  StringFormat
  )
{
  UINTN          Index;
  OC_PARSED_VAR  *Option;

  ASSERT (Name != NULL);
  ASSERT (Value != NULL);

  if (ParsedVars == NULL) {
    return FALSE;
  }

  ASSERT (ParsedVars->Items != NULL);

  for (Index = 0; Index < ParsedVars->Count; ++Index) {
    Option = OcFlexArrayItemAt (ParsedVars, Index);
    if (StringFormat == OcStringFormatUnicode) {
      ASSERT (Option->Unicode.Name != NULL);
      if (StrCmp (Option->Unicode.Name, Name) == 0) {
        *Value = Option->Unicode.Value;
        DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Using \"%s\"=\"%s\"\n", Name, *Value));
        return TRUE;
      }
    } else {
      ASSERT (Option->Ascii.Name != NULL);
      if (AsciiStrCmp (Option->Ascii.Name, Name) == 0) {
        *Value = Option->Ascii.Value;
        DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Using \"%a\"=\"%a\"\n", Name, *Value));
        return TRUE;
      }
    }
  }

  if (StringFormat == OcStringFormatUnicode) {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No value for \"%s\"\n", Name));
  } else {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No value for \"%a\"\n", Name));
  }

  return FALSE;
}

BOOLEAN
OcParsedVarsGetUnicodeStr (
  IN     CONST OC_FLEX_ARRAY  *ParsedVars,
  IN     CONST CHAR16         *Name,
  OUT       CHAR16            **Value
  )
{
  return OcParsedVarsGetStr (ParsedVars, Name, (VOID **)Value, OcStringFormatUnicode);
}

BOOLEAN
OcParsedVarsGetAsciiStr (
  IN     CONST OC_FLEX_ARRAY  *ParsedVars,
  IN     CONST CHAR8          *Name,
  OUT       CHAR8             **Value
  )
{
  return OcParsedVarsGetStr (ParsedVars, Name, (VOID **)Value, OcStringFormatAscii);
}

BOOLEAN
OcHasParsedVar (
  IN     CONST OC_FLEX_ARRAY     *ParsedVars,
  IN     CONST VOID              *Name,
  IN     CONST OC_STRING_FORMAT  StringFormat
  )
{
  VOID  *Value;

  return OcParsedVarsGetStr (ParsedVars, Name, &Value, StringFormat);
}

EFI_STATUS
OcParsedVarsGetInt (
  IN     CONST OC_FLEX_ARRAY     *ParsedVars,
  IN     CONST VOID              *Name,
  OUT       UINTN                *Value,
  IN     CONST OC_STRING_FORMAT  StringFormat
  )
{
  EFI_STATUS  Status;
  VOID        *StrValue;

  if (!OcParsedVarsGetStr (ParsedVars, Name, &StrValue, StringFormat)) {
    return EFI_NOT_FOUND;
  }

  if (StrValue == NULL) {
    return EFI_NOT_FOUND;
  }

  if (StringFormat == OcStringFormatUnicode) {
    if (OcUnicodeStartsWith (StrValue, L"0x", TRUE)) {
      Status = StrHexToUintnS (StrValue, NULL, Value);
    } else {
      Status = StrDecimalToUintnS (StrValue, NULL, Value);
    }
  } else {
    if (OcAsciiStartsWith (StrValue, "0x", TRUE)) {
      Status = AsciiStrHexToUintnS (StrValue, NULL, Value);
    } else {
      Status = AsciiStrDecimalToUintnS (StrValue, NULL, Value);
    }
  }

  return Status;
}

EFI_STATUS
OcParsedVarsGetGuid (
  IN     CONST OC_FLEX_ARRAY     *ParsedVars,
  IN     CONST VOID              *Name,
  OUT       EFI_GUID             *Value,
  IN     CONST OC_STRING_FORMAT  StringFormat
  )
{
  EFI_STATUS  Status;
  VOID        *StrValue;

  if (!OcParsedVarsGetStr (ParsedVars, Name, &StrValue, StringFormat)) {
    return EFI_NOT_FOUND;
  }

  if (StrValue == NULL) {
    return EFI_NOT_FOUND;
  }

  if (StringFormat == OcStringFormatUnicode) {
    Status = StrToGuid (StrValue, Value);
  } else {
    Status = AsciiStrToGuid (StrValue, Value);
  }

  return Status;
}

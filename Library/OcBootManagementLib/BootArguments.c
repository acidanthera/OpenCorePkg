/** @file
  Copyright (C) 2019-2021, vit9696, mikebeaton. All rights reserved.<BR>
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
  PARSE_VARS_VALUE_FIRST,
  PARSE_VARS_VALUE,
  PARSE_VARS_QUOTED_VALUE,
  PARSE_VARS_SHELL_EXPANSION
} PARSE_VARS_STATE;


//
// Shift from token start to current position forwards by offset characters.
//
#define SHIFT_TOKEN(pos, token, offset) do { \
  CopyMem ((UINT8 *)(token) + (offset), (token), (UINT8 *)(pos) - (UINT8 *)(token)); \
  (token) = (UINT8 *)(token) + (offset); \
  (pos)   = (UINT8 *)(pos) + (offset); \
} while (0)

VOID
OcParseBootArgs (
  OUT OC_BOOT_ARGUMENTS *Arguments,
  IN  VOID              *BootArgs
  )
{
  BootArgs1  *BA1 = (BootArgs1 *) BootArgs;
  BootArgs2  *BA2 = (BootArgs2 *) BootArgs;

  ZeroMem (Arguments, sizeof (*Arguments));

  if (BA1->Version == kBootArgsVersion1) {
    //
    // Pre Lion
    //
    Arguments->MemoryMap = &BA1->MemoryMap;
    Arguments->MemoryMapSize = &BA1->MemoryMapSize;
    Arguments->MemoryMapDescriptorSize = &BA1->MemoryMapDescriptorSize;
    Arguments->MemoryMapDescriptorVersion = &BA1->MemoryMapDescriptorVersion;

    Arguments->CommandLine = &BA1->CommandLine[0];

    Arguments->KernelAddrP = &BA1->kaddr;
    Arguments->SystemTableP = &BA1->efiSystemTable;
    Arguments->RuntimeServicesPG = &BA1->efiRuntimeServicesPageStart;
    Arguments->RuntimeServicesV = &BA1->efiRuntimeServicesVirtualPageStart;
    Arguments->DeviceTreeP = &BA1->deviceTreeP;
    Arguments->DeviceTreeLength = &BA1->deviceTreeLength;
    Arguments->SystemTable = (EFI_SYSTEM_TABLE*)(UINTN) BA1->efiSystemTable;
  } else {
    //
    // Lion and newer
    //
    Arguments->MemoryMap = &BA2->MemoryMap;
    Arguments->MemoryMapSize = &BA2->MemoryMapSize;
    Arguments->MemoryMapDescriptorSize = &BA2->MemoryMapDescriptorSize;
    Arguments->MemoryMapDescriptorVersion = &BA2->MemoryMapDescriptorVersion;

    Arguments->CommandLine = &BA2->CommandLine[0];

    Arguments->KernelAddrP = &BA2->kaddr;
    Arguments->SystemTableP = &BA2->efiSystemTable;
    Arguments->RuntimeServicesPG = &BA2->efiRuntimeServicesPageStart;
    Arguments->RuntimeServicesV = &BA2->efiRuntimeServicesVirtualPageStart;
    Arguments->DeviceTreeP = &BA2->deviceTreeP;
    Arguments->DeviceTreeLength = &BA2->deviceTreeLength;
    Arguments->SystemTable = (EFI_SYSTEM_TABLE*)(UINTN) BA2->efiSystemTable;

    if (BA2->flags & kBootArgsFlagCSRActiveConfig) {
      Arguments->CsrActiveConfig = &BA2->csrActiveConfig;
    }
  }
}

CONST CHAR8 *
OcGetArgumentFromCmd (
  IN  CONST CHAR8   *CommandLine,
  IN  CONST CHAR8   *Argument,
  IN  CONST UINTN   ArgumentLength,
  OUT UINTN         *ValueLength OPTIONAL
  )
{
  CHAR8   *Str;
  CHAR8   *StrEnd;

  Str = AsciiStrStr (CommandLine, Argument);

  //
  // Invalidate found boot arg if:
  // - it is neither the beginning of Cmd, nor has space prefix            -> boot arg is a suffix of another arg
  // - it has neither space suffix, nor \0 suffix, and does not end with = -> boot arg is a prefix of another arg
  //
  if (!Str || (Str != CommandLine && *(Str - 1) != ' ') ||
      (Str[ArgumentLength] != ' ' && Str[ArgumentLength] != '\0' &&
       Str[ArgumentLength - 1] != '=')) {
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
  Match = CommandLine;

  do {
    Match = AsciiStrStr (Match, Argument);
    if (Match != NULL && (Match == CommandLine || *(Match - 1) == ' ')
      && (Match[ArgumentLength - 1] == '='
        || Match[ArgumentLength] == ' '
        || Match[ArgumentLength] == '\0')) {
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
  EFI_STATUS Status;
  UINTN      Len = AsciiStrLen (CommandLine);

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
  if (Len + (Len > 0 ? 1 : 0) + ArgumentLength >= BOOT_LINE_LENGTH) {
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
    if (OcOverflowAddUN (TotalLength, ArgumentLength, &TotalLength)) {
      return FALSE;
    }
  }

  if (OcOverflowMulUN (TotalLength, sizeof (CHAR16), &TotalLength)) {
    return FALSE;
  }

  Replace |= LoadedImage->LoadOptionsSize < sizeof (CHAR16);
  if (!Replace
    && OcOverflowTriAddUN (TotalLength, sizeof (CHAR16), LoadedImage->LoadOptionsSize, &TotalLength)) {
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
    Walker += LoadedImage->LoadOptionsSize / sizeof (CHAR16);
    *Walker++ = '\0';
  }

  LoadedImage->LoadOptions = NewArguments;
  LoadedImage->LoadOptionsSize = (UINT32) TotalLength;

  return TRUE;
}

BOOLEAN
OcCheckArgumentFromEnv (
  IN     EFI_LOADED_IMAGE   *LoadedImage OPTIONAL,
  IN     EFI_GET_VARIABLE   GetVariable OPTIONAL,
  IN     CONST CHAR8        *Argument,
  IN     CONST UINTN        ArgumentLength,
  IN OUT CHAR8              **Value OPTIONAL
  )
{
  CHAR16      *Options;
  UINTN       OptionsSize;
  CHAR8       BootArgsVar[BOOT_LINE_LENGTH];
  UINTN       BootArgsVarLen;
  EFI_STATUS  Status;
  UINTN       LastIndex;
  CHAR16      Last;
  BOOLEAN     HasArgument;
  CONST CHAR8 *ArgValue;
  UINTN       ArgValueLength;

  HasArgument = FALSE;

  if (LoadedImage != NULL) {
    Options     = (CHAR16 *) LoadedImage->LoadOptions;
    OptionsSize = LoadedImage->LoadOptionsSize / sizeof (CHAR16);

    if (Options != NULL && OptionsSize > 0) {
      //
      // Just in case we do not have 0-termination.
      // This may cut some data with unexpected options, but it is not like we care.
      //
      LastIndex = OptionsSize - 1;
      Last = Options[LastIndex];
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
    Status = (GetVariable != NULL ? GetVariable : gRT->GetVariable) (
      L"boot-args",
      &gAppleBootVariableGuid,
      NULL,
      &BootArgsVarLen,
      BootArgsVar
      );

    if (!EFI_ERROR (Status) && BootArgsVarLen > 0) {
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

  if (HasArgument && Value != NULL) {
    *Value = AllocateCopyPool (AsciiStrnSizeS (ArgValue, ArgValueLength), ArgValue);
    if (*Value == NULL) {
      return FALSE;
    }
  }

  return HasArgument;
}

EFI_STATUS
OcParseLoadOptions (
  IN     CONST EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
     OUT       OC_FLEX_ARRAY              **ParsedVars
  )
{
  EFI_STATUS      Status;

  ASSERT (LoadedImage != NULL);
  ASSERT (ParsedVars != NULL);
  *ParsedVars = NULL;

  if (LoadedImage->LoadOptionsSize % sizeof (CHAR16) != 0 || LoadedImage->LoadOptionsSize > MAX_LOAD_OPTIONS_SIZE) {
    DEBUG ((DEBUG_ERROR, "OCB: Invalid LoadOptions (%p:%u)\n", LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize));
    return EFI_INVALID_PARAMETER;
  }

  if (LoadedImage->LoadOptions == NULL ||
    LoadedImage->LoadOptionsSize == 0 ||
    ((CHAR16 *)LoadedImage->LoadOptions)[0] == CHAR_NULL) {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No LoadOptions (%p:%u)\n", LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize));
    return EFI_NOT_FOUND;
  }

  Status = OcParseVars (LoadedImage->LoadOptions, ParsedVars, TRUE);

  if (Status == EFI_INVALID_PARAMETER) {
    DEBUG ((DEBUG_ERROR, "OCB: Invalid LoadOptions (%p:%u)\n", LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize));
  } else if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_WARN, "OCB: Empty LoadOptions\n"));
  }

  return Status;
}

EFI_STATUS
OcParseVars (
  IN           VOID                       *StrVars,
     OUT       OC_FLEX_ARRAY              **ParsedVars,
  IN     CONST BOOLEAN                    IsUnicode
  )
{
  VOID                        *Pos;
  PARSE_VARS_STATE            State;
  PARSE_VARS_STATE            PushState;
  BOOLEAN                     Retake;
  CHAR16                      Ch;
  VOID                        *Name;
  VOID                        *Value;
  OC_PARSED_VAR               *Option;

  if (StrVars == NULL || (IsUnicode ? ((CHAR16 *) StrVars)[0] == CHAR_NULL : ((CHAR8 *) StrVars)[0] == '\0')) {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No vars (%p)\n", StrVars));
    return EFI_NOT_FOUND;
  }

  *ParsedVars = OcFlexArrayInit (sizeof (OC_PARSED_VAR), NULL);
  if (*ParsedVars == NULL) {
    return  EFI_OUT_OF_RESOURCES;
  }

  Pos       = StrVars;
  State     = PARSE_VARS_WHITE_SPACE;
  PushState = PARSE_VARS_WHITE_SPACE;
  Retake    = FALSE;

  do {
    Ch = IsUnicode ? *((CHAR16 *) Pos) : *((CHAR8 *) Pos);
    switch (State) {
      case PARSE_VARS_WHITE_SPACE:
        if (Ch == '#') {
          State = PARSE_VARS_COMMENT;
        } else if (!(OcIsSpace (Ch) || Ch == CHAR_NULL)) {
          Name = Pos;
          State = PARSE_VARS_NAME;
        }
        break;

      case PARSE_VARS_COMMENT:
        if (Ch == '\n') {
          State = PARSE_VARS_WHITE_SPACE;
        }
        break;

      case PARSE_VARS_NAME:
        if (Ch == L'=' || OcIsSpace (Ch) || Ch == CHAR_NULL) {
          if (IsUnicode) {
            *((CHAR16 *) Pos) = CHAR_NULL;
          } else {
            *((CHAR8 *) Pos) = '\0';
          }
          if (Ch == L'=') {
            State = PARSE_VARS_VALUE_FIRST;
          } else {
            State = PARSE_VARS_WHITE_SPACE;
          }
          Option = OcFlexArrayAddItem (*ParsedVars);
          if (Option == NULL) {
            OcFlexArrayFree (ParsedVars);
            return EFI_OUT_OF_RESOURCES;
          }
          if (IsUnicode) {
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

      case PARSE_VARS_VALUE_FIRST:
        if (Ch == L'"') {
          State = PARSE_VARS_QUOTED_VALUE;
          Value = (UINT8 *) Pos + (IsUnicode ? sizeof (CHAR16) : sizeof (CHAR8));
        } else {
          State = PARSE_VARS_VALUE;
          Value = Pos;
          Retake = TRUE;
        }
        break;

      case PARSE_VARS_SHELL_EXPANSION:
        if (Ch == '`') {
          ASSERT (PushState != PARSE_VARS_WHITE_SPACE);
          State = PushState;
        }
        break;

      case PARSE_VARS_VALUE:
      case PARSE_VARS_QUOTED_VALUE:
        if (Ch == L'`') {
          PushState = State;
          State = PARSE_VARS_SHELL_EXPANSION;
        } else if (Ch == L'\\') {
          SHIFT_TOKEN (Pos, Value, IsUnicode ? sizeof (CHAR16) : sizeof (CHAR8));
          Ch = IsUnicode ? *((CHAR16 *) Pos) : *((CHAR8 *) Pos);
        } else if (
          (State == PARSE_VARS_VALUE && (OcIsSpace (Ch) || Ch == CHAR_NULL)) ||
          (State == PARSE_VARS_QUOTED_VALUE && Ch == '"')) {
          //
          // Explicitly quoted empty string needs to be stored detectably
          // differently from missing value.
          //
          if (State != PARSE_VARS_QUOTED_VALUE && Pos == Value) {
            DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No value %u\n", 2));
          } else {
            if (PushState != PARSE_VARS_WHITE_SPACE) {
              DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Found shell expansion, cancelling value\n"));
              PushState = PARSE_VARS_WHITE_SPACE;
            } else {
              if (IsUnicode) {
                *((CHAR16 *) Pos) = CHAR_NULL;
                Option->Unicode.Value = Value;
                DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Value=\"%s\"\n", Value));
              } else {
                *((CHAR8 *) Pos) = '\0';
                Option->Ascii.Value = Value;
                DEBUG ((OC_TRACE_PARSE_VARS, "OCB: Value=\"%a\"\n", Value));
              }
            }
          }
          Value = NULL;
          Option = NULL;
          State = PARSE_VARS_WHITE_SPACE;
        }
        break;

      default:
        ASSERT (FALSE);
        break;
    }

    if (Retake) {
      Retake = FALSE;
    } else {
      Pos = (UINT8 *) Pos + (IsUnicode ? sizeof (CHAR16) : sizeof (CHAR8));
    }
  } while (Ch != CHAR_NULL);

  if (State != PARSE_VARS_WHITE_SPACE || PushState != PARSE_VARS_WHITE_SPACE) {
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

BOOLEAN
OcParsedVarsGetStr (
  IN     CONST OC_FLEX_ARRAY      *ParsedVars,
  IN     CONST VOID               *Name,
     OUT       VOID               **Value,
  IN     CONST BOOLEAN            IsUnicode
  )
{
  UINTN           Index;
  OC_PARSED_VAR  *Option;

  ASSERT (Name != NULL);
  ASSERT (Value != NULL);

  if (ParsedVars == NULL) {
    return FALSE;
  }

  ASSERT (ParsedVars->Items != NULL);

  for (Index = 0; Index < ParsedVars->Count; ++Index) {
    Option = OcFlexArrayItemAt (ParsedVars, Index);
    if (IsUnicode) {
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

  if (IsUnicode) {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No value for \"%s\"\n", Name));
  } else {
    DEBUG ((OC_TRACE_PARSE_VARS, "OCB: No value for \"%a\"\n", Name));
  }

  return FALSE;
}

BOOLEAN
OcParsedVarsGetUnicodeStr (
  IN     CONST OC_FLEX_ARRAY      *ParsedVars,
  IN     CONST CHAR16             *Name,
     OUT       CHAR16             **Value
  )
{
  return OcParsedVarsGetStr (ParsedVars, Name, (VOID**)Value, TRUE);
}

BOOLEAN
OcParsedVarsGetAsciiStr (
  IN     CONST OC_FLEX_ARRAY      *ParsedVars,
  IN     CONST CHAR8              *Name,
     OUT       CHAR8              **Value
  )
{
  return OcParsedVarsGetStr (ParsedVars, Name, (VOID**)Value, FALSE);
}

BOOLEAN
OcHasParsedVar (
  IN     CONST OC_FLEX_ARRAY      *ParsedVars,
  IN     CONST VOID               *Name,
  IN     CONST BOOLEAN            IsUnicode
  )
{
  VOID        *Value;

  return OcParsedVarsGetStr (ParsedVars, Name, &Value, IsUnicode);
}

EFI_STATUS
OcParsedVarsGetInt (
  IN     CONST OC_FLEX_ARRAY      *ParsedVars,
  IN     CONST VOID               *Name,
     OUT       UINTN              *Value,
  IN     CONST BOOLEAN            IsUnicode
  )
{
  EFI_STATUS  Status;
  VOID        *StrValue;

  if (!OcParsedVarsGetStr (ParsedVars, Name, &StrValue, IsUnicode)) {
    return EFI_NOT_FOUND;
  }

  if (StrValue == NULL) {
    return EFI_NOT_FOUND;
  }

  if (IsUnicode){
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
  IN     CONST OC_FLEX_ARRAY      *ParsedVars,
  IN     CONST VOID               *Name,
     OUT       EFI_GUID           *Value,
  IN     CONST BOOLEAN            IsUnicode
  )
{
  EFI_STATUS  Status;
  VOID        *StrValue;

  if (!OcParsedVarsGetStr (ParsedVars, Name, &StrValue, IsUnicode)) {
    return EFI_NOT_FOUND;
  }

  if (StrValue == NULL) {
    return EFI_NOT_FOUND;
  }

  if (IsUnicode) {
    Status = StrToGuid (StrValue, Value);
  } else {
    Status = AsciiStrToGuid (StrValue, Value);
  }

  return Status;
}

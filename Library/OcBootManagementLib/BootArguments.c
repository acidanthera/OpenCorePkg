/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/AppleVariable.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

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
  IN CONST CHAR8  *CommandLine,
  IN CONST CHAR8  *Argument,
  IN CONST UINTN  ArgumentLength
  )
{
  CHAR8 *Str;

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

  return Str + ArgumentLength;
}

VOID
OcRemoveArgumentFromCmd (
  IN OUT CHAR8        *CommandLine,
  IN     CONST CHAR8  *Argument
  )
{
  CHAR8 *Match;

  Match = NULL;

  do {
    Match = AsciiStrStr (CommandLine, Argument);
    if (Match && (Match == CommandLine || *(Match - 1) == ' ')) {
      while (*Match != ' ' && *Match != '\0') {
        *Match++ = ' ';
      }
    }
  } while (Match != NULL);

  //
  // Write zeroes to reduce data leak
  //
  CHAR8 *Updated = CommandLine;

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
    Status = Context->RequestPrivilege (
      Context,
      OcPrivilegeAuthorized
      );
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
OcCheckArgumentFromEnv (
  IN EFI_LOADED_IMAGE  *LoadedImage  OPTIONAL,
  IN EFI_GET_VARIABLE  GetVariable  OPTIONAL,
  IN CONST CHAR8       *Argument,
  IN CONST UINTN       ArgumentLength
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

      if (OcGetArgumentFromCmd (BootArgsVar, Argument, ArgumentLength)) {
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

      if (OcGetArgumentFromCmd (BootArgsVar, Argument, ArgumentLength)) {
        HasArgument = TRUE;
      }
    }
  }

  return HasArgument;
}

/** @file
  Play beep.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/ShellParameters.h>

STATIC
EFI_STATUS
GetArguments (
  OUT UINTN   *Argc,
  OUT CHAR16  ***Argv
  )
{
  EFI_STATUS                     Status;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters;

  Status = gBS->HandleProtocol (
    gImageHandle,
    &gEfiShellParametersProtocolGuid,
    (VOID**) &ShellParameters
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Argc = ShellParameters->Argc;
  *Argv = ShellParameters->Argv;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  UINTN             Argc;
  CHAR16            **Argv;
  CHAR16            *Mode;
  EFI_RESET_TYPE    ResetMode;

  Status = GetArguments (&Argc, &Argv);
  if (!EFI_ERROR (Status) && Argc >= 2) {
    Mode = Argv[1];
  } else {
    DEBUG ((DEBUG_INFO, "OCRST: Assuming default to be ResetCold...\n"));
    Mode = L"ResetCold";
  }

  if (StrCmp (Mode, L"ColdReset") == 0) {
    DEBUG ((DEBUG_INFO, "OCRST: Perform cold reset...\n"));
    ResetMode = EfiResetCold;
  } else if (StrCmp (Mode, L"WarmReset") == 0) {
    DEBUG ((DEBUG_INFO, "OCRST: Perform warm reset...\n"));
    ResetMode = EfiResetWarm;
  } else if (StrCmp (Mode, L"Shutdown") == 0) {
    DEBUG ((DEBUG_INFO, "OCRST: Perform shutdown...\n"));
    ResetMode = EfiResetShutdown;
  } else {
    DEBUG ((DEBUG_INFO, "OCRST: Unknown argument %s, defaulting to cold reset...\n", Mode));
    ResetMode = EfiResetCold;
  }

  gRT->ResetSystem (
    ResetMode,
    EFI_SUCCESS,
    0,
    NULL
    );

  DEBUG ((DEBUG_INFO, "OCRST: Failed to reset, trying direct\n"));

  DirectRestCold ();

  DEBUG ((DEBUG_INFO, "OCRST: Failed to reset directly, entering dead loop\n"));

  CpuDeadLoop ();

  return EFI_SUCCESS; //< Unreachable
}

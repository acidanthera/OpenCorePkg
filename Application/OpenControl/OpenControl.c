/** @file
  Control OpenCore instance.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/OcFirmwareRuntime.h>
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
  EFI_STATUS                            Status;
  UINTN                                 Argc;
  CHAR16                                **Argv;
  OC_FIRMWARE_RUNTIME_PROTOCOL          *FwRuntime;
  OC_FWRT_CONFIG                        Config;

  Status = GetArguments (&Argc, &Argv);
  if (!EFI_ERROR (Status) && Argc == 2) {
    Status = gBS->LocateProtocol (
      &gOcFirmwareRuntimeProtocolGuid,
      NULL,
      (VOID **) &FwRuntime
      );

    if (EFI_ERROR (Status)) {
      Print (L"FwRuntime protocol is unavilable - %r\n", Status);
      return EFI_SUCCESS;
    }

    if (StrCmp (Argv[1], L"disable") == 0) {
      ZeroMem (&Config, sizeof (Config));
      FwRuntime->SetOverride (&Config);
      return EFI_SUCCESS;
    }

    if (StrCmp (Argv[1], L"restore") == 0) {
      FwRuntime->SetOverride (NULL);
      return EFI_SUCCESS;
    }
  }

  Print (L"Usage: OpenControl <disable|restore>\n");
  return EFI_SUCCESS;
}

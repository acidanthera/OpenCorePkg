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

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/ShellParameters.h>

EFI_STATUS
GetArguments (
  OUT UINTN   *Argc,
  OUT CHAR16  ***Argv
  )
{
  EFI_STATUS                     Status;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters;
  EFI_LOADED_IMAGE_PROTOCOL      *LoadedImage;

  Status = gBS->HandleProtocol (
    gImageHandle,
    &gEfiShellParametersProtocolGuid,
    (VOID**) &ShellParameters
    );
  if (!EFI_ERROR (Status)) {
    *Argc = ShellParameters->Argc;
    *Argv = ShellParameters->Argv;
    return EFI_SUCCESS;
  }

  Status = gBS->HandleProtocol (
    gImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID**) &LoadedImage
    );
  if (EFI_ERROR (Status) || LoadedImage->LoadOptions == NULL) {
    return EFI_NOT_FOUND;
  }

  STATIC CHAR16 *StArgv[2] = {L"Self", NULL};
  StArgv[1] = LoadedImage->LoadOptions;
  *Argc = ARRAY_SIZE (StArgv);
  *Argv = StArgv;
  return EFI_SUCCESS;
}

EFI_STATUS
OcUninstallAllProtocolInstances (
  EFI_GUID  *Protocol
  )
{
  EFI_STATUS      Status;
  EFI_HANDLE      *Handles;
  UINTN           Index;
  UINTN           NoHandles;
  VOID            *OriginalProto;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    Protocol,
    NULL,
    &NoHandles,
    &Handles
    );

  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < NoHandles; ++Index) {
    Status = gBS->HandleProtocol (
      Handles[Index],
      Protocol,
      &OriginalProto
      );

    if (EFI_ERROR (Status)) {
      break;
    }

    Status = gBS->UninstallProtocolInterface (
      Handles[Index],
      Protocol,
      OriginalProto
      );

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  gBS->FreePool (Handles);

  return Status;
}

EFI_STATUS
OcHandleProtocolFallback (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  EFI_STATUS Status;

  Status = gBS->HandleProtocol (
                  Handle,
                  Protocol,
                  Interface
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (
                    Protocol,
                    NULL,
                    Interface
                    );
  }

  return Status;
}

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

#include <Protocol/ConsoleControl.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC EFI_CONSOLE_CONTROL_SCREEN_MODE mConsoleMode = EfiConsoleControlScreenText;

STATIC
EFI_CONSOLE_CONTROL_PROTOCOL
mOriginalConsoleControlProtocol;

STATIC
EFI_STATUS
EFIAPI
ConsoleControlGetMode (
  IN   EFI_CONSOLE_CONTROL_PROTOCOL    *This,
  OUT  EFI_CONSOLE_CONTROL_SCREEN_MODE *Mode,
  OUT  BOOLEAN                         *GopUgaExists OPTIONAL,
  OUT  BOOLEAN                         *StdInLocked  OPTIONAL
  )
{
  *Mode = mConsoleMode;

  if (GopUgaExists) {
    *GopUgaExists = TRUE;
  }

  if (StdInLocked) {
    *StdInLocked  = FALSE;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsoleControlSetMode (
  IN EFI_CONSOLE_CONTROL_PROTOCOL     *This,
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode
  )
{
  mConsoleMode = Mode;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsoleControlLockStdIn (
  IN EFI_CONSOLE_CONTROL_PROTOCOL *This,
  IN CHAR16                       *Password
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_CONSOLE_CONTROL_PROTOCOL
mConsoleControlProtocol = {
  ConsoleControlGetMode,
  ConsoleControlSetMode,
  ConsoleControlLockStdIn
};

EFI_STATUS
ConfigureConsoleControl (
  IN BOOLEAN              WrapExisting
  )
{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;
  EFI_HANDLE                    NewHandle;

  Status = gBS->LocateProtocol (
    &gEfiConsoleControlProtocolGuid,
    NULL,
    (VOID *) &ConsoleControl
    );

  //
  // Native implementation exists, ignore.
  //
  if (!EFI_ERROR (Status)) {
    if (WrapExisting) {
      CopyMem (
        &mOriginalConsoleControlProtocol,
        ConsoleControl,
        sizeof (mOriginalConsoleControlProtocol)
        );
      CopyMem (
        &ConsoleControl,
        &mConsoleControlProtocol,
        sizeof (mOriginalConsoleControlProtocol)
        );
    }

    return EFI_SUCCESS;
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gEfiConsoleControlProtocolGuid,
    &mConsoleControlProtocol,
    NULL
    );

  return Status;
}

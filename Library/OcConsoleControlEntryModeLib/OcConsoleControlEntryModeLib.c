/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/ConsoleControl.h>

#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
OcConsoleControlEntryModeConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl;

  if (FixedPcdGet8 (PcdConsoleControlEntryMode) < EfiConsoleControlScreenMaxValue) {
    //
    // On several types of firmware, we need to use legacy console control protocol to
    // switch to text mode, otherwise a black screen will be shown.
    //
    Status = gBS->HandleProtocol (
      gST->ConsoleOutHandle,
      &gEfiConsoleControlProtocolGuid,
      (VOID **) &ConsoleControl
      );
    if (EFI_ERROR (Status)) {
      Status = gBS->LocateProtocol (
      &gEfiConsoleControlProtocolGuid,
      NULL,
      (VOID **) &ConsoleControl
      );
    }

    if (!EFI_ERROR (Status)) {
      ConsoleControl->SetMode (
        ConsoleControl,
        FixedPcdGet8 (PcdConsoleControlEntryMode)
        );
    }
  }

  return EFI_SUCCESS;
}

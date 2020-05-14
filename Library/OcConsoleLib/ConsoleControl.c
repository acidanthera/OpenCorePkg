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

#include "OcConsoleLibInternal.h"

#include <Protocol/ConsoleControl.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_CONSOLE_CONTROL_SCREEN_MODE
OcConsoleControlSetMode (
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode
  )
{
  EFI_STATUS                       Status;
  EFI_CONSOLE_CONTROL_PROTOCOL     *ConsoleControl;
  EFI_CONSOLE_CONTROL_SCREEN_MODE  OldMode;

  Status = OcHandleProtocolFallback (
    &gST->ConsoleOutHandle,
    &gEfiConsoleControlProtocolGuid,
    (VOID *) &ConsoleControl
    );

  //
  // No console control, assume already set.
  //
  if (EFI_ERROR (Status)) {
    return Mode;
  }

  Status = ConsoleControl->GetMode (
    ConsoleControl,
    &OldMode,
    NULL,
    NULL
    );

  //
  // Cannot get mode, assume already set.
  // Same mode, do not waste time.
  //
  if (EFI_ERROR (Status) || OldMode == Mode) {
    return Mode;
  }

  ConsoleControl->SetMode (
    ConsoleControl,
    Mode
    );

  return OldMode;
}

EFI_STATUS
OcConsoleControlInstallProtocol (
  IN  EFI_CONSOLE_CONTROL_PROTOCOL     *NewProtocol,
  OUT EFI_CONSOLE_CONTROL_PROTOCOL     *OldProtocol  OPTIONAL,
  OUT EFI_CONSOLE_CONTROL_SCREEN_MODE  *OldMode  OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

  Status = OcHandleProtocolFallback (
    &gST->ConsoleOutHandle,
    &gEfiConsoleControlProtocolGuid,
    (VOID *) &ConsoleControl
    );

  DEBUG ((
    DEBUG_INFO,
    "OCC: Install console control (%p/%p/%p), current - %r\n",
    NewProtocol,
    OldProtocol,
    OldMode,
    Status
    ));

  //
  // Native implementation exists, overwrite.
  //
  if (!EFI_ERROR (Status)) {
    //
    // Provide original mode if requested.
    //
    if (OldMode != NULL) {
      ConsoleControl->GetMode (ConsoleControl, OldMode, NULL, NULL);
    }

    if (OldProtocol != NULL) {
      CopyMem (
        OldProtocol,
        ConsoleControl,
        sizeof (*OldProtocol)
        );
    }

    CopyMem (
      ConsoleControl,
      NewProtocol,
      sizeof (*ConsoleControl)
      );

    return EFI_SUCCESS;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
    &gST->ConsoleOutHandle,
    &gEfiConsoleControlProtocolGuid,
    NewProtocol,
    NULL
    );

  DEBUG ((DEBUG_INFO, "OCC: Install console control, new - %r\n", Status));

  return Status;
}

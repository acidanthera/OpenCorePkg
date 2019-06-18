/** @file
  Test external interface support.

  Copyright (c) 2019, vit9696. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/OcInterface.h>

STATIC
EFI_STATUS
EFIAPI
ExternalGuiRun (
  IN OC_INTERFACE_PROTOCOL  *This,
  IN OC_STORAGE_CONTEXT     *Storage,
  IN OC_PICKER_CONTEXT      *Picker
  )
{
  //
  // Storage could be used to access custom graphical theme.
  // One must NOT access disk bypassing Storage as other
  // data may not be protected by vaulting.
  // Always use OcStorageReadFileUnicode.
  //
  (VOID) Storage;

  //
  // For simplicity this example just calls reference boot picker,
  // however, a more advanced user interface should reimplement
  // OcRunSimpleBootPicker logic.
  //
  return OcRunSimpleBootPicker (Picker);
}

STATIC
OC_INTERFACE_PROTOCOL
mOcInterfaceProtocol = {
  OC_INTERFACE_REVISION,
  ExternalGuiRun
};

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *PrevInterface;
  EFI_HANDLE  NewHandle;

  //
  // Check for previous GUI protocols.
  //
  Status = gBS->LocateProtocol (
    &gOcInterfaceProtocolGuid,
    NULL,
    &PrevInterface
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCE: Another GUI is already present\n"));
    return EFI_ALREADY_STARTED;
  }

  //
  // Install new GUI protocol
  //
  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gOcInterfaceProtocolGuid,
    &mOcInterfaceProtocol,
    NULL
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCE: Registered custom GUI protocol\n"));
  } else {
    DEBUG ((DEBUG_ERROR, "OCE: Failed to install GUI protocol - %r\n", Status));
  }

  return Status;
}

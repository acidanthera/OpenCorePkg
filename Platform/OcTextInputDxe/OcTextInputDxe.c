/** @file
   OcTextInputDxe - Standalone SimpleTextInputEx Protocol Compatibility Driver

   This driver provides SimpleTextInputEx protocol compatibility for systems
   that only have SimpleTextInput protocol available (EFI 1.1 systems).

   This addresses compatibility issues with older systems (like cMP5,1) where
   CTRL key combinations may not work reliably in applications like the Shell
   text editor. The text editor has been enhanced with F10/ESC alternatives.

   This driver can be used for firmware injection outside of OpenCorePkg.

   Copyright (c) 2025, OpenCore Team. All rights reserved.
   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/OcTextInputLib.h>

/**
   The entry point for OcTextInputDxe driver.

   @param[in] ImageHandle    The firmware allocated handle for the EFI image.
   @param[in] SystemTable    A pointer to the EFI System Table.

   @retval EFI_SUCCESS       The entry point is executed successfully.
   @retval other             Some error occurs when executing this entry point.
 **/
EFI_STATUS
EFIAPI
OcTextInputDxeEntry (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "OcTextInputDxe: Driver entry point\n"));

  // Install SimpleTextInputEx compatibility protocol
  Status = OcInstallSimpleTextInputEx ();

  if (EFI_ERROR (Status)) {
    if (Status == EFI_ALREADY_STARTED) {
      DEBUG ((DEBUG_INFO, "OcTextInputDxe: SimpleTextInputEx already available\n"));
      Status = EFI_SUCCESS;
    } else {
      DEBUG ((DEBUG_ERROR, "OcTextInputDxe: Failed to install SimpleTextInputEx compatibility - %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OcTextInputDxe: Successfully installed SimpleTextInputEx compatibility\n"));
  }

  return Status;
}

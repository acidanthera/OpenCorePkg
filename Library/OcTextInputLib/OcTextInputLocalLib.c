/** @file
  OcTextInputLocalLib - SimpleTextInputEx Protocol Compatibility (Local Variant)
  
  This is the "local" variant for use by OpenShell, which uses constructor-based
  approach and OcRegisterBootServicesProtocol instead of standard gBS methods.
  
  Copyright (c) 2025, OpenCore Team. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Protocol/SimpleTextInEx.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/OcTextInputLib.h>

/**
  Install SimpleTextInputEx compatibility protocol using local registration.
  
  This function uses OcRegisterBootServicesProtocol instead of standard gBS
  methods, making it suitable for use within OpenShell.
  
  @retval EFI_SUCCESS          Protocol installed successfully or already present
  @retval EFI_ALREADY_STARTED  Protocol already exists, no action taken
  @retval Others               Installation failed
**/
EFI_STATUS
EFIAPI
OcInstallSimpleTextInputExLocal (
  VOID
  )
{
  // Call the internal implementation with local registration flag
  return OcInstallSimpleTextInputExInternal (TRUE);
}

/**
  Constructor for OcTextInputLocalLib.
  
  Automatically installs SimpleTextInputEx compatibility protocol when
  the library is loaded (constructor-based approach).
  
  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor succeeded.
**/
EFI_STATUS
EFIAPI
OcTextInputLocalLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  
  DEBUG ((DEBUG_INFO, "OcTextInputLocalLib: Constructor called\n"));
  
  // Automatically install SimpleTextInputEx compatibility
  Status = OcInstallSimpleTextInputExLocal ();
  
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((DEBUG_WARN, "OcTextInputLocalLib: Failed to install compatibility in constructor - %r\n", Status));
  }
  
  // Don't fail library loading even if protocol installation fails
  return EFI_SUCCESS;
}

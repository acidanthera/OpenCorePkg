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
#include <Library/OcTextInputLib.h>

#include "OcTextInputLibInternal.h"

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
  // Automatically install SimpleTextInputEx compatibility using local registration
  OcInstallSimpleTextInputExInternal (TRUE);

  // Don't fail library loading even if protocol installation fails
  return EFI_SUCCESS;
}

/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

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
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/AppleFirmwarePassword.h>

#include "AppleFwPasswordInternal.h"

// AppleFirmwarePasswordCheck
///
///
/// @param[in] This             This protocol.
/// @param[in] Arg1
///
/// @retval EFI_SUCCESS         The log was saved successfully.
/// @retval other

EFI_STATUS
EFIAPI
AppleFirmwarePasswordCheck (
  IN  APPLE_FIRMWARE_PASSWORD_PROTOCOL *This,
  IN OUT UINTN *Arg1
  )
{
  return EFI_SUCCESS;
}

// OcFirmwarePasswordInstallProtocol
/// Install the Apple Firmware Password protocol
///
/// @param[in] ImageHandle      The firmware allocated handle for the EFI image.
/// @param[in] SystemTable      A pointer to the EFI System Table.
///
/// @retval EFI_SUCCESS         The entry point is executed successfully.

EFI_STATUS
OcFirmwarePasswordInstallProtocol (
  IN  EFI_HANDLE ImageHandle,
  IN  EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA *Private = NULL;

  Private = (APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA *)AllocateZeroPool (sizeof(APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA));

  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA_SIGNATURE;
  Private->AppleFirmwarePassword.Check = AppleFirmwarePasswordCheck;

  // Install our protocol with dupe check enabled.

  Status = gBS->InstallMultipleProtocolInterfaces (
    &ImageHandle,
    &gAppleFirmwarePasswordProtocolGuid,
    &Private->AppleFirmwarePassword,
    NULL
    );
  if (EFI_ERROR(Status)) {
    FreePool (Private);
  }

  return Status;
}

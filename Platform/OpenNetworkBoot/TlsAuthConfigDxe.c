/** @file
  The DriverEntryPoint for TlsAuthConfigDxe driver.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TlsAuthConfigImpl.h"

/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
TlsAuthConfigDxeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                    Status;
  TLS_AUTH_CONFIG_PRIVATE_DATA  *PrivateData;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **)&PrivateData
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (PrivateData->Signature == TLS_AUTH_CONFIG_PRIVATE_DATA_SIGNATURE);

  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEfiCallerIdGuid,
         PrivateData,
         NULL
         );

  TlsAuthConfigFormUnload (PrivateData);

  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
TlsAuthConfigDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  TLS_AUTH_CONFIG_PRIVATE_DATA  *PrivateData;

  PrivateData = NULL;

  //
  // If already started, return.
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  NULL,
                  ImageHandle,
                  ImageHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Initialize the private data structure.
  //
  PrivateData = AllocateZeroPool (sizeof (TLS_AUTH_CONFIG_PRIVATE_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the HII configuration form.
  //
  Status = TlsAuthConfigFormInit (PrivateData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install private GUID.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  TlsAuthConfigFormUnload (PrivateData);

  return Status;
}

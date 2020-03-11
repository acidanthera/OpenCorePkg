/** @file
  Provides a service to retrieve a pointer to the EFI Boot Services Table.
  Allows overrides to UEFI protocols.
  Only available to DXE and UEFI module types.

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef OC_BOOT_SERVICES_TABLE_LIB_H
#define OC_BOOT_SERVICES_TABLE_LIB_H

#include <Library/UefiBootServicesTableLib.h>

/**
  Provide protocol instance through gBS->LocateProtocol.

  @param[in]   ProtocolGuid       Pointer to protocol GUID.
  @param[in]   ProtocolInstance   Pointer to protocol instance.
  @param[in]   Override           Override protocol instead of fallback when missing.

  @retval  EFI_SUCCESS on successful registration.
  @retval  EFI_OUT_OF_RESOURCES when no free slots are available.
**/
EFI_STATUS
OcRegisterBootServicesProtocol (
  IN EFI_GUID   *ProtocolGuid,
  IN VOID       *ProtocolInstance,
  IN BOOLEAN    Override
  );

#endif // OC_BOOT_SERVICES_TABLE_LIB_H

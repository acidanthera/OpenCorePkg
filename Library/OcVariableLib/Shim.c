/** @file
  Manage variables for GRUB2 shim.

  Copyright (C) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <ShimVars.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_STATUS
OcShimRetainProtocol (
  IN BOOLEAN  Required
  )
{
  UINT8  ShimRetainProtocol;

  if (!Required) {
    return EFI_SUCCESS;
  }

  ShimRetainProtocol = 1;

  return gRT->SetVariable (
                SHIM_RETAIN_PROTOCOL,
                &gShimLockGuid,
                EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (ShimRetainProtocol),
                &ShimRetainProtocol
                );
}

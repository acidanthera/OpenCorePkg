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

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Macros.h>

/**
  @param[in] Protocol    The published unique identifier of the protocol. It is the caller’s responsibility to pass in
                         a valid GUID.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
UninstallAllProtocolInstances (
  EFI_GUID  *Protocol
  )
{
  EFI_STATUS      Status;
  EFI_HANDLE      *Handles;
  UINTN           Index;
  UINTN           NoHandles;
  VOID            *OriginalProto;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    Protocol,
    NULL,
    &NoHandles,
    &Handles
    );

  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < NoHandles; ++Index) {
    Status = gBS->HandleProtocol (
      Handles[Index],
      Protocol,
      &OriginalProto
      );

    if (EFI_ERROR (Status)) {
      break;
    }

    Status = gBS->UninstallProtocolInterface (
      Handles[Index],
      Protocol,
      OriginalProto
      );

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  gBS->FreePool (Handles);

  return Status;
}

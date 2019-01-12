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
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Macros.h>

// SafeHandleProtocol
/**

  @param[in] Handle      The handle being queried. If Handle is not a valid EFI_HANDLE, then EFI_INVALID_PARAMETER is
                         returned.
  @param[in] Protocol    The published unique identifier of the protocol. It is the caller’s responsibility to pass in
                         a valid GUID.
  @param[out] Interface  Supplies the address where a pointer to the corresponding Protocol Interface is returned. 
                         NULL will be returned if a structure is not associated with Protocol.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
SafeHandleProtocol (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  EFI_STATUS Status;

  Status = gBS->HandleProtocol (Handle, Protocol, Interface);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Problem Locating %g on Handle %X - %r\n", Protocol, Handle, Status));
  }

  return Status;
}

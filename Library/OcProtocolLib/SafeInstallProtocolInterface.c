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

// SafeInstallProtocolInterface
/**

  @param[in] ProtocolGuid  The numeric ID of the protocol interface. It is the caller’s responsibility to pass in a
                           valid GUID.
  @param[in] Interface     A pointer to the protocol interface. The Interface must adhere to the structure defined by
                           Protocol.
                           NULL can be used if a structure is not associated with Protocol.
  @param[in] Handle        A pointer to the EFI_HANDLE on which the interface is to be installed. If *Handle is NULL on
                           input, a new handle is created and returned on output.
                           If *Handle is not NULL on input, the protocol is added to the handle, and the handle is
                           returned unmodified. 
                           If *Handle is not a valid handle, then EFI_INVALID_PARAMETER is returned.
  @param[in] DupeCheck     A boolean flag to check if protocol already exists.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
SafeInstallProtocolInterface (
  IN GUID        *ProtocolGuid,
  IN VOID        *Interface,
  IN EFI_HANDLE  Handle,
  IN BOOLEAN     DupeCheck
  )
{
  EFI_STATUS Status;

  VOID       *Protocol;

  Status = EFI_INVALID_PARAMETER;

  if ((ProtocolGuid != NULL) && (Interface != NULL)) {
    if (DupeCheck) {
      Status = gBS->LocateProtocol (ProtocolGuid, NULL, (VOID **)&Protocol);

      if (Status == EFI_SUCCESS) {
        Status = EFI_ALREADY_STARTED;

        DEBUG ((DEBUG_INFO, "Located %g (%X)\n", ProtocolGuid, *(UINT64 *)Protocol));
        goto Return;
      }
    }

    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    Interface
                    );

    DEBUG ((DEBUG_INFO, "Installed %g (%X)\n", ProtocolGuid, *(UINT64 *)Interface));
  }

Return:
  return Status;
}

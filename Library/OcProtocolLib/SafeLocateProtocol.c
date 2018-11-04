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

// SafeLocateProtocol
/**

  @param[in] Protocol      Provides the protocol to search for.
  @param[in] Registration  Optional registration key returned from RegisterProtocolNotify(). If Registration is NULL,
                           then it is ignored.
  @param[out] Interface    On return, a pointer to the first interface that matches Protocol and Registration.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
SafeLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (Protocol, NULL, Interface);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Problem Locating %g - %r\n", Protocol, Status));
  }

  if (*Interface == NULL) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

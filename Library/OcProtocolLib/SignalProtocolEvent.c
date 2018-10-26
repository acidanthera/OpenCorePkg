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

// SignalProtocolEvent
/**

  @param[in] ProtocolGuid  The published unique identifier of the protocol to publish.
**/
VOID
SignalProtocolEvent (
  IN EFI_GUID  *ProtocolGuid
  )
{
  EFI_HANDLE Handle;

  Handle = NULL;

  gBS->InstallProtocolInterface (
         &Handle,
         ProtocolGuid,
         EFI_NATIVE_INTERFACE,
         NULL
         );

  gBS->UninstallProtocolInterface (
         Handle,
         ProtocolGuid,
         NULL
         );
}

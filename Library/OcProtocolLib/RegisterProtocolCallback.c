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
#include <Library/OcPrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Macros.h>

// RegisterProtocolCallback
/**

  @param[in] Protocol        The numeric ID of the protocol for which the event is to be registered.
  @param[in] NotifyFunction  Pointer to the event’s notification function, if any.
  @param[in] NotifyContext   Pointer to the notification function’s context; corresponds to parameter Context in the
                             notification function.
  @param[in] Event           Event that is to be signaled whenever a protocol interface is registered for Protocol.
                             The same EFI_EVENT may be used for multiple protocol notify registrations.
  @param[in] Registration    A pointer to a memory location to receive the registration value.
                             This value must be saved and used by the notification function of Event to retrieve the
                             list of handles that have added a protocol interface of type Protocol.

  @retval EFI_SUCCESS  The event Callback was created successfully.
**/
EFI_STATUS
EFIAPI
RegisterProtocolCallback (
  IN  EFI_GUID          *Protocol,
  IN  EFI_EVENT_NOTIFY  NotifyFunction,
  IN  VOID              *NotifyContext,
  OUT EFI_EVENT         *Event,
  OUT VOID              **Registration
  )
{
  EFI_STATUS Status;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  NotifyFunction,
                  NotifyContext,
                  Event
                  );

  if (!EFI_ERROR (Status)) {
    Status = gBS->RegisterProtocolNotify (
                    Protocol,
                    *Event,
                    Registration
                    );
  }

  return Status;
}

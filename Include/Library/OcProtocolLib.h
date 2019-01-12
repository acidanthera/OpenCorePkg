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

#ifndef OC_PROTOCOL_LIB_H_
#define OC_PROTOCOL_LIB_H_

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
  );

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
  );

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
  );

// SafeHandleProtocol
/**

  @param[in]  Handle     The handle being queried. If Handle is not a valid EFI_HANDLE, then EFI_INVALID_PARAMETER is returned.
  @param[in]  Protocol   The published unique identifier of the protocol. It is the caller’s responsibility to pass in a valid GUID. 
  @param[out] Interface  Supplies the address where a pointer to the corresponding Protocol Interface is returned. 
                         NULL will be returned if a structure is not associated with Protocol.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
SafeHandleProtocol (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  );

// SignalProtocolEvent
/**

  @param[in] ProtocolGuid  The published unique identifier of the protocol to publish.
**/
VOID
SignalProtocolEvent (
  IN EFI_GUID  *ProtocolGuid
  );

/**
  @param[in] Protocol    The published unique identifier of the protocol. It is the caller’s responsibility to pass in
                         a valid GUID.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
UninstallAllProtocolInstances (
  EFI_GUID  *Protocol
  );

#endif // OC_PROTOCOL_LIB_H_

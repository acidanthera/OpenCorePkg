/** @file
  This file implements interaction with HECI.

  Copyright (c) 2019, vit9696. All rights reserved.<BR>
  Portions copyright (c) 2019, savvas. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/OcHeciLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Heci.h>
#include <Protocol/Heci2.h>
#include <IndustryStandard/AppleProvisioning.h>
#include <IndustryStandard/HeciMsg.h>
#include <IndustryStandard/HeciClientMsg.h>

STATIC UINT8 mCurrentMeClientRequestedReceiveMsg;
STATIC UINT8 mCurrentMeClientCanReceiveMsg;
STATIC UINT8 mCurrentMeClientAddress;

STATIC EFI_HECI_PROTOCOL *mHeci;
STATIC EFI_HECI2_PROTOCOL *mHeci2;
STATIC BOOLEAN mSendingHeciCommand;
STATIC BOOLEAN mSendingHeciCommandPerClient;

EFI_STATUS
HeciReadMessage (
  IN      UINT32           Blocking,
  IN      UINT32           *MessageBody,
  IN OUT  UINT32           *Length
  )
{
  if (mHeci != NULL) {
    return mHeci->ReadMsg (
      Blocking,
      MessageBody,
      Length
      );
  }

  if (mHeci2 != NULL) {
    return mHeci2->ReadMsg (
      HECI_DEFAULT_DEVICE,
      Blocking,
      MessageBody,
      Length
      );
  }

  DEBUG ((DEBUG_INFO, "OCME: No ME protocol loaded, cannot read message\n"));
  return EFI_NOT_FOUND;
}

EFI_STATUS
HeciSendMessage (
  IN      UINT32           *Message,
  IN      UINT32           Length,
  IN      UINT8            HostAddress,
  IN      UINT8            MEAddress
  )
{
  if (mHeci != NULL) {
    return mHeci->SendMsg (
      Message,
      Length,
      HostAddress,
      MEAddress
      );
  }

  if (mHeci2 != NULL) {
    return mHeci2->SendMsg (
      HECI_DEFAULT_DEVICE,
      Message,
      Length,
      HostAddress,
      MEAddress
      );
  }

  DEBUG ((DEBUG_INFO, "OCME: No ME protocol loaded, cannot send message\n"));
  return EFI_NOT_FOUND;
}

EFI_STATUS
HeciLocateProtocol (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mHeci != NULL || mHeci2 != NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (
    &gEfiHeciProtocolGuid,
    NULL,
    (VOID **) &mHeci
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCME: Falling back to HECI 2 protocol - %r\n", Status));

    //
    // Ensure we don't have both set
    //
    mHeci = NULL;

    Status = gBS->LocateProtocol (
      &gEfiHeci2ProtocolGuid,
      NULL,
      (VOID **) &mHeci2
      );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCME: Failed to find any HECI protocol - %r\n", Status));
  }

  return Status;
}

VOID
HeciUpdateReceiveMsgStatus (
  VOID
  )
{
  EFI_STATUS        Status;
  UINT32            Size;
  HBM_FLOW_CONTROL  Command;

  if (mSendingHeciCommandPerClient) {
    ZeroMem (&Command, sizeof (Command));
    Size = sizeof (Command);
    Status = HeciReadMessage (
      BLOCKING,
      (UINT32 *) &Command,
      &Size
      );

    if (!EFI_ERROR (Status) && Command.Command.Fields.Command == FLOW_CONTROL) {
      ++mCurrentMeClientCanReceiveMsg;
    }
  }
}

EFI_STATUS
HeciGetResponse (
  OUT VOID    *MessageData,
  IN  UINT32  ResponseSize
  )
{
  EFI_STATUS        Status;
  HBM_FLOW_CONTROL  Command;

  Status = EFI_NOT_READY;

  STATIC_ASSERT (sizeof (HBM_FLOW_CONTROL) == 8, "Invalid ME command size");

  if (mSendingHeciCommandPerClient || mSendingHeciCommand) {
    ZeroMem (MessageData, ResponseSize);

    //
    // Note, this was reworked to make more sense.
    // https://github.com/osy86/OpenCorePkg/commit/8f7188d41876109aec2fe3a721f69daf979dd268.diff
    //
    if (!mCurrentMeClientRequestedReceiveMsg) {
      ZeroMem (&Command, sizeof (Command));
      Command.Command.Fields.Command = FLOW_CONTROL;
      Command.MeAddress              = mCurrentMeClientAddress;
      Command.HostAddress            = HBM_CLIENT_ADDRESS;

      Status = HeciSendMessage (
        (UINT32 *) &Command,
        sizeof (Command),
        HBM_HOST_ADDRESS,
        HBM_ME_ADDRESS
        );

      if (!EFI_ERROR (Status)) {
        ++mCurrentMeClientRequestedReceiveMsg;
      }
    }

    Status = HeciReadMessage (
      BLOCKING,
      MessageData,
      &ResponseSize
      );

    if (!EFI_ERROR (Status)) {
      --mCurrentMeClientRequestedReceiveMsg;
    }
  }

  return Status;
}

EFI_STATUS
HeciSendMessageWithResponse (
  IN OUT VOID    *MessageData,
  IN     UINT32  RequestSize,
  IN     UINT32  ResponseSize
  )
{
  HECI_BUS_MESSAGE  *Message;
  HBM_COMMAND       Command;
  EFI_STATUS        Status;

  mSendingHeciCommand = TRUE;

  Message = (HECI_BUS_MESSAGE *) MessageData;
  Command = Message->Command;

  Status = HeciSendMessage (
    MessageData,
    RequestSize,
    HBM_HOST_ADDRESS,
    HBM_ME_ADDRESS
    );

  if (!EFI_ERROR (Status)) {
    Status = HeciGetResponse (MessageData, ResponseSize);

    if (!EFI_ERROR (Status)
      && Command.Fields.Command != Message->Command.Fields.Command) {
      Status = EFI_PROTOCOL_ERROR;
    }
  }

  mSendingHeciCommand = FALSE;

  return Status;
}

EFI_STATUS
HeciGetClientMap (
  OUT UINT8  *ClientMap,
  OUT UINT8  *ClientActiveCount
  )
{
  EFI_STATUS                   Status;
  HBM_HOST_ENUMERATION_BUFFER  Command;
  UINTN                        Index;
  UINTN                        Index2;
  UINT8                        *ValidAddressesPtr;
  UINT32                       ValidAddresses;

  *ClientActiveCount = 0;

  Status = HeciLocateProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  STATIC_ASSERT (sizeof (Command.Request)  == 4, "Invalid ME command size");
  STATIC_ASSERT (sizeof (Command.Response) == 36, "Invalid ME command size");

  ZeroMem (&Command, sizeof (Command));
  Command.Request.Command.Fields.Command = HOST_ENUMERATION_REQUEST;

  Status = HeciSendMessageWithResponse (
    &Command,
    sizeof (Command.Request),
    sizeof (Command.Response)
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ValidAddressesPtr = &Command.Response.ValidAddresses[0];
  for (Index = 0; Index < HBM_ME_CLIENT_MAX; Index += OC_CHAR_BIT) {
    ValidAddresses = *ValidAddressesPtr;

    for (Index2 = 0; Index2 < OC_CHAR_BIT; Index2++) {
      if ((ValidAddresses & (1U << Index2)) != 0) {
        ClientMap[*ClientActiveCount] = (UINT8) (Index + Index2);
        ++(*ClientActiveCount);
      }
    }

    ++ValidAddressesPtr;
  }

  return Status;
}

EFI_STATUS
HeciGetClientProperties (
  IN  UINT8                   Address,
  OUT HECI_CLIENT_PROPERTIES  *Properties
  )
{
  EFI_STATUS                         Status;
  HBM_HOST_CLIENT_PROPERTIES_BUFFER  Command;

  Status = HeciLocateProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  STATIC_ASSERT (sizeof (Command.Request)  == 4, "Invalid ME command size");
  STATIC_ASSERT (sizeof (Command.Response) == 28, "Invalid ME command size");

  ZeroMem (&Command, sizeof (Command));
  Command.Request.Command.Fields.Command = HOST_CLIENT_PROPERTIES_REQUEST;
  Command.Request.Address                = Address;

  Status = HeciSendMessageWithResponse (
    &Command,
    sizeof (Command.Request),
    sizeof (Command.Response)
    );

  CopyMem (
    Properties,
    &Command.Response.ClientProperties,
    sizeof (*Properties)
    );

  return Status;
}

EFI_STATUS
HeciConnectToClient (
  IN UINT8  Address
  )
{
  EFI_STATUS                 Status;
  HBM_CLIENT_CONNECT_BUFFER  Command;

  Status = HeciLocateProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (&Command, sizeof (Command));
  STATIC_ASSERT (sizeof (Command.Request) == 4, "Invalid ME command size");

  Command.Request.Command.Fields.Command = CLIENT_CONNECT_REQUEST;
  Command.Request.MeAddress              = Address;
  Command.Request.HostAddress            = HBM_CLIENT_ADDRESS;

  Status = HeciSendMessageWithResponse (
    &Command,
    sizeof (Command.Request),
    sizeof (Command.Response)
    );

  DEBUG ((DEBUG_INFO, "OCME: Connect to client %X code %d - %r\n", Address, Command.Response.Status, Status));

  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (Command.Response.Status) {
    case HBM_CLIENT_CONNECT_NOT_FOUND:
      return EFI_NOT_FOUND;
    case HBM_CLIENT_CONNECT_ALREADY_CONNECTED:
      return EFI_ALREADY_STARTED;
    case HBM_CLIENT_CONNECT_OUT_OF_RESOURCES:
      return EFI_OUT_OF_RESOURCES;
    case HBM_CLIENT_CONNECT_INVALID_PARAMETER:
      return EFI_INVALID_PARAMETER;
    default:
      mSendingHeciCommandPerClient        = TRUE;
      mCurrentMeClientRequestedReceiveMsg = 0;
      mCurrentMeClientCanReceiveMsg       = 0;
      mCurrentMeClientAddress             = Address;
      return EFI_SUCCESS;
  }
}

EFI_STATUS
HeciSendMessagePerClient (
  IN VOID    *Message,
  IN UINT32  Size
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mSendingHeciCommandPerClient)  {
    if (!mCurrentMeClientCanReceiveMsg) {
      HeciUpdateReceiveMsgStatus();
    }

    Status = HeciSendMessage (
      Message,
      Size,
      HBM_CLIENT_ADDRESS,
      mCurrentMeClientAddress
      );

    if (!EFI_ERROR (Status)) {
      --mCurrentMeClientCanReceiveMsg;
    }
  }

  return Status;
}

EFI_STATUS
HeciDisconnectFromClients (
  VOID
  )
{
  EFI_STATUS                    Status;
  HBM_CLIENT_DISCONNECT_BUFFER  Command;

  Status = EFI_SUCCESS;

  if (mSendingHeciCommandPerClient) {
    //
    // Note, this is different between HECI 1 and HECI 2.
    // HECI 1 has 4 byte response, and it does not require HeciUpdateReceiveMsgStatus.
    //
    STATIC_ASSERT (sizeof (Command.Request) == 4, "Invalid ME command req size");
    STATIC_ASSERT (sizeof (Command.Response) == 8, "Invalid ME command rsp size");

    if (!mCurrentMeClientCanReceiveMsg) {
      HeciUpdateReceiveMsgStatus();
    }

    ZeroMem (&Command, sizeof (Command));

    Command.Request.Command.Fields.Command = CLIENT_DISCONNECT_REQUEST;
    Command.Request.MeAddress              = mCurrentMeClientAddress;
    Command.Request.HostAddress            = HBM_CLIENT_ADDRESS;

    ++mCurrentMeClientRequestedReceiveMsg;

    Status = HeciSendMessageWithResponse (
      &Command,
      sizeof (Command.Request),
      sizeof (Command.Response)
      );

    DEBUG ((
      DEBUG_INFO,
      "OCME: Disconnect from client %X code %d - %r\n",
      mCurrentMeClientAddress,
      Command.Response.Status,
      Status
      ));

    if (!EFI_ERROR (Status)) {
      mSendingHeciCommandPerClient = FALSE;
    }
  }

  return Status;
}

EFI_STATUS
HeciPavpRequestProvisioning (
  OUT UINT32  *EpidStatus,
  OUT UINT32  *EpidGroupId
  )
{
  EFI_STATUS                        Status;
  ME_PAVP_PROVISION_REQUEST_BUFFER  Command;

  STATIC_ASSERT (sizeof (Command.Request) == 16, "Invalid ME command size");
  STATIC_ASSERT (sizeof (Command.Response) == 24, "Invalid ME command size");

  ZeroMem (&Command, sizeof (Command));
  Command.Request.Header.Version = ME_PAVP_PROTOCOL_VERSION;
  Command.Request.Header.Command = ME_PAVP_PROVISION_REQUEST_COMMAND;
  HeciSendMessagePerClient (&Command, sizeof (Command.Request));

  ZeroMem (&Command, sizeof (Command));
  Status = HeciGetResponse (&Command, sizeof (Command.Response));

  if (!EFI_ERROR (Status)) {
    *EpidStatus  = Command.Response.Status;
    *EpidGroupId = Command.Response.GroupId;
  }

  return Status;
}

EFI_STATUS
HeciPavpPerformProvisioning (
  IN  EPID_CERTIFICATE       *EpidCertificate,
  IN  EPID_GROUP_PUBLIC_KEY  *EpidGroupPublicKey,
  OUT BOOLEAN                *SetVar  OPTIONAL
  )
{
  EFI_STATUS                        Status;
  ME_PAVP_PROVISION_PERFORM_BUFFER  Command;
  UINTN                             Index;

  STATIC_ASSERT (sizeof (Command.Request) == 1284, "Invalid ME command size");
  STATIC_ASSERT (sizeof (Command.Response) == 16, "Invalid ME command size");

  if (SetVar != NULL) {
    *SetVar = FALSE;
  }

  ZeroMem (&Command, sizeof (Command));
  Command.Request.Header.Version     = ME_PAVP_PROTOCOL_VERSION;
  Command.Request.Header.Command     = ME_PAVP_PROVISION_PERFORM_COMMAND;
  Command.Request.Header.PayloadSize = ME_PAVP_PROVISION_PERFORM_PAYLOAD_SIZE;
  CopyMem (&Command.Request.Certificate, EpidCertificate, sizeof (Command.Request.Certificate));
  CopyMem (&Command.Request.PublicKey, EpidGroupPublicKey, sizeof (Command.Request.PublicKey));

  Status = HeciSendMessagePerClient (&Command, sizeof (Command.Request));
  ZeroMem (&Command, sizeof (Command));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCME: Failed to send provisioning command - %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < 3; ++Index) {
    Status = HeciGetResponse (&Command, sizeof (Command.Response));
    if (Status != EFI_TIMEOUT) {
      break;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "OCME: Finished provisioning command with status %x - %r\n",
    Command.Response.Header.Status,
    Status
    ));

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (Command.Response.Header.Status != EPID_STATUS_PROVISIONED) {
    Status = EFI_DEVICE_ERROR;
  }

  if (Command.Response.Header.Status == EPID_STATUS_FAIL_PROVISION) {
    if (SetVar != NULL) {
      *SetVar = TRUE;
    }
  }

  return Status;
}

EFI_STATUS
HeciFpfGetStatus (
  OUT  UINT32 *FpfStatus
  )
{
  EFI_STATUS  Status;
  UINT32      Response[11];
  UINT32      Request[4];

  ZeroMem (Request, sizeof (Request));
  Request[0] = 3;
  HeciSendMessagePerClient (Request, sizeof (Request));

  ZeroMem (Response, sizeof (Response));
  Status = HeciGetResponse (Response, sizeof (Response));

  if (!EFI_ERROR (Status)) {
    *FpfStatus = Response[1];
  }

  return Status;
}

EFI_STATUS
HeciFpfProvision (
  OUT  UINT32 *FpfStatus
  )
{
  EFI_STATUS Status;
  UINT32     Response[2];
  UINT32     Request[3];

  ZeroMem (Request, sizeof (Request));
  Request[0] = 5;
  Request[1] = 1;
  Request[2] = 255;
  HeciSendMessagePerClient (Request, sizeof (Request));

  ZeroMem (Response, sizeof (Response));
  Status = HeciGetResponse (Response, sizeof (Response));

  if (!EFI_ERROR (Status)) {
    *FpfStatus = Response[1];
  }

  return Status;
}

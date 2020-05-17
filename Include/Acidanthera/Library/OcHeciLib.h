/** @file
  This library implements interaction with HECI.

  Copyright (c) 2019, vit9696. All rights reserved.<BR>
  Portions copyright (c) 2019, savvas. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef OC_HECI_LIB_H
#define OC_HECI_LIB_H

#include <IndustryStandard/HeciMsg.h>
#include <IndustryStandard/HeciClientMsg.h>

EFI_STATUS
HeciReadMessage (
  IN      UINT32           Blocking,
  IN      UINT32           *MessageBody,
  IN OUT  UINT32           *Length
  );

EFI_STATUS
HeciSendMessage (
  IN      UINT32           *Message,
  IN      UINT32           Length,
  IN      UINT8            HostAddress,
  IN      UINT8            MEAddress
  );

EFI_STATUS
HeciLocateProtocol (
  VOID
  );

VOID
HeciUpdateReceiveMsgStatus (
  VOID
  );

EFI_STATUS
HeciGetResponse (
  OUT VOID    *MessageData,
  IN  UINT32  ResponseSize
  );

EFI_STATUS
HeciSendMessageWithResponse (
  IN OUT VOID    *MessageData,
  IN     UINT32  RequestSize,
  IN     UINT32  ResponseSize
  );

EFI_STATUS
HeciGetClientMap (
  OUT UINT8  *ClientMap,
  OUT UINT8  *ClientActiveCount
  );

EFI_STATUS
HeciGetClientProperties (
  IN  UINT8                   Address,
  OUT HECI_CLIENT_PROPERTIES  *Properties
  );

EFI_STATUS
HeciConnectToClient (
  IN UINT8  Address
  );

EFI_STATUS
HeciSendMessagePerClient (
  IN VOID    *Message,
  IN UINT32  Size
  );

EFI_STATUS
HeciDisconnectFromClients (
  VOID
  );

EFI_STATUS
HeciPavpRequestProvisioning (
  OUT UINT32  *EpidStatus,
  OUT UINT32  *EpidGroupId
  );

EFI_STATUS
HeciPavpPerformProvisioning (
  IN  EPID_CERTIFICATE       *EpidCertificate,
  IN  EPID_GROUP_PUBLIC_KEY  *EpidGroupPublicKey,
  OUT BOOLEAN                *SetVar  OPTIONAL
  );

EFI_STATUS
HeciFpfGetStatus (
  OUT  UINT32 *FpfStatus
  );

EFI_STATUS
HeciFpfProvision (
  OUT  UINT32 *FpfStatus
  );

#endif // OC_HECI_LIB_H

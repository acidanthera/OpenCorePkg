/** @file
  This protocol provides services for HECI communucation.
  See more details in https://github.com/intel/efiwrapper.

  Copyright (c) 2019, vit9696. All rights reserved.<BR>
  Portions copyright 1999 - 2017 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef EFI_HECI_H
#define EFI_HECI_H

#define EFI_HECI_PROTOCOL_GUID \
  { 0xCFB33810, 0x6E87, 0x4284, { 0xB2, 0x03, 0xA6, 0x6A, 0xbE, 0x07, 0xF6, 0xE8 } }

#define NON_BLOCKING                        0
#define BLOCKING                            1

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_SENDWACK) (
  IN OUT  UINT32           *Message,
  IN OUT  UINT32           Length,
  IN OUT  UINT32           *RecLength,
  IN      UINT8            HostAddress,
  IN      UINT8            MEAddress
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_READ_MESSAGE) (
  IN      UINT32           Blocking,
  IN      UINT32           *MessageBody,
  IN OUT  UINT32           *Length
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_SEND_MESSAGE) (
  IN      UINT32           *Message,
  IN      UINT32           Length,
  IN      UINT8            HostAddress,
  IN      UINT8            MEAddress
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_RESET) (
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_INIT) (
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_REINIT) (
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_RESET_WAIT) (
  IN  UINT32   Delay
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_GET_ME_STATUS) (
  OUT UINT32   *Status
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI_GET_ME_MODE) (
  OUT UINT32   *Mode
  );

typedef struct EFI_HECI_PROTOCOL_ {
  EFI_HECI_SENDWACK       SendwACK;
  EFI_HECI_READ_MESSAGE   ReadMsg;
  EFI_HECI_SEND_MESSAGE   SendMsg;
  EFI_HECI_RESET          ResetHeci;
  EFI_HECI_INIT           InitHeci;
  EFI_HECI_RESET_WAIT     MeResetWait;
  EFI_HECI_REINIT         ReInitHeci;
  EFI_HECI_GET_ME_STATUS  GetMeStatus;
  EFI_HECI_GET_ME_MODE    GetMeMode;
} EFI_HECI_PROTOCOL;

extern EFI_GUID gEfiHeciProtocolGuid;

#endif // EFI_HECI_H

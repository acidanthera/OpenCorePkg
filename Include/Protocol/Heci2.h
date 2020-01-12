/** @file
  This protocol provides services for HECI communucation.
  See more details in https://github.com/intel/efiwrapper.

  Copyright (c) 2019, vit9696. All rights reserved.<BR>
  Portions copyright 1999 - 2017 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef EFI_HECI2_H
#define EFI_HECI2_H

#define EFI_HECI2_PROTOCOL_GUID \
  { 0x3C7BC880, 0x41F8, 0x4869, { 0xAE, 0xFC, 0x87, 0x0A, 0x3E, 0xD2, 0x82, 0x99 } }

typedef UINT32 HECI2_DEVICE;
#define HECI_DEFAULT_DEVICE (0)

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_SENDWACK) (
  IN      HECI2_DEVICE     HeciDev,
  IN OUT  UINT32           *Message,
  IN OUT  UINT32           Length,
  IN OUT  UINT32           *RecLength,
  IN      UINT8            HostAddress,
  IN      UINT8            MEAddress
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_READ_MESSAGE) (
  IN      HECI2_DEVICE     HeciDev,
  IN      UINT32           Blocking,
  IN      UINT32           *MessageBody,
  IN OUT  UINT32           *Length
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_SEND_MESSAGE) (
  IN      HECI2_DEVICE     HeciDev,
  IN      UINT32           *Message,
  IN      UINT32           Length,
  IN      UINT8            HostAddress,
  IN      UINT8            MEAddress
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_RESET) (
  IN      HECI2_DEVICE     HeciDev
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_INIT) (
  IN      HECI2_DEVICE     HeciDev
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_REINIT) (
  IN      HECI2_DEVICE     HeciDev
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_RESET_WAIT) (
  IN      HECI2_DEVICE     HeciDev,
  IN      UINT32           Delay
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_GET_ME_STATUS) (
  OUT UINT32   *Status
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HECI2_GET_ME_MODE) (
  OUT UINT32   *Mode
  );

typedef struct EFI_HECI2_PROTOCOL_ {
  EFI_HECI2_SENDWACK       SendwACK;
  EFI_HECI2_READ_MESSAGE   ReadMsg;
  EFI_HECI2_SEND_MESSAGE   SendMsg;
  EFI_HECI2_RESET          ResetHeci;
  EFI_HECI2_INIT           InitHeci;
  EFI_HECI2_RESET_WAIT     MeResetWait;
  EFI_HECI2_REINIT         ReInitHeci;
  EFI_HECI2_GET_ME_STATUS  GetMeStatus;
  EFI_HECI2_GET_ME_MODE    GetMeMode;
} EFI_HECI2_PROTOCOL;

extern EFI_GUID gEfiHeci2ProtocolGuid;

#endif // EFI_HECI2_H

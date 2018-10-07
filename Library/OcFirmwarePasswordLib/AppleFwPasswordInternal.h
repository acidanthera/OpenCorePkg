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

#ifndef __APPLE_FIRMWARE_PASSWORD_INTERNAL_H
#define  __APPLE_FIRMWARE_PASSWORD_INTERNAL_H

#define  APPLE_FIRMWARE_PASSWORD_PROTOCOL_REVISION  0x10000

typedef struct APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA {

  UINTN Signature;
  APPLE_FIRMWARE_PASSWORD_PROTOCOL AppleFirmwarePassword;

} APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA;

#define APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('A', 'F', 'W', 'P')

#define APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA_FROM_APPLE_SMC_THIS(a) \
  CR(a, APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA, AppleFirmwarePassword, APPLE_FIRMWARE_PASSWORD_PRIVATE_DATA_SIGNATURE)

#endif /* __APPLE_FIRMWARE_PASSWORD_INTERNAL_H */


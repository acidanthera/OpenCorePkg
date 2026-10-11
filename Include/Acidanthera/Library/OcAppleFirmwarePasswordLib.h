/** @file
  Apple Firmware Password protocol implementation.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_APPLE_FIRMWARE_PASSWORD_LIB_H
#define OC_APPLE_FIRMWARE_PASSWORD_LIB_H

#include <Protocol/AppleFirmwarePassword.h>

/**
  Install and initialise Apple Firmware Password protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_FIRMWARE_PASSWORD_PROTOCOL *
OcAppleFirmwarePasswordInstallProtocol (
  VOID
  );

#endif // OC_APPLE_FIRMWARE_PASSWORD_LIB_H

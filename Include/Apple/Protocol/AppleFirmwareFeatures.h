/** @file
  Apple Firmware Features protocol.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_FIRMWARE_FEATURES_H
#define APPLE_FIRMWARE_FEATURES_H

/**
  Apple Firmware Features protocol GUID.
  998C6D8C-5500-4145-953E-B875517A7A34
**/
#define APPLE_FIRMWARE_FEATURES_PROTOCOL_GUID \
  { 0x998C6D8C, 0x5500, 0x4145,     \
    { 0x95, 0x3E, 0xB8, 0x75, 0x51, 0x7A, 0x7A, 0x34 }}

typedef struct APPLE_FIRMWARE_FEATURES_PROTOCOL_ APPLE_FIRMWARE_FEATURES_PROTOCOL;

/**
  Install Firmware Features variables.

  @param[in] This     A pointer to the protocol instance.

  @retval EFI_SUCCESS Firmware Features variables successfully installed.
  @retval Others      Any other status from gRT->SetVariable().
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_FIRMWARE_FEATURES_INSTALL_FEATURES)(
  IN APPLE_FIRMWARE_FEATURES_PROTOCOL *This
  );

/**
  Apple Firmware Features protocol instance.
**/
struct APPLE_FIRMWARE_FEATURES_PROTOCOL_ {
  APPLE_FIRMWARE_FEATURES_INSTALL_FEATURES    Install;
};

extern EFI_GUID  gAppleFirmwareFeaturesProtocolGuid;

#endif // APPLE_FIRMWARE_FEATURES_H

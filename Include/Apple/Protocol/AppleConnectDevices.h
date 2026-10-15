/** @file
  Apple Connect Devices protocol.

  Copyright (c) 2012, tiamo. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_CONNECT_DEVICES_H
#define APPLE_CONNECT_DEVICES_H

/**
  Apple Connect Devices protocol GUID.
  8ECE08D8-A6D4-430B-A7B0-2DF318E7884A
**/
#define APPLE_CONNECT_DEVICES_PROTOCOL_GUID  \
 { 0x8ECE08D8, 0xA6D4, 0x430B,   \
   { 0xA7, 0xB0, 0x2D, 0xF3, 0x18, 0xE7, 0x88, 0x4A}}

/**
  Apple Connect Devices protocol revision.
**/
#define APPLE_CONNECT_DEVICES_PROTOCOL_REVISION  1

typedef struct APPLE_CONNECT_DEVICES_PROTOCOL_ APPLE_CONNECT_DEVICES_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_CONNECT_DEVICES_CONNECT_CONSOLE)(
  IN APPLE_CONNECT_DEVICES_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_CONNECT_DEVICES_CONNECT_SPI_DEVICES)(
  IN APPLE_CONNECT_DEVICES_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_CONNECT_DEVICES_CONNECT_ALL)(
  IN APPLE_CONNECT_DEVICES_PROTOCOL *This
  );

/**
  Apple Connect Devices protocol instance.
**/
struct APPLE_CONNECT_DEVICES_PROTOCOL_ {
  UINTN                                        Revision;
  APPLE_CONNECT_DEVICES_CONNECT_CONSOLE        ConnectConsole;
  APPLE_CONNECT_DEVICES_CONNECT_SPI_DEVICES    ConnectSpiDevices;
  APPLE_CONNECT_DEVICES_CONNECT_ALL            ConnectAll;
};

extern EFI_GUID  gAppleConnectDevicesProtocolGuid;

#endif // APPLE_CONNECT_DEVICES_H

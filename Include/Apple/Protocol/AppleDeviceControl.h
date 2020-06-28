/** @file
Copyright (C) 2012, tiamo.  All rights reserved.<BR>
Copyright (C) 2019, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DEVICE_CONTROL_H
#define APPLE_DEVICE_CONTROL_H

///
/// The GUID of the APPLE_DEVICE_CONTROL_PROTOCOL.
///
#define APPLE_DEVICE_CONTROL_PROTOCOL_GUID  \
  { 0x8ECE08D8, 0xA6D4, 0x430B,          \
    { 0xA7, 0xB0, 0x2D, 0xF3, 0x18, 0xE7, 0x88, 0x4A } }

#define APPLE_DEVOCE_CONTROL_PROTOCOL_VERSION 1

typedef
EFI_STATUS
(EFIAPI *APPLE_DEVICE_CONTROL_CONNECT_DISPLAY) (
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_DEVICE_CONTROL_CONNECT_PCI) (
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_DEVICE_CONTROL_CONNECT_ALL) (
  VOID
  );

typedef struct APPLE_DEVICE_CONTROL_PROTOCOL_ {
  UINTN                                 Version;
  APPLE_DEVICE_CONTROL_CONNECT_DISPLAY  ConnectDisplay;
  APPLE_DEVICE_CONTROL_CONNECT_PCI      ConnectPci;
  APPLE_DEVICE_CONTROL_CONNECT_ALL      ConnectAll;
} APPLE_DEVICE_CONTROL_PROTOCOL;

extern EFI_GUID gAppleDeviceControlProtocolGuid;

#endif // APPLE_DEVICE_CONTROL_H

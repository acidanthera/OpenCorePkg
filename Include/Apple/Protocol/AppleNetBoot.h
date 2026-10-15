/** @file
  Apple Net Boot protocol.

  Copyright (c) 2012, tiamo. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_NET_BOOT_H
#define APPLE_NET_BOOT_H

/**
  Apple Net Boot protocol GUID.
  78EE99FB-6A5E-4186-97DE-CD0ABA345A74
**/
#define APPLE_NET_BOOT_PROTOCOL_GUID \
 { 0x78EE99FB, 0x6A5E, 0x4186,   \
    { 0x97, 0xDE, 0xCD, 0x0A, 0xBA, 0x34, 0x5A, 0x74 }}

typedef struct APPLE_NET_BOOT_PROTOCOL_ APPLE_NET_BOOT_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_NET_BOOT_GET_DHCP_RESPONSE)(
  APPLE_NET_BOOT_PROTOCOL *This,
  UINTN                   *BufferSize,
  VOID                    *DataBuffer
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_NET_BOOT_GET_BSDP_RESPONSE)(
  APPLE_NET_BOOT_PROTOCOL *This,
  UINTN                   *BufferSize,
  VOID                    *DataBuffer
  );

/**
  Apple Net Boot protocol instance.
**/
struct APPLE_NET_BOOT_PROTOCOL_ {
  APPLE_NET_BOOT_GET_DHCP_RESPONSE    GetDhcpResponse;
  APPLE_NET_BOOT_GET_BSDP_RESPONSE    GetBsdpResponse;
};

extern EFI_GUID  gAppleNetBootProtocolGuid;

#endif // APPLE_NET_BOOT_H

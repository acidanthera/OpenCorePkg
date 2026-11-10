/** @file
  Apple UART protocol.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_UART_H
#define APPLE_UART_H

#include <Protocol/SerialIo.h>

/**
  Apple UART protocol GUID.
  099D50C2-881E-4DF0-AE2A-3BEDD13EBC70
**/
#define APPLE_UART_PROTOCOL_GUID  \
  { 0x099D50C2, 0x881E, 0x4DF0,     \
     { 0xAE, 0x2A, 0x3B, 0xED, 0xD1, 0x3E, 0xBC, 0x70 }}

typedef struct APPLE_UART_PROTOCOL_ APPLE_UART_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_UART_RESET)(
  IN APPLE_UART_PROTOCOL  *This
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_UART_SET_ATTRIBUTES)(
  IN APPLE_UART_PROTOCOL            *This,
  IN UINT64                         BaudRate,
  IN UINT32                         ReceiveFifoDepth,
  IN UINT32                         Timeout,
  IN EFI_PARITY_TYPE                Parity,
  IN UINT8                          DataBits,
  IN EFI_STOP_BITS_TYPE             StopBits
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_UART_SET_CONTROL_BITS)(
  IN APPLE_UART_PROTOCOL            *This,
  IN UINT32                         Control
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_UART_GET_CONTROL_BITS)(
  IN  APPLE_UART_PROTOCOL         *This,
  OUT UINT32                      *Control
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_UART_WRITE)(
  IN APPLE_UART_PROTOCOL            *This,
  IN OUT UINTN                      *BufferSize,
  IN VOID                           *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_UART_READ)(
  IN APPLE_UART_PROTOCOL            *This,
  IN OUT UINTN                      *BufferSize,
  OUT VOID                          *Buffer
  );

/**
  Apple UART protocol instance.
**/
struct APPLE_UART_PROTOCOL_ {
  UINTN                          Reserved;
  APPLE_UART_RESET               Reset;
  APPLE_UART_SET_ATTRIBUTES      SetAttributes;
  APPLE_UART_SET_CONTROL_BITS    SetControl;
  APPLE_UART_GET_CONTROL_BITS    GetControl;
  APPLE_UART_WRITE               Write;
  APPLE_UART_READ                Read;
  EFI_SERIAL_IO_MODE             *Mode;
};

extern EFI_GUID  gAppleUartProtocolGuid;

#endif // APPLE_UART_H

/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef EFI_KEYBOARD_INFORMATION_H
#define EFI_KEYBOARD_INFORMATION_H

// EFI_KEYBOARD_INFO_PROTOCOL_GUID
#define EFI_KEYBOARD_INFO_PROTOCOL_GUID                   \
  { 0xE82A0A1E, 0x0E4D, 0x45AC,                           \
    { 0xA6, 0xDC, 0x2A, 0xE0, 0x58, 0x00, 0xD3, 0x11 } }

// KEYBOARD_INFO_GET_INFO
typedef
EFI_STATUS
(EFIAPI *KEYBOARD_INFO_GET_INFO)(
  OUT UINT16  *IdVendor,
  OUT UINT16  *IdProduct,
  OUT UINT8   *CountryCode
  );

// EFI_KEYBOARD_INFO_PROTOCOL
typedef struct {
  KEYBOARD_INFO_GET_INFO GetInfo;
} EFI_KEYBOARD_INFO_PROTOCOL;

// gEfiKeyboardInfoProtocolGuid
extern EFI_GUID gEfiKeyboardInfoProtocolGuid;

#endif // EFI_KEYBOARD_INFORMATION_H

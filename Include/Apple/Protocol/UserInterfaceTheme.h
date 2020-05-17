/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef USER_INTERFACE_THEME_H
#define USER_INTERFACE_THEME_H

// USER_THEME_INTERFACE_PROTOCOL_REVISION
#define USER_THEME_INTERFACE_PROTOCOL_REVISION  0x00000001

// EFI_USER_INTERFACE_THEME_PROTOCOL_GUID
#define EFI_USER_INTERFACE_THEME_PROTOCOL_GUID  \
  { 0xD5B0AC65, 0x9A2D, 0x4D2A,                 \
    { 0xBB, 0xD6, 0xE8, 0x71, 0xA9, 0x5E, 0x04, 0x35 } }

#define APPLE_COLOR_LIGHT_GRAY   0x00BFBFBFU
#define APPLE_COLOR_SYRAH_BLACK  0x00000000U

// UI_THEME_GET_BACKGROUND_COLOR
typedef
EFI_STATUS
(EFIAPI *UI_THEME_GET_BACKGROUND_COLOR)(
  IN OUT UINT32  *BackgroundColor
  );

// EFI_USER_INTERFACE_THEME_PROTOCOL
typedef struct {
  UINTN                         Revision;            ///< Revision.
  UI_THEME_GET_BACKGROUND_COLOR GetBackgroundColor;  ///< Present as of Revision 1.
} EFI_USER_INTERFACE_THEME_PROTOCOL;

// gEfiUserInterfaceThemeProtocolGuid
extern EFI_GUID gEfiUserInterfaceThemeProtocolGuid;

#endif // USER_INTERFACE_THEME_H
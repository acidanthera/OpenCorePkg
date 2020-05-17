/** @file
  Apple LoadImage protocol.

Copyright (C) 2018, savvas.  All rights reserved.<BR>
Copyright (C) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_LOAD_IMAGE_PROTOCOL_H
#define APPLE_LOAD_IMAGE_PROTOCOL_H

//
// 6C6148A4-97B8-429C-955E-4103E8ACA0FA
//
#define APPLE_LOAD_IMAGE_PROTOCOL_GUID \
  { 0x6C6148A4, 0x97B8, 0x429C,        \
    { 0x95, 0x5E, 0x41, 0x03, 0xE8, 0xAC, 0xA0, 0xFA } }

//
// Should return TRUE to perform image signature verification.
//
typedef
BOOLEAN
(EFIAPI *APPLE_LOAD_IMAGE_CALLBACK) (
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_LOAD_IMAGE) (
  IN  BOOLEAN                    BootPolicy,
  IN  EFI_HANDLE                 ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN  VOID                       *SourceBuffer OPTIONAL,
  IN  UINTN                      SourceSize,
  OUT EFI_HANDLE                 *ImageHandle,
  IN  APPLE_LOAD_IMAGE_CALLBACK  Callback OPTIONAL
);

typedef struct {
  APPLE_LOAD_IMAGE  LoadImage;
} APPLE_LOAD_IMAGE_PROTOCOL;

extern EFI_GUID gAppleLoadImageProtocolGuid;

#endif // APPLE_LOAD_IMAGE_PROTOCOL_H

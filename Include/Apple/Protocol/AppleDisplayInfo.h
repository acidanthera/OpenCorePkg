/** @file
Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DISPLAY_INFO_H
#define APPLE_DISPLAY_INFO_H

///
/// The GUID of the APPLE_DISPLAY_INFO_PROTOCOL_GUID.
/// 63FAECF2-E7EE-4CB9-8A0C-11CE5E89E33C
///
#define APPLE_DISPLAY_INFO_PROTOCOL_GUID      \
  { 0x63FAECF2, 0xE7EE, 0x4CB9,               \
    { 0x8A, 0x0C, 0x11, 0xCE, 0x5E, 0x89, 0xE3, 0x3C } }

#define APPLE_DISPLAY_INFO_PROTOCOL_REVISION 0x10002

typedef struct APPLE_DISPLAY_INFO_PROTOCOL_ APPLE_DISPLAY_INFO_PROTOCOL;

enum {
  AppleDisplayRotate0   = 0,
  AppleDisplayRotate90  = 1,
  AppleDisplayRotate180 = 2,
  AppleDisplayRotate270 = 3
};

typedef
EFI_STATUS
(EFIAPI *APPLE_DISPLAY_INFO_GET_ROTATION) (
  IN   APPLE_DISPLAY_INFO_PROTOCOL      *This,
  OUT  UINT32                           *Rotation
  );

struct APPLE_DISPLAY_INFO_PROTOCOL_ {
  UINT32                              Revision;
  UINTN                               Unknown[5];
  APPLE_DISPLAY_INFO_GET_ROTATION     GetRotation;
};

extern EFI_GUID gAppleDisplayInfoProtocolGuid;

#endif // APPLE_DISPLAY_INFO_H

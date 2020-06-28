/** @file
Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FRAMEBUFFER_INFO_H
#define APPLE_FRAMEBUFFER_INFO_H

///
/// The GUID of the APPLE_FRAMEBUFFER_INFO_PROTOCOL.
///
#define APPLE_FRAMEBUFFER_INFO_PROTOCOL_GUID  \
  { 0xE316E100, 0x0751, 0x4C49,               \
    { 0x90, 0x56, 0x48, 0x6C, 0x7E, 0x47, 0x29, 0x03 } }

typedef struct APPLE_FRAMEBUFFER_INFO_PROTOCOL_ APPLE_FRAMEBUFFER_INFO_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_FRAMEBUFFER_INFO_GET_INFO) (
  IN   APPLE_FRAMEBUFFER_INFO_PROTOCOL  *This,
  OUT  EFI_PHYSICAL_ADDRESS             *FramebufferBase,
  OUT  UINT32                           *FramebufferSize,
  OUT  UINT32                           *ScreenRowBytes,
  OUT  UINT32                           *ScreenWidth,
  OUT  UINT32                           *ScreenHeight,
  OUT  UINT32                           *ScreenDepth
  );

struct APPLE_FRAMEBUFFER_INFO_PROTOCOL_ {
  APPLE_FRAMEBUFFER_INFO_GET_INFO     GetInfo;
};

extern EFI_GUID gAppleFramebufferInfoProtocolGuid;

#endif // APPLE_FRAMEBUFFER_INFO_H

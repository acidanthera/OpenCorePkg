/** @file
  Copyright (C) 2020, Goldfish64. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_VBIOS_PATCH_PROTOCOL_H
#define OC_VBIOS_PATCH_PROTOCOL_H

#include <Uefi.h>

//
// OC_VBIOS_PATCH_PROTOCOL_GUID
// BC7EC589-2390-4DA3-8025-77DAD34F3609
//
#define OC_VBIOS_PATCH_PROTOCOL_GUID  \
  { 0xBC7EC589, 0x2390, 0x4DA3, \
    { 0x80, 0x25, 0x77, 0xDA, 0xD3, 0x4F, 0x36, 0x09 } }

typedef struct OC_VBIOS_PATCH_PROTOCOL_ OC_VBIOS_PATCH_PROTOCOL;

/**
  Patch legacy VBIOS for specified resolution and reconnect the controller.

  @param[in,out] This         Protocol instance.
  @param[in]     ScreenY      Desired screen width.
  @param[in]     ScreenX      Desired screen height.

  @retval EFI_SUCCESS on success.
**/
typedef
EFI_STATUS
(EFIAPI* OC_VBIOS_PATCH_SET_RESOLUTION) (
  IN OUT OC_VBIOS_PATCH_PROTOCOL      *This,
  IN     UINT16                       ScreenX,
  IN     UINT16                       ScreenY
  );

struct OC_VBIOS_PATCH_PROTOCOL_ {
  OC_VBIOS_PATCH_SET_RESOLUTION     SetResolution;
};

extern EFI_GUID gOcVbiosPatchProtocolGuid;

#endif

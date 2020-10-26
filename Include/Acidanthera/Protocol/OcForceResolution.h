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

#ifndef OC_FORCE_RESOLUTION_PROTOCOL_H
#define OC_FORCE_RESOLUTION_PROTOCOL_H

#include <Uefi.h>

//
// OC_FORCE_RESOLUTION_PROTOCOL_GUID
// BC7EC589-2390-4DA3-8025-77DAD34F3609
//
#define OC_FORCE_RESOLUTION_PROTOCOL_GUID  \
  { 0xBC7EC589, 0x2390, 0x4DA3, \
    { 0x80, 0x25, 0x77, 0xDA, 0xD3, 0x4F, 0x36, 0x09 } }

typedef struct OC_FORCE_RESOLUTION_PROTOCOL_ OC_FORCE_RESOLUTION_PROTOCOL;

/**
  Force the specified resolution and reconnect the controller.
  Specifying zero for Width and Height will pull the maximum
  supported resolution by the EDID instead.

  @param[in,out] This         Protocol instance.
  @param[in]     Width        Desired screen width.
  @param[in]     Height       Desired screen height.

  @retval EFI_SUCCESS         Resolution successfully forced.
  @retval EFI_UNSUPPORTED     The video adapter or the display are not supported.
  @retval EFI_ALREADY_STARTED The specified resolution is already supported.
**/
typedef
EFI_STATUS
(EFIAPI* OC_FORCE_RESOLUTION_SET_RESOLUTION) (
  IN OUT OC_FORCE_RESOLUTION_PROTOCOL *This,
  IN     UINT32                       Width   OPTIONAL,
  IN     UINT32                       Height  OPTIONAL
  );

struct OC_FORCE_RESOLUTION_PROTOCOL_ {
  OC_FORCE_RESOLUTION_SET_RESOLUTION  SetResolution;
};

extern EFI_GUID gOcForceResolutionProtocolGuid;

#endif

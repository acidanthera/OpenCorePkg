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

#ifndef EDID_H
#define EDID_H

#include <Uefi.h>

#pragma pack(1)

typedef struct {
  UINT16  PixelClock;

  UINT8   HorzActivePixelsLsb;
  UINT16  HorzBlankPixels         : 12;
  UINT16  HorzActivePixelsMsb     : 4;

  UINT8   VertActivePixelsLsb;
  UINT16  VertBlankPixels         : 12;
  UINT16  VertActivePixelsMsb     : 4;

  UINT8   HorzSyncOffsetLsb;
  UINT8   HorzSyncPulseWidthLsb;

  UINT16  VertSyncPulseWidthLsb   : 4;
  UINT16  VertSyncOffsetLsb       : 4;
  
  UINT16  VertSyncPulseWidthMsb   : 2;
  UINT16  VertSyncOffsetMsb       : 2;
  UINT16  HorzSyncPulseWidthMsb   : 2;
  UINT16  HorzSyncOffsetMsb       : 2;

  UINT8   HorzImageSizeLsb;
  UINT16  VertImageSize           : 12;
  UINT16  HorzImageSizeMsb        : 4;

  UINT8   HorzBorderPixels;
  UINT8   VertBorderPixels;

  UINT8   Features;
} EDID_DTD;

#pragma pack()

#endif

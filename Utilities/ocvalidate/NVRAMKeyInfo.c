/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "NvramKeyInfo.h"

STATIC
BOOLEAN
ValidateUIScale (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  UINTN               Index;
  CONST UINT8         *UIScaleValue;
  STATIC CONST UINT8  AllowedUIScaleValue[] = {
    0x01,
    0x02
  };

  UIScaleValue = (CONST UINT8 *) Value;

  if (ValueSize != sizeof (AllowedUIScaleValue[0])) {
    return FALSE;
  }

  for (Index = 0; Index < ARRAY_SIZE (AllowedUIScaleValue); ++Index) {
    if (*UIScaleValue == AllowedUIScaleValue[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
BOOLEAN
ValidateNvdaDrv (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  UINTN               Index;
  CONST UINT8         *NvdaDrvValue;
  STATIC CONST UINT8  AllowedNvdaDrvValue[] = {
    0x31
  };

  NvdaDrvValue = (CONST UINT8 *) Value;

  if (ValueSize != sizeof (AllowedNvdaDrvValue[0])) {
    return FALSE;
  }

  for (Index = 0; Index < ARRAY_SIZE (AllowedNvdaDrvValue); ++Index) {
    if (*NvdaDrvValue == AllowedNvdaDrvValue[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC NVRAM_KEY_MAP  mAppleVendorNvramGuidKeyMaps[] = {
  { "UIScale",  ValidateUIScale },
};

STATIC NVRAM_KEY_MAP  mAppleBootVariableGuidKeyMaps[] = {
  { "nvda_drv", ValidateNvdaDrv },
};

NVRAM_GUID_MAP mGUIDMaps[] = {
  { &gAppleVendorNvramGuid,  &mAppleVendorNvramGuidKeyMaps[0],  ARRAY_SIZE (mAppleVendorNvramGuidKeyMaps) },
  { &gAppleBootVariableGuid, &mAppleBootVariableGuidKeyMaps[0], ARRAY_SIZE (mAppleBootVariableGuidKeyMaps) }
};
UINTN mGUIDMapsCount = ARRAY_SIZE (mGUIDMaps);

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

#include "NVRAMKeyInfo.h"

STATIC
BOOLEAN
ValidateUIScale (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  UINTN        Index;
  CONST UINT8  *UIScaleValue;
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
  UINTN        Index;
  CONST UINT8  *NvdaDrvValue;
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

STATIC
BOOLEAN
CheckNVRAMKeyValueByMap (
  IN  CONST NVRAM_KEY_MAP  *NVRAMKeyMap,
  IN  UINTN                NVRAMKeyMapCount,
  IN  CONST CHAR8          *KeyName,
  IN  CONST VOID           *KeyValue,
  IN  UINT32               ValueSize
  )
{
  UINTN  Index;

  for (Index = 0; Index < NVRAMKeyMapCount; ++Index) {
    if (AsciiStrCmp (KeyName, NVRAMKeyMap[Index].KeyName) == 0) {
      return NVRAMKeyMap[Index].KeyChecker (KeyValue, ValueSize);
    }
  }

  return TRUE;
}

STATIC
BOOLEAN
CheckAppleVendorNvramGuid (
  IN  CONST CHAR8  *KeyName,
  IN  CONST VOID   *KeyValue,
  IN  UINT32       ValueSize
  )
{
  STATIC NVRAM_KEY_MAP  mNVRAMKeyMaps[] = {
    { "UIScale",  ValidateUIScale },
  };

  return CheckNVRAMKeyValueByMap (
           &mNVRAMKeyMaps[0],
           ARRAY_SIZE (mNVRAMKeyMaps),
           KeyName,
           KeyValue,
           ValueSize
           );
}

STATIC
BOOLEAN
CheckAppleBootVariableGuid (
  IN  CONST CHAR8  *KeyName,
  IN  CONST VOID   *KeyValue,
  IN  UINT32       ValueSize
  )
{
  STATIC NVRAM_KEY_MAP  mNVRAMKeyMaps[] = {
    { "nvda_drv", ValidateNvdaDrv },
  };

  return CheckNVRAMKeyValueByMap (
           &mNVRAMKeyMaps[0],
           ARRAY_SIZE (mNVRAMKeyMaps),
           KeyName,
           KeyValue,
           ValueSize
           );
}

NVRAM_GUID_MAP mGUIDMaps[]    = {
  { &gAppleVendorNvramGuid,  CheckAppleVendorNvramGuid },
  { &gAppleBootVariableGuid, CheckAppleBootVariableGuid }
};
UINTN          mGUIDMapsCount = ARRAY_SIZE (mGUIDMaps);

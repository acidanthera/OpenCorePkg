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

#ifndef OC_USER_UTILITIES_OCVALIDATE_NVRAM_KEY_INFO_H
#define OC_USER_UTILITIES_OCVALIDATE_NVRAM_KEY_INFO_H

#include <Library/OcConfigurationLib.h>

/**
  Check whether one NVRAM key has accepted value.
**/
typedef
BOOLEAN
(*NVRAM_KEY_CHECK) (
  IN  CONST VOID       *Value,
  IN  UINT32           ValueSize
  );

/**
  Structure holding NVRAM key maps.
**/
typedef struct NVRAM_KEY_MAP_ {
  CONST CHAR8          *KeyName;
  NVRAM_KEY_CHECK      KeyChecker;
} NVRAM_KEY_MAP;

/**
  Structure holding NVRAM GUID maps.
**/
typedef struct NVRAM_GUID_MAP_ {
  CONST EFI_GUID       *Guid;
  CONST NVRAM_KEY_MAP  *NvramKeyMaps;
  UINTN                NvramKeyMapsCount;
} NVRAM_GUID_MAP;

extern NVRAM_GUID_MAP  mGUIDMaps[];
extern UINTN           mGUIDMapsCount;

#endif // OC_USER_UTILITIES_OCVALIDATE_NVRAM_KEY_INFO_H

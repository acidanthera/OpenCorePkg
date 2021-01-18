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
ValidateNvramKeySize8 (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  (VOID) Value;

  return ValueSize == sizeof (UINT8);
}

STATIC
BOOLEAN
ValidateNvramKeySize32 (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  (VOID) Value;

  return ValueSize == sizeof (UINT32);
}

STATIC
BOOLEAN
ValidateNvramKeySize64 (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  (VOID) Value;

  return ValueSize == sizeof (UINT64);
}

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
    0x02   ///< HiDPI
  };

  UIScaleValue = (CONST UINT8 *) Value;

  if (!ValidateNvramKeySize8 (NULL, ValueSize)) {
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
    0x30,  ///< "0" - WebDriver off
    0x31   ///< "1" - WebDriver on
  };

  NvdaDrvValue = (CONST UINT8 *) Value;

  if (!ValidateNvramKeySize8 (NULL, ValueSize)) {
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
ValidateBootArgs (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  CONST CHAR8   *BootArgsValue;

  BootArgsValue = (CONST CHAR8 *) Value;

  return OcAsciiStringNPrintable (BootArgsValue, ValueSize);
}

STATIC
BOOLEAN
ValidateBooterCfg (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  //
  // bootercfg is similar to boot-args.
  //
  return ValidateBootArgs (Value, ValueSize);
}

STATIC
BOOLEAN
ValidateDefaultBackgroundColor (
  IN  CONST VOID   *Value,
  IN  UINT32       ValueSize
  )
{
  //
  // Cast Value to UINT8 * to ensure alignment.
  //
  CONST UINT8  *DefaultBackgroundColorValue;

  DefaultBackgroundColorValue = (CONST UINT8 *) Value;

  //
  // Even if casted to UINT8 *, DefaultBackgroundColor is still 32-bit.
  //
  if (!ValidateNvramKeySize32 (NULL, ValueSize)) {
    return FALSE;
  }

  //
  // The last byte must be zero.
  //
  return DefaultBackgroundColorValue[3] == 0U;
}

// STATIC
// INTN
// BinarySearch (
//   IN  CONST VOID  *First,
//   IN  UINTN       Start,
//   IN  UINTN       End,
//   IN  UINTN       Key
//   )
// {
//   CONST UINT8  *Array;
//   INTN         RetVal;
//   UINTN        Mid;

//   Array  = (CONST UINT8 *) First; 
//   RetVal = -1;

//   while (Start <= End) {
//     Mid = Start + (End - Start) / 2;

//     if (Array[Mid] < Key) {
//       Start = Mid + 1;
//     } else if (Array[Mid] > Key) {
//       End   = Mid - 1;
//     } else {
//       RetVal = Mid;
//       break;
//     }
//   }

//   return RetVal;
// }

// STATIC
// BOOLEAN
// ValidatePrevLangKbd (
//   IN  CONST VOID   *Value,
//   IN  UINT32       ValueSize
//   )
// {
//   STATIC CONST CHAR8 *AllowedKeyboardLayouts[] = {
//     "cs", "et", "hu", "lv", "lt", "pl", "sk",
//     "bg", "be", "mk", "ru", "sr-Cyrl", "uk", "en",
//     "ja", "fr", "de", "en_AU", "de_AT", "nl_BE", "pt_BR",
//     "en_GB", "fr_CA", "da", "nl", "fi", "it", "nb",
//     "pt_PT", "es", "sv", "fr_CH", "de_CH", "to", "en_US",
//     "ko", "fa", "ps", "uz-Arab", "ta", "ar", "hy", "az-Latn",
//     "bn", "zh-Hant", "chr", "hr", "hi", "fo", "ka", "el",
//     "gu", "pa", "haw", "he", "is", "iu", "ga", "ms-Arab", "kn",
//     "kk", "km", "ckb", "ml", "mt", "mi", "my", "ne", "se", "or",
//     "ro", "st-Latn", "si", "sl", "sv", "ta", "te", "th", "bo",
//     "tr", "ur", "ug", "vi", "cy"
//   };
//   UINTN        AllowedKeyboardLayoutsCount = ARRAY_SIZE (AllowedKeyboardLayouts);

//   UINTN        Index;
//   CONST UINT8  *PrevLangKbdValue;

//   PrevLangKbdValue = (CONST UINT8 *) Value;

//   for (Index = 0; Index < AllowedKeyboardLayoutsCount; ++Index) {
//     // if (ArrayIsSubset (PrevLangKbdValue, AllowedKeyboardLayouts[Index], ValueSize, AsciiStrLen (AllowedKeyboardLayouts[Index]))) {
//       return TRUE;
//     // }
//   }

//   return FALSE;
// }

STATIC NVRAM_KEY_MAP  mAppleBootVariableGuidKeyMaps[] = {
  { "nvda_drv",          ValidateNvdaDrv        },
  { "boot-args",         ValidateBootArgs       },
  { "bootercfg",         ValidateBooterCfg      },
  { "csr-active-config", ValidateNvramKeySize32 },
  // { "prev-lang:kbd",     ValidatePrevLangKbd    },
  { "StartupMute",       ValidateNvramKeySize8  },
  { "SystemAudioVolume", ValidateNvramKeySize8  },
};

STATIC NVRAM_KEY_MAP  mAppleVendorVariableGuidKeyMaps[] = {
  { "UIScale",                  ValidateUIScale                },
  { "FirmwareFeatures",         ValidateNvramKeySize32         },
  { "ExtendedFirmwareFeatures", ValidateNvramKeySize64         },
  { "FirmwareFeaturesMask",     ValidateNvramKeySize32         },
  { "ExtendedFirmwareFeatures", ValidateNvramKeySize64         },
  { "DefaultBackgroundColor",   ValidateDefaultBackgroundColor },
};

NVRAM_GUID_MAP mGUIDMaps[] = {
  { &gAppleBootVariableGuid,   &mAppleBootVariableGuidKeyMaps[0],   ARRAY_SIZE (mAppleBootVariableGuidKeyMaps)   },
  { &gAppleVendorVariableGuid, &mAppleVendorVariableGuidKeyMaps[0], ARRAY_SIZE (mAppleVendorVariableGuidKeyMaps) },
};
UINTN mGUIDMapsCount = ARRAY_SIZE (mGUIDMaps);

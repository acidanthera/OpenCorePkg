/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef APPLE_PLATFORM_INFO_H
#define APPLE_PLATFORM_INFO_H

// APPLE_FSB_FREQUENCY_PLATFORM_INFO_GUID
#define APPLE_FSB_FREQUENCY_PLATFORM_INFO_GUID  \
  { 0xD1A04D55, 0x75B9, 0x41A3,                 \
    { 0x90, 0x36, 0x8F, 0x4A, 0x26, 0x1C, 0xBB, 0xA2 } }

// gAppleFsbFrequencyPlatformInfoGuid
extern EFI_GUID gAppleFsbFrequencyPlatformInfoGuid;

// APPLE_PLATFORM_INFO_KEYBOARD_GUID
#define APPLE_PLATFORM_INFO_KEYBOARD_GUID  \
  { 0x51871CB9, 0xE25D, 0x44B4,            \
    { 0x96, 0x99, 0x0E, 0xE8, 0x64, 0x4C, 0xED, 0x69 } }

// APPLE_KEYBOARD_INFO
typedef UINT32 APPLE_KEYBOARD_INFO;

// gAppleKeyboardPlatformInfoGuid
extern EFI_GUID gAppleKeyboardPlatformInfoGuid;

// APPLE_PRODUCT_INFO_PLATFORM_INFO_GUID
#define APPLE_PRODUCT_INFO_PLATFORM_INFO_GUID  \
  { 0xBF7F6F3A, 0x5523, 0x488E,                \
    { 0x8A, 0x60, 0xF0, 0x48, 0x63, 0xB9, 0x75, 0xC3 } }

// APPLE_PRODUCT_INFO
typedef struct {
  UINT8  Unknown[86];
  CHAR16 Model[64];
  CHAR16 Family[64];
} APPLE_PRODUCT_INFO;

// gAppleProductInfoPlatformInfoGuid
extern EFI_GUID gAppleProductInfoPlatformInfoGuid;

#endif // APPLE_PLATFORM_INFO_H

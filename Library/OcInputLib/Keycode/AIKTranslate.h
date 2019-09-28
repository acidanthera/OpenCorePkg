/** @file
  Key translator

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef AIK_TRANSLATE_H
#define AIK_TRANSLATE_H

#include <IndustryStandard/AppleHid.h>
#include <Protocol/AmiKeycode.h>

#define AIK_MAX_PS2KEY_NUM    128
#define AIK_MAX_EFIKEY_NUM    128
#define AIK_MAX_ASCII_NUM     128
#define AIK_MAX_SCANCODE_NUM  24

#define AIK_MAX_MODIFIERS_NUM ( \
  USB_HID_KB_KP_MODIFIER_RIGHT_SHIFT \
  | USB_HID_KB_KP_MODIFIER_LEFT_SHIFT \
  | USB_HID_KB_KP_MODIFIER_RIGHT_CONTROL \
  | USB_HID_KB_KP_MODIFIER_LEFT_CONTROL \
  | USB_HID_KB_KP_MODIFIER_RIGHT_ALT \
  | USB_HID_KB_KP_MODIFIER_LEFT_ALT \
  | USB_HID_KB_KP_MODIFIER_RIGHT_GUI \
  | USB_HID_KB_KP_MODIFIER_LEFT_GUI \
  )

#define AIK_APPLEKEY_MIN AppleHidUsbKbUsageKeyA
#define AIK_APPLEKEY_MAX AppleHidUsbKbUsageKeyUpArrow
#define AIK_MAX_APPLEKEY_NUM ((AIK_APPLEKEY_MAX) - (AIK_APPLEKEY_MIN) + 1)

typedef struct {
  UINT8         UsbCode;
  CONST CHAR8   *KeyName;
  CONST CHAR8   *ShiftKeyName;
} AIK_PS2KEY_TO_USB;

typedef struct {
  UINT8         UsbCode;
  UINT32        ShiftState;
  CONST CHAR8   *KeyName;
} AIK_EFIKEY_TO_USB;

typedef struct {
  UINT8         UsbCode;
  UINT32        ShiftState;
  CONST CHAR8   *KeyName;
} AIK_ASCII_TO_USB;

typedef struct {
  UINT8         UsbCode;
  CONST CHAR8   *KeyName;
} AIK_SCANCODE_TO_USB;

extern AIK_PS2KEY_TO_USB    gAikPs2KeyToUsbMap[AIK_MAX_PS2KEY_NUM];
extern AIK_EFIKEY_TO_USB    gAikEfiKeyToUsbMap[AIK_MAX_EFIKEY_NUM];
extern AIK_ASCII_TO_USB     gAikAsciiToUsbMap[AIK_MAX_ASCII_NUM];
extern AIK_SCANCODE_TO_USB  gAikScanCodeToUsbMap[AIK_MAX_SCANCODE_NUM];
extern CONST CHAR8 *        gAikModifiersToNameMap[AIK_MAX_MODIFIERS_NUM];
extern CONST CHAR8 *        gAikAppleKeyToNameMap[AIK_MAX_APPLEKEY_NUM];

#define AIK_PS2KEY_TO_NAME(k, m) \
  (((k) < AIK_MAX_PS2KEY_NUM && gAikPs2KeyToUsbMap[k].KeyName) \
  ? (((m) & (EFI_LEFT_SHIFT_PRESSED|EFI_RIGHT_SHIFT_PRESSED)) \
     ? gAikPs2KeyToUsbMap[k].ShiftKeyName : gAikPs2KeyToUsbMap[k].KeyName) \
  : "<ps2key>")

#define AIK_EFIKEY_TO_NAME(k) \
  (((k) < AIK_MAX_EFIKEY_NUM && gAikEfiKeyToUsbMap[k].KeyName) \
  ? gAikEfiKeyToUsbMap[k].KeyName \
  : "<efikey>")

#define AIK_ASCII_TO_NAME(k) \
  (((k) >= 0 && (k) < AIK_MAX_ASCII_NUM && gAikAsciiToUsbMap[k].KeyName) \
  ? gAikAsciiToUsbMap[k].KeyName \
  : "<ascii>")

#define AIK_SCANCODE_TO_NAME(k) \
  (((k) < AIK_MAX_SCANCODE_NUM && gAikScanCodeToUsbMap[k].KeyName) \
  ? gAikScanCodeToUsbMap[k].KeyName \
  : "<scancode>")

#define AIK_MODIFIERS_TO_NAME(k) \
  (((k) < AIK_MAX_MODIFIERS_NUM && gAikModifiersToNameMap[k]) \
  ? gAikModifiersToNameMap[k] \
  : "<none>")

#define AIK_APPLEKEY_TO_NAME(k) \
  (((k) >= AIK_APPLEKEY_MIN && (k) <= AIK_APPLEKEY_MAX && gAikAppleKeyToNameMap[(k) - AIK_APPLEKEY_MIN]) \
  ? gAikAppleKeyToNameMap[(k) - AIK_APPLEKEY_MIN] \
  : "<aapl>")

enum {
  AIK_RIGHT_SHIFT,
  AIK_LEFT_SHIFT,
  AIK_RIGHT_CONTROL,
  AIK_LEFT_CONTROL,
  AIK_RIGHT_ALT,
  AIK_LEFT_ALT,
  AIK_RIGHT_GUI,
  AIK_LEFT_GUI,
  AIK_MODIFIER_MAX
};

VOID
AIKTranslateConfigure (
  VOID
  );

VOID
AIKTranslate (
  IN  AMI_EFI_KEY_DATA    *KeyData,
  OUT APPLE_MODIFIER_MAP  *Modifiers,
  OUT APPLE_KEY_CODE      *Key
  );

#endif

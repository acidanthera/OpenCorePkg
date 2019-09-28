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

#include "AIKTranslate.h"

#include <Library/DebugLib.h>

STATIC
APPLE_MODIFIER_MAP    mModifierRemap[AIK_MODIFIER_MAX];

STATIC
VOID
AIKTranslateModifiers (
  IN  AMI_EFI_KEY_DATA     *KeyData,
  OUT APPLE_MODIFIER_MAP   *Modifiers
  )
{
  UINT32  KeyShiftState;

  KeyShiftState = KeyData->KeyState.KeyShiftState;

  *Modifiers = 0;

  //
  // Handle EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL Shift support, which is not included
  // in KeyShiftState on APTIO and VMware.
  // UPD: Recent APTIO V also does not include it in its own protocol.
  //
  if ((KeyShiftState & EFI_SHIFT_STATE_VALID) && KeyData->Key.UnicodeChar < AIK_MAX_EFIKEY_NUM) {
    KeyShiftState |= gAikAsciiToUsbMap[KeyData->Key.UnicodeChar].ShiftState;
  }

  //
  // Handle legacy EFI_SIMPLE_TEXT_INPUT_PROTOCOL by guessing from EfiKey
  //
  if ((KeyShiftState & EFI_SHIFT_STATE_VALID) != EFI_SHIFT_STATE_VALID
    && KeyData->Key.UnicodeChar < AIK_MAX_EFIKEY_NUM) {
    KeyShiftState = gAikAsciiToUsbMap[KeyData->Key.UnicodeChar].ShiftState;
  }

  if (KeyShiftState & EFI_SHIFT_STATE_VALID) {
    if (KeyShiftState & EFI_RIGHT_SHIFT_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_RIGHT_SHIFT];
    }
    if (KeyShiftState & EFI_LEFT_SHIFT_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_LEFT_SHIFT];
    }
    if (KeyShiftState & EFI_RIGHT_CONTROL_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_RIGHT_CONTROL];
    }
    if (KeyShiftState & EFI_LEFT_CONTROL_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_LEFT_CONTROL];
    }
    if (KeyShiftState & EFI_RIGHT_ALT_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_RIGHT_ALT];
    }
    if (KeyShiftState & EFI_LEFT_ALT_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_LEFT_ALT];
    }
    if (KeyShiftState & EFI_RIGHT_LOGO_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_RIGHT_GUI];
    }
    if (KeyShiftState & EFI_LEFT_LOGO_PRESSED) {
      *Modifiers |= mModifierRemap[AIK_LEFT_GUI];
    }
  }
}

STATIC
VOID
AIKTranslateNumpad (
  IN OUT UINT8    *UsbKey,
  IN     EFI_KEY  EfiKey
  )
{
  switch (EfiKey) {
    case EfiKeyZero:
      *UsbKey = UsbHidUsageIdKbKpKeyZero;
      break;
    case EfiKeyOne:
      *UsbKey = UsbHidUsageIdKbKpKeyOne;
      break;
    case EfiKeyTwo:
      *UsbKey = UsbHidUsageIdKbKpKeyTwo;
      break;
    case EfiKeyThree:
      *UsbKey = UsbHidUsageIdKbKpKeyThree;
      break;
    case EfiKeyFour:
      *UsbKey = UsbHidUsageIdKbKpKeyFour;
      break;
    case EfiKeyFive:
      *UsbKey = UsbHidUsageIdKbKpKeyFive;
      break;
    case EfiKeySix:
      *UsbKey = UsbHidUsageIdKbKpKeySix;
      break;
    case EfiKeySeven:
      *UsbKey = UsbHidUsageIdKbKpKeySeven;
      break;
    case EfiKeyEight:
      *UsbKey = UsbHidUsageIdKbKpKeyEight;
      break;
    case EfiKeyNine:
      *UsbKey = UsbHidUsageIdKbKpKeyNine;
      break;
    default:
      break;
  }
}

VOID
AIKTranslateConfigure (
  VOID
  )
{
  UINTN                     Index;
  CONST APPLE_MODIFIER_MAP  DefaultModifierMap[AIK_MODIFIER_MAX] = {
    USB_HID_KB_KP_MODIFIER_RIGHT_SHIFT,
    USB_HID_KB_KP_MODIFIER_LEFT_SHIFT,
    USB_HID_KB_KP_MODIFIER_RIGHT_CONTROL,
    USB_HID_KB_KP_MODIFIER_LEFT_CONTROL,
    USB_HID_KB_KP_MODIFIER_RIGHT_ALT,
    USB_HID_KB_KP_MODIFIER_LEFT_ALT,
    USB_HID_KB_KP_MODIFIER_RIGHT_GUI,
    USB_HID_KB_KP_MODIFIER_LEFT_GUI
  };

  //TODO: This should be user-configurable, perhaps via a nvram variable.

  // You can swap Alt with Gui, to get Apple layout on a PC keyboard
  CONST UINTN DefaultModifierConfig[AIK_MODIFIER_MAX/2] = {
    AIK_RIGHT_SHIFT,
    AIK_RIGHT_CONTROL,
    AIK_RIGHT_ALT,
    AIK_RIGHT_GUI
  };

  for (Index = 0; Index < AIK_MODIFIER_MAX/2; Index++) {
    mModifierRemap[Index*2]   = DefaultModifierMap[DefaultModifierConfig[Index]];
    mModifierRemap[Index*2+1] = DefaultModifierMap[DefaultModifierConfig[Index]+1];
  }
}

VOID
AIKTranslate (
  IN  AMI_EFI_KEY_DATA    *KeyData,
  OUT APPLE_MODIFIER_MAP  *Modifiers,
  OUT APPLE_KEY_CODE      *Key
  )
{
  AIK_PS2KEY_TO_USB  Ps2Key;

  AIKTranslateModifiers (KeyData, Modifiers);

  *Key = UsbHidUndefined;

  //
  // Firstly check for APTIO protocol, which reported a PS/2 key to us.
  // Otherwise try to decode UnicodeChar and Scancode from simple input.
  //
  if (KeyData->PS2ScanCodeIsValid == 1 && KeyData->PS2ScanCode < AIK_MAX_PS2KEY_NUM) {
    Ps2Key = gAikPs2KeyToUsbMap[KeyData->PS2ScanCode];
    if (Ps2Key.UsbCode != UsbHidUndefined) {
      //
      // We need to process numpad keys separately.
      //
      AIKTranslateNumpad (&Ps2Key.UsbCode, KeyData->EfiKey);
      *Key = APPLE_HID_USB_KB_KP_USAGE (Ps2Key.UsbCode);
    }
  } else if (KeyData->Key.UnicodeChar >= 0
    && KeyData->Key.UnicodeChar < AIK_MAX_ASCII_NUM
    && gAikAsciiToUsbMap[KeyData->Key.UnicodeChar].UsbCode != UsbHidUndefined) {
    *Key = APPLE_HID_USB_KB_KP_USAGE (gAikAsciiToUsbMap[KeyData->Key.UnicodeChar].UsbCode);
  } else if (KeyData->Key.ScanCode < AIK_MAX_SCANCODE_NUM
    && gAikScanCodeToUsbMap[KeyData->Key.ScanCode].UsbCode != UsbHidUndefined) {
    *Key = APPLE_HID_USB_KB_KP_USAGE (gAikScanCodeToUsbMap[KeyData->Key.ScanCode].UsbCode);
  }

  if (*Key != UsbHidUndefined) {
    DEBUG ((DEBUG_VERBOSE, "\nAIKTranslate P1 MOD %a APPLE 0x%X (%a) PS2 0x%X Ps2Name %a\n",
      AIK_MODIFIERS_TO_NAME (*Modifiers), *Key, AIK_APPLEKEY_TO_NAME (*Key),
      KeyData->PS2ScanCode, AIK_PS2KEY_TO_NAME (KeyData->PS2ScanCode, *Modifiers)));
    DEBUG ((DEBUG_VERBOSE, "AIKTranslate P2 AsciiName %a ScanName %a EfiKey %a Scan 0x%X Uni 0x%X SState 0x%X\n",
      AIK_ASCII_TO_NAME (KeyData->Key.UnicodeChar), AIK_SCANCODE_TO_NAME (KeyData->Key.ScanCode),
      AIK_EFIKEY_TO_NAME (KeyData->EfiKey), KeyData->Key.ScanCode, KeyData->Key.UnicodeChar, KeyData->KeyState.KeyShiftState));
  }
}

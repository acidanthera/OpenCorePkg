/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_HID_H
#define APPLE_HID_H

#include <IndustryStandard/UsbHid.h>

// APPLE_USAGE
#define APPLE_HID_GENERIC_USAGE(UsageId, PageId) (((PageId) << 12) | (UsageId))

#define APPLE_HID_USB_KB_KP_USAGE(UsbHidUsageIdKbKp) \
  APPLE_HID_GENERIC_USAGE ((UsbHidUsageIdKbKp), AppleHidKeyboardKeypadPage)

#define APPLE_HID_KP_SCAN_USAGE(UsbHidUsageIdKbKp) \
  APPLE_HID_GENERIC_USAGE ((UsbHidUsageIdKbKp), AppleHidUsbKbUsageKeypadScanPage)

#define APPLE_HID_REMOTE_USAGE(AppleRemoteUsageId)  \
  APPLE_HID_GENERIC_USAGE ((AppleRemoteUsageId), AppleHidRemotePage)

// IS_APPLE_KEY_LETTER
#define IS_APPLE_KEY_LETTER(Usage)                           \
  ((((APPLE_HID_USAGE)(Usage)) >= AppleHidUsbKbUsageKeyA)    \
 && (((APPLE_HID_USAGE)(Usage)) <= AppleHidUsbKbUsageKeyZ))

// APPLE_HID_USAGE_ID
typedef UINT8 APPLE_HID_USAGE_ID;

// APPLE_HID_USAGE
typedef UINT16 APPLE_HID_USAGE;

// APPLE_HID_PAGE_ID
enum {
  AppleHidKeyboardKeypadPage       = UsbHidKeyboardKeypadPage,
  AppleHidUsbKbUsageKeypadScanPage = 0x08,
  AppleHidRemotePage               = 0x08
};

typedef UINT8 APPLE_HID_PAGE_ID;

// APPLE_SCAN_CODE
enum {
  AppleScanKeypadSlash    = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeySlash),
  AppleScanKeypadAsterisk = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyAsterisk),
  AppleScanKeypadMinus    = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyMinus),
  AppleScanKeypadPlus     = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyPlus),
  AppleScanKeypadEnter    = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyEnter),
  AppleScanKeypadOne      = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyOne),
  AppleScanKeypadTwo      = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyTwo),
  AppleScanKeypadThree    = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyThree),
  AppleScanKeypadFour     = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyFour),
  AppleScanKeypadFive     = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyFive),
  AppleScanKeypadSix      = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeySix),
  AppleScanKeypadSeven    = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeySeven),
  AppleScanKeypadEight    = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyEight),
  AppleScanKeypadNine     = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyNine),
  AppleScanKeypadIns      = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyIns),
  AppleScanKeypadDel      = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyDel),
  AppleScanKeypadEquals   = APPLE_HID_KP_SCAN_USAGE (UsbHidUsageIdKbKpPadKeyEquals)
};

typedef UINT16 APPLE_SCAN_CODE;

// Apple modifers

#define APPLE_MODIFIER_LEFT_CONTROL   USB_HID_KB_KP_MODIFIERS_CONTROL
#define APPLE_MODIFIER_LEFT_SHIFT     USB_HID_KB_KP_MODIFIER_LEFT_SHIFT
#define APPLE_MODIFIER_LEFT_OPTION    USB_HID_KB_KP_MODIFIER_LEFT_ALT
#define APPLE_MODIFIER_LEFT_COMMAND   USB_HID_KB_KP_MODIFIER_LEFT_GUI
#define APPLE_MODIFIER_RIGHT_CONTROL  USB_HID_KB_KP_MODIFIER_RIGHT_CONTROL
#define APPLE_MODIFIER_RIGHT_SHIFT    USB_HID_KB_KP_MODIFIER_RIGHT_SHIFT
#define APPLE_MODIFIER_RIGHT_OPTION   USB_HID_KB_KP_MODIFIER_RIGHT_ALT
#define APPLE_MODIFIER_RIGHT_COMMAND  USB_HID_KB_KP_MODIFIER_RIGHT_GUI

// Shortcuts for multiple Apple modifers

#define APPLE_MODIFIERS_CONTROL  \
  (APPLE_MODIFIER_LEFT_CONTROL | APPLE_MODIFIER_RIGHT_CONTROL)

#define APPLE_MODIFIERS_SHIFT  \
  (APPLE_MODIFIER_LEFT_SHIFT | APPLE_MODIFIER_RIGHT_SHIFT)

#define APPLE_MODIFIERS_OPTION  \
  (APPLE_MODIFIER_LEFT_OPTION | APPLE_MODIFIER_RIGHT_OPTION)

#define APPLE_MODIFIERS_COMMAND \
  (APPLE_MODIFIER_LEFT_COMMAND | APPLE_MODIFIER_RIGHT_COMMAND)

// APPLE_MODIFIER_MAP
typedef UINT16 APPLE_MODIFIER_MAP;

// APPLE_HID_USB_KB_USAGE
/// A set of Apple keys used with the AppleKeyMap protocols.
enum {
  AppleHidUsbKbUsageKeyA                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyA),
  AppleHidUsbKbUsageKeyB                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyB),
  AppleHidUsbKbUsageKeyC                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyC),
  AppleHidUsbKbUsageKeyD                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyD),
  AppleHidUsbKbUsageKeyE                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyE),
  AppleHidUsbKbUsageKeyF                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF),
  AppleHidUsbKbUsageKeyG                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyG),
  AppleHidUsbKbUsageKeyH                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyH),
  AppleHidUsbKbUsageKeyI                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyI),
  AppleHidUsbKbUsageKeyJ                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyJ),
  AppleHidUsbKbUsageKeyK                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyK),
  AppleHidUsbKbUsageKeyL                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyL),
  AppleHidUsbKbUsageKeyM                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyM),
  AppleHidUsbKbUsageKeyN                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyN),
  AppleHidUsbKbUsageKeyO                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyO),
  AppleHidUsbKbUsageKeyP                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyP),
  AppleHidUsbKbUsageKeyQ                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyQ),
  AppleHidUsbKbUsageKeyR                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyR),
  AppleHidUsbKbUsageKeyS                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyS),
  AppleHidUsbKbUsageKeyT                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyT),
  AppleHidUsbKbUsageKeyU                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyU),
  AppleHidUsbKbUsageKeyV                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyV),
  AppleHidUsbKbUsageKeyW                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyW),
  AppleHidUsbKbUsageKeyX                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyX),
  AppleHidUsbKbUsageKeyY                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyY),
  AppleHidUsbKbUsageKeyZ                    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyZ),
  AppleHidUsbKbUsageKeyOne                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyOne),
  AppleHidUsbKbUsageKeyTwo                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyTwo),
  AppleHidUsbKbUsageKeyThree                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyThree),
  AppleHidUsbKbUsageKeyFour                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyFour),
  AppleHidUsbKbUsageKeyFive                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyFive),
  AppleHidUsbKbUsageKeySix                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySix),
  AppleHidUsbKbUsageKeySeven                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySeven),
  AppleHidUsbKbUsageKeyEight                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyEight),
  AppleHidUsbKbUsageKeyNine                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyNine),
  AppleHidUsbKbUsageKeyZero                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyZero),
  AppleHidUsbKbUsageKeyEnter                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyEnter),
  AppleHidUsbKbUsageKeyEscape               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyEsc),
  AppleHidUsbKbUsageKeyBackSpace            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyBackSpace),
  AppleHidUsbKbUsageKeyTab                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyTab),
  AppleHidUsbKbUsageKeySpaceBar             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySpaceBar),
  AppleHidUsbKbUsageKeyMinus                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyMinus),
  AppleHidUsbKbUsageKeyEquals               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyEquals),
  AppleHidUsbKbUsageKeyLeftBracket          = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLeftBracket),
  AppleHidUsbKbUsageKeyRightBracket         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyRightBracket),
  AppleHidUsbKbUsageKeyBackslash            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyBackslash),
  AppleHidUsbKbUsageKeyNonUsHash            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyNonUsHash),
  AppleHidUsbKbUsageKeySemicolon            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySemicolon),
  AppleHidUsbKbUsageKeyQuotation            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyQuotation),
  AppleHidUsbKbUsageKeyAcute                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyAcute),
  AppleHidUsbKbUsageKeyComma                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyComma),
  AppleHidUsbKbUsageKeyPeriod               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyPeriod),
  AppleHidUsbKbUsageKeySlash                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySlash),
  AppleHidUsbKbUsageKeyCLock                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyCLock),
  AppleHidUsbKbUsageKeyF1                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF1),
  AppleHidUsbKbUsageKeyF2                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF2),
  AppleHidUsbKbUsageKeyF3                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF3),
  AppleHidUsbKbUsageKeyF4                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF4),
  AppleHidUsbKbUsageKeyF5                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF5),
  AppleHidUsbKbUsageKeyF6                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF6),
  AppleHidUsbKbUsageKeyF7                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF7),
  AppleHidUsbKbUsageKeyF8                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF8),
  AppleHidUsbKbUsageKeyF9                   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF9),
  AppleHidUsbKbUsageKeyF10                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF10),
  AppleHidUsbKbUsageKeyF11                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF11),
  AppleHidUsbKbUsageKeyF12                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF12),
  AppleHidUsbKbUsageKeyPrint                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyPrint),
  AppleHidUsbKbUsageKeySLock                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySLock),
  AppleHidUsbKbUsageKeyPause                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyPause),
  AppleHidUsbKbUsageKeyIns                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyIns),
  AppleHidUsbKbUsageKeyHome                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyHome),
  AppleHidUsbKbUsageKeyPgUp                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyPgUp),
  AppleHidUsbKbUsageKeyDel                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyDel),
  AppleHidUsbKbUsageKeyEnd                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyEnd),
  AppleHidUsbKbUsageKeyPgDn                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyPgDn),
  AppleHidUsbKbUsageKeyRightArrow           = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyRightArrow),
  AppleHidUsbKbUsageKeyLeftArrow            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLeftArrow),
  AppleHidUsbKbUsageKeyDownArrow            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyDownArrow),
  AppleHidUsbKbUsageKeyUpArrow              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyUpArrow),
  AppleHidUsbKbUsageKeyPadNLck              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyNLck),
  AppleHidUsbKbUsageKeyPadSlash             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeySlash),
  AppleHidUsbKbUsageKeyPadAsterisk          = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyAsterisk),
  AppleHidUsbKbUsageKeyPadMinus             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMinus),
  AppleHidUsbKbUsageKeyPadPlus              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyPlus),
  AppleHidUsbKbUsageKeyPadEnter             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyEnter),
  AppleHidUsbKbUsageKeyPadOne               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyOne),
  AppleHidUsbKbUsageKeyPadTwo               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyTwo),
  AppleHidUsbKbUsageKeyPadThree             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyThree),
  AppleHidUsbKbUsageKeyPadFour              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyFour),
  AppleHidUsbKbUsageKeyPadFive              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyFive),
  AppleHidUsbKbUsageKeyPadSix               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeySix),
  AppleHidUsbKbUsageKeyPadSeven             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeySeven),
  AppleHidUsbKbUsageKeyPadEight             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyEight),
  AppleHidUsbKbUsageKeyPadNine              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyNine),
  AppleHidUsbKbUsageKeyPadIns               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyIns),
  AppleHidUsbKbUsageKeyPadDel               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyDel),
  AppleHidUsbKbUsageKeyPadNonUsBackslash    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyNonUsBackslash),
  AppleHidUsbKbUsageKeyPadApplication       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyApplication),
  AppleHidUsbKbUsageKeyPadPower             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyPower),
  AppleHidUsbKbUsageKeyPadEquals            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyEquals),
  AppleHidUsbKbUsageKeyF13                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF13),
  AppleHidUsbKbUsageKeyF14                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF14),
  AppleHidUsbKbUsageKeyF15                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF15),
  AppleHidUsbKbUsageKeyF16                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF16),
  AppleHidUsbKbUsageKeyF17                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF17),
  AppleHidUsbKbUsageKeyF18                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF18),
  AppleHidUsbKbUsageKeyF19                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF19),
  AppleHidUsbKbUsageKeyF20                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF20),
  AppleHidUsbKbUsageKeyF21                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF21),
  AppleHidUsbKbUsageKeyF22                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF22),
  AppleHidUsbKbUsageKeyF23                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF23),
  AppleHidUsbKbUsageKeyF24                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyF24),
  AppleHidUsbKbUsageKeyExecute              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyExecute),
  AppleHidUsbKbUsageKeyHelp                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyHelp),
  AppleHidUsbKbUsageKeyMenu                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyMenu),
  AppleHidUsbKbUsageKeySelect               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySelect),
  AppleHidUsbKbUsageKeyStop                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyStop),
  AppleHidUsbKbUsageKeyAgain                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyAgain),
  AppleHidUsbKbUsageKeyUndo                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyUndo),
  AppleHidUsbKbUsageKeyCut                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyCut),
  AppleHidUsbKbUsageKeyCopy                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyCopy),
  AppleHidUsbKbUsageKeyPaste                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyPaste),
  AppleHidUsbKbUsageKeyFind                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyFind),
  AppleHidUsbKbUsageKeyMute                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyMute),
  AppleHidUsbKbUsageKeyVolumeUp             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyVolumeUp),
  AppleHidUsbKbUsageKeyVolumeDown           = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyVolumeDown),
  AppleHidUsbKbUsageLockingKeyCLock         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpLockKeyCLock),
  AppleHidUsbKbusageLockingKeyNLock         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpLockKeyNLock),
  AppleHidUsbKbUsageLockingKeySLock         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpLockKeySLock),
  AppleHidUsbKbUsageKeyPadComma             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyComma),
  AppleHidUsbKbUsageKeyPadEqualSign         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyEqualSign),
  AppleHidUsbKbUsageKeyInternational1       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational1),
  AppleHidUsbKbUsageKeyInternational2       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational2),
  AppleHidUsbKbUsageKeyInternational3       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational3),
  AppleHidUsbKbUsageKeyInternational4       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational4),
  AppleHidUsbKbUsageKeyInternational5       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational5),
  AppleHidUsbKbUsageKeyInternational6       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational6),
  AppleHidUsbKbUsageKeyInternational7       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational7),
  AppleHidUsbKbUsageKeyInternational8       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational8),
  AppleHidUsbKbUsageKeyInternational9       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyInternational9),
  AppleHidUsbKbUsageKeyLang1                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang1),
  AppleHidUsbKbUsageKeyLang2                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang2),
  AppleHidUsbKbUsageKeyLang3                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang3),
  AppleHidUsbKbUsageKeyLang4                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang4),
  AppleHidUsbKbUsageKeyLang5                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang5),
  AppleHidUsbKbUsageKeyLang6                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang6),
  AppleHidUsbKbUsageKeyLang7                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang7),
  AppleHidUsbKbUsageKeyLang8                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang8),
  AppleHidUsbKbUsageKeyLang9                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyLang9),
  AppleHidUsbKbUsageKeyAlternateErase       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyAlternateErase),
  AppleHidUsbKbUsageKeySysReq               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySysReq),
  AppleHidUsbKbUsageKeyCancel               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyCancel),
  AppleHidUsbKbUsageKeyClear                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyClear),
  AppleHidUsbKbUsageKeyPrior                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyPrior),
  AppleHidUsbKbUsageKeyReturn               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyReturn),
  AppleHidUsbKbUsageKeySeparator            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeySeparator),
  AppleHidUsbKbUsageKeyOut                  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyOut),
  AppleHidUsbKbUsageKeyOper                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyOper),
  AppleHidUsbKbUsageKeyClearAgain           = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyClearAgain),
  AppleHidUsbKbUsageKeyCrSel                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyCrSel),
  AppleHidUsbKbUsageKeyExSel                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyExSel),
  AppleHidUsbKbUsageKeyPadDoubleZero        = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyDoubleZero),
  AppleHidUsbKbUsageKeyTrippleZero          = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyTrippleZero),
  AppleHidUsbKbUsageKeyThousandsSeparator   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyThousandsSeparator),
  AppleHidUsbKbUsageKeyDecimalSeparator     = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyDecimalSeparator),
  AppleHidUsbKbUsageKeyCurrencyUnit         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyCurrencyUnit),
  AppleHidUsbKbUsageKeyCurrencySubUnit      = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpKeyCurrencySubUnit),
  AppleHidUsbKbUsageKeyPadLeftBracket       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyLeftBracket),
  AppleHidUsbKbUsageKeyPadRightBracket      = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyRightBracket),
  AppleHidUsbKbUsageKeyPadCurlyLeftBracket  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyCurlyLeftBracket),
  AppleHidUsbKbUsageKeyPadCurlyRightBracket = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyCurlyRightBracket),
  AppleHidUsbKbUsageKeyPadTab               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyTab),
  AppleHidUsbKbUsageKeyPadBackspace         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyBackspace),
  AppleHidUsbKbUsageKeyPadA                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyA),
  AppleHidUsbKbUsageKeyPadB                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyB),
  AppleHidUsbKbUsageKeyPadC                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyC),
  AppleHidUsbKbUsageKeyPadD                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyD),
  AppleHidUsbKbUsageKeyPadE                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyE),
  AppleHidUsbKbUsageKeyPadF                 = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyF),
  AppleHidUsbKbUsageKeyPadXor               = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyXor),
  AppleHidUsbKbUsageKeyPadCaret             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyCaret),
  AppleHidUsbKbUsageKeyPadPercent           = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyPercent),
  AppleHidUsbKbUsageKeyPadLeftAngleBracket  = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyLeftAngleBracket),
  AppleHidUsbKbUsageKeyPadRightAngleBracket = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyRightAngleBracket),
  AppleHidUsbKbUsageKeyPadBitwiseAnd        = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyBitwiseAnd),
  AppleHidUsbKbUsageKeyPadLogicalAnd        = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyLogicalAnd),
  AppleHidUsbKbUsageKeyPadBitwiseOr         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyBitwiseOr),
  AppleHidUsbKbUsageKeyPadLogicalOr         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyLogicalOr),
  AppleHidUsbKbUsageKeyPadColon             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyColon),
  AppleHidUsbKbUsageKeyPadHash              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyHash),
  AppleHidUsbKbUsageKeyPadSpace             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeySpace),
  AppleHidUsbKbUsageKeyPadAt                = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyAt),
  AppleHidUsbKbUsageKeyPadExclamationMark   = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyExclamationMark),
  AppleHidUsbKbUsageKeyPadMemoryStore       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMemoryStore),
  AppleHidUsbKbUsageKeyPadMemoryRecall      = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMemoryRecall),
  AppleHidUsbKbUsageKeyPadMemoryClear       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMemoryClear),
  AppleHidUsbKbUsageKeyPadMemoryAdd         = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMemoryAdd),
  AppleHidUsbKbUsageKeyPadMemorySubtract    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMemorySubtract),
  AppleHidUsbKbUsageKeyPadMemoryMultiply    = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMemoryMultiply),
  AppleHidUsbKbUsageKeyPadMemoryDivide      = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyMemoryDivide),
  AppleHidUsbKbUsageKeyPadSign              = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeySign),
  AppleHidUsbKbUsageKeyPadClear             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyClear),
  AppleHidUsbKbUsageKeyPadClearEntry        = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyClearEntry),
  AppleHidUsbKbUsageKeyPadBinary            = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyBinary),
  AppleHidUsbKbUsageKeyPadOctal             = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyOctal),
  AppleHidUsbKbUsageKeyPadDecimal           = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyDecimal),
  AppleHidUsbKbUsageKeyPadHexadecimal       = APPLE_HID_USB_KB_KP_USAGE (UsbHidUsageIdKbKpPadKeyHexadecimal)
};

// APPLE_KEY_CODE
typedef APPLE_HID_USAGE APPLE_KEY_CODE;

#endif // APPLE_HID_H

/** @file
  Helper functions for USB Keyboard Driver.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
Portions Copyright (c) 2016 - 2018, Download-Fritz. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "KeyBoard.h"

#include <IndustryStandard/AppleHid.h>

#include <Guid/UsbKeyBoardLayout.h>

#include <Library/ResetSystemLib.h>

USB_KEYBOARD_LAYOUT_PACK_BIN  mUsbKeyboardLayoutBin = {
  sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN),   // Binary size

  //
  // EFI_HII_PACKAGE_HEADER
  //
  {
    sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32),
    EFI_HII_PACKAGE_KEYBOARD_LAYOUT
  },
  1,  // LayoutCount
  sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32) - sizeof (EFI_HII_PACKAGE_HEADER) - sizeof (UINT16), // LayoutLength
  USB_KEYBOARD_LAYOUT_KEY_GUID,  // KeyGuid
  sizeof (UINT16) + sizeof (EFI_GUID) + sizeof (UINT32) + sizeof (UINT8) + (USB_KEYBOARD_KEY_COUNT * sizeof (EFI_KEY_DESCRIPTOR)), // LayoutDescriptorStringOffset
  USB_KEYBOARD_KEY_COUNT, // DescriptorCount
  {
    //
    // EFI_KEY_DESCRIPTOR (total number is USB_KEYBOARD_KEY_COUNT)
    //
    {EfiKeyC1,         'a',      'A',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB5,         'b',      'B',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB3,         'c',      'C',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC3,         'd',      'D',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD3,         'e',      'E',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC4,         'f',      'F',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC5,         'g',      'G',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC6,         'h',      'H',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD8,         'i',      'I',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC7,         'j',      'J',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC8,         'k',      'K',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC9,         'l',      'L',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB7,         'm',      'M',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB6,         'n',      'N',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD9,         'o',      'O',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD10,        'p',      'P',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD1,         'q',      'Q',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD4,         'r',      'R',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC2,         's',      'S',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD5,         't',      'T',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD7,         'u',      'U',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB4,         'v',      'V',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD2,         'w',      'W',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB2,         'x',      'X',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD6,         'y',      'Y',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB1,         'z',      'Z',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyE1,         '1',      '!',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE2,         '2',      '@',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE3,         '3',      '#',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE4,         '4',      '$',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE5,         '5',      '%',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE6,         '6',      '^',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE7,         '7',      '&',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE8,         '8',      '*',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE9,         '9',      '(',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE10,        '0',      ')',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyEnter,      0x0d,     0x0d, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyEsc,        0x1b,     0x1b, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyBackSpace,  0x08,     0x08, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyTab,        0x09,     0x09, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeySpaceBar,   ' ',      ' ',  0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyE11,        '-',      '_',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE12,        '=',      '+',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD11,        '[',      '{',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD12,        ']',      '}',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD13,        '\\',     '|',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyC12,        '\\',     '|',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyC10,        ';',      ':',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyC11,        '\'',     '"',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE0,         '`',      '~',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB8,         ',',      '<',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB9,         '.',      '>',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB10,        '/',      '?',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyCapsLock,   0x00,     0x00, 0, 0,  EFI_CAPS_LOCK_MODIFIER,            0},
    {EfiKeyF1,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_ONE_MODIFIER,     0},
    {EfiKeyF2,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TWO_MODIFIER,     0},
    {EfiKeyF3,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_THREE_MODIFIER,   0},
    {EfiKeyF4,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_FOUR_MODIFIER,    0},
    {EfiKeyF5,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_FIVE_MODIFIER,    0},
    {EfiKeyF6,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_SIX_MODIFIER,     0},
    {EfiKeyF7,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_SEVEN_MODIFIER,   0},
    {EfiKeyF8,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_EIGHT_MODIFIER,   0},
    {EfiKeyF9,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_NINE_MODIFIER,    0},
    {EfiKeyF10,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TEN_MODIFIER,     0},
    {EfiKeyF11,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_ELEVEN_MODIFIER,  0},
    {EfiKeyF12,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TWELVE_MODIFIER,  0},
    {EfiKeyPrint,      0x00,     0x00, 0, 0,  EFI_PRINT_MODIFIER,                0},
    {EfiKeySLck,       0x00,     0x00, 0, 0,  EFI_SCROLL_LOCK_MODIFIER,          0},
    {EfiKeyPause,      0x00,     0x00, 0, 0,  EFI_PAUSE_MODIFIER,                0},
    {EfiKeyIns,        0x00,     0x00, 0, 0,  EFI_INSERT_MODIFIER,               0},
    {EfiKeyHome,       0x00,     0x00, 0, 0,  EFI_HOME_MODIFIER,                 0},
    {EfiKeyPgUp,       0x00,     0x00, 0, 0,  EFI_PAGE_UP_MODIFIER,              0},
    {EfiKeyDel,        0x00,     0x00, 0, 0,  EFI_DELETE_MODIFIER,               0},
    {EfiKeyEnd,        0x00,     0x00, 0, 0,  EFI_END_MODIFIER,                  0},
    {EfiKeyPgDn,       0x00,     0x00, 0, 0,  EFI_PAGE_DOWN_MODIFIER,            0},
    {EfiKeyRightArrow, 0x00,     0x00, 0, 0,  EFI_RIGHT_ARROW_MODIFIER,          0},
    {EfiKeyLeftArrow,  0x00,     0x00, 0, 0,  EFI_LEFT_ARROW_MODIFIER,           0},
    {EfiKeyDownArrow,  0x00,     0x00, 0, 0,  EFI_DOWN_ARROW_MODIFIER,           0},
    {EfiKeyUpArrow,    0x00,     0x00, 0, 0,  EFI_UP_ARROW_MODIFIER,             0},
    {EfiKeyNLck,       0x00,     0x00, 0, 0,  EFI_NUM_LOCK_MODIFIER,             0},
    {EfiKeySlash,      '/',      '/',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyAsterisk,   '*',      '*',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyMinus,      '-',      '-',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyPlus,       '+',      '+',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyEnter,      0x0d,     0x0d, 0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyOne,        '1',      '1',  0, 0,  EFI_END_MODIFIER,         EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyTwo,        '2',      '2',  0, 0,  EFI_DOWN_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyThree,      '3',      '3',  0, 0,  EFI_PAGE_DOWN_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyFour,       '4',      '4',  0, 0,  EFI_LEFT_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyFive,       '5',      '5',  0, 0,  EFI_NULL_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeySix,        '6',      '6',  0, 0,  EFI_RIGHT_ARROW_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeySeven,      '7',      '7',  0, 0,  EFI_HOME_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyEight,      '8',      '8',  0, 0,  EFI_UP_ARROW_MODIFIER,    EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyNine,       '9',      '9',  0, 0,  EFI_PAGE_UP_MODIFIER,     EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyZero,       '0',      '0',  0, 0,  EFI_INSERT_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyPeriod,     '.',      '.',  0, 0,  EFI_DELETE_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyA4,         0x00,     0x00, 0, 0,  EFI_MENU_MODIFIER,            0},
    {EfiKeyLCtrl,      0,        0,    0, 0,  EFI_LEFT_CONTROL_MODIFIER,    0},
    {EfiKeyLShift,     0,        0,    0, 0,  EFI_LEFT_SHIFT_MODIFIER,      0},
    {EfiKeyLAlt,       0,        0,    0, 0,  EFI_LEFT_ALT_MODIFIER,        0},
    {EfiKeyA0,         0,        0,    0, 0,  EFI_LEFT_LOGO_MODIFIER,       0},
    {EfiKeyRCtrl,      0,        0,    0, 0,  EFI_RIGHT_CONTROL_MODIFIER,   0},
    {EfiKeyRShift,     0,        0,    0, 0,  EFI_RIGHT_SHIFT_MODIFIER,     0},
    {EfiKeyA2,         0,        0,    0, 0,  EFI_RIGHT_ALT_MODIFIER,       0},
    {EfiKeyA3,         0,        0,    0, 0,  EFI_RIGHT_LOGO_MODIFIER,      0},
  },
  1,                          // DescriptionCount
  {'e', 'n', '-', 'U', 'S'},  // RFC4646 language code
  ' ',                        // Space
  {'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', '\0'}, // DescriptionString[]
};

//
// EFI_KEY to USB Keycode conversion table
// EFI_KEY is defined in UEFI spec.
// USB Keycode is defined in USB HID Firmware spec.
//
UINT8 EfiKeyToUsbKeyCodeConvertionTable[] = {
  0xe0,  //  EfiKeyLCtrl
  0xe3,  //  EfiKeyA0
  0xe2,  //  EfiKeyLAlt
  0x2c,  //  EfiKeySpaceBar
  0xe6,  //  EfiKeyA2
  0xe7,  //  EfiKeyA3
  0x65,  //  EfiKeyA4
  0xe4,  //  EfiKeyRCtrl
  0x50,  //  EfiKeyLeftArrow
  0x51,  //  EfiKeyDownArrow
  0x4F,  //  EfiKeyRightArrow
  0x62,  //  EfiKeyZero
  0x63,  //  EfiKeyPeriod
  0x28,  //  EfiKeyEnter
  0xe1,  //  EfiKeyLShift
  0x64,  //  EfiKeyB0
  0x1D,  //  EfiKeyB1
  0x1B,  //  EfiKeyB2
  0x06,  //  EfiKeyB3
  0x19,  //  EfiKeyB4
  0x05,  //  EfiKeyB5
  0x11,  //  EfiKeyB6
  0x10,  //  EfiKeyB7
  0x36,  //  EfiKeyB8
  0x37,  //  EfiKeyB9
  0x38,  //  EfiKeyB10
  0xe5,  //  EfiKeyRShift
  0x52,  //  EfiKeyUpArrow
  0x59,  //  EfiKeyOne
  0x5A,  //  EfiKeyTwo
  0x5B,  //  EfiKeyThree
  0x39,  //  EfiKeyCapsLock
  0x04,  //  EfiKeyC1
  0x16,  //  EfiKeyC2
  0x07,  //  EfiKeyC3
  0x09,  //  EfiKeyC4
  0x0A,  //  EfiKeyC5
  0x0B,  //  EfiKeyC6
  0x0D,  //  EfiKeyC7
  0x0E,  //  EfiKeyC8
  0x0F,  //  EfiKeyC9
  0x33,  //  EfiKeyC10
  0x34,  //  EfiKeyC11
  0x32,  //  EfiKeyC12
  0x5C,  //  EfiKeyFour
  0x5D,  //  EfiKeyFive
  0x5E,  //  EfiKeySix
  0x57,  //  EfiKeyPlus
  0x2B,  //  EfiKeyTab
  0x14,  //  EfiKeyD1
  0x1A,  //  EfiKeyD2
  0x08,  //  EfiKeyD3
  0x15,  //  EfiKeyD4
  0x17,  //  EfiKeyD5
  0x1C,  //  EfiKeyD6
  0x18,  //  EfiKeyD7
  0x0C,  //  EfiKeyD8
  0x12,  //  EfiKeyD9
  0x13,  //  EfiKeyD10
  0x2F,  //  EfiKeyD11
  0x30,  //  EfiKeyD12
  0x31,  //  EfiKeyD13
  0x4C,  //  EfiKeyDel
  0x4D,  //  EfiKeyEnd
  0x4E,  //  EfiKeyPgDn
  0x5F,  //  EfiKeySeven
  0x60,  //  EfiKeyEight
  0x61,  //  EfiKeyNine
  0x35,  //  EfiKeyE0
  0x1E,  //  EfiKeyE1
  0x1F,  //  EfiKeyE2
  0x20,  //  EfiKeyE3
  0x21,  //  EfiKeyE4
  0x22,  //  EfiKeyE5
  0x23,  //  EfiKeyE6
  0x24,  //  EfiKeyE7
  0x25,  //  EfiKeyE8
  0x26,  //  EfiKeyE9
  0x27,  //  EfiKeyE10
  0x2D,  //  EfiKeyE11
  0x2E,  //  EfiKeyE12
  0x2A,  //  EfiKeyBackSpace
  0x49,  //  EfiKeyIns
  0x4A,  //  EfiKeyHome
  0x4B,  //  EfiKeyPgUp
  0x53,  //  EfiKeyNLck
  0x54,  //  EfiKeySlash
  0x55,  //  EfiKeyAsterisk
  0x56,  //  EfiKeyMinus
  0x29,  //  EfiKeyEsc
  0x3A,  //  EfiKeyF1
  0x3B,  //  EfiKeyF2
  0x3C,  //  EfiKeyF3
  0x3D,  //  EfiKeyF4
  0x3E,  //  EfiKeyF5
  0x3F,  //  EfiKeyF6
  0x40,  //  EfiKeyF7
  0x41,  //  EfiKeyF8
  0x42,  //  EfiKeyF9
  0x43,  //  EfiKeyF10
  0x44,  //  EfiKeyF11
  0x45,  //  EfiKeyF12
  0x46,  //  EfiKeyPrint
  0x47,  //  EfiKeySLck
  0x48   //  EfiKeyPause
};

//
// Keyboard modifier value to EFI Scan Code conversion table
// EFI Scan Code and the modifier values are defined in UEFI spec.
//
UINT8 ModifierValueToEfiScanCodeConvertionTable[] = {
  SCAN_NULL,       // EFI_NULL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_ALT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_ALT_MODIFIER
  SCAN_NULL,       // EFI_ALT_GR_MODIFIER
  SCAN_INSERT,     // EFI_INSERT_MODIFIER
  SCAN_DELETE,     // EFI_DELETE_MODIFIER
  SCAN_PAGE_DOWN,  // EFI_PAGE_DOWN_MODIFIER
  SCAN_PAGE_UP,    // EFI_PAGE_UP_MODIFIER
  SCAN_HOME,       // EFI_HOME_MODIFIER
  SCAN_END,        // EFI_END_MODIFIER
  SCAN_NULL,       // EFI_LEFT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_CAPS_LOCK_MODIFIER
  SCAN_NULL,       // EFI_NUM_LOCK_MODIFIER
  SCAN_LEFT,       // EFI_LEFT_ARROW_MODIFIER
  SCAN_RIGHT,      // EFI_RIGHT_ARROW_MODIFIER
  SCAN_DOWN,       // EFI_DOWN_ARROW_MODIFIER
  SCAN_UP,         // EFI_UP_ARROW_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_DEPENDENCY_MODIFIER
  SCAN_F1,         // EFI_FUNCTION_KEY_ONE_MODIFIER
  SCAN_F2,         // EFI_FUNCTION_KEY_TWO_MODIFIER
  SCAN_F3,         // EFI_FUNCTION_KEY_THREE_MODIFIER
  SCAN_F4,         // EFI_FUNCTION_KEY_FOUR_MODIFIER
  SCAN_F5,         // EFI_FUNCTION_KEY_FIVE_MODIFIER
  SCAN_F6,         // EFI_FUNCTION_KEY_SIX_MODIFIER
  SCAN_F7,         // EFI_FUNCTION_KEY_SEVEN_MODIFIER
  SCAN_F8,         // EFI_FUNCTION_KEY_EIGHT_MODIFIER
  SCAN_F9,         // EFI_FUNCTION_KEY_NINE_MODIFIER
  SCAN_F10,        // EFI_FUNCTION_KEY_TEN_MODIFIER
  SCAN_F11,        // EFI_FUNCTION_KEY_ELEVEN_MODIFIER
  SCAN_F12,        // EFI_FUNCTION_KEY_TWELVE_MODIFIER
  //
  // For Partial Keystroke support
  //
  SCAN_NULL,       // EFI_PRINT_MODIFIER
  SCAN_NULL,       // EFI_SYS_REQUEST_MODIFIER
  SCAN_NULL,       // EFI_SCROLL_LOCK_MODIFIER
  SCAN_PAUSE,      // EFI_PAUSE_MODIFIER
  SCAN_NULL,       // EFI_BREAK_MODIFIER
  SCAN_NULL,       // EFI_LEFT_LOGO_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_LOGO_MODIFER
  SCAN_NULL,       // EFI_MENU_MODIFER
};


/**
  Uses USB I/O to check whether the device is a USB keyboard device.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is a USB keyboard device.
  @retval FALSE    Device is a not USB keyboard device.

**/
BOOLEAN
IsUSBKeyboard (
  IN  EFI_USB_IO_PROTOCOL       *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Get the default interface descriptor
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (InterfaceDescriptor.InterfaceClass == CLASS_HID &&
      InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT &&
      InterfaceDescriptor.InterfaceProtocol == PROTOCOL_KEYBOARD
      ) {
    return TRUE;
  }

  return FALSE;
}

/**
  Find Key Descriptor in Key Convertion Table given its USB keycode.

  @param  UsbKeyboardDevice   The USB_KB_DEV instance.
  @param  KeyCode             USB Keycode.

  @return The Key Descriptor in Key Convertion Table.
          NULL means not found.

**/
EFI_KEY_DESCRIPTOR *
GetKeyDescriptor (
  IN USB_KB_DEV        *UsbKeyboardDevice,
  IN UINT8             KeyCode
  )
{
  UINT8  Index;

  //
  // Make sure KeyCode is in the range of [0x4, 0x65] or [0xe0, 0xe7]
  //
  if ((!USBKBD_VALID_KEYCODE (KeyCode)) || ((KeyCode > 0x65) && (KeyCode < 0xe0)) || (KeyCode > 0xe7)) {
    return NULL;
  }

  //
  // Calculate the index of Key Descriptor in Key Convertion Table
  //
  if (KeyCode <= 0x65) {
    Index = (UINT8) (KeyCode - 4);
  } else {
    Index = (UINT8) (KeyCode - 0xe0 + NUMBER_OF_VALID_NON_MODIFIER_USB_KEYCODE);
  }

  return &UsbKeyboardDevice->KeyConvertionTable[Index];
}

/**
  Find Non-Spacing key for given Key descriptor.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyDescriptor        Key descriptor.

  @return The Non-Spacing key corresponding to KeyDescriptor
          NULL means not found.

**/
USB_NS_KEY *
FindUsbNsKey (
  IN USB_KB_DEV          *UsbKeyboardDevice,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  LIST_ENTRY      *Link;
  LIST_ENTRY      *NsKeyList;
  USB_NS_KEY      *UsbNsKey;

  NsKeyList = &UsbKeyboardDevice->NsKeyList;
  Link = GetFirstNode (NsKeyList);
  while (!IsNull (NsKeyList, Link)) {
    UsbNsKey = USB_NS_KEY_FORM_FROM_LINK (Link);

    if (UsbNsKey->NsKey[0].Key == KeyDescriptor->Key) {
      return UsbNsKey;
    }

    Link = GetNextNode (NsKeyList, Link);
  }

  return NULL;
}

/**
  Find physical key definition for a given key descriptor.

  For a specified non-spacing key, there are a list of physical
  keys following it. This function traverses the list of
  physical keys and tries to find the physical key matching
  the KeyDescriptor.

  @param  UsbNsKey          The non-spacing key information.
  @param  KeyDescriptor     The key descriptor.

  @return The physical key definition.
          If no physical key is found, parameter KeyDescriptor is returned.

**/
EFI_KEY_DESCRIPTOR *
FindPhysicalKey (
  IN USB_NS_KEY          *UsbNsKey,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  UINTN               Index;
  EFI_KEY_DESCRIPTOR  *PhysicalKey;

  PhysicalKey = &UsbNsKey->NsKey[1];
  for (Index = 0; Index < UsbNsKey->KeyCount; Index++) {
    if (KeyDescriptor->Key == PhysicalKey->Key) {
      return PhysicalKey;
    }

    PhysicalKey++;
  }

  //
  // No children definition matched, return original key
  //
  return KeyDescriptor;
}

/**
  Destroy resources for keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

**/
VOID
ReleaseKeyboardLayoutResources (
  IN OUT USB_KB_DEV              *UsbKeyboardDevice
  )
{
  USB_NS_KEY      *UsbNsKey;
  LIST_ENTRY      *Link;

  if (UsbKeyboardDevice->KeyConvertionTable != NULL) {
    FreePool (UsbKeyboardDevice->KeyConvertionTable);
  }
  UsbKeyboardDevice->KeyConvertionTable = NULL;

  if (UsbKeyboardDevice->NsKeyList.ForwardLink == NULL) {
    return;
  }

  while (!IsListEmpty (&UsbKeyboardDevice->NsKeyList)) {
    Link = GetFirstNode (&UsbKeyboardDevice->NsKeyList);
    UsbNsKey = USB_NS_KEY_FORM_FROM_LINK (Link);
    RemoveEntryList (&UsbNsKey->Link);

    FreePool (UsbNsKey->NsKey);
    FreePool (UsbNsKey);
  }
}

/**
  Initialize USB keyboard layout.

  This function initializes Key Convertion Table for the USB keyboard device.
  It first tries to retrieve layout from HII database. If failed and default
  layout is enabled, then it just uses the default layout.

  @param  UsbKeyboardDevice      The USB_KB_DEV instance.

  @retval EFI_SUCCESS            Initialization succeeded.
  @retval EFI_NOT_READY          Keyboard layout cannot be retrieve from HII
                                 database, and default layout is disabled.
  @retval Other                  Fail to register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group.

**/
EFI_STATUS
InitKeyboardLayout (
  OUT USB_KB_DEV   *UsbKeyboardDevice
  )
{
  EFI_KEY_DESCRIPTOR        TempKey;
  EFI_KEY_DESCRIPTOR        *KeyDescriptor;
  EFI_KEY_DESCRIPTOR        *TableEntry;
  EFI_KEY_DESCRIPTOR        *NsKey;
  USB_NS_KEY                *UsbNsKey;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     KeyCount;
  UINT8                     KeyCode;

  UsbKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((NUMBER_OF_VALID_USB_KEYCODE) * sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (UsbKeyboardDevice->KeyConvertionTable != NULL);

  InitializeListHead (&UsbKeyboardDevice->NsKeyList);
  UsbKeyboardDevice->CurrentNsKey = NULL;

  //
  // Traverse the list of key descriptors following the header of EFI_HII_KEYBOARD_LAYOUT
  //
  KeyDescriptor = (EFI_KEY_DESCRIPTOR *) (&mUsbKeyboardLayoutBin.KeyDescriptor[0]);
  for (Index = 0; Index < mUsbKeyboardLayoutBin.DescriptorCount; Index++) {
    //
    // Copy from HII keyboard layout package binary for alignment
    //
    CopyMem (&TempKey, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // Fill the key into KeyConvertionTable, whose index is calculated from USB keycode.
    //
    KeyCode = EfiKeyToUsbKeyCodeConvertionTable [(UINT8) (TempKey.Key)];
    TableEntry = GetKeyDescriptor (UsbKeyboardDevice, KeyCode);
    if (TableEntry == NULL) {
      ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // For non-spacing key, create the list with a non-spacing key followed by physical keys.
    //
    if (TempKey.Modifier == EFI_NS_KEY_MODIFIER) {
      UsbNsKey = AllocateZeroPool (sizeof (USB_NS_KEY));
      ASSERT (UsbNsKey != NULL);

      //
      // Search for sequential children physical key definitions
      //
      KeyCount = 0;
      NsKey = KeyDescriptor + 1;
      for (Index2 = (UINT8) Index + 1; Index2 < mUsbKeyboardLayoutBin.DescriptorCount; Index2++) {
        CopyMem (&TempKey, NsKey, sizeof (EFI_KEY_DESCRIPTOR));
        if (TempKey.Modifier == EFI_NS_KEY_DEPENDENCY_MODIFIER) {
          KeyCount++;
        } else {
          break;
        }
        NsKey++;
      }

      UsbNsKey->Signature = USB_NS_KEY_SIGNATURE;
      UsbNsKey->KeyCount = KeyCount;
      UsbNsKey->NsKey = AllocateCopyPool (
                          (KeyCount + 1) * sizeof (EFI_KEY_DESCRIPTOR),
                          KeyDescriptor
                          );
      InsertTailList (&UsbKeyboardDevice->NsKeyList, &UsbNsKey->Link);

      //
      // Skip over the child physical keys
      //
      Index += KeyCount;
      KeyDescriptor += KeyCount;
    }

    KeyDescriptor++;
  }

  //
  // There are two EfiKeyEnter, duplicate its key descriptor
  //
  TableEntry = GetKeyDescriptor (UsbKeyboardDevice, 0x58);
  KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, 0x28);
  CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

  return EFI_SUCCESS;
}


/**
  Initialize USB keyboard device and all private data structures.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Keyboard initialization failed.

**/
EFI_STATUS
InitUSBKeyboard (
  IN OUT USB_KB_DEV   *UsbKeyboardDevice
  )
{
  UINT16              ConfigValue;
  UINT8               Protocol;
  EFI_STATUS          Status;
  UINT32              TransferResult;

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST),
    UsbKeyboardDevice->DevicePath
    );

  InitQueue (&UsbKeyboardDevice->UsbKeyQueue, sizeof (USB_KEY));
  InitQueue (&UsbKeyboardDevice->EfiKeyQueue, sizeof (EFI_KEY_DATA));
  InitQueue (&UsbKeyboardDevice->EfiKeyQueueForNotify, sizeof (EFI_KEY_DATA));

  //
  // Use the config out of the descriptor
  // Assumed the first config is the correct one and this is not always the case
  //
  Status = UsbGetConfiguration (
             UsbKeyboardDevice->UsbIo,
             &ConfigValue,
             &TransferResult
             );
  if (EFI_ERROR (Status)) {
    ConfigValue = 0x01;
    //
    // Uses default configuration to configure the USB Keyboard device.
    //
    Status = UsbSetConfiguration (
               UsbKeyboardDevice->UsbIo,
               ConfigValue,
               &TransferResult
               );
    if (EFI_ERROR (Status)) {
      //
      // If configuration could not be set here, it means
      // the keyboard interface has some errors and could
      // not be initialized
      //
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INTERFACE_ERROR),
        UsbKeyboardDevice->DevicePath
        );

      return EFI_DEVICE_ERROR;
    }
  }

  UsbGetProtocolRequest (
    UsbKeyboardDevice->UsbIo,
    UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
    &Protocol
    );
  //
  // Set boot protocol for the USB Keyboard.
  // This driver only supports boot protocol.
  //
  if (Protocol != BOOT_PROTOCOL) {
    UsbSetProtocolRequest (
      UsbKeyboardDevice->UsbIo,
      UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
      BOOT_PROTOCOL
      );
  }

  UsbKeyboardDevice->CtrlOn     = FALSE;
  UsbKeyboardDevice->AltOn      = FALSE;
  UsbKeyboardDevice->ShiftOn    = FALSE;
  UsbKeyboardDevice->NumLockOn  = FALSE;
  UsbKeyboardDevice->CapsOn     = FALSE;
  UsbKeyboardDevice->ScrollOn   = FALSE;

  UsbKeyboardDevice->LeftCtrlOn   = FALSE;
  UsbKeyboardDevice->LeftAltOn    = FALSE;
  UsbKeyboardDevice->LeftShiftOn  = FALSE;
  UsbKeyboardDevice->LeftLogoOn   = FALSE;
  UsbKeyboardDevice->RightCtrlOn  = FALSE;
  UsbKeyboardDevice->RightAltOn   = FALSE;
  UsbKeyboardDevice->RightShiftOn = FALSE;
  UsbKeyboardDevice->RightLogoOn  = FALSE;
  UsbKeyboardDevice->MenuKeyOn    = FALSE;
  UsbKeyboardDevice->SysReqOn     = FALSE;

  UsbKeyboardDevice->AltGrOn      = FALSE;

  UsbKeyboardDevice->CurrentNsKey = NULL;

  //
  // Sync the initial state of lights on keyboard.
  //
  SetKeyLED (UsbKeyboardDevice);

  ZeroMem (UsbKeyboardDevice->LastKeyCodeArray, sizeof (UINT8) * 8);

  //
  // Create event for repeat keys' generation.
  //
  if (UsbKeyboardDevice->RepeatTimer != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->RepeatTimer);
    UsbKeyboardDevice->RepeatTimer = NULL;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_CALLBACK,
         USBKeyboardRepeatHandler,
         UsbKeyboardDevice,
         &UsbKeyboardDevice->RepeatTimer
         );

  //
  // Create event for delayed recovery, which deals with device error.
  //
  if (UsbKeyboardDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
    UsbKeyboardDevice->DelayedRecoveryEvent = NULL;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         USBKeyboardRecoveryHandler,
         UsbKeyboardDevice,
         &UsbKeyboardDevice->DelayedRecoveryEvent
         );

  return EFI_SUCCESS;
}


/**
  Handler function for USB keyboard's asynchronous interrupt transfer.

  This function is the handler function for USB keyboard's asynchronous interrupt transfer
  to manage the keyboard. It parses the USB keyboard input report, and inserts data to
  keyboard buffer according to state of modifer keys and normal keys. Timer for repeat key
  is also set accordingly.

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_KB_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Asynchronous interrupt transfer is handled successfully.
  @retval EFI_DEVICE_ERROR Hardware error occurs.

**/
EFI_STATUS
EFIAPI
KeyboardHandler (
  IN  VOID          *Data,
  IN  UINTN         DataLength,
  IN  VOID          *Context,
  IN  UINT32        Result
  )
{
  USB_KB_DEV          *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               *CurKeyCodeBuffer;
  UINT8               *OldKeyCodeBuffer;
  UINT8               CurModifierMap;
  UINT8               OldModifierMap;
  UINT8               Mask;
  UINTN               Index;
  UINT8               Index2;
  BOOLEAN             KeyRelease;
  BOOLEAN             KeyPress;
  USB_KEY             UsbKey;
  UINT8               NewRepeatKey;
  UINT32              UsbStatus;
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;
  UINTN               NumberOfKeyCodes;
  APPLE_KEY_CODE      KeyCodes[6];

  ASSERT (Context != NULL);

  NewRepeatKey      = 0;
  UsbKeyboardDevice = (USB_KB_DEV *) Context;
  UsbIo             = UsbKeyboardDevice->UsbIo;

  //
  // Analyzes Result and performs corresponding action.
  //
  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INPUT_ERROR),
      UsbKeyboardDevice->DevicePath
      );

    //
    // Stop the repeat key generation if any
    //
    UsbKeyboardDevice->RepeatKey = 0;

    gBS->SetTimer (
          UsbKeyboardDevice->RepeatTimer,
          TimerCancel,
          USBKBD_REPEAT_RATE
          );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      UsbClearEndpointHalt (
        UsbIo,
        UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
        &UsbStatus
        );
    }

    //
    // Delete & Submit this interrupt again
    // Handler of DelayedRecoveryEvent triggered by timer will re-submit the interrupt.
    //
    UsbIo->UsbAsyncInterruptTransfer (
             UsbIo,
             UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
             FALSE,
             0,
             0,
             NULL,
             NULL
             );
    //
    // EFI_USB_INTERRUPT_DELAY is defined in USB standard for error handling.
    //
    gBS->SetTimer (
           UsbKeyboardDevice->DelayedRecoveryEvent,
           TimerRelative,
           EFI_USB_INTERRUPT_DELAY
           );

    return EFI_DEVICE_ERROR;
  }

  //
  // If no error and no data, just return EFI_SUCCESS.
  //
  if (DataLength == 0 || Data == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Following code checks current keyboard input report against old key code buffer.
  // According to USB HID Firmware Specification, the report consists of 8 bytes.
  // Byte 0 is map of Modifier keys.
  // Byte 1 is reserved.
  // Bytes 2 to 7 are keycodes.
  //
  if (DataLength < 8) {
    return EFI_DEVICE_ERROR;
  }

  CurKeyCodeBuffer  = (UINT8 *) Data;
  OldKeyCodeBuffer  = UsbKeyboardDevice->LastKeyCodeArray;

  //
  // Checks for new key stroke.
  //
  for (Index = 0; Index < 8; Index++) {
    if (OldKeyCodeBuffer[Index] != CurKeyCodeBuffer[Index]) {
      break;
    }
  }

  //
  // If no new key, return EFI_SUCCESS immediately.
  //
  if (Index == 8) {
    return EFI_SUCCESS;
  }

  //
  // Parse the modifier key, which is the first byte of keyboard input report.
  //
  CurModifierMap  = CurKeyCodeBuffer[0];
  OldModifierMap  = OldKeyCodeBuffer[0];

  NumberOfKeyCodes = 0;

  //
  // Pass the data to the Apple protocol
  //
  for (Index = 2; Index < 8; Index++) {
    if (USBKBD_VALID_KEYCODE (CurKeyCodeBuffer[Index])) {
      KeyCodes[NumberOfKeyCodes] = APPLE_HID_USB_KB_KP_USAGE (
                                     CurKeyCodeBuffer[Index]
                                     );

      ++NumberOfKeyCodes;
    }
  }

  if (UsbKeyboardDevice->KeyMapDb != NULL) {
    UsbKeyboardDevice->KeyMapDb->SetKeyStrokeBufferKeys (
                                   UsbKeyboardDevice->KeyMapDb,
                                   UsbKeyboardDevice->KeyMapDbIndex,
                                   (APPLE_MODIFIER_MAP)CurModifierMap,
                                   NumberOfKeyCodes,
                                   &KeyCodes[0]
                                   );
  }

  //
  // Handle modifier key's pressing or releasing situation.
  // According to USB HID Firmware spec, Byte 0 uses following map of Modifier keys:
  // Bit0: Left Control,  Keycode: 0xe0
  // Bit1: Left Shift,    Keycode: 0xe1
  // Bit2: Left Alt,      Keycode: 0xe2
  // Bit3: Left GUI,      Keycode: 0xe3
  // Bit4: Right Control, Keycode: 0xe4
  // Bit5: Right Shift,   Keycode: 0xe5
  // Bit6: Right Alt,     Keycode: 0xe6
  // Bit7: Right GUI,     Keycode: 0xe7
  //
  for (Index = 0; Index < 8; Index++) {
    Mask = (UINT8) (1 << Index);
    if ((CurModifierMap & Mask) != (OldModifierMap & Mask)) {
      //
      // If current modifier key is up, then CurModifierMap & Mask = 0;
      // otherwise it is a non-zero value.
      // Insert the changed modifier key into key buffer.
      //
      UsbKey.KeyCode = (UINT8) (0xe0 + Index);
      UsbKey.Down    = (BOOLEAN) ((CurModifierMap & Mask) != 0);
      Enqueue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));
    }
  }

  //
  // Handle normal key's releasing situation
  // Bytes 2 to 7 are for normal keycodes
  //
  KeyRelease = FALSE;
  for (Index = 2; Index < 8; Index++) {

    if (!USBKBD_VALID_KEYCODE (OldKeyCodeBuffer[Index])) {
      continue;
    }
    //
    // For any key in old keycode buffer, if it is not in current keycode buffer,
    // then it is released. Otherwise, it is not released.
    //
    KeyRelease = TRUE;
    for (Index2 = 2; Index2 < 8; Index2++) {

      if (!USBKBD_VALID_KEYCODE (CurKeyCodeBuffer[Index2])) {
        continue;
      }

      if (OldKeyCodeBuffer[Index] == CurKeyCodeBuffer[Index2]) {
        KeyRelease = FALSE;
        break;
      }
    }

    if (KeyRelease) {
      UsbKey.KeyCode = OldKeyCodeBuffer[Index];
      UsbKey.Down    = FALSE;
      Enqueue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));
      //
      // The original repeat key is released.
      //
      if (OldKeyCodeBuffer[Index] == UsbKeyboardDevice->RepeatKey) {
        UsbKeyboardDevice->RepeatKey = 0;
      }
    }
  }

  //
  // If original repeat key is released, cancel the repeat timer
  //
  if (UsbKeyboardDevice->RepeatKey == 0) {
    gBS->SetTimer (
           UsbKeyboardDevice->RepeatTimer,
           TimerCancel,
           USBKBD_REPEAT_RATE
           );
  }

  //
  // Handle normal key's pressing situation
  //
  KeyPress = FALSE;
  for (Index = 2; Index < 8; Index++) {

    if (!USBKBD_VALID_KEYCODE (CurKeyCodeBuffer[Index])) {
      continue;
    }
    //
    // For any key in current keycode buffer, if it is not in old keycode buffer,
    // then it is pressed. Otherwise, it is not pressed.
    //
    KeyPress = TRUE;
    for (Index2 = 2; Index2 < 8; Index2++) {

      if (!USBKBD_VALID_KEYCODE (OldKeyCodeBuffer[Index2])) {
        continue;
      }

      if (CurKeyCodeBuffer[Index] == OldKeyCodeBuffer[Index2]) {
        KeyPress = FALSE;
        break;
      }
    }

    if (KeyPress) {
      UsbKey.KeyCode = CurKeyCodeBuffer[Index];
      UsbKey.Down    = TRUE;
      Enqueue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));

      //
      // Handle repeat key
      //
      KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, CurKeyCodeBuffer[Index]);
      if (KeyDescriptor == NULL) {
        continue;
      }

      if (KeyDescriptor->Modifier == EFI_NUM_LOCK_MODIFIER || KeyDescriptor->Modifier == EFI_CAPS_LOCK_MODIFIER) {
        //
        // For NumLock or CapsLock pressed, there is no need to handle repeat key for them.
        //
        UsbKeyboardDevice->RepeatKey = 0;
      } else {
        //
        // Prepare new repeat key, and clear the original one.
        //
        NewRepeatKey = CurKeyCodeBuffer[Index];
        UsbKeyboardDevice->RepeatKey = 0;
      }
    }
  }

  //
  // Update LastKeycodeArray buffer in the UsbKeyboardDevice data structure.
  //
  for (Index = 0; Index < 8; Index++) {
    UsbKeyboardDevice->LastKeyCodeArray[Index] = CurKeyCodeBuffer[Index];
  }

  //
  // If there is new key pressed, update the RepeatKey value, and set the
  // timer to repeat delay timer
  //
  if (NewRepeatKey != 0) {
    //
    // Sets trigger time to "Repeat Delay Time",
    // to trigger the repeat timer when the key is hold long
    // enough time.
    //
    gBS->SetTimer (
           UsbKeyboardDevice->RepeatTimer,
           TimerRelative,
           USBKBD_REPEAT_DELAY
           );
    UsbKeyboardDevice->RepeatKey = NewRepeatKey;
  }

  return EFI_SUCCESS;
}


/**
  Retrieves a USB keycode after parsing the raw data in keyboard buffer.

  This function parses keyboard buffer. It updates state of modifier key for
  USB_KB_DEV instancem, and returns keycode for output.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyCode              Pointer to the USB keycode for output.

  @retval EFI_SUCCESS          Keycode successfully parsed.
  @retval EFI_NOT_READY        Keyboard buffer is not ready for a valid keycode

**/
EFI_STATUS
USBParseKey (
  IN OUT  USB_KB_DEV  *UsbKeyboardDevice,
     OUT  UINT8       *KeyCode
  )
{
  USB_KEY             UsbKey;
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;

  *KeyCode = 0;

  while (!IsQueueEmpty (&UsbKeyboardDevice->UsbKeyQueue)) {
    //
    // Pops one raw data off.
    //
    Dequeue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));

    KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, UsbKey.KeyCode);
    if (KeyDescriptor == NULL) {
      continue;
    }
    if (!UsbKey.Down) {
      //
      // Key is released.
      //
      switch (KeyDescriptor->Modifier) {

      //
      // Ctrl release
      //
      case EFI_LEFT_CONTROL_MODIFIER:
        UsbKeyboardDevice->LeftCtrlOn = FALSE;
        UsbKeyboardDevice->CtrlOn = FALSE;
        break;
      case EFI_RIGHT_CONTROL_MODIFIER:
        UsbKeyboardDevice->RightCtrlOn = FALSE;
        UsbKeyboardDevice->CtrlOn = FALSE;
        break;

      //
      // Shift release
      //
      case EFI_LEFT_SHIFT_MODIFIER:
        UsbKeyboardDevice->LeftShiftOn = FALSE;
        UsbKeyboardDevice->ShiftOn = FALSE;
        break;
      case EFI_RIGHT_SHIFT_MODIFIER:
        UsbKeyboardDevice->RightShiftOn = FALSE;
        UsbKeyboardDevice->ShiftOn = FALSE;
        break;

      //
      // Alt release
      //
      case EFI_LEFT_ALT_MODIFIER:
        UsbKeyboardDevice->LeftAltOn = FALSE;
        UsbKeyboardDevice->AltOn = FALSE;
        break;
      case EFI_RIGHT_ALT_MODIFIER:
        UsbKeyboardDevice->RightAltOn = FALSE;
        UsbKeyboardDevice->AltOn = FALSE;
        break;

      //
      // Left Logo release
      //
      case EFI_LEFT_LOGO_MODIFIER:
        UsbKeyboardDevice->LeftLogoOn = FALSE;
        break;

      //
      // Right Logo release
      //
      case EFI_RIGHT_LOGO_MODIFIER:
        UsbKeyboardDevice->RightLogoOn = FALSE;
        break;

      //
      // Menu key release
      //
      case EFI_MENU_MODIFIER:
        UsbKeyboardDevice->MenuKeyOn = FALSE;
        break;

      //
      // SysReq release
      //
      case EFI_PRINT_MODIFIER:
      case EFI_SYS_REQUEST_MODIFIER:
        UsbKeyboardDevice->SysReqOn = FALSE;
        break;

      //
      // AltGr release
      //
      case EFI_ALT_GR_MODIFIER:
        UsbKeyboardDevice->AltGrOn = FALSE;
        break;

      default:
        break;
      }

      continue;
    }

    //
    // Analyzes key pressing situation
    //
    switch (KeyDescriptor->Modifier) {

    //
    // Ctrl press
    //
    case EFI_LEFT_CONTROL_MODIFIER:
      UsbKeyboardDevice->LeftCtrlOn = TRUE;
      UsbKeyboardDevice->CtrlOn = TRUE;
      break;
    case EFI_RIGHT_CONTROL_MODIFIER:
      UsbKeyboardDevice->RightCtrlOn = TRUE;
      UsbKeyboardDevice->CtrlOn = TRUE;
      break;

    //
    // Shift press
    //
    case EFI_LEFT_SHIFT_MODIFIER:
      UsbKeyboardDevice->LeftShiftOn = TRUE;
      UsbKeyboardDevice->ShiftOn = TRUE;
      break;
    case EFI_RIGHT_SHIFT_MODIFIER:
      UsbKeyboardDevice->RightShiftOn = TRUE;
      UsbKeyboardDevice->ShiftOn = TRUE;
      break;

    //
    // Alt press
    //
    case EFI_LEFT_ALT_MODIFIER:
      UsbKeyboardDevice->LeftAltOn = TRUE;
      UsbKeyboardDevice->AltOn = TRUE;
      break;
    case EFI_RIGHT_ALT_MODIFIER:
      UsbKeyboardDevice->RightAltOn = TRUE;
      UsbKeyboardDevice->AltOn = TRUE;
      break;

    //
    // Left Logo press
    //
    case EFI_LEFT_LOGO_MODIFIER:
      UsbKeyboardDevice->LeftLogoOn = TRUE;
      break;

    //
    // Right Logo press
    //
    case EFI_RIGHT_LOGO_MODIFIER:
      UsbKeyboardDevice->RightLogoOn = TRUE;
      break;

    //
    // Menu key press
    //
    case EFI_MENU_MODIFIER:
      UsbKeyboardDevice->MenuKeyOn = TRUE;
      break;

    //
    // SysReq press
    //
    case EFI_PRINT_MODIFIER:
    case EFI_SYS_REQUEST_MODIFIER:
      UsbKeyboardDevice->SysReqOn = TRUE;
      break;

    //
    // AltGr press
    //
    case EFI_ALT_GR_MODIFIER:
      UsbKeyboardDevice->AltGrOn = TRUE;
      break;

    case EFI_NUM_LOCK_MODIFIER:
      //
      // Toggle NumLock
      //
      UsbKeyboardDevice->NumLockOn = (BOOLEAN) (!(UsbKeyboardDevice->NumLockOn));
      SetKeyLED (UsbKeyboardDevice);
      break;

    case EFI_CAPS_LOCK_MODIFIER:
      //
      // Toggle CapsLock
      //
      UsbKeyboardDevice->CapsOn = (BOOLEAN) (!(UsbKeyboardDevice->CapsOn));
      SetKeyLED (UsbKeyboardDevice);
      break;

    case EFI_SCROLL_LOCK_MODIFIER:
      //
      // Toggle ScrollLock
      //
      UsbKeyboardDevice->ScrollOn = (BOOLEAN) (!(UsbKeyboardDevice->ScrollOn));
      SetKeyLED (UsbKeyboardDevice);
      break;

    default:
      break;
    }

    //
    // When encountering Ctrl + Alt + Del, then warm reset.
    //
    if (KeyDescriptor->Modifier == EFI_DELETE_MODIFIER) {
      if ((UsbKeyboardDevice->CtrlOn) && (UsbKeyboardDevice->AltOn)) {
        //
        // CHANGE: Use library call to be able to centrally alter the behaviour.
        //
        ResetWarm ();
      }
    }

    *KeyCode = UsbKey.KeyCode;
    return EFI_SUCCESS;
  }

  return EFI_NOT_READY;
}

/**
  Initialize the key state.

  @param  UsbKeyboardDevice     The USB_KB_DEV instance.
  @param  KeyState              A pointer to receive the key state information.
**/
VOID
InitializeKeyState (
  IN  USB_KB_DEV           *UsbKeyboardDevice,
  OUT EFI_KEY_STATE        *KeyState
  )
{
  KeyState->KeyShiftState  = EFI_SHIFT_STATE_VALID;
  KeyState->KeyToggleState = EFI_TOGGLE_STATE_VALID;

  if (UsbKeyboardDevice->LeftCtrlOn) {
    KeyState->KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }
  if (UsbKeyboardDevice->RightCtrlOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_CONTROL_PRESSED;
  }
  if (UsbKeyboardDevice->LeftAltOn) {
    KeyState->KeyShiftState |= EFI_LEFT_ALT_PRESSED;
  }
  if (UsbKeyboardDevice->RightAltOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_ALT_PRESSED;
  }
  if (UsbKeyboardDevice->LeftShiftOn) {
    KeyState->KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }
  if (UsbKeyboardDevice->RightShiftOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_SHIFT_PRESSED;
  }
  if (UsbKeyboardDevice->LeftLogoOn) {
    KeyState->KeyShiftState |= EFI_LEFT_LOGO_PRESSED;
  }
  if (UsbKeyboardDevice->RightLogoOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_LOGO_PRESSED;
  }
  if (UsbKeyboardDevice->MenuKeyOn) {
    KeyState->KeyShiftState |= EFI_MENU_KEY_PRESSED;
  }
  if (UsbKeyboardDevice->SysReqOn) {
    KeyState->KeyShiftState |= EFI_SYS_REQ_PRESSED;
  }

  if (UsbKeyboardDevice->ScrollOn) {
    KeyState->KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }
  if (UsbKeyboardDevice->NumLockOn) {
    KeyState->KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  }
  if (UsbKeyboardDevice->CapsOn) {
    KeyState->KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }
  if (UsbKeyboardDevice->IsSupportPartialKey) {
    KeyState->KeyToggleState |= EFI_KEY_STATE_EXPOSED;
  }
}

/**
  Converts USB Keycode ranging from 0x4 to 0x65 to EFI_INPUT_KEY.

  @param  UsbKeyboardDevice     The USB_KB_DEV instance.
  @param  KeyCode               Indicates the key code that will be interpreted.
  @param  KeyData               A pointer to a buffer that is filled in with
                                the keystroke information for the key that
                                was pressed.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER KeyCode is not in the range of 0x4 to 0x65.
  @retval EFI_INVALID_PARAMETER Translated EFI_INPUT_KEY has zero for both ScanCode and UnicodeChar.
  @retval EFI_NOT_READY         KeyCode represents a dead key with EFI_NS_KEY_MODIFIER
  @retval EFI_DEVICE_ERROR      Keyboard layout is invalid.

**/
EFI_STATUS
UsbKeyCodeToEfiInputKey (
  IN  USB_KB_DEV                *UsbKeyboardDevice,
  IN  UINT8                     KeyCode,
  OUT EFI_KEY_DATA              *KeyData
  )
{
  EFI_KEY_DESCRIPTOR            *KeyDescriptor;
  LIST_ENTRY                    *Link;
  LIST_ENTRY                    *NotifyList;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY *CurrentNotify;

  //
  // KeyCode must in the range of  [0x4, 0x65] or [0xe0, 0xe7].
  //
  KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, KeyCode);
  if (KeyDescriptor == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (KeyDescriptor->Modifier == EFI_NS_KEY_MODIFIER) {
    //
    // If this is a dead key with EFI_NS_KEY_MODIFIER, then record it and return.
    //
    UsbKeyboardDevice->CurrentNsKey = FindUsbNsKey (UsbKeyboardDevice, KeyDescriptor);
    return EFI_NOT_READY;
  }

  if (UsbKeyboardDevice->CurrentNsKey != NULL) {
    //
    // If this keystroke follows a non-spacing key, then find the descriptor for corresponding
    // physical key.
    //
    KeyDescriptor = FindPhysicalKey (UsbKeyboardDevice->CurrentNsKey, KeyDescriptor);
    UsbKeyboardDevice->CurrentNsKey = NULL;
  }

  //
  // Make sure modifier of Key Descriptor is in the valid range according to UEFI spec.
  //
  if (KeyDescriptor->Modifier >= (sizeof (ModifierValueToEfiScanCodeConvertionTable) / sizeof (UINT8))) {
    return EFI_DEVICE_ERROR;
  }

  KeyData->Key.ScanCode    = ModifierValueToEfiScanCodeConvertionTable[KeyDescriptor->Modifier];
  KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_STANDARD_SHIFT)!= 0) {
    if (UsbKeyboardDevice->ShiftOn) {
      KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedUnicode;

      //
      // Need not return associated shift state if a class of printable characters that
      // are normally adjusted by shift modifiers. e.g. Shift Key + 'f' key = 'F'
      //
      if ((KeyDescriptor->Unicode != CHAR_NULL) && (KeyDescriptor->ShiftedUnicode != CHAR_NULL) &&
          (KeyDescriptor->Unicode != KeyDescriptor->ShiftedUnicode)) {
        UsbKeyboardDevice->LeftShiftOn = FALSE;
        UsbKeyboardDevice->RightShiftOn = FALSE;
      }

      if (UsbKeyboardDevice->AltGrOn) {
        KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedAltGrUnicode;
      }
    } else {
      //
      // Shift off
      //
      KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;

      if (UsbKeyboardDevice->AltGrOn) {
        KeyData->Key.UnicodeChar = KeyDescriptor->AltGrUnicode;
      }
    }
  }

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_CAPS_LOCK) != 0) {
    if (UsbKeyboardDevice->CapsOn) {
      if (KeyData->Key.UnicodeChar == KeyDescriptor->Unicode) {
        KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedUnicode;
      } else if (KeyData->Key.UnicodeChar == KeyDescriptor->ShiftedUnicode) {
        KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;
      }
    }
  }

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_NUM_LOCK) != 0) {
    //
    // For key affected by NumLock, if NumLock is on and Shift is not pressed, then it means
    // normal key, instead of original control key. So the ScanCode should be cleaned.
    // Otherwise, it means control key, so preserve the EFI Scan Code and clear the unicode keycode.
    //
    if ((UsbKeyboardDevice->NumLockOn) && (!(UsbKeyboardDevice->ShiftOn))) {
      KeyData->Key.ScanCode = SCAN_NULL;
    } else {
      KeyData->Key.UnicodeChar = CHAR_NULL;
    }
  }

  //
  // Translate Unicode 0x1B (ESC) to EFI Scan Code
  //
  if (KeyData->Key.UnicodeChar == 0x1B && KeyData->Key.ScanCode == SCAN_NULL) {
    KeyData->Key.ScanCode = SCAN_ESC;
    KeyData->Key.UnicodeChar = CHAR_NULL;
  }

  //
  // Not valid for key without both unicode key code and EFI Scan Code.
  //
  if (KeyData->Key.UnicodeChar == 0 && KeyData->Key.ScanCode == SCAN_NULL) {
    if (!UsbKeyboardDevice->IsSupportPartialKey) {
    return EFI_NOT_READY;
    }
  }

  //
  // Save Shift/Toggle state
  //
  InitializeKeyState (UsbKeyboardDevice, &KeyData->KeyState);

  //
  // Signal KeyNotify process event if this key pressed matches any key registered.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (Link, KEYBOARD_CONSOLE_IN_EX_NOTIFY, NotifyEntry, USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      //
      // The key notification function needs to run at TPL_CALLBACK
      // while current TPL is TPL_NOTIFY. It will be invoked in
      // KeyNotifyProcessHandler() which runs at TPL_CALLBACK.
      //
      Enqueue (&UsbKeyboardDevice->EfiKeyQueueForNotify, KeyData, sizeof (*KeyData));
      gBS->SignalEvent (UsbKeyboardDevice->KeyNotifyProcessEvent);
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Create the queue.

  @param  Queue     Points to the queue.
  @param  ItemSize  Size of the single item.

**/
VOID
InitQueue (
  IN OUT  USB_SIMPLE_QUEUE   *Queue,
  IN      UINTN              ItemSize
  )
{
  UINTN                      Index;

  Queue->ItemSize  = ItemSize;
  Queue->Head      = 0;
  Queue->Tail      = 0;

  if (Queue->Buffer[0] != NULL) {
    FreePool (Queue->Buffer[0]);
  }

  Queue->Buffer[0] = AllocatePool (sizeof (Queue->Buffer) / sizeof (Queue->Buffer[0]) * ItemSize);
  ASSERT (Queue->Buffer[0] != NULL);

  for (Index = 1; Index < sizeof (Queue->Buffer) / sizeof (Queue->Buffer[0]); Index++) {
    Queue->Buffer[Index] = ((UINT8 *) Queue->Buffer[Index - 1]) + ItemSize;
  }
}

/**
  Destroy the queue

  @param Queue    Points to the queue.
**/
VOID
DestroyQueue (
  IN OUT USB_SIMPLE_QUEUE   *Queue
  )
{
  FreePool (Queue->Buffer[0]);
}


/**
  Check whether the queue is empty.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is empty.
  @retval FALSE     Queue is not empty.

**/
BOOLEAN
IsQueueEmpty (
  IN  USB_SIMPLE_QUEUE   *Queue
  )
{
  //
  // Meet FIFO empty condition
  //
  return (BOOLEAN) (Queue->Head == Queue->Tail);
}


/**
  Check whether the queue is full.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is full.
  @retval FALSE     Queue is not full.

**/
BOOLEAN
IsQueueFull (
  IN  USB_SIMPLE_QUEUE   *Queue
  )
{
  return (BOOLEAN) (((Queue->Tail + 1) % (MAX_KEY_ALLOWED + 1)) == Queue->Head);
}


/**
  Enqueue the item to the queue.

  @param  Queue     Points to the queue.
  @param  Item      Points to the item to be enqueued.
  @param  ItemSize  Size of the item.
**/
VOID
Enqueue (
  IN OUT  USB_SIMPLE_QUEUE *Queue,
  IN      VOID             *Item,
  IN      UINTN            ItemSize
  )
{
  ASSERT (ItemSize == Queue->ItemSize);
  //
  // If keyboard buffer is full, throw the
  // first key out of the keyboard buffer.
  //
  if (IsQueueFull (Queue)) {
    Queue->Head = (Queue->Head + 1) % (MAX_KEY_ALLOWED + 1);
  }

  CopyMem (Queue->Buffer[Queue->Tail], Item, ItemSize);

  //
  // Adjust the tail pointer of the FIFO keyboard buffer.
  //
  Queue->Tail = (Queue->Tail + 1) % (MAX_KEY_ALLOWED + 1);
}


/**
  Dequeue a item from the queue.

  @param  Queue     Points to the queue.
  @param  Item      Receives the item.
  @param  ItemSize  Size of the item.

  @retval EFI_SUCCESS        Item was successfully dequeued.
  @retval EFI_DEVICE_ERROR   The queue is empty.

**/
EFI_STATUS
Dequeue (
  IN OUT  USB_SIMPLE_QUEUE *Queue,
     OUT  VOID             *Item,
  IN      UINTN            ItemSize
  )
{
  ASSERT (Queue->ItemSize == ItemSize);

  if (IsQueueEmpty (Queue)) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (Item, Queue->Buffer[Queue->Head], ItemSize);

  //
  // Adjust the head pointer of the FIFO keyboard buffer.
  //
  Queue->Head = (Queue->Head + 1) % (MAX_KEY_ALLOWED + 1);

  return EFI_SUCCESS;
}


/**
  Sets USB keyboard LED state.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

**/
VOID
SetKeyLED (
  IN  USB_KB_DEV    *UsbKeyboardDevice
  )
{
  LED_MAP Led;
  UINT8   ReportId;

  //
  // Set each field in Led map.
  //
  Led.NumLock    = (UINT8) ((UsbKeyboardDevice->NumLockOn) ? 1 : 0);
  Led.CapsLock   = (UINT8) ((UsbKeyboardDevice->CapsOn) ? 1 : 0);
  Led.ScrollLock = (UINT8) ((UsbKeyboardDevice->ScrollOn) ? 1 : 0);
  Led.Resrvd     = 0;

  ReportId       = 0;
  //
  // Call Set_Report Request to lighten the LED.
  //
  UsbSetReportRequest (
    UsbKeyboardDevice->UsbIo,
    UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
    ReportId,
    HID_OUTPUT_REPORT,
    1,
    (UINT8 *) &Led
    );
}


/**
  Handler for Repeat Key event.

  This function is the handler for Repeat Key event triggered
  by timer.
  After a repeatable key is pressed, the event would be triggered
  with interval of USBKBD_REPEAT_DELAY. Once the event is triggered,
  following trigger will come with interval of USBKBD_REPEAT_RATE.

  @param  Event              The Repeat Key event.
  @param  Context            Points to the USB_KB_DEV instance.

**/
VOID
EFIAPI
USBKeyboardRepeatHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{
  USB_KB_DEV  *UsbKeyboardDevice;
  USB_KEY     UsbKey;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  //
  // Do nothing when there is no repeat key.
  //
  if (UsbKeyboardDevice->RepeatKey != 0) {
    //
    // Inserts the repeat key into keyboard buffer,
    //
    UsbKey.KeyCode = UsbKeyboardDevice->RepeatKey;
    UsbKey.Down    = TRUE;
    Enqueue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));

    //
    // Set repeat rate for next repeat key generation.
    //
    gBS->SetTimer (
           UsbKeyboardDevice->RepeatTimer,
           TimerRelative,
           USBKBD_REPEAT_RATE
           );
  }
}


/**
  Handler for Delayed Recovery event.

  This function is the handler for Delayed Recovery event triggered
  by timer.
  After a device error occurs, the event would be triggered
  with interval of EFI_USB_INTERRUPT_DELAY. EFI_USB_INTERRUPT_DELAY
  is defined in USB standard for error handling.

  @param  Event              The Delayed Recovery event.
  @param  Context            Points to the USB_KB_DEV instance.

**/
VOID
EFIAPI
USBKeyboardRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{

  USB_KB_DEV          *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               PacketSize;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  UsbIo             = UsbKeyboardDevice->UsbIo;

  PacketSize        = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

  //
  // Re-submit Asynchronous Interrupt Transfer for recovery.
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
           TRUE,
           UsbKeyboardDevice->IntEndpointDescriptor.Interval,
           PacketSize,
           KeyboardHandler,
           UsbKeyboardDevice
           );
}

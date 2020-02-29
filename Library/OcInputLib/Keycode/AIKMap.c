/** @file
  Key mapping tables.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AIKTranslate.h"

#include <Library/OcDebugLogLib.h>

#define AIK_DEBUG_STR DEBUG_POINTER

// Conversion table
AIK_PS2KEY_TO_USB
gAikPs2KeyToUsbMap[AIK_MAX_PS2KEY_NUM] = {
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x00
  {
    UsbHidUsageIdKbKpKeyEsc,
    AIK_DEBUG_STR("Esc"),
    AIK_DEBUG_STR("^ Esc ^")
  }, // 0x01
  {
    UsbHidUsageIdKbKpKeyOne,
    AIK_DEBUG_STR("1"),
    AIK_DEBUG_STR("!")
  }, // 0x02
  {
    UsbHidUsageIdKbKpKeyTwo,
    AIK_DEBUG_STR("2"),
    AIK_DEBUG_STR("@")
  }, // 0x03
  {
    UsbHidUsageIdKbKpKeyThree,
    AIK_DEBUG_STR("3"),
    AIK_DEBUG_STR("#")
  }, // 0x04
  {
    UsbHidUsageIdKbKpKeyFour,
    AIK_DEBUG_STR("4"),
    AIK_DEBUG_STR("$")
  }, // 0x05
  {
    UsbHidUsageIdKbKpKeyFive,
    AIK_DEBUG_STR("5"),
    AIK_DEBUG_STR("%")
  }, // 0x06
  {
    UsbHidUsageIdKbKpKeySix,
    AIK_DEBUG_STR("6"),
    AIK_DEBUG_STR("^")
  }, // 0x07
  {
    UsbHidUsageIdKbKpKeySeven,
    AIK_DEBUG_STR("7"),
    AIK_DEBUG_STR("&")
  }, // 0x08
  {
    UsbHidUsageIdKbKpKeyEight,
    AIK_DEBUG_STR("8"),
    AIK_DEBUG_STR("*")
  }, // 0x09
  {
    UsbHidUsageIdKbKpKeyNine,
    AIK_DEBUG_STR("9"),
    AIK_DEBUG_STR("(")
  }, // 0x0A
  {
    UsbHidUsageIdKbKpKeyZero,
    AIK_DEBUG_STR("0"),
    AIK_DEBUG_STR(")")
  }, // 0x0B
  {
    UsbHidUsageIdKbKpKeyMinus,
    AIK_DEBUG_STR("-"),
    AIK_DEBUG_STR("_")
  }, // 0x0C
  {
    UsbHidUsageIdKbKpKeyEquals,
    AIK_DEBUG_STR("="),
    AIK_DEBUG_STR("+")
  }, // 0x0D
  {
    UsbHidUsageIdKbKpKeyBackSpace,
    AIK_DEBUG_STR("Backspace"),
    AIK_DEBUG_STR("^ Backspace ^")
  }, // 0x0E
  {
    UsbHidUsageIdKbKpKeyTab,
    AIK_DEBUG_STR("Tab"),
    AIK_DEBUG_STR("^ Tab ^")
  }, // 0x0F
  {
    UsbHidUsageIdKbKpKeyQ,
    AIK_DEBUG_STR("q"),
    AIK_DEBUG_STR("Q")
  }, // 0x10
  {
    UsbHidUsageIdKbKpKeyW,
    AIK_DEBUG_STR("w"),
    AIK_DEBUG_STR("W")
  }, // 0x11
  {
    UsbHidUsageIdKbKpKeyE,
    AIK_DEBUG_STR("e"),
    AIK_DEBUG_STR("E")
  }, // 0x12
  {
    UsbHidUsageIdKbKpKeyR,
    AIK_DEBUG_STR("r"),
    AIK_DEBUG_STR("R")
  }, // 0x13
  {
    UsbHidUsageIdKbKpKeyT,
    AIK_DEBUG_STR("t"),
    AIK_DEBUG_STR("T")
  }, // 0x14
  {
    UsbHidUsageIdKbKpKeyY,
    AIK_DEBUG_STR("y"),
    AIK_DEBUG_STR("Y")
  }, // 0x15
  {
    UsbHidUsageIdKbKpKeyU,
    AIK_DEBUG_STR("u"),
    AIK_DEBUG_STR("U")
  }, // 0x16
  {
    UsbHidUsageIdKbKpKeyI,
    AIK_DEBUG_STR("i"),
    AIK_DEBUG_STR("I")
  }, // 0x17
  {
    UsbHidUsageIdKbKpKeyO,
    AIK_DEBUG_STR("o"),
    AIK_DEBUG_STR("O")
  }, // 0x18
  {
    UsbHidUsageIdKbKpKeyP,
    AIK_DEBUG_STR("p"),
    AIK_DEBUG_STR("P")
  }, // 0x19
  {
    UsbHidUsageIdKbKpKeyLeftBracket,
    AIK_DEBUG_STR("["),
    AIK_DEBUG_STR("{")
  }, // 0x1A
  {
    UsbHidUsageIdKbKpKeyRightBracket,
    AIK_DEBUG_STR("]"),
    AIK_DEBUG_STR("}")
  }, // 0x1B
  {
    UsbHidUsageIdKbKpKeyEnter,
    AIK_DEBUG_STR("Enter"),
    AIK_DEBUG_STR("^ Enter ^")
  }, // 0x1C
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x1D
  {
    UsbHidUsageIdKbKpKeyA,
    AIK_DEBUG_STR("a"),
    AIK_DEBUG_STR("A")
  }, // 0x1E
  {
    UsbHidUsageIdKbKpKeyS,
    AIK_DEBUG_STR("s"),
    AIK_DEBUG_STR("S")
  }, // 0x1F
  {
    UsbHidUsageIdKbKpKeyD,
    AIK_DEBUG_STR("d"),
    AIK_DEBUG_STR("D")
  }, // 0x20
  {
    UsbHidUsageIdKbKpKeyF,
    AIK_DEBUG_STR("f"),
    AIK_DEBUG_STR("F")
  }, // 0x21
  {
    UsbHidUsageIdKbKpKeyG,
    AIK_DEBUG_STR("g"),
    AIK_DEBUG_STR("G")
  }, // 0x22
  {
    UsbHidUsageIdKbKpKeyH,
    AIK_DEBUG_STR("h"),
    AIK_DEBUG_STR("H")
  }, // 0x23
  {
    UsbHidUsageIdKbKpKeyJ,
    AIK_DEBUG_STR("j"),
    AIK_DEBUG_STR("J")
  }, // 0x24
  {
    UsbHidUsageIdKbKpKeyK,
    AIK_DEBUG_STR("k"),
    AIK_DEBUG_STR("K")
  }, // 0x25
  {
    UsbHidUsageIdKbKpKeyL,
    AIK_DEBUG_STR("l"),
    AIK_DEBUG_STR("L")
  }, // 0x26
  {
    UsbHidUsageIdKbKpKeySemicolon,
    AIK_DEBUG_STR(";"),
    AIK_DEBUG_STR(":")
  }, // 0x27
  {
    UsbHidUsageIdKbKpKeyQuotation,
    AIK_DEBUG_STR("'"),
    AIK_DEBUG_STR("\""),
  }, // 0x28
  {
    UsbHidUsageIdKbKpKeyAcute,
    AIK_DEBUG_STR("`"),
    AIK_DEBUG_STR("~")
  }, // 0x29
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x2A
  {
    UsbHidUsageIdKbKpKeyBackslash,
    AIK_DEBUG_STR("\\"),
    AIK_DEBUG_STR("|")
  }, // 0x2B
  {
    UsbHidUsageIdKbKpKeyZ,
    AIK_DEBUG_STR("z"),
    AIK_DEBUG_STR("Z")
  }, // 0x2C
  {
    UsbHidUsageIdKbKpKeyX,
    AIK_DEBUG_STR("x"),
    AIK_DEBUG_STR("X")
  }, // 0x2D
  {
    UsbHidUsageIdKbKpKeyC,
    AIK_DEBUG_STR("c"),
    AIK_DEBUG_STR("C")
  }, // 0x2E
  {
    UsbHidUsageIdKbKpKeyV,
    AIK_DEBUG_STR("v"),
    AIK_DEBUG_STR("V")
  }, // 0x2F
  {
    UsbHidUsageIdKbKpKeyB,
    AIK_DEBUG_STR("b"),
    AIK_DEBUG_STR("B")
  }, // 0x30
  {
    UsbHidUsageIdKbKpKeyN,
    AIK_DEBUG_STR("n"),
    AIK_DEBUG_STR("N")
  }, // 0x31
  {
    UsbHidUsageIdKbKpKeyM,
    AIK_DEBUG_STR("m"),
    AIK_DEBUG_STR("M")
  }, // 0x32
  {
    UsbHidUsageIdKbKpKeyComma,
    AIK_DEBUG_STR(","),
    AIK_DEBUG_STR("<")
  }, // 0x33
  {
    UsbHidUsageIdKbKpKeyPeriod,
    AIK_DEBUG_STR("."),
    AIK_DEBUG_STR(">")
  }, // 0x34
  {
    UsbHidUsageIdKbKpKeySlash,
    AIK_DEBUG_STR("/"),
    AIK_DEBUG_STR("?")
  }, // 0x35
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x36
  {
    UsbHidUsageIdKbKpPadKeyAsterisk,
    AIK_DEBUG_STR("*"),
    AIK_DEBUG_STR("^ * ^")
  }, // 0x37
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x38
  {
    UsbHidUsageIdKbKpKeySpaceBar,
    AIK_DEBUG_STR("Spacebar"),
    AIK_DEBUG_STR("^ Spacebar ^")
  }, // 0x39
  {
    UsbHidUsageIdKbKpKeyCLock,
    AIK_DEBUG_STR("CapsLock"),
    AIK_DEBUG_STR("^ CapsLock ^")
  }, // 0x3A
  {
    UsbHidUsageIdKbKpKeyF1,
    AIK_DEBUG_STR("F1"),
    AIK_DEBUG_STR("^ F1 ^")
  }, // 0x3B
  {
    UsbHidUsageIdKbKpKeyF2,
    AIK_DEBUG_STR("F2"),
    AIK_DEBUG_STR("^ F2 ^")
  }, // 0x3C
  {
    UsbHidUsageIdKbKpKeyF3,
    AIK_DEBUG_STR("F3"),
    AIK_DEBUG_STR("^ F3 ^")
  }, // 0x3D
  {
    UsbHidUsageIdKbKpKeyF4,
    AIK_DEBUG_STR("F4"),
    AIK_DEBUG_STR("^ F4 ^")
  }, // 0x3E
  {
    UsbHidUsageIdKbKpKeyF5,
    AIK_DEBUG_STR("F5"),
    AIK_DEBUG_STR("^ F5 ^")
  }, // 0x3F
  {
    UsbHidUsageIdKbKpKeyF6,
    AIK_DEBUG_STR("F6"),
    AIK_DEBUG_STR("^ F6 ^")
  }, // 0x40
  {
    UsbHidUsageIdKbKpKeyF7,
    AIK_DEBUG_STR("F7"),
    AIK_DEBUG_STR("^ F7 ^")
  }, // 0x41
  {
    UsbHidUsageIdKbKpKeyF8,
    AIK_DEBUG_STR("F8"),
    AIK_DEBUG_STR("^ F8 ^")
  }, // 0x42
  {
    UsbHidUsageIdKbKpKeyF9,
    AIK_DEBUG_STR("F9"),
    AIK_DEBUG_STR("^ F9 ^")
  }, // 0x43
  {
    UsbHidUsageIdKbKpKeyF10,
    AIK_DEBUG_STR("F10"),
    AIK_DEBUG_STR("^ F10 ^")
  }, // 0x44
  {
    UsbHidUsageIdKbKpPadKeyNLck,
    AIK_DEBUG_STR("NumLock"),
    AIK_DEBUG_STR("^ NumLock ^")
  }, // 0x45
  {
    UsbHidUsageIdKbKpKeySLock,
    AIK_DEBUG_STR("Scroll Lock"),
    AIK_DEBUG_STR("^ Scroll Lock ^")
  }, // 0x46
  {
    UsbHidUsageIdKbKpKeyHome,
    AIK_DEBUG_STR("Home"),
    AIK_DEBUG_STR("^ Home ^")
  }, // 0x47
  {
    UsbHidUsageIdKbKpKeyUpArrow,
    AIK_DEBUG_STR("Up"),
    AIK_DEBUG_STR("^ Up ^")
  }, // 0x48
  {
    UsbHidUsageIdKbKpKeyPgUp,
    AIK_DEBUG_STR("PageUp"),
    AIK_DEBUG_STR("^ PageUp ^")
  }, // 0x49
  {
    UsbHidUsageIdKbKpPadKeyMinus,
    AIK_DEBUG_STR("-"),
    AIK_DEBUG_STR("^ - ^")
  }, // 0x4A
  {
    UsbHidUsageIdKbKpKeyLeftArrow,
    AIK_DEBUG_STR("Left"),
    AIK_DEBUG_STR("^ Left ^")
  }, // 0x4B
  {
    UsbHidUsageIdKbKpPadKeyFive,
    AIK_DEBUG_STR("5"),
    AIK_DEBUG_STR("^ 5 ^")
  }, // 0x4C
  {
    UsbHidUsageIdKbKpKeyRightArrow,
    AIK_DEBUG_STR("Right"),
    AIK_DEBUG_STR("^ Right ^")
  }, // 0x4D
  {
    UsbHidUsageIdKbKpPadKeyPlus,
    AIK_DEBUG_STR("+"),
    AIK_DEBUG_STR("^ + ^")
  }, // 0x4E
  {
    UsbHidUsageIdKbKpKeyEnd,
    AIK_DEBUG_STR("End"),
    AIK_DEBUG_STR("^ End ^")
  }, // 0x4F
  {
    UsbHidUsageIdKbKpKeyDownArrow,
    AIK_DEBUG_STR("Down"),
    AIK_DEBUG_STR("^ Down ^")
  }, // 0x50
  {
    UsbHidUsageIdKbKpKeyPgDn,
    AIK_DEBUG_STR("PageDown"),
    AIK_DEBUG_STR("^ PageDown ^")
  }, // 0x51
  {
    UsbHidUsageIdKbKpKeyIns,
    AIK_DEBUG_STR("Insert"),
    AIK_DEBUG_STR("^ Insert ^")
  }, // 0x52
  {
    UsbHidUsageIdKbKpKeyDel,
    AIK_DEBUG_STR("Delete"),
    AIK_DEBUG_STR("^ Delete ^")
  }, // 0x53
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x54
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x55
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x56
  {
    UsbHidUsageIdKbKpKeyF11,
    AIK_DEBUG_STR("F11"),
    AIK_DEBUG_STR("^ F11 ^")
  }, // 0x57
  {
    UsbHidUsageIdKbKpKeyF12,
    AIK_DEBUG_STR("F12"),
    AIK_DEBUG_STR("^ F12 ^")
  }, // 0x58
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x59
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x5A
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x5B
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x5C
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x5D
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x5E
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x5F
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x60
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x61
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x62
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x63
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x64
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x65
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x66
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x67
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x68
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x69
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x6A
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x6B
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x6C
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x6D
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x6E
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x6F
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x70
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x71
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x72
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x73
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x74
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x75
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x76
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x77
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x78
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x79
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x7A
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x7B
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x7C
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x7D
  {
    UsbHidUndefined,
    NULL,
    NULL
  }, // 0x7E
  {
    UsbHidUndefined,
    NULL,
    NULL
  } // 0x7F
};

AIK_ASCII_TO_USB
gAikAsciiToUsbMap[AIK_MAX_ASCII_NUM] = {
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("NUL")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("SOH")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("STX")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("ETX")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EOT")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("ENQ")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("ACK")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("BEL")
  },
  {
    UsbHidUsageIdKbKpKeyBackSpace,
    0,
    AIK_DEBUG_STR("BS")
  },
  {
    UsbHidUsageIdKbKpKeyTab,
    0,
    AIK_DEBUG_STR("HT")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("LF")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("VT")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("FF")
  },
  {
    UsbHidUsageIdKbKpKeyEnter,
    0,
    AIK_DEBUG_STR("CR")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("SO")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("SI")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("DLE")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("DC1")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("DC2")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("DC3")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("DC4")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("NAK")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("SYN")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("ETB")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("CAN")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EM")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("SUB")
  },
  {
    UsbHidUsageIdKbKpKeyEsc,
    0,
    AIK_DEBUG_STR("ESC")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("FS")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("GS")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("RS")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("US")
  },
  {
    UsbHidUsageIdKbKpKeySpaceBar,
    0,
    AIK_DEBUG_STR("SP")
  },
  {
    UsbHidUsageIdKbKpKeyOne,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("!")
  },
  {
    UsbHidUsageIdKbKpKeyQuotation,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("\"")
  },
  {
    UsbHidUsageIdKbKpKeyThree,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("#")
  },
  {
    UsbHidUsageIdKbKpKeyFour,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("$")
  },
  {
    UsbHidUsageIdKbKpKeyFive,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("%")
  },
  {
    UsbHidUsageIdKbKpKeySeven,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("&")
  },
  {
    UsbHidUsageIdKbKpKeyQuotation,
    0,
    AIK_DEBUG_STR("'")
  },
  {
    UsbHidUsageIdKbKpKeyNine,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("(")
  },
  {
    UsbHidUsageIdKbKpKeyZero,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR(")")
  },
  {
    UsbHidUsageIdKbKpKeyEight,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("*")
  },
  {
    UsbHidUsageIdKbKpKeyEquals,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("+")
  },
  {
    UsbHidUsageIdKbKpKeyComma,
    0,
    AIK_DEBUG_STR(",")
  },
  {
    UsbHidUsageIdKbKpKeyMinus,
    0,
    AIK_DEBUG_STR("-")
  },
  {
    UsbHidUsageIdKbKpKeyPeriod,
    0,
    AIK_DEBUG_STR(".")
  },
  {
    UsbHidUsageIdKbKpKeySlash,
    0,
    AIK_DEBUG_STR("/")
  },
  {
    UsbHidUsageIdKbKpKeyZero,
    0,
    AIK_DEBUG_STR("0")
  },
  {
    UsbHidUsageIdKbKpKeyOne,
    0,
    AIK_DEBUG_STR("1")
  },
  {
    UsbHidUsageIdKbKpKeyTwo,
    0,
    AIK_DEBUG_STR("2")
  },
  {
    UsbHidUsageIdKbKpKeyThree,
    0,
    AIK_DEBUG_STR("3")
  },
  {
    UsbHidUsageIdKbKpKeyFour,
    0,
    AIK_DEBUG_STR("4")
  },
  {
    UsbHidUsageIdKbKpKeyFive,
    0,
    AIK_DEBUG_STR("5")
  },
  {
    UsbHidUsageIdKbKpKeySix,
    0,
    AIK_DEBUG_STR("6")
  },
  {
    UsbHidUsageIdKbKpKeySeven,
    0,
    AIK_DEBUG_STR("7")
  },
  {
    UsbHidUsageIdKbKpKeyEight,
    0,
    AIK_DEBUG_STR("8")
  },
  {
    UsbHidUsageIdKbKpKeyNine,
    0,
    AIK_DEBUG_STR("9")
  },
  {
    UsbHidUsageIdKbKpKeySemicolon,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR(":")
  },
  {
    UsbHidUsageIdKbKpKeySemicolon,
    0,
    AIK_DEBUG_STR(";")
  },
  {
    UsbHidUsageIdKbKpKeyComma,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("<")
  },
  {
    UsbHidUsageIdKbKpKeyEquals,
    0,
    AIK_DEBUG_STR("=")
  },
  {
    UsbHidUsageIdKbKpKeyPeriod,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR(">")
  },
  {
    UsbHidUsageIdKbKpKeySlash,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("?")
  },
  {
    UsbHidUsageIdKbKpKeyTwo,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("@")
  },
  {
    UsbHidUsageIdKbKpKeyA,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("A")
  },
  {
    UsbHidUsageIdKbKpKeyB,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("B")
  },
  {
    UsbHidUsageIdKbKpKeyC,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("C")
  },
  {
    UsbHidUsageIdKbKpKeyD,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("D")
  },
  {
    UsbHidUsageIdKbKpKeyE,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("E")
  },
  {
    UsbHidUsageIdKbKpKeyF,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("F")
  },
  {
    UsbHidUsageIdKbKpKeyG,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("G")
  },
  {
    UsbHidUsageIdKbKpKeyH,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("H")
  },
  {
    UsbHidUsageIdKbKpKeyI,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("I")
  },
  {
    UsbHidUsageIdKbKpKeyJ,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("J")
  },
  {
    UsbHidUsageIdKbKpKeyK,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("K")
  },
  {
    UsbHidUsageIdKbKpKeyL,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("L")
  },
  {
    UsbHidUsageIdKbKpKeyM,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("M")
  },
  {
    UsbHidUsageIdKbKpKeyN,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("N")
  },
  {
    UsbHidUsageIdKbKpKeyO,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("O")
  },
  {
    UsbHidUsageIdKbKpKeyP,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("P")
  },
  {
    UsbHidUsageIdKbKpKeyQ,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("Q")
  },
  {
    UsbHidUsageIdKbKpKeyR,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("R")
  },
  {
    UsbHidUsageIdKbKpKeyS,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("S")
  },
  {
    UsbHidUsageIdKbKpKeyT,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("T")
  },
  {
    UsbHidUsageIdKbKpKeyU,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("U")
  },
  {
    UsbHidUsageIdKbKpKeyV,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("V")
  },
  {
    UsbHidUsageIdKbKpKeyW,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("W")
  },
  {
    UsbHidUsageIdKbKpKeyX,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("X")
  },
  {
    UsbHidUsageIdKbKpKeyY,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("Y")
  },
  {
    UsbHidUsageIdKbKpKeyZ,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("Z")
  },
  {
    UsbHidUsageIdKbKpKeyLeftBracket,
    0,
    AIK_DEBUG_STR("[")
  },
  {
    UsbHidUsageIdKbKpKeyBackslash,
    0,
    AIK_DEBUG_STR("\\")
  },
  {
    UsbHidUsageIdKbKpKeyRightBracket,
    0,
    AIK_DEBUG_STR("]")
  },
  {
    UsbHidUsageIdKbKpKeyRightBracket,
    0,
    AIK_DEBUG_STR("^")
  },
  {
    UsbHidUsageIdKbKpKeyMinus,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("_")
  },
  {
    UsbHidUsageIdKbKpKeyAcute,
    0,
    AIK_DEBUG_STR("`")
  },
  {
    UsbHidUsageIdKbKpKeyA,
    0,
    AIK_DEBUG_STR("a")
  },
  {
    UsbHidUsageIdKbKpKeyB,
    0,
    AIK_DEBUG_STR("b")
  },
  {
    UsbHidUsageIdKbKpKeyC,
    0,
    AIK_DEBUG_STR("c")
  },
  {
    UsbHidUsageIdKbKpKeyD,
    0,
    AIK_DEBUG_STR("d")
  },
  {
    UsbHidUsageIdKbKpKeyE,
    0,
    AIK_DEBUG_STR("e")
  },
  {
    UsbHidUsageIdKbKpKeyF,
    0,
    AIK_DEBUG_STR("f")
  },
  {
    UsbHidUsageIdKbKpKeyG,
    0,
    AIK_DEBUG_STR("g")
  },
  {
    UsbHidUsageIdKbKpKeyH,
    0,
    AIK_DEBUG_STR("h")
  },
  {
    UsbHidUsageIdKbKpKeyI,
    0,
    AIK_DEBUG_STR("i")
  },
  {
    UsbHidUsageIdKbKpKeyJ,
    0,
    AIK_DEBUG_STR("j")
  },
  {
    UsbHidUsageIdKbKpKeyK,
    0,
    AIK_DEBUG_STR("k")
  },
  {
    UsbHidUsageIdKbKpKeyL,
    0,
    AIK_DEBUG_STR("l")
  },
  {
    UsbHidUsageIdKbKpKeyM,
    0,
    AIK_DEBUG_STR("m")
  },
  {
    UsbHidUsageIdKbKpKeyN,
    0,
    AIK_DEBUG_STR("n")
  },
  {
    UsbHidUsageIdKbKpKeyO,
    0,
    AIK_DEBUG_STR("o")
  },
  {
    UsbHidUsageIdKbKpKeyP,
    0,
    AIK_DEBUG_STR("p")
  },
  {
    UsbHidUsageIdKbKpKeyQ,
    0,
    AIK_DEBUG_STR("q")
  },
  {
    UsbHidUsageIdKbKpKeyR,
    0,
    AIK_DEBUG_STR("r")
  },
  {
    UsbHidUsageIdKbKpKeyS,
    0,
    AIK_DEBUG_STR("s")
  },
  {
    UsbHidUsageIdKbKpKeyT,
    0,
    AIK_DEBUG_STR("t")
  },
  {
    UsbHidUsageIdKbKpKeyU,
    0,
    AIK_DEBUG_STR("u")
  },
  {
    UsbHidUsageIdKbKpKeyV,
    0,
    AIK_DEBUG_STR("v")
  },
  {
    UsbHidUsageIdKbKpKeyW,
    0,
    AIK_DEBUG_STR("w")
  },
  {
    UsbHidUsageIdKbKpKeyX,
    0,
    AIK_DEBUG_STR("x")
  },
  {
    UsbHidUsageIdKbKpKeyY,
    0,
    AIK_DEBUG_STR("y")
  },
  {
    UsbHidUsageIdKbKpKeyZ,
    0,
    AIK_DEBUG_STR("z")
  },
  {
    UsbHidUsageIdKbKpKeyRightBracket,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("{")
  },
  {
    UsbHidUsageIdKbKpKeyBackslash,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("|")
  },
  {
    UsbHidUsageIdKbKpKeyRightBracket,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("}")
  },
  {
    UsbHidUsageIdKbKpKeyAcute,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("~")
  },
  {
    UsbHidUsageIdKbKpKeyDel,
    0,
    AIK_DEBUG_STR("DEL")
  },
};

AIK_EFIKEY_TO_USB
gAikEfiKeyToUsbMap[AIK_MAX_EFIKEY_NUM] = {
  {
    UsbHidUndefined,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_CONTROL_PRESSED,
    AIK_DEBUG_STR("EfiKeyLCtrl")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyA0")
  },
  {
    UsbHidUndefined,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_ALT_PRESSED,
    AIK_DEBUG_STR("EfiKeyLAlt")
  },
  {
    UsbHidUsageIdKbKpKeySpaceBar,
    0,
    AIK_DEBUG_STR("EfiKeySpaceBar")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyA2")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyA3")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyA4")
  },
  {
    UsbHidUndefined,
    EFI_SHIFT_STATE_VALID | EFI_RIGHT_CONTROL_PRESSED,
    AIK_DEBUG_STR("EfiKeyRCtrl")
  },
  {
    UsbHidUsageIdKbKpKeyLeftArrow,
    0,
    AIK_DEBUG_STR("EfiKeyLeftArrow")
  },
  {
    UsbHidUsageIdKbKpKeyDownArrow,
    0,
    AIK_DEBUG_STR("EfiKeyDownArrow")
  },
  {
    UsbHidUsageIdKbKpKeyRightArrow,
    0,
    AIK_DEBUG_STR("EfiKeyRightArrow")
  },
  {
    UsbHidUsageIdKbKpKeyZero,
    0,
    AIK_DEBUG_STR("EfiKeyZero")
  },
  {
    UsbHidUsageIdKbKpKeyPeriod,
    0,
    AIK_DEBUG_STR("EfiKeyPeriod")
  },
  {
    UsbHidUsageIdKbKpKeyEnter,
    0,
    AIK_DEBUG_STR("EfiKeyEnter")
  },
  {
    UsbHidUndefined,
    EFI_SHIFT_STATE_VALID | EFI_LEFT_SHIFT_PRESSED,
    AIK_DEBUG_STR("EfiKeyLShift")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB0")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB1")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB2")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB3")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB4")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB5")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB6")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB7")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB8")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB9")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyB10")
  },
  {
    UsbHidUndefined,
    EFI_SHIFT_STATE_VALID | EFI_RIGHT_SHIFT_PRESSED,
    AIK_DEBUG_STR("EfiKeyRshift")
  },
  {
    UsbHidUsageIdKbKpKeyUpArrow,
    0,
    AIK_DEBUG_STR("EfiKeyUpArrow")
  },
  {
    UsbHidUsageIdKbKpKeyOne,
    0,
    AIK_DEBUG_STR("EfiKeyOne")
  },
  {
    UsbHidUsageIdKbKpKeyTwo,
    0,
    AIK_DEBUG_STR("EfiKeyTwo")
  },
  {
    UsbHidUsageIdKbKpKeyThree,
    0,
    AIK_DEBUG_STR("EfiKeyThree")
  },
  {
    UsbHidUsageIdKbKpKeyCLock,
    0,
    AIK_DEBUG_STR("EfiKeyCapsLock")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC1")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC2")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC3")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC4")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC5")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC6")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC7")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC8")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC9")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC10")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC11")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyC12")
  },
  {
    UsbHidUsageIdKbKpKeyFour,
    0,
    AIK_DEBUG_STR("EfiKeyFour")
  },
  {
    UsbHidUsageIdKbKpKeyFive,
    0,
    AIK_DEBUG_STR("EfiKeyFive")
  },
  {
    UsbHidUsageIdKbKpKeySix,
    0,
    AIK_DEBUG_STR("EfiKeySix")
  },
  {
    UsbHidUsageIdKbKpPadKeyPlus,
    0,
    AIK_DEBUG_STR("EfiKeyPlus")
  },
  {
    UsbHidUsageIdKbKpKeyTab,
    0,
    AIK_DEBUG_STR("EfiKeyTab")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD1")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD2")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD3")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD4")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD5")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD6")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD7")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD8")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD9")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD10")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD11")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD12")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyD13")
  },
  {
    UsbHidUsageIdKbKpKeyDel,
    0,
    AIK_DEBUG_STR("EfiKeyDel")
  },
  {
    UsbHidUsageIdKbKpKeyEnd,
    0,
    AIK_DEBUG_STR("EfiKeyEnd")
  },
  {
    UsbHidUsageIdKbKpKeyPgDn,
    0,
    AIK_DEBUG_STR("EfiKeyPgDn")
  },
  {
    UsbHidUsageIdKbKpKeySeven,
    0,
    AIK_DEBUG_STR("EfiKeySeven")
  },
  {
    UsbHidUsageIdKbKpKeyEight,
    0,
    AIK_DEBUG_STR("EfiKeyEight")
  },
  {
    UsbHidUsageIdKbKpKeyNine,
    0,
    AIK_DEBUG_STR("EfiKeyNine")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE0")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE1")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE2")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE3")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE4")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE5")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE6")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE7")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE8")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE9")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE10")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE11")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("EfiKeyE12")
  },
  {
    UsbHidUsageIdKbKpKeyBackSpace,
    0,
    AIK_DEBUG_STR("EfiKeyBackSpace")
  },
  {
    UsbHidUsageIdKbKpKeyIns,
    0,
    AIK_DEBUG_STR("EfiKeyIns")
  },
  {
    UsbHidUsageIdKbKpKeyHome,
    0,
    AIK_DEBUG_STR("EfiKeyHome")
  },
  {
    UsbHidUsageIdKbKpKeyPgUp,
    0,
    AIK_DEBUG_STR("EfiKeyPgUp")
  },
  {
    UsbHidUsageIdKbKpPadKeyNLck,
    0,
    AIK_DEBUG_STR("EfiKeyNLck")
  },
  {
    UsbHidUsageIdKbKpKeySlash,
    0,
    AIK_DEBUG_STR("EfiKeySlash")
  },
  {
    UsbHidUsageIdKbKpPadKeyAsterisk,
    0,
    AIK_DEBUG_STR("EfiKeyAsterisk")
  },
  {
    UsbHidUsageIdKbKpPadKeyMinus,
    0,
    AIK_DEBUG_STR("EfiKeyMinus")
  },
  {
    UsbHidUsageIdKbKpKeyEsc,
    0,
    AIK_DEBUG_STR("EfiKeyEsc")
  },
  {
    UsbHidUsageIdKbKpKeyF1,
    0,
    AIK_DEBUG_STR("EfiKeyF1")
  },
  {
    UsbHidUsageIdKbKpKeyF2,
    0,
    AIK_DEBUG_STR("EfiKeyF2")
  },
  {
    UsbHidUsageIdKbKpKeyF3,
    0,
    AIK_DEBUG_STR("EfiKeyF3")
  },
  {
    UsbHidUsageIdKbKpKeyF4,
    0,
    AIK_DEBUG_STR("EfiKeyF4")
  },
  {
    UsbHidUsageIdKbKpKeyF5,
    0,
    AIK_DEBUG_STR("EfiKeyF5")
  },
  {
    UsbHidUsageIdKbKpKeyF6,
    0,
    AIK_DEBUG_STR("EfiKeyF6")
  },
  {
    UsbHidUsageIdKbKpKeyF7,
    0,
    AIK_DEBUG_STR("EfiKeyF7")
  },
  {
    UsbHidUsageIdKbKpKeyF8,
    0,
    AIK_DEBUG_STR("EfiKeyF8")
  },
  {
    UsbHidUsageIdKbKpKeyF9,
    0,
    AIK_DEBUG_STR("EfiKeyF9")
  },
  {
    UsbHidUsageIdKbKpKeyF10,
    0,
    AIK_DEBUG_STR("EfiKeyF10")
  },
  {
    UsbHidUsageIdKbKpKeyF11,
    0,
    AIK_DEBUG_STR("EfiKeyF11")
  },
  {
    UsbHidUsageIdKbKpKeyF12,
    0,
    AIK_DEBUG_STR("EfiKeyF12")
  },
  {
    UsbHidUsageIdKbKpKeyF13,
    0,
    AIK_DEBUG_STR("EfiKeyPrint")
  },
  {
    UsbHidUsageIdKbKpKeySLock,
    0,
    AIK_DEBUG_STR("EfiKeySLck")
  },
  {
    UsbHidUsageIdKbKpKeyPause,
    0,
    AIK_DEBUG_STR("EfiKeyPause")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk105")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk106")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk107")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk108")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk109")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk110")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk111")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk112")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk113")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk114")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk115")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk116")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk117")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk118")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk119")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk120")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk121")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk122")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk123")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk124")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk125")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk126")
  },
  {
    UsbHidUndefined,
    0,
    AIK_DEBUG_STR("Unk127")
  }
};

AIK_SCANCODE_TO_USB
gAikScanCodeToUsbMap[AIK_MAX_SCANCODE_NUM] = {
  {
    UsbHidUndefined,
    NULL
  },
  {
    UsbHidUsageIdKbKpKeyUpArrow,
    AIK_DEBUG_STR("Move cursor up 1 row")
  },
  {
    UsbHidUsageIdKbKpKeyDownArrow,
    AIK_DEBUG_STR("Move cursor down 1 row")
  },
  {
    UsbHidUsageIdKbKpKeyRightArrow,
    AIK_DEBUG_STR("Move cursor right 1 column")
  },
  {
    UsbHidUsageIdKbKpKeyLeftArrow,
    AIK_DEBUG_STR("Move cursor left 1 column")
  },
  {
    UsbHidUsageIdKbKpKeyHome,
    AIK_DEBUG_STR("Home")
  },
  {
    UsbHidUsageIdKbKpKeyEnd,
    AIK_DEBUG_STR("End")
  },
  {
    UsbHidUsageIdKbKpKeyIns,
    AIK_DEBUG_STR("Insert")
  },
  {
    UsbHidUsageIdKbKpKeyDel,
    AIK_DEBUG_STR("Delete")
  },
  {
    UsbHidUsageIdKbKpKeyPgUp,
    AIK_DEBUG_STR("Page Up")
  },
  {
    UsbHidUsageIdKbKpKeyPgDn,
    AIK_DEBUG_STR("Page Down")
  },
  {
    UsbHidUsageIdKbKpKeyF1,
    AIK_DEBUG_STR("Function 1")
  },
  {
    UsbHidUsageIdKbKpKeyF2,
    AIK_DEBUG_STR("Function 2")
  },
  {
    UsbHidUsageIdKbKpKeyF3,
    AIK_DEBUG_STR("Function 3")
  },
  {
    UsbHidUsageIdKbKpKeyF4,
    AIK_DEBUG_STR("Function 4")
  },
  {
    UsbHidUsageIdKbKpKeyF5,
    AIK_DEBUG_STR("Function 5")
  },
  {
    UsbHidUsageIdKbKpKeyF6,
    AIK_DEBUG_STR("Function 6")
  },
  {
    UsbHidUsageIdKbKpKeyF7,
    AIK_DEBUG_STR("Function 7")
  },
  {
    UsbHidUsageIdKbKpKeyF8,
    AIK_DEBUG_STR("Function 8")
  },
  {
    UsbHidUsageIdKbKpKeyF9,
    AIK_DEBUG_STR("Function 9")
  },
  {
    UsbHidUsageIdKbKpKeyF10,
    AIK_DEBUG_STR("Function 10")
  },
  {
    UsbHidUsageIdKbKpKeyF11,
    AIK_DEBUG_STR("Function 11")
  },
  {
    UsbHidUsageIdKbKpKeyF12,
    AIK_DEBUG_STR("Function 12")
  },
  {
    UsbHidUsageIdKbKpKeyEsc,
    AIK_DEBUG_STR("Escape")
  }
};

CONST CHAR8 *
gAikModifiersToNameMap[AIK_MAX_MODIFIERS_NUM] = {
  NULL,
  AIK_DEBUG_STR("LCTRL"),
  AIK_DEBUG_STR("LSHIFT"),
  AIK_DEBUG_STR("LSHIFT|LCTRL"),
  AIK_DEBUG_STR("LALT"),
  AIK_DEBUG_STR("LCTRL|LALT"),
  AIK_DEBUG_STR("LSHIFT|LALT"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|LALT"),
  AIK_DEBUG_STR("LGUI"),
  AIK_DEBUG_STR("LCTRL|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|LGUI"),
  AIK_DEBUG_STR("LALT|LGUI"),
  AIK_DEBUG_STR("LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RCTRL"),
  AIK_DEBUG_STR("RCTRL|LCTRL"),
  AIK_DEBUG_STR("LSHIFT|RCTRL"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL"),
  AIK_DEBUG_STR("RCTRL|LALT"),
  AIK_DEBUG_STR("RCTRL|LCTRL|LALT"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LALT"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|LALT"),
  AIK_DEBUG_STR("RCTRL|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|LGUI"),
  AIK_DEBUG_STR("RCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT"),
  AIK_DEBUG_STR("RSHIFT|LCTRL"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL"),
  AIK_DEBUG_STR("RSHIFT|LALT"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|LALT"),
  AIK_DEBUG_STR("RSHIFT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LALT"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|LALT"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|LALT|LGUI"),
  AIK_DEBUG_STR("RALT"),
  AIK_DEBUG_STR("LCTRL|RALT"),
  AIK_DEBUG_STR("LSHIFT|RALT"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT"),
  AIK_DEBUG_STR("RALT|LALT"),
  AIK_DEBUG_STR("LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("LSHIFT|RALT|LALT"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RALT|LGUI"),
  AIK_DEBUG_STR("LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RALT|LALT|LGUI"),
  AIK_DEBUG_STR("LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RCTRL|RALT"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT"),
  AIK_DEBUG_STR("RCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT|LALT"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RALT"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT"),
  AIK_DEBUG_STR("RSHIFT|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RALT"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RALT|LALT"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RALT|LALT|LGUI"),
  AIK_DEBUG_STR("RGUI"),
  AIK_DEBUG_STR("LCTRL|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RGUI"),
  AIK_DEBUG_STR("LALT|RGUI"),
  AIK_DEBUG_STR("LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RGUI|LGUI"),
  AIK_DEBUG_STR("LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|RGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RGUI"),
  AIK_DEBUG_STR("RCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RALT|RGUI"),
  AIK_DEBUG_STR("LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RALT|LALT|RGUI"),
  AIK_DEBUG_STR("LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|LCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RCTRL|LCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("LSHIFT|RCTRL|LCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|LCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RALT|LALT|RGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|LCTRL|RALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|RCTRL|LCTRL|RALT|LALT|RGUI|LGUI"),
  AIK_DEBUG_STR("RSHIFT|LSHIFT|RCTRL|RALT|LALT|RGUI|LGUI"),
};

CONST CHAR8 *
gAikAppleKeyToNameMap[AIK_MAX_APPLEKEY_NUM] = {
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyA"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyB"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyC"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyD"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyE"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyG"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyH"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyI"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyJ"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyK"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyL"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyM"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyN"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyO"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyP"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyQ"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyR"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyS"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyT"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyU"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyV"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyW"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyX"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyY"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyZ"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyOne"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyTwo"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyThree"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyFour"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyFive"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeySix"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeySeven"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyEight"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyNine"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyZero"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyEnter"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyEscape"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyBackSpace"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyTab"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeySpaceBar"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyMinus"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyEquals"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyLeftBracket"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyRightBracket"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyBackslash"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyNonUsHash"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeySemicolon"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyQuotation"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyAcute"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyComma"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyPeriod"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeySlash"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyCLock"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF1"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF2"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF3"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF4"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF5"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF6"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF7"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF8"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF9"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF10"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF11"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyF12"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyPrint"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeySLock"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyPause"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyIns"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyHome"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyPgUp"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyDel"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyEnd"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyPgDn"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyRightArrow"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyLeftArrow"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyDownArrow"),
  AIK_DEBUG_STR("AppleHidUsbKbUsageKeyUpArrow"),
};

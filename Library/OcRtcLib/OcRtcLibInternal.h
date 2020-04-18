/** @file

  OcRtcLib - library with RTC I/O functions

  Copyright (c) 2020, vit9696

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_RTC_LIB_INTERNAL_H
#define OC_RTC_LIB_INTERNAL_H

//
// Available on all platforms, requires NMI bit handling.
//
#define R_PCH_RTC_INDEX           0x70
#define R_PCH_RTC_TARGET          0x71
#define R_PCH_RTC_EXT_INDEX       0x72
#define R_PCH_RTC_EXT_TARGET      0x73

//
// Available on Ivy Bridge and newer. Ignores NMI bit.
//
#define R_PCH_RTC_INDEX_ALT       0x74
#define R_PCH_RTC_TARGET_ALT      0x75
#define R_PCH_RTC_EXT_INDEX_ALT   0x76
#define R_PCH_RTC_EXT_TARGET_ALT  0x77

//
// RTC Memory bank size
//
#define RTC_BANK_SIZE             0x80

//
// RTC INDEX bit mask
//
#define RTC_DATA_MASK             0x7F
#define RTC_NMI_MASK              0x80

//
// Standard register addresses.
//
#define RTC_ADDRESS_SECONDS           0   // R/W  Range 0..59
#define RTC_ADDRESS_SECONDS_ALARM     1   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES           2   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES_ALARM     3   // R/W  Range 0..59
#define RTC_ADDRESS_HOURS             4   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_HOURS_ALARM       5   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_DAY_OF_THE_WEEK   6   // R/W  Range 1..7
#define RTC_ADDRESS_DAY_OF_THE_MONTH  7   // R/W  Range 1..31
#define RTC_ADDRESS_MONTH             8   // R/W  Range 1..12
#define RTC_ADDRESS_YEAR              9   // R/W  Range 0..99
#define RTC_ADDRESS_REGISTER_A        10  // R/W[0..6]  R0[7]
#define RTC_ADDRESS_REGISTER_B        11  // R/W
#define RTC_ADDRESS_REGISTER_C        12  // RO
#define RTC_ADDRESS_REGISTER_D        13  // RO

//
// Register A update in progress bit.
//
#define RTC_UPDATE_IN_PROGRESS        0x80U

#endif // OC_RTC_LIB_INTERNAL_H

/** @file

OcRtcLib - library with RTC I/O functions

Copyright (c) 2017-2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_RTC_LIB_H
#define OC_RTC_LIB_H

#include <Protocol/AppleRtcRam.h>

//
// Standard interface, available on most Intel chipsets
//

UINT8
OcRtcRead (
  IN  UINT8  Offset
  );

VOID
OcRtcWrite (
  IN UINT8 Offset,
  IN UINT8 Value
  );

/**
  Calculate Apple CMOS checksum.
  This is a modified version of ANSI CRC16 in REFIN mode (0xA001 poly).
  See http://zlib.net/crc_v3.txt for more details.

  1. Effective poly is 0x2001 due to a bitwise OR with BIT15.
     The change turns CRC16 into CRC14, making BIT14 and BIT15 always zero.
     This modification is commonly found in legacy Phoenix firmware,
     where it was used for password hashing as found by dogbert:
     http://sites.google.com/site/dogber1/blag/pwgen-5dec.py

  2. Only 7 shifts per byte are performed instead of the usual 8.
     This might improve checksum quality against specific data, but the exact
     reasons are unknown. The algorithm did not change since its introduction
     in 10.4.x, and since Apple Developer Transition Kit was based on Phoenix
     firmware, this could just be a quick change to get a different checksum.

  @param[in]  Data    Pointer to data to calculate the checksum of.
  @param[in]  Size    Size of data.

  @retval Resulting checksum.
**/
UINT16
OcRtcChecksumApple (
  IN CONST VOID   *Data,
  IN UINTN        Size
  );

/**
  Install and initialise Apple RTC RAM protocol.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_RTC_RAM_PROTOCOL *
OcAppleRtcRamInstallProtocol (
  IN BOOLEAN  Reinstall
  );

//
// Modern faster interface, available on IvyBridge or newer
//

UINT8
OcRtcReadIvy (
  IN  UINT8  Offset
  );

VOID
OcRtcWriteIvy (
  IN UINT8 Offset,
  IN UINT8 Value
  );

#endif // OC_RTC_LIB_H

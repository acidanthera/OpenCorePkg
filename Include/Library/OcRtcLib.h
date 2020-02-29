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

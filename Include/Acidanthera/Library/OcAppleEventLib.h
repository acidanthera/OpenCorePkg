/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_APPLE_EVENT_LIB_H
#define OC_APPLE_EVENT_LIB_H

#include <IndustryStandard/AppleHid.h>

#include <Protocol/AppleEvent.h>

/**
  Install and initialise Apple Event protocol.

  @param[in] Install            If false, do not install even when no suitable OEM version found.
  @param[in] Reinstall          If true, force overwrite installed protocol.
                                If false, use Apple OEM protocol where possible.
  @param[in] CustomDelays       If true, use key delays specified.
                                If false, use Apple OEM default key delay values.
  @param[in] KeyInitialDelay    Key repeat initial delay in 10ms units.
  @param[in] KeySubsequentDelay Key repeat subsequent delay in 10ms units.
                                If zero, warn and use 1.
  @param[in] PointerSpeedDiv    Pointer speed divisor. If zero, warn and use 1.
  @param[in] PointerSpeedMul    Pointer speed multiplier.

  @retval installed or located protocol or NULL.
**/
APPLE_EVENT_PROTOCOL *
OcAppleEventInstallProtocol (
  IN BOOLEAN  Install,
  IN BOOLEAN  Reinstall,
  IN BOOLEAN  CustomDelays,
  IN UINT16   KeyInitialDelay,
  IN UINT16   KeySubsequentDelay,
  IN UINT16   PointerSpeedDiv,
  IN UINT16   PointerSpeedMul
  );

#endif // OC_APPLE_EVENT_LIB_H

/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "../OcCpuInternals.h"

#include <Library/BaseLib.h>
#include <Library/IoLib.h>

VOID
EFIAPI
AsmMeasureTicks (
  IN  UINT32  AcpiTicksDuration,
  IN  UINT16  TimerAddr,
  OUT UINT32  *AcpiTicksDelta,
  OUT UINT64  *TscTicksDelta
  )
{
  UINT32      AcpiTick0;
  UINT32      AcpiTick1;
  UINT32      AcpiCurrentDelta;
  UINT64      Tsc0;
  UINT64      Tsc1;

  AcpiTick0 = IoRead32 (TimerAddr);
  Tsc0      = AsmReadTsc ();

  do {
    CpuPause ();

    //
    // Check how many AcpiTicks have passed since we started.
    //
    AcpiTick1 = IoRead32 (TimerAddr);

    if (AcpiTick0 <= AcpiTick1) {
      //
      // No overflow.
      //
      AcpiCurrentDelta = AcpiTick1 - AcpiTick0;
    } else if (AcpiTick0 - AcpiTick1 <= 0x00FFFFFF) {
      //
      // Overflow, 24-bit timer.
      //
      AcpiCurrentDelta = 0x00FFFFFF - AcpiTick0 + AcpiTick1;
    } else {
      //
      // Overflow, 32-bit timer.
      //
      AcpiCurrentDelta = MAX_UINT32 - AcpiTick0 + AcpiTick1;
    }

    //
    // Keep checking AcpiTicks until target is reached.
    //
  } while (AcpiCurrentDelta < AcpiTicksDuration);

  Tsc1 = AsmReadTsc ();

  //
  // On some systems we may end up waiting for notably longer than 100ms,
  // despite disabling all events. Divide by actual time passed as suggested
  // by asava's Clover patch r2668.
  //
  *TscTicksDelta = Tsc1 - Tsc0;
  *AcpiTicksDelta = AcpiCurrentDelta;
}

/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.<BR>
  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_TIMER_LIB_H
#define OC_TIMER_LIB_H

#include <Library/TimerLib.h>

//
// Minimal CPU delay in microseconds (us, 10^-6) for use with MicroSecondDelay e.g. in polling loops
//
#define OC_MINIMAL_CPU_DELAY 10

/**
  Calculate the TSC frequency

  @retval  The calculated TSC frequency.
**/
UINT64
RecalculateTSC (
  VOID
  );

/**
  Return cached PerformanceCounterFrequency value. For instrumentation purposes only.

  @retval               The timer frequency in use.

**/
UINT64
EFIAPI
GetTscFrequency (
  VOID
  );

#endif // OC_TIMER_LIB_H

/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_TIMER_LIB_H
#define OC_TIMER_LIB_H

#include <Library/TimerLib.h>

/**
  Calculate the TSC frequency

  @retval  The calculated TSC frequency.
**/
UINT64
RecalculateTSC (
  VOID
  );

#endif // OC_TIMER_LIB_H

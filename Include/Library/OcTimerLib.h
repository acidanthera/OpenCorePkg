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

#ifndef OC_TIMER_LIB_H_
#define OC_TIMER_LIB_H_

// CalculateTSC
/** Calculate the TSC frequency

  @param[in] Recalculate  Switch to enable using cached value if already calculated.

  @retval  The calculated TSC frequency.
**/
UINT64
CalculateTSC (
  IN BOOLEAN  Recalculate
  );

#endif // OC_TIMER_LIB_H_

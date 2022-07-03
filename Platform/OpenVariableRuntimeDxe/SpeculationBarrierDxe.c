/** @file
  Barrier to stop speculative execution (DXE version).

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"

/**
  This service is consumed by the variable modules to place a barrier to stop
  speculative execution.

  Ensures that no later instruction will execute speculatively, until all prior
  instructions have completed.

**/
VOID
VariableSpeculationBarrier (
  VOID
  )
{
  //
  // Do nothing.
  //
}

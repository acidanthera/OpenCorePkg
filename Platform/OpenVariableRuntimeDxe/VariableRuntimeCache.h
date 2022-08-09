/** @file
  The common variable volatile store routines shared by the DXE_RUNTIME variable
  module and the DXE_SMM variable module.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_RUNTIME_CACHE_H_
#define _VARIABLE_RUNTIME_CACHE_H_

#include "Variable.h"

/**
  Copies any pending updates to runtime variable caches.

  @retval EFI_UNSUPPORTED         The volatile store to be updated is not initialized properly.
  @retval EFI_SUCCESS             The volatile store was updated successfully.

**/
EFI_STATUS
FlushPendingRuntimeVariableCacheUpdates (
  VOID
  );

/**
  Synchronizes the runtime variable caches with all pending updates outside runtime.

  Ensures all conditions are met to maintain coherency for runtime cache updates. This function will attempt
  to write the given update (and any other pending updates) if the ReadLock is available. Otherwise, the
  update is added as a pending update for the given variable store and it will be flushed to the runtime cache
  at the next opportunity the ReadLock is available.

  @param[in] VariableRuntimeCache Variable runtime cache structure for the runtime cache being synchronized.
  @param[in] Offset               Offset in bytes to apply the update.
  @param[in] Length               Length of data in bytes of the update.

  @retval EFI_SUCCESS             The update was added as a pending update successfully. If the variable runtime
                                  cache ReadLock was available, the runtime cache was updated successfully.
  @retval EFI_UNSUPPORTED         The volatile store to be updated is not initialized properly.

**/
EFI_STATUS
SynchronizeRuntimeVariableCache (
  IN  VARIABLE_RUNTIME_CACHE  *VariableRuntimeCache,
  IN  UINTN                   Offset,
  IN  UINTN                   Length
  );

#endif

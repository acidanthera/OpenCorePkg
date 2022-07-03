/** @file
  Functions related to managing the UEFI variable runtime cache. This file should only include functions
  used by the SMM UEFI variable driver.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data. They may be input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableParsing.h"
#include "VariableRuntimeCache.h"

extern VARIABLE_MODULE_GLOBAL  *mVariableModuleGlobal;
extern VARIABLE_STORE_HEADER   *mNvVariableCache;

/**
  Copies any pending updates to runtime variable caches.

  @retval EFI_UNSUPPORTED         The volatile store to be updated is not initialized properly.
  @retval EFI_SUCCESS             The volatile store was updated successfully.

**/
EFI_STATUS
FlushPendingRuntimeVariableCacheUpdates (
  VOID
  )
{
  VARIABLE_RUNTIME_CACHE_CONTEXT  *VariableRuntimeCacheContext;

  VariableRuntimeCacheContext = &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext;

  if ((VariableRuntimeCacheContext->VariableRuntimeNvCache.Store == NULL) ||
      (VariableRuntimeCacheContext->VariableRuntimeVolatileCache.Store == NULL) ||
      (VariableRuntimeCacheContext->PendingUpdate == NULL))
  {
    return EFI_UNSUPPORTED;
  }

  if (*(VariableRuntimeCacheContext->PendingUpdate)) {
    if ((VariableRuntimeCacheContext->VariableRuntimeHobCache.Store != NULL) &&
        (mVariableModuleGlobal->VariableGlobal.HobVariableBase > 0))
    {
      CopyMem (
        (VOID *)(
                 ((UINT8 *)(UINTN)VariableRuntimeCacheContext->VariableRuntimeHobCache.Store) +
                 VariableRuntimeCacheContext->VariableRuntimeHobCache.PendingUpdateOffset
                 ),
        (VOID *)(
                 ((UINT8 *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase) +
                 VariableRuntimeCacheContext->VariableRuntimeHobCache.PendingUpdateOffset
                 ),
        VariableRuntimeCacheContext->VariableRuntimeHobCache.PendingUpdateLength
        );
      VariableRuntimeCacheContext->VariableRuntimeHobCache.PendingUpdateLength = 0;
      VariableRuntimeCacheContext->VariableRuntimeHobCache.PendingUpdateOffset = 0;
    }

    CopyMem (
      (VOID *)(
               ((UINT8 *)(UINTN)VariableRuntimeCacheContext->VariableRuntimeNvCache.Store) +
               VariableRuntimeCacheContext->VariableRuntimeNvCache.PendingUpdateOffset
               ),
      (VOID *)(
               ((UINT8 *)(UINTN)mNvVariableCache) +
               VariableRuntimeCacheContext->VariableRuntimeNvCache.PendingUpdateOffset
               ),
      VariableRuntimeCacheContext->VariableRuntimeNvCache.PendingUpdateLength
      );
    VariableRuntimeCacheContext->VariableRuntimeNvCache.PendingUpdateLength = 0;
    VariableRuntimeCacheContext->VariableRuntimeNvCache.PendingUpdateOffset = 0;

    CopyMem (
      (VOID *)(
               ((UINT8 *)(UINTN)VariableRuntimeCacheContext->VariableRuntimeVolatileCache.Store) +
               VariableRuntimeCacheContext->VariableRuntimeVolatileCache.PendingUpdateOffset
               ),
      (VOID *)(
               ((UINT8 *)(UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase) +
               VariableRuntimeCacheContext->VariableRuntimeVolatileCache.PendingUpdateOffset
               ),
      VariableRuntimeCacheContext->VariableRuntimeVolatileCache.PendingUpdateLength
      );
    VariableRuntimeCacheContext->VariableRuntimeVolatileCache.PendingUpdateLength = 0;
    VariableRuntimeCacheContext->VariableRuntimeVolatileCache.PendingUpdateOffset = 0;
    *(VariableRuntimeCacheContext->PendingUpdate)                                 = FALSE;
  }

  return EFI_SUCCESS;
}

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
  )
{
  if (VariableRuntimeCache == NULL) {
    return EFI_INVALID_PARAMETER;
  } else if (VariableRuntimeCache->Store == NULL) {
    // The runtime cache may not be active or allocated yet.
    // In either case, return EFI_SUCCESS instead of EFI_NOT_AVAILABLE_YET.
    return EFI_SUCCESS;
  }

  if ((mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.PendingUpdate == NULL) ||
      (mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.ReadLock == NULL))
  {
    return EFI_UNSUPPORTED;
  }

  if (*(mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.PendingUpdate) &&
      (VariableRuntimeCache->PendingUpdateLength > 0))
  {
    VariableRuntimeCache->PendingUpdateLength =
      (UINT32)(
               MAX (
                 (UINTN)(VariableRuntimeCache->PendingUpdateOffset + VariableRuntimeCache->PendingUpdateLength),
                 Offset + Length
                 ) - MIN ((UINTN)VariableRuntimeCache->PendingUpdateOffset, Offset)
               );
    VariableRuntimeCache->PendingUpdateOffset =
      (UINT32)MIN ((UINTN)VariableRuntimeCache->PendingUpdateOffset, Offset);
  } else {
    VariableRuntimeCache->PendingUpdateLength = (UINT32)Length;
    VariableRuntimeCache->PendingUpdateOffset = (UINT32)Offset;
  }

  *(mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.PendingUpdate) = TRUE;

  if (*(mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.ReadLock) == FALSE) {
    return FlushPendingRuntimeVariableCacheUpdates ();
  }

  return EFI_SUCCESS;
}

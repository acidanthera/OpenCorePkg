/** @file
  Copyright (c) 2022, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef USER_MEMORY_H
#define USER_MEMORY_H

extern UINTN  mPoolAllocations;
extern UINTN  mPageAllocations;

VOID
ConfigureMemoryAllocations (
  IN     CONST UINT8  *Data,
  IN     UINTN        Size,
  IN OUT UINT32       *ConfigSize
  );

VOID
SetPoolAllocationSizeLimit (
  UINTN  AllocationSize
  );

#endif // USER_MEMORY_H

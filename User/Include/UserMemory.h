/** @file
  Copyright (c) 2022, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef USER_MEMORY_H
#define USER_MEMORY_H

//
// Limits single pool allocation size to 512MB by default.
//
extern UINTN  mPoolAllocationSizeLimit;

extern UINTN  mPoolAllocations;
extern UINTN  mPageAllocations;

extern UINT64  mPoolAllocationMask;
extern UINTN   mPoolAllocationIndex;
extern UINT64  mPageAllocationMask;
extern UINTN   mPageAllocationIndex;

VOID
SetPoolAllocationSizeLimit (
  UINTN  AllocationSize
  );

#endif // USER_MEMORY_H

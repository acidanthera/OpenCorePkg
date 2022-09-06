/** @file
  Copyright (C) 2022, flagers. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/OcAppleKernelLib.h>

EFI_STATUS
PageableContextInit (
  IN OUT  PAGEABLE_CONTEXT   *Context,
  IN OUT  UINT8              *Base,
  IN      UINT32             BaseSize,
  IN      UINT32             BaseAllocSize,
  IN OUT  UINT8              *Pageable,
  IN      UINT32             PageableSize,
  IN      UINT32             PageableAllocSize
  )
{
  EFI_STATUS         Status;

  ASSERT (Context != NULL);
  //
  // 64-bit is implicit, no KernelCollections builder or bootloader supports 32-bit.
  //
  Status = PrelinkedContextInit (&Context->PrelinkedContext, Base, BaseSize, BaseAllocSize, FALSE);
  ASSERT (Context->PrelinkedContext.IsKernelCollection);

  ASSERT (Pageable != NULL);
  ASSERT (PageableSize > 0);
  ASSERT (PageableAllocSize >= PageableSize);

  return Status;
}
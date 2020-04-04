/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "../BootCompatInternal.h"

#include <Library/DebugLib.h>

VOID
AppleMapPlatformSaveState (
  IN OUT ASM_SUPPORT_STATE  *AsmState,
     OUT ASM_KERNEL_JUMP    *KernelJump
  )
{
  //
  // Save current 64-bit state - will be used later in callback from kernel jump
  // to be able to transition to 64-bit in case 32-bit kernel startup code is used.
  //
  AsmAppleMapPlatformSaveState (AsmState);

  //
  // Assembly state must fit into 32-bit address as we may jumo from 32-bit kernel
  // startup code. This is used instead of GetBootCompatContext.
  //
  ASSERT ((UINT32)(UINTN) AsmState == (UINTN) AsmState);
  gOcAbcAsmStateAddr32 = (UINT32)(UINTN) AsmState;

  //
  // Kernel trampoline for jumping to kernel.
  //
  ASSERT (
    (UINT32)(UINTN) AsmAppleMapPlatformPrepareKernelState
    == (UINTN) AsmAppleMapPlatformPrepareKernelState
    );
  KernelJump->MovInst  = 0xB9;
  KernelJump->Addr     = (UINT32)(UINTN) AsmAppleMapPlatformPrepareKernelState;
  KernelJump->CallInst = 0xD1FF;
}

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

#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

//
// Structure definitions shared with ASM code.
// Keep these definitions in sync with ContextSwitch.nasm!
//

#pragma pack(push, 1)

/**
  Assembly support state.
  This state is used as an intermediate structure to hold UEFI environment
  context and kernel environment context for switching between 32-bit
  and 64-bit modes during booting as normal XNU boot still happens in 32-bit.
**/
typedef PACKED struct ASM_SUPPORT_STATE_ {
  VOID    *KernelEntry;
} ASM_SUPPORT_STATE;

/**
  Assembly kernel trampoline.
  This structure contains encoded assembly to jump from kernel
  code to UEFI code through AsmAppleMapPlatformPrepareKernelState
  intermediate handler.
**/
typedef PACKED struct ASM_KERNEL_JUMP_ {
  UINT8   MovInst;
  UINT32  Addr;
  UINT16  CallInst;
} ASM_KERNEL_JUMP;

#pragma pack(pop)

#endif // CONTEXT_SWITCH_H

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
  UINT64  SavedGDTR;
  UINT16  SavedGDTRLimit;
  UINT64  SavedIDTR;
  UINT16  SavedIDTRLimit;
  UINT64  SavedCR3;
  UINT16  SavedCS;
  UINT16  SavedDS;
  UINT16  SavedES;
  UINT16  SavedFS;
  UINT16  SavedGS;

  UINT64  SavedGDTR32;
  UINT16  SavedGDTR32Limit;
  UINT64  SavedIDTR32;
  UINT16  SavedIDTR32Limit;
  UINT16  SavedCS32;
  UINT16  SavedDS32;
  UINT16  SavedES32;
  UINT16  SavedFS32;
  UINT16  SavedGS32;

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

/**
  Assembly interface to save UEFI environment state in specific way.

  @param[in,out]  AsmState      Assembly state to update, can be preserved.
**/
VOID
EFIAPI
AsmAppleMapPlatformSaveState (
  IN OUT ASM_SUPPORT_STATE  *AsmState
  );

/**
  Assembly interface for backjump from kernel code.
  Takes kernel arguments through RAX or EAX register.
**/
VOID
AsmAppleMapPlatformPrepareKernelState (
  );

/**
  Assembly global variable containing ASM_SUPPORT_STATE address.
  Must fit into lower 32-bit bytes due to 32-bit .
**/
extern UINT32 gOcAbcAsmStateAddr32;

#endif // CONTEXT_SWITCH_H

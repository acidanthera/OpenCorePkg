;------------------------------------------------------------------------------
;  @file
;  Copyright (C) 2024, vit9696. All rights reserved.
;
;  All rights reserved.
;
;  This program and the accompanying materials
;  are licensed and made available under the terms and conditions of the BSD License
;  which accompanies this distribution.  The full text of the license may be found at
;  http://opensource.org/licenses/bsd-license.php
;
;  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;------------------------------------------------------------------------------

BITS    32
DEFAULT REL
SECTION  .text

extern ASM_PFX(AppleMapPrepareKernelState32)

align 8
global ASM_PFX(AsmAppleMapPrepareKernelState32)
ASM_PFX(AsmAppleMapPrepareKernelState32):
  ; Pass kernel arguments as an argument.
  ; EDK II does not seem to align stack by more than 4 bytes.
  push eax

  ; Call our C wrapper.
  call ASM_PFX(AppleMapPrepareKernelState32)

  ; Restore stack pointer for ret function.
  ; We might have updated eax via return for relocation block.
  add esp, 4

  ; Execute overwritten instructions by a jump to our asm handler.
  mov bx, ds
  mov es, ebx

  ; Return to kernel.
  ret

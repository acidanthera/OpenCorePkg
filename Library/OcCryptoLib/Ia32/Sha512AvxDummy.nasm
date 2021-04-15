; @file
; Copyright (C) 2021, ISP RAS. All rights reserved.
;
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
BITS 32

section .bss
global ASM_PFX(mIsAvxEnabled)
ASM_PFX(mIsAvxEnabled): db 0

section .text

align 8
global ASM_PFX(TryEnableAvx)
ASM_PFX(TryEnableAvx):
  mov eax, 0
  ret

align 8
global ASM_PFX(Sha512TransformAvx)
ASM_PFX(Sha512TransformAvx):
  ret

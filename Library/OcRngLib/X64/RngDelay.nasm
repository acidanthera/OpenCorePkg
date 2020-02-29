;------------------------------------------------------------------------------
;  @file
;  Copyright (C) 2020, vit9696. All rights reserved.
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

BITS     64
DEFAULT  REL

SECTION  .text

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmAddRngJitter (
;   IN UINT64  Value
;   );
;------------------------------------------------------------------------------
align 8
global ASM_PFX(AsmAddRngJitter)
ASM_PFX(AsmAddRngJitter):
  ; This assembly code corresponds to Hamming Weight implementation for targets
  ; with fast multiplication.
  ; REF: https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
  mov  rax, rcx
  shr  rax, 1
  mov  rdx, 5555555555555555h
  and  rdx, rax
  sub  rcx, rdx
  mov  rax, 3333333333333333h
  mov  rdx, rcx
  and  rdx, rax
  shr  rcx, 2
  and  rcx, rax
  add  rcx, rdx
  mov  rax, rcx
  shr  rax, 4
  lea  rax, [rax+rcx]
  mov  rcx, 0F0F0F0F0F0F0F0Fh
  and  rcx, rax
  mov  rax, 101010101010101h
  imul rax, rcx
  shr  rax, 38h
  ret

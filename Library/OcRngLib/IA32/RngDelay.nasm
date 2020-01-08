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

BITS     32
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
  push esi
  mov  edx, [esp+8]
  mov  eax, [esp+12]
  mov  ecx, edx
  shr  ecx, 1
  mov  esi, eax
  shr  esi, 1
  and  esi, 55555555h
  and  ecx, 55555555h
  sub  edx, ecx
  sbb  eax, esi
  mov  ecx, eax
  and  ecx, 33333333h
  mov  esi, edx
  and  esi, 33333333h
  shr  edx, 2
  shr  eax, 2
  and  eax, 33333333h
  add  eax, ecx
  and  edx, 33333333h
  add  edx, esi
  mov  ecx, eax
  shld ecx, edx, 1Ch
  mov  esi, eax
  shr  esi, 4
  add  ecx, edx
  adc  esi, eax
  and  esi, 0F0F0F0Fh
  and  ecx, 0F0F0F0Fh
  mov  edx, 1010101h
  mov  eax, ecx
  mul  edx
  imul ecx, 1010101h
  add  ecx, edx
  imul eax, esi, 1010101h
  add  eax, ecx
  shr  eax, 18h
  pop  esi
  ret

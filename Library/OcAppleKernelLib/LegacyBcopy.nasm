;------------------------------------------------------------------------------
;  @file
;  Copyright (C) 2020, vit9696. All rights reserved.
;  Copyright (C) 1991,1990 Carnegie Mellon University
;  ws@tools.de (Wolfgang Solfrank, TooLs GmbH) +49-228-985800
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

global ASM_PFX(AsmLegacyBcopy64)
ASM_PFX(AsmLegacyBcopy64):
  xchg       rsi, rdi
  mov        rcx, rdx
  mov        rax, rdi
  sub        rax, rsi
  cmp        rax, rcx     ; overlapping && src < dst?
  jb         AsmLegacyBcopy64Backwards

  cld                     ; nope, copy forwards
  rep
  movsb
  ret

AsmLegacyBcopy64Backwards:
  add        rdi, rcx     ; copy backwards
  add        rsi, rcx
  dec        rdi
  dec        rsi
  and        rcx, 0x7     ; any fractional bytes?
  std
  rep
  movsb
  mov        rcx, rdx     ; copy remainder by 32-bit words
  shr        rcx, 0x3
  sub        rsi, 0x7
  sub        rdi, 0x7
  rep
  movsq
  cld
  ret
ASM_PFX(AsmLegacyBcopy64End)

global ASM_PFX(AsmLegacyBcopy64Size)
ASM_PFX(AsmLegacyBcopy64Size):
  dq ASM_PFX(AsmLegacyBcopy64End) - ASM_PFX(AsmLegacyBcopy64)

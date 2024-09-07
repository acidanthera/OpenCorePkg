;------------------------------------------------------------------------------
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   MathRShiftU64.nasm
;
; Abstract:
;
;   64-bit Math Worker Function.
;   Shifts a 64-bit unsigned value right by a certain number of bits.
;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
;
; void __cdecl __ashrdi3 (void)
;
;------------------------------------------------------------------------------
global ASM_PFX(__ashrdi3)
ASM_PFX(__ashrdi3):
    cmp cl,0x40
    jnc ReturnZero
    cmp cl,0x20
    jnc CounterMore32
    shrd eax,edx,cl
    sar edx,cl
    ret
CounterMore32:
    mov eax,edx
    xor edx,edx
    and cl,0x1f
    sar eax,cl
    ret
ReturnZero:
    xor eax,eax
    xor edx,edx
    ret

;------------------------------------------------------------------------------
;
; void __cdecl __lshrdi3 (void)
;
;------------------------------------------------------------------------------
global ASM_PFX(__lshrdi3)
ASM_PFX(__lshrdi3):
    cmp cl,0x40
    jnc _Exit
    cmp cl,0x20
    jnc More32
    shrd eax,edx,cl
    shr edx,cl
    ret
More32:
    mov eax,edx
    xor edx,edx
    and cl,0x1f
    shr eax,cl
    ret
_Exit:
    xor eax,eax
    xor edx,edx
    ret

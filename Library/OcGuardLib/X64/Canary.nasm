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
BITS 64
extern __security_cookie

section .text

; #######################################################################
; VOID __stack_chk_fail (VOID)
; #######################################################################
align 8
global __stack_chk_fail
__stack_chk_fail:
  int 3
  ret

; #######################################################################
; VOID InitializeSecurityCookie (VOID)
; #######################################################################
align 8
global ASM_PFX(InitializeSecurityCookie)
ASM_PFX(InitializeSecurityCookie):
  mov eax, 1          ; Feature Information
  cpuid               ; result in EAX, EBX, ECX, EDX
  and ecx, 040000000H
  cmp ecx, 040000000H ; check RDRAND feature flag
  jne noRdRand
retry:
  rdrand rdx
  jae retry           ; RDRAND bad data (CF = 0), retry until (CF = 1).
  jmp done
noRdRand:
  rdtsc               ; Read time-stamp counter into EDX:EAX.
  shld rdx, rdx, 32
  or rdx, rax
done:
  mov [rel __security_cookie], rdx
  ret

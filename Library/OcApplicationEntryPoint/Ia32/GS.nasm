; @file
; Copyright (C) 2021, ISP RAS. All rights reserved.
;*
;*   Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
;*   SPDX-License-Identifier: BSD-2-Clause-Patent
;*
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
BITS 32

extern ASM_PFX(__stack_chk_fail)
extern ASM_PFX(__security_cookie)

section .text

; #######################################################################
; VOID __GSHandlerCheck (VOID)
; #######################################################################
align 8
global ASM_PFX(__GSHandlerCheck)
ASM_PFX(__GSHandlerCheck):
  jmp ASM_PFX(__stack_chk_fail)

; #######################################################################
; VOID __report_rangecheckfailure (VOID)
; #######################################################################
align 8
global ASM_PFX(__report_rangecheckfailure)
ASM_PFX(__report_rangecheckfailure):
  jmp ASM_PFX(__stack_chk_fail)

; #######################################################################
; void __fastcall __security_check_cookie(UINTN cookie)
;
; The first two DWORD or smaller arguments that are found in the argument list
; from left to right are passed in ECX and EDX registers.
; All other arguments are passed on the stack from right to left.
;
; At sign (@) is prefixed to names.
; An at sign followed by the number of bytes (in decimal) in the parameter list is suffixed to names.
; #######################################################################
align 8
global @__security_check_cookie@4
@__security_check_cookie@4:
  cmp  dword [rel ASM_PFX(__security_cookie)], ecx  ; UINTN cookie is already in ECX
  jnz  ASM_PFX(__stack_chk_fail)
  ret

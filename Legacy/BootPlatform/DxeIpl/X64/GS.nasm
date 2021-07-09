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
BITS 64

extern ASM_PFX(__stack_chk_fail)

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

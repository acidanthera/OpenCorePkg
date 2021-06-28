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

extern ASM_PFX(FixedPcdGet8)
extern _ModuleEntryPointReal

section .data
align 8
global __security_cookie
__security_cookie:
  dq 0

section .text
;  BIT4 - Enable BreakPoint as ASSERT. (MdePkg.dec)
%define DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED ((ASM_PFX(FixedPcdGet8(PcdDebugPropertyMask)) & 0x10) != 0)

; #######################################################################
; VOID __stack_chk_fail (VOID)
; #######################################################################
align 8
global __stack_chk_fail
__stack_chk_fail:
%if DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED
  int 3
  ret
%else
back:
  cli
  hlt
  jmp back
%endif

; #######################################################################
; EFI_STATUS
; EFIAPI
; _ModuleEntryPoint (
;   IN EFI_PEI_FILE_HANDLE       FileHandle,
;   IN CONST EFI_PEI_SERVICES    **PeiServices
;   )
; #######################################################################
align 8
global _ModuleEntryPoint
_ModuleEntryPoint:
  ; Save FileHandle and **PeiServices.
  mov r8, rcx
  mov r9, rdx

%if ASM_PFX(FixedPcdGet8(PcdCanaryAllowRdtscFallback))
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
%else
again:
  rdrand rdx
  jae again           ; RDRAND bad data (CF = 0), retry until (CF = 1).
%endif

  mov [rel __security_cookie], rdx
  ; Restore FileHandle and **PeiServices.
  mov rcx, r8
  mov rdx, r9
  jmp _ModuleEntryPointReal
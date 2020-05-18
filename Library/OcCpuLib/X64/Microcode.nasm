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

MSR_IA32_BIOS_SIGN_ID       equ     00000008Bh
CPUID_VERSION_INFO          equ     1

SECTION  .text

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; AsmReadIntelMicrocodeRevision (
;   VOID
;   );
;------------------------------------------------------------------------------
align 8
global ASM_PFX(AsmReadIntelMicrocodeRevision)
ASM_PFX(AsmReadIntelMicrocodeRevision):
  ; Several Intel CPUs, notably Westmere, require a certain assembly
  ; sequence to retrieve microcode revision.
  ; Reference: https://github.com/acidanthera/bugtracker/issues/621.
  push rbx
  mov ecx, MSR_IA32_BIOS_SIGN_ID
  xor eax, eax
  xor edx, edx
  wrmsr
  mov eax, CPUID_VERSION_INFO
  cpuid
  mov ecx, MSR_IA32_BIOS_SIGN_ID
  rdmsr
  mov eax, edx
  pop rbx
  ret

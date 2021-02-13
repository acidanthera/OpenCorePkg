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
; VOID
; EFIAPI
; AsmMeasureTicks (
;   IN  UINT32  AcpiTicksDuration,   ///< ecx
;   IN  UINT16  TimerAddr,           ///< dx
;   OUT UINT32  *AcpiTicksDelta,     ///< r8
;   OUT UINT64  *TscTicksDelta       ///< r9
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmMeasureTicks)
ASM_PFX(AsmMeasureTicks):
  ; Push preserved registers.
  push       rsi
  push       rdi
  push       rbp
  push       rbx

  ; Preserve TimerAddr in r11d as rdtsc overrides edx.
  mov        r11d, edx

  ; Read AcpiTicks0 into ebx.
  in         eax, dx
  mov        ebx, eax

  ; Read Tsc0 into r10.
  rdtsc
  shl        rdx, 32
  or         rax, rdx
  mov        r10, rax

  ; Store MAX_UINT32 - AcpiTick0 into ebp.
  mov        ebp, ebx
  not        ebp

  ; Store MAX_UINT24 - AcpiTick0 into edi.
  mov        edi, 0x00ffffff
  sub        edi, ebx

CalculationLoop:
  nop ; or pause.

  ; Read AcpiTicks1 into esi.
  mov        edx, r11d
  in         eax, dx
  mov        esi, eax

  ; Calculate AcpiTicks0 - AcpiTicks1 into eax.
  mov        eax, ebx
  sub        eax, esi
  jbe        NoOverflow

  ; Check for 32-bit overflow.
  cmp        eax, 0x00ffffff
  ja         Overflow32

Overflow24:
  ; AcpiCurrentDelta (esi) = AcpiTicks1 (esi) + MAX_UINT24 - AcpiTick0 (edi)
  add        esi, edi
  jmp        ContinueLoop

NoOverflow:
  ; AcpiCurrentDelta (esi) = AcpiTicks1 (esi) - AcpiTicks0 (ebx)
  sub        esi, ebx
  jmp        ContinueLoop

Overflow32:
  ; AcpiCurrentDelta (esi) = AcpiTicks1 (esi) + MAX_UINT32 - AcpiTick0 (ebp)
  add        esi, ebp

ContinueLoop:
  cmp        esi, ecx
  jb         CalculationLoop

  ; Read Tsc1 into rax.
  rdtsc
  shl        rdx, 32
  or         rax, rdx

  ; Calculate TscDelta into rax.
  sub        rax, r10

  ; Store TscDelta and AcpiDelta.
  mov        qword [r9], rax
  mov        dword [r8], esi

  ; Pop preserved registers & return.
  pop        rbx
  pop        rbp
  pop        rdi
  pop        rsi
  ret

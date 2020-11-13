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

BITS    64
DEFAULT REL

;------------------------------------------------------------------------------
; Copied from BootCompatInternal.h, keep in sync.
;------------------------------------------------------------------------------
%define ESTIMATED_CALL_GATE_SIZE 256
%define KERNEL_BASE_PADDR        0x100000

;------------------------------------------------------------------------------
; Copy kernel memory to lower memory and jump back to kernel call gate.
;
; To generate the binary blob execute the following command:
; nasm RelocationCallGate.nasm -o /dev/stdout | xxd -i > RelocationCallGate.h
;
; @param[in]  QWordCount   Number of QWORDS to copy (rcx).
; @param[in]  EntryPoint   Kernel entry point (rdx).
; @param[in]  Source       Relocation block address to copy from (r8).
; @param[in]  Args         Kernel arguments (r9).
;
; Kernel call gate resides ESTIMATED_CALL_GATE_SIZE above and expects
; Args (rcx), EntryPoint (rdx) arguments to be passed.
;
; UINTN
; EFIAPI
; AsmCopySelf (
;   IN UINTN                  QWordCount,
;   IN UINTN                  EntryPoint,
;   IN EFI_PHYSICAL_ADDRESS   Source,
;   IN UINTN                  Args
;   );
;------------------------------------------------------------------------------
AsmRelocationCallGate:
  ; Disable interrupts just in case UEFI timer kills us.
  cli

  ; Perform copying with direction reset.
  cld
  mov rsi, r8
  mov edi, KERNEL_BASE_PADDR
  rep movsq

  ; Update stack pointer to point to the relocation block (just in case).
  mov rsp, rsi

  ; Print K and die (useful for testing).
;  mov        cl, 0x4b
;  mov        dx, 0x3fd
;ready1:
;  in         al, dx
;  test       al, 0x20
;  je         ready1
;  mov        dx, 0x3f8
;  mov        al, cl
;  out        dx, al
;  mov        cl, 0xa
;  mov        dx, 0x3fd
;ready2:
;  in         al, dx
;  test       al, 0x20
;  je         ready2
;  mov        dx, 0x3f8
;  mov        al, cl
;  out        dx, al
;freeze:
;  jmp        freeze

  ; Move Args to the first argument.
  mov rcx, r9

  ; Jump back to the Apple call gate.
  jmp AsmRelocationCallGate - ESTIMATED_CALL_GATE_SIZE

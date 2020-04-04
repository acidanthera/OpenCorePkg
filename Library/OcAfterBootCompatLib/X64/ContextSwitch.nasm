;------------------------------------------------------------------------------
;  @file
;  Copyright (C) 2013, dmazar. All rights reserved.
;  Copyright (C) 2019, vit9696. All rights reserved.
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

;------------------------------------------------------------------------------
; Structure definitions shared with C code.
; Keep these definitions in sync with ContextSwitch.h!
;------------------------------------------------------------------------------

struc ASM_SUPPORT_STATE
  ;------------------------------------------------------------------------------
  ; 64-bit state
  ;------------------------------------------------------------------------------

  .SavedGDTR          resq 1
  .SavedGDTRLimit     resw 1
  .SavedIDTR          resq 1
  .SavedIDTRLimit     resw 1
  .SavedCR3           resq 1
  .SavedCS            resw 1
  .SavedDS            resw 1
  .SavedES            resw 1
  .SavedFS            resw 1
  .SavedGS            resw 1

  ;------------------------------------------------------------------------------
  ; 32-bit state
  ;------------------------------------------------------------------------------

  .SavedGDTR32        resq 1
  .SavedGDTR32Limit   resw 1
  .SavedIDTR32        resq 1
  .SavedIDTR32Limit   resw 1
  .SavedCS32          resw 1
  .SavedDS32          resw 1
  .SavedES32          resw 1
  .SavedFS32          resw 1
  .SavedGS32          resw 1

  ;------------------------------------------------------------------------------
  ; Kernel entry address.
  ;------------------------------------------------------------------------------

  .KernelEntry        resq 1

  .Size:
endstruc

struc ASM_KERNEL_JUMP
  .MovInst            resb 1
  .Addr               resd 1
  .CallInst           resw 1

  .Size:
endstruc

;------------------------------------------------------------------------------
; C callback method called on jump to kernel after boot.efi finishes.
;------------------------------------------------------------------------------

extern ASM_PFX(AppleMapPrepareKernelState)

SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmAppleMapPlatformSaveState (
;   OUT ASM_SUPPORT_STATE  *AsmState
;   );
;------------------------------------------------------------------------------
align 8
global ASM_PFX(AsmAppleMapPlatformSaveState)
ASM_PFX(AsmAppleMapPlatformSaveState):
BITS  64
  sgdt  [rcx + ASM_SUPPORT_STATE.SavedGDTR]
  sidt  [rcx + ASM_SUPPORT_STATE.SavedIDTR]
  mov   rax, cr3
  mov   [rcx + ASM_SUPPORT_STATE.SavedCR3], rax
  mov   word [rcx + ASM_SUPPORT_STATE.SavedCS], cs
  mov   word [rcx + ASM_SUPPORT_STATE.SavedDS], ds
  mov   word [rcx + ASM_SUPPORT_STATE.SavedES], es
  mov   word [rcx + ASM_SUPPORT_STATE.SavedFS], fs
  mov   word [rcx + ASM_SUPPORT_STATE.SavedGS], gs
  ret

;------------------------------------------------------------------------------
; Apple kernel starts through call gate, an assembly structure allocated in
; 32-bit high memory, that transitions to 32-bit mode and then calls the kernel
; with 32-bit GDT and UEFI stack.
;
; KernelCallGate:
;   lea     rax, StartKernelIn32Bit
;   mov     cs:gKernelBooter32, eax
;   lea     rax, gKernelGdtTable
;   mov     cs:gKernelGdtBase, rax
;   lgdt    fword ptr cs:gKernelGdtLimit
;   mov     ax, 10h
;   mov     ds, ax
;   mov     es, ax
;   mov     gs, ax
;   mov     fs, ax
;   lea     rax, gKernelBooter32
;   jmp     fword ptr [rax]
;
; StartKernelIn32Bit:
;   mov     rax, cr0
;   btr     eax, 1Fh
;   mov     cr0, rax
;   mov     ebx, ecx        ; ebx = boot-args
;   mov     edi, edx
;   ; Disable long mode
;   mov     ecx, 0C0000080h ; EFER MSR number.
;   rdmsr
;   btr     eax, 8          ; Set LME=0.
;   wrmsr
;   jmp     short SwitchTo32Bit
;
; SwitchTo32Bit:
;   mov     eax, ebx
;   jmp     rdi             ; Jump to kernel
;   hlt
;   ret
;
; gKernelBooter32:
;   dd 0
;   dw 8                    ; New CS value
; gKernelGdtLimit:          ; IA32_DESCRIPTOR
;   dw 18h
; gKernelGdtBase:
;   dq 0
; gKernelGdtTable:          ; Array of IA32_GDT.
;   dw 0                    ; [0] = LimitLow
;   dw 0                    ; [0] = BaseLow
;   db 0                    ; [0] = BaseMid
;   dw 0                    ; [0] = Flags
;   db 0                    ; [0] = BaseHigh
;   dw 0FFFFh               ; [1] = LimitLow
;   dw 0                    ; [1] = BaseLow
;   db 0                    ; [1] = BaseMid
;   dw 0CF9Eh               ; [1] - Flags
;   db 0                    ; [1] = BaseHigh
;   dw 0FFFFh               ; [2] = LimitLow
;   dw 0                    ; [2] = BaseLow
;   db 0                    ; [2] = BaseMid
;   dw 0CF92h               ; [2] = Flags
;   db 0                    ; [2] = BaseHigh
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Long (far) return.
; retfq (lretq) - 64-bit encoding 48 CB
; retf  (lret)  - 32-bit encoding CB
;------------------------------------------------------------------------------
LONG_RET64:
  db  048h
LONG_RET32:
  db  0CBh

;------------------------------------------------------------------------------
; AsmAppleMapPlatformPrepareKernelState
;
; Callback from boot.efi - this is where we jump when boot.efi jumps to kernel.
; eax register contains boot arguments for the kernel.
;
; - test if we are in 32 bit or in 64 bit
; - if 64 bit, then jump to AsmJumpFromKernel64
; - else just continue with AsmJumpFromKernel32
;------------------------------------------------------------------------------
global ASM_PFX(AsmAppleMapPlatformPrepareKernelState)
ASM_PFX(AsmAppleMapPlatformPrepareKernelState):
BITS  32
  push  eax                     ; save bootArgs pointer to stack
  mov   dword ecx, 0C0000080h   ; EFER MSR number.
  rdmsr                         ; Read EFER.
  bt    eax, 8                  ; Check if LME==1 -> CF=1.
  pop   eax
  jc    AsmJumpFromKernel64     ; LME==1 -> jump to 64 bit code
  ; otherwise, continue with AsmJumpFromKernel32

; Above 32-bit code must give the opcodes equivalent to following in 64-bit.
;BITS  64
;  push   rax                  ; save bootArgs pointer to stack
;  mov    ecx, C0000080h       ; EFER MSR number.
;  rdmsr                       ; Read EFER.
;  bt     eax, 8               ; Check if LME==1 -> CF=1.
;  pop    rax
;  jc    AsmJumpFromKernel64   ; LME==1 -> jump to 64 bit code

;------------------------------------------------------------------------------
; AsmJumpFromKernel32
;
; Callback from boot.efi in 32 bit mode.
;------------------------------------------------------------------------------
AsmJumpFromKernel32:
BITS  32
  ; Save bootArgs pointer to edi.
  mov  edi, eax

  ; Load ebx with AsmState - we'll access our saved data with it.
  db   0BBh       ; mov ebx, OFFSET DataBase

;------------------------------------------------------------------------------
; 32-bit pointer to AsmState used to reduce global variable access.
; Defined here becuase 32-bit mode does not support relative addressing.
; As both jumps can happen from 64-bit kernel, the address must fit in 4 bytes.
;------------------------------------------------------------------------------
global ASM_PFX(gOcAbcAsmStateAddr32)
ASM_PFX(gOcAbcAsmStateAddr32):
  dd   0

  ; Store kernel entery point prior to hunk code.
  pop  ecx
  sub  ecx, ASM_KERNEL_JUMP.Size
  mov  dword [ebx + ASM_SUPPORT_STATE.KernelEntry], ecx

  ; Store 32-bit state to be able to recover it later.
  sgdt [ebx + ASM_SUPPORT_STATE.SavedGDTR32]
  sidt [ebx + ASM_SUPPORT_STATE.SavedIDTR32]
  mov  word [ebx + ASM_SUPPORT_STATE.SavedCS32], cs
  mov  word [ebx + ASM_SUPPORT_STATE.SavedDS32], ds
  mov  word [ebx + ASM_SUPPORT_STATE.SavedES32], es
  mov  word [ebx + ASM_SUPPORT_STATE.SavedFS32], fs
  mov  word [ebx + ASM_SUPPORT_STATE.SavedGS32], gs

  ;
  ; Transition to 64-bit mode...
  ; boot.efi disables interrupts for us, so we are safe.
  ;

  ; Load saved UEFI GDT and IDT.
  ; They will become active after code segment is changed in long jump.
  lgdt  [ebx + ASM_SUPPORT_STATE.SavedGDTR]
  lidt  [ebx + ASM_SUPPORT_STATE.SavedIDTR]

  ; Enable the 64-bit page-translation-table entries by setting CR4.PAE=1.
  mov   eax, cr4
  bts   eax, 5
  mov   cr4, eax

  ; Set the long-mode page tables by reusing saved UEFI tables.
  mov   eax, dword [ebx + ASM_SUPPORT_STATE.SavedCR3]
  mov   cr3, eax

  ; Enable long mode (set EFER.LME=1).
  mov   ecx, 0C0000080h      ; EFER MSR number.
  rdmsr                      ; Read EFER.
  bts   eax, 8               ; Set LME=1.
  wrmsr                      ; Write EFER.

  ; Enable paging to activate long mode (set CR0.PG=1).
  mov   eax, cr0             ; Read CR0.
  bts   eax, 31              ; Set PG=1.
  mov   cr0, eax             ; Write CR0.

  ; Jump to the 64-bit code segment.
  mov   ax, word [ebx + ASM_SUPPORT_STATE.SavedCS]
  push  eax
  call  LONG_RET32

BITS 64

  ;
  ; Done transitioning to 64-bit code.
  ;

  ; Set other segment selectors. In most firmwares all segment registers but cs
  ; point to the same selector, but we must not rely on it.
  mov   ax, word [rbx + ASM_SUPPORT_STATE.SavedDS]
  mov   ds, ax
  mov   ax, word [rbx + ASM_SUPPORT_STATE.SavedES]
  mov   es, ax
  mov   ax, word [rbx + ASM_SUPPORT_STATE.SavedFS]
  mov   fs, ax
  mov   ax, word [rbx + ASM_SUPPORT_STATE.SavedGS]
  mov   gs, ax

  ; boot.efi preserves ss selector from UEFI GDT to just use UEFI stack (and memory) as is.
  ; For this reason just assume the stack is useable but align it for definite 64-bit compat.
  and   rsp, 0FFFFFFFFFFFFFFF0h

  ; Call AppleMapPrepareKernelState (rcx = rax = bootArgs, rdx = 0 = 32 bit kernel jump).
  mov   rcx, rdi
  xor   edx, edx
  push  rdx
  push  rdx
  push  rdx
  push  rcx
  call  ASM_PFX(AppleMapPrepareKernelState)

  ; Return value in rax is bootArgs pointer again.
  mov   rdi, rax

  ;
  ; Transition back to 32-bit.
  ;

  ; Load saved 32-bit GDT.
  lgdt  [rbx + ASM_SUPPORT_STATE.SavedGDTR32]

  ; Jump to the 32-bit code segment.
  mov   ax, word [rbx + ASM_SUPPORT_STATE.SavedCS32]
  push  rax
  call  LONG_RET64

BITS  32

  ;
  ; Done transitioning to 32-bit code.
  ;

  ; Disable paging (set CR0.PG=0).
  mov   eax, cr0        ; Read CR0.
  btr   eax, 31         ; Set PG=0.
  mov   cr0, eax        ; Write CR0.

  ; Disable long mode (set EFER.LME=0).
  mov   ecx, 0C0000080h  ; EFER MSR number.
  rdmsr                  ; Read EFER.
  btr   eax, 8           ; Set LME=0.
  wrmsr                  ; Write EFER.
  jmp   AsmJumpFromKernel32Compatibility

AsmJumpFromKernel32Compatibility:

  ;
  ; We are in 32-bit protected mode, no paging.
  ;

  ; Reload saved 32 bit state data.
  ; Since boot.efi relies on segment registers shadow part and preserves ss value,
  ; which contains previously read GDT data, we are not allowed to later update it.
  lidt  [ebx + ASM_SUPPORT_STATE.SavedIDTR32]
  mov   ax, word [ebx + ASM_SUPPORT_STATE.SavedDS32]
  mov   ds, ax
  mov   ax, word [ebx + ASM_SUPPORT_STATE.SavedES32]
  mov   es, ax
  mov   ax, word [ebx + ASM_SUPPORT_STATE.SavedFS32]
  mov   fs, ax
  mov   ax, word [ebx + ASM_SUPPORT_STATE.SavedGS32]
  mov   gs, ax

  ; Jump back to the kernel passing boot arguments in eax.
  mov   eax, edi
  mov   edx, dword [ebx + ASM_SUPPORT_STATE.KernelEntry]
  jmp   edx

;------------------------------------------------------------------------------
; AsmJumpFromKernel64
;
; Callback from boot.efi in 64 bit mode.
; State is prepared for kernel: 64 bit, pointer to bootArgs in rax.
;------------------------------------------------------------------------------
AsmJumpFromKernel64:
BITS  64
  ; Load rbx with AsmState - we'll access our saved data with it.
  mov  ebx, dword [ASM_PFX(gOcAbcAsmStateAddr32)]

  ; Store kernel entery point prior to hunk code.
  pop  rcx
  sub  rcx, ASM_KERNEL_JUMP.Size
  mov  qword [rbx + ASM_SUPPORT_STATE.KernelEntry], rcx

  ; Call AppleMapPrepareKernelState (rcx = rax = bootArgs, rdx = 1 = 64-bit kernel jump).
  mov    rcx, rax
  xor    edx, edx
  push   rdx
  push   rdx
  push   rdx
  push   rcx
  inc    edx
  call  ASM_PFX(AppleMapPrepareKernelState)

  ; Jump back to the kernel passing boot arguments in rax.
  mov    rdx, [rbx + ASM_SUPPORT_STATE.KernelEntry]
  jmp    rdx

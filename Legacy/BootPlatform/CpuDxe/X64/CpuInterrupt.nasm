;      TITLE   CpuInterrupt.nasm:
;------------------------------------------------------------------------------
;*
;*   Copyright 2006 - 2010, Intel Corporation                                                         
;*   All rights reserved. This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    CpuInterrupt.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------
global ASM_PFX(InstallInterruptHandler)
global ASM_PFX(InitDescriptor)
global ASM_PFX(SystemExceptionHandler)
global ASM_PFX(SystemTimerHandler)

SECTION .text

EXTERN ASM_PFX(TimerHandler)
EXTERN ASM_PFX(ExceptionHandler)


ASM_PFX(InitDescriptor):
        lea     rax, [REL GDT_BASE]         ; RAX=PHYSICAL address of gdt
        mov     [REL gdtr + 2], rax         ; Put address of gdt into the gdtr
        lgdt    [REL gdtr]
        mov     eax, 18h
        mov     gs, eax
        mov     fs, eax
        lea     rax, [REL IDT_BASE]         ; RAX=PHYSICAL address of idt
        mov     qword  [REL idtr + 2], rax  ; Put address of idt into the idtr
        lidt    [REL idtr]
        ret


; VOID
; EFIAPI
; InstallInterruptHandler (
;     UINTN Vector,           // rcx
;     void  (*Handler)(void)  // rdx
;     )
ASM_PFX(InstallInterruptHandler):
        push    rbx
        pushfq                              ; save eflags
        cli                                 ; turn off interrupts
        sub     rsp, 10h                    ; open some space on the stack
        mov     rbx, rsp
        sidt    [rbx]                       ; get  tword address of IDT
        mov     rbx, [rbx+2]                ; move offset of IDT into RBX
        add     rsp, 10h                    ; correct stack
        mov     rax, rcx                    ; Get vector number
        shl     rax, 4                      ; multiply by 16 to get offset
        add     rbx, rax                    ; add to IDT base to get entry
        mov     rax, rdx                    ; load new address into IDT entry
        mov     word  [rbx], ax          ; write bits 15..0 of offset
        shr     rax, 16                     ; use ax to copy 31..16 to descriptors
        mov     word  [rbx+6], ax        ; write bits 31..16 of offset
        shr     rax, 16                     ; use eax to copy 63..32 to descriptors
        mov     dword  [rbx+8], eax      ; write bits 63..32 of offset
        popfq                        ; restore flags (possible enabling interrupts)
        pop     rbx
        ret

%macro JmpCommonIdtEntry 0
    ; jmp     commonIdtEntry - this must be hand coded to keep the assembler from
    ;                          using a 8 bit reletive jump when the entries are
    ;                          within 255 bytes of the common entry.  This must
    ;                          be done to maintain the consistency of the size
    ;                          of entry points...
    db      0e9h                        ; jmp 16 bit reletive 
    dd      commonIdtEntry - $ - 4      ;  offset to jump to
%endmacro

     ALIGN 02h,db 0
ASM_PFX(SystemExceptionHandler):
INT0:
    push    0h      ; push error code place holder on the stack
    push    0h
    JmpCommonIdtEntry
;    db      0e9h                        ; jmp 16 bit reletive 
;    dd      commonIdtEntry - $ - 4      ;  offset to jump to
    
INT1:
    push    0h      ; push error code place holder on the stack
    push    1h
    JmpCommonIdtEntry
    
INT2:
    push    0h      ; push error code place holder on the stack
    push    2h
    JmpCommonIdtEntry
    
INT3:
    push    0h      ; push error code place holder on the stack
    push    3h
    JmpCommonIdtEntry
    
INT4:
    push    0h      ; push error code place holder on the stack
    push    4h
    JmpCommonIdtEntry
    
INT5:
    push    0h      ; push error code place holder on the stack
    push    5h
    JmpCommonIdtEntry
    
INT6:
    push    0h      ; push error code place holder on the stack
    push    6h
    JmpCommonIdtEntry
    
INT7:
    push    0h      ; push error code place holder on the stack
    push    7h
    JmpCommonIdtEntry
    
INT8:
;   Double fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    8h
    JmpCommonIdtEntry
    
INT9:
    push    0h      ; push error code place holder on the stack
    push    9h
    JmpCommonIdtEntry
    
INT10:
;   Invalid TSS causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    10
    JmpCommonIdtEntry
    
INT11:
;   Segment Not Present causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    11
    JmpCommonIdtEntry
    
INT12:
;   Stack fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    12
    JmpCommonIdtEntry
    
INT13:
;   GP fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    13
    JmpCommonIdtEntry
    
INT14:
;   Page fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    14
    JmpCommonIdtEntry
    
INT15:
    push    0h      ; push error code place holder on the stack
    push    15
    JmpCommonIdtEntry
    
INT16:
    push    0h      ; push error code place holder on the stack
    push    16
    JmpCommonIdtEntry
    
INT17:
;   Alignment check causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    17
    JmpCommonIdtEntry
    
INT18:
    push    0h      ; push error code place holder on the stack
    push    18
    JmpCommonIdtEntry
    
INT19:
    push    0h      ; push error code place holder on the stack
    push    19
    JmpCommonIdtEntry

INTUnknown:
%rep  (32 - 20)
    push    0h      ; push error code place holder on the stack
;    push    xxh     ; push vector number
    db      06ah
    db      ( $ - INTUnknown - 3 ) / 9 + 20 ; vector number
    JmpCommonIdtEntry
%endrep
;ENDM

ASM_PFX(SystemTimerHandler):
    push    0
    push    0  ;mTimerVector ;to be patched in Cpu.c
    JmpCommonIdtEntry

commonIdtEntry:
; +---------------------+ <-- 16-byte aligned ensured by processor
; +    Old SS           +
; +---------------------+
; +    Old RSP          +
; +---------------------+
; +    RFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    RIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; +    Vector Number    +
; +---------------------+
; +    RBP              +
; +---------------------+ <-- RBP, 16-byte aligned

  cli
  push rbp
  mov  rbp, rsp

  ;
  ; Since here the stack pointer is 16-byte aligned, so
  ; EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
  ; is 16-byte aligned
  ;       

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rax
  push rcx
  push rdx
  push rbx
  push qword  [rbp + 6 * 8]  ; RSP
  push qword  [rbp]          ; RBP
  push rsi
  push rdi

;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
  movzx   rax, word  [rbp + 7 * 8]
  push    rax                      ; for ss
  movzx   rax, word  [rbp + 4 * 8]
  push    rax                      ; for cs
  mov     rax, ds
  push    rax
  mov     rax, es
  push    rax
  mov     rax, fs
  push    rax
  mov     rax, gs
  push    rax

;; UINT64  Rip;
  push    qword  [rbp + 3 * 8]

;; UINT64  Gdtr[2], Idtr[2];
  sub     rsp, 16
  sidt       [rsp]
  sub     rsp, 16
  sgdt       [rsp]

;; UINT64  Ldtr, Tr;
  xor     rax, rax
  str     ax
  push    rax
  sldt    ax
  push    rax

;; UINT64  RFlags;
  push    qword  [rbp + 5 * 8]

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
  mov     rax, cr8
  push    rax
  mov     rax, cr4
  or      rax, 208h
  mov     cr4, rax
  push    rax
  mov     rax, cr3
  push    rax
  mov     rax, cr2
  push    rax
  xor     rax, rax
  push    rax
  mov     rax, cr0
  push    rax

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  mov     rax, dr7
  push    rax
;; clear Dr7 while executing debugger itself
  xor     rax, rax
  mov     dr7, rax

  mov     rax, dr6
  push    rax
;; insure all status bits in dr6 are clear...
  xor     rax, rax
  mov     dr6, rax

  mov     rax, dr3
  push    rax
  mov     rax, dr2
  push    rax
  mov     rax, dr1
  push    rax
  mov     rax, dr0
  push    rax

;; FX_SAVE_STATE_X64 FxSaveState;

  sub rsp, 512
  mov rdi, rsp
  db 0fh, 0aeh, 00000111y ;fxsave [rdi]

;; UINT32  ExceptionData;
  push    qword  [rbp + 2 * 8]

;; call into exception handler
;; Prepare parameter and call
  mov     rcx, qword  [rbp + 1 * 8]
  mov     rdx, rsp
  ;
  ; Per X64 calling convention, allocate maximum parameter stack space
  ; and make sure RSP is 16-byte aligned
  ;
  sub     rsp, 4 * 8 + 8
  cmp     rcx, 32
  jb      CallException
  call    ASM_PFX(TimerHandler)
  jmp     ExceptionDone
CallException:
  call  NEAR  ASM_PFX(ExceptionHandler)
ExceptionDone:
  add     rsp, 4 * 8 + 8

  cli
;; UINT64  ExceptionData;
  add     rsp, 8

;; FX_SAVE_STATE_X64 FxSaveState;

  mov rsi, rsp
  db 0fh, 0aeh, 00001110y ; fxrstor [rsi]
  add rsp, 512

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  pop     rax
  mov     dr0, rax
  pop     rax
  mov     dr1, rax
  pop     rax
  mov     dr2, rax
  pop     rax
  mov     dr3, rax
;; skip restore of dr6.  We cleared dr6 during the context save.
  add     rsp, 8
  pop     rax
  mov     dr7, rax

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
  pop     rax
  mov     cr0, rax
  add     rsp, 8   ; not for Cr1
  pop     rax
  mov     cr2, rax
  pop     rax
  mov     cr3, rax
  pop     rax
  mov     cr4, rax
  pop     rax
  mov     cr8, rax

;; UINT64  RFlags;
  pop     qword  [rbp + 5 * 8]

;; UINT64  Ldtr, Tr;
;; UINT64  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
  add     rsp, 48

;; UINT64  Rip;
  pop     qword  [rbp + 3 * 8]

;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;
  pop     rax
  ; mov     gs, rax ; not for gs
  pop     rax
  ; mov     fs, rax ; not for fs
  ; (X64 will not use fs and gs, so we do not restore it)
  pop     rax
  mov     es, rax
  pop     rax
  mov     ds, rax
  pop     qword  [rbp + 4 * 8]  ; for cs
  pop     qword  [rbp + 7 * 8]  ; for ss

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
  pop     rdi
  pop     rsi
  add     rsp, 8                  ; not for rbp
  pop     qword  [rbp + 6 * 8] ; for rsp
  pop     rbx
  pop     rdx
  pop     rcx
  pop     rax
  pop     r8
  pop     r9
  pop     r10
  pop     r11
  pop     r12
  pop     r13
  pop     r14
  pop     r15

  mov     rsp, rbp
  pop     rbp
  add     rsp, 16
  iretq

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SECTION .data

gdtr    dw GDT_END - GDT_BASE - 1   ; GDT limit
        dq 0   ;GDT_BASE                     ; (GDT base gets set above)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   global descriptor table (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         ALIGN 010h,db 0                      ; make GDT 16-byte align

;global GDT_BASE
GDT_BASE:
; null descriptor
NULL_SEL        equ $-GDT_BASE          ; Selector [0x0]
        dw 0            ; limit 15:0
        dw 0            ; base 15:0
        db 0            ; base 23:16
        db 0            ; type
        db 0            ; limit 19:16, flags
        db 0            ; base 31:24

; linear data segment descriptor
LINEAR_SEL      equ $-GDT_BASE          ; Selector [0x8]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh         ; page-granular, 32-bit
        db 0

; linear code segment descriptor
LINEAR_CODE_SEL equ $-GDT_BASE          ; Selector [0x10]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, code, expand-up, writable
        db 0CFh         ; page-granular, 32-bit
        db 0

; system data segment descriptor
SYS_DATA_SEL    equ $-GDT_BASE          ; Selector [0x18]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh         ; page-granular, 32-bit
        db 0

; system code segment descriptor
SYS_CODE_SEL    equ $-GDT_BASE          ; Selector [0x20]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, code, expand-up, writable
        db 0CFh         ; page-granular, 32-bit
        db 0

; spare segment descriptor
SPARE3_SEL      equ $-GDT_BASE          ; Selector [0x28]
        dw 0
        dw 0
        db 0
        db 0
        db 0
        db 0

; system data segment descriptor
SYS_DATA64_SEL    equ $-GDT_BASE          ; Selector [0x30]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh         ; page-granular, 32-bit
        db 0

; system code segment descriptor
SYS_CODE64_SEL    equ $-GDT_BASE          ; Selector [0x38]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, code, expand-up, writable
        db 0AFh         ; page-granular, 64-bit
        db 0

; spare segment descriptor
SPARE4_SEL  equ $-GDT_BASE            ; Selector [0x40]
        dw 0
        dw 0
        db 0
        db 0
        db 0
        db 0

GDT_END:

idtr    dw IDT_END - IDT_BASE - 1   ; IDT limit
        dq 0      ;IDT_BASE                  ; (IDT base gets set above)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   interrupt descriptor table (IDT)
;
;   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
;       mappings.  This implementation only uses the system timer and all other
;       IRQs will remain masked.  The descriptors for vectors 33+ are provided
;       for convenience.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         ALIGN 08h, db 0       ; make IDT 8-byte align

;global IDT_BASE
IDT_BASE:
; divide by zero (INT 0)
DIV_ZERO_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; debug exception (INT 1)
DEBUG_EXCEPT_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; NMI (INT 2)
NMI_SEL             equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; soft breakpoint (INT 3)
BREAKPOINT_SEL      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; overflow (INT 4)
OVERFLOW_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; bounds check (INT 5)
BOUNDS_CHECK_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; invalid opcode (INT 6)
INVALID_OPCODE_SEL  equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; device not available (INT 7)
DEV_NOT_AVAIL_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; double fault (INT 8)
DOUBLE_FAULT_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; Coprocessor segment overrun - reserved (INT 9)
RSVD_INTR_SEL1      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; invalid TSS (INT 0ah)
INVALID_TSS_SEL     equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; segment not present (INT 0bh)
SEG_NOT_PRESENT_SEL equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; stack fault (INT 0ch)
STACK_FAULT_SEL     equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; general protection (INT 0dh)
GP_FAULT_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; page fault (INT 0eh)
PAGE_FAULT_SEL      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; Intel reserved - do not use (INT 0fh)
RSVD_INTR_SEL2      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; floating point error (INT 10h)
FLT_POINT_ERR_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; alignment check (INT 11h)
ALIGNMENT_CHECK_SEL equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; machine check (INT 12h)
MACHINE_CHECK_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; SIMD floating-point exception (INT 13h)
SIMD_EXCEPTION_SEL  equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

%rep  (32 - 20)
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved
%endrep

; 72 unspecified descriptors
;        db (72 * 16) dup(0)
TIMES (72 * 16) db 0
        
; IRQ 0 (System timer) - (INT 68h)
IRQ0_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 1 (8042 Keyboard controller) - (INT 69h)
IRQ1_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
IRQ2_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 3 (COM 2) - (INT 6bh)
IRQ3_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 4 (COM 1) - (INT 6ch)
IRQ4_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 5 (LPT 2) - (INT 6dh)
IRQ5_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 6 (Floppy controller) - (INT 6eh)
IRQ6_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 7 (LPT 1) - (INT 6fh)
IRQ7_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 8 (RTC Alarm) - (INT 70h)
IRQ8_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 9 - (INT 71h)
IRQ9_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 10 - (INT 72h)
IRQ10_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 11 - (INT 73h)
IRQ11_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 12 (PS/2 mouse) - (INT 74h)
IRQ12_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 13 (Floating point error) - (INT 75h)
IRQ13_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 14 (Secondary IDE) - (INT 76h)
IRQ14_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 15 (Primary IDE) - (INT 77h)
IRQ15_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

;        db (1 * 16) dup(0)
TIMES 16 db 0

IDT_END:


;END

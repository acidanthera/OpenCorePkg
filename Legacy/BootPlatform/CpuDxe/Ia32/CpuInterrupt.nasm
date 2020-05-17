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

;
; Workaround for NASM bug emitting incorrect
;   pc-relative relocations to mach-o i386
;   object files.
;
%ifidn __OUTPUT_FORMAT__, macho
%macro    MakeCall 2
  mov     %1, %2
  call    %1
%endmacro
%elifidn __OUTPUT_FORMAT__, macho32
%macro    MakeCall 2
  mov     %1, %2
  call    %1
%endmacro
%else
%macro    MakeCall 2
  call    %2
%endmacro
%endif

SECTION .text

EXTERN ASM_PFX(TimerHandler)
EXTERN ASM_PFX(ExceptionHandler)


ASM_PFX(InitDescriptor):
        mov     eax, GDT_BASE             ; EAX=PHYSICAL address of gdt
        mov     [gdtr + 2], eax           ; Put address of gdt into the gdtr
        lgdt    [gdtr]

        mov     eax, IDT_BASE             ; EAX=PHYSICAL address of idt
        mov     [idtr + 2], eax           ; Put address of idt into the idtr
        lidt    [idtr]
        ret


; VOID
; EFIAPI
; InstallInterruptHandler (
;     UINTN Vector,             //6(SP)
;     void  (*Handler)(void)    //10(SP)
;     )
ASM_PFX(InstallInterruptHandler):
      push    edi
      pushfd                              ; save eflags
      cli                                 ; turn off interrupts
      sub     esp, 6                      ; open some space on the stack
      mov     edi, esp
      sidt    [edi]                       ; get fword address of IDT
      mov     edi, [edi+2]                ; move offset of IDT into EDI
      add     esp, 6                      ; correct stack
      mov     eax, [esp+12]               ; Get vector number
      shl     eax, 3                      ; multiply by 8 to get offset
      add     edi, eax                    ; add to IDT base to get entry
      mov     eax, [esp+16]               ; load new address into IDT entry
      mov     word [edi], ax              ; write bits 15..0 of offset
      shr     eax, 16                     ; use ax to copy 31..16 to descriptors
      mov     word [edi+6], ax            ; write bits 31..16 of offset
      popfd                       ; restore flags (possible enabling interrupts)
      pop     edi
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

     ALIGN 02h
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
; +---------------------+
; +    EFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    EIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; +    Vector Number    +
; +---------------------+
; +    EBP              +
; +---------------------+ <-- EBP

  cli
  push ebp
  mov  ebp, esp

;
; Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
; is 16-byte aligned
;
  and     esp, 0fffffff0h
  sub     esp, 12

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
  push    eax
  push    ecx
  push    edx
  push    ebx
  lea     ecx, [ebp + 6 * 4]
  push    ecx                          ; ESP
  push    dword  [ebp]              ; EBP
  push    esi
  push    edi

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
  mov  ax, ss
  push eax
  movzx eax, word  [ebp + 4 * 4]
  push eax
  mov  ax, ds
  push eax
  mov  ax, es
  push eax
  mov  ax, fs
  push eax
  mov  ax, gs
  push eax

;; UINT32  Eip;
  push    dword  [ebp + 3 * 4]

;; UINT32  Gdtr[2], Idtr[2];
  sub  esp, 8
  sidt  [esp]
  sub  esp, 8
  sgdt  [esp]

;; UINT32  Ldtr, Tr;
  xor  eax, eax
  str  ax
  push eax
  sldt eax
  push eax

;; UINT32  EFlags;
  push    dword  [ebp + 5 * 4]

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
  mov  eax, cr4
  or   eax, 208h
  mov  cr4, eax
  push eax
  mov  eax, cr3
  push eax
  mov  eax, cr2
  push eax
  xor  eax, eax
  push eax
  mov  eax, cr0
  push eax

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  mov     eax, dr7
  push    eax

;; clear Dr7 while executing debugger itself
  xor     eax, eax
  mov     dr7, eax
  mov     eax, dr6
  push    eax

;; insure all status bits in dr6 are clear...
  xor     eax, eax
  mov     dr6, eax
  mov     eax, dr3
  push    eax
  mov     eax, dr2
  push    eax
  mov     eax, dr1
  push    eax
  mov     eax, dr0
  push    eax

;; FX_SAVE_STATE_IA32 FxSaveState;
  sub esp, 512
  mov edi, esp
  db 0fh, 0aeh, 00000111y       ;fxsave [edi]

;; UINT32  ExceptionData;
  push    dword  [ebp + 2 * 4]

;; Prepare parameter and call
  mov     edx, esp
  push    edx
  mov     eax, dword  [ebp + 1 * 4]
  push    eax
  cmp     eax, 32
  jb      CallException
  MakeCall eax, ASM_PFX(TimerHandler)
  jmp     ExceptionDone
CallException:
  MakeCall eax, ASM_PFX(ExceptionHandler)
ExceptionDone:
  add     esp, 8

  cli
;; UINT32  ExceptionData;
  add esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
  mov esi, esp
  db 0fh, 0aeh, 00001110y     ; fxrstor [esi]
  add esp, 512

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  pop     eax
  mov     dr0, eax
  pop     eax
  mov     dr1, eax
  pop     eax
  mov     dr2, eax
  pop     eax
  mov     dr3, eax
;; skip restore of dr6.  We cleared dr6 during the context save.
  add     esp, 4
  pop     eax
  mov     dr7, eax

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
  pop     eax
  mov     cr0, eax
  add     esp, 4    ; not for Cr1
  pop     eax
  mov     cr2, eax
  pop     eax
  mov     cr3, eax
  pop     eax
  mov     cr4, eax

;; UINT32  EFlags;
  pop     dword  [ebp + 5 * 4]

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
  add     esp, 24

;; UINT32  Eip;
  pop     dword  [ebp + 3 * 4]

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
  pop     gs
  pop     fs
  pop     es
  pop     ds
  pop     dword  [ebp + 4 * 4]
  pop     ss

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
  pop     edi
  pop     esi
  add     esp, 4   ; not for ebp
  add     esp, 4   ; not for esp
  pop     ebx
  pop     edx
  pop     ecx
  pop     eax

  mov     esp, ebp
  pop     ebp
  add     esp, 8
  iretd


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SECTION .data

ALIGN 04h

gdtr    dw GDT_END - GDT_BASE - 1   ; GDT limit
        dd 0   ;GDT_BASE                        ; (GDT base gets set above)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   global descriptor table (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ALIGN 04h                      ; make GDT 4-byte align

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

; spare segment descriptor
SPARE4_SEL  equ $-GDT_BASE            ; Selector [0x30]
        dw 0
        dw 0
        db 0
        db 0
        db 0
        db 0

; spare segment descriptor
SPARE5_SEL  equ $-GDT_BASE            ; Selector [0x38]
        dw 0
        dw 0
        db 0
        db 0
        db 0
        db 0


GDT_END:
        ALIGN 04h

idtr    dw IDT_END - IDT_BASE - 1   ; IDT limit
        dd 0  ;IDT_BASE                        ; (IDT base gets set above)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   interrupt descriptor table (IDT)
;
;   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
;       mappings.  This implementation only uses the system timer and all other
;       IRQs will remain masked.  The descriptors for vectors 33+ are provided
;       for convenience.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         ALIGN 04h       ; make IDT 4-byte align

;global IDT_BASE
IDT_BASE:
; divide by zero (INT 0)
DIV_ZERO_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; debug exception (INT 1)
DEBUG_EXCEPT_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; NMI (INT 2)
NMI_SEL             equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; soft breakpoint (INT 3)
BREAKPOINT_SEL      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; overflow (INT 4)
OVERFLOW_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; bounds check (INT 5)
BOUNDS_CHECK_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; invalid opcode (INT 6)
INVALID_OPCODE_SEL  equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; device not available (INT 7)
DEV_NOT_AVAIL_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; double fault (INT 8)
DOUBLE_FAULT_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; Coprocessor segment overrun - reserved (INT 9)
RSVD_INTR_SEL1      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; invalid TSS (INT 0ah)
INVALID_TSS_SEL     equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; segment not present (INT 0bh)
SEG_NOT_PRESENT_SEL equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; stack fault (INT 0ch)
STACK_FAULT_SEL     equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; general protection (INT 0dh)
GP_FAULT_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; page fault (INT 0eh)
PAGE_FAULT_SEL      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; Intel reserved - do not use (INT 0fh)
RSVD_INTR_SEL2      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; floating point error (INT 10h)
FLT_POINT_ERR_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; alignment check (INT 11h)
ALIGNMENT_CHECK_SEL equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; machine check (INT 12h)
MACHINE_CHECK_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; SIMD floating-point exception (INT 13h)
SIMD_EXCEPTION_SEL  equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

%rep  (32 - 20)
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
%endrep

; 72 unspecified descriptors
;        db (72 * 8) dup(0)
TIMES (72 * 8) db 0
        
; IRQ 0 (System timer) - (INT 68h)
IRQ0_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 1 (8042 Keyboard controller) - (INT 69h)
IRQ1_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
IRQ2_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 3 (COM 2) - (INT 6bh)
IRQ3_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 4 (COM 1) - (INT 6ch)
IRQ4_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 5 (LPT 2) - (INT 6dh)
IRQ5_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 6 (Floppy controller) - (INT 6eh)
IRQ6_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 7 (LPT 1) - (INT 6fh)
IRQ7_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 8 (RTC Alarm) - (INT 70h)
IRQ8_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 9 - (INT 71h)
IRQ9_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 10 - (INT 72h)
IRQ10_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 11 - (INT 73h)
IRQ11_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 12 (PS/2 mouse) - (INT 74h)
IRQ12_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 13 (Floating point error) - (INT 75h)
IRQ13_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 14 (Secondary IDE) - (INT 76h)
IRQ14_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

; IRQ 15 (Primary IDE) - (INT 77h)
IRQ15_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw  SYS_CODE_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 8eh          ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16

;        db (1 * 8) dup(0)
TIMES (1 * 8) db 0

IDT_END:


;END

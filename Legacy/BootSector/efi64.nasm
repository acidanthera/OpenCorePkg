
DEFAULT_HANDLER_SIZE:   equ 9                   ; size of exception/interrupt stub
SYS_CODE_SEL64:         equ 0x38                ; 64-bit code selector in GDT
PE_OFFSET_IN_MZ_STUB:   equ 0x3c                ; place in MsDos stub for offset to PE Header
VGA_FB:                 equ 0x000b8000          ; VGA framebuffer for 80x25 mono mode
VGA_LINE:               equ 80 * 2              ; # bytes in one line
BASE_ADDR_64:           equ 0x00021000
STACK_TOP:              equ 0x001fffe8
SIZE:                   equ 0x1000

struc EFILDR_IMAGE
.CheckSum:      resd 1
.Offset:        resd 1
.Length:        resd 1
.FileName:      resb 52
endstruc

struc EFILDR_HEADER
.Signature:             resd 1
.HeaderCheckSum:        resd 1
.FileLength:            resd 1
.NumberOfImages:        resd 1
endstruc

struc PE_HEADER
                        resb 6
.NumberOfSections:      resw 1
                        resb 12
.SizeOfOptionalHeader:  resw 1
                        resb 2
.Magic:                 resb 16
.AddressOfEntryPoint:   resd 1
                        resb 4
.ImageBase64:           resd 1
endstruc

struc PE_SECTION_HEADER
                        resb 12
.VirtualAddress:        resd 1
.SizeOfRawData:         resd 1
.PointerToRawData:      resd 1
                        resb 16
endstruc

%macro StubWithNoCode 1
        push    0       ; push error code place holder on the stack
        push    %1
        jmp     strict qword commonIdtEntry
%endmacro

%macro StubWithACode 1
        times 2 nop
        push    %1
        jmp     strict qword commonIdtEntry
%endmacro

%macro PrintReg 2
        mov     esi, %1
        call    PrintString
        mov     rax, [rbp + %2]
        call    PrintQword
%endmacro

        bits 64
        org BASE_ADDR_64
global _start
_start:
        mov     esp, STACK_TOP
;
; set OSFXSR and OSXMMEXCPT because some code will use XMM registers
;
        mov     rax, cr4
        or      rax, 0x600
        mov     cr4, rax

;
; Populate IDT with meaningful offsets for exception handlers...
;
        sidt    [REL Idtr]

        mov     eax, StubTable
        mov     ebx, eax                ; use bx to copy 15..0 to descriptors
        shr     eax, 16                 ; use ax to copy 31..16 to descriptors
                                        ; 63..32 of descriptors is 0
        mov     ecx, 0x78               ; 78h IDT entries to initialize with unique entry points
        mov     edi, [REL Idtr + 2]

.StubLoop:      ; loop through all IDT entries exception handlers and initialize to default handler
        mov     [rdi], bx                       ; write bits 15..0 of offset
        mov     word [rdi + 2], SYS_CODE_SEL64  ; SYS_CODE_SEL64 from GDT
        mov     word [rdi + 4], 0x8e00          ; type = 386 interrupt gate, present
        mov     [rdi + 6], ax                   ; write bits 31..16 of offset
        mov     dword [rdi + 8], 0              ; write bits 63..32 of offset
        add     edi, 16                         ; move up to next descriptor
        add     ebx, DEFAULT_HANDLER_SIZE       ; move to next entry point
        loop    .StubLoop                       ; loop back through again until all descriptors are initialized

;
; Load EFILDR
;
        mov     esi, BlockSignature + 2
        add     esi, [rsi + EFILDR_HEADER_size + EFILDR_IMAGE.Offset]   ; esi = Base of EFILDR.C
        mov     ebp, [rsi + PE_OFFSET_IN_MZ_STUB]
        add     ebp, esi                                        ; ebp = PE Image Header for EFILDR.C
        mov     edi, [rbp + PE_HEADER.ImageBase64]              ; edi = ImageBase (63..32 is zero, ignore)
        mov     eax, [rbp + PE_HEADER.AddressOfEntryPoint]      ; eax = EntryPoint
        add     eax, edi                                        ; eax = ImageBase + EntryPoint
        mov     [REL .EfiLdrOffset], eax                        ; Modify jump instruction for correct entry point

        movzx   ebx, word [rbp + PE_HEADER.NumberOfSections]    ; bx = Number of sections
        movzx   eax, word [rbp + PE_HEADER.SizeOfOptionalHeader]; ax = Optional Header Size
        add     ebp, eax
        add     ebp, PE_HEADER.Magic                            ; ebp = Start of 1st Section

.SectionLoop:
        push    rsi                                             ; Save Base of EFILDR.C
        push    rdi                                             ; Save ImageBase
        add     esi, [rbp + PE_SECTION_HEADER.PointerToRawData] ; esi = Base of EFILDR.C + PointerToRawData
        add     edi, [rbp + PE_SECTION_HEADER.VirtualAddress]   ; edi = ImageBase + VirtualAddress
        mov     ecx, [rbp + PE_SECTION_HEADER.SizeOfRawData]    ; ecx = SizeOfRawData

        cld
        shr     ecx, 2
        rep movsd

        pop     rdi                             ; Restore ImageBase
        pop     rsi                             ; Restore Base of EFILDR.C

        add     ebp, PE_SECTION_HEADER_size     ; ebp = Pointer to next section record
        dec     ebx
        jnz     .SectionLoop

        movzx   ecx, word [REL Idtr]    ; get size of IDT
        inc     ecx
        add     ecx, [REL Idtr + 2]     ; add to base of IDT to get location of memory map...
                                        ; argument in RCX
.EfiLdrOffset:  equ $ + 1
        mov     eax, 0x00401000
        jmp     rax                     ; jump to entry point

StubTable:
%assign i 0
%rep 8
        StubWithNoCode  i
%assign i i+1
%endrep
        StubWithACode   8       ; Double Fault
        StubWithNoCode  9
        StubWithACode  10       ; Invalid TSS
        StubWithACode  11       ; Segment Not Present
        StubWithACode  12       ; Stack Fault
        StubWithACode  13       ; GP Fault
        StubWithACode  14       ; Page Fault
        StubWithNoCode 15
        StubWithNoCode 16
        StubWithACode  17       ; Alignment Check
%assign i 18
%rep 102
        StubWithNoCode i
%assign i i+1
%endrep

commonIdtEntry:
        push    rax
        push    rcx
        push    rdx
        push    rbx
        push    rsp
        push    rbp
        push    rsi
        push    rdi
        push    r8
        push    r9
        push    r10
        push    r11
        push    r12
        push    r13
        push    r14
        push    r15
        mov     rbp, rsp
;
;   At this point the stack looks like this:
;
;       Calling SS
;       Calling RSP
;       rflags
;       Calling CS
;       Calling RIP
;       Error code or 0
;       Int num or 0ffh for unknown int num
;       rax
;       rcx
;       rdx
;       rbx
;       rsp
;       rbp
;       rsi
;       rdi
;       r8
;       r9
;       r10
;       r11
;       r12
;       r13
;       r14
;       r15 <------- RSP, RBP
;

        call    ClearScreen
        mov     esi, String1
        call    PrintString
        mov     eax, [rbp + 16 * 8]             ; move Int number into EAX
        cmp     eax, 19
        ja      .PrintDefaultString
        mov     esi, [rax * 8 + StringTable]    ; get offset from StringTable to actual string address
        jmp     .PrintTheString
.PrintDefaultString:
        mov     esi, IntUnknownString
; patch Int number
        mov     edx, eax
        call    A2C
        mov     [rsi + 1], al
        mov     eax, edx
        shr     eax, 4
        call    A2C
        mov     [rsi], al
.PrintTheString:
        call    PrintString
        PrintReg String2, 19 * 8        ; CS
        mov     byte [rdi], ':'
        add     edi, 2
        mov     rax, [rbp + 18 * 8]     ; RIP
        call    PrintQword
        mov     esi, String3
        call    PrintString

        mov     edi, VGA_FB + 2 * VGA_LINE

        PrintReg StringRax, 15 * 8

        PrintReg StringRcx, 14 * 8

        PrintReg StringRdx, 13 * 8

        mov     edi, VGA_FB + 3 * VGA_LINE

        PrintReg StringRbx, 12 * 8

        PrintReg StringRsp, 21 * 8

        PrintReg StringRbp, 10 * 8

        mov     edi, VGA_FB + 4 * VGA_LINE

        PrintReg StringRsi, 9 * 8

        PrintReg StringRdi, 8 * 8

        PrintReg StringEcode, 17 * 8

        mov     edi, VGA_FB + 5 * VGA_LINE

        PrintReg StringR8, 7 * 8

        PrintReg StringR9, 6 * 8

        PrintReg StringR10, 5 * 8

        mov     edi, VGA_FB + 6 * VGA_LINE

        PrintReg StringR11, 4 * 8

        PrintReg StringR12, 3 * 8

        PrintReg StringR13, 2 * 8

        mov     edi, VGA_FB + 7 * VGA_LINE

        PrintReg StringR14, 8

        PrintReg StringR15, 0

        PrintReg StringSs, 22 * 8

        mov     edi, VGA_FB + 8 * VGA_LINE

        PrintReg StringRflags, 20 * 8

        mov     edi, VGA_FB + 10 * VGA_LINE

        mov     rsi, rbp
        add     rsi, 23 * 8     ; stack beyond SS
        mov     ecx, 4

.OuterLoop:
        push    rcx
        mov     ecx, 4
        mov     edx, edi

.InnerLoop:
        mov     rax, [rsi]
        call    PrintQword
        add     rsi, 8
        mov     byte [rdi], ' '
        add     edi, 2
        loop    .InnerLoop

        pop     rcx
        add     edx, VGA_LINE
        mov     edi, edx
        loop    .OuterLoop

        mov     edi, VGA_FB + 15 * VGA_LINE

        mov     rsi, [rbp + 18 * 8]     ; RIP
        sub     rsi, 8 * 8              ; code preceding RIP

        mov     ecx, 4

.OuterLoop1:
        push    rcx
        mov     ecx, 4
        mov     edx, edi

.InnerLoop1:
        mov     rax, [rsi]
        call    PrintQword
        add     rsi, 8
        mov     byte [rdi], ' '
        add     edi, 2
        loop    .InnerLoop1

        pop     rcx
        add     edx, VGA_LINE
        mov     edi, edx
        loop    .OuterLoop1

.Halt:
        hlt
        jmp     .Halt
;
; Return
;
        mov     rsp, rbp
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rdi
        pop     rsi
        pop     rbp
        pop     rax ; rsp
        pop     rbx
        pop     rdx
        pop     rcx
        pop     rax
        add     rsp, 16 ; error code and INT number
        iretq

PrintString:
        push    rax
.loop:
        mov     al, [rsi]
        test    al, al
        jz      .done
        mov     [rdi], al
        inc     esi
        add     edi, 2
        jmp     .loop
.done:
        pop     rax
        ret

; RAX contains qword to print
; RDI contains memory location (screen location) to print it to

PrintQword:
        push    rcx
        push    rbx
        push    rax
        mov     ecx, 16
.looptop:
        rol     rax, 4
        mov     bl, al
        and     bl, 15
        add     bl, '0'
        cmp     bl, '9'
        jbe     .is_digit
        add     bl, 7
.is_digit:
        mov     [rdi], bl
        add     edi, 2
        loop    .looptop
        pop     rax
        pop     rbx
        pop     rcx
        ret

ClearScreen:
        push    rax
        push    rcx
        mov     ax,  0x0c20
        mov     edi, VGA_FB
        mov     ecx, 80 * 25
        rep stosw
        mov     edi, VGA_FB
        pop     rcx
        pop     rax
        ret

A2C:
        and     al, 15
        add     al, '0'
        cmp     al, '9'
        jbe     .is_digit
        add     al, 7
.is_digit:
        ret

String1:        db '*** INT ', 0

Int0String:     db '00h Divide by 0 -', 0
Int1String:     db '01h Debug exception -', 0
Int2String:     db '02h NMI -', 0
Int3String:     db '03h Breakpoint -', 0
Int4String:     db '04h Overflow -', 0
Int5String:     db '05h Bound -', 0
Int6String:     db '06h Invalid opcode -', 0
Int7String:     db '07h Device not available -', 0
Int8String:     db '08h Double fault -', 0
Int9String:     db '09h Coprocessor seg overrun (reserved) -', 0
Int10String:    db '0Ah Invalid TSS -', 0
Int11String:    db '0Bh Segment not present -', 0
Int12String:    db '0Ch Stack fault -', 0
Int13String:    db '0Dh General protection fault -', 0
Int14String:    db '0Eh Page fault -', 0
Int15String:    db '0Fh (Intel reserved) -', 0
Int16String:    db '10h Floating point error -', 0
Int17String:    db '11h Alignment check -', 0
Int18String:    db '12h Machine check -', 0
Int19String:    db '13h SIMD Floating-Point Exception -', 0
IntUnknownString:       db '??h Unknown interrupt -', 0

        align 8, db 0

StringTable:
%assign i 0
%rep 20
        dq Int%[i]String
%assign i i+1
%endrep

String2:        db ' HALT!! *** (', 0
String3:        db ')', 0
StringRax:      db 'RAX=', 0
StringRcx:      db ' RCX=', 0
StringRdx:      db ' RDX=', 0
StringRbx:      db 'RBX=', 0
StringRsp:      db ' RSP=', 0
StringRbp:      db ' RBP=', 0
StringRsi:      db 'RSI=', 0
StringRdi:      db ' RDI=', 0
StringEcode:    db ' ECODE=', 0
StringR8:       db 'R8 =', 0
StringR9:       db ' R9 =', 0
StringR10:      db ' R10=', 0
StringR11:      db 'R11=', 0
StringR12:      db ' R12=', 0
StringR13:      db ' R13=', 0
StringR14:      db 'R14=', 0
StringR15:      db ' R15=', 0
StringSs:       db ' SS =', 0
StringRflags:   db 'RFLAGS=', 0

        align 2, db 0
Idtr:
        times SIZE - 2 - $ + $$ db 0
BlockSignature:
        dw 0xaa55


DEFAULT_HANDLER_SIZE:   equ 9                   ; size of exception/interrupt stub
SYS_CODE_SEL:           equ 0x20                ; 32-bit code selector in GDT
PE_OFFSET_IN_MZ_STUB:   equ 0x3c                ; place in MsDos stub for offset to PE Header
VGA_FB:                 equ 0x000b8000          ; VGA framebuffer for 80x25 mono mode
VGA_LINE:               equ 80 * 2              ; # bytes in one line
BASE_ADDR_32:           equ 0x00021000
STACK_TOP:              equ 0x001ffff0
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
                        resb 8
.ImageBase32:           resd 1
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
        jmp     strict dword commonIdtEntry
%endmacro

%macro StubWithACode 1
        times 2 nop
        push    %1
        jmp     strict dword commonIdtEntry
%endmacro

%macro PrintReg 2
        mov     esi, %1
        call    PrintString
        mov     eax, [ebp + %2]
        call    PrintDword
%endmacro

        bits 32
        org BASE_ADDR_32
global _start
_start:
        mov     ax, bx
        mov     ds, eax
        mov     es, eax
        mov     fs, eax
        mov     gs, eax
        mov     ss, eax
        mov     esp, STACK_TOP

;
; Populate IDT with meaningful offsets for exception handlers...
;
        sidt    [Idtr]

        mov     eax, StubTable
        mov     ebx, eax                ; use bx to copy 15..0 to descriptors
        shr     eax, 16                 ; use ax to copy 31..16 to descriptors
        mov     ecx, 0x78               ; 78h IDT entries to initialize with unique entry points
        mov     edi, [Idtr + 2]

.StubLoop:      ; loop through all IDT entries exception handlers and initialize to default handler
        mov     [edi], bx                       ; write bits 15..0 of offset
        mov     word [edi + 2], SYS_CODE_SEL    ; SYS_CODE_SEL from GDT
        mov     word [edi + 4], 0x8e00          ; type = 386 interrupt gate, present
        mov     [edi + 6], ax                   ; write bits 31..16 of offset
        add     edi, 8                          ; move up to next descriptor
        add     ebx, DEFAULT_HANDLER_SIZE       ; move to next entry point
        loop    .StubLoop                       ; loop back through again until all descriptors are initialized

;
; Load EFILDR
;
        mov     esi, BlockSignature + 2
        add     esi, [esi + EFILDR_HEADER_size + EFILDR_IMAGE.Offset]   ; esi = Base of EFILDR.C
        mov     ebp, [esi + PE_OFFSET_IN_MZ_STUB]
        add     ebp, esi                                        ; ebp = PE Image Header for EFILDR.C
        mov     edi, [ebp + PE_HEADER.ImageBase32]              ; edi = ImageBase
        mov     eax, [ebp + PE_HEADER.AddressOfEntryPoint]      ; eax = EntryPoint
        add     eax, edi                                        ; eax = ImageBase + EntryPoint
        mov     [.EfiLdrOffset], eax                            ; Modify jump instruction for correct entry point

        movzx   ebx, word [ebp + PE_HEADER.NumberOfSections]    ; bx = Number of sections
        movzx   eax, word [ebp + PE_HEADER.SizeOfOptionalHeader]; ax = Optional Header Size
        add     ebp, eax
        add     ebp, PE_HEADER.Magic                            ; ebp = Start of 1st Section

.SectionLoop:
        push    esi                                             ; Save Base of EFILDR.C
        push    edi                                             ; Save ImageBase
        add     esi, [ebp + PE_SECTION_HEADER.PointerToRawData] ; esi = Base of EFILDR.C + PointerToRawData
        add     edi, [ebp + PE_SECTION_HEADER.VirtualAddress]   ; edi = ImageBase + VirtualAddress
        mov     ecx, [ebp + PE_SECTION_HEADER.SizeOfRawData]    ; ecx = SizeOfRawData

        cld
        shr     ecx, 2
        rep movsd

        pop     edi                             ; Restore ImageBase
        pop     esi                             ; Restore Base of EFILDR.C

        add     ebp, PE_SECTION_HEADER_size     ; ebp = Pointer to next section record
        dec     ebx
        jnz     .SectionLoop

        movzx   eax, word [Idtr]        ; get size of IDT
        inc     eax
        add     eax, [Idtr + 2]         ; add to base of IDT to get location of memory map...
        push    eax                     ; push memory map location on stack for call to EFILDR...

        push    eax                     ; push return address (useless, just for stack balance)

.EfiLdrOffset:  equ $ + 1
        mov     eax, 0x00401000
        jmp     eax                     ; jump to entry point

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
        pushad
        mov     ebp, esp
;
;   At this point the stack looks like this:
;
;       eflags
;       Calling CS
;       Calling EIP
;       Error code or 0
;       Int num or 0ffh for unknown int num
;       eax
;       ecx
;       edx
;       ebx
;       esp
;       ebp
;       esi
;       edi <------- ESP, EBP
;

;       call    ClearScreen
        mov     edi, VGA_FB
        mov     esi, String1
        call    PrintString
        mov     eax, [ebp + 8 * 4]              ; move Int number into EAX
        cmp     eax, 19
        ja      .PrintDefaultString
        mov     esi, [eax * 4 + StringTable]    ; get offset from StringTable to actual string address
        jmp     .PrintTheString
.PrintDefaultString:
        mov     esi, IntUnknownString
; patch Int number
        mov     edx, eax
        call    A2C
        mov     [esi + 1], al
        mov     eax, edx
        shr     eax, 4
        call    A2C
        mov     [esi], al
.PrintTheString:
        call    PrintString
        PrintReg String2, 11 * 4        ; CS
        mov     byte [edi], ':'
        add     edi, 2
        mov     eax, [ebp + 10 * 4]     ; EIP
        call    PrintDword
        mov     esi, String3
        call    PrintString

        mov     edi, VGA_FB + 2 * VGA_LINE

        PrintReg StringEax, 7 * 4

        PrintReg StringEbx, 4 * 4

        PrintReg StringEcx, 6 * 4

        PrintReg StringEdx, 5 * 4

        PrintReg StringEcode, 9 * 4

        mov     edi, VGA_FB + 3 * VGA_LINE

        PrintReg StringEsp, 3 * 4

        PrintReg StringEbp, 2 * 4

        PrintReg StringEsi, 4

        PrintReg StringEdi, 0

        PrintReg StringEflags, 12 * 4

        mov     edi, VGA_FB + 5 * VGA_LINE

        mov     esi, ebp
        add     esi, 13 * 4     ; stack beyond eflags
        mov     ecx, 8

.OuterLoop:
        push    ecx
        mov     ecx, 8
        mov     edx, edi

.InnerLoop:
        mov     eax, [esi]
        call    PrintDword
        add     esi, 4
        mov     byte [edi], ' '
        add     edi, 2
        loop    .InnerLoop

        pop     ecx
        add     edx, VGA_LINE
        mov     edi, edx
        loop    .OuterLoop

        mov     edi, VGA_FB + 15 * VGA_LINE

        mov     esi, [ebp + 10 * 4]     ; EIP
        sub     esi, 32 * 4             ; code preceding EIP

        mov     ecx, 8

.OuterLoop1:
        push    ecx
        mov     ecx, 8
        mov     edx, edi

.InnerLoop1:
        mov     eax, [esi]
        call    PrintDword
        add     esi, 4
        mov     byte [edi], ' '
        add     edi, 2
        loop    .InnerLoop1

        pop     ecx
        add     edx, VGA_LINE
        mov     edi, edx
        loop    .OuterLoop1

.Halt:
        hlt
        jmp     .Halt
;
; Return
;
        mov     esp, ebp
        popad
        add     esp, 8 ; error code and INT number
        iretd

PrintString:
        push    eax
.loop:
        mov     al, [esi]
        test    al, al
        jz      .done
        mov     [edi], al
        inc     esi
        add     edi, 2
        jmp     .loop
.done:
        pop     eax
        ret

; EAX contains dword to print
; EDI contains memory location (screen location) to print it to

PrintDword:
        push    ecx
        push    ebx
        push    eax
        mov     ecx, 8
.looptop:
        rol     eax, 4
        mov     bl, al
        and     bl, 15
        add     bl, '0'
        cmp     bl, '9'
        jbe     .is_digit
        add     bl, 7
.is_digit:
        mov     [edi], bl
        add     edi, 2
        loop    .looptop
        pop     eax
        pop     ebx
        pop     ecx
        ret

ClearScreen:
        push    eax
        push    ecx
        mov     ax,  0x0c20
        mov     edi, VGA_FB
        mov     ecx, 80 * 25
        rep stosw
        mov     edi, VGA_FB
        pop     ecx
        pop     eax
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

        align 4, db 0

StringTable:
%assign i 0
%rep 20
        dd Int%[i]String
%assign i i+1
%endrep

String2:        db ' HALT!! *** (', 0
String3:        db ')', 0
StringEax:      db 'EAX=', 0
StringEbx:      db ' EBX=', 0
StringEcx:      db ' ECX=', 0
StringEdx:      db ' EDX=', 0
StringEcode:    db ' ECODE=', 0
StringEsp:      db 'ESP=', 0
StringEbp:      db ' EBP=', 0
StringEsi:      db ' ESI=', 0
StringEdi:      db ' EDI=', 0
StringEflags:   db ' EFLAGS=', 0

        align 2, db 0
Idtr:
        times SIZE - 2 - $ + $$ db 0
BlockSignature:
        dw 0xaa55

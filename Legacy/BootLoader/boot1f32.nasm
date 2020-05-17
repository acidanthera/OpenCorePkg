; Copyright (c) 1999-2003 Apple Computer, Inc. All rights reserved.
;
; @APPLE_LICENSE_HEADER_START@
;
; Portions Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights
; Reserved.  This file contains Original Code and/or Modifications of
; Original Code as defined in and that are subject to the Apple Public
; Source License Version 2.0 (the "License").  You may not use this file
; except in compliance with the License.  Please obtain a copy of the
; License at http://www.apple.com/publicsource and read it before using
; this file.
;
; The Original Code and all software distributed under the License are
; distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
; EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
; INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
; License for the specific language governing rights and limitations
; under the License.
;
; @APPLE_LICENSE_HEADER_END@
;
; Partition Boot Loader: boot1f32
;
; This program is designed to reside in sector 0 of a FAT32 partition.
; It expects that the MBR has left the drive number in DL
; and a pointer to the partition entry in SI.
;
; This version requires a BIOS with EBIOS (LBA) support.
;
; This code is written for the NASM assembler.
;   nasm boot1f32.s -o boot1f32
;
;   dd if=origbs of=newbs skip=3 seek=3 bs=1 count=87 conv=notrunc
;

;
; This version of boot1f32 tries to find a stage2 boot file in the root folder.
;
; Written by mackerintel on 2009-01-26
;

;
; Set to 1 to enable obscure debug messages.
;
DEBUG               EQU     0

;
; Set to 1 to enable verbose mode.
;
VERBOSE             EQU     0

;
; Various constants.
;
NULL                EQU     0
CR                  EQU     0x0D
LF                  EQU     0x0A

maxSectorCount      EQU     64                                  ; maximum sector count for readSectors
kSectorBytes        EQU     512                                 ; sector size in bytes
kBootSignature      EQU     0xAA55                              ; boot sector signature

kBoot1StackAddress  EQU     0xFFF0                              ; boot1 stack pointer
kBoot1LoadAddr      EQU     0xE000                              ; boot1 load address

kBoot2Sectors       EQU     (480 * 1024 - 512) / kSectorBytes   ; max size of 'boot' file in sectors
kBoot2Segment       EQU     0x2000                              ; boot2 load segment
kBoot2Address       EQU     kSectorBytes                        ; boot2 load address

FATBUF              EQU     0x7000                              ; Just place for one sectors
DIRBUFSEG           EQU     0x1000                              ; Cluster sizes >64KB aren't supported

;
; Format of fdisk partition entry.
;
; The symbol 'part_size' is automatically defined as an `EQU'
; giving the size of the structure.
;
            struc part
.bootid     resb 1      ; bootable or not
.head       resb 1      ; starting head, sector, cylinder
.sect       resb 1      ;
.cyl        resb 1      ;
.type       resb 1      ; partition type
.endhead    resb 1      ; ending head, sector, cylinder
.endsect    resb 1      ;
.endcyl     resb 1      ;
.lba        resd 1      ; starting lba
.sectors    resd 1      ; size in sectors
            endstruc

                struc direntry
.nameext    resb 11
.attr       resb 1
.nused1     resb 8
.highclus   resw 1
.nused2     resb 4
.lowclus    resw 1
.size       resd 1
            endstruc


;
; Macros.
;
%macro jmpabs 1
    push    WORD %1
    ret
%endmacro

%macro DebugCharMacro 1
    pushad
    mov     al, %1
    call    print_char
    call    getc
    popad
%endmacro

%macro PrintCharMacro 1
    pushad
    mov     al, %1
    call    print_char
    popad
%endmacro

%macro PutCharMacro 1
    call    print_char
%endmacro

%macro PrintHexMacro 1
    call    print_hex
%endmacro

%macro PrintString 1
    mov     si, %1
    call    print_string
%endmacro

%macro LogString 1
    mov     di, %1
    call    log_string
%endmacro

%if DEBUG
  %define DebugChar(x) DebugCharMacro x
  %define PrintChar(x) PrintCharMacro x
  %define PutChar(x) PutCharMacro
  %define PrintHex(x) PrintHexMacro x
%else
  %define DebugChar(x)
  %define PrintChar(x)
  %define PutChar(x)
  %define PrintHex(x)
%endif

;--------------------------------------------------------------------------
; Start of text segment.

    SEGMENT .text

    ORG     kBoot1LoadAddr

    jmp     start
    times   3-($-$$) nop

gOEMName            times   8   db  0 ;OEMNAME
gBPS                dw      0
gSPC                db      0
gReservedSectors    dw      0
gNumFats            db      0
gCrap1              times   11  db  0
gPartLBA            dd      0
gPartSize           dd      0
gSectPerFat         dd      0
gCrap2              times   4   db  0
gRootCluster        dd      0
gCrap3              times   16  db  0

gBIOSDriveNumber    db      0
gExtInfo            times   25  db  0
gFileName           db      "BOOT       " ; Used as a magic string in boot0

;--------------------------------------------------------------------------
; Boot code is loaded at 0:E000h.
;
start:
    ;
    ; set up the stack to grow down from kBoot1StackSegment:kBoot1StackAddress.
    ; Interrupts should be off while the stack is being manipulated.
    ;
    cli                             ; interrupts off
    xor     eax, eax                ; zero ax
    mov     ss, ax                  ; ss <- 0
    mov     sp, kBoot1StackAddress  ; sp <- top of stack
    sti                             ; reenable interrupts

    mov     ds, ax                  ; ds <- 0
    mov     es, ax                  ; es <- 0

    ;
    ; Initializing global variables.
    ;
    mov     ax, word [gReservedSectors]
    add     eax, [si + part.lba]
    mov     [gPartLBA], eax                 ; save the current FAT LBA offset
    mov     [gBIOSDriveNumber], dl          ; save BIOS drive number
    xor     eax,eax
    mov     al, [gNumFats]
    mul     dword [gSectPerFat]
    mov     [gSectPerFat], eax

;--------------------------------------------------------------------------
; Find stage2 boot file in a FAT32 Volume's root folder.
;
findRootBoot:

%if VERBOSE
    LogString(init_str)
%endif

    mov     eax, [gRootCluster]

nextdirclus:
    mov     edx, DIRBUFSEG<<4
    call    readCluster
    jc      error
    xor     si, si
    mov     bl, [gSPC]
    shl     bx, 9
    add     bx, si

nextdirent:
    mov     di, gFileName
    push    ds
    push    DIRBUFSEG
    pop     ds
    mov     cl, [si]
    test    cl, cl
    jz      dserror
    mov     cx, 11
    repe    cmpsb
    jz      direntfound

falsealert:
    pop     ds
    add     cl, 21
    add     si, cx
    cmp     si, bx
    jz      nextdirclus
    jmp     nextdirent

direntfound:
    lodsb
    test    al, 0x18
    jnz     falsealert
    push    WORD [si + direntry.highclus - 12]
    push    WORD [si + direntry.lowclus - 12]
    pop     eax
    pop     ds
    mov     edx, (kBoot2Segment << 4) + kBoot2Address

cont_read:
    push    edx
    call    readCluster
    pop     edx
    pushf
    xor     ebx,ebx
    mov     bl, [gSPC]
    shl     ebx, 9
    add     edx, ebx
    popf
    jnc     cont_read

boot2:

%if DEBUG
    DebugChar ('!')
%endif

    mov     dl, [gBIOSDriveNumber]          ; load BIOS drive number
    jmp     kBoot2Segment:kBoot2Address

dserror:
    pop ds

error:

%if VERBOSE
    LogString(error_str)
%endif

hang:
    hlt
    jmp     hang

    ; readCluster - Reads cluster EAX to (EDX), updates EAX to next cluster
readCluster:
    cmp     eax, 0x0ffffff8
    jb      do_read
    stc
    ret

do_read:
    push    eax
    xor     ecx,ecx
    dec     eax
    dec     eax
    mov     cl, [gSPC]
    push    edx
    mul     ecx
    pop     edx
    add     eax, [gSectPerFat]
    mov     ecx, eax
    xor     ah,ah
    mov     al, [gSPC]
    call    readSectors
    jc      clusend
    pop     ecx
    push    cx
    shr     ecx, 7
    xor     ax, ax
    inc     ax
    mov     edx, FATBUF
    call    readSectors
    jc      clusend
    pop     si
    and     si, 0x7f
    shl     si, 2
    mov     eax, [FATBUF + si]
    and     eax, 0x0fffffff
    clc
    ret

clusend:
    pop     eax
    ret

;--------------------------------------------------------------------------
; readSectors - Reads more than 127 sectors using LBA addressing.
;
; Arguments:
;   AX = number of 512-byte sectors to read (valid from 1-1280).
;   EDX = pointer to where the sectors should be stored.
;   ECX = sector offset in partition
;
; Returns:
;   CF = 0  success
;        1 error
;
readSectors:
    pushad
    mov     bx, ax

.loop:
    xor     eax, eax                        ; EAX = 0
    mov     al, bl                          ; assume we reached the last block.
    cmp     bx, maxSectorCount              ; check if we really reached the last block
    jb      .readBlock                      ; yes, BX < MaxSectorCount
    mov     al, maxSectorCount              ; no, read MaxSectorCount

.readBlock:
    call    readLBA
    sub     bx, ax                          ; decrease remaning sectors with the read amount
    jz      .exit                           ; exit if no more sectors left to be loaded
    add     ecx, eax                        ; adjust LBA sector offset
    shl     ax, 9                           ; convert sectors to bytes
    add     edx, eax                        ; adjust target memory location
    jmp     .loop                           ; read remaining sectors

.exit:
    popad
    ret

;--------------------------------------------------------------------------
; readLBA - Read sectors from a partition using LBA addressing.
;
; Arguments:
;   AL = number of 512-byte sectors to read (valid from 1-127).
;   EDX = pointer to where the sectors should be stored.
;   ECX = sector offset in partition
;   [bios_drive_number] = drive number (0x80 + unit number)
;
; Returns:
;   CF = 0  success
;        1 error
;
readLBA:
    pushad                                  ; save all registers
    push    es                              ; save ES
    mov     bp, sp                          ; save current SP

    ;
    ; Convert EDX to segment:offset model and set ES:BX
    ;
    ; Some BIOSes do not like offset to be negative while reading
    ; from hard drives. This usually leads to "boot1: error" when trying
    ; to boot from hard drive, while booting normally from USB flash.
    ; The routines, responsible for this are apparently different.
    ; Thus we split linear address slightly differently for these
    ; capricious BIOSes to make sure offset is always positive.
    ;

    mov     bx, dx                          ; save offset to BX
    and     bh, 0x0f                        ; keep low 12 bits
    shr     edx, 4                          ; adjust linear address to segment base
    xor     dl, dl                          ; mask low 8 bits
    mov     es, dx                          ; save segment to ES

    ;
    ; Create the Disk Address Packet structure for the
    ; INT13/F42 (Extended Read Sectors) on the stack.
    ;

    ; push    DWORD 0                       ; offset 12, upper 32-bit LBA
    push    ds                              ; For sake of saving memory,
    push    ds                              ; push DS register, which is 0.

    add     ecx, [gPartLBA]                 ; offset 8, lower 32-bit LBA
    push    ecx

    push    es                              ; offset 6, memory segment

    push    bx                              ; offset 4, memory offset

    xor     ah, ah                          ; offset 3, must be 0
    push    ax                              ; offset 2, number of sectors

    push    WORD 16                         ; offset 0-1, packet size

    ;
    ; INT13 Func 42 - Extended Read Sectors
    ;
    ; Arguments:
    ;   AH    = 0x42
    ;   [bios_drive_number] = drive number (0x80 + unit number)
    ;   DS:SI = pointer to Disk Address Packet
    ;
    ; Returns:
    ;   AH    = return status (sucess is 0)
    ;   carry = 0 success
    ;           1 error
    ;
    ; Packet offset 2 indicates the number of sectors read
    ; successfully.
    ;
    mov     dl, [gBIOSDriveNumber]          ; load BIOS drive number
    mov     si, sp
    mov     ah, 0x42
    int     0x13

    jc      error

    ;
    ; Issue a disk reset on error.
    ; Should this be changed to Func 0xD to skip the diskette controller
    ; reset?
    ;
;   xor     ax, ax                          ; Func 0
;   int     0x13                            ; INT 13
;   stc                                     ; set carry to indicate error

.exit:
    mov     sp, bp                          ; restore SP
    pop     es                              ; restore ES
    popad
    ret

%if VERBOSE

;--------------------------------------------------------------------------
; Write a string with 'boot1: ' prefix to the console.
;
; Arguments:
;   ES:DI   pointer to a NULL terminated string.
;
; Clobber list:
;   DI
;
log_string:
    pushad

    push    di
    mov     si, log_title_str
    call    print_string

    pop     si
    call    print_string

    popad

    ret

;-------------------------------------------------------------------------
; Write a string to the console.
;
; Arguments:
;   DS:SI   pointer to a NULL terminated string.
;
; Clobber list:
;   AX, BX, SI
;
print_string:
    mov     bx, 1                           ; BH=0, BL=1 (blue)

.loop:
    lodsb                                   ; load a byte from DS:SI into AL
    cmp     al, 0                           ; Is it a NULL?
    je      .exit                           ; yes, all done
    mov     ah, 0xE                         ; INT10 Func 0xE
    int     0x10                            ; display byte in tty mode
    jmp     .loop

.exit:
    ret

%endif ; VERBOSE

%if DEBUG

;--------------------------------------------------------------------------
; Write the 4-byte value to the console in hex.
;
; Arguments:
;   EAX = Value to be displayed in hex.
;
print_hex:
    pushad
    mov     cx, WORD 4
    bswap   eax
.loop:
    push    ax
    ror     al, 4
    call    print_nibble                    ; display upper nibble
    pop     ax
    call    print_nibble                    ; display lower nibble
    ror     eax, 8
    loop    .loop

    popad
    ret

print_nibble:
    and     al, 0x0f
    add     al, '0'
    cmp     al, '9'
    jna     .print_ascii
    add     al, 'A' - '9' - 1
.print_ascii:
    call    print_char
    ret

;--------------------------------------------------------------------------
; getc - wait for a key press
;
getc:
    pushad
    mov     ah, 0
    int     0x16
    popad
    ret

;--------------------------------------------------------------------------
; Write a ASCII character to the console.
;
; Arguments:
;   AL = ASCII character.
;
print_char:
    pushad
    mov     bx, 1                           ; BH=0, BL=1 (blue)
    mov     ah, 0x0e                        ; bios INT 10, Function 0xE
    int     0x10                            ; display byte in tty mode
    popad
    ret

%endif ; DEBUG

;--------------------------------------------------------------------------
; Static data.
;

%if VERBOSE
log_title_str   db      CR, LF, 'b1f: ', NULL
init_str        db      'init', NULL
error_str       db      'error', NULL
%endif

;--------------------------------------------------------------------------
; Pad the rest of the 512 byte sized sector with zeroes. The last
; two bytes is the mandatory boot sector signature.
;
; If the booter code becomes too large, then nasm will complain
; that the 'times' argument is negative.

pad_table_and_sig:
    times           496-($-$$) db 0
    ; We will put volume magic identifier here
    times           14 db 0
    dw              kBootSignature

    ABSOLUTE        kBoot1LoadAddr + kSectorBytes

; END

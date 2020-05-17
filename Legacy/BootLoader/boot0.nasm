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
; Boot Loader: boot0
;
; A small boot sector program written in x86 assembly whose only
; responsibility is to locate the active partition, load the
; partition booter into memory, and jump to the booter's entry point.
; It leaves the boot drive in DL and a pointer to the partition entry in SI.
; This version of boot0 implements hybrid GUID/MBR partition scheme support.
;
; This boot loader must be placed in the Master Boot Record.
;
; In order to coexist with a fdisk partition table (64 bytes), and
; leave room for a two byte signature (0xAA55) in the end, boot0 is
; restricted to 446 bytes (512 - 64 - 2). If boot0 did not have to
; live in the MBR, then we would have 510 bytes to work with.
;
; boot0 is always loaded by the BIOS or another booter to 0:7C00h.
;
; This code is written for the NASM assembler.
;   nasm boot0.s -o boot0
;
; Written by Tamás Kosárszky on 2008-03-10 and JrCs on 2013-05-08.
; With additions by Turbo for EFI System Partition boot support.
;

;
; Set to 1 to enable obscure debug messages.
;
DEBUG               EQU  0

;
; Set to 1 to enable verbose mode
;
VERBOSE             EQU  0

;
; Various constants.
;
kBoot0Segment       EQU  0x0000
kBoot0Stack         EQU  0xFFF0         ; boot0 stack pointer
kBoot0LoadAddr      EQU  0x7C00         ; boot0 load address
kBoot1LoadAddr      EQU  0xE000         ; boot1 load address

kMBRBuffer          EQU  0x1000         ; MBR buffer address
kLBA1Buffer         EQU  0x1200         ; LBA1 - GPT Partition Table Header buffer address
kGPTABuffer         EQU  0x1400         ; GUID Partition Entry Array buffer address

kPartTableOffset    EQU  0x1be
kMBRPartTable       EQU  kMBRBuffer + kPartTableOffset

kSectorBytes        EQU  512            ; sector size in bytes
kBootSignature      EQU  0xAA55         ; boot sector signature
kFAT32BootCodeOffset EQU  0x5a          ; offset of boot code in FAT32 boot sector
kBoot1FAT32Magic    EQU  'BO'           ; Magic string to detect our boot1f32 code


kGPTSignatureLow    EQU  'EFI '         ; GUID Partition Table Header Signature
kGPTSignatureHigh   EQU  'PART'
kGUIDLastDwordOffs  EQU  12             ; last 4 byte offset of a GUID

kPartCount          EQU  4              ; number of paritions per table
kPartTypeFAT32      EQU  0x0c           ; FAT32 Filesystem type
kPartTypePMBR       EQU  0xee           ; On all GUID Partition Table disks a Protective MBR (PMBR)
                                        ; in LBA 0 (that is, the first block) precedes the
                                        ; GUID Partition Table Header to maintain compatibility
                                        ; with existing tools that do not understand GPT partition structures.
                                        ; The Protective MBR has the same format as a legacy MBR
                                        ; and contains one partition entry with an OSType set to 0xEE
                                        ; reserving the entire space used on the disk by the GPT partitions,
                                        ; including all headers.

kPartActive         EQU  0x80           ; active flag enabled
kPartInactive       EQU  0x00           ; active flag disabled
kEFISystemGUID      EQU  0x3BC93EC9     ; last 4 bytes of EFI System Partition Type GUID:
                                        ; C12A7328-F81F-11D2-BA4B-00A0C93EC93B
kBasicDataGUID      EQU  0xC79926B7     ; last 4 bytes of Basic Data System Partition Type GUID:
                                        ; EBD0A0A2-B9E5-4433-87C0-68B6B72699C7

;
; Format of fdisk partition entry.
;
; The symbol 'part_size' is automatically defined as an `EQU'
; giving the size of the structure.
;
           struc part
.bootid    resb 1      ; bootable or not
.head      resb 1      ; starting head, sector, cylinder
.sect      resb 1      ;
.cyl       resb 1      ;
.type      resb 1      ; partition type
.endhead   resb 1      ; ending head, sector, cylinder
.endsect   resb 1      ;
.endcyl    resb 1      ;
.lba       resd 1      ; starting lba
.sectors   resd 1      ; size in sectors
           endstruc

;
; Format of GPT Partition Table Header
;
                            struc   gpth
.Signature                  resb    8
.Revision                   resb    4
.HeaderSize                 resb    4
.HeaderCRC32                resb    4
.Reserved                   resb    4
.MyLBA                      resb    8
.AlternateLBA               resb    8
.FirstUsableLBA             resb    8
.LastUsableLBA              resb    8
.DiskGUID                   resb    16
.PartitionEntryLBA          resb    8
.NumberOfPartitionEntries   resb    4
.SizeOfPartitionEntry       resb    4
.PartitionEntryArrayCRC32   resb    4
                            endstruc

;
; Format of GUID Partition Entry Array
;
                            struc   gpta
.PartitionTypeGUID          resb    16
.UniquePartitionGUID        resb    16
.StartingLBA                resb    8
.EndingLBA                  resb    8
.Attributes                 resb    8
.PartitionName              resb    72
                            endstruc

;
; Macros.
;
%macro DebugCharMacro 1
    mov   al, %1
    call  print_char
%endmacro

%macro LogString 1
    mov   di, %1
    call  log_string
%endmacro

%if DEBUG
%define DebugChar(x)  DebugCharMacro x
%else
%define DebugChar(x)
%endif

;--------------------------------------------------------------------------
; Start of text segment.

    SEGMENT .text

    ORG     kBoot0LoadAddr

;--------------------------------------------------------------------------
; Boot code is loaded at 0:7C00h.
;
start:
    ;
    ; Set up the stack to grow down from kBoot0Segment:kBoot0Stack.
    ; Interrupts should be off while the stack is being manipulated.
    ;
    cli                             ; interrupts off
    xor     ax, ax                  ; zero ax
    mov     ss, ax                  ; ss <- 0
    mov     sp, kBoot0Stack         ; sp <- top of stack
    sti                             ; reenable interrupts

    mov     es, ax                  ; es <- 0
    mov     ds, ax                  ; ds <- 0

    DebugChar('>')

%if DEBUG
    mov     al, dl
    call    print_hex
%endif

    ;
    ; Since this code may not always reside in the MBR, always start by
    ; loading the MBR to kMBRBuffer and LBA1 to kGPTBuffer.
    ;

    xor     eax, eax
    mov     [my_lba], eax           ; store LBA sector 0 for read_lba function
    mov     al, 2                   ; load two sectors: MBR and LBA1
    mov     bx, kMBRBuffer          ; MBR load address
    call    load
    jc      error                   ; MBR load error

    ;
    ; Look for the booter partition in the MBR partition table,
    ; which is at offset kMBRPartTable.
    ;
    mov     si, kMBRPartTable       ; pointer to partition table
    call    find_boot               ; will not return on success

error:
    LogString(boot_error_str)

hang:
    hlt
    jmp     hang


;--------------------------------------------------------------------------
; Find the active (boot) partition and load the booter from the partition.
;
; Arguments:
;   DL = drive number (0x80 + unit number)
;   SI = pointer to fdisk partition table.
;
; Clobber list:
;   EAX, BX, EBP
;
find_boot:

    ;
    ; Check for boot block signature 0xAA55 following the 4 partition
    ; entries.
    ;
    cmp     WORD [si + part_size * kPartCount], kBootSignature
    jne     .exit                       ; boot signature not found.

    xor     bx, bx                      ; BL will be set to 1 later in case of
                                        ; Protective MBR has been found

.start_scan:
    mov     cx, kPartCount              ; number of partition entries per table

.loop:

    ;
    ; First scan through the partition table looking for the active
    ; partition.
    ;
%if DEBUG
    mov     al, [si + part.type]       ; print partition type
    call    print_hex
%endif

    mov     eax, [si + part.lba]                    ; save starting LBA of current
    mov     [my_lba], eax                           ; MBR partition entry for read_lba function
    cmp     BYTE [si + part.type], 0                ; unused partition?
    je      .continue                               ; skip to next entry
    cmp     BYTE [si + part.type], kPartTypePMBR    ; check for Protective MBR
    jne     .tryToBootIfActive

    mov     BYTE [si + part.bootid], kPartInactive  ; found Protective MBR
                                                    ; clear active flag to make sure this protective
                                                    ; partition won't be used as a bootable partition.
    mov     bl, 1                                   ; Assume we can deal with GPT but try to scan
                                                    ; later if not found any other bootable partitions.

.tryToBootIfActive:
    ; We're going to try to boot a partition if it is active
    cmp     BYTE [si + part.bootid], kPartActive
    jne     .continue

    ;
    ; Found boot partition, read boot sector to memory.
    ;

    xor     dh, dh      ; Argument for loadBootSector to skip file system signature check.
    call    loadBootSector
    jne     .continue
    jmp     SHORT initBootLoader

.continue:
    add     si, BYTE part_size              ; advance SI to next partition entry
    loop    .loop                           ; loop through all partition entries

    ;
    ; Scanned all partitions but not found any with active flag enabled
    ; Anyway if we found a protective MBR before we still have a chance
    ; for a possible GPT Header at LBA 1
    ;
    dec     bl
    jnz     .exit                           ; didn't find Protective MBR before
    call    checkGPT

.exit:
    ret                                     ; Giving up.


    ;
    ; Jump to partition booter. The drive number is already in register DL.
    ; SI is pointing to the modified partition entry.
    ;
initBootLoader:

DebugChar('J')

%if VERBOSE
    LogString(done_str)
%endif

    jmp     kBoot1LoadAddr

    ;
    ; Found Protective MBR Partition Type: 0xEE
    ; Check for 'EFI PART' string at the beginning
    ; of LBA1 for possible GPT Table Header
    ;
checkGPT:
    push    bx

    mov     di, kLBA1Buffer                     ; address of GUID Partition Table Header
    cmp     DWORD [di], kGPTSignatureLow        ; looking for 'EFI '
    jne     .exit                               ; not found. Giving up.
    cmp     DWORD [di + 4], kGPTSignatureHigh   ; looking for 'PART'
    jne     .exit                               ; not found. Giving up indeed.
    mov     si, di

    ;
    ; Loading GUID Partition Table Array
    ;
    mov     eax, [si + gpth.PartitionEntryLBA]          ; starting LBA of GPT Array
    mov     [my_lba], eax                               ; save starting LBA for read_lba function
    mov     cx, [si + gpth.NumberOfPartitionEntries]    ; number of GUID Partition Array entries
    mov     bx, [si + gpth.SizeOfPartitionEntry]        ; size of GUID Partition Array entry

    push    bx                                          ; push size of GUID Partition entry

    ;
    ; Current GPT Arrays uses 128 partition entries each 128 bytes long
    ; 128 entries * 128 bytes long GPT Array entries / 512 bytes per sector = 32 sectors
    ;
    mov     al, 32                  ; maximum sector size of GPT Array (hardcoded method)

    mov     bx, kGPTABuffer
    push    bx                      ; push address of GPT Array
    call    load                    ; read GPT Array
    pop     si                      ; SI = address of GPT Array
    pop     bx                      ; BX = size of GUID Partition Array entry
    jc      error

    ;
    ; Walk through GUID Partition Table Array
    ; and load boot record from first supported partition.
    ;
    ; If it has boot signature (0xAA55) then jump to it
    ; otherwise skip to next partition.
    ;

%if VERBOSE
    LogString(gpt_str)
%endif

.gpt_loop:

    mov     eax, [si + gpta.PartitionTypeGUID + kGUIDLastDwordOffs]

    ;
    ; Try EFI System Partition
    ;
    cmp     eax, kEFISystemGUID     ; check current GUID Partition for EFI System Partition GUID type
    je      .gpt_ok

    ;
    ; Also try FAT2 System Partition
    ;
    cmp     eax, kBasicDataGUID     ; check current GUID Partition for Basic Data Partition GUID type
    jne     .gpt_continue

.gpt_ok:
    ;
    ; Found a possible good partition try to boot it
    ;

    mov     eax, [si + gpta.StartingLBA]            ; load boot sector from StartingLBA
    mov     [my_lba], eax
    mov     dh, 1                                   ; Argument for loadBootSector to check file system signature.
    call    loadBootSector
    jne     .gpt_continue                           ; no boot loader signature

    mov     si, kMBRPartTable                       ; fake the current GUID Partition
    mov     [si + part.lba], eax                    ; as MBR style partition for boot1f32,
    mov     BYTE [si + part.type], kPartTypeFAT32   ; set filesystem type for cosmetic reasons
    jmp     SHORT initBootLoader

.gpt_continue:

    add     si, bx                                  ; advance SI to next partition entry
    loop    .gpt_loop                               ; loop through all partition entries

.exit:
    pop     bx
    ret                                             ; no more GUID partitions. Giving up.


;--------------------------------------------------------------------------
; loadBootSector - Load boot sector
;
; Arguments:
;   DL = drive number (0x80 + unit number)
;   DH = 0 skip file system signature checking
;        1 enable file system signature checking
;   [my_lba] = starting LBA.
;
; Returns:
;   ZF = 0 if boot sector hasn't kBootSignature
;        1 if boot sector has kBootSignature
;
loadBootSector:
    pusha

    mov     al, 3
    mov     bx, kBoot1LoadAddr
    call    load
    jc      error

    or      dh, dh
    jz      .checkBootSignature

%if VERBOSE
    LogString(test_str)
%endif

    ;
    ; Looking for boot1f32 magic string.
    ;
    mov     ax, [kBoot1LoadAddr + kFAT32BootCodeOffset]
    cmp     ax, kBoot1FAT32Magic
    jne     .exit

.checkBootSignature:
    ;
    ; Check for boot block signature 0xAA55
    ;
    cmp     WORD [kBoot1LoadAddr + kSectorBytes - 2], kBootSignature

.exit:

    popa

    ret


;--------------------------------------------------------------------------
; load - Load one or more sectors from a partition.
;
; Arguments:
;   AL = number of 512-byte sectors to read.
;   ES:BX = pointer to where the sectors should be stored.
;   DL = drive number (0x80 + unit number)
;   [my_lba] = starting LBA.
;
; Returns:
;   CF = 0 success
;        1 error
;
load:
    push    cx

.ebios:
    mov     cx, 5                   ; load retry count
.ebios_loop:
    call    read_lba                ; use INT13/F42
    jnc     .exit
    loop    .ebios_loop

.exit:
    pop     cx
    ret


;--------------------------------------------------------------------------
; read_lba - Read sectors from a partition using LBA addressing.
;
; Arguments:
;   AL = number of 512-byte sectors to read (valid from 1-127).
;   ES:BX = pointer to where the sectors should be stored.
;   DL = drive number (0x80 + unit number)
;   [my_lba] = starting LBA.
;
; Returns:
;   CF = 0 success
;        1 error
;
read_lba:
    pushad                          ; save all registers
    mov     bp, sp                  ; save current SP

    ;
    ; Create the Disk Address Packet structure for the
    ; INT13/F42 (Extended Read Sectors) on the stack.
    ;

;   push    DWORD 0                 ; offset 12, upper 32-bit LBA
    push    ds                      ; For sake of saving memory,
    push    ds                      ; push DS register, which is 0.
    mov     ecx, [my_lba]           ; offset 8, lower 32-bit LBA
    push    ecx
    push    es                      ; offset 6, memory segment
    push    bx                      ; offset 4, memory offset
    xor     ah, ah                  ; offset 3, must be 0
    push    ax                      ; offset 2, number of sectors

    ; It pushes 2 bytes with a smaller opcode than if WORD was used
    push    BYTE 16                 ; offset 0-1, packet size

    DebugChar('<')
%if DEBUG
    mov  eax, ecx
    call print_hex
%endif

    ;
    ; INT13 Func 42 - Extended Read Sectors
    ;
    ; Arguments:
    ;   AH    = 0x42
    ;   DL    = drive number (80h + drive unit)
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
    mov     si, sp
    mov     ah, 0x42
    int     0x13

    jnc     .exit

    DebugChar('R')                  ; indicate INT13/F42 error

    ;
    ; Issue a disk reset on error.
    ; Should this be changed to Func 0xD to skip the diskette controller
    ; reset?
    ;
    xor     ax, ax                  ; Func 0
    int     0x13                    ; INT 13
    stc                             ; set carry to indicate error

.exit:
    mov     sp, bp                  ; restore SP
    popad
    ret


;--------------------------------------------------------------------------
; Write a string with 'boot0: ' prefix to the console.
;
; Arguments:
;   ES:DI   pointer to a NULL terminated string.
;
; Clobber list:
;   DI
;
log_string:
    pusha

    push    di
    mov     si, log_title_str
    call    print_string

    pop     si
    call    print_string

    popa

    ret


;--------------------------------------------------------------------------
; Write a string to the console.
;
; Arguments:
;   DS:SI   pointer to a NULL terminated string.
;
; Clobber list:
;   AX, BX, SI
;
print_string:
    mov     bx, 1                   ; BH=0, BL=1 (blue)
    cld                             ; increment SI after each lodsb call
.loop:
    lodsb                           ; load a byte from DS:SI into AL
    cmp     al, 0                   ; Is it a NULL?
    je      .exit                   ; yes, all done
    mov     ah, 0xE                 ; INT10 Func 0xE
    int     0x10                    ; display byte in tty mode
    jmp     short .loop
.exit:
    ret


%if DEBUG

;--------------------------------------------------------------------------
; Write a ASCII character to the console.
;
; Arguments:
;   AL = ASCII character.
;
print_char:
    pusha
    mov     bx, 1                   ; BH=0, BL=1 (blue)
    mov     ah, 0x0e                ; bios INT 10, Function 0xE
    int     0x10                    ; display byte in tty mode
    popa
    ret


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
    call    print_nibble            ; display upper nibble
    pop     ax
    call    print_nibble            ; display lower nibble
    ror     eax, 8
    loop    .loop

    mov     al, 10                  ; carriage return
    call    print_char
    mov     al, 13
    call    print_char

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

getc:
    pusha
    mov    ah, 0
    int    0x16
    popa
    ret
%endif ;DEBUG


;--------------------------------------------------------------------------
; NULL terminated strings.
;

%if VERBOSE
gpt_str         db  'GPT', 0
test_str        db  'test', 0
done_str        db  'done', 0
%endif

boot_error_str  db  'error', 0

;--------------------------------------------------------------------------
; Pad the rest of the 512 byte sized booter with zeroes. The last
; two bytes is the mandatory boot sector signature.
;
; If the booter code becomes too large, then nasm will complain
; that the 'times' argument is negative.

;
; According to EFI specification, maximum boot code size is 440 bytes
;

pad_boot:
    times  428-($-$$) db 0  ; 428 = 440 - len(log_title_str)

log_title_str:
    db  10, 13, 'boot0af: ', 0  ; can be use as signature

pad_table_and_sig:
    times 510-($-$$) db 0
    dw    kBootSignature

    ABSOLUTE 0xE400

;
; In memory variables.
;
my_lba          resd    1   ; Starting LBA for read_lba function

; END

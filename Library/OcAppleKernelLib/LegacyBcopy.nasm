;------------------------------------------------------------------------------
;  @file
;  Copyright (C) 2020, vit9696. All rights reserved.
;  Copyright (C) 2006, Apple Computer, Inc. All rights reserved.
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
; The bcopy/memcpy loops, tuned for 64-bit Pentium-M class processors with
; Supplemental SSE3 and 64-byte cache lines. This is the 64-bit version.
;
; To generate the binary blob execute the following command:
; nasm LegacyBcopy.nasm -o /dev/stdout | xxd -i > LegacyBcopy.h
;
; The following #defines are tightly coupled to the u-architecture:
;------------------------------------------------------------------------------
%define kShort      80                  ; too short to bother with SSE (must be >=80)
%define kVeryLong   (500*1024)          ; large enough for non-temporal stores (>=8192 and <2GB)
%define kFastUCode  ((16*1024)-15)      ; cutoff for microcode fastpath for "rep/movsl"
%define COMM_PAGE_LONGCOPY 7FFFFFE01200h

;------------------------------------------------------------------------------
; void bcopy(const void *src, void *dst, size_t len)
; src, dst, len ~ rdi, rsi, rdx
;------------------------------------------------------------------------------
Lbcopy:
  push    rbp                      ; set up a frame for backtraces
  mov     rbp, rsp
  mov     rax, rsi                 ; copy dest ptr
  mov     rsi, rdi                 ; xchange source and dest ptrs
  mov     rdi, rax
  sub     rax, rsi                 ; (dest - source)
  cmp     rax, rdx                 ; must move in reverse if (dest - source) < length
  jb      short LReverseIsland
  cmp     rdx, kShort              ; long enough to bother with SSE?
  jbe     short LShort             ; no
  jmp     short LNotShort

;------------------------------------------------------------------------------
; void *memcpy(void *dst, const void *src, size_t len)
; void *memmove(void *dst, const void *src, size_t len)
;
; NB: These need to be 32 bytes from bcopy().
;------------------------------------------------------------------------------
align 32
Lmemcpy:
Lmemmove:
  push    rbp                      ; set up a frame for backtraces
  mov     rbp, rsp
  mov     r11, rdi                 ; save return value here
  mov     rax, rdi
  sub     rax, rsi                 ; (dest - source)
  cmp     rax, rdx                 ; must move in reverse if (dest - source) < length
  jb      short LReverseIsland
  cmp     rdx, kShort              ; long enough to bother with SSE?
  ja      short LNotShort          ; yes

;------------------------------------------------------------------------------
; Handle short forward copies.  As the most common case, this is the fall-through path.
;      rdx = length (<= kShort)
;      rsi = source ptr
;      rdi = dest ptr
;------------------------------------------------------------------------------
LShort:
  mov     ecx, edx                 ; copy length using 32-bit operation
  shr     ecx, 2                   ; get #doublewords
  jz      short LLeftovers

.cycle:                            ; loop copying doublewords
  mov     eax, [rsi]
  add     rsi, 4
  mov     [rdi], eax
  add     rdi, 4
  dec     ecx
  jnz     short .cycle

LLeftovers:                        ; handle leftover bytes (0..3) in last word
  and     edx, 3
  jz      short .skip              ; any leftover bytes?

.cycle:                            ; loop copying bytes
  mov     al, [rsi]
  inc     rsi
  mov     [rdi], al
  inc     rdi
  dec     edx
  jnz     short .cycle

.skip:
  mov     rax, r11                 ; get return value (dst ptr) for memcpy/memmove
  pop     rbp
  retn

LReverseIsland:                    ; keep the "jb" above a short branch...
  jmp     LReverse                 ; ...because reverse moves are uncommon

;------------------------------------------------------------------------------
; Handle forward moves that are long enough to justify use of SSE.
; First, 16-byte align the destination.
;      rdx = length (> kShort)
;      rsi = source ptr
;      rdi = dest ptr
;------------------------------------------------------------------------------
LNotShort:
  cmp     rdx, kVeryLong           ; long enough to justify heavyweight loops?
  jnb     short LVeryLong          ; use very-long-operand path
  mov     ecx, edi                 ; copy low half of destination ptr
  neg     ecx
  and     ecx, 15                  ; get #bytes to align destination
  jz      short LDestAligned       ; already aligned
  sub     edx, ecx                 ; decrement length
  rep movsb                        ; align destination

;------------------------------------------------------------------------------
; Destination is now aligned. Dispatch to the loops over 64-byte chunks,
; based on the alignment of the source. All vector loads and stores are aligned.
; Even though this means we have to shift and repack vectors, doing so is much faster
; than unaligned loads. Since kShort>=80 and we've moved at most 15 bytes already,
; there is at least one chunk. When we enter the copy loops, the following registers
; are set up:
;      rdx = residual length (0..63)
;      rcx = -(length to move), a multiple of 64 less than 2GB
;      rsi = ptr to 1st source byte not to move (unaligned)
;      rdi = ptr to 1st dest byte not to move (aligned)
;------------------------------------------------------------------------------
LDestAligned:
  mov     rcx, rdx                 ; copy length
  mov     eax, esi                 ; copy low half of source address
  and     edx, 63                  ; get remaining bytes for LShort
  and     rcx, -64                 ; get number of bytes we will copy in inner loop
  add     rsi, rcx                 ; point to 1st byte not copied
  add     rdi, rcx
  neg     rcx                      ; now generate offset to 1st byte to be copied
  ; Choose the loop. Without SSSE3 we only have two choices.
  ; 16-byte aligned loop (LMod0) and 1-byte unaligned loop (LMod1).
  and     eax, 15
  jz      short LMod0
  jmp     short LMod1

;------------------------------------------------------------------------------
; Very long forward moves.  These are at least several pages.  They are special cased
; and aggressively optimized, not so much because they are common or useful, but
; because they are subject to benchmark.  There isn't enough room for them in the
; area reserved on the commpage for bcopy, so we put them elsewhere.  We call
; the longcopy routine using the normal ABI:
;      rdi = dest
;      rsi = source
;      rdx = length (>= kVeryLong bytes)
;------------------------------------------------------------------------------
LVeryLong:
  push    r11                      ; save return value
  mov     rax, COMM_PAGE_LONGCOPY
  call    rax                      ; call very long operand routine
  pop     rax                      ; pop return value
  pop     rbp
  retn

;------------------------------------------------------------------------------
; On Pentium-M, the microcode for "rep/movsl" is faster than SSE for 16-byte
; aligned operands from about 32KB up to kVeryLong for the hot cache case, and from
; about 256 bytes up to kVeryLong for cold caches.  This is because the microcode
; avoids having to read destination cache lines that will be completely overwritten.
; The cutoff we use (ie, kFastUCode) must somehow balance the two cases, since
; we do not know if the destination is in cache or not.
;------------------------------------------------------------------------------
Lfastpath:
  add     rsi, rcx                 ; restore ptrs to 1st byte of source and dest
  add     rdi, rcx
  neg     ecx                      ; make length positive (known to be < 2GB)
  or      ecx, edx                 ; restore total #bytes remaining to move
  cld                              ; we'll move forward
  shr     ecx, 2                   ; compute #words to move
  rep movsd                        ; the u-code will optimize this
  jmp     LLeftovers               ; handle 0..3 leftover bytes

;------------------------------------------------------------------------------
; Forward loop for medium length operands in which low four bits of %rsi == 0000
;------------------------------------------------------------------------------
LMod0:
  cmp     ecx, -kFastUCode         ; %rcx == -length, where (length < kVeryLong)
  jle     short Lfastpath          ; long enough for fastpath in microcode
  jmp     short .loop

align 16                           ; 16-byte align inner loops
.loop:                             ; loop over 64-byte chunks
  movdqa  xmm0, oword [rsi+rcx]
  movdqa  xmm1, oword [rsi+rcx+10h]
  movdqa  xmm2, oword [rsi+rcx+20h]
  movdqa  xmm3, oword [rsi+rcx+30h]
  movdqa  oword [rdi+rcx], xmm0
  movdqa  oword [rdi+rcx+10h], xmm1
  movdqa  oword [rdi+rcx+20h], xmm2
  movdqa  oword [rdi+rcx+30h], xmm3
  add     rcx, 64
  jnz     short .loop
  jmp     LShort                    ; copy remaining 0..63 bytes and done

;------------------------------------------------------------------------------
; Forward loop for medium length operands in which low four bits of %rsi != 0000
;------------------------------------------------------------------------------
align 16
LMod1:
  movdqu  xmm0, oword [rsi+rcx]
  movdqu  xmm1, oword [rsi+rcx+10h]
  movdqu  xmm2, oword [rsi+rcx+20h]
  movdqu  xmm3, oword [rsi+rcx+30h]
  movdqa  oword [rdi+rcx], xmm0
  movdqa  oword [rdi+rcx+10h], xmm1
  movdqa  oword [rdi+rcx+20h], xmm2
  movdqa  oword [rdi+rcx+30h], xmm3
  add     rcx, 64
  jnz     short LMod1
  jmp     LShort                    ; copy remaining 0..63 bytes and done

;------------------------------------------------------------------------------
; Reverse moves.  These are not optimized as aggressively as their forward
; counterparts, as they are only used with destructive overlap.
;      rdx = length
;      rsi = source ptr
;      rdi = dest ptr
;------------------------------------------------------------------------------
LReverse:
  add     rsi, rdx                  ; point to end of strings
  add     rdi, rdx
  cmp     rdx, kShort               ; long enough to bother with SSE?
  ja      short LReverseNotShort    ; yes

;------------------------------------------------------------------------------
; Handle reverse short copies.
;      edx = length (<= kShort)
;      rsi = one byte past end of source
;      rdi = one byte past end of dest
;------------------------------------------------------------------------------
LReverseShort:
  mov     ecx, edx                  ; copy length
  shr     ecx, 3                    ; #quadwords
  jz      short .l2

.l1:
  sub     rsi, 8
  mov     rax, [rsi]
  sub     rdi, 8
  mov     [rdi], rax
  dec     ecx
  jnz     short .l1

.l2:
  and     edx, 7                    ; bytes?
  jz      short .l4

.l3:
  dec     rsi
  mov     al, [rsi]
  dec     rdi
  mov     [rdi], al
  dec     edx
  jnz     short .l3

.l4:
  mov     rax, r11                  ; get return value (dst ptr) for memcpy/memmove
  pop     rbp
  ret

;------------------------------------------------------------------------------
; Handle a reverse move long enough to justify using SSE.
;      rdx = length (> kShort)
;      rsi = one byte past end of source
;      rdi = one byte past end of dest
;------------------------------------------------------------------------------
LReverseNotShort:
  mov     ecx, edi                  ; copy destination
  and     ecx, 15                   ; get #bytes to align destination
  jz      short LReverseDestAligned ; already aligned
  sub     rdx, rcx                  ; adjust length
.cycle:                             ; loop copying 1..15 bytes
  dec     rsi
  mov     al, [rsi]
  dec     rdi
  mov     [rdi], al
  dec     ecx
  jnz     short .cycle

;------------------------------------------------------------------------------
; Destination is now aligned.  Prepare for reverse loops.
;------------------------------------------------------------------------------
LReverseDestAligned:
  mov     rcx, rdx                  ; copy length
  and     edx, 63                   ; get remaining bytes for LReverseShort
  and     rcx, -64                  ; get number of bytes we will copy in inner loop
  sub     rsi, rcx                  ; point to endpoint of copy
  sub     rdi, rcx
  test    esi, 15                   ; is source aligned too?
  jnz     short LReverseUnalignedLoop

;------------------------------------------------------------------------------
; Reverse loop over 64-byte aligned chunks.
;------------------------------------------------------------------------------
LReverseAlignedLoop:
  movdqa  xmm0, oword [rsi+rcx-16]
  movdqa  xmm1, oword [rsi+rcx-32]
  movdqa  xmm2, oword [rsi+rcx-48]
  movdqa  xmm3, oword [rsi+rcx-64]
  movdqa  oword [rdi+rcx-16], xmm0
  movdqa  oword [rdi+rcx-32], xmm1
  movdqa  oword [rdi+rcx-48], xmm2
  movdqa  oword [rdi+rcx-64], xmm3
  sub     rcx, 64
  jnz     short LReverseAlignedLoop
  jmp     LReverseShort             ; copy remaining 0..63 bytes and done

;------------------------------------------------------------------------------
; Reverse, unaligned loop. LDDQU==MOVDQU on these machines.
;------------------------------------------------------------------------------
LReverseUnalignedLoop:
  movdqu  xmm0, oword [rsi+rcx-16]
  movdqu  xmm1, oword [rsi+rcx-32]
  movdqu  xmm2, oword [rsi+rcx-48]
  movdqu  xmm3, oword [rsi+rcx-64]
  movdqa  oword [rdi+rcx-16], xmm0
  movdqa  oword [rdi+rcx-32], xmm1
  movdqa  oword [rdi+rcx-48], xmm2
  movdqa  oword [rdi+rcx-64], xmm3
  sub     rcx, 64
  jnz     short LReverseUnalignedLoop
  jmp     LReverseShort             ; copy remaining 0..63 bytes and done

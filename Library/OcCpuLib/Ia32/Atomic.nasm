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

BITS     32
DEFAULT  REL

SECTION  .text

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; AsmIncrementUint32 (
;   IN volatile UINT32  *Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmIncrementUint32)
ASM_PFX(AsmIncrementUint32):
  mov       ecx, [esp + 4]
  mov       eax, 1
  lock xadd dword [ecx], eax
  inc       eax
  ret

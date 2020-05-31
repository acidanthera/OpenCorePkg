/** @file
  Check memory routine compatibility.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC UINT8  mSource[64];
STATIC UINT8  mDestination[64];

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT8          *Src;
  UINT8          *Dst;
  UINT32         Size;
  IA32_CR0       Cr0;
  IA32_CR4       Cr4;
  IA32_EFLAGS32  Flags;

  Src  = ALIGN_POINTER (mSource, 16);
  Dst  = ALIGN_POINTER (mDestination, 16);
  Size = 32;

  gST->ConOut->OutputString (gST->ConOut, L"VerifyMemOpt\r\n");
  gBS->Stall (SECONDS_TO_MICROSECONDS (1));

  Cr0.UintN   = AsmReadCr0 ();
  Cr4.UintN   = AsmReadCr4 ();
  Flags.UintN = AsmReadEflags();

  //
  // CR0.MP         bit (BIT1)   must be set.
  // CR0.EM         bit (BIT2)   must be cleared.
  // CR4.OSFXSR     bit (BIT9)   must be set.
  // CR4.OSXMMEXCPT bit (BIT10)  must be set.
  //
  DEBUG ((DEBUG_WARN, "VMOPT: CR0 %08X CR4 %08X EFLAGS %08X\n", Cr0, Cr4, Flags));

  if (Cr0.Bits.MP == 0) {
    DEBUG ((DEBUG_WARN, "VMOPT: WARN CR0 MP is NOT set\n"));
  }

  if (Cr0.Bits.EM != 0) {
    DEBUG ((DEBUG_WARN, "VMOPT: WARN CR0 EM is set\n"));
  }

  if (Cr4.Bits.OSFXSR == 0) {
    DEBUG ((DEBUG_WARN, "VMOPT: WARN CR4 OSFXSR is NOT set\n"));
  }

  if (Cr4.Bits.OSXMMEXCPT == 0) {
    DEBUG ((DEBUG_WARN, "VMOPT: WARN CR4 OSXMMEXCPT is NOT set\n"));
  }

  if (Flags.Bits.DF != 0) {
    DEBUG ((DEBUG_WARN, "VMOPT: WARN EFLAGS DF is set\n"));
  }

  DEBUG ((DEBUG_WARN, "VMOPT: CopyMem aligned src %p/aligned dst %p/size %u\n", Src, Dst, Size));
  gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  CopyMem (Dst, Src, Size);

  Src  += 6;
  DEBUG ((DEBUG_WARN, "VMOPT: CopyMem unaligned src %p/aligned dst %p/size %u\n", Src, Dst, Size));
  gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  CopyMem (Dst, Src, Size);

  Size -= 12;
  DEBUG ((DEBUG_WARN, "VMOPT: CopyMem unaligned src %p/aligned dst %p/size %u\n", Src, Dst, Size));
  gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  CopyMem (Dst, Src, Size);

  Dst  += 6;
  DEBUG ((DEBUG_WARN, "VMOPT: CopyMem unaligned src %p/unaligned dst %p/size %u\n", Src, Dst, Size));
  gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  CopyMem (Dst, Src, Size);

  DEBUG ((DEBUG_WARN, "VMOPT: Done testing\n"));
  gBS->Stall (SECONDS_TO_MICROSECONDS (1));

  return EFI_SUCCESS;
}

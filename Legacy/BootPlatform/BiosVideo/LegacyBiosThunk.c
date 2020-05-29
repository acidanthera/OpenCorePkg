/** @file
  Provide legacy thunk interface for accessing Bios Video Rom.

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BiosVideo.h"

/**
  Initialize legacy environment for BIOS INI caller.

  @param ThunkContext   the instance pointer of THUNK_CONTEXT
**/
VOID
InitializeBiosIntCaller (
  THUNK_CONTEXT     *ThunkContext
  )
{
  EFI_STATUS            Status;
  UINT32                RealModeBufferSize;
  UINT32                ExtraStackSize;
  EFI_PHYSICAL_ADDRESS  LegacyRegionBase;
  UINT32                LegacyRegionSize;
  //
  // Get LegacyRegion
  //
  AsmGetThunk16Properties (&RealModeBufferSize, &ExtraStackSize);
  LegacyRegionSize = (((RealModeBufferSize + ExtraStackSize) / EFI_PAGE_SIZE) + 1) * EFI_PAGE_SIZE;
  LegacyRegionBase = LEGACY_REGION_BASE;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(LegacyRegionSize),
                  &LegacyRegionBase
                  );
  ASSERT_EFI_ERROR (Status);

  ThunkContext->RealModeBuffer     = (VOID*)(UINTN)LegacyRegionBase;
  ThunkContext->RealModeBufferSize = LegacyRegionSize;
  ThunkContext->ThunkAttributes    = THUNK_ATTRIBUTE_BIG_REAL_MODE|THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15;
  AsmPrepareThunk16(ThunkContext);
}

/**
   Initialize interrupt redirection code and entries, because
   IDT Vectors 0x68-0x6f must be redirected to IDT Vectors 0x08-0x0f.
   Or the interrupt will lost when we do thunk.
   NOTE: We do not reset 8259 vector base, because it will cause pending
   interrupt lost.

   @param Legacy8259  Instance pointer for EFI_LEGACY_8259_PROTOCOL.

**/
VOID
InitializeInterruptRedirection (
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  LegacyRegionBase;
  UINTN                 LegacyRegionLength;
  volatile UINT32       *IdtArray;
  UINTN                 Index;
  UINT8                 ProtectedModeBaseVector;

  STATIC CONST UINT32 InterruptRedirectionCode[] = {
    0x90CF08CD, // INT8; IRET; NOP
    0x90CF09CD, // INT9; IRET; NOP
    0x90CF0ACD, // INTA; IRET; NOP
    0x90CF0BCD, // INTB; IRET; NOP
    0x90CF0CCD, // INTC; IRET; NOP
    0x90CF0DCD, // INTD; IRET; NOP
    0x90CF0ECD, // INTE; IRET; NOP
    0x90CF0FCD  // INTF; IRET; NOP
  };


  //
  // Get LegacyRegion
  //
  LegacyRegionLength = sizeof(InterruptRedirectionCode);
  LegacyRegionBase = LEGACY_REGION_BASE;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(LegacyRegionLength),
                  &LegacyRegionBase
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Copy code to legacy region
  //
  CopyMem ((VOID *)(UINTN)LegacyRegionBase, InterruptRedirectionCode, sizeof (InterruptRedirectionCode));

  //
  // Get VectorBase, it should be 0x68
  //
  Status = Legacy8259->GetVector (Legacy8259, Efi8259Irq0, &ProtectedModeBaseVector);
  ASSERT_EFI_ERROR (Status);

  //
  // Patch IVT 0x68 ~ 0x6f
  //
  IdtArray = (UINT32 *) 0;
  for (Index = 0; Index < 8; Index++) {
    IdtArray[ProtectedModeBaseVector + Index] = ((EFI_SEGMENT (LegacyRegionBase + Index * 4)) << 16) | (EFI_OFFSET (LegacyRegionBase + Index * 4));
  }
}

/**
  Thunk to 16-bit real mode and execute a software interrupt with a vector
  of BiosInt. Regs will contain the 16-bit register context on entry and
  exit.

  @param  This    Protocol instance pointer.
  @param  BiosInt Processor interrupt vector to invoke
  @param  Reg     Register contexted passed into (and returned) from thunk to 16-bit mode

  @retval TRUE   Thunk completed, and there were no BIOS errors in the target code.
                 See Regs for status.
  @retval FALSE  There was a BIOS erro in the target code.
**/
BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  BIOS_VIDEO_DEV                 *BiosDev,
  IN  UINT8                           BiosInt,
  IN  IA32_REGISTER_SET              *Regs
  )
{
  UINTN                 Status;
  IA32_REGISTER_SET     ThunkRegSet;
  BOOLEAN               Ret;
  UINT16                *Stack16;
  BOOLEAN               Enabled;

  ZeroMem (&ThunkRegSet, sizeof (ThunkRegSet));
  ThunkRegSet.E.EFLAGS.Bits.Reserved_0 = 1;
  ThunkRegSet.E.EFLAGS.Bits.Reserved_1 = 0;
  ThunkRegSet.E.EFLAGS.Bits.Reserved_2 = 0;
  ThunkRegSet.E.EFLAGS.Bits.Reserved_3 = 0;
  ThunkRegSet.E.EFLAGS.Bits.IOPL       = 3;
  ThunkRegSet.E.EFLAGS.Bits.NT         = 0;
  ThunkRegSet.E.EFLAGS.Bits.IF         = 1;
  ThunkRegSet.E.EFLAGS.Bits.TF         = 0;
  ThunkRegSet.E.EFLAGS.Bits.CF         = 0;

  ThunkRegSet.E.EDI  = Regs->E.EDI;
  ThunkRegSet.E.ESI  = Regs->E.ESI;
  ThunkRegSet.E.EBP  = Regs->E.EBP;
  ThunkRegSet.E.EBX  = Regs->E.EBX;
  ThunkRegSet.E.EDX  = Regs->E.EDX;
  ThunkRegSet.E.ECX  = Regs->E.ECX;
  ThunkRegSet.E.EAX  = Regs->E.EAX;
  ThunkRegSet.E.DS   = Regs->E.DS;
  ThunkRegSet.E.ES   = Regs->E.ES;

  //
  // The call to Legacy16 is a critical section to EFI
  //
  Enabled = SaveAndDisableInterrupts();

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = BiosDev->Legacy8259->SetMode (BiosDev->Legacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  Stack16 = (UINT16 *)((UINT8 *) BiosDev->ThunkContext->RealModeBuffer + BiosDev->ThunkContext->RealModeBufferSize - sizeof (UINT16));

  ThunkRegSet.E.SS   = (UINT16) (((UINTN) Stack16 >> 16) << 12);
  ThunkRegSet.E.ESP  = (UINT16) (UINTN) Stack16;

  ThunkRegSet.E.Eip  = (UINT16)((volatile UINT32 *)NULL)[BiosInt];
  ThunkRegSet.E.CS   = (UINT16)(((volatile UINT32 *)NULL)[BiosInt] >> 16);
  BiosDev->ThunkContext->RealModeState = &ThunkRegSet;
  AsmThunk16 (BiosDev->ThunkContext);

  //
  // Restore protected mode interrupt state
  //
  Status = BiosDev->Legacy8259->SetMode (BiosDev->Legacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // End critical section
  //
  SetInterruptState (Enabled);

  Regs->E.EDI      = ThunkRegSet.E.EDI;
  Regs->E.ESI      = ThunkRegSet.E.ESI;
  Regs->E.EBP      = ThunkRegSet.E.EBP;
  Regs->E.EBX      = ThunkRegSet.E.EBX;
  Regs->E.EDX      = ThunkRegSet.E.EDX;
  Regs->E.ECX      = ThunkRegSet.E.ECX;
  Regs->E.EAX      = ThunkRegSet.E.EAX;
  Regs->E.SS       = ThunkRegSet.E.SS;
  Regs->E.CS       = ThunkRegSet.E.CS;
  Regs->E.DS       = ThunkRegSet.E.DS;
  Regs->E.ES       = ThunkRegSet.E.ES;

  CopyMem (&(Regs->E.EFLAGS), &(ThunkRegSet.E.EFLAGS), sizeof (UINT32));

  Ret = (BOOLEAN) (Regs->E.EFLAGS.Bits.CF == 1);

  return Ret;
}

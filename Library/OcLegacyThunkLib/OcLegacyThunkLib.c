/** @file
  This file implements thunking to legacy 16-bit environment.

  Copyright (c) 2023, Goldfish64. All rights reserved.<BR>
  Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#include <Uefi.h>

#include <IndustryStandard/Pci.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/OcLegacyThunkLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/PciIo.h>
#include <Protocol/Timer.h>

//
// 8254 Timer registers
//
#define TIMER0_COUNT_PORT    0x40
#define TIMER1_COUNT_PORT    0x41
#define TIMER2_COUNT_PORT    0x42
#define TIMER_CONTROL_PORT   0x43
#define TIMER0_CONTROL_WORD  0x36

//
// Vector base definitions
//
//
// 8259 Hardware definitions
//
#define LEGACY_MODE_BASE_VECTOR_MASTER  0x08
#define LEGACY_MODE_BASE_VECTOR_SLAVE   0x70

//
// The original PC used INT8-F for master PIC. Since these mapped over
// processor exceptions TIANO moved the master PIC to INT68-6F.
//
// The vector base for slave PIC is set as 0x70 for PC-AT compatibility.
//
#define PROTECTED_MODE_BASE_VECTOR_MASTER  0x68
#define PROTECTED_MODE_BASE_VECTOR_SLAVE   0x70

/**
  Initialize legacy environment for BIOS INI caller.

  @param ThunkContext   The instance pointer of THUNK_CONTEXT.
**/
EFI_STATUS
OcLegacyThunkInitializeBiosIntCaller (
  THUNK_CONTEXT  *ThunkContext
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
  Status           = gBS->AllocatePages (
                            AllocateMaxAddress,
                            EfiACPIMemoryNVS,
                            EFI_SIZE_TO_PAGES (LegacyRegionSize),
                            &LegacyRegionBase
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem ((VOID *)(UINTN)LegacyRegionBase, LegacyRegionSize);

  ThunkContext->RealModeBuffer     = (VOID *)(UINTN)LegacyRegionBase;
  ThunkContext->RealModeBufferSize = LegacyRegionSize;
  ThunkContext->ThunkAttributes    = THUNK_ATTRIBUTE_BIG_REAL_MODE|THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15;
  AsmPrepareThunk16 (ThunkContext);
  return Status;
}

/**
   Initialize interrupt redirection code and entries, because
   IDT Vectors 0x68-0x6f must be redirected to IDT Vectors 0x08-0x0f.
   Or the interrupt will lost when we do thunk.
   NOTE: We do not reset 8259 vector base, because it will cause pending
   interrupt lost.

   @param Legacy8259  Instance pointer for EFI_LEGACY_8259_PROTOCOL.

**/
EFI_STATUS
OcLegacyThunkInitializeInterruptRedirection (
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  LegacyRegionBase;
  UINTN                 LegacyRegionLength;
  volatile UINT32       *IdtArray;
  UINTN                 Index;
  UINT8                 ProtectedModeBaseVector;

  STATIC CONST UINT32  InterruptRedirectionCode[] = {
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
  LegacyRegionLength = sizeof (InterruptRedirectionCode);
  LegacyRegionBase   = LEGACY_REGION_BASE;
  Status             = gBS->AllocatePages (
                              AllocateMaxAddress,
                              EfiACPIMemoryNVS,
                              EFI_SIZE_TO_PAGES (LegacyRegionLength),
                              &LegacyRegionBase
                              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
  IdtArray = (UINT32 *)0;
  for (Index = 0; Index < 8; Index++) {
    IdtArray[ProtectedModeBaseVector + Index] = ((EFI_SEGMENT (LegacyRegionBase + Index * 4)) << 16) | (EFI_OFFSET (LegacyRegionBase + Index * 4));
  }

  return Status;
}

/**
   Disconnect all EFI graphics device handles in preparation for calling to legacy mode.
**/
VOID
OcLegacyThunkDisconnectEfiGraphics (
  VOID
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  EFI_HANDLE           *HandleBuffer;
  UINTN                Index;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_CLASSCODE        ClassCode;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint8,
                          PCI_CLASSCODE_OFFSET,
                          sizeof (PCI_CLASSCODE) / sizeof (UINT8),
                          &ClassCode
                          );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((ClassCode.BaseCode == PCI_CLASS_DISPLAY) && (ClassCode.SubClassCode == PCI_CLASS_DISPLAY_VGA)) {
      Status = gBS->DisconnectController (
                      HandleBuffer[Index],
                      NULL,
                      NULL
                      );
      DEBUG ((DEBUG_INFO, "OCLT: Disconnected graphics controller - %r\n", Status));
    }
  }

  FreePool (HandleBuffer);
}

/**
  Thunk to 16-bit real mode and execute a software interrupt with a vector
  of BiosInt. Regs will contain the 16-bit register context on entry and
  exit.

  @param  ThunkContext    The instance pointer of THUNK_CONTEXT.
  @param  Legacy8259      Instance pointer for EFI_LEGACY_8259_PROTOCOL.
  @param  BiosInt         Processor interrupt vector to invoke
  @param  Reg             Register contexted passed into (and returned) from thunk to 16-bit mode

  @retval TRUE   Thunk completed, and there were no BIOS errors in the target code.
                 See Regs for status.
  @retval FALSE  There was a BIOS error in the target code.
**/
BOOLEAN
EFIAPI
OcLegacyThunkBiosInt86 (
  IN  THUNK_CONTEXT             *ThunkContext,
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  IN  UINT8                     BiosInt,
  IN  IA32_REGISTER_SET         *Regs
  )
{
  UINTN              Status;
  IA32_REGISTER_SET  ThunkRegSet;
  BOOLEAN            Ret;
  UINT16             *Stack16;
  BOOLEAN            Enabled;

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

  ThunkRegSet.E.EDI = Regs->E.EDI;
  ThunkRegSet.E.ESI = Regs->E.ESI;
  ThunkRegSet.E.EBP = Regs->E.EBP;
  ThunkRegSet.E.EBX = Regs->E.EBX;
  ThunkRegSet.E.EDX = Regs->E.EDX;
  ThunkRegSet.E.ECX = Regs->E.ECX;
  ThunkRegSet.E.EAX = Regs->E.EAX;
  ThunkRegSet.E.DS  = Regs->E.DS;
  ThunkRegSet.E.ES  = Regs->E.ES;

  //
  // The call to Legacy16 is a critical section to EFI
  //
  Enabled = SaveAndDisableInterrupts ();

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = Legacy8259->SetMode (Legacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  Stack16 = (UINT16 *)((UINT8 *)ThunkContext->RealModeBuffer + ThunkContext->RealModeBufferSize - sizeof (UINT16));

  ThunkRegSet.E.SS  = (UINT16)(((UINTN)Stack16 >> 16) << 12);
  ThunkRegSet.E.ESP = (UINT16)(UINTN)Stack16;

  ThunkRegSet.E.Eip           = (UINT16)((volatile UINT32 *)NULL)[BiosInt];
  ThunkRegSet.E.CS            = (UINT16)(((volatile UINT32 *)NULL)[BiosInt] >> 16);
  ThunkContext->RealModeState = &ThunkRegSet;
  AsmThunk16 (ThunkContext);

  //
  // Restore protected mode interrupt state
  //
  Status = Legacy8259->SetMode (Legacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // End critical section
  //
  SetInterruptState (Enabled);

  Regs->E.EDI = ThunkRegSet.E.EDI;
  Regs->E.ESI = ThunkRegSet.E.ESI;
  Regs->E.EBP = ThunkRegSet.E.EBP;
  Regs->E.EBX = ThunkRegSet.E.EBX;
  Regs->E.EDX = ThunkRegSet.E.EDX;
  Regs->E.ECX = ThunkRegSet.E.ECX;
  Regs->E.EAX = ThunkRegSet.E.EAX;
  Regs->E.SS  = ThunkRegSet.E.SS;
  Regs->E.CS  = ThunkRegSet.E.CS;
  Regs->E.DS  = ThunkRegSet.E.DS;
  Regs->E.ES  = ThunkRegSet.E.ES;

  CopyMem (&(Regs->E.EFLAGS), &(ThunkRegSet.E.EFLAGS), sizeof (UINT32));

  Ret = (BOOLEAN)(Regs->E.EFLAGS.Bits.CF == 1);

  return Ret;
}

BOOLEAN
EFIAPI
OcLegacyThunkFarCall86 (
  IN  THUNK_CONTEXT             *ThunkContext,
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259,
  IN  UINT16                    Segment,
  IN  UINT16                    Offset,
  IN  IA32_REGISTER_SET         *Regs,
  IN  VOID                      *Stack,
  IN  UINTN                     StackSize
  )
{
  UINTN              Status;
  UINT16             *Stack16;
  IA32_REGISTER_SET  ThunkRegSet;
  BOOLEAN            Ret;
  UINT64             TimerPeriod;
  BOOLEAN            Enabled;

  EFI_TIMER_ARCH_PROTOCOL  *TimerProtocol;

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

  ThunkRegSet.E.EDI = Regs->E.EDI;
  ThunkRegSet.E.ESI = Regs->E.ESI;
  ThunkRegSet.E.EBP = Regs->E.EBP;
  ThunkRegSet.E.EBX = Regs->E.EBX;
  ThunkRegSet.E.EDX = Regs->E.EDX;
  ThunkRegSet.E.ECX = Regs->E.ECX;
  ThunkRegSet.E.EAX = Regs->E.EAX;
  ThunkRegSet.E.DS  = Regs->E.DS;
  ThunkRegSet.E.ES  = Regs->E.ES;

  //
  // The call to Legacy16 is a critical section to EFI
  //
  Enabled = SaveAndDisableInterrupts ();

  //
  // Save current rate of DXE timer and disable it
  // while executing in real mode.
  //
  Status = gBS->LocateProtocol (
                  &gEfiTimerArchProtocolGuid,
                  NULL,
                  (VOID **)&TimerProtocol
                  );
  if (!EFI_ERROR (Status)) {
    TimerProtocol->GetTimerPeriod (TimerProtocol, &TimerPeriod);
    TimerProtocol->SetTimerPeriod (TimerProtocol, 0);
  } else {
    TimerProtocol = NULL;
  }

  //
  // Reset timer to default legacy rate of 54.9254.
  //
  IoWrite8 (TIMER_CONTROL_PORT, TIMER0_CONTROL_WORD);
  IoWrite8 (TIMER0_COUNT_PORT, 0x00);
  IoWrite8 (TIMER0_COUNT_PORT, 0x00);

  //
  // Put the 8259 into its legacy mode by reprogramming the vector bases
  //
  Status = Legacy8259->SetVectorBase (Legacy8259, LEGACY_MODE_BASE_VECTOR_MASTER, LEGACY_MODE_BASE_VECTOR_SLAVE);
  ASSERT_EFI_ERROR (Status);

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = Legacy8259->SetMode (Legacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Clear the error flag; thunk code may set it. Stack16 should be the high address.
  // Make Stack16 address the low 16 bit must be not zero.
  //
  Stack16 = (UINT16 *)((UINT8 *)ThunkContext->RealModeBuffer + ThunkContext->RealModeBufferSize - sizeof (UINT16));

  if ((Stack != NULL) && (StackSize != 0)) {
    //
    // Copy Stack to low memory stack
    //
    Stack16 -= StackSize / sizeof (UINT16);
    CopyMem (Stack16, Stack, StackSize);
  }

  ThunkRegSet.E.SS  = (UINT16)(((UINTN)Stack16 >> 16) << 12);
  ThunkRegSet.E.ESP = (UINT16)(UINTN)Stack16;
  ThunkRegSet.E.CS  = Segment;
  ThunkRegSet.E.Eip = Offset;

  ThunkContext->RealModeState = &ThunkRegSet;
  AsmThunk16 (ThunkContext);

  //
  // EFI is likely trashed if we get here, but attempt to restore state.
  //
  if ((Stack != NULL) && (StackSize != 0)) {
    //
    // Copy low memory stack to Stack
    //
    CopyMem (Stack, Stack16, StackSize);
  }

  //
  // Restore protected mode interrupt state
  //
  Status = Legacy8259->SetMode (Legacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  ThunkContext->RealModeState = NULL;

  //
  // Enable and restore rate of DXE Timer
  //
  if (TimerProtocol != NULL) {
    TimerProtocol->SetTimerPeriod (TimerProtocol, TimerPeriod);
  }

  //
  // End critical section
  //
  SetInterruptState (Enabled);

  Regs->E.EDI = ThunkRegSet.E.EDI;
  Regs->E.ESI = ThunkRegSet.E.ESI;
  Regs->E.EBP = ThunkRegSet.E.EBP;
  Regs->E.EBX = ThunkRegSet.E.EBX;
  Regs->E.EDX = ThunkRegSet.E.EDX;
  Regs->E.ECX = ThunkRegSet.E.ECX;
  Regs->E.EAX = ThunkRegSet.E.EAX;
  Regs->E.SS  = ThunkRegSet.E.SS;
  Regs->E.CS  = ThunkRegSet.E.CS;
  Regs->E.DS  = ThunkRegSet.E.DS;
  Regs->E.ES  = ThunkRegSet.E.ES;

  CopyMem (&(Regs->E.EFLAGS), &(ThunkRegSet.E.EFLAGS), sizeof (UINT32));

  Ret = (BOOLEAN)(Regs->E.EFLAGS.Bits.CF == 1);

  return Ret;
}

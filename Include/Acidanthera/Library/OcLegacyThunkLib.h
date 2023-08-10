/** @file
  This library implements thunking to legacy 16-bit environment.

  Copyright (c) 2023, Goldfish64. All rights reserved.<BR>
  Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef OC_LEGACY_THUNK_LIB_H
#define OC_LEGACY_THUNK_LIB_H

#include <Protocol/Legacy8259.h>

//
// Legacy region base is 0x0C0000.
//
#define LEGACY_REGION_BASE  0x0C0000
#define LEGACY_REGION_SIZE  0x10000

#define EFI_SEGMENT(_Adr)  (UINT16) ((UINT16) (((UINTN) (_Adr)) >> 4) & 0xf000)
#define EFI_OFFSET(_Adr)   (UINT16) (((UINT16) ((UINTN) (_Adr))) & 0xffff)

/**
  Initialize legacy environment for BIOS INI caller.

  @param ThunkContext   The instance pointer of THUNK_CONTEXT.
**/
EFI_STATUS
OcLegacyThunkInitializeBiosIntCaller (
  IN OUT THUNK_CONTEXT  *ThunkContext
  );

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
  IN EFI_LEGACY_8259_PROTOCOL  *Legacy8259
  );

/**
   Disconnect all EFI graphics device handles in preparation for calling to legacy mode.
**/
VOID
OcLegacyThunkDisconnectEfiGraphics (
  VOID
  );

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
  );

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
  );

#endif // OC_LEGACY_THUNK_LIB_H

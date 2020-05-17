/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  CpuDxe.h

Abstract:

--*/
#ifndef _CPU_DXE_H
#define _CPU_DXE_H

#include <Protocol/Cpu.h>
#include <Protocol/Legacy8259.h>
#include <Protocol/LegacyBios.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define CPU_EXCEPTION_DEBUG_OUTPUT   0
#define CPU_EXCEPTION_VGA_SWITCH     1

#define INTERRUPT_VECTOR_NUMBER 0x100

//
// Function declarations
//
EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE                       ImageHandle,
  IN EFI_SYSTEM_TABLE                 *SystemTable
  );

EFI_STATUS
EFIAPI
CpuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL           *This,
  IN EFI_PHYSICAL_ADDRESS            Start,
  IN UINT64                          Length,
  IN EFI_CPU_FLUSH_TYPE              FlushType
  );

EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL           *This
  );

EFI_STATUS
EFIAPI
CpuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL           *This
  );

EFI_STATUS
EFIAPI
CpuGetInterruptState (
  IN  EFI_CPU_ARCH_PROTOCOL         *This,
  OUT BOOLEAN                       *State
  );

EFI_STATUS
EFIAPI
CpuInit (
  IN EFI_CPU_ARCH_PROTOCOL          *This,
  IN EFI_CPU_INIT_TYPE               InitType
  );

EFI_STATUS
EFIAPI
CpuRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL          *This,
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  );

EFI_STATUS
EFIAPI
CpuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL          *This,
  IN  UINT32                         TimerIndex,
  OUT UINT64                         *TimerValue,
  OUT UINT64                         *TimerPeriod   OPTIONAL
  );

EFI_STATUS
EFIAPI
CpuSetMemoryAttributes(
  IN  EFI_CPU_ARCH_PROTOCOL           *This,
  IN  EFI_PHYSICAL_ADDRESS            BaseAddress,
  IN  UINT64                          Length,
  IN  UINT64                          Attributes
  );

VOID
EFIAPI
InstallInterruptHandler (
  UINTN Vector,
  VOID  (EFIAPI *Handler)(VOID)
  );

VOID
EFIAPI
SystemExceptionHandler (
  VOID
  );

VOID
EFIAPI
SystemTimerHandler (
  VOID
  );

VOID
EFIAPI
InitDescriptor (
  VOID
  );

BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  UINT8                           BiosInt,
  IN  EFI_IA32_REGISTER_SET           *Regs
  );

#endif

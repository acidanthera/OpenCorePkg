/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  CpuDxe.c

Abstract:

--*/

#include "CpuDxe.h"

//
// Global Variables
//

UINT32 CONST                         mExceptionCodeSize = 9;
BOOLEAN                              mInterruptState = FALSE;
UINTN                                mTimerVector = 0;
volatile EFI_CPU_INTERRUPT_HANDLER   mTimerHandler = NULL;
EFI_LEGACY_8259_PROTOCOL             *gLegacy8259 = NULL;
THUNK_CONTEXT                        mThunkContext;

VOID
InitializeBiosIntCaller (
  VOID
  );

//
// The Cpu Architectural Protocol that this Driver produces
//
EFI_HANDLE              mHandle = NULL;
EFI_CPU_ARCH_PROTOCOL   mCpu = {
  CpuFlushCpuDataCache,        ///< Used in LightMemoryTest
  CpuEnableInterrupt,          ///< Not-used in LegacyBoot
  CpuDisableInterrupt,
  CpuGetInterruptState,
  CpuInit,
  CpuRegisterInterruptHandler, ///< Used in 8254Timer, HPETTimer,
  CpuGetTimerValue,
  CpuSetMemoryAttributes,
  1,                           ///< NumberOfTimers
  4,                           ///< DmaBufferAlignment
};

EFI_STATUS
EFIAPI
CpuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL           *This,
  IN EFI_PHYSICAL_ADDRESS            Start,
  IN UINT64                          Length,
  IN EFI_CPU_FLUSH_TYPE              FlushType
  )
/*++

Routine Description:
  Flush CPU data cache. If the instruction cache is fully coherent
  with all DMA operations then function can just return EFI_SUCCESS.

Arguments:
  This                - Protocol instance structure
  Start               - Physical address to start flushing from.
  Length              - Number of bytes to flush. Round up to chipset
                         granularity.
  FlushType           - Specifies the type of flush operation to perform.

Returns:

  EFI_SUCCESS           - If cache was flushed
  EFI_UNSUPPORTED       - If flush type is not supported.
  EFI_DEVICE_ERROR      - If requested range could not be flushed.

--*/
{
  if (FlushType == EfiCpuFlushTypeWriteBackInvalidate) {
    AsmWbinvd ();
    return EFI_SUCCESS;
  } else if (FlushType == EfiCpuFlushTypeInvalidate) {
    AsmInvd ();
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}


EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
/*++

Routine Description:
  Enables CPU interrupts.

Arguments:
  This                - Protocol instance structure

Returns:
  EFI_SUCCESS           - If interrupts were enabled in the CPU
  EFI_DEVICE_ERROR      - If interrupts could not be enabled on the CPU.

--*/
{
  EnableInterrupts ();

  mInterruptState  = TRUE;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
CpuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
/*++

Routine Description:
  Disables CPU interrupts.

Arguments:
  This                - Protocol instance structure

Returns:
  EFI_SUCCESS           - If interrupts were disabled in the CPU.
  EFI_DEVICE_ERROR      - If interrupts could not be disabled on the CPU.

--*/
{
  DisableInterrupts ();

  mInterruptState = FALSE;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
CpuGetInterruptState (
  IN  EFI_CPU_ARCH_PROTOCOL         *This,
  OUT BOOLEAN                       *State
  )
/*++

Routine Description:
  Return the state of interrupts.

Arguments:
  This                - Protocol instance structure
  State               - Pointer to the CPU's current interrupt state

Returns:
  EFI_SUCCESS           - If interrupts were disabled in the CPU.
  EFI_INVALID_PARAMETER - State is NULL.

--*/
{
  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *State = mInterruptState;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
CpuInit (
  IN EFI_CPU_ARCH_PROTOCOL           *This,
  IN EFI_CPU_INIT_TYPE               InitType
  )

/*++

Routine Description:
  Generates an INIT to the CPU

Arguments:
  This                - Protocol instance structure
  InitType            - Type of CPU INIT to perform

Returns:
  EFI_SUCCESS           - If CPU INIT occurred. This value should never be
        seen.
  EFI_DEVICE_ERROR      - If CPU INIT failed.
  EFI_NOT_SUPPORTED     - Requested type of CPU INIT not supported.

--*/
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
CpuRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL          *This,
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
/*++

Routine Description:
  Registers a function to be called from the CPU interrupt handler.

Arguments:
  This                - Protocol instance structure
  InterruptType       - Defines which interrupt to hook. IA-32 valid range
                         is 0x00 through 0xFF
  InterruptHandler    - A pointer to a function of type
      EFI_CPU_INTERRUPT_HANDLER that is called when a
      processor interrupt occurs. A null pointer
      is an error condition.

Returns:
  EFI_SUCCESS           - If handler installed or uninstalled.
  EFI_ALREADY_STARTED   - InterruptHandler is not NULL, and a handler for
        InterruptType was previously installed
  EFI_INVALID_PARAMETER - InterruptHandler is NULL, and a handler for
        InterruptType was not previously installed.
  EFI_UNSUPPORTED       - The interrupt specified by InterruptType is not
        supported.

--*/
{
  if ((InterruptType < 0) || (InterruptType >= INTERRUPT_VECTOR_NUMBER)) {
    return EFI_UNSUPPORTED;
  }
  if ((UINTN)(UINT32)InterruptType != mTimerVector) {
    return EFI_UNSUPPORTED;
  }
  if ((mTimerHandler == NULL) && (InterruptHandler == NULL)) {
    return EFI_INVALID_PARAMETER;
  } else if ((mTimerHandler != NULL) && (InterruptHandler != NULL)) {
    return EFI_ALREADY_STARTED;
  }
  mTimerHandler = InterruptHandler;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CpuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL          *This,
  IN  UINT32                         TimerIndex,
  OUT UINT64                         *TimerValue,
  OUT UINT64                         *TimerPeriod   OPTIONAL
  )
/*++

Routine Description:
  Returns a timer value from one of the CPU's internal timers. There is no
  inherent time interval between ticks but is a function of the CPU
  frequency.

Arguments:
  This                - Protocol instance structure
  TimerIndex          - Specifies which CPU timer ie requested
  TimerValue          - Pointer to the returned timer value
  TimerPeriod         -

Returns:
  EFI_SUCCESS           - If the CPU timer count was returned.
  EFI_UNSUPPORTED       - If the CPU does not have any readable timers
  EFI_DEVICE_ERROR      - If an error occurred reading the timer.
  EFI_INVALID_PARAMETER - TimerIndex is not valid

--*/
{
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (TimerIndex == 0) {
    *TimerValue = AsmReadTsc ();
    if (TimerPeriod != NULL) {
      //
      // BugBug: Hard coded. Don't know how to do this generically
      //
      *TimerPeriod = 1000000000;
    }
    return EFI_SUCCESS;
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
/*++

Routine Description:
  Set memory cacheability attributes for given range of memory

Arguments:
  This                - Protocol instance structure
  BaseAddress         - Specifies the start address of the memory range
  Length              - Specifies the length of the memory range
  Attributes          - The memory cacheability for the memory range

Returns:
  EFI_SUCCESS           - If the cacheability of that memory range is set successfully
  EFI_UNSUPPORTED       - If the desired operation cannot be done
  EFI_INVALID_PARAMETER - The input parameter is not correct, such as Length = 0

--*/
{
  return EFI_UNSUPPORTED;
}

#if CPU_EXCEPTION_DEBUG_OUTPUT
VOID
DumpExceptionDataDebugOut (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINT32        ErrorCodeFlag;

  ErrorCodeFlag = 0x00027d00;

#ifdef MDE_CPU_IA32
  DEBUG ((
    EFI_D_ERROR,
    "!!!! IA32 Exception Type - %08x !!!!\n",
    InterruptType
    ));
  DEBUG ((
    EFI_D_ERROR,
    "EIP - %08x, CS - %08x, EFLAGS - %08x\n",
    SystemContext.SystemContextIa32->Eip,
    SystemContext.SystemContextIa32->Cs,
    SystemContext.SystemContextIa32->Eflags
    ));
  if (ErrorCodeFlag & (1 << InterruptType)) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %08x\n",
      SystemContext.SystemContextIa32->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "EAX - %08x, ECX - %08x, EDX - %08x, EBX - %08x\n",
    SystemContext.SystemContextIa32->Eax,
    SystemContext.SystemContextIa32->Ecx,
    SystemContext.SystemContextIa32->Edx,
    SystemContext.SystemContextIa32->Ebx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "ESP - %08x, EBP - %08x, ESI - %08x, EDI - %08x\n",
    SystemContext.SystemContextIa32->Esp,
    SystemContext.SystemContextIa32->Ebp,
    SystemContext.SystemContextIa32->Esi,
    SystemContext.SystemContextIa32->Edi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DS - %08x, ES - %08x, FS - %08x, GS - %08x, SS - %08x\n",
    SystemContext.SystemContextIa32->Ds,
    SystemContext.SystemContextIa32->Es,
    SystemContext.SystemContextIa32->Fs,
    SystemContext.SystemContextIa32->Gs,
    SystemContext.SystemContextIa32->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDTR - %08x %08x, IDTR - %08x %08x\n",
    SystemContext.SystemContextIa32->Gdtr[0],
    SystemContext.SystemContextIa32->Gdtr[1],
    SystemContext.SystemContextIa32->Idtr[0],
    SystemContext.SystemContextIa32->Idtr[1]
    ));
  DEBUG ((
    EFI_D_ERROR,
    "LDTR - %08x, TR - %08x\n",
    SystemContext.SystemContextIa32->Ldtr,
    SystemContext.SystemContextIa32->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %08x, CR2 - %08x, CR3 - %08x, CR4 - %08x\n",
    SystemContext.SystemContextIa32->Cr0,
    SystemContext.SystemContextIa32->Cr2,
    SystemContext.SystemContextIa32->Cr3,
    SystemContext.SystemContextIa32->Cr4
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %08x, DR1 - %08x, DR2 - %08x, DR3 - %08x\n",
    SystemContext.SystemContextIa32->Dr0,
    SystemContext.SystemContextIa32->Dr1,
    SystemContext.SystemContextIa32->Dr2,
    SystemContext.SystemContextIa32->Dr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR6 - %08x, DR7 - %08x\n",
    SystemContext.SystemContextIa32->Dr6,
    SystemContext.SystemContextIa32->Dr7
    ));
#else
  DEBUG ((
    EFI_D_ERROR,
    "!!!! X64 Exception Type - %016lx !!!!\n",
    (UINT64)InterruptType
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RIP - %016lx, CS - %016lx, RFLAGS - %016lx\n",
    SystemContext.SystemContextX64->Rip,
    SystemContext.SystemContextX64->Cs,
    SystemContext.SystemContextX64->Rflags
    ));
  if (ErrorCodeFlag & (1 << InterruptType)) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %016lx\n",
      SystemContext.SystemContextX64->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "RAX - %016lx, RCX - %016lx, RDX - %016lx\n",
    SystemContext.SystemContextX64->Rax,
    SystemContext.SystemContextX64->Rcx,
    SystemContext.SystemContextX64->Rdx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RBX - %016lx, RSP - %016lx, RBP - %016lx\n",
    SystemContext.SystemContextX64->Rbx,
    SystemContext.SystemContextX64->Rsp,
    SystemContext.SystemContextX64->Rbp
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RSI - %016lx, RDI - %016lx\n",
    SystemContext.SystemContextX64->Rsi,
    SystemContext.SystemContextX64->Rdi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R8 - %016lx, R9 - %016lx, R10 - %016lx\n",
    SystemContext.SystemContextX64->R8,
    SystemContext.SystemContextX64->R9,
    SystemContext.SystemContextX64->R10
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R11 - %016lx, R12 - %016lx, R13 - %016lx\n",
    SystemContext.SystemContextX64->R11,
    SystemContext.SystemContextX64->R12,
    SystemContext.SystemContextX64->R13
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R14 - %016lx, R15 - %016lx\n",
    SystemContext.SystemContextX64->R14,
    SystemContext.SystemContextX64->R15
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DS - %016lx, ES - %016lx, FS - %016lx\n",
    SystemContext.SystemContextX64->Ds,
    SystemContext.SystemContextX64->Es,
    SystemContext.SystemContextX64->Fs
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GS - %016lx, SS - %016lx\n",
    SystemContext.SystemContextX64->Gs,
    SystemContext.SystemContextX64->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDTR - %016lx %016lx, LDTR - %016lx\n",
    SystemContext.SystemContextX64->Gdtr[0],
    SystemContext.SystemContextX64->Gdtr[1],
    SystemContext.SystemContextX64->Ldtr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "IDTR - %016lx %016lx, TR - %016lx\n",
    SystemContext.SystemContextX64->Idtr[0],
    SystemContext.SystemContextX64->Idtr[1],
    SystemContext.SystemContextX64->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %016lx, CR2 - %016lx, CR3 - %016lx\n",
    SystemContext.SystemContextX64->Cr0,
    SystemContext.SystemContextX64->Cr2,
    SystemContext.SystemContextX64->Cr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR4 - %016lx, CR8 - %016lx\n",
    SystemContext.SystemContextX64->Cr4,
    SystemContext.SystemContextX64->Cr8
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %016lx, DR1 - %016lx, DR2 - %016lx\n",
    SystemContext.SystemContextX64->Dr0,
    SystemContext.SystemContextX64->Dr1,
    SystemContext.SystemContextX64->Dr2
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR3 - %016lx, DR6 - %016lx, DR7 - %016lx\n",
    SystemContext.SystemContextX64->Dr3,
    SystemContext.SystemContextX64->Dr6,
    SystemContext.SystemContextX64->Dr7
    ));
#endif
  return ;
}
#endif

STATIC
CHAR16*
DumpMemoryVgaOut (
  IN CHAR16* VideoBuffer,
  IN UINTN ColumnMax,
  IN UINTN MemoryAddress,
  IN UINTN Count
  )
{
  UINT32        Space;
  UINT32        InLine;
  CHAR16        *Line;
  UINT8 CONST   *Data;

  Space = 8U;
  InLine = 4U;
  Line = VideoBuffer;
  Data = (UINT8 CONST*) MemoryAddress;

  while (Count) {
    UnicodeSPrintAsciiFormat (
      VideoBuffer,
      3U * sizeof (CHAR16),
      "%02x",
      (UINTN) *Data
      );
    VideoBuffer += 2;
    --Space;
    if (!Space) {
      *(CHAR8*) VideoBuffer = ' ';
      ++VideoBuffer;
      Space = 8U;
      --InLine;
      if (!InLine) {
        Line += ColumnMax;
        VideoBuffer = Line;
        InLine = 4U;
      }
    }
    ++Data;
    --Count;
  }
  if (VideoBuffer != Line) {
    Line += ColumnMax;
    VideoBuffer = Line;
  }
  return VideoBuffer;
}

VOID
DumpExceptionDataVgaOut (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINTN         COLUMN_MAX;
  UINTN         ROW_MAX;
  UINT32        ErrorCodeFlag;
  CHAR16        *VideoBufferBase;
  CHAR16        *VideoBuffer;
  UINTN         Index;

  COLUMN_MAX      = 80;
  ROW_MAX         = 25;
  ErrorCodeFlag   = 0x00027d00;
  VideoBufferBase = (CHAR16 *) (UINTN) 0xb8000;
  VideoBuffer     = (CHAR16 *) (UINTN) 0xb8000;

#ifdef MDE_CPU_IA32
  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "!!!! IA32 Exception Type - %08x !!!!",
    InterruptType
    );
  VideoBuffer += COLUMN_MAX;
  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "EIP - %08x, CS - %08x, EFLAGS - %08x",
    SystemContext.SystemContextIa32->Eip,
    SystemContext.SystemContextIa32->Cs,
    SystemContext.SystemContextIa32->Eflags
    );
  VideoBuffer += COLUMN_MAX;
  if (ErrorCodeFlag & (1 << InterruptType)) {
    UnicodeSPrintAsciiFormat (
      VideoBuffer,
      COLUMN_MAX * sizeof (CHAR16),
      "ExceptionData - %08x",
      SystemContext.SystemContextIa32->ExceptionData
    );
    VideoBuffer += COLUMN_MAX;
  }
  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "EAX - %08x, ECX - %08x, EDX - %08x, EBX - %08x",
    SystemContext.SystemContextIa32->Eax,
    SystemContext.SystemContextIa32->Ecx,
    SystemContext.SystemContextIa32->Edx,
    SystemContext.SystemContextIa32->Ebx
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "ESP - %08x, EBP - %08x, ESI - %08x, EDI - %08x",
    SystemContext.SystemContextIa32->Esp,
    SystemContext.SystemContextIa32->Ebp,
    SystemContext.SystemContextIa32->Esi,
    SystemContext.SystemContextIa32->Edi
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "DS - %08x, ES - %08x, FS - %08x, GS - %08x, SS - %08x",
    SystemContext.SystemContextIa32->Ds,
    SystemContext.SystemContextIa32->Es,
    SystemContext.SystemContextIa32->Fs,
    SystemContext.SystemContextIa32->Gs,
    SystemContext.SystemContextIa32->Ss
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "GDTR - %08x %08x, IDTR - %08x %08x",
    SystemContext.SystemContextIa32->Gdtr[0],
    SystemContext.SystemContextIa32->Gdtr[1],
    SystemContext.SystemContextIa32->Idtr[0],
    SystemContext.SystemContextIa32->Idtr[1]
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "LDTR - %08x, TR - %08x",
    SystemContext.SystemContextIa32->Ldtr,
    SystemContext.SystemContextIa32->Tr
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "CR0 - %08x, CR2 - %08x, CR3 - %08x, CR4 - %08x",
    SystemContext.SystemContextIa32->Cr0,
    SystemContext.SystemContextIa32->Cr2,
    SystemContext.SystemContextIa32->Cr3,
    SystemContext.SystemContextIa32->Cr4
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "DR0 - %08x, DR1 - %08x, DR2 - %08x, DR3 - %08x",
    SystemContext.SystemContextIa32->Dr0,
    SystemContext.SystemContextIa32->Dr1,
    SystemContext.SystemContextIa32->Dr2,
    SystemContext.SystemContextIa32->Dr3
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "DR6 - %08x, DR7 - %08x",
    SystemContext.SystemContextIa32->Dr6,
    SystemContext.SystemContextIa32->Dr7
    );
  VideoBuffer += COLUMN_MAX;
  VideoBuffer = DumpMemoryVgaOut (
    VideoBuffer,
    COLUMN_MAX,
    (UINTN) SystemContext.SystemContextIa32->Esp,
    128U
    );
  VideoBuffer = DumpMemoryVgaOut (
    VideoBuffer,
    COLUMN_MAX,
    (UINTN) SystemContext.SystemContextIa32->Eip - 64U,
    128U
    );
#else
  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "!!!! X64 Exception Type - %016lx !!!!",
    (UINT64)InterruptType
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "RIP - %016lx, CS - %016lx, RFLAGS - %016lx",
    SystemContext.SystemContextX64->Rip,
    SystemContext.SystemContextX64->Cs,
    SystemContext.SystemContextX64->Rflags
    );
  VideoBuffer += COLUMN_MAX;

  if (ErrorCodeFlag & (1 << InterruptType)) {
    UnicodeSPrintAsciiFormat (
      VideoBuffer,
      COLUMN_MAX * sizeof (CHAR16),
      "ExceptionData - %016lx",
      SystemContext.SystemContextX64->ExceptionData
      );
    VideoBuffer += COLUMN_MAX;
  }

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "RAX - %016lx, RCX - %016lx, RDX - %016lx",
    SystemContext.SystemContextX64->Rax,
    SystemContext.SystemContextX64->Rcx,
    SystemContext.SystemContextX64->Rdx
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "RBX - %016lx, RSP - %016lx, RBP - %016lx",
    SystemContext.SystemContextX64->Rbx,
    SystemContext.SystemContextX64->Rsp,
    SystemContext.SystemContextX64->Rbp
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "RSI - %016lx, RDI - %016lx",
    SystemContext.SystemContextX64->Rsi,
    SystemContext.SystemContextX64->Rdi
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "R8 - %016lx, R9 - %016lx, R10 - %016lx",
    SystemContext.SystemContextX64->R8,
    SystemContext.SystemContextX64->R9,
    SystemContext.SystemContextX64->R10
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "R11 - %016lx, R12 - %016lx, R13 - %016lx",
    SystemContext.SystemContextX64->R11,
    SystemContext.SystemContextX64->R12,
    SystemContext.SystemContextX64->R13
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "R14 - %016lx, R15 - %016lx",
    SystemContext.SystemContextX64->R14,
    SystemContext.SystemContextX64->R15
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "DS - %016lx, ES - %016lx, FS - %016lx",
    SystemContext.SystemContextX64->Ds,
    SystemContext.SystemContextX64->Es,
    SystemContext.SystemContextX64->Fs
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "GS - %016lx, SS - %016lx",
    SystemContext.SystemContextX64->Gs,
    SystemContext.SystemContextX64->Ss
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "GDTR - %016lx %016lx, LDTR - %016lx",
    SystemContext.SystemContextX64->Gdtr[0],
    SystemContext.SystemContextX64->Gdtr[1],
    SystemContext.SystemContextX64->Ldtr
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "IDTR - %016lx %016lx, TR - %016lx",
    SystemContext.SystemContextX64->Idtr[0],
    SystemContext.SystemContextX64->Idtr[1],
    SystemContext.SystemContextX64->Tr
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "CR0 - %016lx, CR2 - %016lx, CR3 - %016lx",
    SystemContext.SystemContextX64->Cr0,
    SystemContext.SystemContextX64->Cr2,
    SystemContext.SystemContextX64->Cr3
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "CR4 - %016lx, CR8 - %016lx",
    SystemContext.SystemContextX64->Cr4,
    SystemContext.SystemContextX64->Cr8
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "DR0 - %016lx, DR1 - %016lx, DR2 - %016lx",
    SystemContext.SystemContextX64->Dr0,
    SystemContext.SystemContextX64->Dr1,
    SystemContext.SystemContextX64->Dr2
    );
  VideoBuffer += COLUMN_MAX;

  UnicodeSPrintAsciiFormat (
    VideoBuffer,
    COLUMN_MAX * sizeof (CHAR16),
    "DR3 - %016lx, DR6 - %016lx, DR7 - %016lx",
    SystemContext.SystemContextX64->Dr3,
    SystemContext.SystemContextX64->Dr6,
    SystemContext.SystemContextX64->Dr7
    );
  VideoBuffer += COLUMN_MAX;
  VideoBuffer = DumpMemoryVgaOut (
    VideoBuffer,
    COLUMN_MAX,
    (UINTN) SystemContext.SystemContextX64->Rsp,
    128U
    );
  VideoBuffer = DumpMemoryVgaOut (
    VideoBuffer,
    COLUMN_MAX,
    (UINTN) SystemContext.SystemContextX64->Rip - 64U,
    128U
    );
#endif

  for (Index = 0; Index < COLUMN_MAX * ROW_MAX; Index ++) {
    if (Index > (UINTN)(VideoBuffer - VideoBufferBase)) {
      VideoBufferBase[Index] = 0x0c20;
    } else {
      VideoBufferBase[Index] |= 0x0c00;
    }
  }

  return ;
}

#if CPU_EXCEPTION_VGA_SWITCH
UINT16
SwitchVideoMode (
  UINT16    NewVideoMode
  )
/*++
Description
  Switch Video Mode from current mode to new mode, and return the old mode.
  Use Thuink

Arguments
  NewVideoMode - new video mode want to set

Return
  UINT16       - (UINT16) -1 indicates failure
                 Other value indicates the old mode, which can be used for restore later

--*/
{
  EFI_IA32_REGISTER_SET           Regs;
  UINT16                          OriginalVideoMode = (UINT16) -1;


  //
  // Set new video mode
  //
  if (NewVideoMode < 0x100) {
    //
    // Set the 80x25 Text VGA Mode: Assume successful always
    //
    // VIDEO - SET VIDEO MODE
    // AH = 00h
    // AL = desired video mode (see #0009)
    // Return:AL = video mode flag (Phoenix, AMI BIOS)
    // 20h mode > 7
    // 30h modes 0-5 and 7
    // 3Fh mode 6
    // AL = CRT controller mode byte (Phoenix 386 BIOS v1.10)
    //
    gBS->SetMem (&Regs, sizeof (Regs), 0);
    Regs.H.AH = 0x00;
    Regs.H.AL = (UINT8) NewVideoMode;
    LegacyBiosInt86 (0x10, &Regs);

    //
    // VIDEO - TEXT-MODE CHARGEN - LOAD ROM 8x16 CHARACTER SET (VGA)
    // AX = 1114h
    // BL = block to load
    // Return:Nothing
    //
    gBS->SetMem (&Regs, sizeof (Regs), 0);
    Regs.H.AH = 0x11;
    Regs.H.AL = 0x14;
    Regs.H.BL = 0;
    LegacyBiosInt86 (0x10, &Regs);
  } else {
    //
    //    VESA SuperVGA BIOS - SET SuperVGA VIDEO MODE
    //    AX = 4F02h
    //    BX = mode (see #0082,#0083)
    //    bit 15 set means don't clear video memory
    //    bit 14 set means enable linear framebuffer mode (VBE v2.0+)
    //    Return:AL = 4Fh if function supported
    //    AH = status
    //    00h successful
    //    01h failed
    //
    gBS->SetMem (&Regs, sizeof (Regs), 0);
    Regs.X.AX = 0x4F02;
    Regs.X.BX = NewVideoMode;
    LegacyBiosInt86 (0x10, &Regs);
    if (Regs.X.AX != 0x004F) {
      //
      // SORRY: Cannot set to video mode!
      //
      return (UINT16) -1;
    }
  }

  return OriginalVideoMode;
}
#endif

VOID
EFIAPI
ExceptionHandler (
  IN EFI_EXCEPTION_TYPE    InterruptType,
  IN EFI_SYSTEM_CONTEXT    SystemContext
  )
{
#if CPU_EXCEPTION_VGA_SWITCH
  UINT16                          VideoMode;
#endif

#if CPU_EXCEPTION_DEBUG_OUTPUT
  DumpExceptionDataDebugOut (InterruptType, SystemContext);
#endif

#if CPU_EXCEPTION_VGA_SWITCH
  //
  // Switch to text mode for RED-SCREEN output
  //
  VideoMode = SwitchVideoMode (0x83);
#endif

  DumpExceptionDataVgaOut (InterruptType, SystemContext);

  //
  // Use this macro to hang so that the compiler does not optimize out
  // the following RET instructions. This allows us to return if we
  // have a debugger attached.
  //
  CpuDeadLoop ();

#if CPU_EXCEPTION_VGA_SWITCH
  //
  // Switch back to the old video mode
  //
  if (VideoMode != (UINT16)-1) {
    SwitchVideoMode (VideoMode);
  }
#endif

  return ;
}

VOID
EFIAPI
TimerHandler (
  IN EFI_EXCEPTION_TYPE    InterruptType,
  IN EFI_SYSTEM_CONTEXT    SystemContext
  )
{
  if (mTimerHandler != NULL) {
    mTimerHandler (InterruptType, SystemContext);
  }
}

EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  Initialize the state information for the CPU Architectural Protocol

Arguments:
  ImageHandle of the loaded driver
  Pointer to the System Table

Returns:
  EFI_SUCCESS           - thread can be successfully created
  EFI_OUT_OF_RESOURCES  - cannot allocate protocol data structure
  EFI_DEVICE_ERROR      - cannot create the thread

--*/
{
  EFI_STATUS                  Status;
  EFI_8259_IRQ                Irq;
  UINT32                      InterruptVector;

  //
  // Find the Legacy8259 protocol.
  //
  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &gLegacy8259);
  ASSERT_EFI_ERROR (Status);

  //
  // Get the interrupt vector number corresponding to IRQ0 from the 8259 driver
  //
  Status = gLegacy8259->GetVector (gLegacy8259, Efi8259Irq0, (UINT8 *) &mTimerVector);
  ASSERT_EFI_ERROR (Status);

  //
  // Reload GDT, IDT
  //
  InitDescriptor ();

  //
  // Install Exception Handler (0x00 ~ 0x1F)
  //
  for (InterruptVector = 0; InterruptVector < 0x20; InterruptVector++) {
    InstallInterruptHandler (
      InterruptVector,
      (VOID (EFIAPI *)(VOID)) (UINTN) ((UINTN) SystemExceptionHandler + mExceptionCodeSize * InterruptVector)
      );
  }

  //
  // Install Timer Handler
  //
  InstallInterruptHandler (mTimerVector, SystemTimerHandler);
  ((volatile UINT8 *)(UINTN)&SystemTimerHandler)[3] = (UINT8)mTimerVector;

  //
  // BUGBUG: We add all other interrupt vector
  //
  for (Irq = Efi8259Irq1; Irq <= Efi8259Irq15; Irq++) {
    InterruptVector = 0;
    Status = gLegacy8259->GetVector (gLegacy8259, Irq, (UINT8 *) &InterruptVector);
    ASSERT_EFI_ERROR (Status);
    InstallInterruptHandler (InterruptVector, SystemTimerHandler);
  }

  InitializeBiosIntCaller();

  //
  // Install CPU Architectural Protocol and the thunk protocol
  //
  mHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiCpuArchProtocolGuid,
                  &mCpu,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

VOID
InitializeBiosIntCaller (
  VOID
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
  LegacyRegionBase = 0x0C0000;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(LegacyRegionSize),
                  &LegacyRegionBase
                  );
  ASSERT_EFI_ERROR (Status);

  mThunkContext.RealModeBuffer     = (VOID*)(UINTN)LegacyRegionBase;
  mThunkContext.RealModeBufferSize = LegacyRegionSize;
  mThunkContext.ThunkAttributes    = 3;
  AsmPrepareThunk16(&mThunkContext);

}

BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  UINT8                           BiosInt,
  IN  EFI_IA32_REGISTER_SET           *Regs
  )
{
  UINTN                 Status;
  BOOLEAN               InterruptsEnabled;
  IA32_REGISTER_SET     ThunkRegSet;
  BOOLEAN               Ret;
  UINT16                *Stack16;

  if (!gLegacy8259 || !mThunkContext.RealModeBuffer) {
    return FALSE;
  }
  Regs->X.Flags.Reserved1 = 1;
  Regs->X.Flags.Reserved2 = 0;
  Regs->X.Flags.Reserved3 = 0;
  Regs->X.Flags.Reserved4 = 0;
  Regs->X.Flags.IOPL      = 3;
  Regs->X.Flags.NT        = 0;
  Regs->X.Flags.IF        = 1;
  Regs->X.Flags.TF        = 0;
  Regs->X.Flags.CF        = 0;

  ZeroMem (&ThunkRegSet, sizeof (ThunkRegSet));
  ThunkRegSet.E.EDI  = Regs->E.EDI;
  ThunkRegSet.E.ESI  = Regs->E.ESI;
  ThunkRegSet.E.EBP  = Regs->E.EBP;
  ThunkRegSet.E.EBX  = Regs->E.EBX;
  ThunkRegSet.E.EDX  = Regs->E.EDX;
  ThunkRegSet.E.ECX  = Regs->E.ECX;
  ThunkRegSet.E.EAX  = Regs->E.EAX;
  ThunkRegSet.E.DS   = Regs->E.DS;
  ThunkRegSet.E.ES   = Regs->E.ES;

  CopyMem (&(ThunkRegSet.E.EFLAGS), &(Regs->E.EFlags), sizeof (UINT32));

  //
  // The call to Legacy16 is a critical section to EFI
  //
  InterruptsEnabled = SaveAndDisableInterrupts ();

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = gLegacy8259->SetMode (gLegacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  Stack16 = (UINT16 *)((UINT8 *) mThunkContext.RealModeBuffer + mThunkContext.RealModeBufferSize - sizeof (UINT16));
  Stack16 -= sizeof (ThunkRegSet.E.EFLAGS) / sizeof (UINT16);
  CopyMem (Stack16, &ThunkRegSet.E.EFLAGS, sizeof (ThunkRegSet.E.EFLAGS));

  ThunkRegSet.E.SS   = (UINT16) (((UINTN) Stack16 >> 16) << 12);
  ThunkRegSet.E.ESP  = (UINT16) (UINTN) Stack16;
  ThunkRegSet.E.Eip  = (UINT16)((volatile UINT32 *)NULL)[BiosInt];
  ThunkRegSet.E.CS   = (UINT16)(((volatile UINT32 *)NULL)[BiosInt] >> 16);

  mThunkContext.RealModeState = &ThunkRegSet;
  AsmThunk16 (&mThunkContext);

  //
  // Restore protected mode interrupt state
  //
  Status = gLegacy8259->SetMode (gLegacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // End critical section
  //
  SetInterruptState (InterruptsEnabled);

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

  CopyMem (&(Regs->E.EFlags), &(ThunkRegSet.E.EFLAGS), sizeof (UINT32));

  Ret = (BOOLEAN) (Regs->E.EFlags.CF == 1);

  return Ret;
}

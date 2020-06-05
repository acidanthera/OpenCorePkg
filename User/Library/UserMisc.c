/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>

VOID
EFIAPI
CpuBreakpoint (
  VOID
  )
{
  ASSERT (FALSE);
  while (TRUE);
}

VOID
EFIAPI
CpuPause (
  VOID
  )
{
}

VOID
EFIAPI
DisableInterrupts (
  VOID
  )
{
}

VOID
EFIAPI
EnableInterrupts (
  VOID
  )
{
}

UINT32
AsmCpuid (
  UINT32 Index,
  UINT32 *Eax,
  UINT32 *Ebx,
  UINT32 *Ecx,
  UINT32 *Edx
  )
{
  UINT32 eax = 0, ebx = 0, ecx = 0, edx = 0;

  asm ("cpuid\n"
       : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
       : "0" (Index));

  if (Eax) *Eax = eax;
  if (Ebx) *Ebx = ebx;
  if (Ecx) *Ecx = ecx;
  if (Edx) *Edx = edx;

  return Index;
}

UINT32
AsmCpuidEx (
  IN      UINT32                    Index,
  IN      UINT32                    SubIndex,
  OUT     UINT32                    *Eax,  OPTIONAL
  OUT     UINT32                    *Ebx,  OPTIONAL
  OUT     UINT32                    *Ecx,  OPTIONAL
  OUT     UINT32                    *Edx   OPTIONAL
  )
{
  UINT32 eax = 0, ebx = 0, ecx = 0, edx = 0;

  asm ("cpuid\n"
       : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
       : "0" (Index), "2" (SubIndex));

  if (Eax) *Eax = eax;
  if (Ebx) *Ebx = ebx;
  if (Ecx) *Ecx = ecx;
  if (Edx) *Edx = edx;

  return Index;
}

UINT32
EFIAPI
AsmIncrementUint32 (
  IN volatile UINT32  *Value
  )
{
  ASSERT (FALSE);

  return 0;
}

UINT32
EFIAPI
AsmReadIntelMicrocodeRevision (
  VOID
  )
{
  return 0;
}

UINTN
EFIAPI
AsmReadCr4 (
  VOID
  )
{
  return 0;
}

UINT16
EFIAPI
AsmReadCs (
  VOID
  )
{
  return 3;
}

UINTN
EFIAPI
AsmReadEflags (
  VOID
  )
{
  return 0;
}

UINT64
EFIAPI
AsmReadMsr64 (
  IN      UINT32                    Index
  )
{
  return 0;
}

UINT64
EFIAPI
AsmReadTsc (
  VOID
  )
{
  return 0;
}

UINTN
EFIAPI
AsmWriteCr4 (
  UINTN  Cr4
  )
{
  return 0;
}

UINT64
EFIAPI
AsmWriteMsr64 (
  IN      UINT32                    Index,
  IN      UINT64                    Value
  )
{
  return 0;
}

UINT64
EFIAPI
AsmMsrAndThenOr64 (
  IN      UINT32                    Index,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  )
{
  //
  // MSRs cannot be read at userspace level.
  //
  return 0;
}

VOID
EFIAPI
AsmDisableCache (
  VOID
  )
{
}

VOID
EFIAPI
AsmEnableCache (
  VOID
  )
{
}

VOID
EFIAPI
CpuFlushTlb (
  VOID
  )
{
}

UINT32
EFIAPI
IoRead32 (
  IN      UINTN                     Port
  )
{
  return 0;
}

UINT16
EFIAPI
MmioAnd16 (
  IN      UINTN                     Address,
  IN      UINT16                    AndData
  )
{
  return 0;
}

UINT32
EFIAPI
MmioAnd32 (
  IN      UINTN                     Address,
  IN      UINT16                    AndData
  )
{
  return 0;
}

UINT8
EFIAPI
MmioAnd8 (
  IN      UINTN                     Address,
  IN      UINT8                     AndData
  )
{
  return 0;
}

UINT16
EFIAPI
MmioAndThenOr16 (
  IN      UINTN                     Address,
  IN      UINT16                    AndData,
  IN      UINT16                    OrData
  )
{
  return 0;
}

UINT32
EFIAPI
MmioAndThenOr32 (
  IN      UINTN                     Address,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  )
{
  return 0;
}

UINT8
EFIAPI
MmioAndThenOr8 (
  IN      UINTN                     Address,
  IN      UINT8                     AndData,
  IN      UINT8                     OrData
  )
{
  return 0;
}

UINT16
EFIAPI
MmioBitFieldAnd16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData
  )
{
  return 0;
}

UINT32
EFIAPI
MmioBitFieldAnd32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData
  )
{
  return 0;
}

UINT8
EFIAPI
MmioBitFieldAnd8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData
  )
{
  return 0;
}

UINT16
EFIAPI
MmioBitFieldAndThenOr16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData,
  IN      UINT16                    OrData
  )
{
  return 0;
}

UINT32
EFIAPI
MmioBitFieldAndThenOr32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  )
{
  return 0;
}

UINT8
EFIAPI
MmioBitFieldAndThenOr8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData,
  IN      UINT8                     OrData
  )
{
  return 0;
}

UINT16
EFIAPI
MmioBitFieldOr16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    OrData
  )
{
  return 0;
}

UINT32
EFIAPI
MmioBitFieldOr32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    OrData
  )
{
  return 0;
}

UINT8
EFIAPI
MmioBitFieldOr8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     OrData
  )
{
  return 0;
}

UINT16
EFIAPI
MmioBitFieldRead16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  return 0;
}

UINT32
EFIAPI
MmioBitFieldRead32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  return 0;
}

UINT8
EFIAPI
MmioBitFieldRead8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  return 0;
}

UINT16
EFIAPI
MmioBitFieldWrite16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    Value
  )
{
  return 0;
}

UINT32
EFIAPI
MmioBitFieldWrite32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    Value
  )
{
  return 0;
}

UINT8
EFIAPI
MmioBitFieldWrite8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     Value
  )
{
  return 0;
}

UINT16
EFIAPI
MmioOr16 (
  IN      UINTN                     Address,
  IN      UINT16                    OrData
  )
{
  return 0;
}

UINT32
EFIAPI
MmioOr32 (
  IN      UINTN                     Address,
  IN      UINT32                    OrData
  )
{
  return 0;
}

UINT8
EFIAPI
MmioOr8 (
  IN      UINTN                     Address,
  IN      UINT8                     OrData
  )
{
  return 0;
}

UINT16
EFIAPI
MmioRead16 (
  IN      UINTN                     Address
  )
{
  return 0;
}

UINT32
EFIAPI
MmioRead32 (
  IN      UINTN                     Address
  )
{
  return 0;
}

UINT8
EFIAPI
MmioRead8 (
  IN      UINTN                     Address
  )
{
  return 0;
}

UINT16
EFIAPI
MmioWrite16 (
  IN      UINTN                     Address,
  IN      UINT16                    Value
  )
{
  return 0;
}

UINT32
EFIAPI
MmioWrite32 (
  IN      UINTN                     Address,
  IN      UINT32                    Value
  )
{
  return 0;
}

UINT8
EFIAPI
MmioWrite8 (
  IN      UINTN                     Address,
  IN      UINT8                     Value
  )
{
  return 0;
}

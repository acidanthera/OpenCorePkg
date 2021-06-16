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
  IN      UINT32                    Index,
  OUT     UINT32                    *Eax,  OPTIONAL
  OUT     UINT32                    *Ebx,  OPTIONAL
  OUT     UINT32                    *Ecx,  OPTIONAL
  OUT     UINT32                    *Edx   OPTIONAL
  )
{
  #if defined(__i386__) || defined(__x86_64__)
  UINT32  EaxVal;
  UINT32  EbxVal;
  UINT32  EcxVal;
  UINT32  EdxVal;

  EaxVal  = 0;
  EbxVal  = 0;
  EcxVal  = 0;
  EdxVal  = 0;

  asm (
    "cpuid\n"
    : "=a" (EaxVal), "=b" (EbxVal), "=c" (EcxVal), "=d" (EdxVal)
    : "0" (Index)
    );

  if (Eax != NULL) {
    *Eax = EaxVal;
  }
  if (Ebx != NULL) {
    *Ebx = EbxVal;
  }
  if (Ecx != NULL) {
    *Ecx = EcxVal;
  }
  if (Edx != NULL) {
    *Edx = EdxVal;
  }

  return Index;
  #else
  if (Eax != NULL) {
    *Eax = 0;
  }
  if (Ebx != NULL) {
    *Ebx = 0;
  }
  if (Ecx != NULL) {
    *Ecx = 0;
  }
  if (Edx != NULL) {
    *Edx = 0;
  }

  return 0;
  #endif
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
  #if defined(__i386__) || defined(__x86_64__)
  UINT32  EaxVal;
  UINT32  EbxVal;
  UINT32  EcxVal;
  UINT32  EdxVal;

  EaxVal  = 0;
  EbxVal  = 0;
  EcxVal  = 0;
  EdxVal  = 0;

  asm (
    "cpuid\n"
    : "=a" (EaxVal), "=b" (EbxVal), "=c" (EcxVal), "=d" (EdxVal)
    : "0"  (Index),
    "2"    (SubIndex)
    );

  if (Eax != NULL) {
    *Eax = EaxVal;
  }
  if (Ebx != NULL) {
    *Ebx = EbxVal;
  }
  if (Ecx != NULL) {
    *Ecx = EcxVal;
  }
  if (Edx != NULL) {
    *Edx = EdxVal;
  }

  return Index;
  #else
  if (Eax != NULL) {
    *Eax = 0;
  }
  if (Ebx != NULL) {
    *Ebx = 0;
  }
  if (Ecx != NULL) {
    *Ecx = 0;
  }
  if (Edx != NULL) {
    *Edx = 0;
  }

  return 0;
  #endif
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

UINT8
EFIAPI
IoRead8 (
  IN      UINTN                     Port
  )
{
  return 0;
}

UINT8
EFIAPI
IoWrite8 (
  IN      UINTN                     Port,
  IN      UINT8                     Value
  )
{
  (void) Port;
  return Value;
}

UINT32
EFIAPI
IoRead32 (
  IN      UINTN                     Port
  )
{
  ASSERT ((Port & 3) == 0);
  return 0;
}

UINT32
EFIAPI
IoWrite32 (
  IN      UINTN                     Port,
  IN      UINT32                    Value
  )
{
  ASSERT ((Port & 3) == 0);
  return Value;
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

VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID         *Guid
  )
{
  return NULL;
}

EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
DevicePathFromHandle (
  IN EFI_HANDLE                      Handle
  )
{
  return NULL;
}

EFI_FILE_PROTOCOL *
LocateRootVolume (
  IN  EFI_HANDLE                         DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath     OPTIONAL
  )
{
  return NULL;
}

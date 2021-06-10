/** @file
  Copyright (C) 2021, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <IndustryStandard/ProcessorInfo.h>
#include <Register/Intel/ArchitecturalMsr.h>
#include <Register/Intel/Msr/AtomMsr.h>
#include <Register/Intel/Msr/CoreMsr.h>
#include <Register/Intel/Msr/Core2Msr.h>
#include <Register/Intel/Msr/NehalemMsr.h>
#include <Register/Intel/Msr/GoldmontMsr.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/OcGuardLib.h>

//
// NOTE: This function is copied from OcAudioLib.
// TODO: Move to OcMiscLib.
//
STATIC
VOID
EFIAPI
PrintBuffer (
  IN OUT CHAR8        **AsciiBuffer,
  IN     UINTN        *AsciiBufferSize,
  IN     CONST CHAR8  *FormatString,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Marker;
  CHAR8       Tmp[256];
  CHAR8       *NewBuffer;
  UINTN       NewBufferSize;

  if (*AsciiBuffer == NULL) {
    return;
  }

  VA_START (Marker, FormatString);
  AsciiVSPrint (Tmp, sizeof (Tmp), FormatString, Marker);
  VA_END (Marker);

  Status = AsciiStrCatS (*AsciiBuffer, *AsciiBufferSize, Tmp);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    if (OcOverflowMulUN (*AsciiBufferSize, 2, &NewBufferSize)) {
      return;
    }
    NewBuffer = ReallocatePool (*AsciiBufferSize, NewBufferSize, *AsciiBuffer);
    if (NewBuffer == NULL) {
      FreePool (*AsciiBuffer);

      *AsciiBuffer     = NULL;
      *AsciiBufferSize = 0;

      return;
    }

    *AsciiBuffer      = NewBuffer;
    *AsciiBufferSize  = NewBufferSize;

    AsciiStrCatS (*AsciiBuffer, *AsciiBufferSize, Tmp);
  }
}

EFI_STATUS
OcCpuDump (
  IN OC_CPU_INFO        *CpuInfo,
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS         Status;
  OC_CPU_MSR_REPORT  Report;

  CHAR8              *FileBuffer;
  UINTN              FileBufferSize;
  CHAR16             TmpFileName[32];

  ASSERT (CpuInfo != NULL);
  ASSERT (Root != NULL);

  //
  // The CPU model must be Intel.
  //
  if (CpuInfo->Vendor[0] != CPUID_VENDOR_INTEL) {
    return EFI_UNSUPPORTED;
  }

  FileBufferSize = SIZE_1KB;
  FileBuffer     = AllocateZeroPool (FileBufferSize);
  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Ref: https://github.com/acidanthera/bugtracker/issues/1580
  //
  if (CpuInfo->Model <= CPU_MODEL_PENRYN) {
    //
    // Before Penryn, the following registers are read:
    // IA32_MISC_ENABLE
    // MSR_IA32_EXT_CONFIG
    // MSR_FSB_FREQ
    // MSR_IA32_PERF_STATUS
    //
    OcCpuGetMsrReport (MSR_IA32_MISC_ENABLES, &Report);
    if (Report.CpuHasMsr) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_MISC_ENABLES: %llX\n");
    }

    OcCpuGetMsrReport (MSR_IA32_EXT_CONFIG, &Report);
    if (Report.CpuHasMsr) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_EXT_CONFIG: %llX\n");
    }

    OcCpuGetMsrReport (MSR_FSB_FREQ, &Report);
    if (Report.CpuHasMsr) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_FSB_FREQ: %llX\n");
    }

    OcCpuGetMsrReport (MSR_IA32_PERF_STATUS, &Report);
    if (Report.CpuHasMsr) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_PERF_STATUS: %llX\n");
    }
  } else {
    //
    // Afterwards, the following registers are read:
    // MSR_PLATFORM_INFO
    // MSR_TURBO_RATIO_LIMIT
    // MSR_PKG_POWER_INFO (To be confirmed)
    //
    OcCpuGetMsrReport (MSR_NEHALEM_PLATFORM_INFO, &Report);
    if (Report.CpuHasMsr) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PLATFORM_INFO: %llX\n");
    }

    //
    // NOTE: On Haswell-E there are also
    //       MSR_TURBO_RATIO_LIMIT1 and MSR_TURBO_RATIO_LIMIT2, which represent
    //       0x1AE and 0x1AF respectively.
    //
    OcCpuGetMsrReport (MSR_NEHALEM_TURBO_RATIO_LIMIT, &Report);
    if (Report.CpuHasMsr) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_TURBO_RATIO_LIMIT: %llX\n");
    }

    OcCpuGetMsrReport (MSR_GOLDMONT_PKG_POWER_INFO, &Report);
    if (Report.CpuHasMsr) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PKG_POWER_INFO: %llX\n");
    }
  }

  //
  // Save dumped controller data to file.
  //
  if (FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"MSRStatus.txt");
    Status = SetFileData (Root, TmpFileName, FileBuffer, (UINT32) AsciiStrLen (FileBuffer));
    DEBUG ((DEBUG_INFO, "OCAU: Dumped MSR value - %r\n", Status));

    FreePool (FileBuffer);
  }

  return EFI_SUCCESS;
}

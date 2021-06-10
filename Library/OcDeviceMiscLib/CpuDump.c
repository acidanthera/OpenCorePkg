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

  FileBufferSize = SIZE_1KB;
  FileBuffer     = AllocateZeroPool (FileBufferSize);
  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OcCpuGetMsrReport (CpuInfo, &Report);

  //
  // MSR_PLATFORM_INFO
  //
  if (Report.CpuHasMsrPlatformInfo) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PLATFORM_INFO: %llX\n", Report.CpuMsrPlatformInfoValue);
  }
  //
  // MSR_TURBO_RATIO_LIMIT
  //
  if (Report.CpuHasMsrTurboRatioLimit) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_TURBO_RATIO_LIMIT: %llX\n", Report.CpuMsrTurboRatioLimitValue);
  }
  //
  // MSR_PKG_POWER_INFO (TODO: To be confirmed)
  //
  if (Report.CpuHasMsrPkgPowerInfo) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PKG_POWER_INFO: %llX\n", Report.CpuMsrPkgPowerInfoValue);
  }

  //
  // IA32_MISC_ENABLE
  //
  if (Report.CpuHasMsrIa32MiscEnable) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "IA32_MISC_ENABLE: %llX\n", Report.CpuMsrIa32MiscEnableValue);
  }
  //
  // MSR_IA32_EXT_CONFIG
  //
  if (Report.CpuHasMsrIa32ExtConfig) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_EXT_CONFIG: %llX\n", Report.CpuMsrIa32ExtConfigValue);
  }
  //
  // MSR_FSB_FREQ
  //
  if (Report.CpuHasMsrFsbFreq) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_FSB_FREQ: %llX\n", Report.CpuMsrFsbFreqValue);
  }
  //
  // MSR_IA32_PERF_STATUS
  //
  if (Report.CpuHasMsrIa32PerfStatus) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_PERF_STATUS: %llX\n", Report.CpuMsrIa32PerfStatusValue);
  }

  //
  // Save dumped MSR data to file.
  //
  if (FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"MSRStatus.txt");
    Status = SetFileData (Root, TmpFileName, FileBuffer, (UINT32) AsciiStrLen (FileBuffer));
    DEBUG ((DEBUG_INFO, "OCAU: Dumped MSR data - %r\n", Status));

    FreePool (FileBuffer);
  }

  return EFI_SUCCESS;
}

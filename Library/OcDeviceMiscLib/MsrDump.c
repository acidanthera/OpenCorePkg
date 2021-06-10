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
OcMsrDump (
  IN OC_CPU_INFO        *CpuInfo,
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS         Status;
  OC_CPU_MSR_REPORT  *Reports;
  UINTN              EntryCount;
  UINTN              Index;

  CHAR8              *FileBuffer;
  UINTN              FileBufferSize;
  CHAR16             TmpFileName[32];

  ASSERT (CpuInfo != NULL);
  ASSERT (Root    != NULL);

  FileBufferSize = SIZE_1KB;
  FileBuffer     = (CHAR8 *) AllocateZeroPool (FileBufferSize);
  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Reports = OcCpuGetMsrReports (CpuInfo, &EntryCount);
  if (Reports == NULL) {
    return EFI_UNSUPPORTED;
  }

  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "CPU BrandString: %a\nMicrocodeRevision: 0x%08X\nVirtualization %d\n",
    CpuInfo->BrandString,
    CpuInfo->MicrocodeRevision,
    CpuInfo->Hypervisor
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "CPUID %08X %08X %08X %08X, ExtSigEcx %08X ExtSigEdx %08X\n",
    CpuInfo->CpuidVerEax.Uint32,
    CpuInfo->CpuidVerEbx.Uint32,
    CpuInfo->CpuidVerEcx.Uint32,
    CpuInfo->CpuidVerEdx.Uint32,
    CpuInfo->CpuidExtSigEcx.Uint32,
    CpuInfo->CpuidExtSigEdx.Uint32
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "Type 0x%02X, Family 0x%02X, Model 0x%02X, ExtModel 0x%02X, ExtFamily 0x%02X, Stepping 0x%02X\n",
    CpuInfo->Type,
    CpuInfo->Family,
    CpuInfo->Model,
    CpuInfo->ExtModel,
    CpuInfo->ExtFamily,
    CpuInfo->Stepping
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "Features 0x%16LX, ExtFeatures 0x%16LX, Signature 0x%08X\n",
    CpuInfo->Features,
    CpuInfo->ExtFeatures,
    CpuInfo->Signature
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "Brand 0x%02X, AppleProcessorType 0x%04X, CstConfigLock %d, CpuGeneration %d\n",
    CpuInfo->Brand,
    CpuInfo->AppleProcessorType,
    CpuInfo->CstConfigLock,
    CpuInfo->CpuGeneration
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "MaxId 0x%08X, MaxExtId 0x%08X\n",
    CpuInfo->MaxId,
    CpuInfo->MaxExtId
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "PackageCount %u\nCoreCount %u\nThreadCount %u\n",
    CpuInfo->PackageCount,
    CpuInfo->CoreCount,
    CpuInfo->ThreadCount
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "ExternalClock %u\n",
    CpuInfo->ExternalClock
    );
  PrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "ARTFrequency %Lu\nCPUFrequency %Lu\nCPUFrequencyFromTSC %Lu\nCPUFrequencyFromART %Lu\nTscAdjust %Lu\nCPUFrequencyFromApple %Lu\nCPUFrequencyFromVMT %Lu\nFSBFrequency %Lu\n",
    CpuInfo->ARTFrequency,
    CpuInfo->CPUFrequency,
    CpuInfo->CPUFrequencyFromTSC,
    CpuInfo->CPUFrequencyFromART,
    CpuInfo->TscAdjust,
    CpuInfo->CPUFrequencyFromApple,
    CpuInfo->CPUFrequencyFromVMT,
    CpuInfo->FSBFrequency
    );

  for (Index = 0; Index < EntryCount; ++Index) {
    PrintBuffer (&FileBuffer, &FileBufferSize, "CPU%02d:\n", Index);

    //
    // MSR_PLATFORM_INFO
    //
    if (Reports[Index].CpuHasMsrPlatformInfo) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PLATFORM_INFO: 0x%llX\n", Reports[Index].CpuMsrPlatformInfoValue);
    }
    //
    // MSR_TURBO_RATIO_LIMIT
    //
    if (Reports[Index].CpuHasMsrTurboRatioLimit) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_TURBO_RATIO_LIMIT: 0x%llX\n", Reports[Index].CpuMsrTurboRatioLimitValue);
    }
    //
    // MSR_PKG_POWER_INFO (TODO: To be confirmed)
    //
    if (Reports[Index].CpuHasMsrPkgPowerInfo) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PKG_POWER_INFO: 0x%llX\n", Reports[Index].CpuMsrPkgPowerInfoValue);
    }

    //
    // MSR_BROADWELL_PKG_CST_CONFIG_CONTROL_REGISTER (0xE2)
    //
    if (Reports[Index].CpuHasMsrE2) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_BROADWELL_PKG_CST_CONFIG_CONTROL_REGISTER (0xE2): 0x%llX\n", Reports[Index].CpuMsrPkgPowerInfoValue);
    }

    //
    // IA32_MISC_ENABLE
    //
    if (Reports[Index].CpuHasMsrIa32MiscEnable) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "IA32_MISC_ENABLE: 0x%llX\n", Reports[Index].CpuMsrIa32MiscEnableValue);
    }
    //
    // MSR_IA32_EXT_CONFIG
    //
    if (Reports[Index].CpuHasMsrIa32ExtConfig) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_EXT_CONFIG: 0x%llX\n", Reports[Index].CpuMsrIa32ExtConfigValue);
    }
    //
    // MSR_FSB_FREQ
    //
    if (Reports[Index].CpuHasMsrFsbFreq) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_FSB_FREQ: 0x%llX\n", Reports[Index].CpuMsrFsbFreqValue);
    }
    //
    // MSR_IA32_PERF_STATUS
    //
    if (Reports[Index].CpuHasMsrIa32PerfStatus) {
      PrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_PERF_STATUS: 0x%llX\n", Reports[Index].CpuMsrIa32PerfStatusValue);
    }

    PrintBuffer (&FileBuffer, &FileBufferSize, "\n");
  }

  //
  // Save dumped MSR data to file.
  //
  if (FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"MSRStatus.txt");
    Status = SetFileData (Root, TmpFileName, FileBuffer, (UINT32) AsciiStrLen (FileBuffer));
    DEBUG ((DEBUG_INFO, "OCDM: Dumped MSR data - %r\n", Status));

    FreePool (FileBuffer);
  }

  FreePool (Reports);

  return EFI_SUCCESS;
}

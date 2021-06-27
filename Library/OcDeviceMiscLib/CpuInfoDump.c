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
#include <Library/OcStringLib.h>

EFI_STATUS
OcCpuInfoDump (
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

  //
  // Firstly, dump basic CPU info.
  //
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "CPU BrandString: %a\nMicrocodeRevision: 0x%08X\nVirtualization %d\n",
    CpuInfo->BrandString,
    CpuInfo->MicrocodeRevision,
    CpuInfo->Hypervisor
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "CPUID %08X %08X %08X %08X\nExtSigEcx %08X\nExtSigEdx %08X\n",
    CpuInfo->CpuidVerEax.Uint32,
    CpuInfo->CpuidVerEbx.Uint32,
    CpuInfo->CpuidVerEcx.Uint32,
    CpuInfo->CpuidVerEdx.Uint32,
    CpuInfo->CpuidExtSigEcx.Uint32,
    CpuInfo->CpuidExtSigEdx.Uint32
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "Type 0x%02X\nFamily 0x%02X\nModel 0x%02X\nExtModel 0x%02X\nExtFamily 0x%02X\nStepping 0x%02X\n",
    CpuInfo->Type,
    CpuInfo->Family,
    CpuInfo->Model,
    CpuInfo->ExtModel,
    CpuInfo->ExtFamily,
    CpuInfo->Stepping
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "Features 0x%16LX\nExtFeatures 0x%16LX\nSignature 0x%08X\n",
    CpuInfo->Features,
    CpuInfo->ExtFeatures,
    CpuInfo->Signature
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "Brand 0x%02X\nAppleProcessorType 0x%04X\nCstConfigLock %d\nCpuGeneration %d\n",
    CpuInfo->Brand,
    CpuInfo->AppleProcessorType,
    CpuInfo->CstConfigLock,
    CpuInfo->CpuGeneration
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "MaxId 0x%08X\nMaxExtId 0x%08X\n",
    CpuInfo->MaxId,
    CpuInfo->MaxExtId
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "PackageCount %u\nCoreCount %u\nThreadCount %u\n",
    CpuInfo->PackageCount,
    CpuInfo->CoreCount,
    CpuInfo->ThreadCount
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "ExternalClock %u\n",
    CpuInfo->ExternalClock
    );
  OcAsciiPrintBuffer (
    &FileBuffer,
    &FileBufferSize,
    "ARTFrequency %Lu\nCPUFrequency %Lu\nCPUFrequencyFromTSC %Lu\nCPUFrequencyFromART %Lu\nTscAdjust %Lu\nCPUFrequencyFromApple %Lu\nCPUFrequencyFromVMT %Lu\nFSBFrequency %Lu\n\n",
    CpuInfo->ARTFrequency,
    CpuInfo->CPUFrequency,
    CpuInfo->CPUFrequencyFromTSC,
    CpuInfo->CPUFrequencyFromART,
    CpuInfo->TscAdjust,
    CpuInfo->CPUFrequencyFromApple,
    CpuInfo->CPUFrequencyFromVMT,
    CpuInfo->FSBFrequency
    );

  //
  // Then, get reports of MSR status.
  //
  Reports = OcCpuGetMsrReports (CpuInfo, &EntryCount);
  if (Reports == NULL) {
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < EntryCount; ++Index) {
    OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "CPU%02u:\n", Index);

    //
    // MSR_PLATFORM_INFO
    //
    if (Reports[Index].CpuHasMsrPlatformInfo) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PLATFORM_INFO: 0x%llX\n", Reports[Index].CpuMsrPlatformInfoValue);
    }
    //
    // MSR_TURBO_RATIO_LIMIT
    //
    if (Reports[Index].CpuHasMsrTurboRatioLimit) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_TURBO_RATIO_LIMIT: 0x%llX\n", Reports[Index].CpuMsrTurboRatioLimitValue);
    }
    //
    // MSR_PKG_POWER_INFO (TODO: To be confirmed)
    //
    if (Reports[Index].CpuHasMsrPkgPowerInfo) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_PKG_POWER_INFO: 0x%llX\n", Reports[Index].CpuMsrPkgPowerInfoValue);
    }

    //
    // MSR_BROADWELL_PKG_CST_CONFIG_CONTROL_REGISTER (0xE2)
    //
    if (Reports[Index].CpuHasMsrE2) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_BROADWELL_PKG_CST_CONFIG_CONTROL_REGISTER (0xE2): 0x%llX\n", Reports[Index].CpuMsrPkgPowerInfoValue);
    }

    //
    // MSR_IA32_MISC_ENABLE
    //
    if (Reports[Index].CpuHasMsrIa32MiscEnable) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_MISC_ENABLE: 0x%llX\n", Reports[Index].CpuMsrIa32MiscEnableValue);
    }
    //
    // MSR_IA32_EXT_CONFIG
    //
    if (Reports[Index].CpuHasMsrIa32ExtConfig) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_EXT_CONFIG: 0x%llX\n", Reports[Index].CpuMsrIa32ExtConfigValue);
    }
    //
    // MSR_FSB_FREQ
    //
    if (Reports[Index].CpuHasMsrFsbFreq) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_FSB_FREQ: 0x%llX\n", Reports[Index].CpuMsrFsbFreqValue);
    }
    //
    // MSR_IA32_PERF_STATUS
    //
    if (Reports[Index].CpuHasMsrIa32PerfStatus) {
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "MSR_IA32_PERF_STATUS: 0x%llX\n", Reports[Index].CpuMsrIa32PerfStatusValue);
    }

    OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "\n");
  }

  //
  // Save dumped CPU info to file.
  //
  if (FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"CPUInfo.txt");
    Status = SetFileData (Root, TmpFileName, FileBuffer, (UINT32) AsciiStrLen (FileBuffer));
    DEBUG ((DEBUG_INFO, "OCDM: Dumped CPU info - %r\n", Status));

    FreePool (FileBuffer);
  }

  FreePool (Reports);

  return EFI_SUCCESS;
}

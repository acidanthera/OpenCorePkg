/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_CPU_LIB_H
#define OC_CPU_LIB_H

#include <Uefi.h>
#include <IndustryStandard/CpuId.h>
#include <IndustryStandard/AppleIntelCpuInfo.h>
#include <Protocol/FrameworkMpService.h>
#include <Protocol/MpService.h>

/**
  Assumed CPU frequency when it cannot be detected.
  Can be overridden by e.g. emulator.
**/
#ifndef OC_FALLBACK_CPU_FREQUENCY
#define OC_FALLBACK_CPU_FREQUENCY 1000000000
#endif

/**
  Sorted Intel CPU generations.
**/
typedef enum {
  OcCpuGenerationUnknown,
  OcCpuGenerationBanias,
  OcCpuGenerationPrePenryn,
  OcCpuGenerationPenryn,
  OcCpuGenerationNehalem,
  OcCpuGenerationBonnell,
  OcCpuGenerationWestmere,
  OcCpuGenerationSilvermont,
  OcCpuGenerationSandyBridge,
  OcCpuGenerationPostSandyBridge,
  OcCpuGenerationIvyBridge,
  OcCpuGenerationHaswell,
  OcCpuGenerationBroadwell,
  OcCpuGenerationSkylake,
  OcCpuGenerationKabyLake,
  OcCpuGenerationCoffeeLake,
  OcCpuGenerationCometLake,
  OcCpuGenerationRocketLake,
  OcCpuGenerationCannonLake,
  OcCpuGenerationIceLake,
  OcCpuGenerationTigerLake,
  OcCpuGenerationMaxGeneration
} OC_CPU_GENERATION;

typedef struct {
  //
  // Note, Vendor and BrandString are reordered for proper alignment.
  //
  UINT32                      Vendor[4];
  CHAR8                       BrandString[48];

  CPUID_VERSION_INFO_EAX      CpuidVerEax;
  CPUID_VERSION_INFO_EBX      CpuidVerEbx;
  CPUID_VERSION_INFO_ECX      CpuidVerEcx;
  CPUID_VERSION_INFO_EDX      CpuidVerEdx;

  CPUID_EXTENDED_CPU_SIG_ECX  CpuidExtSigEcx;
  CPUID_EXTENDED_CPU_SIG_EDX  CpuidExtSigEdx;

  UINT32                      MicrocodeRevision;
  BOOLEAN                     Hypervisor;   ///< indicate whether we are under virtualization

  UINT8                       Type;
  UINT8                       Family;
  UINT8                       Model;
  UINT8                       ExtModel;
  UINT8                       ExtFamily;
  UINT8                       Stepping;
  UINT64                      Features;
  UINT64                      ExtFeatures;
  UINT32                      Signature;
  UINT8                       Brand;
  UINT16                      AppleProcessorType;
  BOOLEAN                     CstConfigLock;

  OC_CPU_GENERATION           CpuGeneration;

  UINT32                      MaxId;
  UINT32                      MaxExtId;

  UINT16                      PackageCount;
  UINT16                      CoreCount;
  UINT16                      ThreadCount;

  //
  // External clock for SMBIOS Type4 table.
  //
  UINT16                      ExternalClock;

  //
  // Platform-dependent frequency for the Always Running Timer (ART), normally
  // 24Mhz. The firmware may choose to override this. Some CPUs like Xeon Scalable
  // use a different frequency. CPUs report the frequency through CPUID.15H.ECX.
  // If unreported, the frequency is looked up based on the model and family.
  //
  // Nominal Core Crystal Clock Frequency for known processor families:
  //   Intel Xeon Scalable with CPUID signature 0x0655: 25 Mhz     (server segment)
  //   6th and 7th generation Intel Core & Xeon W:      24 Mhz     (client segment)
  //   Nex Generation Intel Atom with CPUID 0x065C:     19.2 Mhz   (atom segment)
  //
  UINT64                      ARTFrequency;

  //
  // The CPU frequency derived from either CPUFrequencyFromTSC (legacy) or
  // CPUFrequencyFromART (preferred for Skylake and presumably newer processors
  // that have an Always Running Timer).
  //
  // CPUFrequencyFromTSC should approximate equal CPUFrequencyFromART. If not,
  // there is likely a bug or miscalculation.
  //
  UINT64                      CPUFrequency;

  //
  // The CPU frequency as reported by the Time Stamp Counter (TSC).
  //
  UINT64                      CPUFrequencyFromTSC;

  //
  // The CPU frequency derived from the Always Running Timer (ART) frequency:
  //   TSC Freq = (ART Freq * CPUID.15H:EBX[31:0]) / CPUID.15H:EAX[31:0]
  //
  // 0 if ART is not present.
  //
  UINT64                      CPUFrequencyFromART;

  //
  // TSC adjustment value read from MSR_IA32_TSC_ADJUST if present.
  //
  UINT64                      TscAdjust;

  //
  // The CPU frequency derived from Apple Platform Info.
  // 0 if Apple Platform Info is not present.
  //
  UINT64                      CPUFrequencyFromApple;

  //
  // The CPU frequency derived from the CPUID VMWare Timing leaf.
  // 0 if VMWare Timing leaf is not present.
  //
  UINT64                      CPUFrequencyFromVMT;

  //
  // The Front Side Bus (FSB) frequency calculated from dividing the CPU
  // frequency by the Max Ratio.
  //
  UINT64                      FSBFrequency;
} OC_CPU_INFO;

typedef struct {
  //
  // MSR_PLATFORM_INFO
  //
  BOOLEAN                     CpuHasMsrPlatformInfo;
  UINT64                      CpuMsrPlatformInfoValue;

  //
  // MSR_TURBO_RATIO_LIMIT
  //
  BOOLEAN                     CpuHasMsrTurboRatioLimit;
  UINT64                      CpuMsrTurboRatioLimitValue;

  //
  // MSR_PKG_POWER_INFO (TODO: To be confirmed)
  //
  BOOLEAN                     CpuHasMsrPkgPowerInfo;
  UINT64                      CpuMsrPkgPowerInfoValue;

  //
  // IA32_MISC_ENABLE
  //
  BOOLEAN                     CpuHasMsrIa32MiscEnable;
  UINT64                      CpuMsrIa32MiscEnableValue;

  //
  // MSR_IA32_EXT_CONFIG
  //
  BOOLEAN                     CpuHasMsrIa32ExtConfig;
  UINT64                      CpuMsrIa32ExtConfigValue;

  //
  // MSR_FSB_FREQ
  //
  BOOLEAN                     CpuHasMsrFsbFreq;
  UINT64                      CpuMsrFsbFreqValue;

  //
  // MSR_IA32_PERF_STATUS
  //
  BOOLEAN                     CpuHasMsrIa32PerfStatus;
  UINT64                      CpuMsrIa32PerfStatusValue;

  //
  // MSR_BROADWELL_PKG_CST_CONFIG_CONTROL_REGISTER (0xE2)
  //
  BOOLEAN                     CpuHasMsrE2;
  UINT64                      CpuMsrE2Value;
} OC_CPU_MSR_REPORT;

//
// Wrapped structure to be passed as ProcedureArgument to MpServices->StartupAllAPs ().
//
typedef struct {
  //
  // Pointer to MP Services.
  //
  EFI_MP_SERVICES_PROTOCOL  *MpServices;
  //
  // Pointer to CPU MSR report list.
  //
  OC_CPU_MSR_REPORT         *Reports;
  //
  // Pointer to the CPU Info.
  //
  OC_CPU_INFO               *CpuInfo;
} OC_CPU_MSR_REPORT_PROCEDURE_ARGUMENT;

/**
  Scan the processor and fill the cpu info structure with results.

  @param[in,out] Cpu  A pointer to the cpu info structure to fill with results.
**/
VOID
OcCpuScanProcessor (
  IN OUT OC_CPU_INFO  *Cpu
  );

/**
  Get the MSR report of one core on the CPU.

  @param[in]   CpuInfo  A pointer to the cpu info.
  @param[out]  Report   The report generated based on CpuInfo.
**/
VOID
OcCpuGetMsrReport (
  IN  OC_CPU_INFO        *CpuInfo,
  OUT OC_CPU_MSR_REPORT  *Report
  );

/**
 Get the MSR report of a single core on the CPU. Used as a parameter of MpServices->StartupAllAPs ().

 @param[in,out] Buffer  The pointer to private data buffer.
 **/
VOID
EFIAPI
OcCpuGetMsrReportPerCore (
  IN OUT VOID  *Buffer
  );

/**
 Get the MSR reports of all cores on the CPU.

 @param[in]   CpuInfo     A pointer to the cpu info.
 @param[out]  EntryCount  Number of CPU cores.

 @return A list of reports of MSR status at each core that must be freed manually, or NULL on failure.
 **/
OC_CPU_MSR_REPORT *
OcCpuGetMsrReports (
  IN  OC_CPU_INFO        *CpuInfo,
  OUT UINTN              *EntryCount
  );

/**
  Disable flex ratio if it has invalid value.
  Commonly fixes early reboot on APTIO IV (Ivy/Haswell).

  @param[in] Cpu  A pointer to the cpu info.
**/
VOID
OcCpuCorrectFlexRatio (
  IN OC_CPU_INFO  *Cpu
  );

/**
  Synchronise TSC on all cores (needed on server chipsets and some laptops).
  This does not fully replace VoodooTscSync or TSCAdjustReset due to
  the need to sync on S3 as well and may also work far less reliably
  due to the limitation of UEFI firmware not permitting MSR update runs in
  parallel with BSP and AP cores. However, it lets debug kernels work
  most of the time till the time TSC kexts start.

  @param[in]  Cpu       A pointer to the cpu info.
  @param[in]  Timeout   Amount of time to wait for CPU core rendezvous.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcCpuCorrectTscSync (
  IN OC_CPU_INFO  *Cpu,
  IN UINTN        Timeout
  );

/**
  Converts CPUID Family and Model extracted from EAX
  CPUID (1) call to AppleFamily value. This implements
  cpuid_set_cpufamily functionality as it is in XNU.

  @param[in] VersionEax  CPUID (1) EAX value.

  @retval Apple Family (e.g. CPUFAMILY_UNKNOWN)
**/
UINT32
OcCpuModelToAppleFamily (
  IN CPUID_VERSION_INFO_EAX  VersionEax
  );

/**
  Converts calculated CPU frequency in Hz to rounded
  value in MHz.

  @param[in] Frequency  CPU frequency in Hz.

  @return Rounded CPU frequency in MHz.
**/
UINT16
OcCpuFrequencyToDisplayFrequency (
  IN UINT64  Frequency
  );

/**
  Obtain CPU's invariant TSC frequency.

  @retval CPU's TSC frequency or OC_FALLBACK_CPU_FREQUENCY.
**/
UINT64
OcGetTSCFrequency (
  VOID
  );

#endif // OC_CPU_LIB_H_

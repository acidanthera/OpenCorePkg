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

#include <IndustryStandard/CpuId.h>
#include <IndustryStandard/AppleIntelCpuInfo.h>

//
// External clock value on Sandy Bridge and above (in MHz).
// Rounded FSB * 4.
//
#define OC_CPU_SNB_QPB_CLOCK 0x19U

typedef struct {
  //
  // Note, Vendor and BrandString are reordered for proper alignment.
  //
  UINT32                  Vendor[4];
  CHAR8                   BrandString[48];

  CPUID_VERSION_INFO_EAX  CpuidVerEax;
  CPUID_VERSION_INFO_EBX  CpuidVerEbx;
  CPUID_VERSION_INFO_ECX  CpuidVerEcx;
  CPUID_VERSION_INFO_EDX  CpuidVerEdx;

  UINT32                  MicrocodeRevision;

  UINT8                   Type;
  UINT8                   Family;
  UINT8                   Model;
  UINT8                   ExtModel;
  UINT8                   ExtFamily;
  UINT8                   Stepping;
  UINT64                  Features;
  UINT64                  ExtFeatures;
  UINT32                  Signature;
  UINT8                   Brand;
  UINT16                  AppleProcessorType;
  BOOLEAN                 CstConfigLock;

  UINT32                  MaxExtId;

  UINT8                   MaxDiv;
  UINT8                   CurBusRatio;  ///< Current Multiplier
  UINT8                   MinBusRatio;  ///< Min Bus Ratio
  UINT8                   MaxBusRatio;  ///< Max Bus Ratio

  UINT8                   TurboBusRatio1;
  UINT8                   TurboBusRatio2;
  UINT8                   TurboBusRatio3;
  UINT8                   TurboBusRatio4;

  UINT16                  PackageCount;
  UINT16                  CoreCount;
  UINT16                  ThreadCount;

  UINT64                  ARTFrequency;
  UINT64                  TSCFrequency;
  UINT64                  CPUFrequency;
  UINT64                  FSBFrequency;
} OC_CPU_INFO;

/**
  Scan the processor and fill the cpu info structure with results.

  @param[in] Cpu  A pointer to the cpu info structure to fill with results.
**/
VOID
OcCpuScanProcessor (
  IN OUT OC_CPU_INFO  *Cpu
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
  Converts CPUID Family and Model extracted from EAX
  CPUID (1) call to AppleFamily value. This implements
  cpuid_set_cpufamily functionality as it is in XNU.

  @param[in] VersionEax  CPUID (1) EAX value.

  @return Apple Family (e.g. CPUFAMILY_UNKNOWN)
**/
UINT32
OcCpuModelToAppleFamily (
  IN UINT32      VersionEax
  );

#endif // OC_CPU_LIB_H_

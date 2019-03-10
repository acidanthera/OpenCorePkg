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

//
// External clock value on Sandy Bridge and above (in MHz).
// Rounded FSB * 4.
//
#define OC_CPU_SNB_QPB_CLOCK 0x19U

typedef struct {
  //
  // Note, Vendor and BrandString are reordered for proper alignment.
  //
  UINT8                Vendor[16];
  CHAR8                BrandString[48];
  UINT8                Type;

  UINT8                Family;
  UINT8                Model;
  UINT8                ExtModel;
  UINT8                ExtFamily;
  UINT8                Stepping;
  UINT64               Features;
  UINT64               ExtFeatures;
  UINT32               Signature;
  UINT8                Brand;
  UINT16               AppleProcessorType;
  BOOLEAN              CstConfigLock;

  UINT32               MaxExtId;

  UINT8                MaxDiv;
  UINT8                CurBusRatio;  ///< Current Multiplier
  UINT8                MinBusRatio;  ///< Min Bus Ratio
  UINT8                MaxBusRatio;  ///< Max Bus Ratio
  UINT8                MaxBusRatioDiv;

  UINT8                TurboBusRatio1;
  UINT8                TurboBusRatio2;
  UINT8                TurboBusRatio3;
  UINT8                TurboBusRatio4;

  UINT16               PackageCount;
  UINT16               CoreCount;
  UINT16               ThreadCount;

  UINT64               ARTFrequency;
  UINT64               TSCFrequency;
  UINT64               CPUFrequency;
  UINT64               FSBFrequency;
  
} OC_CPU_INFO;

// OcCpuScanProcessor
/** Scan the processor and fill the cpu info structure with results

  @param[in] Cpu  A pointer to the cpu info structure to fill with results
**/
VOID
OcCpuScanProcessor (
  IN OUT OC_CPU_INFO  *Cpu
  );

#endif // OC_CPU_LIB_H_

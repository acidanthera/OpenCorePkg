/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_CPU_INTERNALS_H
#define OC_CPU_INTERNALS_H

/**
  Returns microcode revision for Intel CPUs.

  @retval  microcode revision.
**/
UINT32
EFIAPI
AsmReadIntelMicrocodeRevision (
  VOID
  );

/**
  Detect Apple CPU major type.

  @param[in] BrandString   CPU Brand String from CPUID.

  @retval Apple CPU major type.
**/
UINT8
InternalDetectAppleMajorType (
  IN  CONST CHAR8  *BrandString
  );

/**
  Detect Apple CPU type.

  @param[in] Model           CPU model from CPUID.
  @param[in] Stepping        CPU stepping from CPUID.
  @param[in] AppleMajorType  Apple CPU major type.

  @retval Apple CPU type.
**/
UINT16
InternalDetectAppleProcessorType (
  IN UINT8  Model,
  IN UINT8  Stepping,
  IN UINT8  AppleMajorType
  );

/**
  Calculate the TSC frequency via PM timer

  @param[in] Recalculate  Do not re-use previously cached information.

  @retval  The calculated TSC frequency.
**/
UINT64
InternalCalculateTSCFromPMTimer (
  IN BOOLEAN  Recalculate
  );

/**
  Calculate the ART frequency and derieve the CPU frequency for Intel CPUs

  @param[out] CPUFrequency  The derieved CPU frequency.
  @param[in]  Recalculate   Do not re-use previously cached information.

  @retval  The calculated ART frequency.
**/
UINT64
InternalCalcluateARTFrequencyIntel (
  OUT UINT64   *CPUFrequency,
  IN  BOOLEAN  Recalculate
  );

#endif // OC_CPU_INTERNALS_H

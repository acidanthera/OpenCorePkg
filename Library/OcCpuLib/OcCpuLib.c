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

#include <Uefi.h>

#include <IndustryStandard/CpuId.h>
#include <IndustryStandard/GenericIch.h>
#include <IndustryStandard/Pci.h>

#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/OcCpuLib.h>
#include <Library/PciLib.h>
#include <Library/OcPrintLib.h>
#include <Library/OcTimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Macros.h>

#include "ProcessorInfo.h"

// OcCpuScanProcessor
/** Scan the processor and fill the cpu info structure with results

  @param[in] Cpu  A pointer to the cpu info structure to fill with results

  @retval EFI_SUCCESS  The scan was completed successfully.
**/
EFI_STATUS
OcCpuScanProcessor (
  IN CPU_INFO  *Cpu
  )
{
  EFI_STATUS Status;

  UINT32     CpuidEax;
  UINT32     CpuidEbx;
  UINT32     CpuidEcx;
  UINT32     CpuidEdx;
  UINT64     Msr = 0;

  DEBUG_FUNCTION_ENTRY (DEBUG_VERBOSE);

  Status = EFI_INVALID_PARAMETER;

  if (Cpu != NULL) {

    // Get vendor CPUID 0x00000000
    AsmCpuid (CPUID_SIGNATURE, &CpuidEax, (UINT32 *)Cpu->Vendor, (UINT32 *)(Cpu->Vendor + 8), (UINT32 *)(Cpu->Vendor + 4));

    // Get extended CPUID 0x80000000
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &CpuidEax, &CpuidEbx, &CpuidEcx, &CpuidEdx);

    Cpu->MaxExtId = CpuidEax;

    // Get brand string CPUID 0x80000002 - 0x80000004
    if (Cpu->MaxExtId >= CPUID_BRAND_STRING3) {

      // The brandstring 48 bytes max, guaranteed NULL terminated.
      UINT32	*BrandString = (UINT32 *)Cpu->BrandString;

      AsmCpuid (
        CPUID_BRAND_STRING1,
        BrandString,
        (BrandString + 1),
        (BrandString + 2),
        (BrandString + 3)
        );

      AsmCpuid (
        CPUID_BRAND_STRING2,
        (BrandString + 4),
        (BrandString + 5),
        (BrandString + 6),
        (BrandString + 7)
        );

      AsmCpuid (
        CPUID_BRAND_STRING3,
        (BrandString + 8),
        (BrandString + 9),
        (BrandString + 10),
        (BrandString + 11)
        );
    }

    // Get processor signature and decode
    if (Cpu->MaxExtId >= CPUID_VERSION_INFO) {

      AsmCpuid (CPUID_VERSION_INFO, &CpuidEax, &CpuidEbx, &CpuidEcx, &CpuidEdx);

      Cpu->Signature = CpuidEax;
      Cpu->Stepping  = (UINT8)BITFIELD (CpuidEax, 3, 0);
      Cpu->ExtModel  = (UINT8)BITFIELD (CpuidEax, 19, 16);
      Cpu->Model     = ((UINT8)BITFIELD (CpuidEax, 7, 4) + (Cpu->ExtModel << 4));
      Cpu->Family    = (UINT8)BITFIELD (CpuidEax, 11, 8);
      Cpu->Type      = (UINT8)BITFIELD (CpuidEax, 13, 12);
      Cpu->ExtFamily = (UINT8)BITFIELD (CpuidEax, 27, 20);
      Cpu->Brand     = (UINT8)BITFIELD (CpuidEbx, 7, 0);
      Cpu->Features  = QUAD (CpuidEcx, CpuidEdx);

    }

    LOG ((EFI_D_INFO, "%a %a\n", "Found", Cpu->BrandString));

    DEBUG ((
      DEBUG_INFO,
      "Signature %0X Stepping %0X Model %0X Family %0X Type %0X ExtModel %0X ExtFamily %0X\n",
      Cpu->Signature,
      Cpu->Stepping,
      Cpu->Model,
      Cpu->Family,
      Cpu->Type,
      Cpu->ExtModel,
      Cpu->ExtFamily
      ));

    if (*(UINT32 *)Cpu->Vendor == CPUID_VENDOR_INTEL) {

      Msr = AsmReadMsr64 (MSR_PKG_CST_CONFIG_CONTROL);

      if ((Cpu->Family == 0x06 && Cpu->Model >= 0x0c) ||
          (Cpu->Family == 0x0f && Cpu->Model >= 0x03))
      {

        Msr = AsmReadMsr64 (MSR_IA32_PERF_STATUS);

        if (Cpu->Model >= CPU_MODEL_NEHALEM) {

          Cpu->CurBusRatio = (UINT8)BITFIELD(Msr, 15, 8);

          Msr = AsmReadMsr64 (MSR_PLATFORM_INFO);

          Cpu->MinBusRatio = (UINT8)(RShiftU64 (Msr, 40) & 0xFF);
          Cpu->MaxBusRatio = (UINT8)(RShiftU64 (Msr, 8) & 0xFF);

        } else {

          Cpu->MaxBusRatio = (UINT8)(Msr >> 8) & 0x1f;

          // Non-integer bus ratio for the max-multi
          Cpu->MaxBusRatioDiv = (UINT8)(Msr >> 46) & 0x01;

          // Non-integer bus ratio for the current-multi (undocumented)
          //CurrDiv = (UINT8)(Msr >> 14) & 0x01;

        }

        if ((Cpu->Model != CPU_MODEL_NEHALEM_EX) &&
            (Cpu->Model != CPU_MODEL_WESTMERE_EX))
        {
          Msr = AsmReadMsr64 (MSR_TURBO_RATIO_LIMIT);
          Cpu->TurboBusRatio1 = (UINT8)(RShiftU64 (Msr, MAX_RATIO_LIMIT_1C_OFFSET) & MAX_RATIO_LIMIT_MASK);
          Cpu->TurboBusRatio2 = (UINT8)(RShiftU64 (Msr, MAX_RATIO_LIMIT_2C_OFFSET) & MAX_RATIO_LIMIT_MASK);
          Cpu->TurboBusRatio3 = (UINT8)(RShiftU64 (Msr, MAX_RATIO_LIMIT_3C_OFFSET) & MAX_RATIO_LIMIT_MASK);
          Cpu->TurboBusRatio4 = (UINT8)(RShiftU64 (Msr, MAX_RATIO_LIMIT_4C_OFFSET) & MAX_RATIO_LIMIT_MASK);
      	}

      	DEBUG ((
          DEBUG_INFO,
          "Ratio Min %d Max %d Current %d Turbo %d %d %d %d\n",
          Cpu->MinBusRatio,
          Cpu->MaxBusRatio,
          Cpu->CurBusRatio,
          Cpu->TurboBusRatio1,
          Cpu->TurboBusRatio2,
          Cpu->TurboBusRatio3,
          Cpu->TurboBusRatio4
          ));
      
        // SkyLake and later have an Always Running Timer
        if (Cpu->Model >= CPU_MODEL_SKYLAKE) {

          AsmCpuid (0x15, &CpuidEax, &CpuidEbx, &CpuidEcx, &CpuidEdx);

          if (CpuidEbx && CpuidEbx) {

            Cpu->CPUFrequency = MultU64x32 (BASE_ART_CLOCK_SOURCE, (UINT32)DivU64x32 (CpuidEbx, CpuidEax));

            LOG ((
              EFI_D_INFO,
              "%a %a %11lld %5dMHz %d * %d / %d = %ld\n",
              "ART",
              "Frequency",
              Cpu->CPUFrequency,
              DivU64x32 (Cpu->CPUFrequency, 1000000),
              BASE_ART_CLOCK_SOURCE,
              CpuidEbx,
              CpuidEax,
              Cpu->CPUFrequency
              ));

            Cpu->FSBFrequency = DivU64x32 (Cpu->CPUFrequency, Cpu->MaxBusRatio);

          }
        }

        // Calculate the Tsc frequency
        Cpu->TSCFrequency = CalculateTSC (FALSE);

        if (Cpu->CPUFrequency == 0) {

          LOG ((
            EFI_D_INFO,
            "%a %a %11lld %5dMHz\n",
            "TSC",
            "Frequency",
            Cpu->TSCFrequency,
            DivU64x32 (Cpu->TSCFrequency, 1000000)
            ));

          // Both checked to workaround virtual cpu
          if ((Cpu->MinBusRatio > 0) &&
          	  (Cpu->MaxBusRatio > Cpu->MinBusRatio))
          {
            Cpu->FSBFrequency = DivU64x32 (Cpu->TSCFrequency, Cpu->MaxBusRatio);
            Cpu->CPUFrequency = MultU64x32 (Cpu->FSBFrequency, Cpu->MaxBusRatio);

          } else {

            Cpu->CPUFrequency = Cpu->TSCFrequency;
            Cpu->FSBFrequency = 100000000;
          }
        }

        LOG ((
          EFI_D_INFO,
          "%a %a %11lld %5dMHz\n",
          "CPU",
          "Frequency",
          Cpu->CPUFrequency,
          DivU64x32 (Cpu->CPUFrequency, 1000000)
          ));

        LOG ((
          EFI_D_INFO,
          "%a %a %11lld %5dMHz\n",
          "FSB",
          "Frequency",
          Cpu->FSBFrequency,
          DivU64x32 (Cpu->FSBFrequency, 1000000)
          ));

        }
      }

    Status = EFI_SUCCESS;
  }

  DEBUG_FUNCTION_RETURN (DEBUG_VERBOSE);

  return Status;
}

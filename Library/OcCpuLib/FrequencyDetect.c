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

#include <Guid/OcVariable.h>
#include <Guid/AppleHob.h>
#include <Guid/ApplePlatformInfo.h>
#include <IndustryStandard/CpuId.h>
#include <IndustryStandard/GenericIch.h>
#include <IndustryStandard/McpMemoryController.h>
#include <Protocol/PciIo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/PciLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <IndustryStandard/ProcessorInfo.h>
#include <Register/Msr.h>

#include "OcCpuInternals.h"

UINTN
InternalGetPmTimerAddr (
  OUT CONST CHAR8 **Type  OPTIONAL
  )
{
  UINTN   TimerAddr;
  UINT32  CpuVendor;

  TimerAddr = 0;

  if (Type != NULL) {
    *Type = "Failure";
  }

  //
  // Intel timer support.
  // Here we obtain the address of 24-bit or 32-bit PM1_TMR.
  // TODO: I believe that there is little reason to enforce our timer lib to calculate
  // CPU frequency through ACPI PM timer on modern Intel CPUs. Starting from Skylake
  // we have crystal clock, which allows us to get quite reliable values. Perhaps
  // this code should be put to OcCpuLib, and the best available source is to be used.
  //
  if (PciRead16 (PCI_ICH_LPC_ADDRESS (0)) == V_ICH_PCI_VENDOR_ID) {
    //
    // On legacy platforms PM1_TMR can be found in ACPI I/O space.
    // 1. For platforms prior to Intel Skylake (Sunrisepoint PCH) iTCO watchdog
    //    resources reside in LPC device (D31:F0).
    // 2. For platforms from Intel Skylake till Intel Kaby Lake inclusive they reside in
    //    PMC controller (D31:F2).
    // Checking whether ACPI I/O space is enabled is done via ACPI_CNTL register bit 0.
    //
    // On modern platforms, starting from Intel Coffee Lake, the space is roughly the same,
    // but it is referred to as PMC I/O space, and the addressing is done through BAR2.
    // In addition to that on B360 and friends PMC controller may be just missing.
    //
    if ((PciRead8 (PCI_ICH_LPC_ADDRESS (R_ICH_LPC_ACPI_CNTL)) & B_ICH_LPC_ACPI_CNTL_ACPI_EN) != 0) {
      TimerAddr = (PciRead16 (PCI_ICH_LPC_ADDRESS (R_ICH_LPC_ACPI_BASE)) & B_ICH_LPC_ACPI_BASE_BAR) + R_ACPI_PM1_TMR;
      if (Type != NULL) {
        *Type = "LPC";
      }
    } else if (PciRead16 (PCI_ICH_PMC_ADDRESS (0)) == V_ICH_PCI_VENDOR_ID) {
      if ((PciRead8 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_ACPI_CNTL)) & B_ICH_PMC_ACPI_CNTL_ACPI_EN) != 0) {
        TimerAddr = (PciRead16 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_ACPI_BASE)) & B_ICH_PMC_ACPI_BASE_BAR) + R_ACPI_PM1_TMR;
        if (Type != NULL) {
          *Type = "PMC ACPI";
        }
      } else if ((PciRead16 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_BAR2_BASE)) & B_ICH_PMC_BAR2_BASE_BAR_EN) != 0) {
        TimerAddr = (PciRead16 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_BAR2_BASE)) & B_ICH_PMC_BAR2_BASE_BAR) + R_ACPI_PM1_TMR;
        if (Type != NULL) {
          *Type = "PMC BAR2";
        }
      } else if (Type != NULL) {
        *Type = "Invalid INTEL PMC";
      }
    } else if (Type != NULL) {
      //
      // This is currently the case for Z390 and B360 boards.
      //
      *Type = "Unknown INTEL";
    }
  }

  //
  // AMD timer support.
  //
  if (TimerAddr == 0) {
    //
    // In an ideal world I believe we should detect AMD SMBus controller...
    //
    CpuVendor = 0;
    AsmCpuid (CPUID_SIGNATURE, NULL, &CpuVendor, NULL, NULL);

    if (CpuVendor == CPUID_VENDOR_AMD) {
      TimerAddr = MmioRead32 (
        R_AMD_ACPI_MMIO_BASE + R_AMD_ACPI_MMIO_PMIO_BASE + R_AMD_ACPI_PM_TMR_BLOCK
        );
      if (Type != NULL) {
        *Type = "AMD";
      }
    }
  }

  return TimerAddr;
}

UINT64
InternalCalculateTSCFromPMTimer (
  IN BOOLEAN  Recalculate
  )
{
  //
  // Cache the result to speed up multiple calls. For example, we might need
  // this frequency on module entry to initialise a TimerLib instance, and at
  // a later point in time to gather CPU information.
  //
  STATIC UINT64 TSCFrequency = 0;

  UINT16      TimerAddr;
  UINTN       VariableSize;
  UINT64      TscTicksDelta;
  UINT32      AcpiTick0;
  UINT32      AcpiTick1;
  UINT32      AcpiTicksDelta;
  UINT32      AcpiTicksDuration;
  BOOLEAN     HasInterrupts;
  EFI_TPL     PrevTpl;
  EFI_STATUS  Status;

  //
  // Do not use ACPI PM timer in ring 3 (e.g. emulator).
  //
  if ((AsmReadCs () & 3U) == 3) {
    return EFI_UNSUPPORTED;
  }

  //
  // Decide whether we need to store the frequency.
  //
  if (TSCFrequency == 0) {
    VariableSize = sizeof (TSCFrequency);
    Status = gRT->GetVariable (
      OC_ACPI_CPU_FREQUENCY_VARIABLE_NAME,
      &gOcVendorVariableGuid,
      NULL,
      &VariableSize,
      &TSCFrequency
      );
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  if (Recalculate) {
    TSCFrequency = 0;
  }

  if (TSCFrequency == 0) {
    TimerAddr       = (UINT16) InternalGetPmTimerAddr (NULL);

    if (TimerAddr != 0) {
      //
      // Check that timer is advancing (it does not on some virtual machines).
      //
      AcpiTick0 = IoRead32 (TimerAddr);
      gBS->Stall (500);
      AcpiTick1 = IoRead32 (TimerAddr);

      if (AcpiTick0 != AcpiTick1) {
        //
        // ACPI PM timers are usually of 24-bit length, but there are some less common cases of 32-bit length also.
        // When the maximal number is reached, it overflows.
        // The code below can handle overflow with AcpiTicksTarget of up to 24-bit size,
        // on both available sizes of ACPI PM Timers (24-bit and 32-bit).
        //
        // 357954 clocks of ACPI timer (200ms)
        //
        AcpiTicksDuration = V_ACPI_TMR_FREQUENCY / 10;

        //
        // Disable all events to ensure that nobody interrupts us.
        //
        PrevTpl   = gBS->RaiseTPL (TPL_HIGH_LEVEL);
        HasInterrupts = SaveAndDisableInterrupts ();
        AsmMeasureTicks (AcpiTicksDuration, TimerAddr, &AcpiTicksDelta, &TscTicksDelta);
        if (HasInterrupts) {
          EnableInterrupts ();
        }
        gBS->RestoreTPL (PrevTpl);

        TSCFrequency = DivU64x32 (
          MultU64x32 (TscTicksDelta, V_ACPI_TMR_FREQUENCY), AcpiTicksDelta
          );
      }
    }

    DEBUG ((DEBUG_VERBOSE, "TscFrequency %lld\n", TSCFrequency));

    //
    // Set the variable if not present and valid.
    //
    if (TSCFrequency != 0 && Status == EFI_NOT_FOUND) {
      gRT->SetVariable (
        OC_ACPI_CPU_FREQUENCY_VARIABLE_NAME,
        &gOcVendorVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS,
        sizeof (TSCFrequency),
        &TSCFrequency
        );
    }
  }

  return TSCFrequency;
}

STATIC
UINT64
InternalSelectAppleFsbFrequency (
  IN UINT64  *FsbFrequency,
  IN UINT32  FsbFrequncyCount
  )
{
  UINT32               Pll;

  if (FsbFrequncyCount < 5) {
    return 0 /* Invalid */;
  }

  //
  // Tested on nForce MCP89 installed in MacBook7,1.
  //
  Pll = IoRead32 (R_NVIDIA_MCP89_DDR_PLL);

  DEBUG ((
    DEBUG_INFO,
    "OCCPU: Selecting FSB freq by PLL freq %u from %Lu %Lu %Lu %Lu %Lu\n",
    Pll,
    FsbFrequency[0],
    FsbFrequency[1],
    FsbFrequency[2],
    FsbFrequency[3],
    FsbFrequency[4]
    ));

  Pll /= 1000000;

  if (Pll == 16) {
    return FsbFrequency[0];
  }
  if (Pll >= 1063 && Pll <= 1069) {
    return FsbFrequency[0];
  }
  if (Pll >= 530 && Pll <= 536) {
    return FsbFrequency[1];
  }
  if (Pll >= 797 && Pll <= 803) {
    return FsbFrequency[2];
  }
  if (Pll >= 663 && Pll <= 669) {
    return FsbFrequency[3];
  }
  if (Pll >= 1330 && Pll <= 1336) {
    return FsbFrequency[4];
  }
  return 0 /* Invalid */;
}

UINT64
InternalCalculateTSCFromApplePlatformInfo (
  OUT  UINT64   *FSBFrequency  OPTIONAL,
  IN   BOOLEAN  Recalculate
  )
{
  //
  // Cache the result to speed up multiple calls.
  //
  STATIC BOOLEAN ObtainedFreqs = FALSE;
  STATIC UINT64  FsbFreq       = 0;
  STATIC UINT64  TscFreq       = 0;

  EFI_STATUS                             Status;
  APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *PlatformInfo;
  UINT32                                 Size;
  UINT64                                 *FsbFreqs;
  UINT32                                 Un44;
  UINT32                                 Un78;
  UINT64                                 Dividend;
  UINT32                                 Divisor;

  if (Recalculate) {
    ObtainedFreqs = FALSE;
    FsbFreq       = 0;
    TscFreq       = 0;
  }

  if (!ObtainedFreqs) {
    ObtainedFreqs = TRUE;
    Size          = sizeof (FsbFreq);

    Status = gBS->LocateProtocol (
      &gApplePlatformInfoDatabaseProtocolGuid,
      NULL,
      (VOID **) &PlatformInfo
      );
    if (!EFI_ERROR (Status)) {
      Status = OcReadApplePlatformFirstData (
        PlatformInfo,
        &gAppleFsbFrequencyPlatformInfoGuid,
        &Size,
        &FsbFreq
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCCPU: Failed to get FSBFrequency first data - %r, trying HOB method\n", Status));
        Status = OcReadApplePlatformData (
          PlatformInfo,
          &gAppleFsbFrequencyPlatformInfoGuid,
          &gAppleFsbFrequencyPlatformInfoIndexHobGuid,
          &Size,
          &FsbFreq
          );
      }

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCCPU: Failed to get FSBFrequency data using HOB method - %r, trying legacy\n", Status));
        Status = OcReadApplePlatformFirstDataAlloc (
          PlatformInfo,
          &gAppleFsbFrequencyListPlatformInfoGuid,
          &Size,
          (VOID **) &FsbFreqs
          );
        if (!EFI_ERROR (Status)) {
          if (Size >= sizeof (UINT64) && Size % sizeof (UINT64) == 0) {
            FsbFreq = InternalSelectAppleFsbFrequency (FsbFreqs, Size / sizeof (UINT64));
          } else {
            DEBUG ((DEBUG_INFO, "OCCPU: Invalid FSBFrequency list size %u - %r\n", Size, Status));
            Status = EFI_INVALID_PARAMETER;
          }

          FreePool (FsbFreqs);
        }
      }
    } else {
      DEBUG ((DEBUG_VERBOSE, "OCCPU: Failed to locate ApplePlatformInfo protocol - %r\n", Status));
    }

    //
    // This is not necessarily Apple, but keep it here for the time being.
    // Should work on more or less any MCP79 device.
    //
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCCPU: Failed to get FSBFrequency data using Apple Platform Info - %r\n", Status));
      
      if (MmioRead16 (B_NVIDIA_MCP_MC_BASE) == V_NVIDIA_MCP_MC_VENDOR) {
        Un44 = MmioRead32 (B_NVIDIA_MCP_MC_BASE + R_NVIDIA_MCP_MC_UN44);
        Un78 = MmioRead32 (B_NVIDIA_MCP_MC_BASE + R_NVIDIA_MCP_MC_UN78);

        Dividend = NVIDIA_MCP79_GET_FSB_FREQUENCY_DIVIDEND (Un44, Un78);
        Divisor  = NVIDIA_MCP79_GET_FSB_FREQUENCY_DIVISOR (Un44, Un78);

        DEBUG ((
          DEBUG_INFO,
          "OCCPU: Found nForce MCP MC 0x%08X (UN44 0x%08X, UN78 0x%08X, DVD 0x%016Lx, DIV 0x%08X)\n",
          MmioRead32 (B_NVIDIA_MCP_MC_BASE),
          Un44,
          Un78,
          Dividend,
          Divisor
          ));

        if (Divisor != 0) {
          FsbFreq = DivU64x32 (Dividend, Divisor);
          if (FsbFreq != 0) {
            Status = EFI_SUCCESS;
          }
        }
      }
    }

    if (EFI_ERROR (Status)) {
      return 0;
    }

    TscFreq = InternalConvertAppleFSBToTSCFrequency (FsbFreq);
  }
  
  //
  // Optionally update FSBFrequency.
  //
  if (FSBFrequency != NULL) {
    *FSBFrequency = FsbFreq;
  }

  return TscFreq;
}

UINT64
InternalCalculateARTFrequencyIntel (
  OUT UINT64   *CPUFrequency,
  OUT UINT64   *TscAdjustPtr OPTIONAL,
  IN  BOOLEAN  Recalculate
  )
{
  //
  // Cache the result to speed up multiple calls. For example, we might need
  // this frequency on module entry to initialise a TimerLib instance, and at
  // a later point in time to gather CPU information.
  //
  STATIC UINT64 ARTFrequency        = 0;
  STATIC UINT64 CPUFrequencyFromART = 0;

  UINT32                                            MaxId;
  UINT32                                            CpuVendor;

  UINT32                                            CpuidDenominatorEax;
  UINT32                                            CpuidNumeratorEbx;
  UINT32                                            CpuidARTFrequencyEcx;
  CPUID_PROCESSOR_FREQUENCY_EAX                     CpuidFrequencyEax;
  UINT64                                            TscAdjust;
  UINT64                                            CPUFrequencyFromTSC;
  CPUID_VERSION_INFO_EAX                            CpuidVerEax;
  UINT8                                             Model;

  if (Recalculate) {
    ARTFrequency        = 0;
    CPUFrequencyFromART = 0;
  }

  if (ARTFrequency == 0) {
    //
    // Get vendor CPUID 0x00000000
    //
    AsmCpuid (CPUID_SIGNATURE, &MaxId, &CpuVendor, NULL, NULL);
    //
    // Determine our core crystal clock frequency
    //
    if (CpuVendor == CPUID_VENDOR_INTEL && MaxId >= CPUID_TIME_STAMP_COUNTER) {
      TscAdjust = AsmReadMsr64 (MSR_IA32_TSC_ADJUST);
      DEBUG ((DEBUG_INFO, "OCCPU: TSC Adjust %Lu\n", TscAdjust));

      if (TscAdjustPtr != NULL) {
        *TscAdjustPtr = TscAdjust;
      }

      AsmCpuid (
        CPUID_TIME_STAMP_COUNTER,
        &CpuidDenominatorEax,
        &CpuidNumeratorEbx,
        &CpuidARTFrequencyEcx,
        NULL
        );
      if (CpuidARTFrequencyEcx > 0) {
        ARTFrequency = CpuidARTFrequencyEcx;
        DEBUG ((DEBUG_INFO, "OCCPU: Queried Core Crystal Clock Frequency %11LuHz\n", ARTFrequency));
      } else {
        AsmCpuid (CPUID_VERSION_INFO, &CpuidVerEax.Uint32, NULL, NULL, NULL);
        Model = (UINT8) CpuidVerEax.Bits.Model | (UINT8) (CpuidVerEax.Bits.ExtendedModelId << 4U);
        //
        // Fall back to identifying ART frequency based on known models
        //
        switch (Model) {
          case CPU_MODEL_SKYLAKE:
          case CPU_MODEL_SKYLAKE_DT:
          case CPU_MODEL_KABYLAKE:
          case CPU_MODEL_KABYLAKE_DT:
            ARTFrequency = CLIENT_ART_CLOCK_SOURCE; // 24 Mhz
            break;
          case CPU_MODEL_DENVERTON:
            ARTFrequency = SERVER_ART_CLOCK_SOURCE; // 25 Mhz
            break;
          case CPU_MODEL_GOLDMONT:
            ARTFrequency = ATOM_ART_CLOCK_SOURCE; // 19.2 Mhz
            break;
        }
        if (ARTFrequency > 0) {
          DEBUG ((DEBUG_INFO, "OCCPU: Known Model Core Crystal Clock Frequency %11LuHz\n", ARTFrequency));
        }
      }

      if (CpuidDenominatorEax > 0 && CpuidNumeratorEbx > 0) {
        //
        // Some Intel chips don't report their core crystal clock frequency.
        // Calculate it by dividing the TSC frequency by the TSC ratio.
        //
        if (ARTFrequency == 0 && MaxId >= CPUID_PROCESSOR_FREQUENCY) {
          CPUFrequencyFromTSC = InternalCalculateTSCFromPMTimer (Recalculate);
          ARTFrequency = MultThenDivU64x64x32(
            CPUFrequencyFromTSC,
            CpuidDenominatorEax,
            CpuidNumeratorEbx,
            NULL
            );
          if (ARTFrequency > 0ULL) {
            DEBUG ((
              DEBUG_INFO,
              "OCCPU: Core Crystal Clock Frequency from TSC %11LuHz = %11LuHz * %u / %u\n",
              ARTFrequency,
              CPUFrequencyFromTSC,
              CpuidDenominatorEax,
              CpuidNumeratorEbx
              ));
            //
            // Use the reported CPU frequency rather than deriving it from ARTFrequency
            //
            AsmCpuid (CPUID_PROCESSOR_FREQUENCY, &CpuidFrequencyEax.Uint32, NULL, NULL, NULL);
            CPUFrequencyFromART = MultU64x32 (CpuidFrequencyEax.Bits.ProcessorBaseFrequency, 1000000);
          }
        }

        //
        // If we still can't determine the core crystal clock frequency, assume
        // it's 24 Mhz like most Intel chips to date.
        //
        if (ARTFrequency == 0ULL) {
          ARTFrequency = DEFAULT_ART_CLOCK_SOURCE;
          DEBUG ((DEBUG_INFO, "OCCPU: Fallback Core Crystal Clock Frequency %11LuHz\n", ARTFrequency));
        }

        ASSERT (ARTFrequency > 0ULL);
        if (CPUFrequencyFromART == 0ULL) {
          CPUFrequencyFromART = MultThenDivU64x64x32 (
            ARTFrequency,
            CpuidNumeratorEbx,
            CpuidDenominatorEax,
            NULL
            );
        }
        ASSERT (CPUFrequencyFromART > 0ULL);
        DEBUG ((
          DEBUG_INFO,
          "OCCPU: CPUFrequencyFromART %11LuHz %5LuMHz = %Lu * %u / %u\n",
          CPUFrequencyFromART,
          DivU64x32 (CPUFrequencyFromART, 1000000),
          ARTFrequency,
          CpuidNumeratorEbx,
          CpuidDenominatorEax
          ));
      }
    }
  }

  *CPUFrequency = CPUFrequencyFromART;
  return ARTFrequency;
}

UINT64
InternalCalculateVMTFrequency (
  OUT UINT64   *FSBFrequency     OPTIONAL,
  OUT BOOLEAN  *UnderHypervisor  OPTIONAL
  )
{
  UINT32                  CpuidEax;
  UINT32                  CpuidEbx;
  UINT32                  CpuidEcx;
  UINT32                  CpuidEdx;
  CPUID_VERSION_INFO_ECX  CpuidVerEcx;

  CHAR8                   HvVendor[13];
  UINT64                  Msr;

  AsmCpuid (
    CPUID_VERSION_INFO,
    NULL,
    NULL,
    &CpuidVerEcx.Uint32,
    NULL
    );

  if (FSBFrequency != NULL) {
    *FSBFrequency = 0;
  }

  if (UnderHypervisor != NULL) {
    *UnderHypervisor = CpuidVerEcx.Bits.NotUsed != 0;
  }

  //
  // TODO: We do not have Hypervisor support in EDK II CPUID structure yet.
  // See https://github.com/acidanthera/audk/pull/2.
  // Get Hypervisor/Virtualization information.
  //
  if (CpuidVerEcx.Bits.NotUsed == 0) {
    return 0;
  }

  //
  // If we are under virtualization and cpuid invtsc is enabled, we can just read
  // TSCFrequency and FSBFrequency from VMWare Timing node instead of reading MSR
  // (which hypervisors may not implemented yet), at least in QEMU/VMWare it works.
  // Source:
  //  1. CPUID usage for interaction between Hypervisors and Linux.:
  //     https://lwn.net/Articles/301888/
  //  2. [Qemu-devel] [PATCH v2 0/3] x86-kvm: Fix Mac guest timekeeping by exposi:
  //     https://lists.gnu.org/archive/html/qemu-devel/2017-01/msg04344.html
  //
  // Hyper-V only implements MSRs for TSC and FSB frequencies in Hz.
  // See https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/tlfs
  //
  AsmCpuid (0x40000000, &CpuidEax, &CpuidEbx, &CpuidEcx, &CpuidEdx);

  CopyMem (&HvVendor[0], &CpuidEbx, sizeof (UINT32));
  CopyMem (&HvVendor[4], &CpuidEcx, sizeof (UINT32));
  CopyMem (&HvVendor[8], &CpuidEdx, sizeof (UINT32));
  HvVendor[12] = '\0';

  if (AsciiStrCmp (HvVendor, "Microsoft Hv") == 0) {
    //
    // HV_X64_MSR_APIC_FREQUENCY
    //
    Msr = AsmReadMsr64 (0x40000023);
    if (FSBFrequency != NULL) {
      *FSBFrequency = Msr;
    }

    //
    // HV_X64_MSR_TSC_FREQUENCY
    //
    Msr = AsmReadMsr64 (0x40000022);
    return Msr;
  }

  //
  // Other hypervisors implement TSC/FSB frequency as an additional CPUID leaf.
  //
  if (CpuidEax < 0x40000010) {
    return 0;
  }

  AsmCpuid (0x40000010, &CpuidEax, &CpuidEbx, NULL, NULL);
  if (CpuidEax == 0 || CpuidEbx == 0) {
    return 0;
  }

  //
  // We get kHZ from node and we should translate it first.
  //
  if (FSBFrequency != NULL) {
    *FSBFrequency = CpuidEbx * 1000ULL;
  }

  return CpuidEax * 1000ULL;
}

UINT64
OcGetTSCFrequency (
  VOID
  )
{
  UINT64 CPUFrequency;
  //
  // For Intel platforms (the vendor check is covered by the callee), prefer
  // the CPU Frequency derieved from the ART, as the PM timer might not be
  // available (e.g. 300 series chipsets).
  // TODO: For AMD, the base clock can be determined from P-registers.
  //
  InternalCalculateARTFrequencyIntel (&CPUFrequency, NULL, FALSE);
  if (CPUFrequency == 0) {
    CPUFrequency = InternalCalculateVMTFrequency (NULL, NULL);
    if (CPUFrequency == 0) {
      CPUFrequency = InternalCalculateTSCFromApplePlatformInfo (NULL, FALSE);
      if (CPUFrequency == 0) {
        CPUFrequency = InternalCalculateTSCFromPMTimer (FALSE);
        if (CPUFrequency == 0) {
          //
          // Assume at least some frequency, so that we always work.
          //
          CPUFrequency = OC_FALLBACK_CPU_FREQUENCY;
        }
      }
    }
  }
  //
  // For all known models with an invariant TSC, its frequency is equal to the
  // CPU's specified base clock.
  //
  return CPUFrequency;
}

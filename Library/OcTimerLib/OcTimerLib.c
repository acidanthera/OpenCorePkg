/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/GenericIch.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/CpuId.h>

#include <Library/BaseLib.h>
#include <Library/OcCpuLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>

STATIC UINT64 mPerformanceCounterFrequency = 0;

UINTN
OcGetPmTimerAddr (
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
  if (PciRead16 (PCI_ICH_LPC_ADDRESS (0)) == V_ICH_PCI_DEVICE_ID) {
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
    } else if (PciRead16 (PCI_ICH_PMC_ADDRESS (0)) == V_ICH_PCI_DEVICE_ID) {
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

/**
  Calculate the TSC frequency

  @retval  The calculated TSC frequency.
**/
UINT64
RecalculateTSC (
  VOID
  )
{
  UINTN    TimerAddr;
  UINT64   Tsc0;
  UINT64   Tsc1;
  UINT32   AcpiTick0;
  UINT32   AcpiTick1;
  UINT32   AcpiTicksDelta;
  UINT32   AcpiTicksTarget;
  UINT32   TimerResolution;
  EFI_TPL  PrevTpl;

  TimerAddr       = OcGetPmTimerAddr (NULL);
  TimerResolution = 10;

  if (TimerAddr != 0) {
    mPerformanceCounterFrequency = 0;

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
      // 357954 clocks of ACPI timer (100ms)
      //
      AcpiTicksTarget = V_ACPI_TMR_FREQUENCY / TimerResolution;

      //
      // Disable all events to ensure that nobody interrupts us.
      //
      PrevTpl   = gBS->RaiseTPL (TPL_HIGH_LEVEL);

      AcpiTick0 = IoRead32 (TimerAddr);
      Tsc0      = AsmReadTsc ();

      do {
        CpuPause ();

        //
        // Check how many AcpiTicks have passed since we started.
        //
        AcpiTick1 = IoRead32 (TimerAddr);

        if (AcpiTick0 <= AcpiTick1) {
          //
          // No overflow.
          //
          AcpiTicksDelta = AcpiTick1 - AcpiTick0;
        } else if (AcpiTick0 - AcpiTick1 <= 0x00FFFFFF) {
          //
          // Overflow, 24-bit timer.
          //
          AcpiTicksDelta = 0x00FFFFFF - AcpiTick0 + AcpiTick1;
        } else {
          //
          // Overflow, 32-bit timer.
          //
          AcpiTicksDelta = MAX_UINT32 - AcpiTick0 + AcpiTick1;
        }

        //
        // Keep checking AcpiTicks until target is reached.
        //
      } while (AcpiTicksDelta < AcpiTicksTarget);

      Tsc1 = AsmReadTsc ();

      //
      // On some systems we may end up waiting for notably longer than 100ms,
      // despite disabling all events. Divide by actual time passed as suggested
      // by asava's Clover patch r2668.
      //
      mPerformanceCounterFrequency = DivU64x32 (
        MultU64x32 (Tsc1 - Tsc0, V_ACPI_TMR_FREQUENCY), AcpiTicksDelta
        );

      //
      // Restore to normal TPL.
      //
      gBS->RestoreTPL (PrevTpl);
    }
  }

  DEBUG ((DEBUG_VERBOSE, "TscFrequency %lld\n", mPerformanceCounterFrequency));

  return mPerformanceCounterFrequency;
}

UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  (VOID) MicroSeconds;
  return EFI_UNSUPPORTED;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return The value of NanoSeconds inputted.

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  (VOID) NanoSeconds;
  return EFI_UNSUPPORTED;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  The counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return AsmReadTsc ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 0xffffffffffffffffULL;
  }
  return mPerformanceCounterFrequency;
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN UINT64  Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  if (Frequency == 0) {
    return 0;
  }

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN) Shift);
  Frequency = RShiftU64 (Frequency, (UINTN) Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}

/**
  The constructor function caches PerformanceCounterFrequency.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
OcTimerLibConstructor (
  VOID
  )
{
  RecalculateTSC ();

  return EFI_SUCCESS;
}

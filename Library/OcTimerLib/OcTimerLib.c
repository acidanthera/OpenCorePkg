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

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Macros.h>

UINT64 mTimerLibTscFrequency = 0;

// CalculateTSC
/** Calculate the TSC frequency

  @param[in] Recalculate  The firmware allocated handle for the EFI image.

  @retval  The calculated TSC frequency.
**/
UINT64
CalculateTSC (
  IN BOOLEAN  Recalculate
  )
{
  UINT32 TimerAddr;
  UINT64 Tsc0;
  UINT64 Tsc1;
  UINT64 AcpiTick0;
  UINT64 AcpiTick1;
  UINT64 AcpiTicksDelta;
  UINT32 AcpiTicksTarget;
  UINT32 TimerResolution;

  // Already Calculated, Return Cached Result

  if (((mTimerLibTscFrequency == 0) || Recalculate) && PciRead16 (PCI_ICH_LPC_ADDRESS (0)) == 0x8086) {
    TimerAddr       = 0;
    TimerResolution = 10;

    // Check if ACPI I/O Space Is Enabled On LPC device

    if ((PciRead8 (PCI_ICH_LPC_ADDRESS (R_ICH_LPC_ACPI_CNT)) & B_ICH_LPC_ACPI_CNT_ACPI_EN) != 0) {
      TimerAddr = ((PciRead16 (PCI_ICH_LPC_ADDRESS (R_ICH_LPC_ACPI_BASE)) & B_ICH_LPC_ACPI_BASE_BAR) + R_ACPI_PM1_TMR);
      //DEBUG ((DEBUG_INFO, "Acpi Timer Addr 0x%0x (LPC)\n", TimerAddr));
    } else {
      // Starting from Intel Sunrisepoint (Skylake PCH) the iTCO watchdog resources
      // have been moved to reside under the i801 SMBus host controller whereas
      // previously they were under the LPC device.
      //
      // Check if ACPI I/O space is enabled on SMBUS device

      if ((PciRead8 (PCI_ICH_SMBUS_ADDRESS (R_ICH_SMBUS_ACPI_CNT)) & B_ICH_SMBUS_ACPI_CNT_ACPI_EN) != 0) {
        TimerAddr = ((PciRead16 (PCI_ICH_SMBUS_ADDRESS (R_ICH_SMBUS_ACPI_BASE)) & B_ICH_SMBUS_ACPI_BASE_BAR) + R_ACPI_PM1_TMR);
        //DEBUG ((DEBUG_INFO, "Acpi Timer Addr 0x%0x (SMB)\n", TimerAddr));
      }
    }

    if (TimerAddr != 0) {
      mTimerLibTscFrequency = 0;

      // Check That Timer Is Advancing

      AcpiTick0 = IoRead32 (TimerAddr);

      gBS->Stall (500);

      AcpiTick1 = IoRead32 (TimerAddr);

      if (AcpiTick0 != AcpiTick1) {
        // ACPI PM timers are usually of 24-bit length, but there are some less common cases of 32-bit length also.
        // When the maximal number is reached, it overflows.
        // The code below can handle overflow with AcpiTicksTarget of up to 24-bit size,
        // on both available sizes of ACPI PM Timers (24-bit and 32-bit).
        //
        // 357954 clocks of ACPI timer (100ms)

        AcpiTicksTarget = (V_ACPI_TMR_FREQUENCY / TimerResolution);

        AcpiTick0 = IoRead32 (TimerAddr);
        Tsc0      = AsmReadTsc ();

        do {
          CpuPause ();

          // Check How Many AcpiTicks Have Passed Since We Started

          AcpiTick1 = IoRead32 (TimerAddr);

          if (AcpiTick0 <= AcpiTick1) {
            // No Overflow

            AcpiTicksDelta = (AcpiTick1 - AcpiTick0);
          } else if ((AcpiTick0 - AcpiTick1) <= 0x00FFFFFF) {
            // Overflow, 24 Bit Timer

            AcpiTicksDelta = ((0x00FFFFFF - AcpiTick0) + AcpiTick1);

          } else {
            // Overflow, 32 Bit Timer

            AcpiTicksDelta = ((MAX_UINT32 - AcpiTick0) + AcpiTick1);
          }

          // Keep Checking Acpi ticks Until Target Is Reached
        } while (AcpiTicksDelta < AcpiTicksTarget);

        Tsc1                  = AsmReadTsc ();
        mTimerLibTscFrequency = MultU64x32 ((Tsc1 - Tsc0), TimerResolution);

      }
    }
  }

  //DEBUG ((DEBUG_INFO, "TscFrequency %lld\n", mTimerLibTscFrequency));

  return mTimerLibTscFrequency;
}

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

#ifndef GENERIC_ICH_H
#define GENERIC_ICH_H

// GenericIchDefs  Generic ICH Definitions.
//
//  Definitions beginning with "R_" are registers.
//  Definitions beginning with "B_" are bits within registers.
//  Definitions beginning with "V_" are meaningful values of bits within the registers.

// IchPciAddressing  PCI Bus Address for ICH.

#define PCI_BUS_NUMBER_ICH             0x00  ///< ICH is on PCI Bus 0.
#define PCI_DEVICE_NUMBER_ICH          31    ///< ICH is Device 31.
#define PCI_FUNCTION_NUMBER_ICH_LPC    0     ///< LPC is Function 0.
#define PCI_FUNCTION_NUMBER_ICH_PMC    2     ///< PMC is Function 2.

#define V_ICH_PCI_VENDOR_ID     0x8086  ///< Intel vendor-id

// IchAcpiCntr   Control for the ICH's ACPI Counter.

#define R_ICH_ACPI_BASE         0x40
#define B_ICH_ACPI_BASE_BAR     0x0000FF80
#define R_ICH_ACPI_CNTL         0x44    ///< See ACPI_CNTL
#define B_ICH_ACPI_CNTL_ACPI_EN 0x80

#define R_ICH_BAR2_BASE         0x20
#define B_ICH_BAR2_BASE_BAR     0x0000FFC0
#define B_ICH_BAR2_BASE_BAR_EN  0x1

// Pre Intel Sunrisepoint

#define R_ICH_LPC_ACPI_BASE         R_ICH_ACPI_BASE
#define B_ICH_LPC_ACPI_BASE_BAR     B_ICH_ACPI_BASE_BAR
#define R_ICH_LPC_ACPI_CNTL         R_ICH_ACPI_CNTL
#define B_ICH_LPC_ACPI_CNTL_ACPI_EN B_ICH_ACPI_CNTL_ACPI_EN

// Intel Sunrisepoint

#define R_ICH_PMC_ACPI_BASE         R_ICH_ACPI_BASE
#define B_ICH_PMC_ACPI_BASE_BAR     B_ICH_ACPI_BASE_BAR
#define R_ICH_PMC_ACPI_CNTL         R_ICH_ACPI_CNTL
#define B_ICH_PMC_ACPI_CNTL_ACPI_EN B_ICH_ACPI_CNTL_ACPI_EN

// Intel Coffee Lake

#define R_ICH_PMC_BAR2_BASE         R_ICH_BAR2_BASE
#define B_ICH_PMC_BAR2_BASE_BAR     B_ICH_BAR2_BASE_BAR
#define B_ICH_PMC_BAR2_BASE_BAR_EN  B_ICH_BAR2_BASE_BAR_EN

// AMD Bolton (AMD Bolton Register Reference Guide 3.03)

#define R_AMD_ACPI_MMIO_BASE          0xFED80000 ///< AcpiMMioAddr (3-268)
#define R_AMD_ACPI_MMIO_PMIO_BASE     0x300      ///< PMIO (3-268)
#define R_AMD_ACPI_PM_TMR_BLOCK       0x64       ///< AcpiPmTmrBlk (3-289)

// IchAcpiTimer  The ICH's ACPI Timer.

#define R_ACPI_PM1_TMR          0x08
#define V_ACPI_TMR_FREQUENCY    3579545
#define V_ACPI_PM1_TMR_MAX_VAL  0x01000000  ///< The timer is 24 bit overflow.

/// Macro to generate the PCI address of any given ICH LPC Register.
#define PCI_ICH_LPC_ADDRESS(Register) \
  ((UINTN)(PCI_LIB_ADDRESS (PCI_BUS_NUMBER_ICH, PCI_DEVICE_NUMBER_ICH, PCI_FUNCTION_NUMBER_ICH_LPC, (Register))))

/// Macro to generate the PCI address of any given ICH PMC Register.
#define PCI_ICH_PMC_ADDRESS(Register) \
  ((UINTN)(PCI_LIB_ADDRESS (PCI_BUS_NUMBER_ICH, PCI_DEVICE_NUMBER_ICH, PCI_FUNCTION_NUMBER_ICH_PMC, (Register))))

#endif // GENERIC_ICH_H

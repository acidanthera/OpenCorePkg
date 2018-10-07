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

#ifndef GENERIC_ICH_H_
#define GENERIC_ICH_H_

// GenericIchDefs  Generic ICH Definitions.
//
//  Definitions beginning with "R_" are registers.
//  Definitions beginning with "B_" are bits within registers.
//  Definitions beginning with "V_" are meaningful values of bits within the registers.

// IchPciAddressing  PCI Bus Address for ICH.

#define PCI_BUS_NUMBER_ICH             0x00  ///< ICH is on PCI Bus 0.
#define PCI_DEVICE_NUMBER_ICH          31    ///< ICH is Device 31.
#define PCI_FUNCTION_NUMBER_ICH_LPC    0     ///< LPC is Function 0.
#define PCI_FUNCTION_NUMBER_ICH_SMBUS  2     ///< SMB is Function 2.


// IchAcpiCntr   Control for the ICH's ACPI Counter.

#define R_ICH_ACPI_BASE         0x40
#define B_ICH_ACPI_BASE_BAR     0x0000FF80
#define R_ICH_ACPI_CNT          0x44
#define B_ICH_ACPI_CNT_ACPI_EN  0x80

// Pre Intel Sunrisepoint

#define R_ICH_LPC_ACPI_BASE         R_ICH_ACPI_BASE
#define B_ICH_LPC_ACPI_BASE_BAR     B_ICH_ACPI_BASE_BAR
#define R_ICH_LPC_ACPI_CNT          R_ICH_ACPI_CNT
#define B_ICH_LPC_ACPI_CNT_ACPI_EN  B_ICH_ACPI_CNT_ACPI_EN

// Intel Sunrisepoint

#define R_ICH_SMBUS_ACPI_BASE         R_ICH_ACPI_BASE
#define B_ICH_SMBUS_ACPI_BASE_BAR     B_ICH_ACPI_BASE_BAR
#define R_ICH_SMBUS_ACPI_CNT          R_ICH_ACPI_CNT
#define B_ICH_SMBUS_ACPI_CNT_ACPI_EN  B_ICH_ACPI_CNT_ACPI_EN


// IchAcpiTimer  The ICH's ACPI Timer.

#define R_ACPI_PM1_TMR          0x08
#define V_ACPI_TMR_FREQUENCY    3579545
#define V_ACPI_PM1_TMR_MAX_VAL  0x01000000  ///< The timer is 24 bit overflow.

// PCI_ICH_LPC_ADDRESS
/// Macro to generate the PCI address of any given ICH LPC Register.
#define PCI_ICH_LPC_ADDRESS(Register) \
  ((UINTN)(PCI_LIB_ADDRESS (PCI_BUS_NUMBER_ICH, PCI_DEVICE_NUMBER_ICH, PCI_FUNCTION_NUMBER_ICH_LPC, (Register))))

// PCI_ICH_SMBUS_ADDRESS
/// Macro to generate the PCI address of any given ICH SMBUS Register.
#define PCI_ICH_SMBUS_ADDRESS(Register) \
  ((UINTN)(PCI_LIB_ADDRESS (PCI_BUS_NUMBER_ICH, PCI_DEVICE_NUMBER_ICH, PCI_FUNCTION_NUMBER_ICH_SMBUS, (Register))))

#endif // GENERIC_ICH_H_

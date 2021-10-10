/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef PCI_EXT_INTERNAL_H
#define PCI_EXT_INTERNAL_H

/**
  Offset to Root Complex Base Address for a PCI bridge device.
**/
#define PCI_BRIDGE_RCBA_OFFSET 0xF0U

/**
  Address bits of the Root Complex Base Address register.
**/
#define PCI_BRIDGE_RCBA_ADDRESS_MASK 0xFFFFC000U

/**
  Access bit of the Root Complex Base Address register.
**/
#define PCI_BRIDGE_RCBA_ACCESS_ENABLE BIT0

/**
  HTPC register in RCBA.
**/
#define RCBA_HTPC_REGISTER 0x3404U

/**
  HPET enable bit in HTPC.
**/
#define RCBA_HTPC_HPET_ENABLE BIT7

/**
  XHCI registers.
**/
#define XHC_HCCPARAMS_OFFSET      0x10
#define XHC_NEXT_CAPABILITY_MASK  0xFF00
#define XHC_CAPABILITY_ID_MASK    0xFF
#define XHC_USBCMD_OFFSET         0x0    ///< USB Command Register Offset
#define XHC_USBSTS_OFFSET         0x4    ///< USB Status Register Offset
#define XHC_POLL_DELAY            1000

/**
  EHCI registers.
**/
#define EHC_BAR_INDEX             0x0
#define EHC_HCCPARAMS_OFFSET      0x8
#define EHC_USBCMD_OFFSET         0x0    ///< USB Command Register Offset
#define EHC_USBSTS_OFFSET         0x4    ///< USB Status Register Offset
#define EHC_USBINT_OFFSET         0x8    ///< USB Interrupt Enable Register


/**
  HDA audio class missing from Pci22.h.
**/
#define PCI_CLASS_MEDIA_HDA 0x03

/**
  Offset to TCSEL register for a PCI media device.
**/
#define PCI_MEDIA_TCSEL_OFFSET 0x44U

/**
  TCSEL class mask.
**/
#define TCSEL_CLASS_MASK 0x7U

/**
  Valid PCI BAR sizes for PCI 2.0, modern PCI permit up to 8EB.
**/
#define PCI_BAR_1MB     0U
#define PCI_BAR_2MB     1U
#define PCI_BAR_4MB     2U
#define PCI_BAR_8MB     3U
#define PCI_BAR_16MB    4U
#define PCI_BAR_32MB    4U
#define PCI_BAR_64MB    5U
#define PCI_BAR_128MB   6U
#define PCI_BAR_256MB   7U
#define PCI_BAR_512MB   8U
#define PCI_BAR_1GB     9U ///< Maximum for macOS IOPCIFamily.
#define PCI_BAR_2GB    10U
#define PCI_BAR_4GB    11U
#define PCI_BAR_8GB    12U
#define PCI_BAR_16GB   14U
#define PCI_BAR_32GB   15U
#define PCI_BAR_64GB   16U
#define PCI_BAR_128GB  17U
#define PCI_BAR_256GB  18U
#define PCI_BAR_512GB  19U

/**
  When the bit of Capabilities Set, it indicates that the Function supports
  operating with the BAR sized to (2^Bit) MB. Example:
  - Bit 0 is set: supports operating with the BAR sized to 1 MB
  - Bit 1 is set: supports operating with the BAR sized to 2 MB
  - Bit n is set: supports operating with the BAR sized to (2^n) MB
**/
#define PCI_BAR_CAP_1MB     BIT0
#define PCI_BAR_CAP_2MB     BIT1
#define PCI_BAR_CAP_4MB     BIT2
#define PCI_BAR_CAP_8MB     BIT3
#define PCI_BAR_CAP_16MB    BIT4
#define PCI_BAR_CAP_32MB    BIT4
#define PCI_BAR_CAP_64MB    BIT5
#define PCI_BAR_CAP_128MB   BIT6
#define PCI_BAR_CAP_256MB   BIT7
#define PCI_BAR_CAP_512MB   BIT8
#define PCI_BAR_CAP_1GB     BIT9
#define PCI_BAR_CAP_2GB    BIT10
#define PCI_BAR_CAP_4GB    BIT11
#define PCI_BAR_CAP_8GB    BIT12
#define PCI_BAR_CAP_16GB   BIT14
#define PCI_BAR_CAP_32GB   BIT15
#define PCI_BAR_CAP_64GB   BIT16
#define PCI_BAR_CAP_128GB  BIT17
#define PCI_BAR_CAP_256GB  BIT18
#define PCI_BAR_CAP_512GB  BIT19

#endif // PCI_EXT_INTERNAL_H

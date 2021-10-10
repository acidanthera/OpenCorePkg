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
#define PCI_BAR_CAP_32MB    BIT5
#define PCI_BAR_CAP_64MB    BIT6
#define PCI_BAR_CAP_128MB   BIT7
#define PCI_BAR_CAP_256MB   BIT8
#define PCI_BAR_CAP_512MB   BIT9
#define PCI_BAR_CAP_1GB    BIT10
#define PCI_BAR_CAP_2GB    BIT11
#define PCI_BAR_CAP_4GB    BIT12
#define PCI_BAR_CAP_8GB    BIT13
#define PCI_BAR_CAP_16GB   BIT14
#define PCI_BAR_CAP_32GB   BIT15
#define PCI_BAR_CAP_64GB   BIT16
#define PCI_BAR_CAP_128GB  BIT17
#define PCI_BAR_CAP_256GB  BIT18
#define PCI_BAR_CAP_512GB  BIT19

/**
  Capability limit mask from BarSize (e.g. PciBar1MB).
**/
#define PCI_BAR_CAP_LIMIT(BarSize) ((1U << ((BarSize) + 1)) - 1)

#endif // PCI_EXT_INTERNAL_H

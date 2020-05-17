/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PciCommand.h
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#ifndef _EFI_PCI_COMMAND_H
#define _EFI_PCI_COMMAND_H

#include "PciBus.h"

//
// The PCI Command register bits owned by PCI Bus driver.
//
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCI_COMMAND_BITS_OWNED                          ( \
                EFI_PCI_COMMAND_IO_SPACE                    | \
                EFI_PCI_COMMAND_MEMORY_SPACE                | \
                EFI_PCI_COMMAND_BUS_MASTER                  | \
                EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE | \
                EFI_PCI_COMMAND_VGA_PALETTE_SNOOP           | \
                EFI_PCI_COMMAND_FAST_BACK_TO_BACK             \
                )

//
// The PCI Bridge Control register bits owned by PCI Bus driver.
// 
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCI_BRIDGE_CONTROL_BITS_OWNED                   ( \
                EFI_PCI_BRIDGE_CONTROL_ISA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA_16               | \
                EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK      \
                )

//
// The PCCard Bridge Control register bits owned by PCI Bus driver.
// 
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCCARD_BRIDGE_CONTROL_BITS_OWNED                ( \
                EFI_PCI_BRIDGE_CONTROL_ISA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA                  | \
                EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK      \
                )

EFI_STATUS 
PciReadCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  OUT UINT16       *Command
);

  
EFI_STATUS 
PciSetCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
);

EFI_STATUS 
PciEnableCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
);

EFI_STATUS 
PciDisableCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
);

EFI_STATUS 
PciDisableBridgeControlRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
);


EFI_STATUS 
PciEnableBridgeControlRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
);

EFI_STATUS 
PciReadBridgeControlRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  OUT UINT16       *Command
);

BOOLEAN
PciCapabilitySupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
LocateCapabilityRegBlock (
  IN     PCI_IO_DEVICE *PciIoDevice,
  IN     UINT8         CapId,
  IN OUT UINT8         *Offset,
     OUT UINT8         *NextRegBlock OPTIONAL
  )
/*++

Routine Description:

  Locate Capability register.

Arguments:

  PciIoDevice         - A pointer to the PCI_IO_DEVICE.
  CapId               - The capability ID.
  Offset              - A pointer to the offset. 
                        As input: the default offset; 
                        As output: the offset of the found block.
  NextRegBlock        - An optional pointer to return the value of next block.

Returns:
  
  EFI_UNSUPPORTED     - The Pci Io device is not supported.
  EFI_NOT_FOUND       - The Pci Io device cannot be found.
  EFI_SUCCESS         - The Pci Io device is successfully located.

--*/
;


#endif


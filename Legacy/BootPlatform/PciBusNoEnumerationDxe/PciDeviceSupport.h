/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciDeviceSupport.h
  
Abstract:

  

Revision History

--*/

#ifndef _EFI_PCI_DEVICE_SUPPORT_H
#define _EFI_PCI_DEVICE_SUPPORT_H

EFI_STATUS
InitializePciDevicePool (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
InsertPciDevice (
  PCI_IO_DEVICE *Bridge,
  PCI_IO_DEVICE *PciDeviceNode
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge        - TODO: add argument description
  PciDeviceNode - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DestroyPciDeviceTree (
  IN PCI_IO_DEVICE *Bridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DestroyRootBridgeByHandle (
  EFI_HANDLE Controller
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
RegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  PCI_IO_DEVICE                  *PciIoDevice,
  OUT EFI_HANDLE                     *Handle OPTIONAL
  )
/*++

Routine Description:

  This function registers the PCI IO device. It creates a handle for this PCI IO device 
  (if the handle does not exist), attaches appropriate protocols onto the handle, does
  necessary initialization, and sets up parent/child relationship with its bus controller.

Arguments:

  Controller    - An EFI handle for the PCI bus controller.
  PciIoDevice   - A PCI_IO_DEVICE pointer to the PCI IO device to be registered.
  Handle        - A pointer to hold the EFI handle for the PCI IO device.

Returns:

  EFI_SUCCESS   - The PCI device is successfully registered.
  Others        - An error occurred when registering the PCI device.

--*/
;

EFI_STATUS
DeRegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller  - TODO: add argument description
  Handle      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
StartPciDevices (
  IN EFI_HANDLE                         Controller,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller          - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
CreateRootBridge (
  IN EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
GetRootBridgeByHandle (
  EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS 
InsertRootBridge (
  PCI_IO_DEVICE *RootBridge
);

EFI_STATUS 
DestroyRootBridge ( 
   IN PCI_IO_DEVICE *RootBridge 
);

BOOLEAN
RootBridgeExisted (
  IN EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
PciDeviceExisted (
  IN PCI_IO_DEVICE    *Bridge,
  IN PCI_IO_DEVICE    *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge      - TODO: add argument description
  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
ActiveVGADeviceOnTheSameSegment (
  IN PCI_IO_DEVICE        *VgaDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  VgaDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
ActiveVGADeviceOnTheRootBridge (
  IN PCI_IO_DEVICE        *RootBridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;
#endif

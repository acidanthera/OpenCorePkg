/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciBus.c
  
Abstract:

  PCI Bus Driver

Revision History
 
--*/

#include "PciBus.h"

//
// PCI Bus Support Function Prototypes
//

EFI_STATUS
EFIAPI
PciBusEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

EFI_STATUS
EFIAPI
PciBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
PciBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
PciBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  );


//
// PCI Bus Driver Global Variables
//

EFI_DRIVER_BINDING_PROTOCOL                   gPciBusDriverBinding = {
  PciBusDriverBindingSupported,
  PciBusDriverBindingStart,
  PciBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};

BOOLEAN gFullEnumeration;

UINT64 gAllOne    = 0xFFFFFFFFFFFFFFFFULL;
UINT64 gAllZero   = 0;
 
//
// PCI Bus Driver Support Functions
//
EFI_STATUS
EFIAPI
PciBusEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Initialize the global variables
  publish the driver binding protocol

Arguments:

  IN EFI_HANDLE     ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable

Returns:

  EFI_SUCCESS 
  EFI_DEVICE_ERROR 

--*/
{
  EFI_STATUS         Status;

  //
  // Initialize the EFI Driver Library
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gPciBusDriverBinding,
             ImageHandle,
             &gPciBusComponentName,
             &gPciBusComponentName2
             );
  ASSERT_EFI_ERROR (Status);
  
  InitializePciDevicePool ();

  gFullEnumeration = TRUE;
  
  return Status;
}

EFI_STATUS
EFIAPI
PciBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

Routine Description:

  Check to see if pci bus driver supports the given controller

Arguments:
  
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath

Returns:

  EFI_SUCCESS

--*/
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  EFI_DEV_PATH_PTR                Node;

  if (RemainingDevicePath != NULL) {
    Node.DevPath = RemainingDevicePath;
    if (Node.DevPath->Type != HARDWARE_DEVICE_PATH ||
        Node.DevPath->SubType != HW_PCI_DP         ||
        DevicePathNodeLength(Node.DevPath) != sizeof(PCI_DEVICE_PATH)) {
      return EFI_UNSUPPORTED;
    }
  }
  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciRootBridgeIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  Start to management the controller passed in

Arguments:
  
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath

Returns:
 

--*/
{
  EFI_STATUS                          Status;

  //
  // Enumerate the entire host bridge
  // After enumeration, a database that records all the device information will be created
  //
  //
  Status = PciEnumerator (Controller);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Enable PCI device specified by remaining device path. BDS or other driver can call the
  // start more than once.
  //
  
  StartPciDevices (Controller, RemainingDevicePath);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
/*++

Routine Description:

  Stop one or more children created at start of pci bus driver
  if all the the children get closed, close the protocol

Arguments:
  
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer

Returns:

  
--*/
{
  EFI_STATUS  Status;
  UINTN       Index;
  BOOLEAN     AllChildrenStopped;

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    gBS->CloseProtocol (
           Controller,
           &gEfiPciRootBridgeIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    DestroyRootBridgeByHandle (
      Controller
      );

    return EFI_SUCCESS;
  }

  //
  // Stop all the children
  //

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    //
    // De register all the pci device
    //
    Status = DeRegisterPciDevice (Controller, ChildHandleBuffer[Index]);

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

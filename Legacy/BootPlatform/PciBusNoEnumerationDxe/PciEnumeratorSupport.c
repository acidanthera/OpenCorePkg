/*++

Copyright (c) 2005 - 2016, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciEnumeratorSupport.c
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#include "PciBus.h"

EFI_STATUS 
InitializePPB (
  IN PCI_IO_DEVICE *PciIoDevice 
);

EFI_STATUS 
InitializeP2C (
  IN PCI_IO_DEVICE *PciIoDevice 
);

PCI_IO_DEVICE* 
CreatePciIoDevice (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
);


PCI_IO_DEVICE*
GatherP2CInfo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
);

UINTN
PciParseBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
);


EFI_STATUS
PciSearchDevice (
  IN PCI_IO_DEVICE                      *Bridge,
  PCI_TYPE00                            *Pci,
  UINT8                                 Bus,
  UINT8                                 Device,
  UINT8                                 Func,
  PCI_IO_DEVICE                         **PciDevice
);


EFI_STATUS 
DetermineDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice
);

EFI_STATUS 
BarExisted (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINTN         Offset,
  OUT UINT32       *BarLengthValue,
  OUT UINT32       *OriginalBarValue
  );



EFI_DEVICE_PATH_PROTOCOL*
CreatePciDevicePath(
  IN  EFI_DEVICE_PATH_PROTOCOL *ParentDevicePath,
  IN  PCI_IO_DEVICE            *PciIoDevice 
);

PCI_IO_DEVICE* 
GatherDeviceInfo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
);

PCI_IO_DEVICE* 
GatherPPBInfo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
);

EFI_STATUS
PciDevicePresent (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  PCI_TYPE00                          *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

  This routine is used to check whether the pci device is present

Arguments:

Returns:

  None

--*/
{
  UINT64      Address;
  EFI_STATUS  Status;

  //
  // Create PCI address map in terms of Bus, Device and Func
  //
  Address = EFI_PCI_ADDRESS (Bus, Device, Func, 0);

  //
  // Read the Vendor Id register
  //
  Status = PciRootBridgeIo->Pci.Read (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint32,
                                  Address,
                                  1,
                                  Pci
                                  );

  if (!EFI_ERROR (Status) && (Pci->Hdr).VendorId != 0xffff) {

    //
    // Read the entire config header for the device
    //

    PciRootBridgeIo->Pci.Read (
                                    PciRootBridgeIo,
                                    EfiPciWidthUint32,
                                    Address,
                                    sizeof (PCI_TYPE00) / sizeof (UINT32),
                                    Pci
                                    );

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
PciPciDeviceInfoCollector (
  IN PCI_IO_DEVICE                      *Bridge,
  UINT8                                 StartBusNumber
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS          Status;
  PCI_TYPE00          Pci;
  UINT8               Device;
  UINT8               Func;
  UINT8               SecBus;
  PCI_IO_DEVICE       *PciIoDevice;
  EFI_PCI_IO_PROTOCOL *PciIo;

  Status      = EFI_SUCCESS;
  SecBus      = 0;
  PciIoDevice = NULL;

  for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {

    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {

      //
      // Check to see whether PCI device is present
      //

      Status = PciDevicePresent (
                Bridge->PciRootBridgeIo,
                &Pci,
                (UINT8) StartBusNumber,
                (UINT8) Device,
                (UINT8) Func
                );

      if (EFI_ERROR (Status) && Func == 0) {
        //
        // go to next device if there is no Function 0
        //
        break;
      }

      if (!EFI_ERROR (Status)) {

        //
        // Collect all the information about the PCI device discovered
        //
        Status = PciSearchDevice (
                  Bridge,
                  &Pci,
                  (UINT8) StartBusNumber,
                  Device,
                  Func,
                  &PciIoDevice
                  );

        //
        // Recursively scan PCI busses on the other side of PCI-PCI bridges
        //
        //

        if (!EFI_ERROR (Status) && (IS_PCI_BRIDGE (&Pci) || IS_CARDBUS_BRIDGE (&Pci))) {

          //
          // If it is PPB, we need to get the secondary bus to continue the enumeration
          //
          PciIo   = &(PciIoDevice->PciIo);

          Status  = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x19, 1, &SecBus);

          if (EFI_ERROR (Status)) {
            return Status;
          }
              
          //
          // If the PCI bridge is initialized then enumerate the next level bus
          //
          if (SecBus != 0) {
            Status = PciPciDeviceInfoCollector (
                      PciIoDevice,
                      (UINT8) (SecBus)
                      );
          }
        }

        if (Func == 0 && !IS_PCI_MULTI_FUNC (&Pci)) {

          //
          // Skip sub functions, this is not a multi function device
          //
          Func = PCI_MAX_FUNC;
        }
      }

    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PciSearchDevice (
  IN  PCI_IO_DEVICE                         *Bridge,
  IN  PCI_TYPE00                            *Pci,
  IN  UINT8                                 Bus,
  IN  UINT8                                 Device,
  IN  UINT8                                 Func,
  OUT PCI_IO_DEVICE                         **PciDevice
  )
/*++

Routine Description:

  Search required device.

Arguments:

  Bridge     - A pointer to the PCI_IO_DEVICE.
  Pci        - A pointer to the PCI_TYPE00.
  Bus        - Bus number.
  Device     - Device number.
  Func       - Function number.
  PciDevice  - The Required pci device.

Returns:

  Status code.

--*/
{
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = NULL;

  if (!IS_PCI_BRIDGE (Pci)) {

    if (IS_CARDBUS_BRIDGE (Pci)) {
      PciIoDevice = GatherP2CInfo (
                      Bridge->PciRootBridgeIo,
                      Pci,
                      Bus,
                      Device,
                      Func
                      );
      if ((PciIoDevice != NULL) && (gFullEnumeration == TRUE)) {
        InitializeP2C (PciIoDevice);
      }
    } else {

      //
      // Create private data for Pci Device
      //
      PciIoDevice = GatherDeviceInfo (
                      Bridge->PciRootBridgeIo,
                      Pci,
                      Bus,
                      Device,
                      Func
                      );

    }

  } else {

    //
    // Create private data for PPB
    //
    PciIoDevice = GatherPPBInfo (
                    Bridge->PciRootBridgeIo,
                    Pci,
                    Bus,
                    Device,
                    Func
                    );

    //
    // Special initialization for PPB including making the PPB quiet
    //
    if ((PciIoDevice != NULL) && (gFullEnumeration == TRUE)) {
      InitializePPB (PciIoDevice);
    }
  }

  if (!PciIoDevice) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Create a device path for this PCI device and store it into its private data
  //
  CreatePciDevicePath(
    Bridge->DevicePath,
    PciIoDevice 
    );
  
  //
  // ** CHANGE **
  // PCI Option ROM support removed (continuation of patch by nms42).
  //
  if (gFullEnumeration) {
    ResetPowerManagementFeature (PciIoDevice);
  }

 
  //
  // Insert it into a global tree for future reference
  //
  InsertPciDevice (Bridge, PciIoDevice);

  //
  // Determine PCI device attributes
  //
  DetermineDeviceAttribute (PciIoDevice);

  if (PciDevice != NULL) {
    *PciDevice = PciIoDevice;
  }

  return EFI_SUCCESS;
}

PCI_IO_DEVICE *
GatherDeviceInfo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINTN                           Offset;
  UINTN                           BarIndex;
  PCI_IO_DEVICE                   *PciIoDevice;

  PciIoDevice = CreatePciIoDevice (
                  PciRootBridgeIo,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (!PciIoDevice) {
    return NULL;
  }

  //
  // If it is a full enumeration, disconnect the device in advance
  //
  if (gFullEnumeration) {

    PciDisableCommandRegister (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

  }

  //
  // Start to parse the bars
  //
  for (Offset = 0x10, BarIndex = 0; Offset <= 0x24; BarIndex++) {
    Offset = PciParseBar (PciIoDevice, Offset, BarIndex);
  }

  return PciIoDevice;
}

PCI_IO_DEVICE *
GatherPPBInfo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  PCI_IO_DEVICE                   *PciIoDevice;
  EFI_STATUS                      Status;
  UINT8                           Value;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  UINT8                           Temp;

  PciIoDevice = CreatePciIoDevice (
                  PciRootBridgeIo,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (!PciIoDevice) {
    return NULL;
  }
  
  if (gFullEnumeration) {
    PciDisableCommandRegister (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

    //
    // Initalize the bridge control register
    //
    PciDisableBridgeControlRegister (PciIoDevice, EFI_PCI_BRIDGE_CONTROL_BITS_OWNED);
  }

  PciIo = &PciIoDevice->PciIo;

  //
  // Test whether it support 32 decode or not
  //
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Temp);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &gAllOne);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Value);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Temp);

  if (Value) {
    if (Value & 0x01) {
      PciIoDevice->Decodes |= EFI_BRIDGE_IO32_DECODE_SUPPORTED;
    } else {
      PciIoDevice->Decodes |= EFI_BRIDGE_IO16_DECODE_SUPPORTED;
    }
  }

  Status = BarExisted (
            PciIoDevice,
            0x24,
            NULL,
            NULL
            );

  //
  // test if it supports 64 memory or not
  //
  if (!EFI_ERROR (Status)) {

    Status = BarExisted (
              PciIoDevice,
              0x28,
              NULL,
              NULL
              );

    if (!EFI_ERROR (Status)) {
      PciIoDevice->Decodes |= EFI_BRIDGE_PMEM32_DECODE_SUPPORTED;
      PciIoDevice->Decodes |= EFI_BRIDGE_PMEM64_DECODE_SUPPORTED;
    } else {
      PciIoDevice->Decodes |= EFI_BRIDGE_PMEM32_DECODE_SUPPORTED;
    }
  }

  //
  // Memory 32 code is required for ppb
  //
  PciIoDevice->Decodes |= EFI_BRIDGE_MEM32_DECODE_SUPPORTED;

  return PciIoDevice;
}

PCI_IO_DEVICE *
GatherP2CInfo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  PCI_IO_DEVICE         *PciIoDevice;
  
  PciIoDevice = CreatePciIoDevice (
                  PciRootBridgeIo,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (!PciIoDevice) {
    return NULL;
  }

  if (gFullEnumeration) {
    PciDisableCommandRegister (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

    //
    // Initalize the bridge control register
    //
    PciDisableBridgeControlRegister (PciIoDevice, EFI_PCCARD_BRIDGE_CONTROL_BITS_OWNED);

  }
  //
  // P2C only has one bar that is in 0x10
  //
  PciParseBar(PciIoDevice, 0x10, 0);
  
  PciIoDevice->Decodes = EFI_BRIDGE_MEM32_DECODE_SUPPORTED  |
                         EFI_BRIDGE_PMEM32_DECODE_SUPPORTED |
                         EFI_BRIDGE_IO32_DECODE_SUPPORTED;

  return PciIoDevice;
}

EFI_DEVICE_PATH_PROTOCOL *
CreatePciDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *ParentDevicePath,
  IN  PCI_IO_DEVICE            *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{

  PCI_DEVICE_PATH PciNode;

  //
  // Create PCI device path
  //
  PciNode.Header.Type     = HARDWARE_DEVICE_PATH;
  PciNode.Header.SubType  = HW_PCI_DP;
  SetDevicePathNodeLength (&PciNode.Header, sizeof (PciNode));

  PciNode.Device          = PciIoDevice->DeviceNumber;
  PciNode.Function        = PciIoDevice->FunctionNumber;
  PciIoDevice->DevicePath = AppendDevicePathNode (ParentDevicePath, &PciNode.Header);

  return PciIoDevice->DevicePath;
}

EFI_STATUS
BarExisted (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINTN         Offset,
  OUT UINT32       *BarLengthValue,
  OUT UINT32       *OriginalBarValue
  )
/*++

Routine Description:

  Check the bar is existed or not.

Arguments:

  PciIoDevice       - A pointer to the PCI_IO_DEVICE.
  Offset            - The offset.
  BarLengthValue    - The bar length value.
  OriginalBarValue  - The original bar value.

Returns:

  EFI_NOT_FOUND     - The bar don't exist.
  EFI_SUCCESS       - The bar exist.

--*/
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT32              OriginalValue;
  UINT32              Value;
  EFI_TPL             OldTpl;

  PciIo = &PciIoDevice->PciIo;

  //
  // Preserve the original value
  //

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &OriginalValue);

  //
  // Raise TPL to high level to disable timer interrupt while the BAR is probed
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &gAllOne);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &Value);

  //
  // Write back the original value
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &OriginalValue);

  //
  // Restore TPL to its original level
  //
  gBS->RestoreTPL (OldTpl);

  if (BarLengthValue != NULL) {
    *BarLengthValue = Value;
  }

  if (OriginalBarValue != NULL) {
    *OriginalBarValue = OriginalValue;
  }

  if (Value == 0) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_SUCCESS;
  }
}


EFI_STATUS
DetermineDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice
  )
/*++

Routine Description:
 
  Determine the related attributes of all devices under a Root Bridge

Arguments:

Returns:

  None

--*/
{
  UINT16          Command;
  UINT16          BridgeControl;

  Command = 0;

  PciIoDevice->Supports |= EFI_PCI_DEVICE_ENABLE;
  PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE;

  if (IS_PCI_VGA (&(PciIoDevice->Pci))){

    //
    // If the device is VGA, VGA related Attributes are supported
    //
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO ;
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY   ;
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_VGA_IO    ;
  }

  if(IS_ISA_BRIDGE(&(PciIoDevice->Pci)) || IS_INTEL_ISA_BRIDGE(&(PciIoDevice->Pci))) {
    //
    // If the devie is a ISA Bridge, set the two attributes
    //
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO;
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_ISA_IO;
  }

  if (IS_PCI_GFX (&(PciIoDevice->Pci))) {

    //
    // If the device is GFX, then only set the EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO
    // attribute
    //
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO    ;
  }


  //
  // If the device is IDE, IDE related attributes are supported
  //
  if (IS_PCI_IDE (&(PciIoDevice->Pci))) {
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO  ;
    PciIoDevice->Supports |= EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO  ;
  }

  PciReadCommandRegister(PciIoDevice, &Command);

  
  if (Command & EFI_PCI_COMMAND_IO_SPACE) {
    PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_IO;
  }

  if (Command & EFI_PCI_COMMAND_MEMORY_SPACE) {
    PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_MEMORY;
  }

  if (Command & EFI_PCI_COMMAND_BUS_MASTER) {
    PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_BUS_MASTER;
  }

  if (IS_PCI_BRIDGE (&(PciIoDevice->Pci)) || 
      IS_CARDBUS_BRIDGE (&(PciIoDevice->Pci))){

    //
    // If it is a PPB, read the Bridge Control Register to determine
    // the relevant attributes
    //
    BridgeControl = 0;
    PciReadBridgeControlRegister(PciIoDevice, &BridgeControl);

    //
    // Determine whether the ISA bit is set
    // If ISA Enable on Bridge is set, the PPB
    // will block forwarding 0x100-0x3ff for each 1KB in the 
    // first 64KB I/O range.
    //
    if ((BridgeControl & EFI_PCI_BRIDGE_CONTROL_ISA) != 0) {
      PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_ISA_IO;
    } 

    //
    // Determine whether the VGA bit is set
    // If it is set, the bridge is set to decode VGA memory range
    // and palette register range
    //
    if (IS_PCI_VGA (&(PciIoDevice->Pci)) &&BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA) {
      PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO;
      PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY;
      PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
    }

    //
    // if the palette snoop bit is set, then the brige is set to 
    // decode palette IO write
    //
    if (Command & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) {
      PciIoDevice->Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
    }
  } 

  return EFI_SUCCESS;
}

UINTN
PciParseBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT32      Value;
  UINT32      OriginalValue;
  UINT32      Mask;
  EFI_STATUS  Status;

  OriginalValue = 0;
  Value         = 0;

  Status = BarExisted (
            PciIoDevice,
            Offset,
            &Value,
            &OriginalValue
            );

  if (EFI_ERROR (Status)) {
    PciIoDevice->PciBar[BarIndex].BaseAddress = 0;
    PciIoDevice->PciBar[BarIndex].Length      = 0;
    PciIoDevice->PciBar[BarIndex].Alignment   = 0;

    //
    // Some devices don't fully comply to PCI spec 2.2. So be to scan all the BARs anyway
    //
    PciIoDevice->PciBar[BarIndex].Offset = (UINT8) Offset;
    return Offset + 4;
  }

  PciIoDevice->PciBar[BarIndex].Offset = (UINT8) Offset;
  if (Value & 0x01) {
    //
    // Device I/Os
    //
    Mask = 0xfffffffc;

    if (Value & 0xFFFF0000) {
      //
      // It is a IO32 bar
      //
      PciIoDevice->PciBar[BarIndex].BarType   = PciBarTypeIo32;
      PciIoDevice->PciBar[BarIndex].Length    = ((~(Value & Mask)) + 1);
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

    } else {
      //
      // It is a IO16 bar
      //
      PciIoDevice->PciBar[BarIndex].BarType   = PciBarTypeIo16;
      PciIoDevice->PciBar[BarIndex].Length    = 0x0000FFFF & ((~(Value & Mask)) + 1);
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

    }
    //
    // Workaround. Some platforms inplement IO bar with 0 length
    // Need to treat it as no-bar
    //
    if (PciIoDevice->PciBar[BarIndex].Length == 0) {
      PciIoDevice->PciBar[BarIndex].BarType = PciBarTypeUnknown;
    }

    PciIoDevice->PciBar[BarIndex].Prefetchable  = FALSE;
    PciIoDevice->PciBar[BarIndex].BaseAddress   = OriginalValue & Mask;

  } else {

    Mask  = 0xfffffff0;

    PciIoDevice->PciBar[BarIndex].BaseAddress = OriginalValue & Mask;

    switch (Value & 0x07) {

    //
    //memory space; anywhere in 32 bit address space
    //
    case 0x00:
      if (Value & 0x08) {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypePMem32;
      } else {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypeMem32;
      }

      PciIoDevice->PciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      break;

    //
    // memory space; anywhere in 64 bit address space
    //
    case 0x04:
      if (Value & 0x08) {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypePMem64;
      } else {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypeMem64;
      }

      //
      // According to PCI 2.2,if the bar indicates a memory 64 decoding, next bar
      // is regarded as an extension for the first bar. As a result
      // the sizing will be conducted on combined 64 bit value
      // Here just store the masked first 32bit value for future size
      // calculation
      //
      PciIoDevice->PciBar[BarIndex].Length    = Value & Mask;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      //
      // Increment the offset to point to next DWORD
      //
      Offset += 4;

      Status = BarExisted (
                PciIoDevice,
                Offset,
                &Value,
                &OriginalValue
                );

      if (EFI_ERROR (Status)) {
        return Offset + 4;
      }

      //
      // Fix the length to support some spefic 64 bit BAR
      //
      Value |= ((UINT32)(-1) << HighBitSet32 (Value)); 

      //
      // Calculate the size of 64bit bar
      //
      PciIoDevice->PciBar[BarIndex].BaseAddress |= LShiftU64 ((UINT64) OriginalValue, 32);

      PciIoDevice->PciBar[BarIndex].Length    = PciIoDevice->PciBar[BarIndex].Length | LShiftU64 ((UINT64) Value, 32);
      PciIoDevice->PciBar[BarIndex].Length    = (~(PciIoDevice->PciBar[BarIndex].Length)) + 1;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      break;

    //
    // reserved
    //
    default:
      PciIoDevice->PciBar[BarIndex].BarType   = PciBarTypeUnknown;
      PciIoDevice->PciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      break;
    }
  }
  
  //
  // Check the length again so as to keep compatible with some special bars
  //
  if (PciIoDevice->PciBar[BarIndex].Length == 0) {
    PciIoDevice->PciBar[BarIndex].BarType     = PciBarTypeUnknown;
    PciIoDevice->PciBar[BarIndex].BaseAddress = 0;
    PciIoDevice->PciBar[BarIndex].Alignment   = 0;
  }
  
  //
  // Increment number of bar
  //
  return Offset + 4;
}

EFI_STATUS
InitializePPB (
  IN PCI_IO_DEVICE *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  EFI_PCI_IO_PROTOCOL *PciIo;

  PciIo = &(PciIoDevice->PciIo);

  //
  // Put all the resource apertures including IO16
  // Io32, pMem32, pMem64 to quiescent state
  // Resource base all ones, Resource limit all zeros
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1D, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x20, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x22, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x24, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x26, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x28, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x2C, 1, &gAllZero);

  //
  // don't support use io32 as for now
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x30, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x32, 1, &gAllZero);

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeP2C (
  IN PCI_IO_DEVICE *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  EFI_PCI_IO_PROTOCOL *PciIo;

  PciIo = &(PciIoDevice->PciIo);

  //
  // Put all the resource apertures including IO16
  // Io32, pMem32, pMem64 to quiescent state(
  // Resource base all ones, Resource limit all zeros
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x1c, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x20, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x24, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x28, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x2c, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x30, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x34, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x38, 1, &gAllZero);

  return EFI_SUCCESS;
}

PCI_IO_DEVICE *
CreatePciIoDevice (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{

  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = NULL;

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (PCI_IO_DEVICE),
                  (VOID **) &PciIoDevice
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem (PciIoDevice, sizeof (PCI_IO_DEVICE));

  PciIoDevice->Signature        = PCI_IO_DEVICE_SIGNATURE;
  PciIoDevice->Handle           = NULL;
  PciIoDevice->PciRootBridgeIo  = PciRootBridgeIo;
  PciIoDevice->DevicePath       = NULL;
  PciIoDevice->BusNumber        = Bus;
  PciIoDevice->DeviceNumber     = Device;
  PciIoDevice->FunctionNumber   = Func;
  PciIoDevice->Decodes          = 0;
  if (gFullEnumeration) {
    PciIoDevice->Allocated = FALSE;
  } else {
    PciIoDevice->Allocated = TRUE;
  }

  PciIoDevice->Attributes         = 0;
  PciIoDevice->Supports           = 0;
  PciIoDevice->IsPciExp           = FALSE;

  CopyMem (&(PciIoDevice->Pci), Pci, sizeof (PCI_TYPE01));

  //
  // Initialize the PCI I/O instance structure
  //

  InitializePciIoInstance (PciIoDevice);

  if (EFI_ERROR (Status)) {
    gBS->FreePool (PciIoDevice);
    return NULL;
  }

  //
  // Initialize the reserved resource list
  //
  InitializeListHead (&PciIoDevice->ReservedResourceList);

  //
  // Initialize the child list
  //
  InitializeListHead (&PciIoDevice->ChildList);

  return PciIoDevice;
}

EFI_STATUS
PciEnumeratorLight (
  IN EFI_HANDLE                    Controller
  )
/*++

Routine Description:

  This routine is used to enumerate entire pci bus system 
  in a given platform

Arguments:

Returns:

  None

--*/
{

  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *PciRootBridgeIo;
  PCI_IO_DEVICE                     *RootBridgeDev;
  UINT16                            MinBus;
  UINT16                            MaxBus;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;

  MinBus      = 0;
  MaxBus      = PCI_MAX_BUS;
  Descriptors = NULL;

  //
  // If this host bridge has been already enumerated, then return successfully
  //
  if (RootBridgeExisted (Controller)) {
    return EFI_SUCCESS;
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller      ,   
                  &gEfiDevicePathProtocolGuid,  
                  (VOID **)&ParentDevicePath,
                  gPciBusDriverBinding.DriverBindingHandle,     
                  Controller,   
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  //
  // Open pci root bridge io protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  //
  // ** CHANGE **
  // PCI Option ROM support removed (continuation of patch by nms42).
  //

  Status = PciRootBridgeIo->Configuration (PciRootBridgeIo, (VOID **) &Descriptors);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (PciGetBusRange (&Descriptors, &MinBus, &MaxBus, NULL) == EFI_SUCCESS) {

    //
    // Create a device node for root bridge device with a NULL host bridge controller handle
    //
    RootBridgeDev = CreateRootBridge (Controller);

    //
    // Record the root bridge device path
    //
    RootBridgeDev->DevicePath = ParentDevicePath;

    //
    // Record the root bridge io protocol
    //
    RootBridgeDev->PciRootBridgeIo = PciRootBridgeIo;

    Status = PciPciDeviceInfoCollector (
              RootBridgeDev,
              (UINT8) MinBus
              );

    if (!EFI_ERROR (Status)) {

      //
      // If successfully, insert the node into device pool
      //
      InsertRootBridge (RootBridgeDev);
    } else {

      //
      // If unsuccessly, destroy the entire node
      //
      DestroyRootBridge (RootBridgeDev);
    }

    Descriptors++;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PciGetBusRange (
  IN     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
  OUT    UINT16                             *MinBus,
  OUT    UINT16                             *MaxBus,
  OUT    UINT16                             *BusRange
  )
/*++

Routine Description:

  Get the bus range.

Arguments:

  Descriptors     - A pointer to the address space descriptor.
  MinBus          - The min bus.
  MaxBus          - The max bus.
  BusRange        - The bus range.
  
Returns:
  
  Status Code.

--*/
{

  while ((*Descriptors)->Desc != ACPI_END_TAG_DESCRIPTOR) {
    if ((*Descriptors)->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
      if (MinBus != NULL) {
        *MinBus = (UINT16)(*Descriptors)->AddrRangeMin;
      }

      if (MaxBus != NULL) {
        *MaxBus = (UINT16)(*Descriptors)->AddrRangeMax;
      }

      if (BusRange != NULL) {
        *BusRange = (UINT16)(*Descriptors)->AddrLen;
      }
      return EFI_SUCCESS;
    }

    (*Descriptors)++;
  }

  return EFI_NOT_FOUND;
}


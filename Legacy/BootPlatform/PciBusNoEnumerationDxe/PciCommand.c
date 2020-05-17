/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciCommand.c
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#include "PciBus.h"


EFI_STATUS 
PciReadCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  OUT UINT16       *Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{

  EFI_PCI_IO_PROTOCOL   *PciIo;

  *Command = 0;
  PciIo = &PciIoDevice->PciIo;

  return PciIo->Pci.Read (
                PciIo, 
                EfiPciIoWidthUint16, 
                PCI_COMMAND_OFFSET, 
                1, 
                Command
                );
}
  
EFI_STATUS 
PciSetCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT16                Temp;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  
  Temp = Command;
  PciIo = &PciIoDevice->PciIo;

  return PciIo->Pci.Write (
              PciIo, 
              EfiPciIoWidthUint16, 
              PCI_COMMAND_OFFSET, 
              1, 
              &Temp
              );
  
}


EFI_STATUS 
PciEnableCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT16  OldCommand;
  EFI_PCI_IO_PROTOCOL   *PciIo;

  OldCommand = 0;
  PciIo = &PciIoDevice->PciIo;

  PciIo->Pci.Read (
          PciIo, 
          EfiPciIoWidthUint16, 
          PCI_COMMAND_OFFSET, 
          1, 
          &OldCommand
          );

  OldCommand = (UINT16) (OldCommand | Command);

  return PciIo->Pci.Write (
              PciIo, 
              EfiPciIoWidthUint16, 
              PCI_COMMAND_OFFSET, 
              1, 
              &OldCommand
              );
  
}


EFI_STATUS 
PciDisableCommandRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT16  OldCommand;
  EFI_PCI_IO_PROTOCOL   *PciIo;

  OldCommand = 0;
  PciIo = &PciIoDevice->PciIo;

  PciIo->Pci.Read (
          PciIo, 
          EfiPciIoWidthUint16, 
          PCI_COMMAND_OFFSET, 
          1, 
          &OldCommand
          );

  OldCommand = (UINT16) (OldCommand & ~(Command));

  return PciIo->Pci.Write (
            PciIo, 
            EfiPciIoWidthUint16, 
            PCI_COMMAND_OFFSET, 
            1, 
            &OldCommand
           );
  
}



EFI_STATUS 
PciSetBridgeControlRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT16                Temp;
  EFI_PCI_IO_PROTOCOL   *PciIo;

  Temp = Command;
  PciIo = &PciIoDevice->PciIo;

  return PciIo->Pci.Write (
                PciIo, 
                EfiPciIoWidthUint16, 
                PCI_BRIDGE_CONTROL_REGISTER_OFFSET, 
                1, 
                &Temp
               );
 
}


EFI_STATUS 
PciEnableBridgeControlRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT16  OldCommand;
  EFI_PCI_IO_PROTOCOL   *PciIo;

  OldCommand = 0;
  PciIo = &PciIoDevice->PciIo;

  PciIo->Pci.Read (
          PciIo, 
          EfiPciIoWidthUint16, 
          PCI_BRIDGE_CONTROL_REGISTER_OFFSET, 
          1, 
          &OldCommand
          );

  OldCommand = (UINT16) (OldCommand | Command);

  return PciIo->Pci.Write (
              PciIo, 
              EfiPciIoWidthUint16, 
              PCI_BRIDGE_CONTROL_REGISTER_OFFSET, 
              1, 
              &OldCommand
             );
  
}

EFI_STATUS 
PciDisableBridgeControlRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINT16        Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  UINT16  OldCommand;
  EFI_PCI_IO_PROTOCOL   *PciIo;

  OldCommand = 0;
  PciIo = &PciIoDevice->PciIo;

  PciIo->Pci.Read (
          PciIo, 
          EfiPciIoWidthUint16, 
          PCI_BRIDGE_CONTROL_REGISTER_OFFSET, 
          1, 
          &OldCommand
          );

  OldCommand = (UINT16) (OldCommand & ~(Command));

  return PciIo->Pci.Write (
              PciIo, 
              EfiPciIoWidthUint16, 
              PCI_BRIDGE_CONTROL_REGISTER_OFFSET, 
              1, 
              &OldCommand
              );
 
}



EFI_STATUS 
PciReadBridgeControlRegister (
  IN PCI_IO_DEVICE *PciIoDevice,
  OUT UINT16       *Command
)
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{

  EFI_PCI_IO_PROTOCOL   *PciIo;

  *Command = 0;
  PciIo = &PciIoDevice->PciIo;

  return PciIo->Pci.Read (
                PciIo, 
                EfiPciIoWidthUint16, 
                PCI_BRIDGE_CONTROL_REGISTER_OFFSET, 
                1, 
                Command
                );
 
}

BOOLEAN
PciCapabilitySupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:
  
  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
{

  if (PciIoDevice->Pci.Hdr.Status & EFI_PCI_STATUS_CAPABILITY) {
    return TRUE;
  }

  return FALSE;
}

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
{
  UINT8   CapabilityPtr;
  UINT16  CapabilityEntry;
  UINT8   CapabilityID;

  //
  // To check the capability of this device supports
  //
  if (!PciCapabilitySupport (PciIoDevice)) {
    return EFI_UNSUPPORTED;
  }

  if (*Offset != 0) {
    CapabilityPtr = *Offset;
  } else {

    CapabilityPtr = 0;
    if (IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {

      PciIoDevice->PciIo.Pci.Read (
                               &PciIoDevice->PciIo,
                               EfiPciIoWidthUint8,
                               EFI_PCI_CARDBUS_BRIDGE_CAPABILITY_PTR,
                               1,
                               &CapabilityPtr
                               );
    } else {

      PciIoDevice->PciIo.Pci.Read (
                               &PciIoDevice->PciIo,
                               EfiPciIoWidthUint8,
                               PCI_CAPBILITY_POINTER_OFFSET,
                               1,
                               &CapabilityPtr
                               );
    }
  }

  while ((CapabilityPtr >= 0x40) && ((CapabilityPtr & 0x03) == 0x00)) {
    PciIoDevice->PciIo.Pci.Read (
                             &PciIoDevice->PciIo,
                             EfiPciIoWidthUint16,
                             CapabilityPtr,
                             1,
                             &CapabilityEntry
                             );

    CapabilityID = (UINT8) CapabilityEntry;

    if (CapabilityID == CapId) {
      *Offset = CapabilityPtr;
      if (NextRegBlock != NULL) {
        *NextRegBlock = (UINT8) (CapabilityEntry >> 8);
      }

      return EFI_SUCCESS;
    }

    CapabilityPtr = (UINT8) (CapabilityEntry >> 8);
  }

  return EFI_NOT_FOUND;
}

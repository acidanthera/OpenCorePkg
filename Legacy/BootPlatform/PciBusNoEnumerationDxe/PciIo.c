/*++

Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciIo.c
  
Abstract:

  PCI I/O Abstraction Driver

Revision History

--*/

#include "PciBus.h"

//
// PCI I/O Support Function Prototypes
//
//

BOOLEAN 
PciDevicesOnTheSamePath (
  IN PCI_IO_DEVICE       *PciDevice1,
  IN PCI_IO_DEVICE       *PciDevice2
);


EFI_STATUS 
UpStreamBridgesAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes
);


BOOLEAN 
CheckBarType ( 
  IN PCI_IO_DEVICE  *PciIoDevice,
  UINT8             BarIndex,
  PCI_BAR_TYPE      BarType
);


EFI_STATUS 
SetBootVGA ( 
  IN  PCI_IO_DEVICE                  *PciIoDevice
);

EFI_STATUS 
DisableBootVGA ( 
  IN  PCI_IO_DEVICE                  *PciIoDevice
);


EFI_STATUS
PciIoVerifyBarAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  UINT8                         BarIndex,
  PCI_BAR_TYPE                  Type,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  UINT64                        *Offset
);

EFI_STATUS
PciIoVerifyConfigAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
);

EFI_STATUS
EFIAPI
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
);
  
EFI_STATUS
EFIAPI
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
);    

EFI_STATUS
EFIAPI
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoIoRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
);

EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN     EFI_PCI_IO_PROTOCOL  *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
);

EFI_STATUS
EFIAPI
PciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
);

EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
);

EFI_STATUS
EFIAPI
PciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL    *This,
  IN  EFI_ALLOCATE_TYPE      Type,
  IN  EFI_MEMORY_TYPE        MemoryType,
  IN  UINTN                  Pages,
  OUT VOID                   **HostAddress,
  IN  UINT64                 Attributes
);

EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  );

EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  );

EFI_STATUS
EFIAPI
PciIoAttributes (
  IN  EFI_PCI_IO_PROTOCOL              *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result   OPTIONAL
  );

EFI_STATUS
EFIAPI
PciIoGetBarAttributes(
  IN  EFI_PCI_IO_PROTOCOL    *This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports,   OPTIONAL
  OUT VOID                           **Resources  OPTIONAL
  );

EFI_STATUS
EFIAPI
PciIoSetBarAttributes(
  IN     EFI_PCI_IO_PROTOCOL  *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  );


//
// Pci Io Protocol Interface
//
EFI_PCI_IO_PROTOCOL  PciIoInterface = {
  PciIoPollMem,
  PciIoPollIo,
  {
    PciIoMemRead,
    PciIoMemWrite
  },
  {
    PciIoIoRead,
    PciIoIoWrite
  },
  {
    PciIoConfigRead,
    PciIoConfigWrite
  },
  PciIoCopyMem,
  PciIoMap,
  PciIoUnmap,
  PciIoAllocateBuffer,
  PciIoFreeBuffer,
  PciIoFlush,
  PciIoGetLocation,
  PciIoAttributes,
  PciIoGetBarAttributes,
  PciIoSetBarAttributes,
  0,
  NULL
};


EFI_STATUS
InitializePciIoInstance (
  PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

  Initializes a PCI I/O Instance

Arguments:
  
Returns:

  None

--*/

{
  CopyMem (&PciIoDevice->PciIo, &PciIoInterface, sizeof (EFI_PCI_IO_PROTOCOL));
  return EFI_SUCCESS;
}

EFI_STATUS
PciIoVerifyBarAccess (
  PCI_IO_DEVICE                   *PciIoDevice,
  UINT8                           BarIndex,
  PCI_BAR_TYPE                    Type,
  IN EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN UINTN                        Count,
  UINT64                          *Offset
  )
/*++

Routine Description:

  Verifies access to a PCI Base Address Register (BAR)

Arguments:

Returns:

  None

--*/
{
  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (BarIndex == EFI_PCI_IO_PASS_THROUGH_BAR) {
    return EFI_SUCCESS;
  }

  //
  // BarIndex 0-5 is legal
  //
  if (BarIndex >= PCI_MAX_BAR) {
    return EFI_INVALID_PARAMETER;
  }

  if (!CheckBarType (PciIoDevice, BarIndex, Type)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Width is EfiPciIoWidthFifoUintX then convert to EfiPciIoWidthUintX
  // If Width is EfiPciIoWidthFillUintX then convert to EfiPciIoWidthUintX
  //
  if (Width >= EfiPciIoWidthFifoUint8 && Width <= EfiPciIoWidthFifoUint64) {
    Count = 1;
  }

  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);

  if ((*Offset + Count * ((UINTN)1 << Width)) - 1 >= PciIoDevice->PciBar[BarIndex].Length) {
    return EFI_INVALID_PARAMETER;
  }

  *Offset = *Offset + PciIoDevice->PciBar[BarIndex].BaseAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
PciIoVerifyConfigAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
  )
/*++

Routine Description:

  Verifies access to a PCI Config Header

Arguments:

Returns:

  None

--*/
{
  UINT64  ExtendOffset;

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Width is EfiPciIoWidthFifoUintX then convert to EfiPciIoWidthUintX
  // If Width is EfiPciIoWidthFillUintX then convert to EfiPciIoWidthUintX
  //
  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);

  if (PciIoDevice->IsPciExp) {
    if ((*Offset + Count * ((UINTN)1 << Width)) - 1 >= PCI_EXP_MAX_CONFIG_OFFSET) {
      return EFI_UNSUPPORTED;
    }

    ExtendOffset  = LShiftU64 (*Offset, 32);
    *Offset       = EFI_PCI_ADDRESS (PciIoDevice->BusNumber, PciIoDevice->DeviceNumber, PciIoDevice->FunctionNumber, 0);
    *Offset       = (*Offset) | ExtendOffset;

  } else {
    if ((*Offset + Count * ((UINTN)1 << Width)) - 1 >= PCI_MAX_CONFIG_OFFSET) {
      return EFI_UNSUPPORTED;
    }

    *Offset = EFI_PCI_ADDRESS (PciIoDevice->BusNumber, PciIoDevice->DeviceNumber, PciIoDevice->FunctionNumber, *Offset);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
/*++

Routine Description:

  Poll PCI Memmory

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoDevice->PciRootBridgeIo->PollMem (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Offset,
                                          Mask,
                                          Value,
                                          Delay,
                                          Result
                                          );
  return Status;
}

EFI_STATUS
EFIAPI
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
/*++

Routine Description:

  Poll PCI IO

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->PollIo (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Offset,
                                          Mask,
                                          Value,
                                          Delay,
                                          Result
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Memory Read Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Mem.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Memory Write Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }
  
  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Mem.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoIoRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI I/O Read Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Io.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI I/O Write Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Buffer == NULL){
    return EFI_INVALID_PARAMETER;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Io.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Configuration Read Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  UINT64        Address;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Address     = Offset;
  Status      = PciIoVerifyConfigAccess (PciIoDevice, Width, Count, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIoDevice->PciRootBridgeIo->Pci.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Address,
                                              Count,
                                              Buffer
                                              );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Configuration Write Cycle

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  UINT64        Address;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Address     = Offset;
  Status      = PciIoVerifyConfigAccess (PciIoDevice, Width, Count, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIoDevice->PciRootBridgeIo->Pci.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Address,
                                              Count,
                                              Buffer
                                              );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  )
/*++

Routine Description:

  Copy PCI Memory

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == EfiPciIoWidthFifoUint8  ||
      Width == EfiPciIoWidthFifoUint16 ||
      Width == EfiPciIoWidthFifoUint32 ||
      Width == EfiPciIoWidthFifoUint64 ||
      Width == EfiPciIoWidthFillUint8  ||
      Width == EfiPciIoWidthFillUint16 ||
      Width == EfiPciIoWidthFillUint32 ||
      Width == EfiPciIoWidthFillUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, DestBarIndex, PciBarTypeMem, Width, Count, &DestOffset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, SrcBarIndex, PciBarTypeMem, Width, Count, &SrcOffset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->CopyMem (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          DestOffset,
                                          SrcOffset,
                                          Count
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
/*++

Routine Description:

  Maps a memory region for DMA

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Operation >= EfiPciIoOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (HostAddress == NULL || NumberOfBytes == NULL || DeviceAddress == NULL || Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) {
    Operation = (EFI_PCI_IO_PROTOCOL_OPERATION) (Operation + EfiPciOperationBusMasterRead64);
  }

  Status = PciIoDevice->PciRootBridgeIo->Map (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION) Operation,
                                          HostAddress,
                                          NumberOfBytes,
                                          DeviceAddress,
                                          Mapping
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  )
/*++

Routine Description:

  Unmaps a memory region for DMA

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Status = PciIoDevice->PciRootBridgeIo->Unmap (
                                          PciIoDevice->PciRootBridgeIo,
                                          Mapping
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 Pages,
  OUT VOID                  **HostAddress,
  IN  UINT64                Attributes
  )
/*++

Routine Description:

  Allocates a common buffer for DMA

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Attributes &
      (~(EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE | EFI_PCI_ATTRIBUTE_MEMORY_CACHED))) {
    return EFI_UNSUPPORTED;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) {
    Attributes |= EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE;
  }

  Status = PciIoDevice->PciRootBridgeIo->AllocateBuffer (
                                          PciIoDevice->PciRootBridgeIo,
                                          Type,
                                          MemoryType,
                                          Pages,
                                          HostAddress,
                                          Attributes
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
/*++

Routine Description:

  Frees a common buffer 

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  
  if( HostAddress == NULL ){
     return EFI_INVALID_PARAMETER;
  } 

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Status = PciIoDevice->PciRootBridgeIo->FreeBuffer (
                                          PciIoDevice->PciRootBridgeIo,
                                          Pages,
                                          HostAddress
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  )
/*++

Routine Description:

  Flushes a DMA buffer

Arguments:

Returns:

  None

--*/

{
  EFI_STATUS    Status;
  UINT32         Register;
  PCI_IO_DEVICE  *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  //
  // If the device is behind a PCI-PCI Bridge, then perform a read cycles to the device to 
  // flush the posted write cycles through the PCI-PCI bridges
  //
  if (PciIoDevice->Parent != NULL) {
    Status = This->Pci.Read (This, EfiPciIoWidthUint32, 0, 1, &Register);
  }

  //
  // Call the PCI Root Bridge I/O Protocol to flush the posted write cycles through the chipset
  //
  Status = PciIoDevice->PciRootBridgeIo->Flush (
                                           PciIoDevice->PciRootBridgeIo
                                           );

  return Status;
}

EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  )
/*++

Routine Description:

  Gets a PCI device's current bus number, device number, and function number.

Arguments:

Returns:

  None

--*/
{
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Segment == NULL || Bus == NULL || Device == NULL || Function == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Segment  = PciIoDevice->PciRootBridgeIo->SegmentNumber;
  *Bus      = PciIoDevice->BusNumber;
  *Device   = PciIoDevice->DeviceNumber;
  *Function = PciIoDevice->FunctionNumber;

  return EFI_SUCCESS;
}

BOOLEAN
CheckBarType (
  IN PCI_IO_DEVICE       *PciIoDevice,
  UINT8                  BarIndex,
  PCI_BAR_TYPE           BarType
  )
/*++

Routine Description:

  Sets a PCI controllers attributes on a resource range

Arguments:

Returns:

  None

--*/
{
  switch (BarType) {

  case PciBarTypeMem:

    if (PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeMem32  &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypePMem32 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypePMem64 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeMem64    ) {
      return FALSE;
    }

    return TRUE;

  case PciBarTypeIo:
    if (PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeIo32 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeIo16){
      return FALSE;
    }

    return TRUE;

  default:
    break;
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
PciIoAttributes (
  IN EFI_PCI_IO_PROTOCOL                       * This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
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
  PCI_IO_DEVICE *Temp;
  UINT64         NewAttributes;
  UINT64         PciRootBridgeSupports;
  UINT64         PciRootBridgeAttributes;
  UINT64         NewPciRootBridgeAttributes;
  UINT64         NewUpStreamBridgeAttributes;
  UINT64         ModifiedPciRootBridgeAttributes;
  UINT16         EnableCommand;
  UINT16         DisableCommand;
  UINT16         EnableBridge;
  UINT16         DisableBridge;
  UINT16         Command;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);
  NewUpStreamBridgeAttributes = 0;

  EnableCommand   = 0;
  DisableCommand  = 0;
  EnableBridge    = 0;
  DisableBridge   = 0;

  switch (Operation) {
  case EfiPciIoAttributeOperationGet:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    *Result = PciIoDevice->Attributes;
    return EFI_SUCCESS;

  case EfiPciIoAttributeOperationSupported:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    *Result = PciIoDevice->Supports;
    return EFI_SUCCESS;

  case EfiPciIoAttributeOperationEnable:
    if(Attributes & ~(PciIoDevice->Supports)) {
      return EFI_UNSUPPORTED;
    }
    NewAttributes = PciIoDevice->Attributes | Attributes;
    break;
  case EfiPciIoAttributeOperationDisable:
    if(Attributes & ~(PciIoDevice->Supports)) {
      return EFI_UNSUPPORTED;
    }
    NewAttributes = PciIoDevice->Attributes & (~Attributes);
    break;
  case EfiPciIoAttributeOperationSet:
    if(Attributes & ~(PciIoDevice->Supports)) {
      return EFI_UNSUPPORTED;
    }
    NewAttributes = Attributes;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  //
  // If VGA_IO is set, then set VGA_MEMORY too.  This driver can not enable them seperately.
  //
  if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO) {
    NewAttributes |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY;
  }

  //
  // If VGA_MEMORY is set, then set VGA_IO too.  This driver can not enable them seperately.
  //
  if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY) {
    NewAttributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO;
  }

  //
  // If the attributes are already set correctly, then just return EFI_SUCCESS;
  //
  if ((NewAttributes ^ PciIoDevice->Attributes) == 0) {
    return EFI_SUCCESS;
  }

  //
  // This driver takes care of EFI_PCI_IO_ATTRIBUTE_IO, EFI_PCI_IO_ATTRIBUTE_MEMORY, and
  // EFI_PCI_IO_ATTRIBUTE_BUS_MASTER.  Strip these 3 bits off the new attribute mask so
  // a call to the PCI Root Bridge I/O Protocol can be made
  //

  if (!IS_PCI_BRIDGE(&PciIoDevice->Pci)) {
    NewPciRootBridgeAttributes = NewAttributes & (~(EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_MEMORY | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER | EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE));

    //  
    // Get the current attributes of this PCI device's PCI Root Bridge
    //
    Status = PciIoDevice->PciRootBridgeIo->GetAttributes (
                                            PciIoDevice->PciRootBridgeIo,
                                            &PciRootBridgeSupports,
                                            &PciRootBridgeAttributes
                                            );

    //
    // Check to see if any of the PCI Root Bridge attributes are being modified
    //
    ModifiedPciRootBridgeAttributes = NewPciRootBridgeAttributes ^ PciRootBridgeAttributes;
    if (ModifiedPciRootBridgeAttributes) {

      //
      // Check to see if the PCI Root Bridge supports modifiying the attributes that are changing
      //
      if ((ModifiedPciRootBridgeAttributes & PciRootBridgeSupports) != ModifiedPciRootBridgeAttributes) {
  //     return EFI_UNSUPPORTED;
      }
      //
      // Call the PCI Root Bridge to attempt to modify the attributes
      //
      Status = PciIoDevice->PciRootBridgeIo->SetAttributes (
                                             PciIoDevice->PciRootBridgeIo,
                                             NewPciRootBridgeAttributes,
                                             NULL,
                                             NULL
                                             );
      if (EFI_ERROR (Status)) {
      //
      // The PCI Root Bridge could not modify the attributes, so return the error.
      //
        return Status;
      }
    }
  }


  if (IS_PCI_BRIDGE(&PciIoDevice->Pci)) {


    //
    // Check to see if an VGA related attributes are being set.
    //
    if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO)) {

      if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO) {
        EnableBridge  |= EFI_PCI_BRIDGE_CONTROL_VGA;
      } else {
        DisableBridge  |= EFI_PCI_BRIDGE_CONTROL_VGA;
      }
    }

    //
    // Check to see if an VGA related attributes are being set.
    // If ISA Enable on the PPB is set, the PPB will block the
    // 0x100-0x3FF for each 1KB block in the first 64K I/O block
    //
    if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_ISA_IO) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_ISA_IO)) {

      if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_ISA_IO) {
        DisableBridge  |= EFI_PCI_BRIDGE_CONTROL_ISA;
      } else {
        EnableBridge  |= EFI_PCI_BRIDGE_CONTROL_ISA;
      }
    }

    //
    // Check to see if an VGA related attributes are being set.
    //
    if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO)) {

      if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO) {
        EnableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
      } else {
        DisableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
      }
    }

  } else {

    if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO)) {

      if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO) {

        //
        //Check if there have been an active VGA device on the same segment
        //
        Temp = ActiveVGADeviceOnTheSameSegment (PciIoDevice);

        if (Temp && Temp != PciIoDevice) {
          return EFI_UNSUPPORTED;
        }
      }
    }

    if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO)) {
      if (IS_PCI_GFX(&PciIoDevice->Pci)) {

        //
        //Get the boot VGA on the same segment
        //
        Temp = ActiveVGADeviceOnTheSameSegment (PciIoDevice);

        if (!Temp) {

          //
          // If there is no VGA device on the segment, set
          // this graphics card to decode the palette range
          //
          DisableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
        } else {

          //
          // Check these two agents are on the same path
          //
          if (PciDevicesOnTheSamePath(Temp, PciIoDevice)) {

            //
            // Check if they are on the same bus
            //
            if (Temp->Parent == PciIoDevice->Parent) {

              PciReadCommandRegister (Temp, &Command);

              //
              // If they are on the same bus, either one can
              // be set to snoop, the other set to decode
              //
              if (Command & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) {
                DisableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
              } else {
                EnableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
              }
            } else {

              //
              // If they are on  the same path but on the different bus
              // The first agent is set to snoop, the second one set to
              // decode
              //
              if (Temp->BusNumber > PciIoDevice->BusNumber) {
                PciEnableCommandRegister(Temp,EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
                DisableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
              } else {
                PciDisableCommandRegister(Temp,EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
                EnableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
              }
            }
          } else {

            EnableCommand  |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
          }
        }
      }
    }
  }

  //
  // Check to see of the I/O enable is being modified
  //
  if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_IO) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_IO)) {
    if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_IO) {
      EnableCommand |= EFI_PCI_COMMAND_IO_SPACE;
    } else {
      DisableCommand |= EFI_PCI_COMMAND_IO_SPACE;
    }
  }

  //
  // Check to see of the Memory enable is being modified
  //
  if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_MEMORY) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_MEMORY)) {
    if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_MEMORY) {
      EnableCommand |= EFI_PCI_COMMAND_MEMORY_SPACE;
    } else {
      DisableCommand |= EFI_PCI_COMMAND_MEMORY_SPACE;
    }
  }

  //
  // Check to see of the Bus Master enable is being modified
  //
  if ((NewAttributes & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER) ^ (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER)) {
    if (NewAttributes & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER) {
      EnableCommand  |= EFI_PCI_COMMAND_BUS_MASTER;
    } else {
      DisableCommand |= EFI_PCI_COMMAND_BUS_MASTER;
    }
  }

  Status = EFI_SUCCESS;
  if (EnableCommand) {
    Status = PciEnableCommandRegister(PciIoDevice, EnableCommand);
  } 

  if (DisableCommand) {
    Status = PciDisableCommandRegister(PciIoDevice, DisableCommand);
  }

  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }

  if (EnableBridge) {
    Status = PciEnableBridgeControlRegister(PciIoDevice, EnableBridge);
  }

  if (DisableBridge) {
    Status = PciDisableBridgeControlRegister(PciIoDevice, DisableBridge);
  }
   
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Set the upstream bridge attributes
  //
  if (Operation != EfiPciIoAttributeOperationGet && Operation != EfiPciIoAttributeOperationSupported) {

    //
    // EFI_PCI_IO_ATTRIBUTE_MEMORY, EFI_PCI_IO_ATTRIBUTE_IO, EFI_PCI_IO_ATTRIBUTE_BUS_MASTER
    // EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE, EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED
    // EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE will not effect to upstream bridge
    // 
    NewUpStreamBridgeAttributes = Attributes & \
                                (~(EFI_PCI_IO_ATTRIBUTE_IO | \
                                EFI_PCI_IO_ATTRIBUTE_MEMORY | \
                                EFI_PCI_IO_ATTRIBUTE_BUS_MASTER | \
                                EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE | \
                                EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED | \
                                EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE));

    if (NewUpStreamBridgeAttributes){
      UpStreamBridgesAttributes(PciIoDevice, Operation, NewUpStreamBridgeAttributes);
    }
  }
  
  PciIoDevice->Attributes = NewAttributes;

  return Status;
}

EFI_STATUS
EFIAPI
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL             * This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
{

  UINT8                             *Configuration;
  PCI_IO_DEVICE                     *PciIoDevice;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *AddressSpace;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Supports == NULL && Resources == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BarIndex >= PCI_MAX_BAR) || (PciIoDevice->PciBar[BarIndex].BarType == PciBarTypeUnknown)) {
    return EFI_UNSUPPORTED;
  }

  //
  // This driver does not support modifications to the WRITE_COMBINE or
  // CACHED attributes for BAR ranges.
  //
  if (Supports != NULL) {
    *Supports = PciIoDevice->Supports & EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED & EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE;
  }

  if (Resources != NULL) {
    Configuration = AllocateZeroPool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Configuration == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AddressSpace = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;

    AddressSpace->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    AddressSpace->Len          = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);

    AddressSpace->AddrRangeMin = PciIoDevice->PciBar[BarIndex].BaseAddress;
    AddressSpace->AddrLen      = PciIoDevice->PciBar[BarIndex].Length;
    AddressSpace->AddrRangeMax = PciIoDevice->PciBar[BarIndex].Alignment;

    switch (PciIoDevice->PciBar[BarIndex].BarType) {
    case PciBarTypeIo16:
    case PciBarTypeIo32:
      //
      // Io
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_IO;
      break;

    case PciBarTypeMem32:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // 32 bit
      //
      AddressSpace->AddrSpaceGranularity = 32;
      break;

    case PciBarTypePMem32:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // prefechable
      //
      AddressSpace->SpecificFlag = 0x6;
      //
      // 32 bit
      //
      AddressSpace->AddrSpaceGranularity = 32;
      break;

    case PciBarTypeMem64:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // 64 bit
      //
      AddressSpace->AddrSpaceGranularity = 64;
      break;

    case PciBarTypePMem64:
      //
      // Mem
      //
      AddressSpace->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // prefechable
      //
      AddressSpace->SpecificFlag = 0x6;
      //
      // 64 bit
      //
      AddressSpace->AddrSpaceGranularity = 64;
      break;

    default:
      break;
    }

    //
    // put the checksum
    //
    End           = (EFI_ACPI_END_TAG_DESCRIPTOR *) (AddressSpace + 1);
    End->Desc     = ACPI_END_TAG_DESCRIPTOR;
    End->Checksum = 0;

    *Resources    = Configuration;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
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
  UINT64        NonRelativeOffset;
  UINT64        Supports;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  //
  // Make sure Offset and Length are not NULL
  //
  if (Offset == NULL || Length == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PciIoDevice->PciBar[BarIndex].BarType == PciBarTypeUnknown) {
    return EFI_UNSUPPORTED;
  }
  //
  // This driver does not support setting the WRITE_COMBINE or the CACHED attributes.
  // If Attributes is not 0, then return EFI_UNSUPPORTED.
  //
  Supports = PciIoDevice->Supports & EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED & EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE;

  if (Attributes != (Attributes & Supports)) {
    return EFI_UNSUPPORTED;
  }
  //
  // Attributes must be supported.  Make sure the BAR range describd by BarIndex, Offset, and
  // Length are valid for this PCI device.
  //
  NonRelativeOffset = *Offset;
  Status = PciIoVerifyBarAccess (
            PciIoDevice,
            BarIndex,
            PciBarTypeMem,
            EfiPciIoWidthUint8,
            (UINT32) *Length,
            &NonRelativeOffset
            );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
UpStreamBridgesAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  PCI_IO_DEVICE       *Parent;
  EFI_PCI_IO_PROTOCOL *PciIo;

  Parent = PciIoDevice->Parent;

  while (Parent && IS_PCI_BRIDGE (&Parent->Pci)) {

    //
    // Get the PciIo Protocol
    //
    PciIo = &Parent->PciIo;

    PciIo->Attributes (PciIo, Operation, Attributes, NULL);

    Parent = Parent->Parent;
  }

  return EFI_SUCCESS;
}

BOOLEAN
PciDevicesOnTheSamePath (
  IN PCI_IO_DEVICE        *PciDevice1,
  IN PCI_IO_DEVICE        *PciDevice2
  )
/*++

Routine Description:

Arguments:

  PciDevice1  -  The pointer to the first PCI_IO_DEVICE.
  PciDevice2  -  The pointer to the second PCI_IO_DEVICE.

Returns:

  TRUE   -  The two Pci devices are on the same path.
  FALSE  -  The two Pci devices are not on the same path.

--*/
{

  if (PciDevice1->Parent == PciDevice2->Parent) {
    return TRUE;
  }

  return (BOOLEAN) ((PciDeviceExisted (PciDevice1->Parent, PciDevice2)|| PciDeviceExisted (PciDevice2->Parent, PciDevice1)));
}

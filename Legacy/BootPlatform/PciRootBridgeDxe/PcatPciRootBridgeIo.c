/*++

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
    PcatPciRootBridgeIo.c
    
Abstract:

    EFI PC AT PCI Root Bridge Io Protocol

Revision History

--*/

#include "PcatPciRootBridge.h"

//
// Protocol Member Function Prototypes
//
EFI_STATUS
EFIAPI
PcatRootBridgeIoPollMem ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );
  
EFI_STATUS
EFIAPI
PcatRootBridgeIoPollIo ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );
  
EFI_STATUS
EFIAPI
PcatRootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 DestAddress,
  IN UINT64                                 SrcAddress,
  IN UINTN                                  Count
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoPciWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoMap (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoUnmap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN VOID                             *Mapping
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoAllocateBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoFlush (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,
  OUT UINT64                           *Attributes
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength 
  ); 

EFI_STATUS
EFIAPI
PcatRootBridgeIoConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT VOID                             **Resources
  );

//
// Private Function Prototypes
//
EFI_STATUS
EFIAPI
PcatRootBridgeIoMemRW (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                                  Count,
  IN  BOOLEAN                                InStrideFlag,
  IN  PTR                                    In,
  IN  BOOLEAN                                OutStrideFlag,
  OUT PTR                                    Out
  );

EFI_STATUS
PcatRootBridgeIoConstructor (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *Protocol,
  IN UINTN                            SegmentNumber
  )
/*++

Routine Description:

    Contruct the Pci Root Bridge Io protocol

Arguments:

    Protocol - protocol to initialize
    
Returns:

    None

--*/
{
  Protocol->ParentHandle   = NULL;

  Protocol->PollMem        = PcatRootBridgeIoPollMem;
  Protocol->PollIo         = PcatRootBridgeIoPollIo;

  Protocol->Mem.Read       = PcatRootBridgeIoMemRead;
  Protocol->Mem.Write      = PcatRootBridgeIoMemWrite;

  Protocol->Io.Read        = PcatRootBridgeIoIoRead;
  Protocol->Io.Write       = PcatRootBridgeIoIoWrite;

  Protocol->CopyMem        = PcatRootBridgeIoCopyMem;

  Protocol->Pci.Read       = PcatRootBridgeIoPciRead;
  Protocol->Pci.Write      = PcatRootBridgeIoPciWrite;

  Protocol->Map            = PcatRootBridgeIoMap;
  Protocol->Unmap          = PcatRootBridgeIoUnmap;

  Protocol->AllocateBuffer = PcatRootBridgeIoAllocateBuffer;
  Protocol->FreeBuffer     = PcatRootBridgeIoFreeBuffer;

  Protocol->Flush          = PcatRootBridgeIoFlush;

  Protocol->GetAttributes  = PcatRootBridgeIoGetAttributes;
  Protocol->SetAttributes  = PcatRootBridgeIoSetAttributes;

  Protocol->Configuration  = PcatRootBridgeIoConfiguration;

  Protocol->SegmentNumber  = (UINT32)SegmentNumber;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoPollMem ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  )
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;
  UINT32      Remainder;

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  if ((UINT32)Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // No matter what, always do a single poll.
  //
  Status = This->Mem.Read (This, Width, Address, 1, Result);
  if ( EFI_ERROR(Status) ) {
    return Status;
  }    
  if ( (*Result & Mask) == Value ) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  } else {

    NumberOfTicks = DivU64x32Remainder (Delay, 100, &Remainder);
    if ( Remainder !=0 ) {
      NumberOfTicks += 1;
    }
    NumberOfTicks += 1;
  
    while ( NumberOfTicks ) {

      gBS->Stall (10);

      Status = This->Mem.Read (This, Width, Address, 1, Result);
      if ( EFI_ERROR(Status) ) {
        return Status;
      }
    
      if ( (*Result & Mask) == Value ) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }
  return EFI_TIMEOUT;
}
  
EFI_STATUS
EFIAPI
PcatRootBridgeIoPollIo ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  )
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;
  UINT32       Remainder;

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // No matter what, always do a single poll.
  //
  Status = This->Io.Read (This, Width, Address, 1, Result);
  if ( EFI_ERROR(Status) ) {
    return Status;
  }    
  if ( (*Result & Mask) == Value ) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  } else {

    NumberOfTicks = DivU64x32Remainder (Delay, 100, &Remainder);
    if ( Remainder !=0 ) {
      NumberOfTicks += 1;
    }
    NumberOfTicks += 1;
  
    while ( NumberOfTicks ) {

      gBS->Stall(10);
    
      Status = This->Io.Read (This, Width, Address, 1, Result);
      if ( EFI_ERROR(Status) ) {
        return Status;
      }
    
      if ( (*Result & Mask) == Value ) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }
  return EFI_TIMEOUT;
}

BOOLEAN
PcatRootBridgeMemAddressValid (
  IN PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData,
  IN UINT64                         Address
  )
{
  if ((Address >= PrivateData->PciExpressBaseAddress) && (Address < PrivateData->PciExpressBaseAddress + 0x10000000)) {
    return TRUE;
  }
  if ((Address >= PrivateData->MemBase) && (Address < PrivateData->MemLimit)) {
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  UINTN                          AlignMask;
  PTR                            In;
  PTR                            Out;

  if ( Buffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  if (!PcatRootBridgeMemAddressValid (PrivateData, Address)) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  Address += PrivateData->PhysicalMemoryBase;

  In.buf  = Buffer;
  Out.buf = (VOID *)(UINTN) Address;
  if ((UINT32)Width <= EfiPciWidthUint64) {
    return PcatRootBridgeIoMemRW (Width, Count, TRUE, In, TRUE, Out);
  }
  if (Width >= EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    return PcatRootBridgeIoMemRW (Width, Count, TRUE, In, FALSE, Out);
  }
  if (Width >= EfiPciWidthFillUint8 && Width <= EfiPciWidthFillUint64) {
    return PcatRootBridgeIoMemRW (Width, Count, FALSE, In, TRUE, Out);
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE *PrivateData;
  UINTN  AlignMask;
  PTR    In;
  PTR    Out;

  if ( Buffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  if (!PcatRootBridgeMemAddressValid (PrivateData, Address)) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  Address += PrivateData->PhysicalMemoryBase;

  In.buf  = (VOID *)(UINTN) Address;
  Out.buf = Buffer;
  if ((UINT32)Width <= EfiPciWidthUint64) {
    return PcatRootBridgeIoMemRW (Width, Count, TRUE, In, TRUE, Out);
  }
  if (Width >= EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    return PcatRootBridgeIoMemRW (Width, Count, FALSE, In, TRUE, Out);
  }
  if (Width >= EfiPciWidthFillUint8 && Width <= EfiPciWidthFillUint64) {
    return PcatRootBridgeIoMemRW (Width, Count, TRUE, In, FALSE, Out);
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 DestAddress,
  IN UINT64                                 SrcAddress,
  IN UINTN                                  Count
  )

{
  EFI_STATUS  Status;
  BOOLEAN     Direction;
  UINTN       Stride;
  UINTN       Index;
  UINT64      Result;

  if ((UINT32)Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }       

  if (DestAddress == SrcAddress) {
    return EFI_SUCCESS;
  }

  Stride = (UINTN)1 << Width;

  Direction = TRUE;
  if ((DestAddress > SrcAddress) && (DestAddress < (SrcAddress + Count * Stride))) {
    Direction   = FALSE;
    SrcAddress  = SrcAddress  + (Count-1) * Stride;
    DestAddress = DestAddress + (Count-1) * Stride;
  }

  for (Index = 0;Index < Count;Index++) {
    Status = PcatRootBridgeIoMemRead (
               This,
               Width,
               SrcAddress,
               1,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = PcatRootBridgeIoMemWrite (
               This,
               Width,
               DestAddress,
               1,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    if (Direction) {
      SrcAddress  += Stride;
      DestAddress += Stride;
    } else {
      SrcAddress  -= Stride;
      DestAddress -= Stride;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return PcatRootBridgeIoPciRW (This, FALSE, Width, Address, Count, Buffer);
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoPciWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  return PcatRootBridgeIoPciRW (This, TRUE, Width, Address, Count, Buffer);
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoMap (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  )

{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  MAP_INFO              *MapInfo;
  MAP_INFO_INSTANCE    *MapInstance;
  PCAT_PCI_ROOT_BRIDGE_INSTANCE *PrivateData;

  if ( HostAddress == NULL || NumberOfBytes == NULL || 
       DeviceAddress == NULL || Mapping == NULL ) {
    
    return EFI_INVALID_PARAMETER;
  }

  //
  // Perform a fence operation to make sure all memory operations are flushed
  //
  MemoryFence();

  //
  // Initialize the return values to their defaults
  //
  *Mapping = NULL;

  //
  // Make sure that Operation is valid
  //
  if ((UINT32)Operation >= EfiPciOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Most PCAT like chipsets can not handle performing DMA above 4GB.
  // If any part of the DMA transfer being mapped is above 4GB, then
  // map the DMA transfer to a buffer below 4GB.
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;
  if ((PhysicalAddress + *NumberOfBytes) > 0x100000000ULL) {

    //
    // Common Buffer operations can not be remapped.  If the common buffer
    // if above 4GB, then it is not possible to generate a mapping, so return 
    // an error.
    //
    if (Operation == EfiPciOperationBusMasterCommonBuffer || Operation == EfiPciOperationBusMasterCommonBuffer64) {
      return EFI_UNSUPPORTED;
    }

    //
    // Allocate a MAP_INFO structure to remember the mapping when Unmap() is
    // called later.
    //
    Status = gBS->AllocatePool (
                    EfiBootServicesData, 
                    sizeof(MAP_INFO), 
                    (VOID **)&MapInfo
                    );
    if (EFI_ERROR (Status)) {
      *NumberOfBytes = 0;
      return Status;
    }

    //
    // Return a pointer to the MAP_INFO structure in Mapping
    //
    *Mapping = MapInfo;

    //
    // Initialize the MAP_INFO structure
    //
    MapInfo->Operation         = Operation;
    MapInfo->NumberOfBytes     = *NumberOfBytes;
    MapInfo->NumberOfPages     = EFI_SIZE_TO_PAGES(*NumberOfBytes);
    MapInfo->HostAddress       = PhysicalAddress;
    MapInfo->MappedHostAddress = 0x00000000ffffffff;

    //
    // Allocate a buffer below 4GB to map the transfer to.
    //
    Status = gBS->AllocatePages (
                    AllocateMaxAddress, 
                    EfiBootServicesData, 
                    MapInfo->NumberOfPages,
                    &MapInfo->MappedHostAddress
                    );
    if (EFI_ERROR(Status)) {
      gBS->FreePool (MapInfo);
      *NumberOfBytes = 0;
      return Status;
    }

    //
    // If this is a read operation from the Bus Master's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so the Bus Master can read the contents of the real buffer.
    //
    if (Operation == EfiPciOperationBusMasterRead || Operation == EfiPciOperationBusMasterRead64) {
      CopyMem (
        (VOID *)(UINTN)MapInfo->MappedHostAddress, 
        (VOID *)(UINTN)MapInfo->HostAddress,
        MapInfo->NumberOfBytes
        );
    }


  Status =gBS->AllocatePool (
                    EfiBootServicesData, 
                    sizeof(MAP_INFO_INSTANCE), 
                    (VOID **)&MapInstance
                    );                    
    if (EFI_ERROR(Status)) {
      gBS->FreePages (MapInfo->MappedHostAddress,MapInfo->NumberOfPages);
      gBS->FreePool (MapInfo);
      *NumberOfBytes = 0;
      return Status;
    }

    MapInstance->Map=MapInfo;
    PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);
    InsertTailList(&PrivateData->MapInfo,&MapInstance->Link);
    
  //
    // The DeviceAddress is the address of the maped buffer below 4GB
    //
    *DeviceAddress = MapInfo->MappedHostAddress;
  } else {
    //
    // The transfer is below 4GB, so the DeviceAddress is simply the HostAddress
    //
    *DeviceAddress = PhysicalAddress;
  }

  //
  // Perform a fence operation to make sure all memory operations are flushed
  //
  MemoryFence();

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoUnmap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN VOID                             *Mapping
  )

{
  MAP_INFO    *MapInfo;
  PCAT_PCI_ROOT_BRIDGE_INSTANCE *PrivateData;
  LIST_ENTRY *Link;

  //
  // Perform a fence operation to make sure all memory operations are flushed
  //
  MemoryFence();

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);
  //
  // See if the Map() operation associated with this Unmap() required a mapping buffer.
  // If a mapping buffer was not required, then this function simply returns EFI_SUCCESS.
  //
  if (Mapping != NULL) {
    //
    // Get the MAP_INFO structure from Mapping
    //
    MapInfo = (MAP_INFO *)Mapping;

  for (Link = PrivateData->MapInfo.ForwardLink; Link != &PrivateData->MapInfo; Link = Link->ForwardLink) {
      if (((MAP_INFO_INSTANCE*)Link)->Map == MapInfo)
        break;
    }

    if (Link == &PrivateData->MapInfo) {
      return EFI_INVALID_PARAMETER;
  }

    RemoveEntryList(Link);
    ((MAP_INFO_INSTANCE*)Link)->Map = NULL;
    gBS->FreePool((MAP_INFO_INSTANCE*)Link);

    //
    // If this is a write operation from the Bus Master's point of view,
    // then copy the contents of the mapped buffer into the real buffer
    // so the processor can read the contents of the real buffer.
    //
    if (MapInfo->Operation == EfiPciOperationBusMasterWrite || MapInfo->Operation == EfiPciOperationBusMasterWrite64) {
      CopyMem (
        (VOID *)(UINTN)MapInfo->HostAddress, 
        (VOID *)(UINTN)MapInfo->MappedHostAddress,
        MapInfo->NumberOfBytes
        );
    }

    //
    // Free the mapped buffer and the MAP_INFO structure.
    //
    gBS->FreePages (MapInfo->MappedHostAddress, MapInfo->NumberOfPages);
    gBS->FreePool (Mapping);
  }

  //
  // Perform a fence operation to make sure all memory operations are flushed
  //
  MemoryFence();

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoAllocateBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  //
  // Validate Attributes
  //
  if (Attributes & EFI_PCI_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check for invalid inputs
  //
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  if (MemoryType != EfiBootServicesData && MemoryType != EfiRuntimeServicesData) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Limit allocations to memory below 4GB
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS)(0xffffffff);

  Status = gBS->AllocatePages (AllocateMaxAddress, MemoryType, Pages, &PhysicalAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *HostAddress = (VOID *)(UINTN)PhysicalAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  )

{

  if( HostAddress == NULL ){
     return EFI_INVALID_PARAMETER;
  } 
  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress, Pages);
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoFlush (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  )

{
  //
  // Perform a fence operation to make sure all memory operations are flushed
  //
  MemoryFence();

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,  OPTIONAL
  OUT UINT64                           *Attributes
  )

{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE *PrivateData;

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  if (Attributes == NULL && Supported == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Supported is an OPTIONAL parameter.  See if it is NULL
  //
  if (Supported) {
    //
    // This is a generic driver for a PC-AT class system.  It does not have any
    // chipset specific knowlegde, so none of the attributes can be set or 
    // cleared.  Any attempt to set attribute that are already set will succeed, 
    // and any attempt to set an attribute that is not supported will fail.
    //
    *Supported = PrivateData->Attributes;
  }

  //
  // Set Attrbutes to the attributes detected when the PCI Root Bridge was initialized
  //
  
  if (Attributes) {
    *Attributes = PrivateData->Attributes;
  }
  
   
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength 
  )

{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE   *PrivateData;
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  //
  // This is a generic driver for a PC-AT class system.  It does not have any
  // chipset specific knowlegde, so none of the attributes can be set or 
  // cleared.  Any attempt to set attribute that are already set will succeed, 
  // and any attempt to set an attribute that is not supported will fail.
  //
  if (Attributes & (~PrivateData->Attributes)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT VOID                             **Resources
  )

{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE   *PrivateData;
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  *Resources = PrivateData->Configuration;

  return EFI_SUCCESS;
}

//
// Internal function
//

EFI_STATUS
EFIAPI
PcatRootBridgeIoMemRW (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                                  Count,
  IN  BOOLEAN                                InStrideFlag,
  IN  PTR                                    In,
  IN  BOOLEAN                                OutStrideFlag,
  OUT PTR                                    Out
  )
/*++

Routine Description:

  Private service to provide the memory read/write

Arguments:

  Width of the Memory Access
  Count of the number of accesses to perform

Returns:

  Status

  EFI_SUCCESS           - Successful transaction
  EFI_INVALID_PARAMETER - Unsupported width and address combination

--*/
{
  UINTN  Stride;
  UINTN  InStride;
  UINTN  OutStride;


  Width     = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (Width & 0x03);
  Stride    = (UINTN)1 << Width;
  InStride  = InStrideFlag  ? Stride : 0;
  OutStride = OutStrideFlag ? Stride : 0;

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiPciWidthUint8:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      MemoryFence();
      *In.ui8 = *Out.ui8;
      MemoryFence();
    }
    break;
  case EfiPciWidthUint16:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      MemoryFence();
      *In.ui16 = *Out.ui16;
      MemoryFence();
    }
    break;
  case EfiPciWidthUint32:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      MemoryFence();
      *In.ui32 = *Out.ui32;
      MemoryFence();
    }
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


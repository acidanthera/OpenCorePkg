/*++

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    DeviceIo.c
    
Abstract:

    EFI PC-AT PCI Device IO driver

--*/
#include "PcatPciRootBridge.h"
#include "DeviceIo.h"

EFI_STATUS
DeviceIoConstructor (
  IN EFI_HANDLE                      Handle,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo,
  IN EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  IN UINT16                          PrimaryBus,
  IN UINT16                          SubordinateBus
  )
/*++

Routine Description:
  
  Initialize and install a Device IO protocol on a empty device path handle.

Arguments:
  
  Handle               - Handle of PCI RootBridge IO instance
  PciRootBridgeIo      - PCI RootBridge IO instance
  DevicePath           - Device Path of PCI RootBridge IO instance
  PrimaryBus           - Primary Bus
  SubordinateBus       - Subordinate Bus

Returns:
  
  EFI_SUCCESS          -  This driver is added to ControllerHandle.  
  EFI_ALREADY_STARTED  -  This driver is already running on ControllerHandle.   
  Others               -  This driver does not support this device.

--*/
{
  EFI_STATUS                      Status;
  DEVICE_IO_PRIVATE_DATA          *Private;

  //
  // Initialize the Device IO device instance.
  //
  Private = AllocateZeroPool (sizeof (DEVICE_IO_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature                = DEVICE_IO_PRIVATE_DATA_SIGNATURE;
  Private->Handle                   = Handle;
  Private->PciRootBridgeIo          = PciRootBridgeIo;
  Private->DevicePath               = DevicePath;
  Private->PrimaryBus               = PrimaryBus;
  Private->SubordinateBus           = SubordinateBus;

  Private->DeviceIo.Mem.Read        = DeviceIoMemRead;
  Private->DeviceIo.Mem.Write       = DeviceIoMemWrite;
  Private->DeviceIo.Io.Read         = DeviceIoIoRead;
  Private->DeviceIo.Io.Write        = DeviceIoIoWrite;
  Private->DeviceIo.Pci.Read        = DeviceIoPciRead;
  Private->DeviceIo.Pci.Write       = DeviceIoPciWrite;
  Private->DeviceIo.PciDevicePath   = DeviceIoPciDevicePath;
  Private->DeviceIo.Map             = DeviceIoMap;
  Private->DeviceIo.Unmap           = DeviceIoUnmap;
  Private->DeviceIo.AllocateBuffer  = DeviceIoAllocateBuffer;
  Private->DeviceIo.Flush           = DeviceIoFlush;
  Private->DeviceIo.FreeBuffer      = DeviceIoFreeBuffer;

  //
  // Install protocol interfaces for the Device IO device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiDeviceIoProtocolGuid,
                  &Private->DeviceIo,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoMemRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  )
/*++

Routine Description:
  
  Perform reading memory mapped I/O space of device.

Arguments:
  
  This     -  A pointer to EFI_DEVICE_IO protocol instance.  
  Width    -  Width of I/O operations.
  Address  -  The base address of I/O operations.  
  Count    -  The number of I/O operations to perform. 
              Bytes moves is Width size * Count, starting at Address.           
  Buffer   -  The destination buffer to store results.

Returns:
    
  EFI_SUCCESS            -  The data was read from the device.  
  EFI_INVALID_PARAMETER  -  Width is invalid.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to lack of resources.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width > MMIO_COPY_UINT64) {
    return EFI_INVALID_PARAMETER;
  }
  if (Width >= MMIO_COPY_UINT8) {
    Width = (EFI_IO_WIDTH) (Width - MMIO_COPY_UINT8);
    Status = Private->PciRootBridgeIo->CopyMem (
                                         Private->PciRootBridgeIo,
                                         (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                         (UINT64)(UINTN) Buffer,
                                         Address,
                                         Count
                                         );
  } else {
    Status = Private->PciRootBridgeIo->Mem.Read (
                                             Private->PciRootBridgeIo,
                                             (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                             Address,
                                             Count,
                                             Buffer
                                             );
  }

  return Status;
}



EFI_STATUS
EFIAPI
DeviceIoMemWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  )
/*++

Routine Description:
  
  Perform writing memory mapped I/O space of device.

Arguments:
  
  This     -  A pointer to EFI_DEVICE_IO protocol instance.  
  Width    -  Width of I/O operations.  
  Address  -  The base address of I/O operations.   
  Count    -  The number of I/O operations to perform. 
              Bytes moves is Width size * Count, starting at Address.            
  Buffer   -  The source buffer of data to be written.

Returns:
    
  EFI_SUCCESS            -  The data was written to the device.
  EFI_INVALID_PARAMETER  -  Width is invalid.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to lack of resources.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width > MMIO_COPY_UINT64) {
    return EFI_INVALID_PARAMETER;
  }
  if (Width >= MMIO_COPY_UINT8) {
    Width = (EFI_IO_WIDTH) (Width - MMIO_COPY_UINT8);
    Status = Private->PciRootBridgeIo->CopyMem (
                                         Private->PciRootBridgeIo,
                                         (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                         Address,
                                         (UINT64)(UINTN) Buffer,
                                         Count
                                         );
  } else {
    Status = Private->PciRootBridgeIo->Mem.Write (
                                             Private->PciRootBridgeIo,
                                             (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                             Address,
                                             Count,
                                             Buffer
                                             );
  }

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoIoRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  )
/*++

Routine Description:
  
  Perform reading I/O space of device.

Arguments:
  
  This     -  A pointer to EFI_DEVICE_IO protocol instance.  
  Width    -  Width of I/O operations.
  Address  -  The base address of I/O operations. 
  Count    -  The number of I/O operations to perform. 
              Bytes moves is Width size * Count, starting at Address.          
  Buffer   -  The destination buffer to store results.

Returns:
    
  EFI_SUCCESS            -  The data was read from the device.
  EFI_INVALID_PARAMETER  -  Width is invalid.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to lack of resources.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Io.Read (
                                          Private->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Address,
                                          Count,
                                          Buffer
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoIoWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  )
/*++

Routine Description:
  
  Perform writing I/O space of device.

Arguments:
  
  This     -  A pointer to EFI_DEVICE_IO protocol instance.  
  Width    -  Width of I/O operations.
  Address  -  The base address of I/O operations.
  Count    -  The number of I/O operations to perform. 
              Bytes moves is Width size * Count, starting at Address.        
  Buffer   -  The source buffer of data to be written.

Returns:
    
  EFI_SUCCESS            -  The data was written to the device.
  EFI_INVALID_PARAMETER  -  Width is invalid.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to lack of resources.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Io.Write (
                                          Private->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Address,
                                          Count,
                                          Buffer
                                          );

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoPciRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  )
/*++

Routine Description:
  
  Perform reading PCI configuration space of device

Arguments:
  
  This     -  A pointer to EFI_DEVICE_IO protocol instance.  
  Width    -  Width of I/O operations. 
  Address  -  The base address of I/O operations. 
  Count    -  The number of I/O operations to perform. 
              Bytes moves is Width size * Count, starting at Address.           
  Buffer   -  The destination buffer to store results.

Returns:
    
  EFI_SUCCESS            -  The data was read from the device.
  EFI_INVALID_PARAMETER  -  Width is invalid.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to lack of resources.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if ((UINT32)Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Pci.Read (
                                           Private->PciRootBridgeIo,
                                           (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                           Address,
                                           Count,
                                           Buffer
                                           );

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoPciWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  )
/*++

Routine Description:
  
  Perform writing PCI configuration space of device.

Arguments:
  
  This     -  A pointer to EFI_DEVICE_IO protocol instance.   
  Width    -  Width of I/O operations. 
  Address  -  The base address of I/O operations. 
  Count    -  The number of I/O operations to perform. 
              Bytes moves is Width size * Count, starting at Address.         
  Buffer   -  The source buffer of data to be written.

Returns:
    
  EFI_SUCCESS            -  The data was written to the device.
  EFI_INVALID_PARAMETER  -  Width is invalid.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to lack of resources.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if ((UINT32)Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Pci.Write (
                                           Private->PciRootBridgeIo,
                                           (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                           Address,
                                           Count,
                                           Buffer
                                           );

  return Status;
}

EFI_DEVICE_PATH_PROTOCOL *
AppendPciDevicePath (
  IN     DEVICE_IO_PRIVATE_DATA    *Private,
  IN     UINT8                     Bus,
  IN     UINT8                     Device,
  IN     UINT8                     Function,
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN OUT UINT16                    *BridgePrimaryBus,
  IN OUT UINT16                    *BridgeSubordinateBus
  )
/*++

Routine Description:
  
  Append a PCI device path node to another device path.

Arguments:
  
  Private               -  A pointer to DEVICE_IO_PRIVATE_DATA instance.  
  Bus                   -  PCI bus number of the device.
  Device                -  PCI device number of the device.
  Function              -  PCI function number of the device.
  DevicePath            -  Original device path which will be appended a PCI device path node.
  BridgePrimaryBus      -  Primary bus number of the bridge.
  BridgeSubordinateBus  -  Subordinate bus number of the bridge.

Returns:
  
  Pointer to the appended PCI device path.

--*/
{
  UINT16                    ThisBus;
  UINT8                     ThisDevice;
  UINT8                     ThisFunc;
  UINT64                    Address;
  PCI_TYPE01                PciBridge;
  PCI_TYPE01                *PciPtr;
  EFI_DEVICE_PATH_PROTOCOL  *ReturnDevicePath;
  PCI_DEVICE_PATH           PciNode;

  PciPtr = &PciBridge;
  for (ThisBus = *BridgePrimaryBus; ThisBus <= *BridgeSubordinateBus; ThisBus++) {
    for (ThisDevice = 0; ThisDevice <= PCI_MAX_DEVICE; ThisDevice++) {
      for (ThisFunc = 0; ThisFunc <= PCI_MAX_FUNC; ThisFunc++) {
        Address = EFI_PCI_ADDRESS (ThisBus, ThisDevice, ThisFunc, 0);
        ZeroMem (PciPtr, sizeof (PCI_TYPE01));
        Private->DeviceIo.Pci.Read (
                                &Private->DeviceIo,
                                IO_UINT32,
                                Address,
                                1,
                                &(PciPtr->Hdr.VendorId)
                                );
        if ((PciPtr->Hdr.VendorId == 0xffff) && (ThisFunc == 0)) {
          break;
        }
        if (PciPtr->Hdr.VendorId == 0xffff) {
          continue;
        }

        Private->DeviceIo.Pci.Read (
                                &Private->DeviceIo,
                                IO_UINT32,
                                Address,
                                sizeof (PCI_TYPE01) / sizeof (UINT32),
                                PciPtr
                                );
        if (IS_PCI_BRIDGE (PciPtr)) {
          if (Bus >= PciPtr->Bridge.SecondaryBus && Bus <= PciPtr->Bridge.SubordinateBus) {

            PciNode.Header.Type     = HARDWARE_DEVICE_PATH;
            PciNode.Header.SubType  = HW_PCI_DP;
            SetDevicePathNodeLength (&PciNode.Header, sizeof (PciNode));

            PciNode.Device        = ThisDevice;
            PciNode.Function      = ThisFunc;
            ReturnDevicePath      = AppendDevicePathNode (DevicePath, &PciNode.Header);

            *BridgePrimaryBus     = PciPtr->Bridge.SecondaryBus;
            *BridgeSubordinateBus = PciPtr->Bridge.SubordinateBus;
            return ReturnDevicePath;
          }
        }

        if ((ThisFunc == 0) && ((PciPtr->Hdr.HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x0)) {
          //
          // Skip sub functions, this is not a multi function device
          //
          break;
        }
      }
    }
  }

  ZeroMem (&PciNode, sizeof (PciNode));
  PciNode.Header.Type     = HARDWARE_DEVICE_PATH;
  PciNode.Header.SubType  = HW_PCI_DP;
  SetDevicePathNodeLength (&PciNode.Header, sizeof (PciNode));
  PciNode.Device        = Device;
  PciNode.Function      = Function;

  ReturnDevicePath      = AppendDevicePathNode (DevicePath, &PciNode.Header);

  *BridgePrimaryBus     = 0xffff;
  *BridgeSubordinateBus = 0xffff;
  return ReturnDevicePath;
}

EFI_STATUS
EFIAPI
DeviceIoPciDevicePath (
  IN     EFI_DEVICE_IO_PROTOCOL        *This,
  IN     UINT64                        Address,
  IN OUT EFI_DEVICE_PATH_PROTOCOL      **PciDevicePath
  )
/*++

Routine Description:
  
  Provides an EFI Device Path for a PCI device with the given PCI configuration space address.

Arguments:
  
  This           -  A pointer to the EFI_DEVICE_IO_INTERFACE instance.  
  Address        -  The PCI configuration space address of the device whose Device Path
                    is going to be returned.           
  PciDevicePath  -  A pointer to the pointer for the EFI Device Path for PciAddress.
                    Memory for the Device Path is allocated from the pool.

Returns:
  
  EFI_SUCCESS           -  The PciDevicePath returns a pointer to a valid EFI Device Path.
  EFI_UNSUPPORTED       -  The PciAddress does not map to a valid EFI Device Path. 
  EFI_OUT_OF_RESOURCES  -  The request could not be completed due to a lack of resources.

--*/
{
  DEVICE_IO_PRIVATE_DATA  *Private;
  UINT16                  PrimaryBus;
  UINT16                  SubordinateBus;
  UINT8                   Bus;
  UINT8                   Device;
  UINT8                   Func;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  Bus     = (UINT8) (((UINT32) Address >> 24) & 0xff);
  Device  = (UINT8) (((UINT32) Address >> 16) & 0xff);
  Func    = (UINT8) (((UINT32) Address >> 8) & 0xff);

  if (Bus < Private->PrimaryBus || Bus > Private->SubordinateBus) {
    return EFI_UNSUPPORTED;
  }

  *PciDevicePath  = Private->DevicePath;
  PrimaryBus      = Private->PrimaryBus;
  SubordinateBus  = Private->SubordinateBus;
  do {
    *PciDevicePath = AppendPciDevicePath (
                       Private,
                       Bus,
                       Device,
                       Func,
                       *PciDevicePath,
                       &PrimaryBus,
                       &SubordinateBus
                       );
    if (*PciDevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } while (PrimaryBus != 0xffff);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DeviceIoMap (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_OPERATION_TYPE    Operation,
  IN     EFI_PHYSICAL_ADDRESS     *HostAddress,
  IN OUT UINTN                    *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS     *DeviceAddress,
  OUT    VOID                     **Mapping
  )
/*++

Routine Description:
  
  Provides the device-specific addresses needed to access system memory.

Arguments:
  
  This           -  A pointer to the EFI_DEVICE_IO_INTERFACE instance. 
  Operation      -  Indicates if the bus master is going to read or write to system memory.  
  HostAddress    -  The system memory address to map to the device.
  NumberOfBytes  -  On input the number of bytes to map. On output the number of bytes
                    that were mapped.
  DeviceAddress  -  The resulting map address for the bus master device to use to access the
                    hosts HostAddress.
  Mapping        -  A resulting value to pass to Unmap().

Returns:
  
  EFI_SUCCESS            -  The range was mapped for the returned NumberOfBytes. 
  EFI_INVALID_PARAMETER  -  The Operation or HostAddress is undefined. 
  EFI_UNSUPPORTED        -  The HostAddress cannot be mapped as a common buffer.
  EFI_DEVICE_ERROR       -  The system hardware could not map the requested address. 
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of resources.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if ((UINT32)Operation > EfiBusMasterCommonBuffer) {
    return EFI_INVALID_PARAMETER;
  }

  if (((UINTN) (*HostAddress) != (*HostAddress)) && Operation == EfiBusMasterCommonBuffer) {
    return EFI_UNSUPPORTED;
  }

  Status = Private->PciRootBridgeIo->Map (
                                       Private->PciRootBridgeIo,
                                       (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION) Operation,
                                       (VOID *) (UINTN) (*HostAddress),
                                       NumberOfBytes,
                                       DeviceAddress,
                                       Mapping
                                       );

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoUnmap (
  IN EFI_DEVICE_IO_PROTOCOL   *This,
  IN VOID                     *Mapping
  )
/*++

Routine Description:
  
  Completes the Map() operation and releases any corresponding resources.

Arguments:
  
  This     -  A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  Mapping  -  The mapping value returned from Map().

Returns:
  
  EFI_SUCCESS       -  The range was unmapped.
  EFI_DEVICE_ERROR  -  The data was not committed to the target system memory.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  Status = Private->PciRootBridgeIo->Unmap (
                                       Private->PciRootBridgeIo,
                                       Mapping
                                       );

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoAllocateBuffer (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_ALLOCATE_TYPE         Type,
  IN     EFI_MEMORY_TYPE           MemoryType,
  IN     UINTN                     Pages,
  IN OUT EFI_PHYSICAL_ADDRESS      *PhysicalAddress
  )
/*++

Routine Description:
  
  Allocates pages that are suitable for an EFIBusMasterCommonBuffer mapping.

Arguments:
  
  This             -  A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  Type             -  The type allocation to perform.
  MemoryType       -  The type of memory to allocate, EfiBootServicesData or
                      EfiRuntimeServicesData.
  Pages            -  The number of pages to allocate.
  PhysicalAddress  -  A pointer to store the base address of the allocated range.

Returns:
  
  EFI_SUCCESS            -  The requested memory pages were allocated.
  EFI_OUT_OF_RESOURCES   -  The memory pages could not be allocated.
  EFI_INVALID_PARAMETER  -  The requested memory type is invalid.
  EFI_UNSUPPORTED        -  The requested PhysicalAddress is not supported on
                            this platform.

--*/
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  HostAddress;

  HostAddress = *PhysicalAddress;

  if ((MemoryType != EfiBootServicesData) && (MemoryType != EfiRuntimeServicesData)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Type >= MaxAllocateType) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Type == AllocateAddress) && (HostAddress + EFI_PAGES_TO_SIZE (Pages) - 1 > MAX_COMMON_BUFFER)) {
    return EFI_UNSUPPORTED;
  }

  if ((AllocateAnyPages == Type) || (AllocateMaxAddress == Type && HostAddress > MAX_COMMON_BUFFER)) {
    Type        = AllocateMaxAddress;
    HostAddress = MAX_COMMON_BUFFER;
  }

  Status = gBS->AllocatePages (
                  Type,
                  MemoryType,
                  Pages,
                  &HostAddress
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }


  *PhysicalAddress = HostAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DeviceIoFlush (
  IN EFI_DEVICE_IO_PROTOCOL  *This
  )
/*++

Routine Description:
  
  Flushes any posted write data to the device.

Arguments:
  
  This  -  A pointer to the EFI_DEVICE_IO_INTERFACE instance.

Returns:
  
  EFI_SUCCESS       -  The buffers were flushed.
  EFI_DEVICE_ERROR  -  The buffers were not flushed due to a hardware error.

--*/
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = Private->PciRootBridgeIo->Flush (Private->PciRootBridgeIo);

  return Status;
}

EFI_STATUS
EFIAPI
DeviceIoFreeBuffer (
  IN EFI_DEVICE_IO_PROTOCOL   *This,
  IN UINTN                    Pages,
  IN EFI_PHYSICAL_ADDRESS     HostAddress
  )
/*++

Routine Description:
  
  Frees pages that were allocated with AllocateBuffer().

Arguments:
  
  This         -  A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  Pages        -  The number of pages to free.
  HostAddress  -  The base address of the range to free.

Returns:
  
  EFI_SUCCESS            -  The requested memory pages were freed.
  EFI_NOT_FOUND          -  The requested memory pages were not allocated with
                            AllocateBuffer().               
  EFI_INVALID_PARAMETER  -  HostAddress is not page aligned or Pages is invalid.

--*/
{
  if (((HostAddress & EFI_PAGE_MASK) != 0) || (Pages <= 0)) {
    return EFI_INVALID_PARAMETER;
  }

  return gBS->FreePages (HostAddress, Pages);
}

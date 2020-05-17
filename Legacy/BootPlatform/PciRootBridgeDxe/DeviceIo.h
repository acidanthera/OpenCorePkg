/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    DeviceIo.h
    
Abstract:

    Private Data definition for Device IO driver

--*/

#ifndef _DEVICE_IO_H
#define _DEVICE_IO_H



#define DEVICE_IO_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('d', 'e', 'v', 'I')

#define MAX_COMMON_BUFFER                 0x00000000FFFFFFFF

typedef struct {
  UINTN                           Signature;
  EFI_HANDLE                      Handle;
  EFI_DEVICE_IO_PROTOCOL          DeviceIo;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  UINT16                          PrimaryBus;
  UINT16                          SubordinateBus;
} DEVICE_IO_PRIVATE_DATA;

#define DEVICE_IO_PRIVATE_DATA_FROM_THIS(a) CR (a, DEVICE_IO_PRIVATE_DATA, DeviceIo, DEVICE_IO_PRIVATE_DATA_SIGNATURE)

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
;

EFI_STATUS
EFIAPI
DeviceIoMemRead (
  IN     EFI_DEVICE_IO_PROTOCOL *This,
  IN     EFI_IO_WIDTH           Width,
  IN     UINT64                 Address,
  IN     UINTN                  Count,
  IN OUT VOID                   *Buffer
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
;

EFI_STATUS
EFIAPI
DeviceIoMemWrite (
  IN     EFI_DEVICE_IO_PROTOCOL *This,
  IN     EFI_IO_WIDTH           Width,
  IN     UINT64                 Address,
  IN     UINTN                  Count,
  IN OUT VOID                   *Buffer
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
;

EFI_STATUS
EFIAPI
DeviceIoIoRead (
  IN     EFI_DEVICE_IO_PROTOCOL *This,
  IN     EFI_IO_WIDTH           Width,
  IN     UINT64                 Address,
  IN     UINTN                  Count,
  IN OUT VOID                   *Buffer
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
;

EFI_STATUS
EFIAPI
DeviceIoIoWrite (
  IN     EFI_DEVICE_IO_PROTOCOL *This,
  IN     EFI_IO_WIDTH           Width,
  IN     UINT64                 Address,
  IN     UINTN                  Count,
  IN OUT VOID                   *Buffer
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
;

EFI_STATUS
EFIAPI
DeviceIoPciRead (
  IN     EFI_DEVICE_IO_PROTOCOL *This,
  IN     EFI_IO_WIDTH           Width,
  IN     UINT64                 Address,
  IN     UINTN                  Count,
  IN OUT VOID                   *Buffer
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
;

EFI_STATUS
EFIAPI
DeviceIoPciWrite (
  IN     EFI_DEVICE_IO_PROTOCOL *This,
  IN     EFI_IO_WIDTH           Width,
  IN     UINT64                 Address,
  IN     UINTN                  Count,
  IN OUT VOID                   *Buffer
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
;

EFI_STATUS
EFIAPI
DeviceIoPciDevicePath (
  IN     EFI_DEVICE_IO_PROTOCOL        *This,
  IN     UINT64                        Address,
  IN OUT EFI_DEVICE_PATH_PROTOCOL      **PciDevicePath
  )
/*++

Routine Description:
  
  Append a PCI device path node to another device path.

Arguments:
  
  This                  -  A pointer to EFI_DEVICE_IO_PROTOCOL.  
  Address               -  PCI bus,device, function.
  PciDevicePath         -  PCI device path.

Returns:
  
  Pointer to the appended PCI device path.

--*/
;

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
;

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
;

EFI_STATUS
EFIAPI
DeviceIoAllocateBuffer (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_ALLOCATE_TYPE         Type,
  IN     EFI_MEMORY_TYPE           MemoryType,
  IN     UINTN                     Pages,
  IN OUT EFI_PHYSICAL_ADDRESS      *HostAddress
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
  HostAddress      -  A pointer to store the base address of the allocated range.

Returns:
  
  EFI_SUCCESS            -  The requested memory pages were allocated.
  EFI_OUT_OF_RESOURCES   -  The memory pages could not be allocated.
  EFI_INVALID_PARAMETER  -  The requested memory type is invalid.
  EFI_UNSUPPORTED        -  The requested PhysicalAddress is not supported on
                            this platform.

--*/
;

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
;

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
;

#endif


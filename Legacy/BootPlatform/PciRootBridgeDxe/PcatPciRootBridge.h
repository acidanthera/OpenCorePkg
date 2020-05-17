/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PcatPciRootBridge.h

Abstract:

  The driver for the host to pci bridge (root bridge).

--*/

#ifndef _PCAT_PCI_ROOT_BRIDGE_H_
#define _PCAT_PCI_ROOT_BRIDGE_H_

#include <PiDxe.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/DeviceIo.h>
#include <Protocol/CpuIo2.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>

#include <Guid/PciOptionRomTable.h>
#include <Guid/HobList.h>
#include <Guid/PciExpressBaseAddress.h>

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Pci.h>

#define PCI_MAX_SEGMENT   0
//
// Driver Instance Data Prototypes
//
#define PCAT_PCI_ROOT_BRIDGE_SIGNATURE  SIGNATURE_32('p', 'c', 'r', 'b')

typedef struct {
  UINT32                            Signature;
  EFI_HANDLE                        Handle;
                                    
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   Io;
  EFI_CPU_IO2_PROTOCOL              *CpuIo;

  UINT32                            RootBridgeNumber;
  UINT32                            PrimaryBus;
  UINT32                            SubordinateBus;
                                     
  UINT64                            MemBase;     // Offsets host to bus memory addr.
  UINT64                            MemLimit;    // Max allowable memory access
                                    
  UINT64                            IoBase;      // Offsets host to bus io addr.
  UINT64                            IoLimit;     // Max allowable io access
                                    
  UINT64                            PciAddress;
  UINT64                            PciData;
                                    
  UINT64                            PhysicalMemoryBase;
  UINT64                            PhysicalIoBase;
                                     
  EFI_LOCK                          PciLock;
                                    
  UINT64                            Attributes;
                                    
  UINT64                            Mem32Base;
  UINT64                            Mem32Limit;
  UINT64                            Pmem32Base;
  UINT64                            Pmem32Limit;
  UINT64                            Mem64Base;
  UINT64                            Mem64Limit;
  UINT64                            Pmem64Base;
  UINT64                            Pmem64Limit;

  UINT64                            PciExpressBaseAddress;

  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration;

  LIST_ENTRY                    MapInfo;
} PCAT_PCI_ROOT_BRIDGE_INSTANCE;

//
// Driver Instance Data Macros
//
#define DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(a) \
  CR(a, PCAT_PCI_ROOT_BRIDGE_INSTANCE, Io, PCAT_PCI_ROOT_BRIDGE_SIGNATURE)

//
// Private data types
//
typedef union {
  UINT8   volatile  *buf;
  UINT8   volatile  *ui8;
  UINT16  volatile  *ui16;
  UINT32  volatile  *ui32;
  UINT64  volatile  *ui64;
  UINTN   volatile  ui;
} PTR;

typedef struct {
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation;
  UINTN                                      NumberOfBytes;
  UINTN                                      NumberOfPages;
  EFI_PHYSICAL_ADDRESS                       HostAddress;
  EFI_PHYSICAL_ADDRESS                       MappedHostAddress;
} MAP_INFO;

typedef struct {
  LIST_ENTRY Link;
  MAP_INFO * Map;  
} MAP_INFO_INSTANCE;

typedef
VOID
(*EFI_PCI_BUS_SCAN_CALLBACK) (
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev,
  UINT16                           MinBus,
  UINT16                           MaxBus,
  UINT16                           MinDevice,
  UINT16                           MaxDevice,
  UINT16                           MinFunc,
  UINT16                           MaxFunc,
  UINT16                           Bus,
  UINT16                           Device,
  UINT16                           Func,
  IN VOID                          *Context
  );

typedef struct {
  UINT16                    *CommandRegisterBuffer;
  UINT32                    PpbMemoryWindow;     
} PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT;

typedef struct {
  UINT8 Register;
  UINT8 Function;
  UINT8 Device;
  UINT8 Bus;
  UINT8 Reserved[4];
} DEFIO_PCI_ADDR;

//
// Driver Protocol Constructor Prototypes
//
EFI_STATUS 
ConstructConfiguration(
  IN OUT PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData
  );

EFI_STATUS
PcatPciRootBridgeParseBars (
  IN PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData,
  IN UINT16                         Command,
  IN UINTN                          Bus,
  IN UINTN                          Device,
  IN UINTN                          Function
  );

EFI_STATUS
PcatRootBridgeDevicePathConstructor (
  IN EFI_DEVICE_PATH_PROTOCOL  **Protocol,
  IN UINTN                     RootBridgeNumber,
  IN BOOLEAN                   IsPciExpress
  );

EFI_STATUS
PcatRootBridgeIoConstructor (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *Protocol,
  IN UINTN                            SegmentNumber
  );

EFI_STATUS
PcatRootBridgeIoGetIoPortMapping (
  OUT EFI_PHYSICAL_ADDRESS  *IoPortMapping,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryPortMapping
  );

EFI_STATUS
PcatRootBridgeIoPciRW (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN BOOLEAN                                Write,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                               *UserBuffer
  );

UINT64
GetPciExpressBaseAddressForRootBridge (
  IN UINTN    HostBridgeNumber,
  IN UINTN    RootBridgeNumber
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  );

EFI_STATUS
EFIAPI
PcatRootBridgeIoIoWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  );

//
// Driver entry point prototype
//
EFI_STATUS
EFIAPI
InitializePcatPciRootBridge (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

extern EFI_CPU_IO2_PROTOCOL  *gCpuIo;

#endif

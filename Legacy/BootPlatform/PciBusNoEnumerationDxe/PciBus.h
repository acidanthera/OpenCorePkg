/*++

Copyright (c) 2005 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciBus.h
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#ifndef _EFI_PCI_BUS_H
#define _EFI_PCI_BUS_H

#include <PiDxe.h>

#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Decompress.h>
#include <Protocol/UgaIo.h>
#include <Protocol/LoadedImage.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffLib.h>

//
// Driver Produced Protocol Prototypes
//

#define VGABASE1  0x3B0
#define VGALIMIT1 0x3BB

#define VGABASE2  0x3C0
#define VGALIMIT2 0x3DF

#define ISABASE   0x100
#define ISALIMIT  0x3FF

typedef enum {
  PciBarTypeUnknown = 0,
  PciBarTypeIo16,
  PciBarTypeIo32,
  PciBarTypeMem32,
  PciBarTypePMem32,
  PciBarTypeMem64,
  PciBarTypePMem64,
  PciBarTypeIo,
  PciBarTypeMem,
  PciBarTypeMaxType
} PCI_BAR_TYPE;

typedef struct {
  UINT64        BaseAddress;
  UINT64        Length;
  UINT64        Alignment;
  PCI_BAR_TYPE  BarType;
  BOOLEAN       Prefetchable;
  UINT8         MemType;
  UINT8         Offset;
} PCI_BAR;

#define PCI_IO_DEVICE_SIGNATURE   SIGNATURE_32 ('p','c','i','o')

#define EFI_BRIDGE_IO32_DECODE_SUPPORTED        0x0001 
#define EFI_BRIDGE_PMEM32_DECODE_SUPPORTED      0x0002 
#define EFI_BRIDGE_PMEM64_DECODE_SUPPORTED      0x0004 
#define EFI_BRIDGE_IO16_DECODE_SUPPORTED        0x0008  
#define EFI_BRIDGE_PMEM_MEM_COMBINE_SUPPORTED   0x0010  
#define EFI_BRIDGE_MEM64_DECODE_SUPPORTED       0x0020
#define EFI_BRIDGE_MEM32_DECODE_SUPPORTED       0x0040


typedef struct _PCI_IO_DEVICE {
  UINT32                                    Signature;
  EFI_HANDLE                                Handle;
  EFI_PCI_IO_PROTOCOL                       PciIo;
  LIST_ENTRY                            Link;

  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *PciRootBridgeIo;

  //
  // PCI configuration space header type
  //
  PCI_TYPE00                                Pci;

  //
  // Bus number, Device number, Function number
  //
  UINT8                                     BusNumber;
  UINT8                                     DeviceNumber;
  UINT8                                     FunctionNumber;

  //
  // BAR for this PCI Device
  //
  PCI_BAR                                   PciBar[PCI_MAX_BAR];

  //
  // The bridge device this pci device is subject to
  //
  struct _PCI_IO_DEVICE                     *Parent;

  //
  // A linked list for children Pci Device if it is bridge device
  //
  LIST_ENTRY                            ChildList;

  //
  // TRUE if the PCI bus driver creates the handle for this PCI device
  //
  BOOLEAN                                   Registered;

  //
  // TRUE if the PCI bus driver successfully allocates the resource required by
  // this PCI device
  //
  BOOLEAN                                   Allocated;

  //
  // The attribute this PCI device currently set
  //
  UINT64                                    Attributes;

  //
  // The attributes this PCI device actually supports
  //
  UINT64                                    Supports;

  //
  // The resource decode the bridge supports
  //
  UINT32                                    Decodes;

  //
  //  A list tracking reserved resource on a bridge device
  //
  LIST_ENTRY                            ReservedResourceList;

  BOOLEAN                                   IsPciExp;

} PCI_IO_DEVICE;


#define PCI_IO_DEVICE_FROM_PCI_IO_THIS(a) \
  CR (a, PCI_IO_DEVICE, PciIo, PCI_IO_DEVICE_SIGNATURE)

#define PCI_IO_DEVICE_FROM_LINK(a) \
  CR (a, PCI_IO_DEVICE, Link, PCI_IO_DEVICE_SIGNATURE)

//
// Global Variables
//
extern EFI_COMPONENT_NAME_PROTOCOL gPciBusComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPciBusComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL  gPciBusDriverBinding;

extern BOOLEAN                     gFullEnumeration;
extern UINT64                      gAllOne;
extern UINT64                      gAllZero;

#include "PciIo.h"
#include "PciCommand.h"
#include "PciDeviceSupport.h"
#include "PciEnumerator.h"
#include "PciEnumeratorSupport.h"
#include "PciPowerManagement.h"


#define IS_ISA_BRIDGE(_p)       IS_CLASS2 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA)  
#define IS_INTEL_ISA_BRIDGE(_p) (IS_CLASS2 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA_PDECODE) && ((_p)->Hdr.VendorId == 0x8086) && ((_p)->Hdr.DeviceId == 0x7110))
#define IS_PCI_GFX(_p)     IS_CLASS2 (_p, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_OTHER)

#endif

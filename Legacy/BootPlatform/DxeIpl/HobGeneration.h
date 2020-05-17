/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  HobGeneration.h

Abstract:

Revision History:

**/

#ifndef _DXELDR_HOB_GENERATION_H_
#define _DXELDR_HOB_GENERATION_H_

#include "DxeIpl.h"

//
// ** CHANGE **
//
#define EFI_MEMORY_BELOW_1MB_START     0x18000
#define EFI_MEMORY_BELOW_1MB_END       0x9F800
#define EFI_MEMORY_STACK_PAGE_NUM      0x20
#define CONSUMED_MEMORY                0x100000 * 80

#define NV_STORAGE_START               0x15000
#define NV_STORAGE_STATE               0x19000

#pragma pack(1)

typedef struct {
  EFI_HOB_GUID_TYPE             Hob;
  EFI_MEMORY_TYPE_INFORMATION   Info[10];
} MEMORY_TYPE_INFORMATION_HOB;

typedef struct {
  EFI_HOB_GUID_TYPE             Hob;
  EFI_PHYSICAL_ADDRESS          Table;
} TABLE_HOB;

typedef struct {
  EFI_HOB_GUID_TYPE             Hob;
  EFI_PHYSICAL_ADDRESS          Interface;
} PROTOCOL_HOB;

typedef struct {
  EFI_HOB_GUID_TYPE                         Hob;
  // Note: we get only one PCI Segment now.
  EFI_PCI_EXPRESS_BASE_ADDRESS_INFORMATION  PciExpressBaseAddressInfo;
} PCI_EXPRESS_BASE_HOB;

typedef struct {
  EFI_HOB_GUID_TYPE             Hob;
  EFI_ACPI_DESCRIPTION          AcpiDescription;
} ACPI_DESCRIPTION_HOB;

typedef struct {
  EFI_HOB_GUID_TYPE             Hob;
  EFI_FLASH_MAP_FS_ENTRY_DATA   FvbInfo;
} FVB_HOB;

typedef struct {
  EFI_HOB_HANDOFF_INFO_TABLE        Phit;
  EFI_HOB_FIRMWARE_VOLUME           Bfv;
  EFI_HOB_RESOURCE_DESCRIPTOR       BfvResource;
  EFI_HOB_CPU                       Cpu;
  EFI_HOB_MEMORY_ALLOCATION_STACK   Stack;
  EFI_HOB_MEMORY_ALLOCATION         MemoryAllocation;
  EFI_HOB_RESOURCE_DESCRIPTOR       MemoryFreeUnder1MB;
  EFI_HOB_RESOURCE_DESCRIPTOR       MemoryAbove1MB;
  EFI_HOB_RESOURCE_DESCRIPTOR       MemoryAbove4GB;
  EFI_HOB_MEMORY_ALLOCATION_MODULE  DxeCore;
  EFI_HOB_RESOURCE_DESCRIPTOR       MemoryDxeCore;
  MEMORY_TYPE_INFORMATION_HOB       MemoryTypeInfo;
  TABLE_HOB                         Acpi;
  TABLE_HOB                         Acpi20;
  TABLE_HOB                         Smbios;
  TABLE_HOB                         Mps;
  /**
  PROTOCOL_HOB                      FlushInstructionCache;
  PROTOCOL_HOB                      TransferControl;
  PROTOCOL_HOB                      PeCoffLoader;
  PROTOCOL_HOB                      EfiDecompress;
  PROTOCOL_HOB                      TianoDecompress;
  **/
  PROTOCOL_HOB                      SerialStatusCode;
  MEMORY_DESC_HOB                   MemoryDescriptor;
  PCI_EXPRESS_BASE_HOB              PciExpress;
  ACPI_DESCRIPTION_HOB              AcpiInfo;
  
  EFI_HOB_RESOURCE_DESCRIPTOR       NvStorageFvResource;

  FVB_HOB                           NvStorageFvb;
  FVB_HOB                           NvStorage;
  
  EFI_HOB_RESOURCE_DESCRIPTOR       NvFtwFvResource;
  FVB_HOB                           NvFtwFvb;
  FVB_HOB                           NvFtwWorking;
  FVB_HOB                           NvFtwSpare;
  
  EFI_HOB_GENERIC_HEADER            EndOfHobList;
} HOB_TEMPLATE;

#pragma pack()

extern HOB_TEMPLATE  *gHob;

VOID *
PrepareHobStack (
  IN VOID *StackTop
  );

VOID
PrepareHobBfv (
  VOID  *Bfv,
  UINTN BfvLength
  );

VOID *
PrepareHobMemory (
  IN UINTN                    NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR    *EfiMemoryDescriptor
  );

VOID
PrepareHobDxeCore (
  VOID                  *DxeCoreEntryPoint,
  EFI_PHYSICAL_ADDRESS  DxeCoreImageBase,
  UINT64                DxeCoreLength
  );

VOID *
PreparePageTable (
  VOID  *PageNumberTop,
  UINT8 SizeOfMemorySpace
  );

VOID *
PrepareHobMemoryDescriptor (
  VOID                          *MemoryDescriptorTop,
  UINTN                         MemDescCount,
  EFI_MEMORY_DESCRIPTOR         *MemDesc
  );

VOID
PrepareHobPhit (
  VOID *MemoryTop,
  VOID *FreeMemoryTop
  );

VOID *
PrepareHobNvStorage (
  VOID *NvStorageTop
  );

VOID
PrepareHobCpu (
  VOID
  );

VOID
CompleteHobGeneration (
  VOID
  );

#endif

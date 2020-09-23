/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_MEMORY_LIB_H
#define OC_MEMORY_LIB_H

#include <Uefi.h>
#include <Guid/MemoryAttributesTable.h>
#include <IndustryStandard/VirtualMemory.h>

/**
  Reverse equivalent of NEXT_MEMORY_DESCRIPTOR.
**/
#define PREV_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

/**
  Get last descriptor address.
  It is assumed that the descriptor contains pages.
**/
#define LAST_DESCRIPTOR_ADDR(Desc) \
  ((Desc)->PhysicalStart + (EFI_PAGES_TO_SIZE ((UINTN) (Desc)->NumberOfPages) - 1))

/**
  Check if area is within the specified descriptor.
  It is assumed that the descriptor contains pages and AreaSize is not 0.
**/
#define AREA_WITHIN_DESCRIPTOR(Desc, Area, AreaSize) \
  ((Area) >= (Desc)->PhysicalStart && ((Area) + ((AreaSize) - 1)) <= LAST_DESCRIPTOR_ADDR (Desc))

/**
  Reasonable default virtual memory page pool size (2 MB).
**/
#define OC_DEFAULT_VMEM_PAGE_COUNT 0x200

/**
  Reasonable default memory map size used when allocations are problematic.
  Note, that MacPro5,1 is known to have 8880 memory map.
**/
#define OC_DEFAULT_MEMORY_MAP_SIZE  (EFI_PAGE_SIZE*3)

#define OC_MEMORY_TYPE_DESC_COUNT 16

typedef struct {
  CHAR8            *Name;
  EFI_MEMORY_TYPE  Type;
} OC_MEMORY_TYPE_DESC;

/**
  Lock the legacy region specified to enable modification.

  @param[in] LegacyAddress  The address of the region to lock.
  @param[in] LegacyLength   The size of the region to lock.

  @retval EFI_SUCCESS  The region was locked successfully.
**/
EFI_STATUS
LegacyRegionLock (
  IN UINT32  LegacyAddress,
  IN UINT32  LegacyLength
  );

/**
  Unlock the legacy region specified to enable modification.

  @param[in] LegacyAddress  The address of the region to unlock.
  @param[in] LegacyLength   The size of the region to unlock.

  @retval EFI_SUCCESS  The region was unlocked successfully.
**/
EFI_STATUS
LegacyRegionUnlock (
  IN UINT32  LegacyAddress,
  IN UINT32  LegacyLength
  );

/**
  Get current memory map allocated on pool with reserved entries.

  @param[out]  MemoryMapSize          Resulting memory map size in bytes.
  @param[out]  DescriptorSize         Resulting memory map descriptor size in bytes.
  @param[out]  MapKey                 Memory map key, optional.
  @param[out]  DescriptorVersion      Memory map descriptor version, optional.
  @param[out]  OriginalMemoryMapSize  Actual pool allocation memory, optional.
  @param[out]  IncludeSplitSpace      Allocate memory to permit splitting memory map.

  @retval current memory map or NULL.
**/
EFI_MEMORY_DESCRIPTOR *
OcGetCurrentMemoryMap (
  OUT UINTN   *MemoryMapSize,
  OUT UINTN   *DescriptorSize,
  OUT UINTN   *MapKey                 OPTIONAL,
  OUT UINT32  *DescriptorVersion      OPTIONAL,
  OUT UINTN   *OriginalMemoryMapSize  OPTIONAL,
  IN  BOOLEAN IncludeSplitSpace
  );

/**
  Get current memory map of custom allocation.

  @param[out]    MemoryMapSize      Resulting memory map size in bytes.
  @param[out]    MemoryMap          Resulting memory map.
  @param[out]    MapKey             Memory map key.
  @param[out]    DescriptorSize     Resulting memory map descriptor size in bytes.
  @param[out]    DescriptorVersion  Memory map descriptor version.
  @param[in]     GetMemoryMap       Custom GetMemoryMap implementation to use, optional.
  @param[in,out] TopMemory          Base top address for OcAllocatePagesFromTop allocation, number of pages after return.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcGetCurrentMemoryMapAlloc (
     OUT UINTN                  *MemoryMapSize,
     OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
     OUT UINTN                  *MapKey,
     OUT UINTN                  *DescriptorSize,
     OUT UINT32                 *DescriptorVersion,
  IN     EFI_GET_MEMORY_MAP     GetMemoryMap  OPTIONAL,
  IN OUT EFI_PHYSICAL_ADDRESS   *TopMemory  OPTIONAL
  );

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param  MemoryMapSize          Size, in bytes, of the MemoryMap buffer.
  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
VOID
OcSortMemoryMap (
  IN UINTN                      MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      DescriptorSize
  );

/**
  Shrink memory map by joining non-runtime records.
  Requires sorted memory map.

  @param[in,out]  MemoryMapSize      Memory map size in bytes, updated on shrink.
  @param[in,out]  MemoryMap          Memory map to shrink.
  @param[in]      DescriptorSize     Memory map descriptor size in bytes.

  @retval EFI_SUCCESS on success.
  @retval EFI_NOT_FOUND when cannot join anything.
**/
EFI_STATUS
OcShrinkMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  );

/**
  Deduplicate memory descriptors. Requires sorted entry list.

  @param[in,out]  EntryCount         Memory map size in entries, updated on shrink.
  @param[in,out]  MemoryMap          Memory map to shrink.
  @param[in]      DescriptorSize     Memory map descriptor size in bytes.

  @retval EFI_SUCCESS on success.
  @retval EFI_NOT_FOUND when cannot join anything.
**/
EFI_STATUS
OcDeduplicateDescriptors (
  IN OUT UINT32                 *EntryCount,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  );

/**
  Check range allocation compatibility callback.

  @param[in]  Address      Starting address.
  @param[in]  Size         Size of memory range.

  @retval TRUE when suitable for allocation.
**/
typedef
BOOLEAN
(*CHECK_ALLOCATION_RANGE) (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN UINTN                 Size
  );

/**
  Filter memory map entries.

  @param[in]      Context            Parameterised filter data.
  @param[in,out]  MemoryMapSize      Memory map size in bytes.
  @param[in,out]  MemoryMap          Memory map to filter.
  @param[in]      DescriptorSize     Memory map descriptor size in bytes.
**/
typedef
VOID
(*OC_MEMORY_FILTER) (
  IN     VOID                        *Context  OPTIONAL,
  IN     UINTN                       MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR       *MemoryMap,
  IN     UINTN                       DescriptorSize
  );

/**
  Allocate pages from the top of physical memory up to address specified in Memory.
  Unlike AllocateMaxAddress, this method guarantees to choose top most address.

  @param[in]      MemoryType       Allocated memory type.
  @param[in]      Pages            Amount of pages to allocate.
  @param[in,out]  Memory           Top address for input, allocated address for output.
  @param[in]      GetMemoryMap     Custom GetMemoryMap implementation to use, optional.
  @param[in]      CheckRange       Handler allowing to not allocate select ranges, optional.

  @retval EFI_SUCCESS on successful allocation.
**/
EFI_STATUS
OcAllocatePagesFromTop (
  IN     EFI_MEMORY_TYPE         MemoryType,
  IN     UINTN                   Pages,
  IN OUT EFI_PHYSICAL_ADDRESS    *Memory,
  IN     EFI_GET_MEMORY_MAP      GetMemoryMap  OPTIONAL,
  IN     CHECK_ALLOCATION_RANGE  CheckRange  OPTIONAL
  );

/**
  Calculate number of runtime pages in the memory map.

  @param[in]     MemoryMapSize      Memory map size in bytes.
  @param[in]     MemoryMap          Memory map to inspect.
  @param[in]     DescriptorSize     Memory map descriptor size in bytes.
  @param[out]    DescriptorCount    Number of relevant descriptors, optional.

  @retval Number of runtime pages.
**/
UINT64
OcCountRuntimePages (
  IN  UINTN                  MemoryMapSize,
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN  UINTN                  DescriptorSize,
  OUT UINTN                  *DescriptorCount OPTIONAL
  );

/**
  Calculate number of free pages in the memory map.

  @param[out]    LowerMemory    Number of free pages below 4 GB, optional.

  @retval Number of free pages.
**/
UINTN
OcCountFreePages (
  OUT UINTN                  *LowerMemory  OPTIONAL
  );

/**
  Print memory attributes table if present.
**/
VOID
OcPrintMemoryAttributesTable (
  VOID
  );

/**
  Print memory map.

  @param[in]  MemoryMapSize   Memory map size in bytes.
  @param[in]  MemoryMap       Memory map to print.
  @param[in]  DescriptorSize  Memory map descriptor size in bytes.
**/
VOID
OcPrintMemoryMap (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize
  );

/**
  Refresh memory descriptor containing the specified address.

  @param[in]  MemoryMapSize   Memory map size in bytes.
  @param[in]  MemoryMap       Memory map to refresh.
  @param[in]  DescriptorSize  Memory map descriptor size in bytes.
  @param[in]  Address         Address contained in the updated entry.
  @param[in]  Type            Memory type to assign to the entry.
  @param[in]  SetAttributes    Attributes to set.
  @param[in]  DropAttributes  Attributes to remove.

  @retval EFI_SUCCESS on success.
  @retval EFI_NOT_FOUND no entry contains the specified address.
  @retval EFI_UNSUPPORTED memory attributes are not supported by the platform.
**/
EFI_STATUS
OcUpdateDescriptors (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize,
  IN EFI_PHYSICAL_ADDRESS   Address,
  IN EFI_MEMORY_TYPE        Type,
  IN UINT64                 SetAttributes,
  IN UINT64                 DropAttributes
  );

/**
  Obtain memory attributes table.

  @param[out] MemoryAttributesEntry  memory descriptor pointer, optional.

  @retval pointer to memory attributes table.
  @retval NULL if memory attributes table is unsupported.
**/
EFI_MEMORY_ATTRIBUTES_TABLE *
OcGetMemoryAttributes (
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryAttributesEntry  OPTIONAL
  );

/**
  Refresh memory attributes entry containing the specified address.

  @param[in]  Address         Address contained in the updated entry.
  @param[in]  GetMemoryMap

  @retval EFI_SUCCESS on success.
  @retval EFI_NOT_FOUND no entry contains the specified address.
  @retval EFI_UNSUPPORTED memory attributes are not supported by the platform.
**/
EFI_STATUS
OcRebuildAttributes (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN EFI_GET_MEMORY_MAP    GetMemoryMap  OPTIONAL
  );

/**
  Count upper bound of split runtime descriptors.

  @retval amount of runtime descriptors.
**/
UINTN
OcCountSplitDescriptors (
  VOID
  );

/**
  Split memory map by memory attributes if available.
  Requires sorted memory map!

  @param[in]     MaxMemoryMapSize        Upper memory map size bound for growth.
  @param[in,out] MemoryMapSize           Current memory map size, updated on return.
  @param[in,out] MemoryMap               Memory map to split.
  @param[in]     DescriptorSize          Memory map descriptor size.

  Note, the function is guaranteed to return valid memory map, though not necessarily split.

  @retval EFI_SUCCESS on success.
  @retval EFI_UNSUPPORTED memory attributes are not supported by the platform.
  @retval EFI_OUT_OF_RESOURCES new memory map did not fit.
**/
EFI_STATUS
OcSplitMemoryMapByAttributes (
  IN     UINTN                  MaxMemoryMapSize,
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  );

/**
  Return pointer to PML4 table in PageTable and PWT and PCD flags in Flags.

  @param[out]  Flags      Current page table PWT and PCT flags.

  @retval Current page table address.
**/
PAGE_MAP_AND_DIRECTORY_POINTER  *
OcGetCurrentPageTable (
  OUT UINTN                           *Flags  OPTIONAL
  );

/**
  Return physical addrress for given virtual addrress.

  @param[in]  PageTable       Page table to use for solving.
  @param[in]  VirtualAddr     Virtual address to look up.
  @param[out] PhysicalAddr    Physical address to return.

  @retval EFI_SUCCESS on successful lookup.
**/
EFI_STATUS
OcGetPhysicalAddress (
  IN  PAGE_MAP_AND_DIRECTORY_POINTER   *PageTable  OPTIONAL,
  IN  EFI_VIRTUAL_ADDRESS              VirtualAddr,
  OUT EFI_PHYSICAL_ADDRESS             *PhysicalAddr
  );

/**
  Return EFI memory type for given type description

  @param[in]  MemoryTypeDesc  Memory type string representation.
  @param[out] MemoryType      EFI memory type to return.

  @retval EFI_NOT_FOUND on unsuccessful lookup.
  @retval EFI_INVALID_PARAMETER on wrong passed agruments.
  @retval EFI_SUCCESS on successful lookup.
**/
EFI_STATUS
OcDescToMemoryType (
  IN  CHAR8            *MemoryTypeDesc,
  OUT EFI_MEMORY_TYPE  *MemoryType
  );

/**
  Virtual memory context
**/
typedef struct OC_VMEM_CONTEXT_ {
  ///
  /// Memory pool containing memory to be spread across allocations.
  ///
  UINT8  *MemoryPool;
  ///
  /// Free pages in the memory pool.
  ///
  UINTN  FreePages;
} OC_VMEM_CONTEXT;

/**
  Allocate EfiBootServicesData virtual memory pool from boot services
  in the end of BASE_4GB of RAM. Should be called while boot services are
  still usable.

  @param[out]  Context       Virtual memory pool context.
  @param[in]   NumPages      Number of pages to be allocated in the pool.
  @param[in]   GetMemoryMap  Custom GetMemoryMap implementation to use, optional.

  @retval EFI_SUCCESS on successful allocation.
**/
EFI_STATUS
VmAllocateMemoryPool (
  OUT OC_VMEM_CONTEXT     *Context,
  IN  UINTN               NumPages,
  IN  EFI_GET_MEMORY_MAP  GetMemoryMap  OPTIONAL
  );

/**
  Allocate pages for e.g. vm page maps.

  @param[in,out]  Context   Virtual memory pool context.
  @param[in]      NumPages  Number of pages to allocate.

  @retval allocated pages or NULL.
**/
VOID *
VmAllocatePages (
  IN OUT OC_VMEM_CONTEXT  *Context,
  IN     UINTN            NumPages
  );

/**
  Map (remap) given page at physical address to given virtual address in
  the specified page table.

  @param[in,out]  Context       Virtual memory pool context.
  @param[in]      PageTable     Page table to update.
  @param[in]      VirtualAddr   Virtual memory address to map at.
  @param[in]      PhysicalAddr  Physical memory address to map from.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
VmMapVirtualPage (
  IN OUT OC_VMEM_CONTEXT                 *Context,
  IN OUT PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable  OPTIONAL,
  IN     EFI_VIRTUAL_ADDRESS             VirtualAddr,
  IN     EFI_PHYSICAL_ADDRESS            PhysicalAddr
  );

/**
  Map (remap) a range of 4K pages at physical address to given virtual address
  in the specified page table.

  @param[in,out]  Context       Virtual memory pool context.
  @param[in]      PageTable     Page table to update.
  @param[in]      VirtualAddr   Virtual memory address to map at.
  @param[in]      NumPages      Number of 4K pages to map.
  @param[in]      PhysicalAddr  Physical memory address to map from.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
VmMapVirtualPages (
  IN OUT OC_VMEM_CONTEXT                 *Context,
  IN OUT PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable  OPTIONAL,
  IN     EFI_VIRTUAL_ADDRESS             VirtualAddr,
  IN     UINT64                          NumPages,
  IN     EFI_PHYSICAL_ADDRESS            PhysicalAddr
  );

/**
  Flushes TLB caches.
**/
VOID
VmFlushCaches (
  VOID
  );

/**
  Check whether built-in allocator is initialized.

  @retval TRUE on success.
**/
BOOLEAN
UmmInitialized (
  VOID
  );

/**
  Initialize built-in allocator.

  @param[in]  Heap  Memory pool used for allocations.
  @param[in]  Size  Memory pool size.
**/
VOID
UmmSetHeap (
  IN VOID    *Heap,
  IN UINT32  Size
  );

/**
  Perform allocation from built-in allocator.

  @param[in]  Size  Allocation size.

  @retval allocated memory on success.
**/
VOID *
UmmMalloc (
  IN UINT32  Size
  );

/**
  Perform free of allocated memory. Accepts NULL pointer
  and checks whether memory belongs to itself.

  @param[in]  Ptr  Memory to free.

  @retval TRUE on success
**/
BOOLEAN
UmmFree (
  IN VOID  *Ptr
  );

#endif // OC_MEMORY_LIB_H

/** @file
  Apple RAM Disk library.

Copyright (C) 2019, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/AppleRamDisk.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleRamDiskLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#define INTERNAL_ASSERT_EXTENT_TABLE_VALID(ExtentTable)                        \
  ASSERT ((ExtentTable)->Signature  == APPLE_RAM_DISK_EXTENT_SIGNATURE);       \
  ASSERT ((ExtentTable)->Version    == APPLE_RAM_DISK_EXTENT_VERSION);         \
  ASSERT ((ExtentTable)->Reserved   == 0);                                     \
  ASSERT ((ExtentTable)->Signature2 == APPLE_RAM_DISK_EXTENT_SIGNATURE);       \
  ASSERT ((ExtentTable)->ExtentCount > 0);                                     \
  ASSERT ((ExtentTable)->ExtentCount <= ARRAY_SIZE ((ExtentTable)->Extents))

/**
  Insert allocated area into extent list. If no extent list
  was created, then it gets allocated.

  @param[in,out]  ExtentTable        Extent table, potentially pointing to NULL.
  @param[in]      AllocatedArea      Allocated area of 1 or more pages.
  @paran[in]      AllocatedAreaSize  Actual size of allocated area in bytes.
**/
STATIC
VOID
InternalAddAllocatedArea (
  IN OUT APPLE_RAM_DISK_EXTENT_TABLE  **ExtentTable,
  IN     EFI_PHYSICAL_ADDRESS         AllocatedArea,
  IN     UINTN                        AllocatedAreaSize
  )
{
  APPLE_RAM_DISK_EXTENT_TABLE  *Table;

  ASSERT (AllocatedAreaSize >= EFI_PAGE_SIZE);

  ZeroMem (
    (VOID *) AllocatedArea,
    EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (AllocatedAreaSize))
    );

  if (*ExtentTable == NULL) {
    *ExtentTable = (APPLE_RAM_DISK_EXTENT_TABLE *) AllocatedArea;

    (*ExtentTable)->Signature   = APPLE_RAM_DISK_EXTENT_SIGNATURE;
    (*ExtentTable)->Version     = APPLE_RAM_DISK_EXTENT_VERSION;
    (*ExtentTable)->Signature2  = APPLE_RAM_DISK_EXTENT_SIGNATURE;

    AllocatedArea     += EFI_PAGE_SIZE;
    AllocatedAreaSize -= EFI_PAGE_SIZE;

    OC_INLINE_STATIC_ASSERT (
      sizeof (APPLE_RAM_DISK_EXTENT_TABLE) == EFI_PAGE_SIZE,
      "Extent table different from EFI_PAGE_SIZE is unsupported!"
      );

    if (AllocatedAreaSize == 0) {
      return;
    }
  }

  Table = *ExtentTable;
  Table->Extents[Table->ExtentCount].Start  = AllocatedArea;
  Table->Extents[Table->ExtentCount].Length = AllocatedAreaSize;
  ++Table->ExtentCount;
}

/**
  Perform allocation of RemainingSize data with biggest contiguous area
  first strategy. Allocation extent map is put to the first allocated
  extent.

  @param[in]     BaseAddress    Starting allocation address.
  @param[in]     MemoryType     Requested memory type.
  @param[in,out] MemoryMap      Current memory map modified as we go.
  @param[in]     MemoryMapSize  Current memory map size.
  @param[in]     DescriptorSize Current memory map descriptor size.
  @param[in]     RemainingSize  Remaining size to allocate.
  @param[in,out] ExtentTable    Updated pointer to allocated area.

  @retval Size of allocated data.
**/
STATIC
UINTN
InternalAllocateRemainingSize (
  IN     EFI_PHYSICAL_ADDRESS         BaseAddress,
  IN     EFI_MEMORY_TYPE              MemoryType,
  IN OUT EFI_MEMORY_DESCRIPTOR        *MemoryMap,
  IN     UINTN                        MemoryMapSize,
  IN     UINTN                        DescriptorSize,
  IN     UINTN                        RemainingSize,
  IN OUT APPLE_RAM_DISK_EXTENT_TABLE  **ExtentTable
  )
{
  EFI_STATUS             Status;
  EFI_MEMORY_DESCRIPTOR  *EntryWalker;
  EFI_MEMORY_DESCRIPTOR  *BiggestEntry;
  EFI_PHYSICAL_ADDRESS   AllocatedArea;
  UINTN                  UsedSize;

  while (RemainingSize > 0 && (*ExtentTable == NULL
    || (*ExtentTable)->ExtentCount < ARRAY_SIZE ((*ExtentTable)->Extents))) {

    BiggestEntry = NULL;

    for (
      EntryWalker = MemoryMap;
      (UINT8 *)EntryWalker < ((UINT8 *)MemoryMap + MemoryMapSize);
      EntryWalker = NEXT_MEMORY_DESCRIPTOR (EntryWalker, DescriptorSize)) {

      if (EntryWalker->Type != EfiConventionalMemory || EntryWalker->PhysicalStart < BaseAddress) {
        continue;
      }

      if (BiggestEntry == NULL || EntryWalker->NumberOfPages > BiggestEntry->NumberOfPages) {
        BiggestEntry = EntryWalker;
      }
    }

    if (BiggestEntry == NULL || BiggestEntry->NumberOfPages == 0) {
      return FALSE;
    }

    UsedSize = MIN (EFI_PAGES_TO_SIZE (BiggestEntry->NumberOfPages), RemainingSize);

    AllocatedArea = BiggestEntry->PhysicalStart;
    Status = gBS->AllocatePages (
      AllocateAddress,
      MemoryType,
      EFI_SIZE_TO_PAGES (UsedSize),
      &AllocatedArea
      );

    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    InternalAddAllocatedArea (ExtentTable, AllocatedArea, UsedSize);

    RemainingSize -= UsedSize;

    BiggestEntry->PhysicalStart += EFI_SIZE_TO_PAGES (UsedSize);
    BiggestEntry->NumberOfPages -= EFI_SIZE_TO_PAGES (UsedSize);
  }

  return RemainingSize;
}

/**
  Request allocation of Size bytes in extents table.
  Extents are put to the first allocated page.

  1. Retrieve actual memory mapping.
  2. Do allocation at BASE_4GB if requested.
  3. Do additional allocation at any address if still have pages to allocate.

  @param[in] Size           Requested memory size.
  @param[in] MemoryType     Requested memory type.
  @param[in] PreferHighMem  Try to allocate in upper 4GB first.

  @retval Allocated extent table.
**/
STATIC
CONST APPLE_RAM_DISK_EXTENT_TABLE *
InternalAppleRamDiskAllocate (
  IN UINTN            Size,
  IN EFI_MEMORY_TYPE  MemoryType,
  IN BOOLEAN          PreferHighMem
  )
{
  UINTN                        MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR        *MemoryMap;
  UINTN                        DescriptorSize;
  UINTN                        RemainingSize;
  APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable;

  MemoryMap = GetCurrentMemoryMap (&MemoryMapSize, &DescriptorSize, NULL, NULL);
  if (MemoryMap == NULL) {
    return NULL;
  }

  OC_INLINE_STATIC_ASSERT (
    sizeof (APPLE_RAM_DISK_EXTENT_TABLE) == EFI_PAGE_SIZE,
    "Extent table different from EFI_PAGE_SIZE is unsupported!"
    );

  RemainingSize  = Size + EFI_PAGE_SIZE;
  ExtentTable    = NULL;

  //
  // We implement PreferHighMem to avoid colliding with the kernel, which sits
  // in the lower addresses (see more detail in AptioMemoryFix) depending on
  // KASLR offset generated randomly or with slide boot argument.
  //
  if (PreferHighMem) {
    RemainingSize = InternalAllocateRemainingSize (
      BASE_4GB,
      MemoryType,
      MemoryMap,
      MemoryMapSize,
      DescriptorSize,
      RemainingSize,
      &ExtentTable
      );
  }

  //
  // This may be tried improved when PreferHighMem is FALSE.
  // 1. One way is implement a bugtracking algorithm, similar to DF algo originally in place.
  //    The allocations will go recursively from biggest >= BASE_4G until the strategy fails.
  //    When it does, last allocation is discarded, and the strategy tries lower in this case.
  //    memory until success.
  //    The implementation must be very careful about avoiding recursion and high complexity.
  // 2. Another way could be to try to avoid allocating in kernel area specifically, though
  //    allowing to allocate in lower memory. This has a backdash of allocation failures
  //    when extent amount exceeds the limit, and may want a third pass. It may also collide
  //    with firmware pool allocator.
  //
  // None of those (or any extra) are implemented, as with PreferHighMem = TRUE it should
  // always succeed allocating in higher memory on any host with >= 8 GB of RAM, which is
  // the requirement for modern macOS.
  //
  RemainingSize = InternalAllocateRemainingSize (
    0,
    MemoryType,
    MemoryMap,
    MemoryMapSize,
    DescriptorSize,
    RemainingSize,
    &ExtentTable
    );

  if (RemainingSize > 0 && ExtentTable != NULL) {
    OcAppleRamDiskFree (ExtentTable);

    ExtentTable = NULL;
  }

  return ExtentTable;
}

CONST APPLE_RAM_DISK_EXTENT_TABLE *
OcAppleRamDiskAllocate (
  IN UINTN            Size,
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  CONST APPLE_RAM_DISK_EXTENT_TABLE *ExtentTable;

  //
  // Try to allocate preferrably above BASE_4GB to avoid colliding with the kernel.
  //
  ExtentTable = InternalAppleRamDiskAllocate (Size, MemoryType, TRUE);
  if (ExtentTable == NULL) {
    //
    // Being here means that we exceeded entry amount in the extent table.
    // Retry with any addresses. Should never happen in reality.
    //
    ExtentTable = InternalAppleRamDiskAllocate (Size, MemoryType, FALSE);
  }

  return ExtentTable;
}

BOOLEAN
OcAppleRamDiskRead (
  IN  CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN  UINT64                             Offset,
  IN  UINTN                              Size,
  OUT VOID                               *Buffer
  )
{
  UINT8                       *BufferBytes;

  UINT32                      Index;
  CONST APPLE_RAM_DISK_EXTENT *Extent;

  UINT64                      CurrentOffset;
  UINT64                      LocalOffset;
  UINTN                       LocalSize;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);
  ASSERT (Size > 0);
  ASSERT (Buffer != NULL);

  BufferBytes = Buffer;

  for (
    Index = 0, CurrentOffset = 0;
    Index < ExtentTable->ExtentCount;
    ++Index, CurrentOffset += Extent->Length
    ) {
    Extent = &ExtentTable->Extents[Index];

    if (Offset >= CurrentOffset) {
      LocalOffset = (Offset - CurrentOffset);
      LocalSize   = (UINTN)MIN ((Extent->Length - LocalOffset), Size);
      CopyMem (
        BufferBytes,
        (VOID *)((UINTN)Extent->Start + LocalOffset),
        LocalSize
        );

      Size -= LocalSize;
      if (Size == 0) {
        return TRUE;
      }

      BufferBytes += LocalSize;
      Offset      += LocalSize;
    }
  }

  return FALSE;
}

BOOLEAN
OcAppleRamDiskWrite (
  IN CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN UINT64                             Offset,
  IN UINTN                              Size,
  IN CONST VOID                         *Buffer
  )
{
  CONST UINT8                 *BufferBytes;

  UINT32                      Index;
  CONST APPLE_RAM_DISK_EXTENT *Extent;

  UINT64                      CurrentOffset;
  UINT64                      LocalOffset;
  UINTN                       LocalSize;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);
  ASSERT (Size > 0);
  ASSERT (Buffer != NULL);

  BufferBytes = Buffer;

  for (
    Index = 0, CurrentOffset = 0;
    Index < ExtentTable->ExtentCount;
    ++Index, CurrentOffset += Extent->Length
    ) {
    Extent = &ExtentTable->Extents[Index];

    if (Offset >= CurrentOffset) {
      LocalOffset = (Offset - CurrentOffset);
      LocalSize   = (UINTN)MIN ((Extent->Length - LocalOffset), Size);
      CopyMem (
        (VOID *)((UINTN)Extent->Start + LocalOffset),
        BufferBytes,
        LocalSize
        );

      Size -= LocalSize;
      if (Size == 0) {
        return TRUE;
      }

      BufferBytes += LocalSize;
      Offset      += LocalSize;
    }
  }

  return FALSE;
}

BOOLEAN
OcAppleRamDiskLoadFile (
  IN CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN EFI_FILE_PROTOCOL                  *File,
  IN UINTN                              FileSize
  )
{
  EFI_STATUS Status;
  UINT64     FilePosition;
  UINT32     Index;
  UINTN      RequestedSize;
  UINTN      ReadSize;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);
  ASSERT (File != NULL);
  ASSERT (FileSize > 0);

  FilePosition = 0;

  Status = File->SetPosition (File, FilePosition);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  for (Index = 0; FileSize > 0 && Index < ExtentTable->ExtentCount; ++Index) {
    RequestedSize = ReadSize = MIN (FileSize, ExtentTable->Extents[Index].Length);

    Status = File->Read (
      File,
      &RequestedSize,
      (VOID *) ExtentTable->Extents[Index].Start
      );

    if (EFI_ERROR (Status) || RequestedSize != ReadSize) {
      return FALSE;
    }

    FileSize -= RequestedSize;
  }

  return TRUE;
}

VOID
OcAppleRamDiskFree (
  IN CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable
  )
{
  UINT32 Index;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);

  //
  // Extents are allocated in the first page.
  //
  for (Index = 1; Index < ExtentTable->ExtentCount; ++Index) {
    gBS->FreePages (
      ExtentTable->Extents[Index].Start,
      EFI_SIZE_TO_PAGES (ExtentTable->Extents[Index].Length)
      );
  }

  gBS->FreePages (
    (UINTN) ExtentTable,
    EFI_SIZE_TO_PAGES (ExtentTable->Extents[Index].Length) + 1
    );
}

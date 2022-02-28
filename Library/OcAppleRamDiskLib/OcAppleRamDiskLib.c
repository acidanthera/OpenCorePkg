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

#include <Guid/OcVariable.h>

#include <Protocol/AppleRamDisk.h>

#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleRamDiskLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
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

  ASSERT (AllocatedArea + AllocatedAreaSize - 1 <= MAX_UINTN);
  ASSERT (AllocatedAreaSize >= EFI_PAGE_SIZE);

  ZeroMem (
    (VOID *)(UINTN)AllocatedArea,
    EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (AllocatedAreaSize))
    );

  if (*ExtentTable == NULL) {
    *ExtentTable = (APPLE_RAM_DISK_EXTENT_TABLE *)(UINTN)AllocatedArea;

    (*ExtentTable)->Signature   = APPLE_RAM_DISK_EXTENT_SIGNATURE;
    (*ExtentTable)->Version     = APPLE_RAM_DISK_EXTENT_VERSION;
    (*ExtentTable)->Signature2  = APPLE_RAM_DISK_EXTENT_SIGNATURE;

    AllocatedArea     += EFI_PAGE_SIZE;
    AllocatedAreaSize -= EFI_PAGE_SIZE;

    STATIC_ASSERT (
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
  @param[in]     TopAddress     Ending allocation address.
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
  IN     EFI_PHYSICAL_ADDRESS         TopAddress,
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
  UINT64                 BiggestSize;
  UINT64                 UsedSize;
  UINTN                  FinalUsedSize;

  //
  // Require page aligned base and top addresses.
  //
  ASSERT (BaseAddress == EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (BaseAddress)));
  ASSERT (TopAddress  == EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (TopAddress)));

  while (RemainingSize > 0 && (*ExtentTable == NULL
    || (*ExtentTable)->ExtentCount < ARRAY_SIZE ((*ExtentTable)->Extents))) {

    BiggestEntry = NULL;
    BiggestSize = 0;

    for (
      EntryWalker = MemoryMap;
      (UINT8 *)EntryWalker < ((UINT8 *)MemoryMap + MemoryMapSize);
      EntryWalker = NEXT_MEMORY_DESCRIPTOR (EntryWalker, DescriptorSize)) {

      //
      // FIXME: This currently skips segments starting before BaseAddress but potentially lasting
      // further: 0, PhysicalStart, BaseAddress, PhysicalEnd, infinity. This was done intentionally,
      // to avoid splitting one entry into two, when TopAddress is before PhysicalEnd, but can still
      // be improved.
      //
      if (EntryWalker->Type != EfiConventionalMemory
        || EntryWalker->PhysicalStart < BaseAddress
        || EntryWalker->PhysicalStart >= TopAddress) {
        continue;
      }

      UsedSize = EFI_PAGES_TO_SIZE (EntryWalker->NumberOfPages);
      if (EntryWalker->PhysicalStart + UsedSize > TopAddress) {
        //
        // Guaranteed to be page aligned as TopAddress is page aligned.
        //
        UsedSize = TopAddress - EntryWalker->PhysicalStart;
      }

      if (BiggestEntry == NULL || UsedSize > BiggestSize) {
        BiggestEntry = EntryWalker;
        BiggestSize  = UsedSize;
      }
    }

    if (BiggestEntry == NULL || BiggestSize == 0) {
      DEBUG ((
        DEBUG_INFO,
        "OCRAM: No entry for allocation %p / 0x%Lx bytes, rem 0x%Lx in 0x%Lx:0x%Lx\n",
        BiggestEntry,
        (UINT64) BiggestSize,
        (UINT64) RemainingSize,
        (UINT64) BaseAddress,
        (UINT64) TopAddress
        ));
      return RemainingSize;
    }

    FinalUsedSize = (UINTN) MIN (BiggestSize, RemainingSize);

    AllocatedArea = BiggestEntry->PhysicalStart;
    Status = gBS->AllocatePages (
      AllocateAddress,
      MemoryType,
      EFI_SIZE_TO_PAGES (FinalUsedSize),
      &AllocatedArea
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCRAM: Broken allocator for 0x%Lx in 0x%Lx bytes, rem 0x%Lx - %r\n",
        (UINT64) BiggestEntry->PhysicalStart,
        (UINT64) FinalUsedSize,
        (UINT64) RemainingSize,
        Status
        ));
      return RemainingSize;
    }

    InternalAddAllocatedArea (ExtentTable, AllocatedArea, FinalUsedSize);

    RemainingSize -= FinalUsedSize;

    BiggestEntry->PhysicalStart += EFI_SIZE_TO_PAGES (FinalUsedSize);
    BiggestEntry->NumberOfPages -= EFI_SIZE_TO_PAGES (FinalUsedSize);
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
  BOOLEAN                      Result;
  UINTN                        MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR        *MemoryMap;
  UINTN                        DescriptorSize;
  UINTN                        RemainingSize;
  APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable;

  MemoryMap = OcGetCurrentMemoryMap (&MemoryMapSize, &DescriptorSize, NULL, NULL, NULL, FALSE);
  if (MemoryMap == NULL) {
    return NULL;
  }

  STATIC_ASSERT (
    sizeof (APPLE_RAM_DISK_EXTENT_TABLE) == EFI_PAGE_SIZE,
    "Extent table different from EFI_PAGE_SIZE is unsupported!"
    );

  Result = OcOverflowAddUN (Size, EFI_PAGE_SIZE, &RemainingSize);
  if (Result) {
    return NULL;
  }

  ExtentTable    = NULL;

  //
  // We implement PreferHighMem to avoid colliding with the kernel, which sits
  // in the lower addresses (see more detail in AptioMemoryFix) depending on
  // KASLR offset generated randomly or with slide boot argument.
  //
  if (PreferHighMem) {
    RemainingSize = InternalAllocateRemainingSize (
      BASE_4GB,
      BASE_8EB,
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
    BASE_8EB,
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
  CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable;

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

  DEBUG ((
    DEBUG_BULK_INFO,
    "OCRAM: Extent allocation of %u bytes (%x) gave %p\n",
    (UINT32) Size,
    (UINT32) MemoryType,
    ExtentTable
    ));

  return ExtentTable;
}

BOOLEAN
OcAppleRamDiskRead (
  IN  CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN  UINTN                              Offset,
  IN  UINTN                              Size,
  OUT VOID                               *Buffer
  )
{
  UINT8                       *BufferBytes;

  UINT32                      Index;
  CONST APPLE_RAM_DISK_EXTENT *Extent;

  UINTN                       CurrentOffset;
  UINTN                       LocalOffset;
  UINTN                       LocalSize;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);
  ASSERT (Size > 0);
  ASSERT (Buffer != NULL);

  BufferBytes = Buffer;
  //
  // As per the allocation algorithm, the sum over all Extent->Length must be
  // smaller than MAX_UINTN.
  //
  for (
    Index = 0, CurrentOffset = 0;
    Index < ExtentTable->ExtentCount;
    ++Index, CurrentOffset += (UINTN)Extent->Length
    ) {
    Extent = &ExtentTable->Extents[Index];
    ASSERT (Extent->Start <= MAX_UINTN);
    ASSERT (Extent->Length <= MAX_UINTN);

    if (Offset >= CurrentOffset && (Offset - CurrentOffset) < Extent->Length) {
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
  IN UINTN                              Offset,
  IN UINTN                              Size,
  IN CONST VOID                         *Buffer
  )
{
  CONST UINT8                 *BufferBytes;

  UINT32                      Index;
  CONST APPLE_RAM_DISK_EXTENT *Extent;

  UINTN                       CurrentOffset;
  UINTN                       LocalOffset;
  UINTN                       LocalSize;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);
  ASSERT (Size > 0);
  ASSERT (Buffer != NULL);

  BufferBytes = Buffer;
  //
  // As per the allocation algorithm, the sum over all Extent->Length must be
  // smaller than MAX_UINTN.
  //
  for (
    Index = 0, CurrentOffset = 0;
    Index < ExtentTable->ExtentCount;
    ++Index, CurrentOffset += (UINTN)Extent->Length
    ) {
    Extent = &ExtentTable->Extents[Index];
    ASSERT (Extent->Start <= MAX_UINTN);
    ASSERT (Extent->Length <= MAX_UINTN);

    if (Offset >= CurrentOffset && (Offset - CurrentOffset) < Extent->Length) {
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
  EFI_STATUS      Status;
  UINT64          FilePosition;
  UINT32          Index;
  UINTN           RequestedSize;
  UINTN           ReadSize;
  UINTN           ExtentSize;
  SHA256_CONTEXT  Ctx;
  UINT8           Digest[SHA256_DIGEST_SIZE];
  UINT8           *TmpBuffer;
  UINT8           *ExtentBuffer;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);
  ASSERT (File != NULL);
  ASSERT (FileSize > 0);

  //
  // We need a temporary buffer in lower addresses as several motherboards on APTIO IV,
  // e.g. GA-Z77P-D3 (rev. 1.1), GA-Z87X-UD4H, etc. fail to read directly to high addresses
  // when using FAT filesystem. The original workaround to this was AvoidHighAlloc quirk.
  // REF: https://github.com/acidanthera/bugtracker/issues/449
  //
  TmpBuffer = AllocatePool (BASE_4MB);
  if (TmpBuffer == NULL) {
    return FALSE;
  }

  DEBUG_CODE_BEGIN ();
  Sha256Init (&Ctx);
  DEBUG_CODE_END ();

  FilePosition = 0;

  for (Index = 0; Index < ExtentTable->ExtentCount && FileSize > 0; ++Index) {
    ASSERT (ExtentTable->Extents[Index].Start <= MAX_UINTN);
    ASSERT (ExtentTable->Extents[Index].Length <= MAX_UINTN);

    ExtentBuffer = (VOID *)(UINTN) ExtentTable->Extents[Index].Start;
    ExtentSize   = (UINTN) ExtentTable->Extents[Index].Length;

    while (FileSize > 0 && ExtentSize > 0) {
      Status = File->SetPosition (File, FilePosition);
      if (EFI_ERROR (Status)) {
        FreePool (TmpBuffer);
        return FALSE;
      }

      RequestedSize = MIN (MIN (BASE_4MB, FileSize), ExtentSize);
      ReadSize = RequestedSize;
      Status = File->Read (
        File,
        &ReadSize,
        TmpBuffer
        );
      if (EFI_ERROR (Status) || RequestedSize != ReadSize) {
        FreePool (TmpBuffer);
        return FALSE;
      }

      DEBUG_CODE_BEGIN ();
      Sha256Update (&Ctx, TmpBuffer, ReadSize);
      DEBUG_CODE_END ();

      CopyMem (ExtentBuffer, TmpBuffer, ReadSize);

      FilePosition += ReadSize;
      ExtentBuffer += ReadSize;
      ExtentSize   -= ReadSize;
      FileSize     -= ReadSize;
    }
  }

  FreePool (TmpBuffer);

  //
  // Not enough extents.
  //
  if (FileSize != 0) {
    return FALSE;
  }

  DEBUG_CODE_BEGIN ();
  Sha256Final (&Ctx, Digest);
  DEBUG ((
    DEBUG_INFO,
    "OCRAM: SHA-256 Digest is: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
     Digest[0],  Digest[1],  Digest[2],  Digest[3],  Digest[4],  Digest[5],  Digest[6],  Digest[7],  Digest[8],  Digest[9],
    Digest[10], Digest[11], Digest[12], Digest[13], Digest[14], Digest[15], Digest[16], Digest[17], Digest[18], Digest[19],
    Digest[20], Digest[21], Digest[22], Digest[23], Digest[24], Digest[25], Digest[26], Digest[27], Digest[28], Digest[29],
    Digest[30], Digest[31]
    ));
  DEBUG_CODE_END ();
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
  ASSERT (ExtentTable->Extents[0].Start <= MAX_UINTN);
  ASSERT (ExtentTable->Extents[0].Length <= MAX_UINTN);

  //
  // Extents are allocated in the first page.
  //
  for (Index = 1; Index < ExtentTable->ExtentCount; ++Index) {
    ASSERT (ExtentTable->Extents[Index].Start <= MAX_UINTN);
    ASSERT (ExtentTable->Extents[Index].Length <= MAX_UINTN);

    gBS->FreePages (
      (UINTN)ExtentTable->Extents[Index].Start,
      (UINTN)EFI_SIZE_TO_PAGES (ExtentTable->Extents[Index].Length)
      );
  }
  //
  // One page is added to account for the header.
  //
  gBS->FreePages (
    (UINTN)ExtentTable,
    (UINTN)EFI_SIZE_TO_PAGES (ExtentTable->Extents[0].Length) + 1
    );
}

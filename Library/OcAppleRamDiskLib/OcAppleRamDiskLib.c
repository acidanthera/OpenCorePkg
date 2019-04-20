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
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#define INTERNAL_ASSERT_EXTENT_TABLE_VALID(ExtentTable)                        \
  ASSERT ((ExtentTable)->Signature  == APPLE_RAM_DISK_EXTENT_SIGNATURE);       \
  ASSERT ((ExtentTable)->Version    == APPLE_RAM_DISK_EXTENT_VERSION);         \
  ASSERT ((ExtentTable)->Reserved   == 0);                                     \
  ASSERT ((ExtentTable)->Signature2 == APPLE_RAM_DISK_EXTENT_SIGNATURE);       \
  ASSERT ((ExtentTable)->ExtentCount > 0);                                     \
  ASSERT ((ExtentTable)->ExtentCount <= ARRAY_SIZE ((ExtentTable)->Extents))

STATIC
BOOLEAN
InternalAllocateRemaining (
  OUT APPLE_RAM_DISK_EXTENT  *Extents,
  OUT UINT32                 *NumExtents,
  IN  UINT32                 MaxExtents,
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN  UINTN                  MemoryMapSize,
  IN  UINTN                  DescriptorSize,
  IN  UINTN                  RemainingPages,
  IN  EFI_MEMORY_TYPE        MemoryType
  );

STATIC
BOOLEAN
InternalAllocateRemainingWorker (
  OUT APPLE_RAM_DISK_EXTENT  *Extents,
  OUT UINT32                 *NumExtents,
  IN  UINT32                 MaxExtents,
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN  UINTN                  MemoryMapSize,
  IN  UINTN                  DescriptorSize,
  IN  UINTN                  RemainingPages,
  IN  EFI_MEMORY_TYPE        MemoryType,
  IN  BOOLEAN                Above4G
  )
{
  EFI_STATUS            Status;
  BOOLEAN               Result;

  EFI_PHYSICAL_ADDRESS  ExtentMemory;

  EFI_MEMORY_DESCRIPTOR *EntryWalker;
  EFI_MEMORY_DESCRIPTOR *BiggestEntry;

  UINT32                Index;

  ASSERT (Extents != NULL);
  ASSERT (MaxExtents > 0);
  ASSERT (NumExtents != NULL);
  ASSERT (MemoryMap != NULL);
  ASSERT (MemoryMapSize > 0);
  ASSERT (DescriptorSize > 0);
  ASSERT ((MemoryMapSize % DescriptorSize) == 0);
  ASSERT (RemainingPages > 0);

  BiggestEntry = NULL;

  for (
    EntryWalker = MemoryMap;
    (UINT8 *)EntryWalker < ((UINT8 *)MemoryMap + MemoryMapSize);
    EntryWalker = NEXT_MEMORY_DESCRIPTOR (EntryWalker, DescriptorSize)) {
    if ((EntryWalker->Type != EfiConventionalMemory)
     || (((Above4G ? 1 : 0) ^ ((EntryWalker->PhysicalStart < BASE_4GB) ? 1 : 0)) == 0)) {
      continue;
    }

    if ((BiggestEntry == NULL)
     || (EntryWalker->NumberOfPages > BiggestEntry->NumberOfPages)) {
      BiggestEntry = EntryWalker;
    }
  }

  if (BiggestEntry == NULL) {
    return FALSE;
  }

  if (BiggestEntry->NumberOfPages >= RemainingPages) {
    //
    // Backtracking - potential finish
    //
    ExtentMemory  = BiggestEntry->PhysicalStart;
    ExtentMemory += EFI_PAGES_TO_SIZE (BiggestEntry->NumberOfPages - RemainingPages);
    Status = gBS->AllocatePages (
                    AllocateAddress,
                    MemoryType,
                    RemainingPages,
                    &ExtentMemory
                    );
    if (!EFI_ERROR (Status)) {
      Extents[0].Start  = ExtentMemory;
      Extents[0].Length = EFI_PAGES_TO_SIZE (RemainingPages);
      *NumExtents = 1;

      return TRUE;
    }
  }

  if (MaxExtents == 1) {
    return FALSE;
  }
  //
  // Backtracking - iteration
  //
  BiggestEntry->Type = MemoryType;
  Result = InternalAllocateRemaining (
             &Extents[1],
             NumExtents,
             (MaxExtents - 1),
             MemoryMap,
             MemoryMapSize,
             DescriptorSize,
             (RemainingPages - BiggestEntry->NumberOfPages),
             MemoryType
             );
  BiggestEntry->Type = EfiConventionalMemory;

  if (!Result) {
    return FALSE;
  }

  ExtentMemory = BiggestEntry->PhysicalStart;
  Status = gBS->AllocatePages (
                  AllocateAddress,
                  MemoryType,
                  BiggestEntry->NumberOfPages,
                  &ExtentMemory
                  );
  if (EFI_ERROR (Status)) {
    //
    // Clean up all successfully backtracked allocations.
    //
    for (Index = 0; Index < *NumExtents; ++Index) {
      gBS->FreePages (
             Extents[Index + 1].Start,
             EFI_SIZE_TO_PAGES (Extents[Index + 1].Length)
             );
    }

    *NumExtents = 0;
    return FALSE;
  }

  Extents[0].Start  = ExtentMemory;
  Extents[0].Length = EFI_PAGES_TO_SIZE (BiggestEntry->NumberOfPages);
  ++(*NumExtents);

  return TRUE;
}

STATIC
BOOLEAN
InternalAllocateRemaining (
  OUT APPLE_RAM_DISK_EXTENT  *Extents,
  OUT UINT32                 *NumExtents,
  IN  UINT32                 MaxExtents,
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN  UINTN                  MemoryMapSize,
  IN  UINTN                  DescriptorSize,
  IN  UINTN                  RemainingPages,
  IN  EFI_MEMORY_TYPE        MemoryType
  )
{
  BOOLEAN Result;
  //
  // Prefer fitting all contents above 4 GB.
  //
  Result = InternalAllocateRemainingWorker (
              Extents,
              NumExtents,
              MaxExtents,
              MemoryMap,
              MemoryMapSize,
              DescriptorSize,
              RemainingPages,
              MemoryType,
              TRUE
              );
  if (!Result) {
    Result = InternalAllocateRemainingWorker (
                Extents,
                NumExtents,
                MaxExtents,
                MemoryMap,
                MemoryMapSize,
                DescriptorSize,
                RemainingPages,
                MemoryType,
                FALSE
                );
  }

  return Result;
}

CONST APPLE_RAM_DISK_EXTENT_TABLE *
OcAppleRamDiskAllocate (
  IN UINTN            Size,
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  EFI_STATUS                  Status;
  BOOLEAN                     Result;

  UINTN                       MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR       *MemoryMap;
  UINTN                       MapKey;
  UINTN                       DescriptorSize;
  UINT32                      DescriptorVersion;

  APPLE_RAM_DISK_EXTENT_TABLE *ExtentTable;
  UINT64                      ExtentTableMemory;

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  MemoryType,
                  EFI_SIZE_TO_PAGES (sizeof (*ExtentTable)),
                  &ExtentTableMemory
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  NULL,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    gBS->FreePages (
           ExtentTableMemory,
           EFI_SIZE_TO_PAGES (sizeof (*ExtentTable))
           );
    return NULL;
  }
  //
  // Apple uses 1024 as constant, however it will grow by at least
  // DescriptorSize.
  //
  Result = OcOverflowAddUN (
             MemoryMapSize,
             MAX (DescriptorSize, 1024),
             &MemoryMapSize
             );
  if (Result) {
    gBS->FreePages (
           ExtentTableMemory,
           EFI_SIZE_TO_PAGES (sizeof (*ExtentTable))
           );
    return NULL;
  }

  MemoryMap = AllocatePool (MemoryMapSize);
  if (MemoryMap == NULL) {
    gBS->FreePages (
           ExtentTableMemory,
           EFI_SIZE_TO_PAGES (sizeof (*ExtentTable))
           );
    return NULL;
  }

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  if (EFI_ERROR (Status)) {
    gBS->FreePages (
           ExtentTableMemory,
           EFI_SIZE_TO_PAGES (sizeof (*ExtentTable))
           );
    FreePool (MemoryMap);
    return NULL;
  }

  ExtentTable = (VOID *)(UINTN)ExtentTableMemory;
  ExtentTable->Signature   = APPLE_RAM_DISK_EXTENT_SIGNATURE;
  ExtentTable->Version     = APPLE_RAM_DISK_EXTENT_VERSION;
  ExtentTable->Reserved    = 0;
  ExtentTable->Signature2  = APPLE_RAM_DISK_EXTENT_SIGNATURE;
  ExtentTable->ExtentCount = 0;
  Result = InternalAllocateRemaining (
             ExtentTable->Extents,
             &ExtentTable->ExtentCount,
             ARRAY_SIZE (ExtentTable->Extents),
             MemoryMap,
             MemoryMapSize,
             DescriptorSize,
             EFI_SIZE_TO_PAGES (Size),
             MemoryType
             );

  FreePool (MemoryMap);

  if (!Result) {
    gBS->FreePages (
           ExtentTableMemory,
           EFI_SIZE_TO_PAGES (sizeof (*ExtentTable))
           );
    return NULL;
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
  BOOLEAN    Result;

  UINTN      FileBufferSize;
  VOID       *FileBuffer;

  UINTN      CurrentSize;
  UINT64     FilePosition;

  ASSERT (ExtentTable != NULL);
  INTERNAL_ASSERT_EXTENT_TABLE_VALID (ExtentTable);
  ASSERT (File != NULL);
  ASSERT (FileSize > 0);

  FileBufferSize = FileSize;

  while (TRUE) {
    FileBuffer = AllocatePool (FileBufferSize);
    if (FileBuffer != NULL) {
      break;
    }

    if (FileBufferSize <= 2) {
      return FALSE;
    }

    FileBufferSize = ((FileBufferSize / 2) + 1);
  }

  FilePosition = 0;
  CurrentSize  = FileBufferSize;
  do {
    Status = GetFileData (File, FilePosition, CurrentSize, FileBuffer);
    if (!EFI_ERROR (Status)) {
      FreePool (FileBuffer);
      return FALSE;
    }

    Result = OcAppleRamDiskWrite (
               ExtentTable,
               FilePosition,
               CurrentSize,
               FileBuffer
               );
    if (!Result) {
      FreePool (FileBuffer);
      return FALSE;
    }

    FilePosition += CurrentSize;
    FileSize     -= CurrentSize;
    CurrentSize   = MIN (FileSize, FileBufferSize);
  } while (FileSize != 0);

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

  for (Index = 0; Index < ExtentTable->ExtentCount; ++Index) {
    gBS->FreePages (
           ExtentTable->Extents[Index].Start,
           EFI_SIZE_TO_PAGES (ExtentTable->Extents[Index].Length)
           );
  }

  gBS->FreePages (
         (UINTN)ExtentTable,
         EFI_SIZE_TO_PAGES (sizeof (*ExtentTable))
         );
}

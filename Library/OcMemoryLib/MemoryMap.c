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

#include <Uefi.h>

#include <Guid/MemoryAttributesTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

EFI_MEMORY_DESCRIPTOR *
OcGetCurrentMemoryMap (
  OUT UINTN   *MemoryMapSize,
  OUT UINTN   *DescriptorSize,
  OUT UINTN   *MapKey                 OPTIONAL,
  OUT UINT32  *DescriptorVersion      OPTIONAL,
  OUT UINTN   *OriginalMemoryMapSize  OPTIONAL,
  IN  BOOLEAN IncludeSplitSpace
  )
{
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  EFI_STATUS              Status;
  UINTN                   MapKeyValue;
  UINTN                   OriginalSize;
  UINTN                   ExtraSize;
  UINT32                  DescriptorVersionValue;
  BOOLEAN                 Result;

  *MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (
    MemoryMapSize,
    NULL,
    &MapKeyValue,
    DescriptorSize,
    &DescriptorVersionValue
    );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  if (IncludeSplitSpace) {
    ExtraSize = OcCountSplitDescriptors () * *DescriptorSize;
  } else {
    ExtraSize = 0;
  }

  //
  // Apple uses 1024 as constant, however it will grow by at least
  // DescriptorSize.
  //
  Result = OcOverflowAddUN (
    *MemoryMapSize,
    MAX (*DescriptorSize + ExtraSize, 1024 + ExtraSize),
    MemoryMapSize
    );

  if (Result) {
    return NULL;
  }

  OriginalSize = *MemoryMapSize;
  MemoryMap = AllocatePool (OriginalSize);
  if (MemoryMap == NULL) {
    return NULL;
  }

  Status = gBS->GetMemoryMap (
    MemoryMapSize,
    MemoryMap,
    &MapKeyValue,
    DescriptorSize,
    &DescriptorVersionValue
    );

  if (EFI_ERROR (Status)) {
    FreePool (MemoryMap);
    return NULL;
  }

  if (MapKey != NULL) {
    *MapKey = MapKeyValue;
  }

  if (DescriptorVersion != NULL) {
    *DescriptorVersion = DescriptorVersionValue;
  }

  if (OriginalMemoryMapSize != NULL) {
    *OriginalMemoryMapSize = OriginalSize;
  }

  return MemoryMap;
}

EFI_STATUS
OcGetCurrentMemoryMapAlloc (
     OUT UINTN                  *MemoryMapSize,
     OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
     OUT UINTN                  *MapKey,
     OUT UINTN                  *DescriptorSize,
     OUT UINT32                 *DescriptorVersion,
  IN     EFI_GET_MEMORY_MAP     GetMemoryMap  OPTIONAL,
  IN OUT EFI_PHYSICAL_ADDRESS   *TopMemory  OPTIONAL
  )
{
  EFI_STATUS           Status;
  EFI_PHYSICAL_ADDRESS MemoryMapAlloc;

  *MemoryMapSize = 0;
  *MemoryMap     = NULL;

  if (GetMemoryMap == NULL) {
    GetMemoryMap = gBS->GetMemoryMap;
  }

  Status = GetMemoryMap (
    MemoryMapSize,
    *MemoryMap,
    MapKey,
    DescriptorSize,
    DescriptorVersion
    );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_INFO, "OCMM: Insane GetMemoryMap %r\n", Status));
    return Status;
  }

  do {
    //
    // This is done because extra allocations may increase memory map size.
    //
    *MemoryMapSize += 512;

    //
    // Requested to allocate from top via pages.
    // This may be needed, because the pool memory may collide with the kernel.
    //
    if (TopMemory != NULL) {
      MemoryMapAlloc = *TopMemory;
      *TopMemory     = EFI_SIZE_TO_PAGES (*MemoryMapSize);

      Status = OcAllocatePagesFromTop (
        EfiBootServicesData,
        (UINTN) *TopMemory,
        &MemoryMapAlloc,
        GetMemoryMap,
        NULL
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCMM: Temp memory map allocation from top failure - %r\n", Status));
        *MemoryMap = NULL;
        return Status;
      }

      *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)(UINTN) MemoryMapAlloc;
    } else {
      *MemoryMap = AllocatePool (*MemoryMapSize);
      if (*MemoryMap == NULL) {
        DEBUG ((DEBUG_INFO, "OCMM: Temp memory map direct allocation failure\n"));
        return EFI_OUT_OF_RESOURCES;
      }
    }

    Status = GetMemoryMap (
      MemoryMapSize,
      *MemoryMap,
      MapKey,
      DescriptorSize,
      DescriptorVersion
      );

    if (EFI_ERROR (Status)) {
      if (TopMemory != NULL) {
        gBS->FreePages (
          (EFI_PHYSICAL_ADDRESS) ((UINTN) *MemoryMap),
          (UINTN) *TopMemory
          );
      } else {
        FreePool (*MemoryMap);
      }

      *MemoryMap = NULL;
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "OCMM: Failed to obtain memory map - %r\n", Status));
  }

  return Status;
}

VOID
OcSortMemoryMap (
  IN UINTN                      MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *NextMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR       TempMemoryMap;

  MemoryMapEntry = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) MemoryMap + MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->PhysicalStart > NextMemoryMapEntry->PhysicalStart) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, sizeof(EFI_MEMORY_DESCRIPTOR));
      }

      NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
    }

    MemoryMapEntry      = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry  = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
}

EFI_STATUS
OcShrinkMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  )
{
  EFI_STATUS              Status;
  UINTN                   SizeFromDescToEnd;
  UINT64                  Bytes;
  EFI_MEMORY_DESCRIPTOR   *PrevDesc;
  EFI_MEMORY_DESCRIPTOR   *Desc;
  BOOLEAN                 CanBeJoinedFree;
  BOOLEAN                 CanBeJoinedRt;
  BOOLEAN                 HasEntriesToRemove;

  Status = EFI_NOT_FOUND;

  if (*MemoryMapSize <= DescriptorSize) {
    return Status;
  }

  PrevDesc           = MemoryMap;
  Desc               = NEXT_MEMORY_DESCRIPTOR (PrevDesc, DescriptorSize);
  SizeFromDescToEnd  = *MemoryMapSize - DescriptorSize;
  *MemoryMapSize     = DescriptorSize;
  HasEntriesToRemove = FALSE;

  while (SizeFromDescToEnd > 0) {
    Bytes = EFI_PAGES_TO_SIZE (PrevDesc->NumberOfPages);
    CanBeJoinedFree = FALSE;
    CanBeJoinedRt   = FALSE;
    if (Desc->Attribute == PrevDesc->Attribute
      && PrevDesc->PhysicalStart + Bytes == Desc->PhysicalStart) {
      //
      // It *should* be safe to join this with conventional memory, because the firmware should not use
      // GetMemoryMap for allocation, and for the kernel it does not matter, since it joins them.
      //
      CanBeJoinedFree = (
          Desc->Type == EfiBootServicesCode
          || Desc->Type == EfiBootServicesData
          || Desc->Type == EfiConventionalMemory
          || Desc->Type == EfiLoaderCode
          || Desc->Type == EfiLoaderData
        ) && (
          PrevDesc->Type == EfiBootServicesCode
          || PrevDesc->Type == EfiBootServicesData
          || PrevDesc->Type == EfiConventionalMemory
          || PrevDesc->Type == EfiLoaderCode
          || PrevDesc->Type == EfiLoaderData
        );

      CanBeJoinedRt = (
          Desc->Type == EfiRuntimeServicesCode
          && PrevDesc->Type == EfiRuntimeServicesCode
        ) || (
          Desc->Type == EfiRuntimeServicesData
          && PrevDesc->Type == EfiRuntimeServicesData
        );
    }

    if (CanBeJoinedFree) {
      //
      // Two entries are the same/similar - join them
      //
      PrevDesc->Type           = EfiConventionalMemory;
      PrevDesc->NumberOfPages += Desc->NumberOfPages;
      HasEntriesToRemove       = TRUE;
      Status                   = EFI_SUCCESS;
    } else if (CanBeJoinedRt) {
      PrevDesc->NumberOfPages += Desc->NumberOfPages;
      HasEntriesToRemove       = TRUE;
      Status                   = EFI_SUCCESS;
    } else {
      //
      // Cannot be joined - we need to move to next
      //
      *MemoryMapSize += DescriptorSize;
      PrevDesc        = NEXT_MEMORY_DESCRIPTOR (PrevDesc, DescriptorSize);
      if (HasEntriesToRemove) {
        //
        // Have entries between PrevDesc and Desc which are joined to PrevDesc,
        // we need to copy [Desc, end of list] to PrevDesc + 1
        //
        CopyMem (PrevDesc, Desc, SizeFromDescToEnd);
        Desc = PrevDesc;
        HasEntriesToRemove = FALSE;
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
    SizeFromDescToEnd -= DescriptorSize;
  }

  //
  // Handle last entries if they were merged.
  //
  if (HasEntriesToRemove) {
    *MemoryMapSize += DescriptorSize;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcDeduplicateDescriptors (
  IN OUT UINT32                 *EntryCount,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  )
{
  EFI_STATUS              Status;
  UINTN                   EntriesToGo;
  EFI_MEMORY_DESCRIPTOR   *PrevDesc;
  EFI_MEMORY_DESCRIPTOR   *Desc;
  BOOLEAN                 IsDuplicate;
  BOOLEAN                 HasEntriesToRemove;

  Status = EFI_NOT_FOUND;

  if (*EntryCount <= 1) {
    return Status;
  }

  PrevDesc           = MemoryMap;
  Desc               = NEXT_MEMORY_DESCRIPTOR (PrevDesc, DescriptorSize);
  EntriesToGo        = *EntryCount - 1;
  *EntryCount        = 1;
  HasEntriesToRemove = FALSE;

  while (EntriesToGo > 0) {
    IsDuplicate = Desc->PhysicalStart == PrevDesc->PhysicalStart
      && Desc->NumberOfPages == PrevDesc->NumberOfPages;

    if (IsDuplicate) {
      //
      // Two entries are duplicate, remove them.
      //
      Status               = EFI_SUCCESS;
      HasEntriesToRemove   = TRUE;
    } else {
      //
      // Not duplicates - we need to move to next
      //
      ++(*EntryCount);
      PrevDesc = NEXT_MEMORY_DESCRIPTOR (PrevDesc, DescriptorSize);
      if (HasEntriesToRemove) {
        //
        // Have same entries between PrevDesc and Desc which are replaced by PrevDesc,
        // we need to copy [Desc, end of list] to PrevDesc + 1.
        //
        CopyMem (PrevDesc, Desc, EntriesToGo * DescriptorSize);
        Desc = PrevDesc;
        HasEntriesToRemove = FALSE;
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
    --EntriesToGo;
  }

  //
  // Handle last entries if they were deduplicated.
  //
  if (HasEntriesToRemove) {
    ++(*EntryCount);
  }

  return Status;
}

EFI_STATUS
OcUpdateDescriptors (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize,
  IN EFI_PHYSICAL_ADDRESS   Address,
  IN EFI_MEMORY_TYPE        Type,
  IN UINT64                 SetAttributes,
  IN UINT64                 DropAttributes
  )
{
  UINTN  Index;
  UINTN  EntryCount;

  EntryCount = MemoryMapSize / DescriptorSize;

  for (Index = 0; Index < EntryCount; ++Index) {
    if (AREA_WITHIN_DESCRIPTOR (MemoryMap, Address, 1)) {
      MemoryMap->Type       = Type;
      MemoryMap->Attribute |= SetAttributes;
      MemoryMap->Attribute &= ~DropAttributes;
      return EFI_SUCCESS;
    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (
      MemoryMap,
      DescriptorSize
      );
  }

  return EFI_NOT_FOUND;
}

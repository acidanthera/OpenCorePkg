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
    ExtraSize = OcCountSplitDescritptors () * *DescriptorSize;
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
GetCurrentMemoryMapAlloc (
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

      Status = AllocatePagesFromTop (
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
ShrinkMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  )
{
  UINTN                   SizeFromDescToEnd;
  UINT64                  Bytes;
  EFI_MEMORY_DESCRIPTOR   *PrevDesc;
  EFI_MEMORY_DESCRIPTOR   *Desc;
  BOOLEAN                 CanBeJoined;
  BOOLEAN                 HasEntriesToRemove;

  PrevDesc           = MemoryMap;
  Desc               = NEXT_MEMORY_DESCRIPTOR (PrevDesc, DescriptorSize);
  SizeFromDescToEnd  = *MemoryMapSize - DescriptorSize;
  *MemoryMapSize     = DescriptorSize;
  HasEntriesToRemove = FALSE;

  while (SizeFromDescToEnd > 0) {
    Bytes = EFI_PAGES_TO_SIZE (PrevDesc->NumberOfPages);
    CanBeJoined = FALSE;
    if (Desc->Attribute == PrevDesc->Attribute
      && PrevDesc->PhysicalStart + Bytes == Desc->PhysicalStart) {
      //
      // It *should* be safe to join this with conventional memory, because the firmware should not use
      // GetMemoryMap for allocation, and for the kernel it does not matter, since it joins them.
      //
      CanBeJoined = (Desc->Type == EfiBootServicesCode ||
        Desc->Type == EfiBootServicesData ||
        Desc->Type == EfiConventionalMemory ||
        Desc->Type == EfiLoaderCode ||
        Desc->Type == EfiLoaderData) && (
        PrevDesc->Type == EfiBootServicesCode ||
        PrevDesc->Type == EfiBootServicesData ||
        PrevDesc->Type == EfiConventionalMemory ||
        PrevDesc->Type == EfiLoaderCode ||
        PrevDesc->Type == EfiLoaderData);
    }

    if (CanBeJoined) {
      //
      // Two entries are the same/similar - join them
      //
      PrevDesc->Type           = EfiConventionalMemory;
      PrevDesc->NumberOfPages += Desc->NumberOfPages;
      HasEntriesToRemove       = TRUE;
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
}

EFI_STATUS
AllocatePagesFromTop (
  IN     EFI_MEMORY_TYPE         MemoryType,
  IN     UINTN                   Pages,
  IN OUT EFI_PHYSICAL_ADDRESS    *Memory,
  IN     EFI_GET_MEMORY_MAP      GetMemoryMap,
  IN     CHECK_ALLOCATION_RANGE  CheckRange  OPTIONAL
  )
{
  EFI_STATUS              Status;
  UINTN                   MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  UINTN                   MapKey;
  UINTN                   DescriptorSize;
  UINT32                  DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR   *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR   *Desc;

  Status = GetCurrentMemoryMapAlloc (
    &MemoryMapSize,
    &MemoryMap,
    &MapKey,
    &DescriptorSize,
    &DescriptorVersion,
    GetMemoryMap,
    NULL
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  MemoryMapEnd = NEXT_MEMORY_DESCRIPTOR (MemoryMap, MemoryMapSize);
  Desc = PREV_MEMORY_DESCRIPTOR (MemoryMapEnd, DescriptorSize);

  for ( ; Desc >= MemoryMap; Desc = PREV_MEMORY_DESCRIPTOR (Desc, DescriptorSize)) {
    //
    // We are looking for some free memory descriptor that contains enough
    // space below the specified memory.
    //
    if (Desc->Type == EfiConventionalMemory && Pages <= Desc->NumberOfPages &&
      Desc->PhysicalStart + EFI_PAGES_TO_SIZE (Pages) <= *Memory) {

      //
      // Free block found
      //
      if (Desc->PhysicalStart + EFI_PAGES_TO_SIZE (Desc->NumberOfPages) <= *Memory) {
        //
        // The whole block is under Memory: allocate from the top of the block
        //
        *Memory = Desc->PhysicalStart + EFI_PAGES_TO_SIZE (Desc->NumberOfPages - Pages);
      } else {
        //
        // The block contains enough pages under Memory, but spans above it - allocate below Memory
        //
        *Memory = *Memory - EFI_PAGES_TO_SIZE (Pages);
      }

      //
      // Ensure that the found block does not overlap with the restricted area.
      //
      if (CheckRange != NULL && CheckRange (*Memory, EFI_PAGES_TO_SIZE (Pages))) {
        continue;
      }

      Status = gBS->AllocatePages (
        AllocateAddress,
        MemoryType,
        Pages,
        Memory
        );

      break;
    }
  }

  FreePool (MemoryMap);

  return Status;
}

UINT64
CountRuntimePages (
  IN  UINTN                  MemoryMapSize,
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN  UINTN                  DescriptorSize,
  OUT UINTN                  *DescriptorCount OPTIONAL
  )
{
  UINTN                  DescNum;
  UINT64                 PageNum;
  UINTN                  NumEntries;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Desc;

  DescNum    = 0;
  PageNum    = 0;
  NumEntries = MemoryMapSize / DescriptorSize;
  Desc       = MemoryMap;

  for (Index = 0; Index < NumEntries; ++Index) {
    if (Desc->Type != EfiReservedMemoryType
      && (Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      ++DescNum;
      PageNum += Desc->NumberOfPages;
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  if (DescriptorCount != NULL) {
    *DescriptorCount = DescNum;
  }

  return PageNum;
}

UINTN
CountFreePages (
  OUT UINTN                  *LowerMemory  OPTIONAL
  )
{
  UINTN                        MemoryMapSize;
  UINTN                        DescriptorSize;
  EFI_MEMORY_DESCRIPTOR        *MemoryMap;
  EFI_MEMORY_DESCRIPTOR        *EntryWalker;
  UINTN                        FreePages;

  FreePages = 0;
  if (LowerMemory != NULL) {
    *LowerMemory = 0;
  }

  MemoryMap = OcGetCurrentMemoryMap (&MemoryMapSize, &DescriptorSize, NULL, NULL, NULL, FALSE);
  if (MemoryMap == NULL) {
    return 0;
  }

  for (
    EntryWalker = MemoryMap;
    (UINT8 *) EntryWalker < ((UINT8 *) MemoryMap + MemoryMapSize);
    EntryWalker = NEXT_MEMORY_DESCRIPTOR (EntryWalker, DescriptorSize)) {

    if (EntryWalker->Type != EfiConventionalMemory) {
      continue;
    }

    //
    // This cannot overflow even on 32-bit systems unless they have > 16 TB of RAM,
    // just assert to ensure that we have valid MemoryMap.
    //
    ASSERT (EntryWalker->NumberOfPages <= MAX_UINTN);
    ASSERT (MAX_UINTN - EntryWalker->NumberOfPages >= FreePages);
    FreePages += (UINTN) EntryWalker->NumberOfPages;

    if (LowerMemory == NULL || EntryWalker->PhysicalStart >= BASE_4GB) {
      continue;
    }

    if (EntryWalker->PhysicalStart + EFI_PAGES_TO_SIZE (EntryWalker->NumberOfPages) > BASE_4GB) {
      *LowerMemory += (UINTN) EFI_SIZE_TO_PAGES (BASE_4GB - EntryWalker->PhysicalStart);
    } else {
      *LowerMemory += (UINTN) EntryWalker->NumberOfPages;
    }
  }

  FreePool (MemoryMap);

  return FreePages;
}

VOID
OcPrintMemoryAttributesTable (
  VOID
  )
{
  UINTN                             Index;
  CONST EFI_MEMORY_ATTRIBUTES_TABLE *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR             *MemoryAttributesEntry;

  MemoryAttributesTable = OcGetMemoryAttributes (&MemoryAttributesEntry);
  if (MemoryAttributesTable == NULL) {
    DEBUG ((DEBUG_INFO, "OCMM: MemoryAttributesTable is not present!\n"));
    return;
  }

  DEBUG ((DEBUG_INFO, "OCMM: MemoryAttributesTable:\n"));
  DEBUG ((DEBUG_INFO, "OCMM:   Version              - 0x%08x\n", MemoryAttributesTable->Version));
  DEBUG ((DEBUG_INFO, "OCMM:   NumberOfEntries      - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
  DEBUG ((DEBUG_INFO, "OCMM:   DescriptorSize       - 0x%08x\n", MemoryAttributesTable->DescriptorSize));

  for (Index = 0; Index < MemoryAttributesTable->NumberOfEntries; ++Index) {
    DEBUG ((DEBUG_INFO, "OCMM: Entry (0x%x)\n", MemoryAttributesEntry));
    DEBUG ((DEBUG_INFO, "OCMM:   Type              - 0x%x\n", MemoryAttributesEntry->Type));
    DEBUG ((DEBUG_INFO, "OCMM:   PhysicalStart     - 0x%016lx\n", MemoryAttributesEntry->PhysicalStart));
    DEBUG ((DEBUG_INFO, "OCMM:   VirtualStart      - 0x%016lx\n", MemoryAttributesEntry->VirtualStart));
    DEBUG ((DEBUG_INFO, "OCMM:   NumberOfPages     - 0x%016lx\n", MemoryAttributesEntry->NumberOfPages));
    DEBUG ((DEBUG_INFO, "OCMM:   Attribute         - 0x%016lx\n", MemoryAttributesEntry->Attribute));

    MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR (
      MemoryAttributesEntry,
      MemoryAttributesTable->DescriptorSize
      );
  }
}

VOID
OcPrintMemoryMap (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize
  )
{
  UINTN     Index;
  UINT32    NumberOfEntries;

  NumberOfEntries = (UINT32) (MemoryMapSize / DescriptorSize);

  DEBUG ((DEBUG_INFO, "OCMM: MemoryMap:\n"));
  DEBUG ((DEBUG_INFO, "OCMM:   Size                 - 0x%08x\n", MemoryMapSize));
  DEBUG ((DEBUG_INFO, "OCMM:   NumberOfEntries      - 0x%08x\n", NumberOfEntries));
  DEBUG ((DEBUG_INFO, "OCMM:   DescriptorSize       - 0x%08x\n", DescriptorSize));

  for (Index = 0; Index < NumberOfEntries; ++Index) {
    DEBUG ((DEBUG_INFO, "OCMM: Entry (0x%x)\n", MemoryMap));
    DEBUG ((DEBUG_INFO, "OCMM:   Type              - 0x%x\n", MemoryMap->Type));
    DEBUG ((DEBUG_INFO, "OCMM:   PhysicalStart     - 0x%016lx\n", MemoryMap->PhysicalStart));
    DEBUG ((DEBUG_INFO, "OCMM:   VirtualStart      - 0x%016lx\n", MemoryMap->VirtualStart));
    DEBUG ((DEBUG_INFO, "OCMM:   NumberOfPages     - 0x%016lx\n", MemoryMap->NumberOfPages));
    DEBUG ((DEBUG_INFO, "OCMM:   Attribute         - 0x%016lx\n", MemoryMap->Attribute));

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (
      MemoryMap,
      DescriptorSize
      );
  }
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

EFI_MEMORY_ATTRIBUTES_TABLE *
OcGetMemoryAttributes (
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryAttributesEntry  OPTIONAL
  )
{
  EFI_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable; 
  UINTN                        Index;

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiMemoryAttributesTableGuid)) {
      MemoryAttributesTable = gST->ConfigurationTable[Index].VendorTable;
      if (MemoryAttributesEntry != NULL) {
        *MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *) (MemoryAttributesTable + 1);
      }
      return MemoryAttributesTable;
    }
  }

  return NULL;
}

EFI_STATUS
OcUpdateAttributes (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN EFI_MEMORY_TYPE       Type,
  IN UINT64                SetAttributes,
  IN UINT64                DropAttributes
  )
{
  CONST EFI_MEMORY_ATTRIBUTES_TABLE *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR             *MemoryAttributesEntry;

  MemoryAttributesTable = OcGetMemoryAttributes (&MemoryAttributesEntry);
  if (MemoryAttributesTable == NULL) {
    return EFI_UNSUPPORTED;
  }

  return OcUpdateDescriptors (
    MemoryAttributesTable->NumberOfEntries * MemoryAttributesTable->DescriptorSize,
    MemoryAttributesEntry,
    MemoryAttributesTable->DescriptorSize,
    Address,
    Type,
    SetAttributes,
    DropAttributes
    );
}

UINTN
OcCountSplitDescritptors (
  VOID
  )
{
  UINTN                             Index;
  UINTN                             DescriptorCount;
  CONST EFI_MEMORY_ATTRIBUTES_TABLE *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR             *MemoryAttributesEntry;

  MemoryAttributesTable = OcGetMemoryAttributes (&MemoryAttributesEntry);
  if (MemoryAttributesTable == NULL) {
    return 0;
  }

  DescriptorCount = 0;
  for (Index = 0; Index < MemoryAttributesTable->NumberOfEntries; ++Index) {
    if (MemoryAttributesEntry->Type == EfiRuntimeServicesCode
      || MemoryAttributesEntry->Type == EfiRuntimeServicesData) {
      ++DescriptorCount;
    }

    MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR (
      MemoryAttributesEntry,
      MemoryAttributesTable->DescriptorSize
      );
  }

  return DescriptorCount;
}

/**
  Split memory map descriptor by attribute.

  @param[in,out] RetMemoryMapEntry    Pointer to descriptor in the memory map, updated to next proccessed.
  @param[in,out] CurrentEntryIndex    Current index of the descriptor in the memory map, updated on increase.
  @param[in,out] CurrentEntryCount    Number of descriptors in the memory map, updated on increase.
  @param[in]     TotalEntryCount      Max number of descriptors in the memory map.
  @param[in]     MemoryAttribute      Memory attribute used for splitting.
  @param[in]     DescriptorSize       Memory map descriptor size.

  @retval EFI_SUCCESS on success.
  @retval EFI_OUT_OF_RESOURCES when there are not enough free descriptor slots.
**/
STATIC
EFI_STATUS
OcSplitMemoryEntryByAttribute (
  IN OUT EFI_MEMORY_DESCRIPTOR  **RetMemoryMapEntry,
  IN OUT UINTN                  *CurrentEntryIndex,
  IN OUT UINTN                  *CurrentEntryCount,
  IN     UINTN                  TotalEntryCount,
  IN     EFI_MEMORY_DESCRIPTOR  *MemoryAttribte,
  IN     UINTN                  DescriptorSize

  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NewMemoryMapEntry;
  UINTN                  DiffPages;

  MemoryMapEntry = *RetMemoryMapEntry;

  //
  // Memory attribute starts after our descriptor.
  // Shorten the existing descriptor and insert the new one after it.
  // [DESC1] -> [DESC1][DESC2]
  //
  if (MemoryAttribte->PhysicalStart > MemoryMapEntry->PhysicalStart) {
    if (*CurrentEntryCount == TotalEntryCount) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    DiffPages         = (UINTN) EFI_SIZE_TO_PAGES (MemoryAttribte->PhysicalStart - MemoryMapEntry->PhysicalStart);
    CopyMem (
      NewMemoryMapEntry,
      MemoryMapEntry,
      DescriptorSize * (*CurrentEntryCount - *CurrentEntryIndex)
      );
    MemoryMapEntry->NumberOfPages     = DiffPages;
    NewMemoryMapEntry->PhysicalStart  = MemoryAttribte->PhysicalStart;
    NewMemoryMapEntry->NumberOfPages -= DiffPages;

    MemoryMapEntry = NewMemoryMapEntry;

    //
    // Current processed entry is now the one we inserted.
    //
    ++(*CurrentEntryIndex);
    ++(*CurrentEntryCount);
  }

  ASSERT (MemoryAttribte->PhysicalStart == MemoryMapEntry->PhysicalStart);

  //
  // Memory attribute matches our descriptor.
  // Simply update its protection.
  // [DESC1] -> [DESC1*]
  //
  if (MemoryMapEntry->NumberOfPages == MemoryAttribte->NumberOfPages) {
    MemoryMapEntry->Type = MemoryAttribte->Type;
    *RetMemoryMapEntry = MemoryMapEntry;
    return EFI_SUCCESS;
  }

  //
  // Memory attribute is shorter than our descriptor.
  // Shorten current descriptor, update its type, and inseret the new one after it.
  // [DESC1] -> [DESC1*][DESC2]
  //
  if (*CurrentEntryCount == TotalEntryCount) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  DiffPages         = (UINTN) (MemoryMapEntry->NumberOfPages - MemoryAttribte->NumberOfPages);
  CopyMem (
    NewMemoryMapEntry,
    MemoryMapEntry,
    DescriptorSize * (*CurrentEntryCount - *CurrentEntryIndex)
    );
  MemoryMapEntry->Type              = MemoryAttribte->Type;
  MemoryMapEntry->NumberOfPages     = DiffPages;
  NewMemoryMapEntry->PhysicalStart += EFI_PAGES_TO_SIZE (DiffPages);
  NewMemoryMapEntry->NumberOfPages -= DiffPages;

  //
  // Current processed entry is now the one we need to process.
  //
  ++(*CurrentEntryIndex);
  ++(*CurrentEntryCount);

  *RetMemoryMapEntry = NewMemoryMapEntry;

  return EFI_SUCCESS;
}

EFI_STATUS
OcSplitMemoryMapByAttributes (
  IN     UINTN                  OriginalMemoryMapSize,
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  )
{
  EFI_STATUS                         Status;
  CONST EFI_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR              *MemoryAttributesEntry;
  EFI_MEMORY_DESCRIPTOR              *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR              *LastAttributeEntry;
  UINTN                              LastAttributeIndex;
  UINTN                              Index;
  UINTN                              Index2;
  UINTN                              CurrentEntryCount;
  UINTN                              TotalEntryCount;
  UINTN                              AttributeCount;
  BOOLEAN                            CanSplit;
  BOOLEAN                            InDescAttrs;

  ASSERT (OriginalMemoryMapSize >= *MemoryMapSize);

  MemoryAttributesTable = OcGetMemoryAttributes (&MemoryAttributesEntry);
  if (MemoryAttributesTable == NULL) {
    return EFI_UNSUPPORTED;
  }

  LastAttributeEntry = MemoryAttributesEntry;
  LastAttributeIndex = 0;
  MemoryMapEntry     = MemoryMap;
  CurrentEntryCount  = *MemoryMapSize / DescriptorSize;
  TotalEntryCount    = OriginalMemoryMapSize / DescriptorSize;
  AttributeCount     = MemoryAttributesTable->NumberOfEntries;

  //
  // We assume that the memory map and attribute table are sorted.
  // If this is not the case we can add a sort call here.
  //
  for (Index = 0; Index < CurrentEntryCount; ++Index) {
    //
    // Split entry by as many attributes as possible.
    //
    CanSplit = TRUE;
    while ((MemoryMapEntry->Type == EfiRuntimeServicesCode
      || MemoryMapEntry->Type == EfiRuntimeServicesData) && CanSplit) {
      //
      // Find corresponding memory attribute.
      //
      InDescAttrs = FALSE;
      MemoryAttributesEntry = LastAttributeEntry;
      for (Index2 = LastAttributeIndex; Index2 < AttributeCount; ++Index2) {
        if (MemoryAttributesEntry->Type == EfiRuntimeServicesCode
          || MemoryAttributesEntry->Type == EfiRuntimeServicesData) {
          //
          // UEFI spec says attribute entries are fully within memory map entries.
          // Find first one of a different type.
          //
          if (AREA_WITHIN_DESCRIPTOR (
            MemoryMapEntry,
            MemoryAttributesEntry->PhysicalStart,
            EFI_PAGES_TO_SIZE (MemoryAttributesEntry->NumberOfPages))) {
            //
            // We are within descriptor attribute sequence.
            //
            InDescAttrs = TRUE;
            //
            // No need to process the attribute of the same type.
            // FIXME: Do we need this to inspect attributes?
            //
            if (MemoryAttributesEntry->Type != MemoryMapEntry->Type) {
              //
              // Start with the next attribute on the second iteration.
              //
              LastAttributeEntry = NEXT_MEMORY_DESCRIPTOR (
                MemoryAttributesEntry,
                MemoryAttributesTable->DescriptorSize
                );
              LastAttributeIndex = Index2 + 1;
              break;
            }
          } else if (InDescAttrs) {
            //
            // Reached the end of descriptor attribute sequence, abort.
            //
            InDescAttrs = FALSE;
            break;
          }
        }

        MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR (
          MemoryAttributesEntry,
          MemoryAttributesTable->DescriptorSize
          );
      }

      if (Index2 < AttributeCount && InDescAttrs) {
        //
        // Split current memory map entry.
        //
        Status = OcSplitMemoryEntryByAttribute (
          &MemoryMapEntry,
          &Index,
          &CurrentEntryCount,
          TotalEntryCount,
          MemoryAttributesEntry,
          DescriptorSize
          );
        if (EFI_ERROR (Status)) {
          *MemoryMapSize = CurrentEntryCount * DescriptorSize;
          return Status;
        }
      } else {
        //
        // Did not find a suitable attribute or processed all the attributes.
        //
        CanSplit = FALSE;
      }
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (
      MemoryMapEntry,
      DescriptorSize
      );
  }

  *MemoryMapSize = CurrentEntryCount * DescriptorSize;
  return EFI_SUCCESS;
}

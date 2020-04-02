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
GetCurrentMemoryMap (
  OUT UINTN   *MemoryMapSize,
  OUT UINTN   *DescriptorSize,
  OUT UINTN   *MapKey             OPTIONAL,
  OUT UINT32  *DescriptorVersion  OPTIONAL
  )
{
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  EFI_STATUS              Status;
  UINTN                   MapKeyValue;
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

  //
  // Apple uses 1024 as constant, however it will grow by at least
  // DescriptorSize.
  //
  Result = OcOverflowAddUN (
    *MemoryMapSize,
    MAX (*DescriptorSize, 1024),
    MemoryMapSize
    );

  if (Result) {
    return NULL;
  }

  MemoryMap = AllocatePool (*MemoryMapSize);
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

  MemoryMap = GetCurrentMemoryMap (&MemoryMapSize, &DescriptorSize, NULL, NULL);
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
  CONST EFI_MEMORY_DESCRIPTOR       *MemoryAttributesEntry;

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiMemoryAttributesTableGuid)) {
      MemoryAttributesTable = (CONST EFI_MEMORY_ATTRIBUTES_TABLE *) gST->ConfigurationTable[Index].VendorTable;

      DEBUG ((DEBUG_INFO, "OCMM: MemoryAttributesTable:\n"));
      DEBUG ((DEBUG_INFO, "OCMM:   Version              - 0x%08x\n", MemoryAttributesTable->Version));
      DEBUG ((DEBUG_INFO, "OCMM:   NumberOfEntries      - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
      DEBUG ((DEBUG_INFO, "OCMM:   DescriptorSize       - 0x%08x\n", MemoryAttributesTable->DescriptorSize));

      MemoryAttributesEntry = (CONST EFI_MEMORY_DESCRIPTOR *) (MemoryAttributesTable + 1);

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

      return;
    }
  }

  DEBUG ((DEBUG_INFO, "OCMM: MemoryAttributesTable is not present!\n"));
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

EFI_STATUS
OcUpdateAttributes (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN EFI_MEMORY_TYPE       Type,
  IN UINT64                SetAttributes,
  IN UINT64                DropAttributes
  )
{
  UINTN                             Index;
  CONST EFI_MEMORY_ATTRIBUTES_TABLE *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR             *MemoryAttributesEntry;

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiMemoryAttributesTableGuid)) {
      MemoryAttributesTable = (CONST EFI_MEMORY_ATTRIBUTES_TABLE *) gST->ConfigurationTable[Index].VendorTable;
      MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *) (MemoryAttributesTable + 1);
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
  }

  return EFI_UNSUPPORTED;
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

  DescriptorCount = 0;

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiMemoryAttributesTableGuid)) {
      MemoryAttributesTable = (CONST EFI_MEMORY_ATTRIBUTES_TABLE *) gST->ConfigurationTable[Index].VendorTable;
      MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *) (MemoryAttributesTable + 1);
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
    }
  }

  return DescriptorCount;
}

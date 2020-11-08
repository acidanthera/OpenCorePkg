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

EFI_STATUS
OcAllocatePagesFromTop (
  IN     EFI_MEMORY_TYPE         MemoryType,
  IN     UINTN                   Pages,
  IN OUT EFI_PHYSICAL_ADDRESS    *Memory,
  IN     EFI_GET_MEMORY_MAP      GetMemoryMap   OPTIONAL,
  IN     EFI_ALLOCATE_PAGES      AllocatePages  OPTIONAL,
  IN     CHECK_ALLOCATION_RANGE  CheckRange     OPTIONAL
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

  Status = OcGetCurrentMemoryMapAlloc (
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

      Status = (AllocatePages != NULL ? AllocatePages : gBS->AllocatePages) (
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
OcCountRuntimePages (
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
OcCountFreePages (
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

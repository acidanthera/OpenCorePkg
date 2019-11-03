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
          (EFI_PHYSICAL_ADDRESS) *MemoryMap,
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

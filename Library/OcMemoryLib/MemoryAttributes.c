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

/**
  Determine actual memory type from the attribute.

  @param[in]  MemoryAttribute  Attribute to inspect.
**/
STATIC
UINT32
OcRealMemoryType (
  IN EFI_MEMORY_DESCRIPTOR  *MemoryAttribte
  )
{
  ASSERT (MemoryAttribte->Type == EfiRuntimeServicesCode
    || MemoryAttribte->Type == EfiRuntimeServicesData);

  //
  // Use code for write-protected areas.
  //
  if ((MemoryAttribte->Attribute & EFI_MEMORY_RO) != 0) {
    return EfiRuntimeServicesCode;
  }

  //
  // Use data for executable-protected areas.
  //
  if ((MemoryAttribte->Attribute & EFI_MEMORY_XP) != 0) {
    return EfiRuntimeServicesData;
  }

  //
  // Use whatever is set.
  //
  return MemoryAttribte->Type;
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
  IN     EFI_MEMORY_DESCRIPTOR  *MemoryAttribute,
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
  if (MemoryAttribute->PhysicalStart > MemoryMapEntry->PhysicalStart) {
    if (*CurrentEntryCount == TotalEntryCount) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    DiffPages         = (UINTN) EFI_SIZE_TO_PAGES (MemoryAttribute->PhysicalStart - MemoryMapEntry->PhysicalStart);
    CopyMem (
      NewMemoryMapEntry,
      MemoryMapEntry,
      DescriptorSize * (*CurrentEntryCount - *CurrentEntryIndex)
      );
    MemoryMapEntry->NumberOfPages     = DiffPages;
    NewMemoryMapEntry->PhysicalStart  = MemoryAttribute->PhysicalStart;
    NewMemoryMapEntry->NumberOfPages -= DiffPages;

    MemoryMapEntry = NewMemoryMapEntry;

    //
    // Current processed entry is now the one we inserted.
    //
    ++(*CurrentEntryIndex);
    ++(*CurrentEntryCount);
  }

  ASSERT (MemoryAttribute->PhysicalStart == MemoryMapEntry->PhysicalStart);

  //
  // Memory attribute matches our descriptor.
  // Simply update its protection.
  // [DESC1] -> [DESC1*]
  //
  if (MemoryMapEntry->NumberOfPages == MemoryAttribute->NumberOfPages) {
    MemoryMapEntry->Type = OcRealMemoryType (MemoryAttribute);
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
  CopyMem (
    NewMemoryMapEntry,
    MemoryMapEntry,
    DescriptorSize * (*CurrentEntryCount - *CurrentEntryIndex)
    );
  MemoryMapEntry->Type              = OcRealMemoryType (MemoryAttribute);
  MemoryMapEntry->NumberOfPages     = MemoryAttribute->NumberOfPages;
  NewMemoryMapEntry->PhysicalStart += EFI_PAGES_TO_SIZE (MemoryAttribute->NumberOfPages);
  NewMemoryMapEntry->NumberOfPages -= MemoryAttribute->NumberOfPages;

  //
  // Current processed entry is now the one we need to process.
  //
  ++(*CurrentEntryIndex);
  ++(*CurrentEntryCount);

  *RetMemoryMapEntry = NewMemoryMapEntry;

  return EFI_SUCCESS;
}

/**
  Expand attributes table by adding memory map runtime entries into it.
  Requires sorted memory map.

  @param[in,out]  MemoryAttributesTable   Memory attributes table.
  @param[in,out]  MemoryAttributesEntry   Memory attributes descriptor.
  @param[in]      MaxDescriptors          Maximum amount of descriptors in the attributes table.
  @param[in]      MemoryMapDescriptors    Memory map descriptor count.
  @param[in]      MemoryMap               Memory map.
  @param[in]      DescriptorSize          Memory map descriptor size.

  @retval EFI_SUCCESS on success.
  @retval EFI_NOT_FOUND nothing to do.
**/
STATIC
EFI_STATUS
OcExpandAttributesByMap (
  IN OUT EFI_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable,
  IN OUT EFI_MEMORY_DESCRIPTOR        *MemoryAttributesEntry,
  IN     UINTN                        MaxDescriptors,
  IN     UINTN                        MemoryMapDescriptors,
  IN     EFI_MEMORY_DESCRIPTOR        *MemoryMap,
  IN     UINTN                        DescriptorSize
  )
{
  EFI_STATUS            Status;
  UINTN                 MapIndex;
  UINTN                 MatIndex;
  EFI_PHYSICAL_ADDRESS  LastAddress;
  BOOLEAN               DoneWithMat;

  MatIndex = 0;
  Status   = EFI_NOT_FOUND;

  for (MapIndex = 0; MapIndex < MemoryMapDescriptors; ++MapIndex) {
    if (MemoryMap->Type == EfiRuntimeServicesCode
      || MemoryMap->Type == EfiRuntimeServicesData) {

      LastAddress = LAST_DESCRIPTOR_ADDR (MemoryMap);
      DoneWithMat = FALSE;

      while (MatIndex < MemoryAttributesTable->NumberOfEntries && !DoneWithMat) {
        if (MemoryAttributesEntry->PhysicalStart >= MemoryMap->PhysicalStart
          && MemoryAttributesEntry->PhysicalStart <= LastAddress) {
          //
          // We have an attribute for that memory map descriptor, assume it is parsed.
          //
          DoneWithMat = TRUE;
        } else if (LastAddress < MemoryAttributesEntry->PhysicalStart) {
          //
          // We have an attribute past the memory map descriptor, insert the new one here.
          //
          if (MemoryAttributesTable->NumberOfEntries >= MaxDescriptors) {
            return EFI_OUT_OF_RESOURCES;
          }
          //
          // Copy existing attributes to the right.
          //
          CopyMem (
            NEXT_MEMORY_DESCRIPTOR (MemoryAttributesEntry, MemoryAttributesTable->DescriptorSize),
            MemoryAttributesEntry,
            (MemoryAttributesTable->NumberOfEntries - MatIndex) * MemoryAttributesTable->DescriptorSize
            );
          //
          // Write the new attribute.
          //
          MemoryAttributesEntry->Type          = OcRealMemoryType (MemoryMap);
          MemoryAttributesEntry->PhysicalStart = MemoryMap->PhysicalStart;
          MemoryAttributesEntry->VirtualStart  = 0;
          MemoryAttributesEntry->NumberOfPages = MemoryMap->NumberOfPages;
          MemoryAttributesEntry->Attribute     = EFI_MEMORY_RUNTIME;
          if (MemoryAttributesEntry->Type == EfiRuntimeServicesCode) {
            MemoryAttributesEntry->Attribute |= EFI_MEMORY_RO;
          } else {
            MemoryAttributesEntry->Attribute |= EFI_MEMORY_XP;
          }
          //
          // Increase the amount of entries and complete iteration.
          //
          ++MemoryAttributesTable->NumberOfEntries;
          DoneWithMat = TRUE;
          Status = EFI_SUCCESS;
        }

        MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR (
          MemoryAttributesEntry,
          MemoryAttributesTable->DescriptorSize
          );
        ++MatIndex;
      }
    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (
      MemoryMap,
      DescriptorSize
      );
  }

  return Status;
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
OcRebuildAttributes (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN EFI_GET_MEMORY_MAP    GetMemoryMap  OPTIONAL
  )
{
  EFI_STATUS                         Status;
  EFI_MEMORY_ATTRIBUTES_TABLE        *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR              *MemoryAttributesEntry;
  UINTN                              MaxDescriptors;
  UINTN                              MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR              *MemoryMap;
  UINTN                              DescriptorSize;
  UINTN                              MapKey;
  UINT32                             DescriptorVersion;

  MemoryAttributesTable = OcGetMemoryAttributes (&MemoryAttributesEntry);
  if (MemoryAttributesTable == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Some boards create entry duplicates and lose all non-PE entries
  // after loading runtime drivers after EndOfDxe.
  // REF: https://github.com/acidanthera/bugtracker/issues/491#issuecomment-609014334
  //
  MaxDescriptors = MemoryAttributesTable->NumberOfEntries;
  Status = OcDeduplicateDescriptors (
    &MemoryAttributesTable->NumberOfEntries,
    MemoryAttributesEntry,
    MemoryAttributesTable->DescriptorSize
    );
  if (!EFI_ERROR (Status)) {
    //
    // Statically allocate memory for the memory map to avoid allocations.
    //
    STATIC UINT8 mMemoryMap[OC_DEFAULT_MEMORY_MAP_SIZE];

    //
    // Assume effected and add missing entries.
    //
    if (GetMemoryMap == NULL) {
      GetMemoryMap = gBS->GetMemoryMap;
    }

    MemoryMapSize = sizeof (mMemoryMap);
    MemoryMap     = (EFI_MEMORY_DESCRIPTOR *) mMemoryMap;

    Status = GetMemoryMap (
      &MemoryMapSize,
      MemoryMap,
      &MapKey,
      &DescriptorSize,
      &DescriptorVersion
      );

    if (!EFI_ERROR (Status)) {
      OcSortMemoryMap (
        MemoryMapSize,
        MemoryMap,
        DescriptorSize
        );

      OcExpandAttributesByMap (
        MemoryAttributesTable,
        MemoryAttributesEntry,
        MaxDescriptors,
        MemoryMapSize / DescriptorSize,
        MemoryMap,
        DescriptorSize
        );
    } else {
      //
      // TODO: Scream in fear here? We cannot log, but for a fatal error it is "fine".
      //
    }
  }

  //
  // Some firmwares do not update MAT after loading runtime drivers after EndOfDxe.
  // Since the memory used to allocate runtime driver resides in BINs, MAT has whatever
  // permissions designated for unused memory. Mark unused memory containing our driver
  // as executable here.
  // REF: https://github.com/acidanthera/bugtracker/issues/491#issuecomment-606835337
  //
  Status = OcUpdateDescriptors (
    MemoryAttributesTable->NumberOfEntries * MemoryAttributesTable->DescriptorSize,
    MemoryAttributesEntry,
    MemoryAttributesTable->DescriptorSize,
    Address,
    EfiRuntimeServicesCode,
    EFI_MEMORY_RO,
    EFI_MEMORY_XP
    );

  return Status;
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

EFI_STATUS
OcSplitMemoryMapByAttributes (
  IN     UINTN                  MaxMemoryMapSize,
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

  ASSERT (MaxMemoryMapSize >= *MemoryMapSize);

  MemoryAttributesTable = OcGetMemoryAttributes (&MemoryAttributesEntry);
  if (MemoryAttributesTable == NULL) {
    return EFI_UNSUPPORTED;
  }

  LastAttributeEntry = MemoryAttributesEntry;
  LastAttributeIndex = 0;
  MemoryMapEntry     = MemoryMap;
  CurrentEntryCount  = *MemoryMapSize / DescriptorSize;
  TotalEntryCount    = MaxMemoryMapSize / DescriptorSize;
  AttributeCount     = MemoryAttributesTable->NumberOfEntries;

  //
  // We assume that the memory map and attribute table are sorted.
  //
  Index = 0;
  while (Index < CurrentEntryCount) {
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
            //
            if (OcRealMemoryType (MemoryAttributesEntry) != MemoryMapEntry->Type) {
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
        continue;
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
    ++Index;
  }

  *MemoryMapSize = CurrentEntryCount * DescriptorSize;
  return EFI_SUCCESS;
}

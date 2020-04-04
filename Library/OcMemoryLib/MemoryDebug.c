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

STATIC CONST CHAR8 *mEfiMemoryTypeDesc[EfiMaxMemoryType] = {
  "Reserved ",
  "LDR Code ",
  "LDR Data ",
  "BS Code  ",
  "BS Data  ",
  "RT Code  ",
  "RT Data  ",
  "Available",
  "Unusable ",
  "ACPI RECL",
  "ACPI NVS ",
  "MemMapIO ",
  "MemPortIO",
  "PAL Code ",
  "Persist  "
};

STATIC
VOID
OcPrintMemoryDescritptor (
  IN EFI_MEMORY_DESCRIPTOR  *Desc
  )
{
  CONST CHAR8  *Type;
  CONST CHAR8  *SizeType;
  UINT64       SizeValue;

  if (Desc->Type < ARRAY_SIZE (mEfiMemoryTypeDesc)) {
    Type = mEfiMemoryTypeDesc[Desc->Type];
  } else {
    Type = "Invalid  ";
  }

  SizeValue = EFI_PAGES_TO_SIZE (Desc->NumberOfPages);
  if (SizeValue >= BASE_1MB) {
    SizeValue /= BASE_1MB;
    SizeType   = "MB";
  } else {
    SizeValue /= BASE_1KB;
    SizeType   = "KB";
  }

  DEBUG ((
    DEBUG_INFO,
    "OCMM: %a [%a|%a|%a|%a|%a|%a|%a|%a|%a|%a|%a|%a|%a|%a] 0x%016LX-0x%016LX -> 0x%016X (%Lu %a)\n",
    Type,
    (Desc->Attribute & EFI_MEMORY_RUNTIME)        != 0 ? "RUN" : "   ",
    (Desc->Attribute & EFI_MEMORY_CPU_CRYPTO)     != 0 ? "CRY" : "   ",
    (Desc->Attribute & EFI_MEMORY_SP)             != 0 ? "SP"  : "  ",
    (Desc->Attribute & EFI_MEMORY_RO)             != 0 ? "RO"  : "  ",
    (Desc->Attribute & EFI_MEMORY_MORE_RELIABLE)  != 0 ? "MR"  : "  ",
    (Desc->Attribute & EFI_MEMORY_NV)             != 0 ? "NV"  : "  ",
    (Desc->Attribute & EFI_MEMORY_XP)             != 0 ? "XP"  : "  ",
    (Desc->Attribute & EFI_MEMORY_RP)             != 0 ? "RP"  : "  ",
    (Desc->Attribute & EFI_MEMORY_WP)             != 0 ? "WP"  : "  ",
    (Desc->Attribute & EFI_MEMORY_UCE)            != 0 ? "UCE" : "   ",
    (Desc->Attribute & EFI_MEMORY_WB)             != 0 ? "WB"  : "  ",
    (Desc->Attribute & EFI_MEMORY_WT)             != 0 ? "WT"  : "  ",
    (Desc->Attribute & EFI_MEMORY_WC)             != 0 ? "WC"  : "  ",
    (Desc->Attribute & EFI_MEMORY_UC)             != 0 ? "UC"  : "  ",
    Desc->PhysicalStart,
    Desc->PhysicalStart + (EFI_PAGES_TO_SIZE (Desc->NumberOfPages) - 1),
    Desc->VirtualStart,
    SizeValue,
    SizeType
    ));
}

VOID
OcPrintMemoryAttributesTable (
  VOID
  )
{
  UINTN                             Index;
  UINTN                             RealSize;
  CONST EFI_MEMORY_ATTRIBUTES_TABLE *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR             *MemoryAttributesEntry;

  MemoryAttributesTable = OcGetMemoryAttributes (NULL);
  if (MemoryAttributesTable == NULL) {
    DEBUG ((DEBUG_INFO, "OCMM: MemoryAttributesTable is not present!\n"));
    return;
  }

  //
  // Printing may reallocate, so we create a copy of the memory attributes.
  //
  STATIC UINT8 mMemoryAttributesTable[OC_DEFAULT_MEMORY_MAP_SIZE];
  RealSize = (UINTN) (sizeof (EFI_MEMORY_ATTRIBUTES_TABLE)
    + MemoryAttributesTable->NumberOfEntries * MemoryAttributesTable->DescriptorSize);

  if (RealSize > sizeof(mMemoryAttributesTable)) {
    DEBUG ((DEBUG_INFO, "OCMM: MemoryAttributesTable has too large size %u!\n", (UINT32) RealSize));
    return;
  }

  CopyMem (mMemoryAttributesTable, MemoryAttributesTable, RealSize);

  MemoryAttributesTable = (EFI_MEMORY_ATTRIBUTES_TABLE *) mMemoryAttributesTable;
  MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *) (MemoryAttributesTable + 1);

  DEBUG ((DEBUG_INFO, "OCMM: MemoryAttributesTable:\n"));
  DEBUG ((DEBUG_INFO, "OCMM:   Version              - 0x%08x\n", MemoryAttributesTable->Version));
  DEBUG ((DEBUG_INFO, "OCMM:   NumberOfEntries      - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
  DEBUG ((DEBUG_INFO, "OCMM:   DescriptorSize       - 0x%08x\n", MemoryAttributesTable->DescriptorSize));

  for (Index = 0; Index < MemoryAttributesTable->NumberOfEntries; ++Index) {
    OcPrintMemoryDescritptor (MemoryAttributesEntry);
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
    OcPrintMemoryDescritptor (MemoryMap);
    MemoryMap = NEXT_MEMORY_DESCRIPTOR (
      MemoryMap,
      DescriptorSize
      );
  }
}

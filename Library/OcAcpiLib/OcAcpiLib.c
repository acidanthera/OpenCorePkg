/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>

#include <IndustryStandard/AcpiAml.h>
#include <IndustryStandard/Acpi.h>

#include <Guid/Acpi.h>

#include <Library/OcAcpiLib.h>


/**
  Find RSD_PTR Table In Legacy Area

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
STATIC
EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindLegacyRsdp (
  VOID
  )
{
  EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp;

  UINTN                                        Address;
  UINTN                                        Index;

  //
  // First Search 0x0E0000 - 0x0FFFFF for RSD_PTR
  //

  Rsdp = NULL;

  for (Address = 0x0E0000; Address < 0x0FFFFF; Address += 16) {
    if (*(UINT64 *) Address == EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
      Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) Address;
    }
  }

  //
  // Then Search EBDA 0x40E - 0x800
  //

  if (Rsdp == NULL) {
    Address = ((UINTN)(*(UINT16 *) 0x040E) << 4U);

    for (Index = 0; Index < 0x0400; Index += 16) {
      if (*(UINT64 *) (Address + Index) == EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
        Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) Address;
      }
    }
  }

  return Rsdp;
}

/**
  Find RSD_PTR Table From System Configuration Tables

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
STATIC
EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindRsdp (
  VOID
  )
{
  EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  UINTN                                         Index;

  Rsdp    = NULL;

  //
  // Find ACPI table RSD_PTR from system table
  //

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    //
    // Prefer ACPI 2.0
    //
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiAcpi20TableGuid)) {
      Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      DEBUG ((DEBUG_VERBOSE, "OCA: Found ACPI 2.0 RSDP table %p\n", Rsdp));
      break;
    }

    //
    // Otherwise use ACPI 1.0, but do search for ACPI 2.0.
    //
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiAcpi10TableGuid)) {
      Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      DEBUG ((DEBUG_VERBOSE, "OCA: Found ACPI 1.0 RSDP table %p\n", Rsdp));
    }
  }

  //
  // Try to use legacy search as a last resort.
  //
  if (Rsdp == NULL) {
    Rsdp = AcpiFindLegacyRsdp ();
    if (Rsdp != NULL) {
      DEBUG ((DEBUG_VERBOSE, "OCA: Found ACPI legacy RSDP table %p\n", Rsdp));
    }
  }

  if (Rsdp == NULL) {
    DEBUG ((DEBUG_WARN, "OCA: Failed to find ACPI RSDP table\n"));
  }

  return Rsdp;
}

/**
  Extract and verify ACPI signature from common header.

  @param Common      ACPI common header.

  @return Signature.
**/
STATIC
UINT32
AcpiReadSignature (
  IN  CONST EFI_ACPI_COMMON_HEADER  *Common
  )
{
  UINT32   Result;
  CHAR8    *Walker;
  UINT32   Index;

  Result = Common->Signature;

  Walker = (CHAR8 *) &Result;
  for (Index = 0; Index < sizeof (Common->Signature); Index++) {
    if (!IsAsciiPrint (Walker[Index])) {
      Walker[Index] = '?';
    }
  }

  return Result;
}

/**
  Extract and verify ACPI OemTableId from common header.

  @param Common      ACPI common header.

  @return OemTableId or 0.
**/
STATIC
UINT64
AcpiReadOemTableId (
  IN  CONST EFI_ACPI_COMMON_HEADER  *Common
  )
{
  if (Common->Length <= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
    return 0;
  }

  return ((EFI_ACPI_DESCRIPTION_HEADER *) Common)->OemTableId;
}

/**
  Extract and verify ACPI name from data.

  @param Data        Data of at least OC_ACPI_NAME_SIZE+1 bytes to read name from.
  @param Name        Name buffer of at least OC_ACPI_NAME_SIZE+1 bytes.
  @param NameOffset  Name offset from original data (1 if '\\' and 0 otherwise).

  @return TRUE for valid names.
**/
STATIC
BOOLEAN
AcpiReadName (
  IN  CONST UINT8  *Data,
  OUT CHAR8        *Name,
  OUT UINT32       *NameOffset OPTIONAL
  )
{
  UINT32  Index;
  UINT32  Off;

  //
  // Skip \ in \NAME.
  //
  Off = Data[0] == '\\' ? 1 : 0;

  for (Index = Off; Index < Off + OC_ACPI_NAME_SIZE; ++Index) {
    if (Data[Index] < '/'
      || ((Data[Index] > '9') && (Data[Index] < 'A'))
      || ((Data[Index] > 'Z') && (Data[Index] != '_'))) {
      return FALSE;
    }

    Name[Index - Off] = Data[Index];
  }

  Name[OC_ACPI_NAME_SIZE] = 0;

  if (NameOffset != NULL) {
    *NameOffset = Off;
  }

  return TRUE;
}

/**
  Find ACPI name declaration in data.

  @param Data        ACPI table data.
  @param Length      ACPI table data length.
  @param Name        ACPI name of at least OC_ACPI_NAME_SIZE bytes.

  @return offset > 0 for valid names.
**/
STATIC
UINT32
AcpiFindName (
  IN CONST UINT8  *Data,
  IN UINT32       Length,
  IN CONST CHAR8  *Name
  )
{
  UINT32  Index;

  //
  // Lookup from offset 1 and after.
  //
  if (Length < OC_ACPI_NAME_SIZE + 1) {
    return 0;
  }

  for (Index = 0; Index < Length - OC_ACPI_NAME_SIZE; ++Index) {
    if (Data[Index] == AML_NAME_OP
      && Data[Index+1] == Name[0]
      && Data[Index+2] == Name[1]
      && Data[Index+3] == Name[2]
      && Data[Index+4] == Name[3]) {
      return Index+1;
    }
  }

  return 0;
}

STATIC
VOID
AcpiRefreshTableChecksum (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  UINT32      TablePrintSignature;

  TablePrintSignature = AcpiReadSignature ((EFI_ACPI_COMMON_HEADER *) Table);
  Table->Checksum     = 0;
  Table->Checksum     = CalculateCheckSum8 ((UINT8 *) Table, Table->Length);

  DEBUG ((
    DEBUG_INFO,
    "OCA: Refreshed %.4a checksum to %02x\n",
    (CHAR8 *) &TablePrintSignature,
    Table->Checksum
    ));
}

STATIC
BOOLEAN
AcpiIsTableWritable (
  IN     EFI_ACPI_COMMON_HEADER  *Table
  )
{
  UINT32  LengthOriginal;

  LengthOriginal = Table->Length;
  (*(volatile UINT32 *)&Table->Length)++;

  if (LengthOriginal == *(volatile UINT32 *)&Table->Length) {
    return FALSE;
  }

  Table->Length = LengthOriginal;
  return TRUE;
}

STATIC
EFI_STATUS
AcpiAllocateCopyTable (
  IN     EFI_ACPI_COMMON_HEADER  *Table,
  IN     UINT32                  NewSize OPTIONAL,
     OUT EFI_ACPI_COMMON_HEADER  **NewTable
  )
{
  EFI_STATUS              Status;
  UINT32                  TablePrintSignature;
  EFI_PHYSICAL_ADDRESS    NewTableAddress;

  if (NewSize < Table->Length) {
    NewSize = Table->Length;
  }

  TablePrintSignature = AcpiReadSignature (Table);

  NewTableAddress = BASE_4GB - 1;
  Status = gBS->AllocatePages (
    AllocateMaxAddress,
    EfiACPIMemoryNVS,
    EFI_SIZE_TO_PAGES (NewSize),
    &NewTableAddress
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "OCA: Failed to allocate %u bytes for table %.4a\n",
      NewSize,
      (CHAR8 *) &TablePrintSignature
      ));
    return Status;
  }

  *NewTable = (EFI_ACPI_COMMON_HEADER *)(UINTN) NewTableAddress;

  DEBUG ((
    DEBUG_INFO,
    "OCA: Allocated new table %.4a at %p\n",
    (CHAR8 *) &TablePrintSignature,
    *NewTable
    ));

  CopyMem (*NewTable, Table, Table->Length);
  ZeroMem (((UINT8 *) *NewTable) + Table->Length, EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (NewSize)) - Table->Length);
  (*NewTable)->Length = NewSize;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
AcpiAllocateCopyFadt (
  IN OUT OC_ACPI_CONTEXT             *Context,
  IN     UINT32                      NewSize
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_COMMON_HEADER  *NewFadt;
  UINT32                  Index;

  Status = AcpiAllocateCopyTable (
    (EFI_ACPI_COMMON_HEADER *) Context->Fadt,
    NewSize,
    &NewFadt
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    //
    // There is a possibility for multiple FADT tables.
    //
    if (Context->Tables[Index]->Signature == EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      Context->Tables[Index] = NewFadt;
    }
  }
  Context->Fadt = (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE *) NewFadt;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
AcpiAllocateCopyDsdt (
  IN OUT OC_ACPI_CONTEXT             *Context,
  IN     EFI_ACPI_DESCRIPTION_HEADER *NewDsdt     OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_COMMON_HEADER  *AllocatedDsdt;

  Status = AcpiAllocateCopyTable (
    (EFI_ACPI_COMMON_HEADER *) (NewDsdt != NULL ? NewDsdt : Context->Dsdt),
    0,
    &AllocatedDsdt
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Reallocate FADT if needed.
  //
  if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Fadt)) {
    Status = AcpiAllocateCopyFadt (Context, 0);
    if (EFI_ERROR (Status)) {
      FreePool (AllocatedDsdt);
      return Status;
    }
  }

  if (Context->Fadt->Header.Length >= OFFSET_OF (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE, XDsdt) + sizeof (Context->Fadt->XDsdt)) {
    Context->Fadt->XDsdt = (UINT64)(UINTN) AllocatedDsdt;
  }
  Context->Fadt->Dsdt = (UINT32)(UINTN) AllocatedDsdt;
  AcpiRefreshTableChecksum ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Fadt);

  Context->Dsdt = (EFI_ACPI_DESCRIPTION_HEADER *) AllocatedDsdt;

  return EFI_SUCCESS;
}

/**
  Load ACPI table regions.

  @param Context      ACPI library context.
  @param Table        ACPI table.

  @return EFI_SUCCESS unless memory allocation failure.
**/
STATIC
EFI_STATUS
AcpiLoadTableRegions (
  IN OUT OC_ACPI_CONTEXT         *Context,
  IN     EFI_ACPI_COMMON_HEADER  *Table
  )
{
  UINT32          Index;
  UINT32          Index2;
  UINT32          NameOffset;
  UINT8           *Buffer;
  UINT32          BufferLen;
  CHAR8           Name[OC_ACPI_NAME_SIZE+1];
  CHAR8           NameAddr[OC_ACPI_NAME_SIZE+1];
  OC_ACPI_REGION  *NewRegions;
  UINT32          Address;

  Buffer    = (UINT8 *) Table;
  BufferLen = Table->Length;

  if (BufferLen < sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
    return EFI_SUCCESS;
  }

  for (Index = sizeof (EFI_ACPI_DESCRIPTION_HEADER); Index < BufferLen - 0xF; ++Index) {
    if (Buffer[Index] == AML_EXT_OP
      && Buffer[Index+1] == AML_EXT_REGION_OP
      && AcpiReadName (&Buffer[Index+2], &Name[0], &NameOffset)
      && Buffer[Index+OC_ACPI_NAME_SIZE+2+NameOffset] == EFI_ACPI_6_2_SYSTEM_MEMORY) {
      //
      // This is SystemMemory region. Try to save it.
      //
      Address = 0;
      if (Buffer[Index+OC_ACPI_NAME_SIZE+3+NameOffset] == AML_DWORD_PREFIX) {
        CopyMem (&Address, &Buffer[Index+OC_ACPI_NAME_SIZE+4+NameOffset], sizeof (UINT32));
      } else if (Buffer[Index+OC_ACPI_NAME_SIZE+3+NameOffset] == AML_WORD_PREFIX) {
        CopyMem (&Address, &Buffer[Index+OC_ACPI_NAME_SIZE+4+NameOffset], sizeof (UINT16));
      } else if (AcpiReadName (&Buffer[Index+OC_ACPI_NAME_SIZE+3+NameOffset], &NameAddr[0], NULL)) {
        Index2 = AcpiFindName (Buffer, BufferLen, &NameAddr[0]);
        if (Index2 > 0 && Index2 < BufferLen - 0xF) {
          if (Buffer[Index2+OC_ACPI_NAME_SIZE] == AML_DWORD_PREFIX) {
            CopyMem (&Address, &Buffer[Index2+OC_ACPI_NAME_SIZE+1], sizeof (UINT32));
          } else if (Buffer[Index2+OC_ACPI_NAME_SIZE] == AML_WORD_PREFIX) {
            CopyMem (&Address, &Buffer[Index2+OC_ACPI_NAME_SIZE+1], sizeof (UINT16));
          }
        }
      }

      for (Index2 = 0; Index2 < Context->NumberOfRegions; ++Index2) {
        if (Context->Regions[Index2].Address == Address) {
          Address = 0;
          break;
        }
      }

      if (Address != 0) {
        if (Context->AllocatedRegions == Context->NumberOfRegions) {
          NewRegions = AllocatePool ((Context->AllocatedRegions + 2) * sizeof (Context->Regions[0]));
          if (NewRegions == NULL) {
            DEBUG ((DEBUG_WARN, "OCA: Failed to allocate memory for %u regions\n", Context->AllocatedRegions+2));
            return EFI_OUT_OF_RESOURCES;
          }
          CopyMem (NewRegions, Context->Regions, Context->NumberOfRegions * sizeof (Context->Regions[0]));
          FreePool (Context->Regions);

          Context->Regions = NewRegions;
          Context->AllocatedRegions += 2;
        }

        DEBUG ((DEBUG_INFO, "OCA: Found OperationRegion %a at %08X\n", Name, Address));
        Context->Regions[Context->NumberOfRegions].Address = Address;
        CopyMem (&Context->Regions[Context->NumberOfRegions].Name[0], &Name[0], sizeof (Name));
        ++Context->NumberOfRegions;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Relocate ACPI table regions.

  @param Context      ACPI library context.
  @param Table        ACPI table.
**/
STATIC
VOID
AcpiRelocateTableRegions (
  IN OUT OC_ACPI_CONTEXT         *Context,
  IN     EFI_ACPI_COMMON_HEADER  *Table
  )
{
  UINT32          Index;
  UINT32          Index2;
  UINT32          RegionIndex;
  UINT32          NameOffset;
  UINT8           *Buffer;
  UINT32          BufferLen;
  CHAR8           Name[OC_ACPI_NAME_SIZE+1];
  CHAR8           NameAddr[OC_ACPI_NAME_SIZE+1];
  UINT32          OldAddress;
  BOOLEAN         Modified;

  Buffer    = (UINT8 *) Table;
  BufferLen = Table->Length;
  Modified  = FALSE;

  if (BufferLen < sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
    return;
  }

  for (Index = sizeof (EFI_ACPI_DESCRIPTION_HEADER); Index < BufferLen - 0xF; ++Index) {
    if (Buffer[Index] == AML_EXT_OP
      && Buffer[Index+1] == AML_EXT_REGION_OP
      && AcpiReadName (&Buffer[Index+2], &Name[0], &NameOffset)
      && Buffer[Index+OC_ACPI_NAME_SIZE+2+NameOffset] == EFI_ACPI_6_2_SYSTEM_MEMORY) {
      //
      // This is region. Compare to current BIOS tables and relocate.
      //
      for (RegionIndex = 0; RegionIndex < Context->NumberOfRegions; ++RegionIndex) {
        if (AsciiStrCmp (Context->Regions[RegionIndex].Name, Name) == 0) {
          OldAddress = 0;
          if (Buffer[Index+OC_ACPI_NAME_SIZE+3+NameOffset] == AML_DWORD_PREFIX) {
            CopyMem (&OldAddress, &Buffer[Index+OC_ACPI_NAME_SIZE+4+NameOffset], sizeof (UINT32));
            CopyMem (&Buffer[Index+OC_ACPI_NAME_SIZE+4+NameOffset], &Context->Regions[RegionIndex].Address, sizeof (UINT32));
            Modified = TRUE;
          } else if (Buffer[Index+OC_ACPI_NAME_SIZE+3+NameOffset] == AML_WORD_PREFIX) {
            CopyMem (&OldAddress, &Buffer[Index+OC_ACPI_NAME_SIZE+4+NameOffset], sizeof (UINT16));
            CopyMem (&Buffer[Index+OC_ACPI_NAME_SIZE+4+NameOffset], &Context->Regions[RegionIndex].Address, sizeof (UINT16));
            Modified = TRUE;
          } else if (AcpiReadName (&Buffer[Index+OC_ACPI_NAME_SIZE+3+NameOffset], &NameAddr[0], NULL)) {
            Index2 = AcpiFindName (Buffer, BufferLen, &NameAddr[0]);
            if (Index2 > 0 && Index2 < BufferLen - 0xF) {
              if (Buffer[Index2+OC_ACPI_NAME_SIZE] == AML_DWORD_PREFIX) {
                CopyMem (&OldAddress, &Buffer[Index2+OC_ACPI_NAME_SIZE+1], sizeof (UINT32));
                CopyMem (&Buffer[Index2+OC_ACPI_NAME_SIZE+1], &Context->Regions[RegionIndex].Address, sizeof (UINT32));
                Modified = TRUE;
              } else if (Buffer[Index2+OC_ACPI_NAME_SIZE] == AML_WORD_PREFIX) {
                CopyMem (&OldAddress, &Buffer[Index2+OC_ACPI_NAME_SIZE+1], sizeof (UINT16));
                CopyMem (&Buffer[Index2+OC_ACPI_NAME_SIZE+1], &Context->Regions[RegionIndex].Address, sizeof (UINT16));
                Modified = TRUE;
              }
            }
          }

          if (Modified && OldAddress != Context->Regions[RegionIndex].Address) {
            DEBUG ((
              DEBUG_INFO,
              "OCA: Region %a address relocated from %08X to %08X\n",
              Context->Regions[RegionIndex].Name,
              OldAddress,
              Context->Regions[RegionIndex].Address
              ));
          }

          break;
        }
      }
    }
  }

  //
  // Update checksum
  //
  if (Modified) {
    ((EFI_ACPI_DESCRIPTION_HEADER *)Table)->Checksum = 0;
    ((EFI_ACPI_DESCRIPTION_HEADER *)Table)->Checksum = CalculateCheckSum8 (
      (UINT8 *) Table,
      Table->Length
      );
  }
}

/**
  Cleanup ACPI table from unprintable symbols.
  Reference: https://alextjam.es/debugging-appleacpiplatform/.

  @param Table        ACPI table.
**/
STATIC
BOOLEAN
AcpiNormalizeTableHeaders (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  BOOLEAN  Modified;
  CHAR8    *Walker;
  UINT32   Index;

  if (Table->Length < sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
    return FALSE;
  }

  Modified = FALSE;

  Walker = (CHAR8 *) &Table->Signature;
  for (Index = 0; Index < sizeof (Table->Signature); ++Index) {
    if (!IsAsciiPrint (Walker[Index])) {
      Walker[Index] = '?';
      Modified = TRUE;
    }
  }

  Walker = (CHAR8 *) &Table->OemId;
  for (Index = 0; Index < sizeof (Table->OemId); ++Index) {
    if (!IsAsciiPrint (Walker[Index])) {
      Walker[Index] = '?';
      Modified = TRUE;
    }
  }

  Walker = (CHAR8 *) &Table->OemTableId;
  for (Index = 0; Index < sizeof (Table->OemTableId); ++Index) {
    if (!IsAsciiPrint (Walker[Index])) {
      Walker[Index] = '?';
      Modified = TRUE;
    }
  }

  Walker = (CHAR8 *) &Table->CreatorId;
  for (Index = 0; Index < sizeof (Table->CreatorId); ++Index) {
    if (!IsAsciiPrint (Walker[Index])) {
      Walker[Index] = '?';
      Modified = TRUE;
    }
  }

  if (Modified) {
    Table->Checksum = 0;
    Table->Checksum = CalculateCheckSum8 (
      (UINT8 *) Table,
      Table->Length
      );
  }

  return Modified;
}

/**
  Cleanup RSDP table from unprintable symbols.
  Reference: https://alextjam.es/debugging-appleacpiplatform/.

  @param Rsdp        RSDP table.
  @param HasXsdt     RSDP has XSDT and is extended.
**/
STATIC
BOOLEAN
AcpiNormalizeRsdp (
  IN EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp,
  IN BOOLEAN                                       HasXsdt
  )
{
  BOOLEAN  Modified;
  CHAR8    *Walker;
  UINT32   Index;

  Modified = FALSE;

  Walker = (CHAR8 *) &Rsdp->OemId;
  for (Index = 0; Index < sizeof (Rsdp->OemId); ++Index) {
    if (!IsAsciiPrint (Walker[Index])) {
      Walker[Index] = '?';
      Modified = TRUE;
    }
  }

  if (Modified) {
    //
    // Checksum is to be the first 0-19 bytes of RSDP.
    // ExtendedChecksum is the entire table, only if newer than ACPI 1.0.
    //
    Rsdp->Checksum = 0;
    Rsdp->Checksum = CalculateCheckSum8 (
      (UINT8 *) Rsdp, 20
      );

    if (HasXsdt) {
      Rsdp->ExtendedChecksum = 0;
      Rsdp->ExtendedChecksum = CalculateCheckSum8 (
        (UINT8 *) Rsdp, Rsdp->Length
        );
    }
  }

  return Modified;
}

EFI_STATUS
AcpiInitContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  UINT32  Index;
  UINT32  DstIndex;
  UINT32  TablePrintSignature;

  ZeroMem (Context, sizeof (*Context));

  Context->Rsdp = AcpiFindRsdp ();

  if (Context->Rsdp == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Support RSDT on ACPI 1.0 and newer.
  //
  Context->Rsdt = (OC_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_TABLE *)(UINTN) Context->Rsdp->RsdtAddress;
  DEBUG ((DEBUG_VERBOSE, "OCA: Found ACPI RSDT table %p", Context->Rsdt));

  //
  // ACPI 2.0 and newer have XSDT as well.
  //
  if (Context->Rsdp->Revision > 0) {
    Context->Xsdt = (OC_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE *)(UINTN) Context->Rsdp->XsdtAddress;
    DEBUG ((DEBUG_VERBOSE, "OCA: Found ACPI XSDT table %p", Context->Xsdt));
  }

  if (Context->Rsdt == NULL && Context->Xsdt == NULL) {
    DEBUG ((DEBUG_WARN, "OCA: Failed to find ACPI RSDT or XSDT tables\n"));
    return EFI_NOT_FOUND;
  }

  if (Context->Xsdt != NULL) {
    Context->NumberOfTables = (Context->Xsdt->Header.Length - sizeof (Context->Xsdt->Header))
      / sizeof (Context->Xsdt->Tables[0]);
  } else {
    Context->NumberOfTables = (Context->Rsdt->Header.Length - sizeof (Context->Rsdt->Header))
      / sizeof (Context->Rsdt->Tables[0]);
  }

  DEBUG ((DEBUG_INFO, "OCA: Found %u ACPI tables\n", Context->NumberOfTables));

  if (Context->NumberOfTables == 0) {
    DEBUG ((DEBUG_WARN, "OCA: No ACPI tables are available\n"));
    return EFI_INVALID_PARAMETER;
  }

  Context->Tables = AllocatePool (Context->NumberOfTables * sizeof (Context->Tables[0]));
  if (Context->Tables == NULL) {
    DEBUG ((DEBUG_WARN, "OCA: Cannot allocate space for %u ACPI tables\n", Context->NumberOfTables));
    return EFI_OUT_OF_RESOURCES;
  }

  Context->AllocatedTables = Context->NumberOfTables;

  for (DstIndex = 0, Index = 0; Index < Context->NumberOfTables; ++Index) {
    Context->Tables[DstIndex] = (EFI_ACPI_COMMON_HEADER *)(Context->Xsdt != NULL
      ? (UINTN) Context->Xsdt->Tables[Index] : (UINTN) Context->Rsdt->Tables[Index]);

    //
    // Skip NULL table entries, DSDT, and RSDP if any.
    //
    if (Context->Tables[DstIndex] == NULL
      || Context->Tables[DstIndex]->Signature == EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
      || *(UINT64 *) Context->Tables[DstIndex] == EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
      continue;
    }

    TablePrintSignature = AcpiReadSignature (Context->Tables[DstIndex]);

    DEBUG ((
      DEBUG_INFO,
      "OCA: Detected table %.4a (%08x) (OEM %016Lx) at %p of %u bytes at index %u\n",
      (CHAR8 *) &TablePrintSignature,
      Context->Tables[DstIndex]->Signature,
      AcpiReadOemTableId (Context->Tables[DstIndex]),
      Context->Tables[DstIndex],
      Context->Tables[DstIndex]->Length,
      Index
      ));

    //
    // Unlock table if in lower memory.
    //
    if ((UINTN)Context->Tables[DstIndex] < BASE_1MB) {
      DEBUG ((
        DEBUG_INFO,
        "OCA: Unlocking table %.4a (%08x) at %p\n",
        (CHAR8 *) &TablePrintSignature,
        Context->Tables[DstIndex]->Signature,
        Context->Tables[DstIndex]
        ));
      LegacyRegionUnlock ((UINT32)(UINTN)Context->Tables[DstIndex], Context->Tables[DstIndex]->Length);
    }

    if (Context->Tables[DstIndex]->Signature == EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      Context->Fadt = (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE *) Context->Tables[DstIndex];

      if (Context->Fadt->Header.Length >= OFFSET_OF (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE, XDsdt) + sizeof (Context->Fadt->XDsdt)) {
        Context->Dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN) Context->Fadt->XDsdt;
      } else {
        Context->Dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN) Context->Fadt->Dsdt;
      }
      DEBUG ((
        DEBUG_INFO,
        "OCA: Detected DSDT at %p of %u bytes at index %u\n",
        Context->Dsdt,
        Context->Dsdt->Length,
        Index
        ));

      if ((UINTN)Context->Dsdt < BASE_1MB) {
        DEBUG ((
          DEBUG_INFO,
          "OCA: Unlocking DSDT at %p\n",
          Context->Dsdt
          ));
        LegacyRegionUnlock ((UINT32)(UINTN)Context->Tables[DstIndex], Context->Tables[DstIndex]->Length);
      }
    }

    ++DstIndex;
  }

  if (Context->NumberOfTables != DstIndex) {
    DEBUG ((DEBUG_INFO, "OCA: Only %u ACPI tables out of %u were valid\n", DstIndex, Context->NumberOfTables));
    Context->NumberOfTables = DstIndex;
  }

  if (Context->Fadt == NULL) {
    DEBUG ((DEBUG_WARN, "OCA: Failed to find ACPI FADT table\n"));
  }

  if (Context->Dsdt == NULL) {
    DEBUG ((DEBUG_WARN, "OCA: Failed to find ACPI DSDT table\n"));
  }

  return EFI_SUCCESS;
}

VOID
AcpiFreeContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  if (Context->Tables != NULL) {
    FreePool (Context->Tables);
    Context->Tables = NULL;
  }

  if (Context->Regions != NULL) {
    FreePool (Context->Regions);
    Context->Regions = NULL;
  }
}

EFI_STATUS
AcpiApplyContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  EFI_STATUS            Status;
  UINT32                XsdtSize;
  UINT32                RsdtSize;
  UINT32                Index;
  UINT32                Size;
  EFI_PHYSICAL_ADDRESS  Table;
  UINT32                TablePrintSignature;

  XsdtSize = Context->Xsdt == NULL ? 0 : sizeof (*Context->Xsdt) + sizeof (Context->Xsdt->Tables[0]) * Context->NumberOfTables;
  RsdtSize = Context->Rsdt == NULL ? 0 : sizeof (*Context->Rsdt) + sizeof (Context->Rsdt->Tables[0]) * Context->NumberOfTables;
  Size     = ALIGN_VALUE (XsdtSize, sizeof (UINT64)) + ALIGN_VALUE (RsdtSize, sizeof (UINT64));

  Table = BASE_4GB - 1;
  Status = gBS->AllocatePages (
          AllocateMaxAddress,
          EfiACPIMemoryNVS,
          EFI_SIZE_TO_PAGES (Size),
          &Table
          );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCA: Failed to allocate %u bytes for ACPI system tables\n", Size));
    return Status;
  }

  if (Context->Xsdt != NULL) {
    CopyMem ((VOID *)(UINTN)Table, Context->Xsdt, sizeof (*Context->Xsdt));
    Context->Xsdt = (OC_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE *)(UINTN)Table;
    Context->Xsdt->Header.Length = XsdtSize;

    for (Index = 0; Index < Context->NumberOfTables; ++Index) {
      Context->Xsdt->Tables[Index] = (UINT64)(UINTN) Context->Tables[Index];

      TablePrintSignature = AcpiReadSignature (Context->Tables[Index]);

      DEBUG ((
        DEBUG_INFO,
        "OCA: Exposing XSDT table table %.4a (%08x) (OEM %016Lx) at %p of %u bytes at index %u\n",
        (CHAR8 *) &TablePrintSignature,
        Context->Tables[Index]->Signature,
        AcpiReadOemTableId (Context->Tables[Index]),
        Context->Tables[Index],
        Context->Tables[Index]->Length,
        Index
        ));
    }

    Context->Xsdt->Header.Checksum = 0;
    Context->Xsdt->Header.Checksum = CalculateCheckSum8 (
      (UINT8 *) Context->Xsdt,
      Context->Xsdt->Header.Length
      );

    Context->Rsdp->XsdtAddress = (UINT64) Table;
    Table += ALIGN_VALUE (XsdtSize, sizeof (UINT64));
  }

  if (Context->Rsdt != NULL) {
    CopyMem ((VOID *)(UINTN)Table, Context->Rsdt, sizeof (*Context->Rsdt));
    Context->Rsdt = (OC_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_TABLE *)(UINTN)Table;
    Context->Rsdt->Header.Length = RsdtSize;

    for (Index = 0; Index < Context->NumberOfTables; ++Index) {
      Context->Rsdt->Tables[Index] = (UINT32)(UINTN) Context->Tables[Index];

      TablePrintSignature = AcpiReadSignature (Context->Tables[Index]);

      DEBUG ((
        Context->Xsdt != NULL ? DEBUG_BULK_INFO : DEBUG_INFO,
        "OCA: Exposing RSDT table table %.4a (%08x) (OEM %016Lx) at %p of %u bytes at index %u\n",
        (CHAR8 *) &TablePrintSignature,
        Context->Tables[Index]->Signature,
        AcpiReadOemTableId (Context->Tables[Index]),
        Context->Tables[Index],
        Context->Tables[Index]->Length,
        Index
        ));
    }

    Context->Rsdt->Header.Checksum = 0;
    Context->Rsdt->Header.Checksum = CalculateCheckSum8 (
      (UINT8 *) Context->Rsdt,
      Context->Rsdt->Header.Length
      );

    Context->Rsdp->RsdtAddress = (UINT32) Table;
    Table += ALIGN_VALUE (RsdtSize, sizeof (UINT64));
  }

  //
  // Checksum is to be the first 0-19 bytes of RSDP.
  // ExtendedChecksum is the entire table, only if newer than ACPI 1.0.
  //
  Context->Rsdp->Checksum = 0;
  Context->Rsdp->Checksum = CalculateCheckSum8 (
    (UINT8 *) Context->Rsdp, 20
    );

  if (Context->Xsdt != NULL) {
    Context->Rsdp->ExtendedChecksum = 0;
    Context->Rsdp->ExtendedChecksum = CalculateCheckSum8 (
      (UINT8 *) Context->Rsdp, Context->Rsdp->Length
      );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AcpiDeleteTable (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     UINT32           Signature,
  IN     UINT32           Length,
  IN     UINT64           OemTableId,
  IN     BOOLEAN          All
  )
{
  UINT32   Index;
  UINT64   CurrOemTableId;
  BOOLEAN  Found;
  UINT32   TablePrintSignature;

  Index = 0;
  Found = FALSE;

  while (Index < Context->NumberOfTables) {
    if ((Signature == 0 || Context->Tables[Index]->Signature == Signature)
      && (Length == 0 || Context->Tables[Index]->Length == Length)) {

      if (Context->Tables[Index]->Length >= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
        CurrOemTableId = ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Tables[Index])->OemTableId;
      } else {
        CurrOemTableId = 0;
      }

      if (OemTableId == 0 || CurrOemTableId == OemTableId) {
        TablePrintSignature = AcpiReadSignature (Context->Tables[Index]);

        DEBUG ((
          DEBUG_INFO,
          "OCA: Deleting table %.4a (%08x) (OEM %016Lx) of %u bytes with %016Lx ID at index %u\n",
          (CHAR8 *) &TablePrintSignature,
          Context->Tables[Index]->Signature,
          AcpiReadOemTableId (Context->Tables[Index]),
          Context->Tables[Index]->Length,
          CurrOemTableId,
          Index
          ));

        CopyMem (
          &Context->Tables[Index],
          &Context->Tables[Index+1],
          (Context->NumberOfTables - Index - 1) * sizeof (Context->Tables[0])
          );
        --Context->NumberOfTables;

        if (All) {
          Found = TRUE;
          continue;
        } else {
          return EFI_SUCCESS;
        }
      }
    }

    ++Index;
  }

  if (Found) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
AcpiInsertTable (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     CONST UINT8      *Data,
  IN     UINT32           Length
  )
{
  EFI_ACPI_COMMON_HEADER    *Common;
  EFI_STATUS                Status;
  EFI_ACPI_COMMON_HEADER    *NewTable;
  EFI_ACPI_COMMON_HEADER    **NewTables;
  BOOLEAN                   ReplaceDsdt;
  UINT32                    TablePrintSignature;

  if (Length < sizeof (EFI_ACPI_COMMON_HEADER)) {
    DEBUG ((DEBUG_WARN, "OCA: Inserted ACPI table is only %u bytes, ignoring\n", Length));
    return EFI_INVALID_PARAMETER;
  }

  Common = (EFI_ACPI_COMMON_HEADER *) Data;
  if (Common->Length != Length) {
    DEBUG ((DEBUG_WARN, "OCA: Inserted ACPI table has length mismatch %u vs %u, ignoring\n", Length, Common->Length));
    return EFI_INVALID_PARAMETER;
  }

  ReplaceDsdt = Common->Signature == EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;

  if (ReplaceDsdt && (Context->Dsdt == NULL || Context->Fadt == NULL)) {
    DEBUG ((DEBUG_WARN, "OCA: We do not have DSDT to replace\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!ReplaceDsdt && Context->NumberOfTables == Context->AllocatedTables) {
    NewTables = AllocatePool ((Context->NumberOfTables + 2) * sizeof (Context->Tables[0]));
    if (NewTables == NULL) {
      DEBUG ((DEBUG_WARN, "OCA: Cannot allocate space for new %u ACPI tables\n", Context->NumberOfTables+2));
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (NewTables, Context->Tables, Context->NumberOfTables * sizeof (Context->Tables[0]));
    FreePool (Context->Tables);

    Context->Tables = NewTables;
    Context->AllocatedTables += 2;
  }

  if (ReplaceDsdt) {
    DEBUG ((
      DEBUG_INFO,
      "OCA: Replaced DSDT of %u bytes into ACPI\n",
      Common->Length
      ));

    Status = AcpiAllocateCopyDsdt (Context, (EFI_ACPI_DESCRIPTION_HEADER *) Data);
    if (EFI_ERROR (Status)) {
      return Status;
    }

  } else {
    Status = AcpiAllocateCopyTable ((EFI_ACPI_COMMON_HEADER *) Data, 0, &NewTable);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    TablePrintSignature = AcpiReadSignature (NewTable);

    DEBUG ((
      DEBUG_INFO,
      "OCA: Inserted table %.4a (%08x) (OEM %016Lx) of %u bytes into ACPI at index %u\n",
      (CHAR8 *) &TablePrintSignature,
      NewTable->Signature,
      AcpiReadOemTableId (NewTable),
      NewTable->Length,
      Context->NumberOfTables
      ));

    Context->Tables[Context->NumberOfTables] = NewTable;
    ++Context->NumberOfTables;
  }

  return EFI_SUCCESS;
}

VOID
AcpiNormalizeHeaders (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;
  EFI_ACPI_COMMON_HEADER  *NewTable;
  UINT32                  TablePrintSignature;

  if (AcpiNormalizeRsdp (Context->Rsdp, Context->Xsdt != NULL)) {
    DEBUG ((DEBUG_INFO, "OCA: Normalized RSDP\n"));
  }

  if (Context->Xsdt != NULL) {
    if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Xsdt)) {
      Status = AcpiAllocateCopyTable ((EFI_ACPI_COMMON_HEADER *) Context->Xsdt, 0, &NewTable);
      if (EFI_ERROR (Status)) {
        return;
      }
      Context->Xsdt = (OC_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE *) NewTable;
    }

    if (AcpiNormalizeTableHeaders ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Xsdt)) {
      DEBUG ((DEBUG_INFO, "OCA: Normalized XSDT of %u bytes headers\n", Context->Xsdt->Header.Length));
    }
  }

  if (Context->Rsdt != NULL) {
    if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Rsdt)) {
      Status = AcpiAllocateCopyTable ((EFI_ACPI_COMMON_HEADER *) Context->Rsdt, 0, &NewTable);
      if (EFI_ERROR (Status)) {
        return;
      }
      Context->Rsdt = (OC_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_TABLE *) NewTable;
    }

    if (AcpiNormalizeTableHeaders ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Rsdt)) {
      DEBUG ((DEBUG_INFO, "OCA: Normalized RSDT of %u bytes headers\n", Context->Rsdt->Header.Length));
    }
  }

  if (Context->Dsdt != NULL) {
    if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Dsdt)) {
      Status = AcpiAllocateCopyDsdt (Context, NULL);
      if (EFI_ERROR (Status)) {
        return;
      }
    }

    if (AcpiNormalizeTableHeaders (Context->Dsdt)) {
      DEBUG ((DEBUG_INFO, "OCA: Normalized DSDT of %u bytes headers\n", Context->Dsdt->Length));
    }
  }

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if (!AcpiIsTableWritable (Context->Tables[Index])) {
      Status = AcpiAllocateCopyTable (Context->Tables[Index], 0, &NewTable);
      if (EFI_ERROR (Status)) {
        return;
      }
      Context->Tables[Index] = NewTable;
    }

    if (AcpiNormalizeTableHeaders ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Tables[Index])) {
      TablePrintSignature = AcpiReadSignature (Context->Tables[Index]);

      DEBUG ((
        DEBUG_INFO,
        "OCA: Normalized %.4a (%08x) (OEM %016Lx) of %u bytes headers at index %u\n",
        (CHAR8 *) &TablePrintSignature,
        Context->Tables[Index]->Signature,
        AcpiReadOemTableId (Context->Tables[Index]),
        Context->Tables[Index]->Length,
        Index
        ));
    }
  }
}

EFI_STATUS
AcpiApplyPatch (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     OC_ACPI_PATCH    *Patch
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_COMMON_HEADER  *NewTable;
  UINT32                  Index;
  UINT32                  BaseOffset;
  UINT64                  CurrOemTableId;
  UINT32                  ReplaceCount;
  UINT32                  ReplaceLimit;
  UINT32                  TablePrintSignature;

  DEBUG ((DEBUG_INFO, "OCA: Applying %u byte ACPI patch skip %u, count %u\n", Patch->Size, Patch->Skip, Patch->Count));

  if (Context->Dsdt != NULL
    && (Patch->TableSignature == 0 || Patch->TableSignature == EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)
    && (Patch->TableLength == 0 || Context->Dsdt->Length == Patch->TableLength)
    && (Patch->OemTableId == 0 || Context->Dsdt->OemTableId == Patch->OemTableId)) {
    ReplaceLimit = Context->Dsdt->Length;
    if (Patch->Limit > 0) {
      ReplaceLimit = MIN (ReplaceLimit, Patch->Limit);
    }

    BaseOffset = 0;

    if (Patch->Base != NULL && Patch->Base[0] != '\0') {
      Status = AcpiFindEntryInMemory (
        (VOID *) Context->Dsdt,
        Patch->Base,
        (UINT8) (Patch->BaseSkip + 1),
        &BaseOffset,
        Context->Dsdt->Length
        );
      if (!EFI_ERROR (Status)) {
        ReplaceLimit = MIN (ReplaceLimit, Context->Dsdt->Length - BaseOffset);
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCA: Patching DSDT of %u bytes failed to find base %a\n",
          Context->Dsdt->Length,
          Patch->Base
          ));
      }      
    } else {
      Status = EFI_SUCCESS;
    }

    if (!EFI_ERROR (Status)) {
      if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Dsdt)) {
        Status = AcpiAllocateCopyDsdt (Context, NULL);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      ReplaceCount = ApplyPatch (
        Patch->Find,
        Patch->Mask,
        Patch->Size,
        Patch->Replace,
        Patch->ReplaceMask,
        (UINT8 *) Context->Dsdt + BaseOffset,
        ReplaceLimit,
        Patch->Count,
        Patch->Skip
        );

      DEBUG ((
        ReplaceCount > 0 ? DEBUG_INFO : DEBUG_BULK_INFO,
        "OCA: Patching DSDT of %u bytes with %016Lx ID replaced %u of %u\n",
        ReplaceLimit,
        Patch->OemTableId,
        ReplaceCount,
        Patch->Count
        ));

      if (ReplaceCount > 0) {
        AcpiRefreshTableChecksum (Context->Dsdt);
      }
    }
  }

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if ((Patch->TableSignature == 0 || Context->Tables[Index]->Signature == Patch->TableSignature)
      && (Patch->TableLength == 0 || Context->Tables[Index]->Length == Patch->TableLength)) {

      if (Context->Tables[Index]->Length >= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
        CurrOemTableId = ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Tables[Index])->OemTableId;
      } else {
        CurrOemTableId = 0;
      }

      if (Patch->OemTableId != 0 && CurrOemTableId != Patch->OemTableId) {
        continue;
      }

      ReplaceLimit = Context->Tables[Index]->Length;
      if (Patch->Limit > 0) {
        ReplaceLimit = MIN (ReplaceLimit, Patch->Limit);
      }

      TablePrintSignature = AcpiReadSignature (Context->Tables[Index]);

      BaseOffset = 0;
      if (Patch->Base != NULL && Patch->Base[0] != '\0') {
        Status = AcpiFindEntryInMemory (
          (VOID *) Context->Tables[Index],
          Patch->Base,
          (UINT8) (Patch->BaseSkip + 1),
          &BaseOffset,
          Context->Tables[Index]->Length
          );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "OCA: Patching %.4a (%08x) (OEM %016Lx) of %u bytes with %016Lx ID failed to find base %a\n",
            (CHAR8 *) &TablePrintSignature,
            Context->Tables[Index]->Signature,
            AcpiReadOemTableId (Context->Tables[Index]),
            Context->Tables[Index]->Length,
            CurrOemTableId,
            Patch->Base
            ));
          continue;
        }

        ReplaceLimit = MIN (ReplaceLimit, Context->Tables[Index]->Length - BaseOffset);
      }

      if (!AcpiIsTableWritable (Context->Tables[Index])) {
        Status = AcpiAllocateCopyTable (Context->Tables[Index], 0, &NewTable);
        if (EFI_ERROR (Status)) {
          return Status;
        }
        Context->Tables[Index] = NewTable;
      }

      ReplaceCount = ApplyPatch (
        Patch->Find,
        Patch->Mask,
        Patch->Size,
        Patch->Replace,
        Patch->ReplaceMask,
        (UINT8 *) Context->Tables[Index] + BaseOffset,
        ReplaceLimit,
        Patch->Count,
        Patch->Skip
        );

      DEBUG ((
        ReplaceCount > 0 ? DEBUG_INFO : DEBUG_BULK_INFO,
        "OCA: Patching %.4a (%08x) (OEM %016Lx) of %u bytes with %016Lx ID at %u replaced %u of %u\n",
        (CHAR8 *) &TablePrintSignature,
        Context->Tables[Index]->Signature,
        AcpiReadOemTableId (Context->Tables[Index]),
        Context->Tables[Index]->Length,
        CurrOemTableId,
        Index,
        ReplaceCount,
        Patch->Count
        ));

      if (ReplaceCount > 0 && Context->Tables[Index]->Length >= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
        AcpiRefreshTableChecksum ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Tables[Index]);
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AcpiLoadRegions (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  //
  // Should not be called twice, but just in case.
  //
  ASSERT (Context->Regions == NULL);

  //
  // Allocate something reasonably large by default.
  //
  Context->NumberOfRegions  = 0;
  Context->AllocatedRegions = 8;
  Context->Regions = AllocatePool (sizeof (Context->Regions[0]) * Context->AllocatedRegions);

  if (Context->Regions == NULL) {
    DEBUG ((DEBUG_WARN, "OCA: Failed to allocate memory for %u regions\n", Context->NumberOfRegions));
    return EFI_OUT_OF_RESOURCES;
  }

  if (Context->Dsdt != NULL) {
    Status = AcpiLoadTableRegions (Context, (EFI_ACPI_COMMON_HEADER *) Context->Dsdt);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if (Context->Tables[Index]->Signature == EFI_ACPI_6_2_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      Status = AcpiLoadTableRegions (Context, Context->Tables[Index]);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

VOID
AcpiRelocateRegions (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;
  EFI_ACPI_COMMON_HEADER  *NewTable;

  //
  // Should not be called before AcpiLoadRegions, but just in case.
  //
  ASSERT (Context->Regions != NULL);

  if (Context->NumberOfRegions == 0) {
    return;
  }

  if (Context->Dsdt != NULL) {
    if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Dsdt)) {
      Status = AcpiAllocateCopyDsdt (Context, NULL);
      if (EFI_ERROR (Status)) {
        return;
      }
    }

    AcpiRelocateTableRegions (Context, (EFI_ACPI_COMMON_HEADER *) Context->Dsdt);
  }

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if (Context->Tables[Index]->Signature == EFI_ACPI_6_2_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      if (!AcpiIsTableWritable (Context->Tables[Index])) {
        Status = AcpiAllocateCopyTable (Context->Tables[Index], 0, &NewTable);
        if (EFI_ERROR (Status)) {
          return;
        }
        Context->Tables[Index] = NewTable;
      }

      AcpiRelocateTableRegions (Context, Context->Tables[Index]);
    }
  }
}

EFI_STATUS
AcpiFadtEnableReset (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  EFI_STATUS    Status;
  UINT32        OldSize;
  UINT32        RequiredSize;

  if (Context->Fadt == NULL) {
    return EFI_NOT_FOUND;
  }

  OldSize      = Context->Fadt->Header.Length;
  RequiredSize = OFFSET_OF (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE, ArmBootArch);

  DEBUG ((DEBUG_INFO, "OCA: FADT reset got %u need %u\n", OldSize, RequiredSize));

  //
  // One some types of firmware, the table is too small and does not include
  // Reset Register area. We will reallocate in this case.
  // Interestingly EFI_ACPI_6_2_RESET_REG_SUP may be set.
  //
  // REF: https://github.com/acidanthera/bugtracker/issues/897
  //

  if (OldSize < RequiredSize || !AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Fadt)) {
    Status = AcpiAllocateCopyFadt (Context, OldSize < RequiredSize ? RequiredSize : 0);
    if (EFI_ERROR (Status)) {
      return Status;
    }

  } else if (!((Context->Fadt->Flags & EFI_ACPI_6_2_RESET_REG_SUP) == 0
    || (Context->Fadt->Flags & EFI_ACPI_6_2_SLP_BUTTON) == 0
    || (Context->Fadt->Flags & EFI_ACPI_6_2_PWR_BUTTON) != 0
    || Context->Fadt->ResetReg.Address == 0
    || Context->Fadt->ResetReg.RegisterBitWidth != 8)) {
    return EFI_SUCCESS;
  }

  //
  // Enable sleep button, but disable power button actions.
  // This resolves power button action in macOS on some models.
  //
  Context->Fadt->Flags |= EFI_ACPI_6_2_SLP_BUTTON | EFI_ACPI_6_2_RESET_REG_SUP;
  Context->Fadt->Flags &= ~EFI_ACPI_6_2_PWR_BUTTON;

  if (Context->Fadt->ResetReg.Address == 0 || Context->Fadt->ResetReg.RegisterBitWidth != 8) {
    //
    // Resetting through port 0xCF9 is universal on Intel and AMD.
    // But may not be the case on some laptops, which use 0xB2.
    //
    Context->Fadt->ResetReg.AddressSpaceId    = EFI_ACPI_6_2_SYSTEM_IO;
    Context->Fadt->ResetReg.RegisterBitWidth  = 8;
    Context->Fadt->ResetReg.RegisterBitOffset = 0;
    Context->Fadt->ResetReg.AccessSize        = EFI_ACPI_6_2_BYTE;
    Context->Fadt->ResetReg.Address           = 0xCF9;
    Context->Fadt->ResetValue                 = 6;
  }

  AcpiRefreshTableChecksum ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Fadt);

  return EFI_SUCCESS;
}

VOID
AcpiResetLogoStatus (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  EFI_ACPI_6_2_BOOT_GRAPHICS_RESOURCE_TABLE *Bgrt;
  EFI_ACPI_6_2_BOOT_GRAPHICS_RESOURCE_TABLE *BgrtNew;

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if (Context->Tables[Index]->Signature == EFI_ACPI_6_2_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE) {
      if (Context->Tables[Index]->Length >= sizeof (EFI_ACPI_6_2_BOOT_GRAPHICS_RESOURCE_TABLE)) {
        Bgrt = (EFI_ACPI_6_2_BOOT_GRAPHICS_RESOURCE_TABLE *) Context->Tables[Index];

        if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Bgrt)) {
          Status = AcpiAllocateCopyTable (
            (EFI_ACPI_COMMON_HEADER *) Bgrt,
            0,
            (EFI_ACPI_COMMON_HEADER **) &BgrtNew
            );

          if (EFI_ERROR (Status)) {
            return;
          }

          Context->Tables[Index] = (EFI_ACPI_COMMON_HEADER *) BgrtNew;
          Bgrt                   = BgrtNew;
        }

        Bgrt->Status &= ~EFI_ACPI_6_2_BGRT_STATUS_DISPLAYED;
        Bgrt->Header.Checksum = 0;
        Bgrt->Header.Checksum = CalculateCheckSum8 (
          (UINT8 *) Bgrt,
          Bgrt->Header.Length
          );
      }
      break;
    }
  }
}

VOID
AcpiSyncTableIds (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  *Slic;
  UINT32                       Index;

  Slic = NULL;
  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if (Context->Tables[Index]->Signature == EFI_ACPI_6_2_SOFTWARE_LICENSING_TABLE_SIGNATURE) {
      Slic = (VOID *)Context->Tables[Index];
      break;
    }
  }

  if (Slic == NULL) {
    DEBUG ((DEBUG_INFO, "OCA: SLIC table is not found\n"));
    return;
  }

  //
  // SLIC identifiers must match RSDT and FADT, also doing XSDT for newer EFI just in case.
  // REF: https://bugzilla.redhat.com/show_bug.cgi?id=1248758
  //

  if (Context->Rsdt != NULL) {
    CopyMem (&Context->Rsdt->Header.OemId, &Slic->OemId, sizeof (Context->Rsdt->Header.OemId));
    Context->Rsdt->Header.OemTableId = Slic->OemTableId;
    Context->Rsdt->Header.Checksum = 0;
    Context->Rsdt->Header.Checksum = CalculateCheckSum8 (
      (UINT8 *) Context->Rsdt,
      Context->Rsdt->Header.Length
      );
    DEBUG ((DEBUG_INFO, "OCA: SLIC table IDs fixed in RSDT\n"));
  }

  if (Context->Xsdt != NULL) {
    CopyMem (&Context->Xsdt->Header.OemId, &Slic->OemId, sizeof (Context->Xsdt->Header.OemId));
    Context->Xsdt->Header.OemTableId = Slic->OemTableId;
    Context->Xsdt->Header.Checksum = 0;
    Context->Xsdt->Header.Checksum = CalculateCheckSum8 (
      (UINT8 *) Context->Xsdt,
      Context->Xsdt->Header.Length
      );
    DEBUG ((DEBUG_INFO, "OCA: SLIC table IDs fixed in XSDT\n"));
  }

  if (Context->Fadt != NULL) {
    CopyMem (&Context->Fadt->Header.OemId, &Slic->OemId, sizeof (Context->Fadt->Header.OemId));
    Context->Fadt->Header.OemTableId = Slic->OemTableId;
    Context->Fadt->Header.Checksum = 0;
    Context->Fadt->Header.Checksum = CalculateCheckSum8 (
      (UINT8 *) Context->Fadt,
      Context->Fadt->Header.Length
      );
    DEBUG ((DEBUG_INFO, "OCA: SLIC table IDs fixed in FADT\n"));
  }
}

VOID
AcpiHandleHardwareSignature (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     BOOLEAN          Reset
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsXFirmwareCtrl;

  EFI_ACPI_6_2_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  EFI_ACPI_6_2_FIRMWARE_ACPI_CONTROL_STRUCTURE  *FacsNew;

  if (Context->Fadt == NULL) {
    return;
  }

  if (Context->Fadt->Header.Length >= OFFSET_OF (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE, XFirmwareCtrl) + sizeof (Context->Fadt->XFirmwareCtrl)) {
    Facs = (EFI_ACPI_6_2_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN) Context->Fadt->XFirmwareCtrl;
    IsXFirmwareCtrl = TRUE;
  } else {
    Facs = NULL;
    IsXFirmwareCtrl = FALSE;
  }

  if (Facs == NULL && Context->Fadt->Header.Length >= OFFSET_OF (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE, FirmwareCtrl) + sizeof (Context->Fadt->FirmwareCtrl)) {
    Facs = (EFI_ACPI_6_2_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN) Context->Fadt->FirmwareCtrl;
  }

  if (Facs != NULL && Facs->Length >= OFFSET_OF (EFI_ACPI_6_2_FIRMWARE_ACPI_CONTROL_STRUCTURE, Flags) + sizeof (Facs->HardwareSignature)) {
    DEBUG ((DEBUG_INFO, "OCA: FACS signature is %X (%d)\n", Facs->Flags, Reset));

    if (Reset) {
      if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Facs)) {
        Status = AcpiAllocateCopyTable (
          (EFI_ACPI_COMMON_HEADER *) Facs,
          0,
          (EFI_ACPI_COMMON_HEADER **) &FacsNew
          );
        if (EFI_ERROR (Status)) {
          return;
        }

        //
        // Reallocate FADT if needed.
        //
        if (!AcpiIsTableWritable ((EFI_ACPI_COMMON_HEADER *) Context->Fadt)) {
          Status = AcpiAllocateCopyFadt (Context, 0);
          if (EFI_ERROR (Status)) {
            FreePool (FacsNew);
            return;
          }
        }

        if (IsXFirmwareCtrl) {
          Context->Fadt->XFirmwareCtrl = (UINT64)(UINTN) FacsNew;
        } else {
          Context->Fadt->FirmwareCtrl = (UINT32)(UINTN) FacsNew;
        }
        AcpiRefreshTableChecksum ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Fadt);

        Facs = FacsNew;
      }

      //
      // TODO: We might also want to unset S4BIOS_F flag in Facs->Flags.
      //
      Facs->HardwareSignature = 0x0;
    }
  } else {
    //
    // For macOS this is just fine.
    //
    DEBUG ((
      DEBUG_INFO,
      "OCA: FACS signature is too far %d / %u\n",
      Facs != NULL,
      Facs != NULL ? Facs->Length : 0
      ));
  }
}

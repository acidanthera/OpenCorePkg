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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcMiscLib.h>

#include <IndustryStandard/AcpiAml.h>
#include <IndustryStandard/Acpi62.h>

#include <Guid/Acpi.h>

#include <Library/OcAcpiLib.h>


/** Find RSD_PTR Table In Legacy Area

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
    Address = ((*(UINT16 *) 0x040E) << 4);

    for (Index = 0; Index < 0x0400; Index += 16) {
      if (*(UINT64 *) (Address + Index) == EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
        Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) Address;
      }
    }
  }

  return Rsdp;
}

/** Find RSD_PTR Table From System Configuration Tables

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
      DEBUG ((DEBUG_VERBOSE, "Found ACPI 2.0 RSDP table %p\n", Rsdp));
      break;
    }

    //
    // Otherwise use ACPI 1.0, but do search for ACPI 2.0.
    //
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiAcpi10TableGuid)) {
      Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      DEBUG ((DEBUG_VERBOSE, "Found ACPI 1.0 RSDP table %p\n", Rsdp));
    }
  }

  //
  // Try to use legacy search as a last resort.
  //
  if (Rsdp == NULL) {
    Rsdp = AcpiFindLegacyRsdp ();
    if (Rsdp != NULL) {
      DEBUG ((DEBUG_VERBOSE, "Found ACPI legacy RSDP table %p\n", Rsdp));
    }
  }

  if (Rsdp == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to find ACPI RSDP table\n"));
  }

  return Rsdp;
}

STATIC
EFI_STATUS
AcpiReplaceDsdt (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     CONST UINT8      *Data,
  IN     UINT32           Length
  )
{
  DEBUG ((DEBUG_WARN, "Patching DSDT is not yet supported\n"));
  return EFI_UNSUPPORTED;
}

EFI_STATUS
AcpiInitContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  UINT32  Index;
  UINT32  DstIndex;

  ZeroMem (Context, sizeof (*Context));

  Context->Rsdp = AcpiFindRsdp ();

  if (Context->Rsdp == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Support RSDT on ACPI 1.0 and newer.
  //
  Context->Rsdt = (OC_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_TABLE *)(UINTN) Context->Rsdp->RsdtAddress;
  DEBUG ((DEBUG_VERBOSE, "Found ACPI RSDT table %p", Context->Rsdt));

  //
  // ACPI 2.0 and newer have XSDT as well.
  //
  if (Context->Rsdp->Revision > 0) {
    Context->Xsdt = (OC_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE *)(UINTN) Context->Rsdp->XsdtAddress;
    DEBUG ((DEBUG_VERBOSE, "Found ACPI XSDT table %p", Context->Xsdt));
  }

  if (Context->Rsdt == NULL && Context->Xsdt == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to find ACPI RSDT or XSDT tables\n"));
    return EFI_NOT_FOUND;
  }

  if (Context->Xsdt != NULL) {
    Context->NumberOfTables = (Context->Xsdt->Header.Length - sizeof (Context->Xsdt->Header))
      / sizeof (Context->Xsdt->Tables[0]);
  } else {
    Context->NumberOfTables = (Context->Rsdt->Header.Length - sizeof (Context->Rsdt->Header))
      / sizeof (Context->Rsdt->Tables[0]);
  }

  DEBUG ((DEBUG_INFO, "Found %u ACPI tables\n", Context->NumberOfTables));

  if (Context->NumberOfTables == 0) {
    DEBUG ((DEBUG_WARN, "No ACPI tables are available\n"));
    return EFI_INVALID_PARAMETER;
  }

  Context->Tables = AllocatePool (Context->NumberOfTables * sizeof (Context->Tables[0]));
  if (Context->Tables == NULL) {
    DEBUG ((DEBUG_WARN, "Cannot allocate space for %u ACPI tables\n", Context->NumberOfTables));
    return EFI_OUT_OF_RESOURCES;
  }

  Context->AllocatedTables = Context->NumberOfTables;

  for (DstIndex = 0, Index = 0; Index < Context->NumberOfTables; ++Index) {
    Context->Tables[DstIndex] = (EFI_ACPI_COMMON_HEADER *)(Context->Xsdt != NULL
      ? Context->Xsdt->Tables[Index] : (UINT64) Context->Rsdt->Tables[Index]);

    //
    // Skip NULL table entries, DSDT, and RSDP if any.
    //
    if (Context->Tables[DstIndex] == NULL
      || Context->Tables[DstIndex]->Signature == EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
      || *(UINT64 *) Context->Tables[DstIndex] == EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "Detected table %08x of %u bytes at index %u\n",
      Context->Tables[DstIndex]->Signature,
      Context->Tables[DstIndex]->Length,
      Index
      ));

    if (Context->Tables[DstIndex]->Signature == EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      Context->Fadt = (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE *) Context->Tables[DstIndex];

      if (Context->Fadt->Header.Length >= OFFSET_OF (EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE, XDsdt) + sizeof (Context->Fadt->XDsdt)) {
        Context->Dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN) Context->Fadt->XDsdt;
      } else {
        Context->Dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN) Context->Fadt->Dsdt;
      }
    }

    ++DstIndex;
  }

  if (Context->NumberOfTables != DstIndex) {
    DEBUG ((DEBUG_WARN, "Only %u ACPI tables out of %u were valid\n", DstIndex, Context->NumberOfTables));
    Context->NumberOfTables = DstIndex;
  }

  if (Context->Fadt == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to find ACPI FADT table\n"));
  }

  if (Context->Dsdt == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to find ACPI DSDT table\n"));
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
    DEBUG ((DEBUG_WARN, "Failed to allocate %u bytes for ACPI system tables\n", Size));
    return Status;
  }

  if (Context->Xsdt != NULL) {
    CopyMem ((VOID *) Table, Context->Xsdt, sizeof (*Context->Xsdt));
    Context->Xsdt = (OC_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE *) Table;
    Context->Xsdt->Header.Length = XsdtSize;

    for (Index = 0; Index < Context->NumberOfTables; ++Index) {
      Context->Xsdt->Tables[Index] = (UINT64)(UINTN) Context->Tables[Index];
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
    CopyMem ((VOID *) Table, Context->Rsdt, sizeof (*Context->Rsdt));
    Context->Rsdt = (OC_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_TABLE *) Table;
    Context->Rsdt->Header.Length = RsdtSize;

    for (Index = 0; Index < Context->NumberOfTables; ++Index) {
      Context->Rsdt->Tables[Index] = (UINT32)(UINTN) Context->Tables[Index];
    }

    Context->Rsdt->Header.Checksum = 0;
    Context->Rsdt->Header.Checksum = CalculateCheckSum8 (
      (UINT8 *) Context->Rsdt,
      Context->Rsdt->Header.Length
      );

    Table += ALIGN_VALUE (RsdtSize, sizeof (UINT64));
  }

  Context->Rsdp->Checksum = 0;
  Context->Rsdp->Checksum = CalculateCheckSum8 (
    (UINT8 *) Context->Rsdp,
    Context->Xsdt != NULL ? Context->Rsdp->Length : 20
    );

  return EFI_UNSUPPORTED;
}

EFI_STATUS
AcpiDropTable (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     UINT32           Signature,
  IN     UINT32           Length,
  IN     UINT64           OemTableId
  )
{
  UINT32  Index;
  UINT64  CurrOemTableId;

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if ((Signature == 0 || Context->Tables[Index]->Signature == Signature)
      && (Length == 0 || Context->Tables[Index]->Length == Length)) {

      if (Context->Tables[Index]->Length >= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
        CurrOemTableId = ((EFI_ACPI_DESCRIPTION_HEADER *) Context->Tables[Index])->OemTableId;
      } else {
        CurrOemTableId = 0;
      }

      if (OemTableId != 0 && CurrOemTableId != OemTableId) {
        continue;
      }

      DEBUG ((
        DEBUG_INFO,
        "Dropping table %08x of %u bytes with %016Lx ID at index %u\n",
        Context->Tables[Index]->Signature,
        Context->Tables[Index]->Length,
        CurrOemTableId,
        Index
        ));

      CopyMem (
        &Context->Tables[Index],
        &Context->Tables[Index+1],
        Context->NumberOfTables - Index - 1
        );
      --Context->NumberOfTables;
      return EFI_SUCCESS;
    }
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
  EFI_PHYSICAL_ADDRESS      Table;
  EFI_ACPI_COMMON_HEADER    **NewTables;

  if (Length < sizeof (EFI_ACPI_COMMON_HEADER)) {
    DEBUG ((DEBUG_WARN, "Inserted ACPI table is only %u bytes, ignoring\n", Length));
    return EFI_INVALID_PARAMETER;
  }

  Common = (EFI_ACPI_COMMON_HEADER *) Data;
  if (Common->Length != Length) {
    DEBUG ((DEBUG_WARN, "Inserted ACPI table has length mismatch %u vs %u, ignoring\n", Length, Common->Length));
    return EFI_INVALID_PARAMETER;
  }

  if (Common->Signature == EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
    return AcpiReplaceDsdt (Context, Data, Length);
  }

  if (Context->NumberOfTables == Context->AllocatedTables) {
    NewTables = AllocatePool ((Context->NumberOfTables + 2) * sizeof (Context->Tables[0]));
    if (NewTables == NULL) {
      DEBUG ((DEBUG_WARN, "Cannot allocate space for new %u ACPI tables\n", Context->NumberOfTables+2));
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (NewTables, Context->Tables, Context->NumberOfTables * sizeof (Context->Tables[0]));
    FreePool (Context->Tables);

    Context->Tables = NewTables;
    Context->AllocatedTables += 2;
  }

  Table = BASE_4GB - 1;
  Status = gBS->AllocatePages (
          AllocateMaxAddress,
          EfiACPIMemoryNVS,
          EFI_SIZE_TO_PAGES (Length),
          &Table
          );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to allocate %u bytes for inserted ACPI table\n", Length));
    return Status;
  }

  CopyMem ((UINT8 *) Table, Data, Length);
  ZeroMem ((UINT8 *) Table + Length, EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Length)) - Length);

  DEBUG ((
    DEBUG_INFO,
    "Inserted table %08x of %u bytes into ACPI at index %u\n",
    Common->Signature,
    Common->Length,
    Context->NumberOfTables
    ));

  Context->Tables[Context->NumberOfTables] = (EFI_ACPI_COMMON_HEADER *) Table;
  ++Context->NumberOfTables;

  return EFI_UNSUPPORTED;
}

VOID
AcpiNormalizeHeaders (
  IN OUT OC_ACPI_CONTEXT  *Context
  )
{
  UINT32  Index;

  for (Index = 0; Index < Context->NumberOfTables; ++Index) {
    if (Context->Tables[Index]->Length >= sizeof (EFI_ACPI_COMMON_HEADER)) {
      // TODO: normalize headers (https://alextjam.es/debugging-appleacpiplatform/)
    }
  }
}

EFI_STATUS
AcpiApplyPatch (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     OC_ACPI_PATCH    *Patch
  )
{
  UINT32  Index;
  UINT64  CurrOemTableId;
  UINT32  ReplaceCount;

  DEBUG ((DEBUG_INFO, "Applying %u byte ACPI patch skip %u, count %u\n", Patch->Size, Patch->Skip, Patch->Count));

  if (Context->Dsdt != NULL
    && (Patch->TableSignature == 0 || Patch->TableSignature == EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)
    && (Patch->TableLength == 0 || Context->Dsdt->Length == Patch->TableLength)
    && (Patch->OemTableId == 0 || Context->Dsdt->OemTableId == Patch->OemTableId)) {
    DEBUG ((
      DEBUG_INFO,
      "Patching DSDT of %u bytes with %016Lx ID\n",
      Patch->TableLength,
      Patch->OemTableId
      ));

    ReplaceCount = ApplyPatch (
      Patch->Find,
      Patch->Mask,
      Patch->Size,
      Patch->Replace,
      (UINT8 *) Context->Dsdt,
      Context->Dsdt->Length,
      Patch->Count,
      Patch->Skip
      );

    (VOID) ReplaceCount;
    DEBUG ((DEBUG_INFO, "Replaced %u matches out of requested %u in DSDT\n", ReplaceCount, Patch->Count));

    if (ReplaceCount > 0) {
      Context->Dsdt->Checksum = 0;
      Context->Dsdt->Checksum = CalculateCheckSum8 (
        (UINT8 *) Context->Dsdt,
        Context->Dsdt->Length
        );

      DEBUG ((
        DEBUG_INFO,
        "Refreshed DSDT checksum to %02x\n",
        Context->Dsdt->Checksum
        ));
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

      DEBUG ((
        DEBUG_INFO,
        "Patching table %08x of %u bytes with %016Lx ID at index %u\n",
        Context->Tables[Index]->Signature,
        Context->Tables[Index]->Length,
        CurrOemTableId,
        Index
        ));

      ReplaceCount = ApplyPatch (
        Patch->Find,
        Patch->Mask,
        Patch->Size,
        Patch->Replace,
        (UINT8 *) Context->Tables[Index],
        Context->Tables[Index]->Length,
        Patch->Count,
        Patch->Skip
        );

      (VOID) ReplaceCount;
      DEBUG ((DEBUG_INFO, "Replaced %u matches out of requested %u\n", ReplaceCount, Patch->Count));

      if (ReplaceCount > 0 && Context->Tables[Index]->Length >= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
        ((EFI_ACPI_DESCRIPTION_HEADER *)Context->Tables[Index])->Checksum = 0;
        ((EFI_ACPI_DESCRIPTION_HEADER *)Context->Tables[Index])->Checksum = CalculateCheckSum8 (
          (UINT8 *) Context->Tables[Index],
          Context->Tables[Index]->Length
          );

        DEBUG ((
          DEBUG_INFO,
          "Refreshed checksum to %02x\n",
          ((EFI_ACPI_DESCRIPTION_HEADER *)Context->Tables[Index])->Checksum
          ));
      }
    }
  }

  return EFI_UNSUPPORTED;
}

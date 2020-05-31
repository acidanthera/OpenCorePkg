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
#include <Library/OcFileLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcMiscLib.h>

#include <IndustryStandard/AcpiAml.h>
#include <IndustryStandard/Acpi.h>

#include <Guid/Acpi.h>

#include <Library/OcAcpiLib.h>

STATIC
EFI_STATUS
AcpiDumpTable (
  IN EFI_FILE_PROTOCOL  *Root,
  IN CONST CHAR8        *Name,
  IN VOID               *Data,
  IN UINTN              Size
  )
{
  CHAR16  TempName[16];
  UnicodeSPrint (TempName, sizeof (TempName), L"%a.aml", Name);
  return SetFileData (Root, TempName, Data, (UINT32) Size);
}

STATIC
VOID
AcpiGetTableName (
  IN  EFI_FILE_PROTOCOL       *Root,
  IN  EFI_ACPI_COMMON_HEADER  *Data,
  OUT CHAR8                   *Name,
  IN  UINTN                   NameSize
  )
{
  EFI_STATUS         Status;
  CHAR16             TempName[16];
  CHAR8              *Signature;
  UINTN              Index;
  EFI_FILE_PROTOCOL  *TmpFile;

  ASSERT (NameSize >= 12);

  Signature = (CHAR8*) &Data->Signature;

  for (Index = 0; Index < sizeof (Data->Signature); ++Index) {
    if ((Signature[Index] >= 'A' && Signature[Index] <= 'Z')
      || (Signature[Index] >= 'a' && Signature[Index] <= 'z')
      || (Signature[Index] >= '0' && Signature[Index] <= '9')) {
      Name[Index] = Signature[Index];
    } else {
      Name[Index] = '_';
    }
  }

  Name[Index] = '\0';

  for (Index = 0; Index < 256; ++Index) {
    UnicodeSPrint (TempName, sizeof (TempName), L"%a-%u.aml", Name, (UINT32) (Index + 1));

    Status = SafeFileOpen (
      Root,
      &TmpFile,
      TempName,
      EFI_FILE_MODE_READ,
      0
      );

    if (EFI_ERROR (Status)) {
      break;
    }

    TmpFile->Close (TmpFile);
  }

  AsciiSPrint (&Name[4], NameSize - 4, "-%u", (UINT32) (Index + 1));
}

EFI_STATUS
AcpiDumpTables (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS       Status;
  OC_ACPI_CONTEXT  Context;
  UINTN            Index;
  UINTN            Length;
  CHAR8            Name[16];

  Status = AcpiInitContext (&Context);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCA: Failed to init context for dumping - %r\n", Status));
    return Status;
  }

  //
  // The revision of this structure. Larger revision numbers are backward compatible to
  // lower revision numbers. The ACPI version 1.0 revision number of this table is zero.
  // The ACPI version 1.0 RSDP Structure only includes the first 20 bytes of this table,
  // bytes 0 to 19. It does not include the Length field and beyond. The current value
  // for this field is 2.
  //
  if (Context.Rsdp->Revision == 0) {
    Length = OFFSET_OF (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER, Length);
  } else {
    Length = Context.Rsdp->Length;
  }
  Status = AcpiDumpTable (Root, "RSDP", Context.Rsdp, Length);
  DEBUG ((DEBUG_INFO, "OCA: Dumped RSDP (%u bytes) - %r\n", (UINT32) Length, Status));

  Status = AcpiDumpTable (Root, "RSDT", Context.Rsdt, Context.Rsdt->Header.Length);
  DEBUG ((DEBUG_INFO, "OCA: Dumped RSDT (%u bytes) - %r\n", (UINT32) Context.Rsdt->Header.Length, Status));

  if (Context.Xsdt != NULL) {
    Status = AcpiDumpTable (Root, "XSDT", Context.Xsdt, Context.Xsdt->Header.Length);
    DEBUG ((DEBUG_INFO, "OCA: Dumped XSDT (%u bytes) - %r\n", (UINT32) Context.Xsdt->Header.Length, Status));
  }

  if (Context.Dsdt != NULL) {
    Status = AcpiDumpTable (Root, "DSDT", Context.Dsdt, Context.Dsdt->Length);
    DEBUG ((DEBUG_INFO, "OCA: Dumped DSDT (%u bytes) - %r\n", (UINT32) Context.Dsdt->Length, Status));
  }

  for (Index = 0; Index < Context.NumberOfTables; ++Index) {
    AcpiGetTableName (Root, Context.Tables[Index], Name, sizeof (Name));
    Status = AcpiDumpTable (Root, Name, Context.Tables[Index], Context.Tables[Index]->Length);
    DEBUG ((
      DEBUG_INFO,
      "OCA: Dumped table %a %u/%u (%u bytes) - %r\n",
      Name,
      (UINT32) (Index + 1),
      (UINT32) (Context.NumberOfTables),
      (UINT32) Context.Tables[Index]->Length,
      Status
      ));
  }

  AcpiFreeContext (&Context);
  return EFI_SUCCESS;
}

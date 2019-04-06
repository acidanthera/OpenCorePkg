/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAcpiLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVirtualFsLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC
VOID
OcAcpiAddTables (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_ACPI_CONTEXT     *Context
  )
{
  EFI_STATUS           Status;
  UINT8                *TableData;
  UINT32               TableDataLength;
  UINT32               Index;
  CONST CHAR8          *Table;
  CHAR16               FullPath[128];

  for (Index = 0; Index < Config->Acpi.Add.Count; ++Index) {
    Table = OC_BLOB_GET (Config->Acpi.Add.Values[Index]);

    if (Table[0] == '\0') {
      continue;
    }

    UnicodeSPrint (FullPath, sizeof (FullPath), OPEN_CORE_ACPI_PATH "%a", Table);
    UnicodeUefiSlashes (FullPath);

    TableData = OcStorageReadFileUnicode (Storage, FullPath, &TableDataLength);

    if (TableData == NULL) {
      DEBUG ((
        DEBUG_WARN,
        "OC: Failed to find ACPI %a\n",
        Table
        ));
      continue;
    }

    Status = AcpiInsertTable (Context, TableData, TableDataLength);

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "OC: Failed to add ACPI %a - %r\n",
        Table,
        Status
        ));
    }
  }
}

STATIC
VOID
OcAcpiBlockTables (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_ACPI_CONTEXT     *Context
  )
{
  EFI_STATUS           Status;
  UINT32               Index;
  UINT32               Signature;
  UINT64               OemTableId;
  OC_ACPI_BLOCK_ENTRY  *Table;

  for (Index = 0; Index < Config->Acpi.Block.Count; ++Index) {
    Table = Config->Acpi.Block.Values[Index];

    if (Table->Disabled) {
      continue;
    }

    CopyMem (&Signature, Table->TableSignature, sizeof (Table->TableSignature));
    CopyMem (&OemTableId, Table->OemTableId, sizeof (Table->OemTableId));

    Status = AcpiDropTable (
      Context,
      Signature,
      Table->TableLength,
      OemTableId
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "OC: Failed to drop ACPI %08X %016LX %u - %r\n",
        Signature,
        OemTableId,
        Table->TableLength,
        Status
        ));
    }
  }
}

STATIC
VOID
OcAcpiPatchTables (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_ACPI_CONTEXT     *Context
  )
{
  EFI_STATUS           Status;
  UINT32               Index;
  OC_ACPI_PATCH_ENTRY  *UserPatch;
  OC_ACPI_PATCH        Patch;

  for (Index = 0; Index < Config->Acpi.Patch.Count; ++Index) {
    UserPatch = Config->Acpi.Patch.Values[Index];

    if (UserPatch->Disabled) {
      continue;
    }

    //
    // Ignore patch if:
    // - There is nothing to replace.
    // - Find and replace mismatch in size.
    // - Mask and ReplaceMask mismatch in size when are available.
    //
    if (UserPatch->Replace.Size == 0
      || UserPatch->Find.Size != UserPatch->Replace.Size
      || (UserPatch->Mask.Size > 0 && UserPatch->Find.Size != UserPatch->Mask.Size)
      || (UserPatch->ReplaceMask.Size > 0 && UserPatch->Find.Size != UserPatch->ReplaceMask.Size)) {
      DEBUG ((DEBUG_ERROR, "OC: ACPI patch %u is borked\n", Index));
      continue;
    }

    ZeroMem (&Patch, sizeof (Patch));

    Patch.Find  = OC_BLOB_GET (&UserPatch->Find);
    Patch.Replace = OC_BLOB_GET (&UserPatch->Replace);

    if (UserPatch->Mask.Size > 0) {
      Patch.Mask  = OC_BLOB_GET (&UserPatch->Mask);
    }

    if (UserPatch->ReplaceMask.Size > 0) {
      Patch.ReplaceMask = OC_BLOB_GET (&UserPatch->ReplaceMask);
    }

    Patch.Size        = UserPatch->Replace.Size;
    Patch.Count       = UserPatch->Count;
    Patch.Skip        = UserPatch->Skip;
    Patch.Limit       = UserPatch->Limit;
    CopyMem (&Patch.TableSignature, UserPatch->TableSignature, sizeof (UserPatch->TableSignature));
    Patch.TableLength = UserPatch->TableLength;
    CopyMem (&Patch.OemTableId, UserPatch->OemTableId, sizeof (UserPatch->OemTableId));

    Status = AcpiApplyPatch (Context, &Patch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OC: ACPI patcher failed %u - %r\n", Index, Status));
    }
  }
}

VOID
OcLoadAcpiSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS        Status;
  OC_ACPI_CONTEXT   Context;

  Status = AcpiInitContext (&Context);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to initialize ACPI support - %r", Status));
    return;
  }

  if (Config->Acpi.Quirks.RebaseRegions) {
    AcpiLoadRegions (&Context);
  }

  OcAcpiBlockTables (Config, &Context);

  OcAcpiAddTables (Config, Storage, &Context);

  OcAcpiPatchTables (Config, &Context);

  if (Config->Acpi.Quirks.FadtEnableReset) {
    AcpiFadtEnableReset (&Context);
  }

  if (Config->Acpi.Quirks.RebaseRegions) {
    AcpiRelocateRegions (&Context);
  }

  if (Config->Acpi.Quirks.NormalizeHeaders) {
    AcpiNormalizeHeaders (&Context);
  }

  AcpiApplyContext (&Context);

  AcpiFreeContext (&Context);
}

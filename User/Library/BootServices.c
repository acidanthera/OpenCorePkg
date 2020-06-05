/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <BootServices.h>

EFI_BOOT_SERVICES mBootServices =
{
  .RaiseTPL       = DummyRaiseTPL,
  .LocateProtocol = DummyLocateProtocol,
  .AllocatePages  = DummyAllocatePages,
  .InstallConfigurationTable = DummyInstallConfigurationTable
};

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL mConOut = 
{
  .OutputString = NullTextOutputString
};

EFI_SYSTEM_TABLE mSystemTable =
{
  .BootServices = &mBootServices,
  .ConOut       = &mConOut
};

EFI_RUNTIME_SERVICES mRuntimeServices = {
  
};

EFI_SYSTEM_TABLE  *gST  = &mSystemTable;
EFI_BOOT_SERVICES *gBS  = &mBootServices;

EFI_HANDLE gImageHandle = (EFI_HANDLE)(UINTN) 0x12345;

BOOLEAN mPostEBS = FALSE;
EFI_SYSTEM_TABLE *mDebugST = &mSystemTable;

EFI_RUNTIME_SERVICES *gRT  = &mRuntimeServices;

#define CONFIG_TABLE_SIZE_INCREASED 0x10
UINTN mSystemTableAllocateSize = 0;

EFI_TPL
EFIAPI
DummyRaiseTPL (
  IN EFI_TPL      NewTpl
  )
{
  ASSERT (FALSE);
  return 0;
}

EFI_STATUS
DummyLocateProtocol (
  EFI_GUID        *ProtocolGuid,
  VOID            *Registration,
  VOID            **Interface
  )
{
  (VOID) ProtocolGuid;
  (VOID) Registration;
  (VOID) Interface;
  return EFI_NOT_FOUND;
}

EFI_STATUS
DummyAllocatePages (
  IN     EFI_ALLOCATE_TYPE            Type,
  IN     EFI_MEMORY_TYPE              MemoryType,
  IN     UINTN                        Pages,
  IN OUT EFI_PHYSICAL_ADDRESS         *Memory
  )
{
  *Memory = (UINTN) AllocatePages (Pages);

  return Memory != NULL ? EFI_SUCCESS : EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
DummyInstallConfigurationTable (
  IN EFI_GUID *Guid,
  IN VOID     *Table
  )
{
  UINTN                   Index;
  EFI_CONFIGURATION_TABLE *EfiConfigurationTable;
  EFI_CONFIGURATION_TABLE *OldTable;

  //
  // If Guid is NULL, then this operation cannot be performed
  //
  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiConfigurationTable = gST->ConfigurationTable;

  //
  // Search all the table for an entry that matches Guid
  //
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (Guid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      break;
    }
  }

  if (Index < gST->NumberOfTableEntries) {
    //
    // A match was found, so this is either a modify or a delete operation
    //
    if (Table != NULL) {
      //
      // If Table is not NULL, then this is a modify operation.
      // Modify the table entry and return.
      //
      gST->ConfigurationTable[Index].VendorTable = Table;

      return EFI_SUCCESS;
    }

    //
    // A match was found and Table is NULL, so this is a delete operation.
    //
    gST->NumberOfTableEntries--;

    //
    // Copy over deleted entry
    //
    CopyMem (
      &(EfiConfigurationTable[Index]),
      &(gST->ConfigurationTable[Index + 1]),
      (gST->NumberOfTableEntries - Index) * sizeof (EFI_CONFIGURATION_TABLE)
      );

  } else {

    //
    // No matching GUIDs were found, so this is an add operation.
    //

    if (Table == NULL) {
      //
      // If Table is NULL on an add operation, then return an error.
      //
      return EFI_NOT_FOUND;
    }

    //
    // Assume that Index == gST->NumberOfTableEntries
    //
    if ((Index * sizeof (EFI_CONFIGURATION_TABLE)) >= mSystemTableAllocateSize) {
      //
      // Allocate a table with one additional entry.
      //
      mSystemTableAllocateSize += (CONFIG_TABLE_SIZE_INCREASED * sizeof (EFI_CONFIGURATION_TABLE));
      EfiConfigurationTable = AllocatePool (mSystemTableAllocateSize);
      if (EfiConfigurationTable == NULL) {
        //
        // If a new table could not be allocated, then return an error.
        //
        return EFI_OUT_OF_RESOURCES;
      }

      if (gST->ConfigurationTable != NULL) {
        //
        // Copy the old table to the new table.
        //
        CopyMem (
          EfiConfigurationTable,
          gST->ConfigurationTable,
          Index * sizeof (EFI_CONFIGURATION_TABLE)
          );

        //
        // Record the old table pointer.
        //
        OldTable = gST->ConfigurationTable;

        //
        // As the CoreInstallConfigurationTable() may be re-entered by CoreFreePool()
        // in its calling stack, updating System table to the new table pointer must
        // be done before calling CoreFreePool() to free the old table.
        // It can make sure the gST->ConfigurationTable point to the new table
        // and avoid the errors of use-after-free to the old table by the reenter of
        // CoreInstallConfigurationTable() in CoreFreePool()'s calling stack.
        //
        gST->ConfigurationTable = EfiConfigurationTable;

        //
        // Free the old table after updating System Table to the new table pointer.
        //
        FreePool (OldTable);
      } else {
        //
        // Update System Table
        //
        gST->ConfigurationTable = EfiConfigurationTable;
      }
    }

    //
    // Fill in the new entry
    //
    CopyGuid ((VOID *)&EfiConfigurationTable[Index].VendorGuid, Guid);
    EfiConfigurationTable[Index].VendorTable  = Table;

    //
    // This is an add operation, so increment the number of table entries
    //
    gST->NumberOfTableEntries++;
  }

  //
  // Fix up the CRC-32 in the EFI System Table
  //
  // CalculateEfiHdrCrc (&gST->Hdr);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NullTextOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN CHAR16                          *String
  )
{
  while (*String) {
    if (*String != '\r') {
      printf("%c", (char) *String);
    }

    ++String;
  }
  return EFI_SUCCESS;
}

/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserBootServices.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

EFI_BOOT_SERVICES  mBootServices = {
  .RaiseTPL                  = DummyRaiseTPL,
  .RestoreTPL                = DummyRestoreTPL,
  .LocateProtocol            = DummyLocateProtocol,
  .AllocatePages             = DummyAllocatePages,
  .InstallConfigurationTable = DummyInstallConfigurationTable,
  .CalculateCrc32            = DummyCalculateCrc32
};

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  mConOut = {
  .OutputString = NullTextOutputString
};

EFI_SYSTEM_TABLE  mSystemTable = {
  .BootServices = &mBootServices,
  .ConOut       = &mConOut
};

EFI_RUNTIME_SERVICES  mRuntimeServices = {
  .Hdr     = { 0 },
  .GetTime = DummyGetTime,
};

EFI_SYSTEM_TABLE   *gST = &mSystemTable;
EFI_BOOT_SERVICES  *gBS = &mBootServices;

EFI_HANDLE  gImageHandle = (EFI_HANDLE)(UINTN)0xDEADBABEULL;

BOOLEAN           mPostEBS  = FALSE;
EFI_SYSTEM_TABLE  *mDebugST = &mSystemTable;

EFI_RUNTIME_SERVICES  *gRT = &mRuntimeServices;

#define CONFIG_TABLE_SIZE_INCREASED  0x10
UINTN  mSystemTableAllocateSize = 0ULL;

EFI_TPL
EFIAPI
DummyRaiseTPL (
  IN EFI_TPL  NewTpl
  )
{
  return 0;
}

VOID
EFIAPI
DummyRestoreTPL (
  IN EFI_TPL  NewTpl
  )
{
  return;
}

EFI_STATUS
EFIAPI
DummyLocateProtocol (
  IN  EFI_GUID *Protocol,
  IN  VOID *Registration, OPTIONAL
  OUT VOID      **Interface
  )
{
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
DummyAllocatePages (
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 Pages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  *Memory = (UINTN)AllocatePages (Pages);

  return Memory != NULL ? EFI_SUCCESS : EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
DummyInstallConfigurationTable (
  IN EFI_GUID  *Guid,
  IN VOID      *Table
  )
{
  //
  // NOTE: This code is adapted from original EDK II implementations.
  //

  UINTN                    Index;
  EFI_CONFIGURATION_TABLE  *EfiConfigurationTable;
  EFI_CONFIGURATION_TABLE  *OldTable;

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
      EfiConfigurationTable     = AllocatePool (mSystemTableAllocateSize);
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
    EfiConfigurationTable[Index].VendorTable = Table;

    //
    // This is an add operation, so increment the number of table entries
    //
    gST->NumberOfTableEntries++;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DummyCalculateCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  )
{
  if ((Data == NULL) || (DataSize == 0) || (CrcOut == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *CrcOut = CalculateCrc32 (Data, DataSize);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NullTextOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  )
{
  while (*String != '\0') {
    if (*String != '\r') {
      //
      // Cast string to CHAR8 to truncate unicode symbols.
      //
      putchar ((CHAR8)*String);
    }

    ++String;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DummyGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  time_t     EpochTime;
  struct tm  *TimeInfo;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EpochTime = time (NULL);
  TimeInfo  = localtime (&EpochTime);

  if (TimeInfo == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Time->TimeZone = EFI_UNSPECIFIED_TIMEZONE;
  Time->Daylight = 0;
  Time->Second   = (UINT8)TimeInfo->tm_sec;
  Time->Minute   = (UINT8)TimeInfo->tm_min;
  Time->Hour     = (UINT8)TimeInfo->tm_hour;
  Time->Day      = (UINT8)TimeInfo->tm_mday;
  //
  // The EFI_TIME Month field count months from 1 to 12,
  // while tm_mon counts from 0 to 11
  //
  Time->Month = (UINT8)(TimeInfo->tm_mon + 1);
  //
  // According ISO/IEC 9899:1999 7.23.1 the tm_year field
  // contains number of years since 1900
  //
  Time->Year = (UINT16)(TimeInfo->tm_year + 1900);

  if (Capabilities) {
    Capabilities->Accuracy   = 0;
    Capabilities->Resolution = 1;
    Capabilities->SetsToZero = FALSE;
  }

  return EFI_SUCCESS;
}

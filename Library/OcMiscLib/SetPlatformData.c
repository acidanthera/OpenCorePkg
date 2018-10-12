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

#include <IndustryStandard/Pci.h>

#include <Guid/ApplePlatformInfo.h>

#include <Protocol/DataHub.h>
#include <Protocol/LegacyRegion.h>
#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcPrintLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Macros.h>

// SetPlatformData
/**

  @param[in] DataRecordGuid  The guid of the record to use.
  @param[in] Key             A pointer to the ascii key string.
  @param[in] Data            A pointer to the data to store.
  @param[in] DataSize        The length of the data to store.

  @retval EFI_SUCCESS  The datahub  was updated successfully.
**/
EFI_STATUS
SetPlatformData (
  IN EFI_GUID  *DataRecordGuid,
  IN CHAR8     *Key,
  IN VOID      *Data,
  IN UINT32    DataSize
  )
{
  EFI_STATUS            Status;

  EFI_DATA_HUB_PROTOCOL *DataHub;
  PLATFORM_DATA_HEADER  *Entry;
  CHAR8                 *DataString;
  UINT32                 KeySize;

  // Locate DataHub
  DataHub = NULL;
  Status  = gBS->LocateProtocol (
                   &gEfiDataHubProtocolGuid,
                   NULL,
                   (VOID **)&DataHub
                   );

  if (!EFI_ERROR (Status)) {
    KeySize   = (UINT32)(AsciiStrSize (Key) * (sizeof (CHAR16)));
    Entry     = AllocateZeroPool (DataSize + sizeof (*Entry) + KeySize);
    Status    = EFI_OUT_OF_RESOURCES;

    if (Entry) {

      Entry->KeySize  = KeySize;
      Entry->DataSize = DataSize;
      DataString          = NULL;

      OcAsciiStrToUnicode (Key, (CHAR16 *)(((UINTN)Entry) + sizeof (*Entry)), 0);

      CopyMem ((VOID *)(((UINTN)Entry) + sizeof (*Entry) + Entry->KeySize), Data, (UINTN)Entry->DataSize);

      Status = DataHub->LogData (
                          DataHub,
                          DataRecordGuid,
                          &gApplePlatformProducerNameGuid,
                          EFI_DATA_RECORD_CLASS_DATA,
                          Entry,
                          (sizeof (*Entry) + Entry->KeySize + Entry->DataSize)
                          );

      if (DataSize < 32) {
        DataString = ConvertDataToString (Data, DataSize);
      }

      if (DataString != NULL) {
        DEBUG ((
          DEBUG_INFO,
          "Setting DataHub %g:%a = %a (%d)\n",
          &gApplePlatformProducerNameGuid,
          Key,
          DataString,
          DataSize
          ));

        FreePool ((VOID *)DataString);
      } else {
        DEBUG ((
          DEBUG_INFO,
          "Setting DataHub %g:%a (%d)\n",
          &gApplePlatformProducerNameGuid,
          Key,
          DataSize
          ));
      }

      FreePool ((VOID *)Entry);
    }
  }

  return Status;
}

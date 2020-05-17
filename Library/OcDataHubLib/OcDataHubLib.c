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

#include <IndustryStandard/ProcessorInfo.h>
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
#include <Library/OcDataHubLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Driver's Entry point routine that install Driver to produce Data Hub protocol.

**/
EFI_DATA_HUB_PROTOCOL *
DataHubInstall (
  VOID
  );

EFI_DATA_HUB_PROTOCOL *
OcDataHubInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS             Status;
  EFI_DATA_HUB_PROTOCOL  *DataHub;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gEfiDataHubProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCDH: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status  = gBS->LocateProtocol (
      &gEfiDataHubProtocolGuid,
      NULL,
      (VOID **) &DataHub
      );

    if (!EFI_ERROR (Status)) {
      return DataHub;
    }
  }

  return DataHubInstall ();
}

STATIC
EFI_STATUS
DataHubSetAppleMiscAscii (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN CONST CHAR16           *Key,
  IN CONST CHAR8            *Value
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (Value != NULL) {
    Status = SetDataHubEntry (
      DataHub,
      &gEfiMiscSubClassGuid,
      Key,
      Value,
      (UINT32)AsciiStrSize (Value)
      );
  }

  return Status;
}

STATIC
EFI_STATUS
DataHubSetAppleMiscUnicode (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN CONST CHAR16           *Key,
  IN CONST CHAR8            *Value
  )
{
  EFI_STATUS  Status;
  CHAR16      *UnicodeValue;

  Status = EFI_SUCCESS;

  if (Value != NULL) {
    UnicodeValue = AsciiStrCopyToUnicode (Value, 0);
    if (UnicodeValue == NULL) {
      DEBUG ((DEBUG_WARN, "OCDH: Data Hub failed to allocate %s\n", Key));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SetDataHubEntry (
      DataHub,
      &gEfiMiscSubClassGuid,
      Key,
      UnicodeValue,
      (UINT32)StrSize (UnicodeValue)
      );

    FreePool (UnicodeValue);
  }

  return Status;
}

STATIC
EFI_STATUS
DataHubSetAppleMiscData (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN CONST CHAR16           *Key,
  IN CONST VOID             *Value,
  IN UINT32                 Size
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (Value != NULL) {
    Status = SetDataHubEntry (
      DataHub,
      &gEfiMiscSubClassGuid,
      Key,
      Value,
      Size
      );
  }

  return Status;
}

STATIC
EFI_STATUS
DataHubSetAppleProcessorData (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN CONST CHAR16           *Key,
  IN CONST VOID             *Value,
  IN UINT32                 Size
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (Value != NULL) {
    Status = SetDataHubEntry (
      DataHub,
      &gEfiProcessorSubClassGuid,
      Key,
      Value,
      Size
      );
  }

  return Status;
}

EFI_STATUS
SetDataHubEntry (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN EFI_GUID               *DataRecordGuid,
  IN CONST CHAR16           *Key,
  IN CONST VOID             *Data,
  IN UINT32                 DataSize
  )
{
  EFI_STATUS                  Status;
  
  APPLE_PLATFORM_DATA_RECORD  *Entry;
  UINT32                      KeySize;
  UINT32                      TotalSize;
  BOOLEAN                     Result;

  KeySize = (UINT32) StrSize (Key);
  Result  = OcOverflowTriAddU32 (
              sizeof (*Entry),
              KeySize,
              DataSize,
              &TotalSize
              );
  if (Result) {
    return EFI_INVALID_PARAMETER;
  }

  Entry = AllocateZeroPool (TotalSize);

  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // TODO: We may want to fill header some day.
  // Currently it is not important.
  //
  Entry->KeySize   = KeySize;
  Entry->ValueSize = DataSize;

  CopyMem (&Entry->Data[0], Key, KeySize);
  CopyMem (&Entry->Data[Entry->KeySize], Data, DataSize);

  Status = DataHub->LogData (
    DataHub,
    DataRecordGuid,
    &gApplePlatformProducerNameGuid,
    EFI_DATA_RECORD_CLASS_DATA,
    Entry,
    TotalSize
    );

  DEBUG ((
    EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
    "OCDH: Setting DataHub %g:%s (%u) - %r\n",
    &gApplePlatformProducerNameGuid,
    Key,
    DataSize,
    Status
    ));

  FreePool (Entry);

  return Status;
}

EFI_STATUS
UpdateDataHub (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN OC_DATA_HUB_DATA       *Data,
  IN OC_CPU_INFO            *CpuInfo
  )
{
  GUID                   SystemId;

  DataHubSetAppleMiscAscii (DataHub, OC_PLATFORM_NAME, Data->PlatformName);
  DataHubSetAppleMiscUnicode (DataHub, OC_SYSTEM_PRODUCT_NAME, Data->SystemProductName);
  DataHubSetAppleMiscUnicode (DataHub, OC_SYSTEM_SERIAL_NUMBER, Data->SystemSerialNumber);
  if (Data->SystemUUID != NULL) {
    //
    // Byte order for SystemId must be swapped.
    //
    CopyGuid (&SystemId, Data->SystemUUID);
    SystemId.Data1 = SwapBytes32 (SystemId.Data1);
    SystemId.Data2 = SwapBytes16 (SystemId.Data2);
    SystemId.Data3 = SwapBytes16 (SystemId.Data3);
    DataHubSetAppleMiscData (DataHub, OC_SYSTEM_UUID, &SystemId, sizeof (SystemId));
  }
  DataHubSetAppleMiscAscii (DataHub, OC_BOARD_PRODUCT, Data->BoardProduct);
  DataHubSetAppleMiscData (DataHub, OC_BOARD_REVISION, Data->BoardRevision, sizeof (*Data->BoardRevision));
  DataHubSetAppleMiscData (DataHub, OC_STARTUP_POWER_EVENTS, Data->StartupPowerEvents, sizeof (*Data->StartupPowerEvents));
  DataHubSetAppleProcessorData (DataHub, OC_INITIAL_TSC, Data->InitialTSC, sizeof (*Data->InitialTSC));
  if (Data->FSBFrequency == NULL) {
    Data->FSBFrequency = &CpuInfo->FSBFrequency;
  }
  DataHubSetAppleProcessorData (DataHub, OC_FSB_FREQUENCY, Data->FSBFrequency, sizeof (*Data->FSBFrequency));
  if (Data->ARTFrequency == NULL && CpuInfo->ARTFrequency > 0 && CpuInfo->ARTFrequency != DEFAULT_ART_CLOCK_SOURCE) {
    Data->ARTFrequency = &CpuInfo->ARTFrequency;
  }
  DataHubSetAppleProcessorData (DataHub, OC_ART_FREQUENCY, Data->ARTFrequency, sizeof (*Data->ARTFrequency));
  DataHubSetAppleMiscData (DataHub, OC_DEVICE_PATHS_SUPPORTED, Data->DevicePathsSupported, sizeof (*Data->DevicePathsSupported));
  DataHubSetAppleMiscData (DataHub, OC_SMC_REVISION, Data->SmcRevision, OC_SMC_REVISION_SIZE);
  DataHubSetAppleMiscData (DataHub, OC_SMC_BRANCH, Data->SmcBranch, OC_SMC_BRANCH_SIZE);
  DataHubSetAppleMiscData (DataHub, OC_SMC_PLATFORM, Data->SmcPlatform, OC_SMC_PLATFORM_SIZE);

  return EFI_SUCCESS;
}

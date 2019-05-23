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

#ifndef OC_DATA_HUB_LIB_H
#define OC_DATA_HUB_LIB_H

#include <Guid/AppleDataHub.h>
#include <Library/OcCpuLib.h>
#include <Protocol/DataHub.h>

#define OC_PLATFORM_NAME           L"name"
#define OC_SYSTEM_PRODUCT_NAME     L"Model"
#define OC_SYSTEM_SERIAL_NUMBER    L"SystemSerialNumber"
#define OC_SYSTEM_UUID             L"system-id"
#define OC_BOARD_PRODUCT           L"board-id"
#define OC_BOARD_REVISION          L"board-rev"
#define OC_STARTUP_POWER_EVENTS    L"StartupPowerEvents"
#define OC_INITIAL_TSC             L"InitialTSC"
#define OC_FSB_FREQUENCY           L"FSBFrequency"
#define OC_ART_FREQUENCY           L"ARTFrequency"
#define OC_DEVICE_PATHS_SUPPORTED  L"DevicePathsSupported"

//
// These are custom and match VirtualSMC, FakeSMC, and Clover.
//
#define OC_SMC_REVISION            L"REV"
#define OC_SMC_BRANCH              L"RBr"
#define OC_SMC_PLATFORM            L"RPlt"

#define OC_SMC_REVISION_SIZE         6U
#define OC_SMC_BRANCH_SIZE           8U
#define OC_SMC_PLATFORM_SIZE         8U

typedef struct {
  CONST CHAR8   *PlatformName;
  CONST CHAR8   *SystemProductName;
  CONST CHAR8   *SystemSerialNumber;
  CONST GUID    *SystemUUID;
  CONST CHAR8   *BoardProduct;
  CONST UINT8   *BoardRevision;
  CONST UINT64  *StartupPowerEvents;
  CONST UINT64  *InitialTSC;
  CONST UINT64  *FSBFrequency;
  CONST UINT64  *ARTFrequency;
  CONST UINT32  *DevicePathsSupported;
  CONST UINT8   *SmcRevision;
  CONST UINT8   *SmcBranch;
  CONST UINT8   *SmcPlatform;
} OC_DATA_HUB_DATA;

/**
  Locate Data Hub protocol.

  @param[in] Reinstall       Force local Data Hub instance.

  @retval Data Hub protocol instance or NULL.
**/
EFI_DATA_HUB_PROTOCOL *
OcDataHubInstallProtocol (
  IN BOOLEAN  Reinstall
  );

/**
  Set Data Hub entry.

  @param[in] DataHub         Data Hub protocol instance.
  @param[in] DataRecordGuid  The guid of the record to use.
  @param[in] Key             A pointer to the unicode key string.
  @param[in] Data            A pointer to the data to store.
  @param[in] DataSize        The length of the data to store.

  @retval EFI_SUCCESS  The datahub  was updated successfully.
**/
EFI_STATUS
SetDataHubEntry (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN EFI_GUID               *DataRecordGuid,
  IN CONST CHAR16           *Key,
  IN CONST VOID             *Data,
  IN UINT32                 DataSize
  );

/**
  Update DataHub entries.

  @param[in] Data         Data to be used for updating.

  @retval EFI_SUCCESS  The datahub  was updated successfully.
**/
EFI_STATUS
UpdateDataHub (
  IN EFI_DATA_HUB_PROTOCOL  *DataHub,
  IN OC_DATA_HUB_DATA       *Data,
  IN OC_CPU_INFO            *CpuInfo
  );

#endif // OC_DATA_HUB_LIB_H

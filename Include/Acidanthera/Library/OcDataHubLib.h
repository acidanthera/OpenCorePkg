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
#define OC_COPROCESSOR_VERSION     L"apple-coprocessor-version"
#define OC_CHASSIS_TYPE            L"chassis-type"
#define OC_DEVICE_COLORS           L"device-colors"

//
// These are custom and match VirtualSMC, FakeSMC, and Clover.
//
#define OC_SMC_REVISION  L"REV"
#define OC_SMC_BRANCH    L"RBr"
#define OC_SMC_PLATFORM  L"RPlt"

#define OC_SMC_REVISION_SIZE  6U
#define OC_SMC_BRANCH_SIZE    8U
#define OC_SMC_PLATFORM_SIZE  8U

//
// Specifies the icon for MacPro7,1 SMBIOS.
// Tower: 225, 225, 223
// Rack:  226, 226, 224
// TODO: Maybe Apple uses it somewhere else?
//
#pragma pack (push, 1)
typedef struct {
  UINT16    Unknown; ///< Apple sets to 0x0200
  UINT16    Reserved0;
  UINT8     Blue;
  UINT8     Green;
  UINT8     Red;
  UINT8     Reserved1;
  UINT8     Reserved2[8];
} OC_DEVICE_COLORS_CONTEXT;
#pragma pack (pop)

STATIC_ASSERT (sizeof (OC_DEVICE_COLORS_CONTEXT) == 16, "Invalid OC_DEVICE_COLORS_CONTEXT size");

typedef struct {
  CONST CHAR8     *PlatformName;
  CONST CHAR8     *SystemProductName;
  CONST CHAR8     *SystemSerialNumber;
  CONST GUID      *SystemUUID;
  CONST CHAR8     *BoardProduct;
  CONST UINT8     *BoardRevision;
  CONST UINT64    *StartupPowerEvents;
  CONST UINT64    *InitialTSC;
  CONST UINT64    *FSBFrequency;
  CONST UINT64    *ARTFrequency;
  CONST UINT32    *DevicePathsSupported;
  CONST UINT8     *SmcRevision;
  CONST UINT8     *SmcBranch;
  CONST UINT8     *SmcPlatform;
  CONST UINT32    *CoprocessorVersion;
  CONST CHAR8     *ChassisType;
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

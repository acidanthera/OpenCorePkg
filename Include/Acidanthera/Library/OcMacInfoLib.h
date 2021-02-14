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

#ifndef OC_MAC_INFO_LIB_H
#define OC_MAC_INFO_LIB_H

//
// Maximum characters for valid Mac-like product name.
//
#define OC_OEM_NAME_MAX 48

//
// Maximum characters for valid Mac-like serial name.
//
#define OC_OEM_SERIAL_MAX 24

//
// Maximum characters for ROM address.
//
#define OC_OEM_ROM_MAX 6

typedef struct MAC_INFO_DATA_SMBIOS_ {
  //
  // Type 0
  //
  CONST CHAR8     *BIOSVersion;
  CONST CHAR8     *BIOSReleaseDate;
  //
  // Type 1
  //
  CONST CHAR8     *SystemProductName;
  CONST CHAR8     *SystemVersion;
  CONST CHAR8     *SystemSKUNumber;
  CONST CHAR8     *SystemFamily;
  //
  // Type 2
  //
  CONST CHAR8     *BoardProduct;
  CONST CHAR8     *BoardVersion;
  CONST CHAR8     *BoardAssetTag;
  CONST CHAR8     *BoardLocationInChassis;
  CONST UINT8     *BoardType;
  //
  // Type 3
  //
  CONST UINT8     *ChassisType;
  CONST CHAR8     *ChassisVersion;
  CONST CHAR8     *ChassisAssetTag;
  //
  // Type 17
  //
  CONST UINT8     *MemoryFormFactor;
  //
  // Type 128
  // FirmwareFeatures and FirmwareFeaturesMask are split into two UINT32
  // values, the lower referring to the traditional FirmwareFeatures and the
  // upper representing the additions introduced by ExtendedFirmwareFeatures.
  //
  UINT64          FirmwareFeatures;
  UINT64          FirmwareFeaturesMask;
  //
  // Type 133
  //
  CONST UINT32    *PlatformFeature;
} MAC_INFO_DATA_SMBIOS;

typedef struct MAC_INFO_DATA_DATAHUB_ {
  CONST CHAR8   *PlatformName;
  CONST CHAR8   *SystemProductName;
  CONST CHAR8   *BoardProduct;
  CONST UINT8   *BoardRevision;
  CONST UINT32  *DevicePathsSupported;
  CONST UINT8   *SmcRevision;
  CONST UINT8   *SmcBranch;
  CONST UINT8   *SmcPlatform;
} MAC_INFO_DATA_DATAHUB;

typedef struct MAC_INFO_DATA_OEM_ {
  CHAR8         SystemSerialNumber[OC_OEM_SERIAL_MAX];
  CHAR8         Mlb[OC_OEM_SERIAL_MAX];
  UINT8         Rom[OC_OEM_ROM_MAX];
  EFI_GUID      SystemUuid;
} MAC_INFO_DATA_OEM;

typedef struct MAC_INFO_DATA_ {
  //
  // DataHub data.
  //
  MAC_INFO_DATA_DATAHUB DataHub;
  //
  // SMBIOS data.
  //
  MAC_INFO_DATA_SMBIOS  Smbios;
  //
  // Serial data.
  //
  MAC_INFO_DATA_OEM     Oem;
} MAC_INFO_DATA;

/**
  Obtain Mac info based on product name.
  When exact match is not possible, any sane data may be returned.

  @param[in]  ProductName  Product to get information for.
  @param[out] MacInfo      Information data to fill.
**/
VOID
GetMacInfo (
  IN  CONST CHAR8    *ProductName,
  OUT MAC_INFO_DATA  *MacInfo
  );

/**
  Determine if specified product name is a real Mac model.

  @param[in] ProductName   Product to check information for.

  @retval TRUE if ProductName is a real Mac model.
**/
BOOLEAN
HasMacInfo (
  IN CONST CHAR8     *ProductName
  );

/**
  Determine if specified model and kernel version can
  run in 64-bit kernel mode.

  @param[in] ProductName        Product to get information for.
  @param[in] KernelVersion      Kernel version.

  @retval TRUE if supported.
**/
BOOLEAN
IsMacModel64BitCompatible (
  IN CONST CHAR8    *ProductName,
  IN UINT32         KernelVersion
  );

#endif // OC_MAC_INFO_LIB_H

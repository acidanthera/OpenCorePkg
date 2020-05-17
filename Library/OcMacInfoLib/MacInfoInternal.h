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

#ifndef MAC_INFO_INTERNAL_H
#define MAC_INFO_INTERNAL_H

#define MAC_INFO_PLATFORM_FEATURE_MISSING 0xFFFFFFFFU

#define MAC_INFO_BOARD_REVISION_MISSING   0xFFU

typedef struct {
  CONST CHAR8 * CONST SystemProductName;
  CONST CHAR8 * CONST BoardProduct;
  CONST UINT8         BoardRevision;
  CONST UINT8         SmcRevision[6];
  CONST UINT8         SmcBranch[8];
  CONST UINT8         SmcPlatform[8];
  CONST CHAR8 * CONST BIOSVersion;
  CONST CHAR8 * CONST BIOSReleaseDate;
  CONST CHAR8 * CONST SystemVersion;
  CONST CHAR8 * CONST SystemSKUNumber;
  CONST CHAR8 * CONST SystemFamily;
  CONST CHAR8 * CONST BoardVersion;
  CONST CHAR8 * CONST BoardAssetTag;
  CONST CHAR8 * CONST BoardLocationInChassis;
  CONST UINT8         SmcGeneration;
  CONST UINT8         BoardType;
  CONST UINT8         ChassisType;
  CONST UINT8         MemoryFormFactor;
  CONST UINT32        PlatformFeature;
  CONST CHAR8 * CONST ChassisAssetTag;
  CONST UINT64        FirmwareFeatures;
  CONST UINT64        FirmwareFeaturesMask;
} MAC_INFO_INTERNAL_ENTRY;

/**
  Statically compiled array with model entries.
  The array is sorted by ProductName field.
**/
extern CONST MAC_INFO_INTERNAL_ENTRY gMacInfoModels[];

/**
  Entry count in statically compiled array with model entries.
**/
extern CONST UINTN gMacInfoModelCount;

/**
  Default entry in a statically compiled array with model entries.
**/
extern CONST UINTN gMacInfoDefaultModel;

#endif // MAC_INFO_INTERNAL_H

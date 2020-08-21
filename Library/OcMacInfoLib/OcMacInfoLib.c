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

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMacInfoLib.h>

#include "MacInfoInternal.h"

STATIC CONST UINT32 mDevicePathsSupported = 1;

STATIC
CONST MAC_INFO_64BIT_COMPAT_ENTRY Mac64BitModels[] = {
  {
    "Macmini", 0, 3
  },
  {
    "MacBook", 0, 5
  },
  {
    "MacBookAir", 0, 2
  },
  {
    "MacBookPro", 4, 3
  },
  {
    "iMac", 8, 7
  },
  {
    "MacPro", 3, 3
  },
  {
    "Xserve", 2, 2
  }
};

STATIC
CONST MAC_INFO_INTERNAL_ENTRY *
LookupInternalEntry (
  CONST CHAR8    *ProductName
  )
{
  UINTN  Start;
  UINTN  End;
  UINTN  Curr;
  INTN   Cmp;

  ASSERT (gMacInfoModelCount > 0);
  ASSERT (gMacInfoDefaultModel < gMacInfoModelCount);

  //
  // Classic binary search in a sorted string list.
  //
  Start = 0;
  End   = gMacInfoModelCount - 1;

  while (Start <= End) {
    Curr = (Start + End) / 2;
    Cmp = AsciiStrCmp (gMacInfoModels[Curr].SystemProductName, ProductName);

    if (Cmp == 0) {
      return &gMacInfoModels[Curr];
    } else if (Cmp < 0) {
      Start = Curr + 1;
    } else if (Curr > 0) {
      End = Curr - 1;
    } else {
      //
      // Even the first element does not match, required due to unsigned End.
      //
      return &gMacInfoModels[gMacInfoDefaultModel];
    }
  }

  return &gMacInfoModels[gMacInfoDefaultModel];
}

VOID
GetMacInfo (
  IN CONST CHAR8     *ProductName,
  OUT MAC_INFO_DATA  *MacInfo
  )
{
  CONST MAC_INFO_INTERNAL_ENTRY  *InternalEntry;

  ZeroMem (MacInfo, sizeof (*MacInfo));

  InternalEntry = LookupInternalEntry (ProductName);

  //
  // Fill in DataHub values.
  //
  MacInfo->DataHub.PlatformName          = "platform";
  MacInfo->DataHub.SystemProductName     = ProductName;
  MacInfo->DataHub.BoardProduct          = InternalEntry->BoardProduct;
  if (InternalEntry->BoardRevision != MAC_INFO_BOARD_REVISION_MISSING) {
    MacInfo->DataHub.BoardRevision       = &InternalEntry->BoardRevision;
  }
  MacInfo->DataHub.DevicePathsSupported  = &mDevicePathsSupported;
  //
  // T2-based macs have no SMC branch or revision.
  //
  if (InternalEntry->SmcGeneration < 3) {
    MacInfo->DataHub.SmcRevision         = InternalEntry->SmcRevision;
    MacInfo->DataHub.SmcBranch           = InternalEntry->SmcBranch;
  }
  MacInfo->DataHub.SmcPlatform           = InternalEntry->SmcPlatform;

  MacInfo->Smbios.BIOSVersion            = InternalEntry->BIOSVersion;
  MacInfo->Smbios.BIOSReleaseDate        = InternalEntry->BIOSReleaseDate;
  MacInfo->Smbios.SystemProductName      = ProductName;
  MacInfo->Smbios.SystemVersion          = InternalEntry->SystemVersion;
  MacInfo->Smbios.SystemSKUNumber        = InternalEntry->SystemSKUNumber;
  MacInfo->Smbios.SystemFamily           = InternalEntry->SystemFamily;
  MacInfo->Smbios.BoardProduct           = InternalEntry->BoardProduct;
  MacInfo->Smbios.BoardVersion           = InternalEntry->BoardVersion;
  MacInfo->Smbios.BoardAssetTag          = InternalEntry->BoardAssetTag;
  MacInfo->Smbios.BoardLocationInChassis = InternalEntry->BoardLocationInChassis;
  MacInfo->Smbios.BoardType              = &InternalEntry->BoardType;
  MacInfo->Smbios.ChassisType            = &InternalEntry->ChassisType;
  MacInfo->Smbios.ChassisVersion         = InternalEntry->BoardProduct;
  MacInfo->Smbios.ChassisAssetTag        = InternalEntry->ChassisAssetTag;
  MacInfo->Smbios.MemoryFormFactor       = &InternalEntry->MemoryFormFactor;
  MacInfo->Smbios.FirmwareFeatures       = InternalEntry->FirmwareFeatures;
  MacInfo->Smbios.FirmwareFeaturesMask   = InternalEntry->FirmwareFeaturesMask;

  if (InternalEntry->PlatformFeature != MAC_INFO_PLATFORM_FEATURE_MISSING) {
    MacInfo->Smbios.PlatformFeature      = &InternalEntry->PlatformFeature;
  }
}

BOOLEAN
IsMacModel64BitCompatible (
  IN CONST CHAR8    *ProductName,
  IN UINT32         KernelVersion
  )
{
  EFI_STATUS                      Status;
  UINT32                          Index;
  UINTN                           CurrentModelLength;
  UINTN                           SystemModelLength;
  CONST CHAR8                     *SystemModelSuffix;
  CHAR8                           *SystemModelSeparator;
  UINT64                          SystemModelMajor;

  ASSERT (ProductName != NULL);

  //
  // <= 10.5 is always 32-bit, and >= 10.8 is always 64-bit.
  //
  if (OcMatchDarwinVersion (KernelVersion, 0, KERNEL_VERSION_LEOPARD_MAX)) {
    return FALSE;
  } else if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOUNTAIN_LION_MIN, 0)) {
    return TRUE;
  }

  SystemModelLength = AsciiStrLen (ProductName);

  for (Index = 0; Index < sizeof (Mac64BitModels); Index++) {
    CurrentModelLength = AsciiStrLen (Mac64BitModels[Index].ModelName);
    if (SystemModelLength <= CurrentModelLength) {
      continue;
    }

    if (AsciiStrnCmp (ProductName, Mac64BitModels[Index].ModelName, CurrentModelLength) == 0) {
      SystemModelSuffix = &ProductName[CurrentModelLength];

      SystemModelSeparator = AsciiStrStr (SystemModelSuffix, ",");
      Status = AsciiStrDecimalToUint64S (SystemModelSuffix, &SystemModelSeparator, &SystemModelMajor);
      if (!EFI_ERROR (Status)) {
        if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_SNOW_LEOPARD_MIN, KERNEL_VERSION_SNOW_LEOPARD_MAX)) {
          return Mac64BitModels[Index].SnowLeoMin64 != 0 && SystemModelMajor >= Mac64BitModels[Index].SnowLeoMin64;
        } else if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LION_MIN, KERNEL_VERSION_LION_MAX)) {
          return Mac64BitModels[Index].LionMin64 != 0 && SystemModelMajor >= Mac64BitModels[Index].LionMin64;
        }
      }
    }
  }

  return FALSE;
}

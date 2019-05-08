/** @file

OcSmbiosLib

Copyright (c) 2019, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_SMBIOS_LIB_H
#define OC_SMBIOS_LIB_H

#include <Library/OcCpuLib.h>

//
// This GUID is used for storing SMBIOS data when the firmware overwrites SMBIOS data at original
// GUID at ExitBootServices, like it happens on some Dell computers.
// Found by David Passmore. Guid matches syscl's implementation in Clover.
// See: https://sourceforge.net/p/cloverefiboot/tickets/203/#c070
//
#define OC_CUSTOM_SMBIOS_TABLE_GUID \
  { \
    0xeb9d2d35, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

extern EFI_GUID       gOcCustomSmbiosTableGuid;

//
// We use this vendor name to spoof SMBIOS data on request.
// Note, never use Apple or similar on non apple instances (e.g. VMs).
// This breaks many internal and external os mechanisms.
//
#define OC_SMBIOS_VENDOR_NAME "Acidanthera"

typedef struct OC_SMBIOS_DATA_ {
  //
  // Type 0
  //
  CONST CHAR8     *BIOSVendor;
  CONST CHAR8     *BIOSVersion;
  CONST CHAR8     *BIOSReleaseDate;
  //
  // Type 1
  //
  CONST CHAR8     *SystemManufacturer;
  CONST CHAR8     *SystemProductName;
  CONST CHAR8     *SystemVersion;
  CONST CHAR8     *SystemSerialNumber;
  CONST GUID      *SystemUUID;
  CONST CHAR8     *SystemSKUNumber;
  CONST CHAR8     *SystemFamily;
  //
  // Type 2
  //
  CONST CHAR8     *BoardManufacturer;
  CONST CHAR8     *BoardProduct;
  CONST CHAR8     *BoardVersion;
  CONST CHAR8     *BoardSerialNumber;
  CONST CHAR8     *BoardAssetTag;
  CONST CHAR8     *BoardLocationInChassis;
  CONST UINT8     *BoardType;
  //
  // Type 3
  //
  CONST UINT8     *ChassisType;
  CONST CHAR8     *ChassisManufacturer;
  CONST CHAR8     *ChassisVersion;
  CONST CHAR8     *ChassisSerialNumber;
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
  // Type 131
  //
  UINT16          *ProcessorType;
  //
  // Type 133
  //
  UINT32          PlatformFeature;
} OC_SMBIOS_DATA;

typedef enum OC_SMBIOS_UPDATE_MODE_ {
  //
  // OcSmbiosUpdateOverwrite if new size is <= than the page-aligned original and
  // there are no issues with legacy region unlock. OcSmbiosUpdateCreate otherwise.
  //
  OcSmbiosUpdateAuto       = 0,
  //
  // Replace the tables with newly allocated. Normally it is in EfiRuntimeData (EDK 2), but
  // boot.efi relocates EfiRuntimeData unless AptioMemoryFix is used, so it is incorrect.
  // * MacEFI allocates in EfiReservedMemoryType at BASE_4GB-1:
  // - gEfiSmbiosTableGuid entry:             AllocateMaxAddress or AllocateAnyPages
  // - gEfiSmbiosTableGuid pages:             AllocateMaxAddress
  // - gEfiSmbiosTable3Guid entry and pages:  AllocateAnyPages
  // * Clover similarly allocates at EfiACPIMemoryNVS or EfiACPIReclaimMemory, if allocating
  // from EfiACPIMemoryNVS fails. EfiACPIReclaimMemory is actually unsafe, as ACPI driver may
  // theoretically nuke it after loading as per 7.2 Memory Allocation Services, Table 29 of
  // UEFI 2.7A specification (page 166).
  // * We allocate EfiReservedMemoryType at AllocateMaxAddress without any fallbacks.
  //
  OcSmbiosUpdateCreate     = 1,
  //
  // Overwrite existing gEfiSmbiosTableGuid and gEfiSmbiosTable3Guid data if it fits new size.
  // Abort with unspecified state otherwise.
  //
  OcSmbiosUpdateOverwrite  = 2,
  //
  // Writes first SMBIOS table (gEfiSmbiosTableGuid) to gOcCustomSmbiosTableGuid to workaround
  // firmwares overwriting SMBIOS contents at ExitBootServices. Otherwise equivalent to
  // OcSmbiosUpdateCreate. Requires patching AppleSmbios.kext and AppleACPIPlatform.kext to read
  // from another GUID: "EB9D2D31" -> "EB9D2D35" (in ASCII).
  //
  OcSmbiosUpdateCustom     = 3,
} OC_SMBIOS_UPDATE_MODE;

EFI_STATUS
CreateSmbios (
  IN OC_SMBIOS_DATA         *Data,
  IN OC_SMBIOS_UPDATE_MODE  Mode,
  IN OC_CPU_INFO            *CpuInfo
  );

#endif // OC_SMBIOS_LIB_H

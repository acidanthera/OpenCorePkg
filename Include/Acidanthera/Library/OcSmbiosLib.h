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
#include <Library/OcFileLib.h>
#include <Library/OcMacInfoLib.h>
#include <IndustryStandard/AppleSmBios.h>
#include <Guid/OcSmBios.h>

//
// We use this vendor name to spoof SMBIOS data on request.
// Note, never use Apple or similar on non-Apple instances (e.g. VMs).
// This breaks many internal and external OS mechanisms.
//
#define OC_SMBIOS_VENDOR_NAME "Acidanthera"

typedef struct OC_SMBIOS_MEMORY_DEVICE_DATA_ {
  //
  // Strings.
  //
  CONST CHAR8     *AssetTag;
  CONST CHAR8     *BankLocator;
  CONST CHAR8     *DeviceLocator;
  CONST CHAR8     *Manufacturer;
  CONST CHAR8     *PartNumber;
  CONST CHAR8     *SerialNumber;
  //
  // Size is in megabytes and speed is in MT/s.
  //
  CONST UINT32    *Size;
  CONST UINT16    *Speed;
} OC_SMBIOS_MEMORY_DEVICE_DATA;

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
  // Type 16
  //
  BOOLEAN         HasCustomMemory;
  CONST UINT8     *MemoryErrorCorrection;
  UINT16          MemoryDevicesCount;
  CONST UINT64    *MemoryMaxCapacity;
  //
  // Type 17
  //
  CONST UINT16                  *MemoryDataWidth;
  OC_SMBIOS_MEMORY_DEVICE_DATA  *MemoryDevices;
  CONST UINT8                   *MemoryFormFactor;
  //
  // Forcibly override MemoryFormFactor on non-Automatic mode when TRUE.
  //
  BOOLEAN                       ForceMemoryFormFactor;
  CONST UINT16                  *MemoryTotalWidth;
  CONST UINT8                   *MemoryType;
  CONST UINT16                  *MemoryTypeDetail;
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
  CONST UINT16    *ProcessorType;
  //
  // Type 133
  //
  CONST UINT32    *PlatformFeature;
  //
  // Type 134
  //
  CONST UINT8     *SmcVersion;
} OC_SMBIOS_DATA;

typedef enum OC_SMBIOS_UPDATE_MODE_ {
  //
  // OcSmbiosUpdateOverwrite if new size is <= than the page-aligned original and
  // there are no issues with legacy region unlock. OcSmbiosUpdateCreate otherwise.
  //
  OcSmbiosUpdateTryOverwrite = 0,
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
  OcSmbiosUpdateCreate       = 1,
  //
  // Overwrite existing gEfiSmbiosTableGuid and gEfiSmbiosTable3Guid data if it fits new size.
  // Abort with unspecified state otherwise.
  //
  OcSmbiosUpdateOverwrite    = 2,
  //
  // Writes first SMBIOS table (gEfiSmbiosTableGuid) to gOcCustomSmbiosTableGuid to workaround
  // some types of firmware overwriting SMBIOS contents at ExitBootServices. Otherwise equivalent
  // to OcSmbiosUpdateCreate. Requires patching AppleSmbios.kext and AppleACPIPlatform.kext 
  // to read from another GUID: "EB9D2D31" -> "EB9D2D35" (in ASCII).
  //
  OcSmbiosUpdateCustom       = 3,
} OC_SMBIOS_UPDATE_MODE;

//
// Growing SMBIOS table data.
//
typedef struct OC_SMBIOS_TABLE_ {
  //
  // SMBIOS table.
  //
  UINT8                            *Table;
  //
  // Current table position.
  //
  APPLE_SMBIOS_STRUCTURE_POINTER   CurrentPtr;
  //
  // Current string position.
  //
  CHAR8                            *CurrentStrPtr;
  //
  // Allocated table size, multiple of EFI_PAGE_SIZE.
  //
  UINT32                           AllocatedTableSize;
  //
  // Incrementable handle.
  //
  SMBIOS_HANDLE                    Handle;
  //
  // Largest structure size within the table.
  //
  UINT16                           MaxStructureSize;
  //
  // Number of structures within the table.
  //
  UINT16                           NumberOfStructures;
} OC_SMBIOS_TABLE;

/**
  Prepare new SMBIOS table based on host data.

  @param  SmbiosTable

  @retval EFI_SUCCESS if buffer is ready to be filled.
**/
EFI_STATUS
OcSmbiosTablePrepare (
  IN OUT OC_SMBIOS_TABLE  *SmbiosTable
  );

/**
  Free SMBIOS table

  @param[in,out]   Table  Current table buffer.

  @retval EFI_SUCCESS on success
**/
VOID
OcSmbiosTableFree (
  IN OUT OC_SMBIOS_TABLE  *Table
  );

/**
  Create new SMBIOS based on specified overrides.

  @param[in,out] SmbiosTable  SMBIOS Table handle.
  @param[in]     Data         SMBIOS overrides.
  @param[in]     Mode         SMBIOS update mode.
  @param[in]     CpuInfo      CPU information.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSmbiosCreate (
  IN OUT OC_SMBIOS_TABLE        *SmbiosTable,
  IN     OC_SMBIOS_DATA         *Data,
  IN     OC_SMBIOS_UPDATE_MODE  Mode,
  IN     OC_CPU_INFO            *CpuInfo
  );

/**
  Extract OEM information from SMBIOS to different places.
  Note, for MLB and ROM NVRAM values take precedence.

  @param[in,out] SmbiosTable         SMBIOS Table handle.
  @param[out]    ProductName         Export SMBIOS Type 1 product name,
                                     requiring OC_OEM_NAME_MAX bytes.
  @param[out]    SerialNumber        Export SMBIOS Type 1 product serial,
                                     requiring OC_OEM_SERIAL_MAX bytes.
  @param[out]    SystemUuid          Export SMBIOS Type 1 system UUID.
                                     UUID is always returned in RAW format.
  @param[out]    Mlb                 Export SMBIOS Type 2 board serial,
                                     requiring OC_OEM_SERIAL_MAX bytes.
  @param[out]    Rom                 Export ROM serial, requiring
                                     OC_OEM_ROM_MAX bytes.
  @param[out]    UuidIsRawEncoded    Pass FALSE to assume SMBIOS stores
                                     SystemUuid in Little Endian format
                                     and needs byte-swap.
  @param[in]     UseVariableStorage  Export OEM information to NVRAM.
**/
VOID
OcSmbiosExtractOemInfo (
  IN  OC_SMBIOS_TABLE   *SmbiosTable,
  OUT CHAR8             *ProductName        OPTIONAL,
  OUT CHAR8             *SerialNumber       OPTIONAL,
  OUT EFI_GUID          *SystemUuid         OPTIONAL,
  OUT CHAR8             *Mlb                OPTIONAL,
  OUT UINT8             *Rom                OPTIONAL,
  IN  BOOLEAN           UuidIsRawEncoded,
  IN  BOOLEAN           UseVariableStorage
  );

/**
  Convert SMC revision from SMC REV key format (6-byte)
  to SMBIOS ASCII format (16-byte, APPLE_SMBIOS_SMC_VERSION_SIZE).

  @param[in]  SmcRevision  SMC revision in REV key format.
  @param[out] SmcVersion   SMC revision in SMBIOS format.  
**/
VOID
OcSmbiosGetSmcVersion (
  IN  CONST UINT8  *SmcRevision,
  OUT UINT8        *SmcVersion
  );

/**
  Choose update mode based on default representation.
  Always returns valid update mode, by falling back
  to Create when unknown mode was found. Known modes are:
  TryOverwrite, Create, Overwrite, Custom.

  @param[in] UpdateMode in ASCII format.

  @returns SMBIS update mode in enum format.
**/
OC_SMBIOS_UPDATE_MODE
OcSmbiosGetUpdateMode (
  IN CONST CHAR8  *UpdateMode
  );

/**
  Dump current SMBIOS data into specified directory
  under EntryV#.bin/DataV#.bin names, where # is
  SMBIOS version: 1, 3, or both.

  @param[in] Root  Directory to dump SMBIOS data.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcSmbiosDump (
  IN EFI_FILE_PROTOCOL  *Root
  );

#endif // OC_SMBIOS_LIB_H

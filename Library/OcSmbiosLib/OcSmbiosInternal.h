/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_SMBIOS_INTERNAL_H
#define OC_SMBIOS_INTERNAL_H

#include <IndustryStandard/AppleSmBios.h>
#include <Library/OcGuardLib.h>

#define SMBIOS_STRUCTURE_TERMINATOR_SIZE 2

//
// According to SMBIOS spec (3.2.0, page 26) SMBIOS handle is a number from 0 to 0xFF00.
// SMBIOS spec does not require handles to be contiguous or remain valid across SMBIOS.
// The only requirements is uniqueness and range conformance, so we reserve select handles
// for dedicated tables here for easier assignment. Automatic handle assignment starts with
// OcSmbiosAutomaticHandle.
//
enum {
  OcSmbiosInvalidHandle,
  OcSmbiosBiosInformationHandle,
  OcSmbiosSystemInformationHandle,
  OcSmbiosBaseboardInformationHandle,
  OcSmbiosSystemEnclosureHandle,
  OcSmbiosProcessorInformationHandle,
  OcSmbiosMemoryControllerInformationHandle,
  OcSmbiosMemoryModuleInformatonHandle,
  /* OcSmbiosCacheInformationHandle, */
  OcSmbiosL1CacheHandle,
  OcSmbiosL2CacheHandle,
  OcSmbiosL3CacheHandle,
  /* OcSmbiosPortConnectorInformationHandle, */
  /* OcSmbiosSystemSlotsHandle, */
  OcSmbiosOnboardDeviceInformationHandle,
  OcSmbiosOemStringsHandle,
  OcSmbiosSystemConfigurationOptionsHandle,
  OcSmbiosBiosLanguageInformationHandle,
  OcSmbiosGroupAssociationsHandle,
  OcSmbiosSystemEventLogHandle,
  OcSmbiosPhysicalMemoryArrayHandle,
  /* OcSmbiosMemoryDeviceHandle, */
  OcSmbios32BitMemoryErrorInformationHandle,
  /* OcSmbiosMemoryArrayMappedAddressHandle, */
  /* OcSmbiosMemoryDeviceMappedAddressHandle, */
  OcSmbiosBuiltInPointingDeviceHandle,
  OcSmbiosPortableBatteryHandle,
  OcSmbiosSystemResetHandle,
  OcSmbiosHardwareSecurityHandle,
  OcSmbiosSystemPowerControlsHandle,
  OcSmbiosVoltageProbeHandle,
  OcSmbiosCoolingDeviceHandle,
  OcSmbiosTemperatureProbeHandle,
  OcSmbiosElectricalCurrentProbeHandle,
  OcSmbiosOutOfBandRemoteAccessHandle,
  OcSmbiosBootIntegrityServiceHandle,
  OcSmbiosSystemBootInformationHandle,
  OcSmbios64BitMemoryErrorInformationHandle,
  OcSmbiosManagementDeviceHandle,
  OcSmbiosManagementDeviceComponentHandle,
  OcSmbiosManagementDeviceThresholdDataHandle,
  OcSmbiosMemoryChannelHandle,
  OcSmbiosIpmiDeviceInformationHandle,
  OcSmbiosSystemPowerSupplyHandle,
  OcSmbiosAdditionalInformationHandle,
  OcSmbiosOnboardDevicesExtendedInformationHandle,
  OcSmbiosManagementControllerHostInterfaceHandle,
  OcSmbiosTpmDeviceHandle,
  OcSmbiosInactiveHandle,
  OcSmbiosEndOfTableHandle,
  OcAppleSmbiosFirmwareInformationHandle,
  OcAppleSmbiosMemorySpdDataHandle,
  OcAppleSmbiosProcessorTypeHandle,
  OcAppleSmbiosProcessorBusSpeedHandle,
  OcAppleSmbiosPlatformFeatureHandle,
  OcAppleSmbiosSmcInformationHandle,

  OcSmbiosLastReservedHandle,
  OcSmbiosAutomaticHandle = 128,
};

OC_GLOBAL_STATIC_ASSERT (OcSmbiosAutomaticHandle > OcSmbiosLastReservedHandle, "Inconsistent handle IDs");

//
// Growing SMBIOS table data
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
  Allocate bytes in SMBIOS table if necessary

  @retval EFI_SUCCESS on success
**/
EFI_STATUS
SmbiosExtendTable (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     UINT32           NewSize
  );

/**
  Initialise SMBIOS structure

  @param[in] Table             Pointer to location containing the current address within the buffer.
  @param[in] Type              Table type.
  @param[in] MinLength         Initial length of the table.
  @param[in] Index             Table index, normally 1.

  @retval
**/
EFI_STATUS
SmbiosInitialiseStruct (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     UINT32           Type,
  IN     UINT32           MinLength,
  IN     UINT16           Index
  );

/**
  Finalise SMBIOS structure

  @param[in] Table                  Pointer to location containing the current address within the buffer.

  @retval
**/
VOID
SmbiosFinaliseStruct (
  IN OUT OC_SMBIOS_TABLE  *Table
  );

// SmbiosGetString
/**

  @retval
**/
CHAR8 *
SmbiosGetString (
  IN APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING            String
  );

// SmbiosSetString
/**

  @param[in] Buffer        Pointer to the buffer where to create the smbios string.
  @param[in] String        Pointer to the null terminated ascii string to set.
  @param[in] Index         Pointer to byte index.

  @retval
**/
UINT8
SmbiosSetString (
  IN  CHAR8     **Buffer,
  IN  CHAR8     *String,
  IN  UINT8     *Index
  );

// SmbiosSetStringHex
/**
  @param[in] Buffer        Pointer to location containing the current address within the buffer.
  @param[in] StringIndex   String Index to retrieve
  @param[in] String        Buffer containing the null terminated ascii string

  @retval
**/
UINT8
SmbiosSetStringHex (
  IN  CHAR8     **Buffer,
  IN  CHAR8     *String,
  IN  UINT8     *Index
  );

// SmbiosOverrideString
/**
  @param[in] Table         Current table buffer.
  @param[in] Override      String data override
  @param[in] Index         Pointer to current string index, incremented on success

  @retval new string index or 0
**/
UINT8
SmbiosOverrideString (
  IN  OC_SMBIOS_TABLE  *Table,
  IN  CONST CHAR8      *Override,
  IN  UINT8            *Index,
  IN  BOOLEAN          Hex
  );

// SmbiosGetTableLength
/**

  @retval
**/
UINTN
SmbiosGetTableLength (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable
  );

/**
  Obtain minimal size for a table of specific type.

  @param[in]  Type   Table type

  @retval underlying table type size or generic header size
**/
UINT32
SmbiosGetTableTypeLength (
  IN  SMBIOS_TYPE                     Type
  );

// SmbiosGetTableFromType
/**

  @retval
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromType (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN  UINT32                          SmbiosTableSize,
  IN  SMBIOS_TYPE                     Type,
  IN  UINT16                          Index
  );

// SmbiosGetTableFromHandle
/**

  @retval
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromHandle (
  IN  SMBIOS_TABLE_ENTRY_POINT    *Smbios,
  IN  SMBIOS_HANDLE               Handle
  );

// SmbiosGetTableCount
/**

  @retval
**/
UINT16
SmbiosGetTableCount (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN  UINT32                          SmbiosTableSize,
  IN  SMBIOS_TYPE                     Type
  );

#endif // OC_SMBIOS_INTERNAL_H

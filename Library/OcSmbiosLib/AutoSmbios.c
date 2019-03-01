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

#include <Uefi.h>

#include <Protocol/PciRootBridgeIo.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcProtocolLib.h>
#include <Library/PrintLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Macros.h>

#include "OcSmbiosInternal.h"
#include "DebugSmbios.h"

EFI_STATUS
SmbiosExtendTable (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     UINT32           NewSize
  )
{
  //TODO: implement
  return EFI_OUT_OF_RESOURCES;
}

UINT8
SmbiosOverrideString (
  IN  OC_SMBIOS_TABLE  *Table,
  IN  CONST CHAR8      *Override,
  IN  UINT8            *Index,
  IN  BOOLEAN          Hex
  )
{
  //TODO: implement
  return 0;
}

STATIC
EFI_STATUS
SmbiosAssignStructHandle (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     UINT32           Type,
  IN     UINT16           Index
  )
{
  //
  // Support select tables to have more than 1 entry.
  //
  if (Type == SMBIOS_TYPE_CACHE_INFORMATION) {
    switch (Index) {
      case 1:
        Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosL1CacheHandle;
        return EFI_SUCCESS;
      case 2:
        Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosL2CacheHandle;
        return EFI_SUCCESS;
      case 3:
        Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosL3CacheHandle;
        return EFI_SUCCESS;
      default:
        ASSERT (FALSE);
        return EFI_INVALID_PARAMETER;
    }
  } else if (Type == SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION
          || Type == SMBIOS_TYPE_SYSTEM_SLOTS
          || Type == SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS
          || Type == SMBIOS_TYPE_MEMORY_DEVICE
          || Type == SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS) {
    Table->CurrentPtr.Standard.Hdr->Handle = Table->Handle++;
    return EFI_SUCCESS;
  } else if (Index != 1) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  switch (Type) {
    case SMBIOS_TYPE_BIOS_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosBiosInformationHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemInformationHandle;
      break;
    case SMBIOS_TYPE_BASEBOARD_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosBaseboardInformationHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_ENCLOSURE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemEnclosureHandle;
      break;
    case SMBIOS_TYPE_PROCESSOR_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosProcessorInformationHandle;
      break;
    case SMBIOS_TYPE_MEMORY_CONTROLLER_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosMemoryControllerInformationHandle;
      break;
    case SMBIOS_TYPE_MEMORY_MODULE_INFORMATON:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosMemoryModuleInformatonHandle;
      break;
    case SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosOnboardDeviceInformationHandle;
      break;
    case SMBIOS_TYPE_OEM_STRINGS:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosOemStringsHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemConfigurationOptionsHandle;
      break;
    case SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosBiosLanguageInformationHandle;
      break;
    case SMBIOS_TYPE_GROUP_ASSOCIATIONS:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosGroupAssociationsHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_EVENT_LOG:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemEventLogHandle;
      break;
    case SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosPhysicalMemoryArrayHandle;
      break;
    case SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbios32BitMemoryErrorInformationHandle;
      break;
    case SMBIOS_TYPE_BUILT_IN_POINTING_DEVICE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosBuiltInPointingDeviceHandle;
      break;
    case SMBIOS_TYPE_PORTABLE_BATTERY:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosPortableBatteryHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_RESET:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemResetHandle;
      break;
    case SMBIOS_TYPE_HARDWARE_SECURITY:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosHardwareSecurityHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_POWER_CONTROLS:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemPowerControlsHandle;
      break;
    case SMBIOS_TYPE_VOLTAGE_PROBE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosVoltageProbeHandle;
      break;
    case SMBIOS_TYPE_COOLING_DEVICE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosCoolingDeviceHandle;
      break;
    case SMBIOS_TYPE_TEMPERATURE_PROBE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosTemperatureProbeHandle;
      break;
    case SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosElectricalCurrentProbeHandle;
      break;
    case SMBIOS_TYPE_OUT_OF_BAND_REMOTE_ACCESS:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosOutOfBandRemoteAccessHandle;
      break;
    case SMBIOS_TYPE_BOOT_INTEGRITY_SERVICE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosBootIntegrityServiceHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemBootInformationHandle;
      break;
    case SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbios64BitMemoryErrorInformationHandle;
      break;
    case SMBIOS_TYPE_MANAGEMENT_DEVICE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosManagementDeviceHandle;
      break;
    case SMBIOS_TYPE_MANAGEMENT_DEVICE_COMPONENT:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosManagementDeviceComponentHandle;
      break;
    case SMBIOS_TYPE_MANAGEMENT_DEVICE_THRESHOLD_DATA:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosManagementDeviceThresholdDataHandle;
      break;
    case SMBIOS_TYPE_MEMORY_CHANNEL:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosMemoryChannelHandle;
      break;
    case SMBIOS_TYPE_IPMI_DEVICE_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosIpmiDeviceInformationHandle;
      break;
    case SMBIOS_TYPE_SYSTEM_POWER_SUPPLY:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosSystemPowerSupplyHandle;
      break;
    case SMBIOS_TYPE_ADDITIONAL_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosAdditionalInformationHandle;
      break;
    case SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosOnboardDevicesExtendedInformationHandle;
      break;
    case SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosManagementControllerHostInterfaceHandle;
      break;
    case SMBIOS_TYPE_TPM_DEVICE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosTpmDeviceHandle;
      break;
    case SMBIOS_TYPE_INACTIVE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosInactiveHandle;
      break;
    case SMBIOS_TYPE_END_OF_TABLE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcSmbiosEndOfTableHandle;
      break;
    case APPLE_SMBIOS_TYPE_FIRMWARE_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcAppleSmbiosFirmwareInformationHandle;
      break;
    case APPLE_SMBIOS_TYPE_MEMORY_SPD_DATA:
      Table->CurrentPtr.Standard.Hdr->Handle = OcAppleSmbiosMemorySpdDataHandle;
      break;
    case APPLE_SMBIOS_TYPE_PROCESSOR_TYPE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcAppleSmbiosProcessorTypeHandle;
      break;
    case APPLE_SMBIOS_TYPE_PROCESSOR_BUS_SPEED:
      Table->CurrentPtr.Standard.Hdr->Handle = OcAppleSmbiosProcessorBusSpeedHandle;
      break;
    case APPLE_SMBIOS_TYPE_PLATFORM_FEATURE:
      Table->CurrentPtr.Standard.Hdr->Handle = OcAppleSmbiosPlatformFeatureHandle;
      break;
    case APPLE_SMBIOS_TYPE_SMC_INFORMATION:
      Table->CurrentPtr.Standard.Hdr->Handle = OcAppleSmbiosSmcInformationHandle;
      break;
    default:
      ASSERT (FALSE);
      Table->CurrentPtr.Standard.Hdr->Handle = Table->Handle++;
      break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SmbiosInitialiseStruct (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     UINT32           Type,
  IN     UINT32           MinLength,
  IN     UINT16           Index
  )
{
  EFI_STATUS  Status;

  Status = SmbiosExtendTable (Table, MinLength + SMBIOS_STRUCTURE_TERMINATOR_SIZE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to extend SMBIOS for table %u - %r", Type, Status));
    return Status;
  }

  Table->CurrentPtr.Standard.Hdr->Type   = Type;
  Table->CurrentPtr.Standard.Hdr->Length = MinLength;
  Status = SmbiosAssignStructHandle (Table, Type, Index);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Table->CurrentStrPtr = (CHAR8 *) Table->CurrentPtr.Standard.Raw + MinLength;

  return EFI_SUCCESS;
}

VOID
SmbiosFinaliseStruct (
  IN OUT OC_SMBIOS_TABLE  *Table
  )
{
  DEBUG_CODE_BEGIN();
  SmbiosDebugAnyStructure (Table->CurrentPtr);
  DEBUG_CODE_END();

  if (Table->CurrentPtr.Standard.Hdr->Length > Table->MaxStructureSize) {
    Table->MaxStructureSize = Table->CurrentPtr.Standard.Hdr->Length;
  }

  Table->CurrentPtr.Raw += Table->CurrentPtr.Standard.Hdr->Length;

  Table->NumberOfStructures++;

  //
  // SMBIOS spec requires 2 terminator bytes after structures without strings and 1 byte otherwise.
  // We initially allocate 2 extra bytes (SMBIOS_STRUCTURE_TERMINATOR_SIZE), and end up using one
  // or two here.
  //
  if (Table->CurrentStrPtr != (CHAR8 *) Table->CurrentPtr.Raw) {
    Table->CurrentPtr.Raw = (UINT8 *) Table->CurrentStrPtr + 1;
  } else {
    Table->CurrentPtr.Raw += 2;
  }
}

// SmbiosGetString
/**
  @param[in] SmbiosTable
  @param[in] StringIndex  String Index to retrieve

  @retval
**/
CHAR8 *
SmbiosGetString (
  IN APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING             String
  )
{
  CHAR8      *AString = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Standard.Hdr->Length);
  UINT8      Index = 1;

  if (String == 0)
    return NULL;

  while (Index != String) {
    while (*AString != 0) {
      AString ++;
    }
    AString ++;
    if (*AString == 0) {
      return AString;
    }
    Index ++;
  }

  return AString;
}

// SmbiosSetString
/**
  @param[in] Buffer        Pointer to location containing the current address within the buffer.
  @param[in] StringIndex   String Index to retrieve
  @param[in] String        Buffer containing the null terminated ascii string

  @retval
**/
UINT8
SmbiosSetString (
  IN  CHAR8     **Buffer,
  IN  CHAR8     *String,
  IN  UINT8     *Index
  )
{
  UINTN   Length  = 0;
  VOID    *Target = (VOID *)*((UINTN *)Buffer);

  if (String == NULL) {
    return 0;
  }

  Length = AsciiStrLen (String);

  // Remove any spaces found at the end
  while ((Length != 0) && (String[Length - 1] == ' '))
  {
    Length--;
  }

  if (Length == 0)
  {
    return 0;
  }

  CopyMem (Target, String, Length);

  *Buffer += (Length + 1);

  return *Index += 1;
}

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
  )
{
  UINTN   Length  = 0;
  CHAR8   Digit;
  CHAR8   *Target = (VOID *)*((UINTN *)Buffer);

  if (String == NULL) {
    return 0;
  }

  Length = AsciiStrLen (String);

  if (Length == 0) {
    return 0;
  }

  // Remove any spaces found at the end
  while ((Length != 0) && (String[Length - 1] == ' '))
  {
    Length--;
  }

  if (Length == 0)
  {
    return 0;
  }

  *Target++ = '0';
  *Target++ = 'x';

  while ((Length != 0) && (*String) != 0)
  {
    Digit = *String++;
    *Target++ = "0123456789ABCDEF"[((Digit >> 4) & 0xF)];
    *Target++ = "0123456789ABCDEF"[(Digit & 0xF)];
    Length--;
  }

  *Buffer += (Target - *Buffer) + 1;

  return *Index += 1;
}

// SmbiosSetOverrideString
/**
  @param[in] Buffer        Pointer to location containing the current address within the buffer.
  @param[in] Override      String data override
  @param[in] Index         Pointer to current string index, incremented on success

  @retval next string index
**/
UINT8
SmbiosSetOverrideString (
  IN  CHAR8         **Buffer,
  IN  CONST CHAR8   *Override,
  IN  UINT8         *Index
  )
{
  UINTN       Length;

  ASSERT (Buffer != NULL);
  ASSERT (Override != NULL);
  ASSERT (Index != NULL);

  Length = AsciiStrLen (Override);
  if (Length > SMBIOS_STRING_MAX_LENGTH) {
    //
    // Truncate on size exceed.
    //
    Length = SMBIOS_STRING_MAX_LENGTH;
  }

  CopyMem (*Buffer, Override, Length);
  *Buffer += Length + 1;
  *Index += 1;

  return *Index;
}

// SmbiosGetTableLength
/**

  @retval
**/
UINTN
SmbiosGetTableLength (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable
  )
{
  CHAR8  *AChar;
  UINTN  Length;

  AChar = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Standard.Hdr->Length);
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++;
  }
  Length = ((UINTN)AChar - (UINTN)SmbiosTable.Raw + 2);

  return Length;
}

// SmbiosGetTableFromType
/**

  @param[in] SmbiosTable      Pointer to SMBIOS table.
  @param[in] SmbiosTableSize  SMBIOS table size
  @param[in] Type             SMBIOS table type
  @param[in] Index            SMBIOS table index starting from 1

  @retval found table or NULL
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromType (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN  UINT32                          SmbiosTableSize,
  IN  SMBIOS_TYPE                     Type,
  IN  UINT16                          Index
  )
{
  UINT16  SmbiosTypeIndex;

  (VOID) SmbiosTableSize;

  //TODO: fix iteration code accessing out of bounds memory.
  // Note, we should not call SmbiosGetTableTypeLength here as vendor extensions are unknown.

  if (SmbiosTable.Raw == NULL) {
    return SmbiosTable;
  }

  SmbiosTypeIndex = 1;

  while ((SmbiosTypeIndex != Index) || (SmbiosTable.Standard.Hdr->Type != Type)) {
    if (SmbiosTable.Standard.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      SmbiosTable.Raw = NULL;
      break;
    }
    if (SmbiosTable.Standard.Hdr->Type == Type) {
      SmbiosTypeIndex++;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosGetTableLength (SmbiosTable));
    if (SmbiosTable.Raw > (UINT8 *)(UINTN)(SmbiosTable.Raw + SmbiosTableSize)) {
      SmbiosTable.Raw = NULL;
      break;
    }
  }

  return SmbiosTable;
}

// SmbiosGetTableFromHandle
/**

  @param[in] Smbios        Pointer to smbios entry point structure.
  @param[in] Handle

  @retval
**/
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetTableFromHandle (
  IN  SMBIOS_TABLE_ENTRY_POINT    *Smbios,
  IN  SMBIOS_HANDLE               Handle
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER    SmbiosTable;

  SmbiosTable.Raw = (UINT8 *)(UINTN)Smbios->TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return SmbiosTable;
  }

  while (SmbiosTable.Standard.Hdr->Handle != Handle) {
    if (SmbiosTable.Standard.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      SmbiosTable.Raw = NULL;
      break;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosGetTableLength (SmbiosTable));
    if (SmbiosTable.Raw > (UINT8 *)(UINTN)(Smbios->TableAddress + Smbios->TableLength)) {
      SmbiosTable.Raw = NULL;
      break;
    }
  }

  return SmbiosTable;
}

// SmbiosGetTableCount
/**

  @param[in] Smbios        Pointer to smbios entry point structure.
  @param[in] Type

  @retval
**/
UINT16
SmbiosGetTableCount (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN  UINT32                          SmbiosTableSize,
  IN  SMBIOS_TYPE                     Type
  )
{
  UINT16  SmbiosTypeIndex;

  SmbiosTypeIndex = 0;
  if (SmbiosTable.Raw == NULL) {
    return 0;
  }

  while (SmbiosTable.Standard.Hdr->Type != SMBIOS_TYPE_END_OF_TABLE) {
    if (SmbiosTable.Standard.Hdr->Type == Type) {
      SmbiosTypeIndex ++;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosGetTableLength (SmbiosTable));
    if (SmbiosTable.Raw > (UINT8 *)(UINTN)(SmbiosTable.Raw + SmbiosTableSize)) {
      break;
    }
  }
  return SmbiosTypeIndex;
}

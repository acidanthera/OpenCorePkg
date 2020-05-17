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
#include <Library/OcSmbiosLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "SmbiosInternal.h"
#include "DebugSmbios.h"

EFI_STATUS
SmbiosExtendTable (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     UINT32           Size
  )
{
  UINT32  TableSize;
  UINT32  RequestedSize;
  UINT8   *NewTable;

  //
  // Always requrest 2 more bytes, so that the table can be terminated.
  //
  Size += SMBIOS_STRUCTURE_TERMINATOR_SIZE;

  if (Table->Table == NULL) {
    TableSize = 0;
  } else {
    TableSize = (UINT32)((UINT8 *) Table->CurrentStrPtr - Table->Table);
  }

  //
  // We are not allowed to allocate more than we can write.
  //
  RequestedSize = TableSize + Size;
  if (RequestedSize > SMBIOS_TABLE_MAX_LENGTH) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Skip reallocation if region fits.
  //
  if (RequestedSize <= Table->AllocatedTableSize) {
    return EFI_SUCCESS;
  }

  RequestedSize = ALIGN_VALUE (RequestedSize, EFI_PAGE_SIZE);
  if (RequestedSize > ALIGN_VALUE (SMBIOS_TABLE_MAX_LENGTH, EFI_PAGE_SIZE)) {
    RequestedSize = ALIGN_VALUE (SMBIOS_TABLE_MAX_LENGTH, EFI_PAGE_SIZE);
  }

  NewTable = ReallocatePool (TableSize, RequestedSize, Table->Table);
  if (NewTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Guarantee zero fill for the table.
  //
  ZeroMem (NewTable + TableSize, RequestedSize - TableSize);

  Table->CurrentPtr.Raw     = NewTable + (Table->CurrentPtr.Raw - Table->Table);
  Table->CurrentStrPtr      = (CHAR8 *) NewTable + TableSize;
  Table->Table              = NewTable;
  Table->AllocatedTableSize = RequestedSize;

  return EFI_SUCCESS;
}

UINT8
SmbiosOverrideString (
  IN  OC_SMBIOS_TABLE  *Table,
  IN  CONST CHAR8      *Override OPTIONAL,
  IN  UINT8            *Index
  )
{
  UINT32  Length;

  //
  // No override.
  //
  if (Override == NULL) {
    return 0;
  }

  Length = (UINT32) AsciiStrLen (Override);

  //
  // Truncate to fit but do not error.
  //
  if (Length > SMBIOS_STRING_MAX_LENGTH) {
    Length = SMBIOS_STRING_MAX_LENGTH;
    DEBUG ((DEBUG_INFO, "OCMB: SMBIOS truncating '%a' to %u bytes\n", Override, Length));
  }

  while (Length > 0 && Override[Length - 1] == ' ') {
    Length--;
  }

  if (Length == 0) {
    return 0;
  }

  if (EFI_ERROR (SmbiosExtendTable (Table, Length + 1))) {
    DEBUG ((DEBUG_WARN, "OCSMB: SMBIOS failed to write '%a' with %u byte extension\n", Override, Length + 1));
    return 0;
  }

  return SmbiosSetString (&Table->CurrentStrPtr, Override, Length, Index);
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
  IN     SMBIOS_TYPE      Type,
  IN     UINT8            MinLength,
  IN     UINT16           Index
  )
{
  EFI_STATUS  Status;

  Status = SmbiosExtendTable (Table, MinLength);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCSMB: Failed to extend SMBIOS for table %u - %r", Type, Status));
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
  // We allocate 2 extra bytes (SMBIOS_STRUCTURE_TERMINATOR_SIZE), and end up using one or two here.
  //
  if (Table->CurrentStrPtr != (CHAR8 *) Table->CurrentPtr.Raw) {
    Table->CurrentStrPtr++;
    Table->CurrentPtr.Raw = (UINT8 *) Table->CurrentStrPtr;
  } else {
    Table->CurrentStrPtr += 2;
    Table->CurrentPtr.Raw += 2;
  }
}

CHAR8 *
SmbiosGetString (
  IN APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING             String
  )
{
  UINT8  *Walker;
  UINT8  Index;

  if (String == 0) {
    return NULL;
  }

  Walker = SmbiosTable.Raw + SmbiosTable.Standard.Hdr->Length;

  for (Index = 1; Index != String; Index++) {
    while (*Walker != 0) {
      Walker++;
    }

    Walker++;

    //
    // Double NULL termination means end of table.
    //
    if (*Walker == 0) {
      return NULL;
    }
  }

  return (CHAR8*) Walker;
}

UINT8
SmbiosSetString (
  IN OUT  CHAR8        **Buffer,
  IN      CONST CHAR8  *String,
  IN      UINT32       Length,
  IN OUT  UINT8        *Index
  )
{
  ASSERT (Buffer != NULL);
  ASSERT (String != NULL);
  ASSERT (Index != NULL);
  ASSERT (Length > 0);

  CopyMem (*Buffer, String, Length);
  *Buffer += Length + 1;
  (*Index)++;

  return *Index;
}

UINT32
SmbiosGetStructureLength (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN  UINT32                          SmbiosTableSize
  )
{
  UINT32  Length;
  UINT8   *Walker;

  Length = SmbiosTable.Standard.Hdr->Length;
  if (Length > SmbiosTableSize) {
    return 0;
  }

  SmbiosTable.Raw += Length;
  SmbiosTableSize -= Length;
  Walker = SmbiosTable.Raw;
  while (SmbiosTableSize >= SMBIOS_STRUCTURE_TERMINATOR_SIZE) {
    if (Walker[0] == 0 && Walker[1] == 0) {
      return Length + (UINT32)(Walker - SmbiosTable.Raw) + SMBIOS_STRUCTURE_TERMINATOR_SIZE;
    }
    Walker++;
    SmbiosTableSize--;
  }

  return 0;
}

APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetStructureOfType (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN  UINT32                          SmbiosTableSize,
  IN  SMBIOS_TYPE                     Type,
  IN  UINT16                          Index
  )
{
  UINT16  SmbiosTypeIndex;
  UINT32  Length;

  SmbiosTypeIndex = 1;

  while (SmbiosTableSize >= sizeof (SMBIOS_STRUCTURE)) {
    //
    // Perform basic size sanity check.
    //
    Length = SmbiosGetStructureLength (SmbiosTable, SmbiosTableSize);
    if (Length == 0) {
      break;
    }

    //
    // We found the right table.
    //
    if (SmbiosTypeIndex == Index && SmbiosTable.Standard.Hdr->Type == Type) {
      return SmbiosTable;
    }

    //
    // Abort on EOT.
    //
    if (SmbiosTable.Standard.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      break;
    }

    //
    // Skip extra tables of this type until we reach requested number.
    //
    if (SmbiosTable.Standard.Hdr->Type == Type) {
      SmbiosTypeIndex++;
      //
      // Unsigned wraparound, we have more than MAX_UINT16 tables of this kind.
      //
      if (SmbiosTypeIndex == 0) {
        break;
      }
    }

    SmbiosTable.Raw += Length;
    SmbiosTableSize -= Length;
  }

  SmbiosTable.Raw = NULL;
  return SmbiosTable;
}

UINT16
SmbiosGetStructureCount (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN  UINT32                          SmbiosTableSize,
  IN  SMBIOS_TYPE                     Type
  )
{
  UINT16  Count;
  UINT32  Length;

  Count = 0;

  while (SmbiosTableSize >= sizeof (SMBIOS_STRUCTURE)) {
    //
    // Perform basic size sanity check.
    //
    Length = SmbiosGetStructureLength (SmbiosTable, SmbiosTableSize);
    if (Length == 0) {
      break;
    }

    //
    // We found the right table.
    //
    if (SmbiosTable.Standard.Hdr->Type == Type) {
      Count++;
      //
      // Unsigned wraparound, we have more than MAX_UINT16 tables of this kind.
      //
      if (Count == 0) {
        break;
      }
    }

    //
    // Abort on EOT.
    //
    if (SmbiosTable.Standard.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      break;
    }

    SmbiosTable.Raw += Length;
    SmbiosTableSize -= Length;
  }

  return Count;
}

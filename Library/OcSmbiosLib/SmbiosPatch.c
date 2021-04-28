/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

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

#include <Guid/AppleVariable.h>
#include <Guid/OcVariable.h>
#include <Guid/SmBios.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <IndustryStandard/AppleSmBios.h>
#include <IndustryStandard/Pci.h>

#include "DebugSmbios.h"
#include "SmbiosInternal.h"

#include <IndustryStandard/ProcessorInfo.h>

STATIC SMBIOS_TABLE_ENTRY_POINT        *mOriginalSmbios;
STATIC SMBIOS_TABLE_3_0_ENTRY_POINT    *mOriginalSmbios3;
STATIC APPLE_SMBIOS_STRUCTURE_POINTER  mOriginalTable;
STATIC UINT32                          mOriginalTableSize;

#define SMBIOS_OVERRIDE_S(Table, Field, Original, Value, Index, Fallback) \
  do { \
    CONST CHAR8  *RealValue__ = (Value); \
    if (RealValue__ == NULL && ((Original).Raw) != NULL && (Original).Raw + (Original).Standard.Hdr->Length \
      >= ((UINT8 *)&((Original).Field) + sizeof (SMBIOS_TABLE_STRING))) { \
      RealValue__ = SmbiosGetString ((Original), ((Original).Field)); \
    } \
    (((Table)->CurrentPtr).Field) = SmbiosOverrideString ( \
      (Table), \
      RealValue__ != NULL ? RealValue__ : (Fallback), \
      (Index) \
      ); \
  } while (0)

#define SMBIOS_OVERRIDE_V(Table, Field, Original, Value, Fallback) \
  do { \
    if ((Value) != NULL) { \
      CopyMem (&(((Table)->CurrentPtr).Field), (Value), sizeof ((((Table)->CurrentPtr).Field))); \
    } else if ((Original).Raw != NULL && (Original).Raw + (Original).Standard.Hdr->Length \
      >= ((UINT8 *)&((Original).Field) + sizeof ((((Table)->CurrentPtr).Field)))) { \
      CopyMem (&(((Table)->CurrentPtr).Field), &((Original).Field), sizeof ((((Table)->CurrentPtr).Field))); \
    } else if ((Fallback) != NULL) { \
      CopyMem (&(((Table)->CurrentPtr).Field), (Fallback), sizeof ((((Table)->CurrentPtr).Field))); \
    } else { \
      /* No ZeroMem call as written area is guaranteed to be 0 */ \
    } \
  } while (0)

#define SMBIOS_ACCESSIBLE(Table, Field) \
  (((UINT8 *) &(Table).Field - (Table).Raw + sizeof ((Table).Field)) <= (Table).Standard.Hdr->Length)

STATIC
APPLE_SMBIOS_STRUCTURE_POINTER
SmbiosGetOriginalStructure (
  IN  SMBIOS_TYPE   Type,
  IN  UINT16        Index
  )
{
  if (mOriginalTable.Raw == NULL) {
    return mOriginalTable;
  }

  return SmbiosGetStructureOfType (mOriginalTable, mOriginalTableSize, Type, Index);
}

STATIC
UINT16
SmbiosGetOriginalStructureCount (
  IN  SMBIOS_TYPE   Type
  )
{
  if (mOriginalTable.Raw == NULL) {
    return 0;
  }

  return SmbiosGetStructureCount (mOriginalTable, mOriginalTableSize, Type);
}

STATIC
BOOLEAN
SmbiosHasValidOemFormFactor (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Original
  )
{
  return Original.Raw != NULL
    && SMBIOS_ACCESSIBLE (Original, Standard.Type17->FormFactor)
    && Original.Standard.Type17->FormFactor != 0;
}

STATIC
UINT8
SmbiosGetFormFactor (
  IN  OC_SMBIOS_DATA                  *Data,
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Original
  )
{
  BOOLEAN  IsAutomatic;

  IsAutomatic = !Data->ForceMemoryFormFactor;

  if (IsAutomatic) {
    //
    // Try to use the original value if valid first.
    //
    if (SmbiosHasValidOemFormFactor (Original)) {
      return Original.Standard.Type17->FormFactor;
    }
    //
    // If not, use the value from database.
    //
    if (Data->MemoryFormFactor != NULL) {
      return *Data->MemoryFormFactor;
    }
  }

  //
  // Under non-Automatic mode, simply use the value from config.
  //
  if (Data->MemoryFormFactor != NULL) {
    return *Data->MemoryFormFactor;
  }
  //
  // If the value is not available from config, then try to use the original value.
  //
  if (SmbiosHasValidOemFormFactor (Original)) {
    return Original.Standard.Type17->FormFactor;
  }

  //
  // If not valid at all, fall back on the failsafe.
  //
  return MemoryFormFactorUnknown;
}

/** Type 0

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.

  @retval TRUE                      Apple SMBIOS detected
  @retval FALSE                     Apple SMBIOS not detected
**/
STATIC
BOOLEAN
PatchBiosInformation (
  IN OUT OC_SMBIOS_TABLE *Table,
  IN     OC_SMBIOS_DATA  *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;
  CHAR8                           *Vendor;
  BOOLEAN                         IsApple;

  Original    = SmbiosGetOriginalStructure (SMBIOS_TYPE_BIOS_INFORMATION, 1);
  MinLength   = sizeof (*Original.Standard.Type0);
  StringIndex = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_BIOS_INFORMATION, MinLength, 1))) {
    return FALSE;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type0->Vendor, Original, Data->BIOSVendor, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type0->BiosVersion, Original, Data->BIOSVersion, &StringIndex, NULL);
  //
  // Leave BiosSegment as 0 to ensure checks for IODeviceTree:/rom@0 or IODeviceTree:/rom@e0000 are met.
  //
  SMBIOS_OVERRIDE_S (Table, Standard.Type0->BiosReleaseDate, Original, Data->BIOSReleaseDate, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type0->BiosSize, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type0->BiosCharacteristics, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type0->BIOSCharacteristicsExtensionBytes, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type0->SystemBiosMajorRelease, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type0->SystemBiosMinorRelease, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type0->EmbeddedControllerFirmwareMajorRelease, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type0->EmbeddedControllerFirmwareMinorRelease, Original, NULL, NULL);

  IsApple = FALSE;

  Vendor = SmbiosGetString (Table->CurrentPtr, (Table->CurrentPtr).Standard.Type0->Vendor);

  if (Vendor != NULL && AsciiStrStr (Vendor, "Apple") != NULL) {
    IsApple = TRUE;
  }

  DEBUG ((DEBUG_INFO, "OCSMB: Post-override BIOS vendor %a %d\n", Vendor, IsApple));

  SmbiosFinaliseStruct (Table);

  return IsApple;
}

/** Type 1

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchSystemInformation (
  IN OUT OC_SMBIOS_TABLE *Table,
  IN     OC_SMBIOS_DATA  *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original    = SmbiosGetOriginalStructure (SMBIOS_TYPE_SYSTEM_INFORMATION, 1);
  MinLength   = sizeof (*Original.Standard.Type1);
  StringIndex = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_SYSTEM_INFORMATION, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type1->Manufacturer, Original, Data->SystemManufacturer, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type1->ProductName, Original, Data->SystemProductName, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type1->Version, Original, Data->SystemVersion, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type1->SerialNumber, Original, Data->SystemSerialNumber, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type1->Uuid, Original, Data->SystemUUID, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type1->WakeUpType, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type1->SKUNumber, Original, Data->SystemSKUNumber, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type1->Family, Original, Data->SystemFamily, &StringIndex, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 2

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchBaseboardInformation (
  IN OUT OC_SMBIOS_TABLE *Table,
  IN     OC_SMBIOS_DATA  *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original      = SmbiosGetOriginalStructure (SMBIOS_TYPE_BASEBOARD_INFORMATION, 1);
  MinLength     = sizeof (*Original.Standard.Type2);
  StringIndex   = 0;

  //
  // Hack for EDK 2 not using flexible array members adding extra data at struct end.
  //
  MinLength -= sizeof (Original.Standard.Type2->ContainedObjectHandles[0]);
  STATIC_ASSERT (
    sizeof (Original.Standard.Type2->ContainedObjectHandles) == sizeof (UINT16),
    "Please remove this hack, the structure should have been fixed by now."
    );

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_BASEBOARD_INFORMATION, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type2->Manufacturer, Original, Data->BoardManufacturer, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type2->ProductName, Original, Data->BoardProduct, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type2->Version, Original, Data->BoardVersion, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type2->SerialNumber, Original, Data->BoardSerialNumber, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type2->AssetTag, Original, Data->BoardAssetTag, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type2->FeatureFlag, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type2->LocationInChassis, Original, Data->BoardLocationInChassis, &StringIndex, NULL);
  Table->CurrentPtr.Standard.Type2->ChassisHandle = OcSmbiosSystemEnclosureHandle;
  SMBIOS_OVERRIDE_V (Table, Standard.Type2->BoardType, Original, Data->BoardType, NULL);

  //
  // Leave NumberOfContainedObjectHandles as 0, just like Apple does.
  //

  SmbiosFinaliseStruct (Table);
}

/** Type 3

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchSystemEnclosure (
  IN OUT OC_SMBIOS_TABLE *Table,
  IN     OC_SMBIOS_DATA  *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original      = SmbiosGetOriginalStructure (SMBIOS_TYPE_SYSTEM_ENCLOSURE, 1);
  MinLength     = sizeof (*Original.Standard.Type3);
  StringIndex   = 0;

  //
  // Hack for EDK 2 not using flexible array members adding extra data at struct end.
  //
  MinLength -= sizeof (Original.Standard.Type3->ContainedElements[0]);
  STATIC_ASSERT (
    sizeof (Original.Standard.Type3->ContainedElements) == sizeof (UINT8)*3,
    "Please remove this hack, the structure should have been fixed by now."
    );

  //
  // Another hack for omitted SKUNumber after flexible array.
  // Note, Apple just ignores this field and just writes Type3 as is.
  //
  MinLength += sizeof (SMBIOS_TABLE_STRING);
  STATIC_ASSERT (
    sizeof (*Original.Standard.Type3) == sizeof (UINT8) * 0x18,
    "The structure has changed, please fix this up."
    );

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_SYSTEM_ENCLOSURE, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type3->Manufacturer, Original, Data->ChassisManufacturer, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->Type, Original, Data->ChassisType, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type3->Version, Original, Data->ChassisVersion, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type3->SerialNumber, Original, Data->ChassisSerialNumber, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type3->AssetTag, Original, Data->ChassisAssetTag, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->BootupState, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->PowerSupplyState, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->ThermalState, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->SecurityStatus, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->OemDefined, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->Height, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->NumberofPowerCords, Original, NULL, NULL);

  //
  // Leave ContainedElementCount and ContainedElementRecordLength 0.
  // Also do not assign anything to SKUNumber.
  //

  SmbiosFinaliseStruct (Table);
}

/** Type 7

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
  @param[in] Index                  Pointer to cache table index.
  @param[in] OriginalHandle         Original cache table handle.
  @param[in] NewHandle              Pointer to new cache table handle.
**/
STATIC
VOID
PatchCacheInformation (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN OUT UINT16           *Index,
  IN     SMBIOS_HANDLE    OriginalHandle,
     OUT SMBIOS_HANDLE    *NewHandle
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT16                          NumberEntries;
  UINT16                          EntryNo;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  //
  // A handle of 0xFFFF indicates no cache data present.
  //
  *NewHandle = 0xFFFF;
  if (OriginalHandle == 0xFFFF) {
    return;
  }

  NumberEntries = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_CACHE_INFORMATION);

  DEBUG ((DEBUG_INFO, "OCSMB: Number of CPU cache entries is %u\n", (UINT32) NumberEntries));

  //
  // Locate original cache data specified by the handle.
  //
  for (EntryNo = 1; EntryNo <= NumberEntries; ++EntryNo) {
    Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_CACHE_INFORMATION, EntryNo);
    if (Original.Raw == NULL
      || !SMBIOS_ACCESSIBLE (Original, Standard.Type7->CacheConfiguration)
      || Original.Standard.Type7->Hdr.Handle != OriginalHandle) {
      continue;
    }

    MinLength     = sizeof (*Original.Standard.Type7);
    StringIndex   = 0;

    if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_CACHE_INFORMATION, MinLength, *Index))) {
      return;
    }

    SMBIOS_OVERRIDE_S (Table, Standard.Type7->SocketDesignation, Original, NULL, &StringIndex, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->CacheConfiguration, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->MaximumCacheSize, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->InstalledSize, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->SupportedSRAMType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->CurrentSRAMType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->CacheSpeed, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->ErrorCorrectionType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->SystemCacheType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->Associativity, Original, NULL, NULL);

    //
    // MaximumCacheSize2 and InstalledSize2 are to be set to the same values
    // as MaximumCacheSize and InstalledSize if the cache size is under 2047MB.
    //
    if (!SMBIOS_ACCESSIBLE (Original, Standard.Type7->MaximumCacheSize2)) {
      Table->CurrentPtr.Standard.Type7->MaximumCacheSize2  = Table->CurrentPtr.Standard.Type7->MaximumCacheSize & 0x7FFF;
      Table->CurrentPtr.Standard.Type7->MaximumCacheSize2 |= Table->CurrentPtr.Standard.Type7->MaximumCacheSize & BIT15 ? BIT31 : 0;
      Table->CurrentPtr.Standard.Type7->InstalledSize2     = Table->CurrentPtr.Standard.Type7->InstalledSize & 0x7FFF;
      Table->CurrentPtr.Standard.Type7->InstalledSize2    |= Table->CurrentPtr.Standard.Type7->InstalledSize & BIT15 ? BIT31 : 0;
    } else {
      SMBIOS_OVERRIDE_V (Table, Standard.Type7->MaximumCacheSize2, Original, NULL, NULL);
      SMBIOS_OVERRIDE_V (Table, Standard.Type7->InstalledSize2, Original, NULL, NULL);
    }

    *NewHandle = Table->CurrentPtr.Standard.Hdr->Handle;
    (*Index)++;
    SmbiosFinaliseStruct (Table);
  }
}

/** Type 4

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.
**/
STATIC
VOID
PatchProcessorInformation (
  IN OUT OC_SMBIOS_TABLE *Table,
  IN     OC_SMBIOS_DATA  *Data,
  IN     OC_CPU_INFO     *CpuInfo
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT16                          NumberEntries;
  UINT16                          EntryNo;
  UINT8                           MinLength;
  UINT8                           StringIndex;
  UINT8                           TmpCount;
  UINT16                          MhzSpeed;

  SMBIOS_HANDLE                   HandleL1Cache;
  SMBIOS_HANDLE                   HandleL2Cache;
  SMBIOS_HANDLE                   HandleL3Cache;
  UINT16                          CacheIndex;

  NumberEntries = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_PROCESSOR_INFORMATION);
  CacheIndex    = 0;

  for (EntryNo = 1; EntryNo <= NumberEntries; EntryNo++) {
    Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_PROCESSOR_INFORMATION, EntryNo);
    if (Original.Raw == NULL) {
      continue;
    }

    //
    // Create cache tables.
    //
    PatchCacheInformation (Table, Data, &CacheIndex, Original.Standard.Type4->L1CacheHandle, &HandleL1Cache);
    PatchCacheInformation (Table, Data, &CacheIndex, Original.Standard.Type4->L2CacheHandle, &HandleL2Cache);
    PatchCacheInformation (Table, Data, &CacheIndex, Original.Standard.Type4->L3CacheHandle, &HandleL3Cache);

    MinLength     = sizeof (*Original.Standard.Type4);
    StringIndex   = 0;

    if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_PROCESSOR_INFORMATION, MinLength, EntryNo))) {
      continue;
    }

    SMBIOS_OVERRIDE_S (Table, Standard.Type4->Socket, Original, NULL, &StringIndex, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorFamily, Original, NULL, NULL);
    SMBIOS_OVERRIDE_S (Table, Standard.Type4->ProcessorManufacturer, Original, NULL, &StringIndex, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorId, Original, NULL, NULL);
    SMBIOS_OVERRIDE_S (Table, Standard.Type4->ProcessorVersion, Original, NULL, &StringIndex, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->Voltage, Original, NULL, NULL);
    Table->CurrentPtr.Standard.Type4->ExternalClock = CpuInfo->ExternalClock;

    MhzSpeed = OcCpuFrequencyToDisplayFrequency (CpuInfo->CPUFrequency);

    DEBUG ((DEBUG_INFO, "OCSMB: CPU%u display frequency is %uMHz\n", EntryNo, MhzSpeed));

    Table->CurrentPtr.Standard.Type4->MaxSpeed      = MhzSpeed;
    Table->CurrentPtr.Standard.Type4->CurrentSpeed  = Table->CurrentPtr.Standard.Type4->MaxSpeed;

    SMBIOS_OVERRIDE_V (Table, Standard.Type4->Status, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorUpgrade, Original, NULL, NULL);

    Table->CurrentPtr.Standard.Type4->L1CacheHandle = HandleL1Cache;
    Table->CurrentPtr.Standard.Type4->L2CacheHandle = HandleL2Cache;
    Table->CurrentPtr.Standard.Type4->L3CacheHandle = HandleL3Cache;

    SMBIOS_OVERRIDE_S (Table, Standard.Type4->SerialNumber, Original, NULL, &StringIndex, NULL);
    //
    // Most bootloaders set this to ProcessorVersion, yet Apple does not care and has UNKNOWN.
    //
    SMBIOS_OVERRIDE_S (Table, Standard.Type4->AssetTag, Original, NULL, &StringIndex, NULL);
    SMBIOS_OVERRIDE_S (Table, Standard.Type4->PartNumber, Original, NULL, &StringIndex, NULL);

    TmpCount = (UINT8) (CpuInfo->CoreCount < 256 ? CpuInfo->CoreCount : 0xFF);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->CoreCount, Original, NULL, &TmpCount);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->EnabledCoreCount, Original, NULL, &TmpCount);
    TmpCount = (UINT8) (CpuInfo->ThreadCount < 256 ? CpuInfo->ThreadCount : 0xFF);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ThreadCount, Original, NULL, &TmpCount);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorCharacteristics, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorFamily2, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->CoreCount2, Original, NULL, &CpuInfo->CoreCount);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->EnabledCoreCount2, Original, NULL, &CpuInfo->CoreCount);
    SMBIOS_OVERRIDE_V (Table, Standard.Type4->ThreadCount2, Original, NULL, &CpuInfo->ThreadCount);

    SmbiosFinaliseStruct (Table);
  }
}

/** Type 8

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchSystemPorts (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT16                          NumberEntries;
  UINT16                          EntryNo;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  NumberEntries = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION);

  for (EntryNo = 1; EntryNo <= NumberEntries; EntryNo++) {
    Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, EntryNo);
    if (Original.Raw == NULL) {
      continue;
    }

    MinLength     = sizeof (*Original.Standard.Type8);
    StringIndex   = 0;

    if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, MinLength, EntryNo))) {
      continue;
    }

    SMBIOS_OVERRIDE_S (Table, Standard.Type8->InternalReferenceDesignator, Original, NULL, &StringIndex, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type8->InternalConnectorType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_S (Table, Standard.Type8->ExternalReferenceDesignator, Original, NULL, &StringIndex, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type8->ExternalConnectorType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type8->PortType, Original, NULL, NULL);

    SmbiosFinaliseStruct (Table);
  }
}

/** Type 9

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchSystemSlots (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER   Original;
  UINT16                           NumberEntries;
  UINT16                           EntryNo;
  UINT8                            MinLength;
  UINT8                            StringIndex;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo;
  EFI_STATUS                       Status;
  UINT64                           PciAddress;
  UINT8                            SecSubBus;
  UINT16                           ClassCode;
  CONST CHAR8                      *SlotDesignation;

  Status = gBS->LocateProtocol (
    &gEfiPciRootBridgeIoProtocolGuid,
    NULL,
    (VOID **) &PciRootBridgeIo
    );
  if (EFI_ERROR (Status)) {
    PciRootBridgeIo = NULL;
  }

  NumberEntries = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_SYSTEM_SLOTS);

  for (EntryNo = 1; EntryNo <= NumberEntries; EntryNo++) {
    Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_SYSTEM_SLOTS, EntryNo);
    if (Original.Raw == NULL) {
      continue;
    }

    MinLength     = sizeof (*Original.Standard.Type9);
    StringIndex   = 0;

    if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_SYSTEM_SLOTS, MinLength, EntryNo))) {
      continue;
    }

    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SlotType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SlotDataBusWidth, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SlotLength, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SlotID, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SlotType, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SlotCharacteristics1, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SlotCharacteristics2, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->SegmentGroupNum, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->BusNum, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type9->DevFuncNum, Original, NULL, NULL);

    //
    // Correct SlotDesignation and CurrentUsage if PCI protocol is available.
    //

    if (PciRootBridgeIo == NULL) {
      SMBIOS_OVERRIDE_S (Table, Standard.Type9->SlotDesignation, Original, NULL, &StringIndex, NULL);
      SMBIOS_OVERRIDE_V (Table, Standard.Type9->CurrentUsage, Original, NULL, NULL);
    } else {
      SlotDesignation = NULL;
      Table->CurrentPtr.Standard.Type9->CurrentUsage = SlotUsageAvailable;

      if (Table->CurrentPtr.Standard.Type9->BusNum != 0) {
        PciAddress = EFI_PCI_ADDRESS (
          BitFieldRead8 (Table->CurrentPtr.Standard.Type9->DevFuncNum, 3, 7),
          Table->CurrentPtr.Standard.Type9->BusNum,
          BitFieldRead8 (Table->CurrentPtr.Standard.Type9->DevFuncNum, 0, 2),
          0
          );
      } else {
        PciAddress = EFI_PCI_ADDRESS (
          Table->CurrentPtr.Standard.Type9->BusNum,
          BitFieldRead8 (Table->CurrentPtr.Standard.Type9->DevFuncNum, 3, 7),
          BitFieldRead8 (Table->CurrentPtr.Standard.Type9->DevFuncNum, 0, 2),
          0
          );
      }

      SecSubBus = MAX_UINT8;
      ClassCode = MAX_UINT16;
      Status = PciRootBridgeIo->Pci.Read (
        PciRootBridgeIo,
        EfiPciWidthUint8,
        PciAddress + PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET,
        1,
        &SecSubBus
        );

      if (!EFI_ERROR (Status) && SecSubBus != MAX_UINT8) {
        Status = PciRootBridgeIo->Pci.Read (
          PciRootBridgeIo,
          EfiPciWidthUint16,
          EFI_PCI_ADDRESS (SecSubBus, 0, 0, 0) + 10,
          1,
          &ClassCode
          );
      }

      if (!EFI_ERROR (Status) && ClassCode != MAX_UINT16) {
        Table->CurrentPtr.Standard.Type9->CurrentUsage = SlotUsageAvailable;
        if (ClassCode == 0x0280) {
          SlotDesignation = "AirPort";
        } else if (ClassCode == 0x0108) {
          //
          // FIXME: Expand checks since not all nvme uses m.2 slot
          //
          SlotDesignation = "M.2";
        }
      }

      SMBIOS_OVERRIDE_S (Table, Standard.Type9->SlotDesignation, Original, SlotDesignation, &StringIndex, NULL);
    }

    SmbiosFinaliseStruct (Table);
  }
}

/** Type 16

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchMemoryArray (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
  IN     APPLE_SMBIOS_STRUCTURE_POINTER  Original,
  IN     UINT16                          Index,
     OUT SMBIOS_HANDLE                   *Handle
  )
{
  UINT8    MinLength;

  *Handle       = OcSmbiosInvalidHandle;
  MinLength     = sizeof (*Original.Standard.Type16);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, MinLength, Index))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type16->Location, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->Use, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->MemoryErrorCorrection, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->MaximumCapacity, Original, NULL, NULL);
  //
  // Do not support memory error information. 0xFFFF indicates no errors previously detected.
  //
  Table->CurrentPtr.Standard.Type16->MemoryErrorInformationHandle = 0xFFFF;
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->NumberOfMemoryDevices, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->ExtendedMaximumCapacity, Original, NULL, NULL);

  //
  // Return assigned handle
  //
  *Handle = Table->CurrentPtr.Standard.Hdr->Handle;

  SmbiosFinaliseStruct (Table);
}

/** Type 16

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
CreateMemoryArray (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
     OUT SMBIOS_HANDLE                   *Handle
  )
{
  UINT8    MinLength;

  *Handle       = OcSmbiosInvalidHandle;
  MinLength     = sizeof (*Table->CurrentPtr.Standard.Type16);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, MinLength, 1))) {
    return;
  }
  Table->CurrentPtr.Standard.Type16->Location               = MemoryArrayLocationSystemBoard;
  Table->CurrentPtr.Standard.Type16->Use                    = MemoryArrayUseSystemMemory;
  Table->CurrentPtr.Standard.Type16->MemoryErrorCorrection  = *Data->MemoryErrorCorrection;
  //
  // Do not support memory error information. 0xFFFF indicates no errors previously detected.
  //
  Table->CurrentPtr.Standard.Type16->MemoryErrorInformationHandle = 0xFFFF;
  Table->CurrentPtr.Standard.Type16->NumberOfMemoryDevices        = Data->MemoryDevicesCount;

  //
  // Sizes over 2TB need to use the Extended Maximum Capacity field which is represented in bytes.
  // Maximum Capacity is represented in KB.
  //
  if (*Data->MemoryMaxCapacity < SIZE_2TB) {
    Table->CurrentPtr.Standard.Type16->MaximumCapacity          = (UINT32)(*Data->MemoryMaxCapacity / SIZE_1KB);
    Table->CurrentPtr.Standard.Type16->ExtendedMaximumCapacity  = 0;
  } else {
    Table->CurrentPtr.Standard.Type16->MaximumCapacity          = SIZE_2TB / SIZE_1KB;
    Table->CurrentPtr.Standard.Type16->ExtendedMaximumCapacity  = *Data->MemoryMaxCapacity;
  }

  //
  // Return assigned handle
  //
  *Handle = Table->CurrentPtr.Standard.Hdr->Handle;

  SmbiosFinaliseStruct (Table);
}

/** Type 17

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchMemoryDevice (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
  IN     SMBIOS_HANDLE                   MemoryArrayHandle,
  IN     APPLE_SMBIOS_STRUCTURE_POINTER  Original,
  IN     UINT16                          Index,
     OUT SMBIOS_HANDLE                   *Handle
  )
{
  UINT8        MinLength;
  UINT8        StringIndex;
  UINT8        FormFactor;

  *Handle       = OcSmbiosInvalidHandle;
  MinLength     = sizeof (*Original.Standard.Type17);
  StringIndex   = 0;
  FormFactor    = SmbiosGetFormFactor (Data, Original);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_MEMORY_DEVICE, MinLength, Index))) {
    return;
  }

  //
  // External handles are set below.
  //
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->TotalWidth, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->DataWidth, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->Size, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->FormFactor, Original, &FormFactor, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->DeviceSet, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type17->DeviceLocator, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type17->BankLocator, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->MemoryType, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->TypeDetail, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->Speed, Original, NULL, NULL);

  //
  // Assign the parent memory array handle.
  // Do not support memory error information. 0xFFFF indicates no errors previously detected.
  //
  Table->CurrentPtr.Standard.Type17->MemoryArrayHandle            = MemoryArrayHandle;
  Table->CurrentPtr.Standard.Type17->MemoryErrorInformationHandle = 0xFFFF;

  //
  // Some machines may have NULL values for these fields, which will cause SPMemoryReporter
  // crashes or ??? to be displayed in About This Mac. Fallback to "Unknown" for such fields.
  // If there is no stick in the slot, the Manufacturer value must be "NO DIMM", this
  // is checked in System Profiler, at least by iMacPro1,1 (see ExpansionSlotSupport.framework).
  //
  if (Table->CurrentPtr.Standard.Type17->Size > 0) {
    SMBIOS_OVERRIDE_S (Table, Standard.Type17->Manufacturer, Original, NULL, &StringIndex, "Unknown");
    SMBIOS_OVERRIDE_S (Table, Standard.Type17->SerialNumber, Original, NULL, &StringIndex, "Unknown");
    SMBIOS_OVERRIDE_S (Table, Standard.Type17->AssetTag, Original, NULL, &StringIndex, "Unknown");
    SMBIOS_OVERRIDE_S (Table, Standard.Type17->PartNumber, Original, NULL, &StringIndex, "Unknown");
  } else {
    SmbiosOverrideString (Table, "NO DIMM", &StringIndex);
    Table->CurrentPtr.Standard.Type17->Manufacturer = StringIndex;
    Table->CurrentPtr.Standard.Type17->SerialNumber = StringIndex;
    Table->CurrentPtr.Standard.Type17->AssetTag     = StringIndex;
    Table->CurrentPtr.Standard.Type17->PartNumber   = StringIndex;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type17->Attributes, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->ExtendedSize, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->ConfiguredMemoryClockSpeed, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->MinimumVoltage, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->MaximumVoltage, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->ConfiguredVoltage, Original, NULL, NULL);

  //
  // Return assigned handle
  //
  *Handle = Table->CurrentPtr.Standard.Hdr->Handle;

  SmbiosFinaliseStruct (Table);
}

/** Type 17

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
CreateMemoryDevice (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
  IN     OC_SMBIOS_MEMORY_DEVICE_DATA    *DeviceData,
  IN     SMBIOS_HANDLE                   MemoryArrayHandle,
  IN     UINT16                          Index,
     OUT SMBIOS_HANDLE                   *Handle
  )
{
  UINT8    MinLength;
  UINT8    StringIndex;

  *Handle       = OcSmbiosInvalidHandle;
  MinLength     = sizeof (*Table->CurrentPtr.Standard.Type17);
  StringIndex   = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_MEMORY_DEVICE, MinLength, Index))) {
    return;
  }

  Table->CurrentPtr.Standard.Type17->TotalWidth = *Data->MemoryTotalWidth;
  Table->CurrentPtr.Standard.Type17->DataWidth  = *Data->MemoryDataWidth;
  Table->CurrentPtr.Standard.Type17->FormFactor = *Data->MemoryFormFactor;
  Table->CurrentPtr.Standard.Type17->MemoryType = *Data->MemoryType;
  Table->CurrentPtr.Standard.Type17->TypeDetail = *((MEMORY_DEVICE_TYPE_DETAIL *)Data->MemoryTypeDetail);
  Table->CurrentPtr.Standard.Type17->Speed      = *DeviceData->Speed;

  //
  // Sizes over 32GB-1MB need to be represented in the Extended Size field.
  // Both will be represented in MB.
  //
  if ((UINT64)(* DeviceData->Size) * SIZE_1MB < SIZE_32GB - SIZE_1MB) {
    Table->CurrentPtr.Standard.Type17->Size         = (UINT16)*DeviceData->Size;
    Table->CurrentPtr.Standard.Type17->ExtendedSize = 0;
  } else {
    Table->CurrentPtr.Standard.Type17->Size         = 0x7FFF;
    Table->CurrentPtr.Standard.Type17->ExtendedSize = *DeviceData->Size;
  }

  SmbiosOverrideString (Table, DeviceData->AssetTag, &StringIndex);
  Table->CurrentPtr.Standard.Type17->AssetTag       = StringIndex;
  SmbiosOverrideString (Table, DeviceData->BankLocator, &StringIndex);
  Table->CurrentPtr.Standard.Type17->BankLocator    = StringIndex;
  SmbiosOverrideString (Table, DeviceData->DeviceLocator, &StringIndex);
  Table->CurrentPtr.Standard.Type17->DeviceLocator  = StringIndex;
  SmbiosOverrideString (Table, DeviceData->Manufacturer, &StringIndex);
  Table->CurrentPtr.Standard.Type17->Manufacturer   = StringIndex;
  SmbiosOverrideString (Table, DeviceData->PartNumber, &StringIndex);
  Table->CurrentPtr.Standard.Type17->PartNumber     = StringIndex;
  SmbiosOverrideString (Table, DeviceData->SerialNumber, &StringIndex);
  Table->CurrentPtr.Standard.Type17->SerialNumber   = StringIndex;

  //
  // Assign the parent memory array handle.
  // Do not support memory error information. 0xFFFF indicates no errors previously detected.
  //
  Table->CurrentPtr.Standard.Type17->MemoryArrayHandle            = MemoryArrayHandle;
  Table->CurrentPtr.Standard.Type17->MemoryErrorInformationHandle = 0xFFFF;

  //
  // Return assigned handle
  //
  *Handle = Table->CurrentPtr.Standard.Hdr->Handle;

  SmbiosFinaliseStruct (Table);
}

/** Type 19

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchMemoryMappedAddress (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
  IN     SMBIOS_HANDLE                   MemoryArrayHandle,
  IN     APPLE_SMBIOS_STRUCTURE_POINTER  Original,
  IN     UINT16                          Index,
  IN OUT OC_SMBIOS_MAPPING               *Mapping,
  IN OUT UINT16                          *MappingNum
  )
{
  UINT8   MinLength;

  MinLength = sizeof (*Original.Standard.Type19);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, MinLength, Index))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type19->StartingAddress, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type19->EndingAddress, Original, NULL, NULL);
  Table->CurrentPtr.Standard.Type19->MemoryArrayHandle = MemoryArrayHandle;
  SMBIOS_OVERRIDE_V (Table, Standard.Type19->PartitionWidth, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type19->ExtendedStartingAddress, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type19->ExtendedEndingAddress, Original, NULL, NULL);

  if (*MappingNum < OC_SMBIOS_MAX_MAPPING) {
    Mapping[*MappingNum].Old = Original.Standard.Hdr->Handle;
    Mapping[*MappingNum].New = Table->CurrentPtr.Standard.Hdr->Handle;
    (*MappingNum)++;
  } else {
    //
    // The value is reasonably large enough for this to never happen, yet just in case.
    //
    DEBUG ((DEBUG_WARN, "OCSMB: OC_SMBIOS_MAX_MAPPING exceeded\n"));
  }

  SmbiosFinaliseStruct (Table);
}

/** Type 20

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchMemoryMappedDevice (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
  IN     APPLE_SMBIOS_STRUCTURE_POINTER  Original,
  IN     UINT16                          Index,
  IN     SMBIOS_HANDLE                   MemoryDeviceHandle,
  IN     OC_SMBIOS_MAPPING               *Mapping,
  IN     UINT16                          MappingNum
  )
{
  UINT8    MinLength;
  UINT16   MapIndex;

  MinLength     = sizeof (*Original.Standard.Type20);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, MinLength, Index))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type20->StartingAddress, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->EndingAddress, Original, NULL, NULL);
  Table->CurrentPtr.Standard.Type20->MemoryDeviceHandle = MemoryDeviceHandle;

  Table->CurrentPtr.Standard.Type20->MemoryArrayMappedAddressHandle = 0xFFFF;
  if (Original.Raw != NULL && SMBIOS_ACCESSIBLE (Original, Standard.Type20->MemoryArrayMappedAddressHandle)) {
    for (MapIndex = 0; MapIndex < MappingNum; MapIndex++) {
      if (Mapping[MapIndex].Old == Original.Standard.Type20->MemoryArrayMappedAddressHandle) {
        Table->CurrentPtr.Standard.Type20->MemoryArrayMappedAddressHandle = Mapping[MapIndex].New;
        break;
      }
    }
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type20->PartitionRowPosition, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->InterleavePosition, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->InterleavedDataDepth, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->ExtendedStartingAddress, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->ExtendedEndingAddress, Original, NULL, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 22

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchPortableBatteryDevice (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original    = SmbiosGetOriginalStructure (SMBIOS_TYPE_PORTABLE_BATTERY, 1);
  MinLength   = sizeof (*Original.Standard.Type22);
  StringIndex = 0;

  //
  // Do not add batteries where it does not exist (e.g. desktop).
  //
  if (Original.Raw == NULL) {
    return;
  }

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_PORTABLE_BATTERY, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type22->Location, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type22->Manufacturer, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type22->ManufactureDate, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type22->SerialNumber, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type22->DeviceName, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->DeviceChemistry, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->DeviceCapacity, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->DesignVoltage, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type22->SBDSVersionNumber, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->MaximumErrorInBatteryData, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->SBDSSerialNumber, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->SBDSManufactureDate, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type22->SBDSDeviceChemistry, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->DesignCapacityMultiplier, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type22->OEMSpecific, Original, NULL, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 32

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
PatchBootInformation (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;

  Original    = SmbiosGetOriginalStructure (SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, 1);
  MinLength   = sizeof (*Original.Standard.Type32);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type32->Reserved, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type32->BootStatus, Original, NULL, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 128

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
  @param[in] HasAppleSMBIOS         TRUE if Apple SMBIOS present.
**/
STATIC
VOID
CreateOrPatchAppleFirmwareVolume (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN     BOOLEAN          HasAppleSMBIOS
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;

  UINT32 FirmwareFeatures;
  UINT32 FirmwareFeaturesMask;
  UINT32 ExtendedFirmwareFeatures;
  UINT32 ExtendedFirmwareFeaturesMask;

  if (HasAppleSMBIOS) {
    Original      = SmbiosGetOriginalStructure (APPLE_SMBIOS_TYPE_FIRMWARE_INFORMATION, 1);
  } else {
    Original.Raw  = NULL;
  }

  MinLength   = sizeof (*Table->CurrentPtr.Type128);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_FIRMWARE_INFORMATION, MinLength, 1))) {
    return;
  }

  FirmwareFeatures             = (UINT32) BitFieldRead64 (Data->FirmwareFeatures    ,  0, 31);
  FirmwareFeaturesMask         = (UINT32) BitFieldRead64 (Data->FirmwareFeaturesMask,  0, 31);
  ExtendedFirmwareFeatures     = (UINT32) BitFieldRead64 (Data->FirmwareFeatures    , 32, 63);
  ExtendedFirmwareFeaturesMask = (UINT32) BitFieldRead64 (Data->FirmwareFeaturesMask, 32, 63);

  SMBIOS_OVERRIDE_V (Table, Type128->FirmwareFeatures            , Original, Data->FirmwareFeatures     == 0 ? NULL : &FirmwareFeatures            , NULL);
  SMBIOS_OVERRIDE_V (Table, Type128->FirmwareFeaturesMask        , Original, Data->FirmwareFeaturesMask == 0 ? NULL : &FirmwareFeaturesMask        , NULL);
  SMBIOS_OVERRIDE_V (Table, Type128->ExtendedFirmwareFeatures    , Original, Data->FirmwareFeatures     == 0 ? NULL : &ExtendedFirmwareFeatures    , NULL);
  SMBIOS_OVERRIDE_V (Table, Type128->ExtendedFirmwareFeaturesMask, Original, Data->FirmwareFeaturesMask == 0 ? NULL : &ExtendedFirmwareFeaturesMask, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 131

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.
**/
STATIC
VOID
CreateAppleProcessorType (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN     OC_CPU_INFO      *CpuInfo
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;

  Original.Raw = NULL;
  MinLength    = sizeof (*Table->CurrentPtr.Type131);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_PROCESSOR_TYPE, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Type131->ProcessorType.Type, Original, Data->ProcessorType, &CpuInfo->AppleProcessorType);

  SmbiosFinaliseStruct (Table);
}

/** Type 132

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.
**/
STATIC
VOID
CreateAppleProcessorSpeed (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN     OC_CPU_INFO      *CpuInfo
  )
{
#ifndef OC_PROVIDE_APPLE_PROCESSOR_BUS_SPEED
  //
  // This table is not added in all modern Macs.
  //
  (VOID) Table;
  (VOID) Data;
  (VOID) CpuInfo;
#else
  UINT8                           MinLength;

  MinLength   = sizeof (*Table->CurrentPtr.Type132);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_PROCESSOR_BUS_SPEED, MinLength, 1))) {
    return;
  }

  Table->CurrentPtr.Type132->ProcessorBusSpeed = CpuInfo->ExternalClock * 4;

  SmbiosFinaliseStruct (Table);
#endif
}

/** Type 133

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
  @param[in] HasAppleSMBIOS         TRUE if Apple SMBIOS present.
**/
STATIC
VOID
CreateOrPatchApplePlatformFeature (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN     BOOLEAN          HasAppleSMBIOS
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;

  if (HasAppleSMBIOS) {
    Original      = SmbiosGetOriginalStructure (APPLE_SMBIOS_TYPE_PLATFORM_FEATURE, 1);
  } else {
    Original.Raw  = NULL;
  }

  //
  // Older Macs do not support PlatformFeature table.
  //
  if (Data->PlatformFeature == NULL && Original.Raw == NULL) {
    return;
  }

  MinLength   = sizeof (*Table->CurrentPtr.Type133);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_PLATFORM_FEATURE, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Type133->PlatformFeature, Original, Data->PlatformFeature, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 134

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
  @param[in] HasAppleSMBIOS         TRUE if Apple SMBIOS present.
**/
STATIC
VOID
CreateOrPatchAppleSmcInformation (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN     BOOLEAN          HasAppleSMBIOS
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;

  if (HasAppleSMBIOS) {
    Original      = SmbiosGetOriginalStructure (APPLE_SMBIOS_TYPE_SMC_INFORMATION, 1);
  } else {
    Original.Raw  = NULL;
  }

  //
  // Newer Macs do not support SmcVersion table.
  //
  if (Data->SmcVersion == NULL && Original.Raw == NULL) {
    return;
  }

  MinLength   = sizeof (*Table->CurrentPtr.Type134);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_SMC_INFORMATION, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Type134->SmcVersion, Original, Data->SmcVersion, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 127

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Data                   Pointer to location containing SMBIOS data.
**/
STATIC
VOID
CreateSmBiosEndOfTable (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  UINT8                           MinLength;

  MinLength   = sizeof (*Table->CurrentPtr.Standard.Type127);

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_END_OF_TABLE, MinLength, 1))) {
    return;
  }

  SmbiosFinaliseStruct (Table);
}

/**
  Lock or unlock legacy region if used

  @param  Unlock  TRUE to unlock, FALSE to lock
**/
STATIC
EFI_STATUS
SmbiosHandleLegacyRegion (
  BOOLEAN  Unlock
  )
{
  EFI_STATUS  Status;

  //
  // Not needed for mOriginalSmbios3.
  //
  if (mOriginalSmbios == NULL) {
    return EFI_SUCCESS;
  }

  if (Unlock) {
    if (((UINTN) mOriginalSmbios) < BASE_1MB) {
      //
      // Enable write access to DMI anchor
      //
      Status = LegacyRegionUnlock (
        (UINT32) ((UINTN) mOriginalSmbios & 0xFFFF8000ULL),
        EFI_PAGE_SIZE
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMB: LegacyRegionUnlock DMI anchor failure - %r\n", Status));
        return Status;
      }
    }

    if (((UINTN) mOriginalSmbios->TableAddress) < BASE_1MB) {
      //
      // Enable write access to DMI table
      //
      Status = LegacyRegionUnlock (
        (UINT32) ((UINTN) mOriginalSmbios->TableAddress & 0xFFFF8000ULL),
        EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (mOriginalSmbios->TableLength))
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMB: LegacyRegionUnlock DMI table failure - %r\n", Status));
        return Status;
      }
    }
  } else {
    if ((UINTN) mOriginalSmbios->TableAddress < BASE_1MB) {
      //
      // Lock write access To DMI table
      //
      Status = LegacyRegionLock (
        (UINT32) ((UINTN) mOriginalSmbios->TableAddress & 0xFFFF8000ULL),
        EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (mOriginalSmbios->TableLength))
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMB: LegacyRegionLock DMI table failure - %r\n", Status));
        return Status;
      }
    }

    if ((UINTN) mOriginalSmbios < BASE_1MB) {
      //
      // Lock write access To DMI anchor
      //
      Status = LegacyRegionLock (
        (UINT32) ((UINTN) mOriginalSmbios & 0xFFFF8000ULL),
        EFI_PAGE_SIZE
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMB: LegacyRegionLock DMI anchor failure - %r\n", Status));
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcSmbiosTablePrepare (
  IN OUT OC_SMBIOS_TABLE  *SmbiosTable
  )
{
  EFI_STATUS  Status;

  mOriginalSmbios    = NULL;
  mOriginalSmbios3   = NULL;
  mOriginalTableSize = 0;
  mOriginalTable.Raw = NULL;
  ZeroMem (SmbiosTable, sizeof (*SmbiosTable));
  SmbiosTable->Handle = OcSmbiosAutomaticHandle;

  Status  = EfiGetSystemConfigurationTable (
              &gEfiSmbiosTableGuid,
              (VOID **) &mOriginalSmbios
              );

  //
  // Perform basic sanity checks. We assume EfiGetSystemConfigurationTable returns trusted data
  // in terms of file size at the very least, but try to detect potential issues.
  //
  if (!EFI_ERROR (Status)) {
    if (mOriginalSmbios->EntryPointLength < sizeof (SMBIOS_TABLE_ENTRY_POINT)) {
      DEBUG ((DEBUG_WARN, "OCSMB: SmbiosLookupHost entry is too small - %u/%u bytes\n",
        mOriginalSmbios->EntryPointLength, (UINT32) sizeof (SMBIOS_TABLE_ENTRY_POINT)));
      mOriginalSmbios = NULL;
    } else if (mOriginalSmbios->TableAddress == 0 || mOriginalSmbios->TableLength == 0) {
      STATIC_ASSERT (
        sizeof (mOriginalSmbios->TableLength) == sizeof (UINT16)
        && SMBIOS_TABLE_MAX_LENGTH == MAX_UINT16,
        "mOriginalTable->TableLength may exceed SMBIOS_TABLE_MAX_LENGTH"
        );
      DEBUG ((DEBUG_WARN, "OCSMB: SmbiosLookupHost entry has invalid table - %08X of %u bytes\n",
        mOriginalSmbios->TableAddress, mOriginalSmbios->TableLength));
      mOriginalSmbios = NULL;
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCSMB: SmbiosLookupHost failed to lookup SMBIOSv1 - %r\n", Status));
  }

  //
  // Do similar checks on SMBIOSv3.
  //
  Status  = EfiGetSystemConfigurationTable (
              &gEfiSmbios3TableGuid,
              (VOID **) &mOriginalSmbios3
              );

  if (!EFI_ERROR (Status)) {
    if (mOriginalSmbios3->EntryPointLength < sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT)) {
      DEBUG ((DEBUG_INFO, "OCSMB: SmbiosLookupHost v3 entry is too small - %u/%u bytes\n",
        mOriginalSmbios3->EntryPointLength, (UINT32) sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT)));
      mOriginalSmbios3 = NULL;
    } else if (mOriginalSmbios3->TableAddress == 0 || mOriginalSmbios3->TableMaximumSize == 0) {
      STATIC_ASSERT (
        sizeof (mOriginalSmbios3->TableMaximumSize) == sizeof (UINT32)
        && SMBIOS_3_0_TABLE_MAX_LENGTH == MAX_UINT32,
        "mOriginalSmbios3->TableMaximumSize may exceed SMBIOS_3_0_TABLE_MAX_LENGTH"
        );
      DEBUG ((DEBUG_INFO, "OCSMB: SmbiosLookupHost v3 entry has invalid table - %016LX of %u bytes\n",
        mOriginalSmbios3->TableAddress, mOriginalSmbios3->TableMaximumSize));
      mOriginalSmbios3 = NULL;
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCSMB: SmbiosLookupHost failed to lookup SMBIOSv3 - %r\n", Status));
  }

  //
  // TODO: I think we may want to try harder.
  //

  if (mOriginalSmbios != NULL) {
    mOriginalTableSize = mOriginalSmbios->TableLength;
    mOriginalTable.Raw = (UINT8 *)(UINTN) mOriginalSmbios->TableAddress;
  } else if (mOriginalSmbios3 != NULL) {
    mOriginalTableSize = mOriginalSmbios3->TableMaximumSize;
    mOriginalTable.Raw = (UINT8 *)(UINTN) mOriginalSmbios3->TableAddress;
  }

  if (mOriginalSmbios != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OCSMB: Found DMI Anchor %p v%u.%u Table Address %08LX Length %04X\n",
      mOriginalSmbios,
      mOriginalSmbios->MajorVersion,
      mOriginalSmbios->MinorVersion,
      (UINT64) mOriginalSmbios->TableAddress,
      mOriginalSmbios->TableLength
      ));
  }

  if (mOriginalSmbios3 != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OCSMB: Found DMI Anchor %p v%u.%u Table Address %08LX Length %04X\n",
      mOriginalSmbios3,
      mOriginalSmbios3->MajorVersion,
      mOriginalSmbios3->MinorVersion,
      mOriginalSmbios3->TableAddress,
      mOriginalSmbios3->TableMaximumSize
      ));
  }

  Status = SmbiosExtendTable (SmbiosTable, 1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "OCSMB: SmbiosLookupHost failed to initialise smbios table - %r\n", Status));
  }

  return Status;
}


VOID
OcSmbiosTableFree (
  IN OUT OC_SMBIOS_TABLE  *Table
  )
{
  if (Table->Table != NULL) {
    FreePool (Table->Table);
  }

  ZeroMem (Table, sizeof (*Table));
}

STATIC
EFI_STATUS
SmbiosTableAllocate (
  IN      UINT16                        TableLength,
  IN OUT  SMBIOS_TABLE_ENTRY_POINT      **TableEntryPoint,
  IN OUT  SMBIOS_TABLE_3_0_ENTRY_POINT  **TableEntryPoint3,
  IN OUT  VOID                          **TableAddress,
  IN OUT  VOID                          **TableAddress3
  )
{
  UINT16                TablePages;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  TmpAddr;

  TablePages         = EFI_SIZE_TO_PAGES (TableLength);
  *TableEntryPoint   = NULL;
  *TableEntryPoint3  = NULL;
  *TableAddress      = NULL;
  *TableAddress3     = NULL;

  TmpAddr = (BASE_4GB - 1);
  Status = gBS->AllocatePages (AllocateMaxAddress, EfiReservedMemoryType, TablePages, &TmpAddr);
  if (!EFI_ERROR (Status)) {
    *TableAddress = (VOID *)(UINTN)TmpAddr;
    TmpAddr = (BASE_4GB - 1);
    Status = gBS->AllocatePages (AllocateMaxAddress, EfiReservedMemoryType, 1,  &TmpAddr);
    if (!EFI_ERROR (Status)) {
      *TableEntryPoint = (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)TmpAddr;
      if (mOriginalSmbios3 != NULL) {
        TmpAddr = (BASE_4GB - 1);
        Status = gBS->AllocatePages (AllocateMaxAddress, EfiReservedMemoryType, 1, &TmpAddr);
        if (!EFI_ERROR (Status)) {
          *TableEntryPoint3 = (SMBIOS_TABLE_3_0_ENTRY_POINT *)(UINTN)TmpAddr;
          *TableAddress3 = *TableAddress;
        }
      }
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCSMB: SmbiosTableAllocate aborts as it cannot allocate SMBIOS pages with %d %d %d - %r\n",
      *TableEntryPoint != NULL, *TableAddress != NULL, *TableEntryPoint3 != NULL, Status));
    if (*TableEntryPoint != NULL) {
      FreePages (*TableEntryPoint, 1);
    }
    if (*TableEntryPoint3 != NULL) {
      FreePages (*TableEntryPoint3, 1);
    }
    if (*TableAddress != NULL) {
      FreePages (*TableAddress, TablePages);
    }
    return Status;
  }

  ZeroMem (*TableAddress, TablePages * EFI_PAGE_SIZE);
  ZeroMem (*TableEntryPoint, EFI_PAGE_SIZE);
  if (mOriginalSmbios3 != NULL) {
    ZeroMem (*TableEntryPoint3, EFI_PAGE_SIZE);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SmbiosTableApply (
  IN OUT OC_SMBIOS_TABLE        *SmbiosTable,
  IN     OC_SMBIOS_UPDATE_MODE  Mode
  )
{
  EFI_STATUS                    Status;
  UINT16                        TableLength;
  SMBIOS_TABLE_ENTRY_POINT      *TableEntryPoint;
  SMBIOS_TABLE_3_0_ENTRY_POINT  *TableEntryPoint3;
  VOID                          *TableAddress;
  VOID                          *TableAddress3;

  ASSERT (Mode == OcSmbiosUpdateCreate
    || Mode == OcSmbiosUpdateOverwrite
    || Mode == OcSmbiosUpdateCustom
    || Mode == OcSmbiosUpdateTryOverwrite
    );

  //
  // We check maximum table size during table extension, so this cast is valid.
  //
  TableLength = (UINT16) (SmbiosTable->CurrentPtr.Raw - SmbiosTable->Table);

  DEBUG ((
    DEBUG_INFO,
    "OCSMB: Applying %u (%d) prev %p (%u/%u), %p (%u/%u)\n",
    (UINT32) TableLength,
    Mode,
    mOriginalSmbios,
    (UINT32) (mOriginalSmbios != NULL ? mOriginalSmbios->TableLength : 0),
    (UINT32) (mOriginalSmbios != NULL ? mOriginalSmbios->EntryPointLength : 0),
    mOriginalSmbios3,
    (UINT32) (mOriginalSmbios3 != NULL ? mOriginalSmbios3->TableMaximumSize : 0),
    (UINT32) (mOriginalSmbios3 != NULL ? mOriginalSmbios3->EntryPointLength : 0)
    ));

  if (Mode == OcSmbiosUpdateTryOverwrite || Mode == OcSmbiosUpdateOverwrite) {
    Status = SmbiosHandleLegacyRegion (TRUE);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCSMB: Apply(%u) cannot handle legacy region - %r\n", Mode, Status));
      if (Mode == OcSmbiosUpdateOverwrite) {
        return Status;
      }

      //
      // Fallback to create mode.
      //
      Mode = OcSmbiosUpdateCreate;
    } else if (mOriginalSmbios == NULL || TableLength > mOriginalSmbios->TableLength
      || mOriginalSmbios->EntryPointLength < sizeof (SMBIOS_TABLE_ENTRY_POINT)) {
      DEBUG ((DEBUG_WARN, "OCSMB: Apply(%u) cannot update old SMBIOS (%p, %u, %u) with %u\n",
        Mode, mOriginalSmbios, mOriginalSmbios != NULL ? mOriginalSmbios->TableLength : 0,
        mOriginalSmbios != NULL ? mOriginalSmbios->EntryPointLength : 0, TableLength));
      if (Mode == OcSmbiosUpdateOverwrite) {
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Fallback to create mode.
      //
      Mode = OcSmbiosUpdateCreate;
    } else if (mOriginalSmbios3 != NULL && (TableLength > mOriginalSmbios3->TableMaximumSize
      || mOriginalSmbios3->EntryPointLength < sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT))) {
      DEBUG ((DEBUG_INFO, "OCSMB: Apply(%u) cannot fit SMBIOSv3 (%p, %u, %u) with %u\n",
        Mode, mOriginalSmbios3, mOriginalSmbios3->EntryPointLength, mOriginalSmbios3->TableMaximumSize, TableLength));
      if (Mode == OcSmbiosUpdateOverwrite) {
        return EFI_OUT_OF_RESOURCES;
      }
      Mode = OcSmbiosUpdateCreate;
    } else {
      Mode = OcSmbiosUpdateOverwrite;
    }
  }

  ASSERT (Mode != OcSmbiosUpdateTryOverwrite);

  if (Mode != OcSmbiosUpdateOverwrite) {
    Status = SmbiosTableAllocate (
      TableLength,
      &TableEntryPoint,
      &TableEntryPoint3,
      &TableAddress,
      &TableAddress3
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    TableEntryPoint = mOriginalSmbios;
    TableEntryPoint3 = mOriginalSmbios3;
    TableAddress = (VOID *)(UINTN) TableEntryPoint->TableAddress;
    ZeroMem (TableEntryPoint, sizeof (SMBIOS_TABLE_ENTRY_POINT));
    ZeroMem (TableAddress, TableEntryPoint->TableLength);
    if (TableEntryPoint3 != NULL) {
      TableAddress3 = (VOID *)(UINTN) TableEntryPoint3->TableAddress;
      ZeroMem (TableEntryPoint3, sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
      ZeroMem (TableAddress3, TableEntryPoint3->TableMaximumSize);
    } else {
      TableAddress3 = NULL;
    }
  }

  CopyMem (
    TableAddress,
    SmbiosTable->Table,
    TableLength
    );

  if (TableAddress3 != NULL && TableAddress3 != TableAddress) {
    CopyMem (
      TableAddress3,
      SmbiosTable->Table,
      TableLength
      );
  }

  TableEntryPoint->AnchorString[0]             = (UINT8) '_';
  TableEntryPoint->AnchorString[1]             = (UINT8) 'S';
  TableEntryPoint->AnchorString[2]             = (UINT8) 'M';
  TableEntryPoint->AnchorString[3]             = (UINT8) '_';
  TableEntryPoint->EntryPointLength            = sizeof (SMBIOS_TABLE_ENTRY_POINT);
  TableEntryPoint->MajorVersion                = 3;
  TableEntryPoint->MinorVersion                = 2;
  TableEntryPoint->MaxStructureSize            = SmbiosTable->MaxStructureSize;
  TableEntryPoint->IntermediateAnchorString[0] = (UINT8) '_';
  TableEntryPoint->IntermediateAnchorString[1] = (UINT8) 'D';
  TableEntryPoint->IntermediateAnchorString[2] = (UINT8) 'M';
  TableEntryPoint->IntermediateAnchorString[3] = (UINT8) 'I';
  TableEntryPoint->IntermediateAnchorString[4] = (UINT8) '_';
  TableEntryPoint->TableLength                 = TableLength;
  TableEntryPoint->TableAddress                = (UINT32)(UINTN) TableAddress;
  TableEntryPoint->NumberOfSmbiosStructures    = SmbiosTable->NumberOfStructures;
  TableEntryPoint->SmbiosBcdRevision           = 0x32;
  TableEntryPoint->IntermediateChecksum        = CalculateCheckSum8 (
    (UINT8 *) TableEntryPoint + OFFSET_OF (SMBIOS_TABLE_ENTRY_POINT, IntermediateAnchorString),
    TableEntryPoint->EntryPointLength - OFFSET_OF (SMBIOS_TABLE_ENTRY_POINT, IntermediateAnchorString)
    );
  TableEntryPoint->EntryPointStructureChecksum = CalculateCheckSum8 (
    (UINT8 *) TableEntryPoint,
    TableEntryPoint->EntryPointLength
    );

  if (TableEntryPoint3 != NULL) {
    TableEntryPoint3->AnchorString[0]             = (UINT8) '_';
    TableEntryPoint3->AnchorString[1]             = (UINT8) 'S';
    TableEntryPoint3->AnchorString[2]             = (UINT8) 'M';
    TableEntryPoint3->AnchorString[3]             = (UINT8) '3';
    TableEntryPoint3->AnchorString[4]             = (UINT8) '_';
    TableEntryPoint3->EntryPointLength            = sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT);
    TableEntryPoint3->MajorVersion                = 3;
    TableEntryPoint3->MinorVersion                = 2;
    TableEntryPoint3->EntryPointRevision          = 1;
    TableEntryPoint3->TableMaximumSize            = TableLength;
    TableEntryPoint3->TableAddress                = (UINT64)(UINTN) TableAddress;
    TableEntryPoint3->EntryPointStructureChecksum = CalculateCheckSum8 (
      (UINT8 *) TableEntryPoint3,
      TableEntryPoint3->EntryPointLength
      );
  }

  DEBUG ((
    DEBUG_INFO,
    "OCSMB: Patched %p v%d.%d Table Address %08LX Length %04X %X %X\n",
    TableEntryPoint,
    TableEntryPoint->MajorVersion,
    TableEntryPoint->MinorVersion,
    (UINT64) TableEntryPoint->TableAddress,
    TableEntryPoint->TableLength,
    TableEntryPoint->EntryPointStructureChecksum,
    TableEntryPoint->IntermediateChecksum
    ));

  if (Mode == OcSmbiosUpdateOverwrite) {
    SmbiosHandleLegacyRegion (FALSE);
  } else {
    if (TableEntryPoint3 != NULL) {
      Status  = gBS->InstallConfigurationTable (
        Mode == OcSmbiosUpdateCustom ? &gOcCustomSmbios3TableGuid : &gEfiSmbios3TableGuid,
        TableEntryPoint3
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OCSMB: Failed to install v3 table - %r\n", Status));
      }
    }

    Status  = gBS->InstallConfigurationTable (
      Mode == OcSmbiosUpdateCustom ? &gOcCustomSmbiosTableGuid : &gEfiSmbiosTableGuid,
      TableEntryPoint
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCSMB: Failed to install v1 table - %r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

VOID
OcSmbiosGetSmcVersion (
  IN  CONST UINT8  *SmcRevision,
  OUT UINT8        *SmcVersion
  )
{
  UINT8                 Temp;
  UINT8                 Index;

  ZeroMem (SmcVersion, APPLE_SMBIOS_SMC_VERSION_SIZE);

  Temp  = SmcRevision[0];
  Index = 0;

  if (Temp < 0x10) {
    SmcVersion[Index] = (Temp + 0x30);
    ++Index;
  } else {
    SmcVersion[Index] = ((Temp >> 4) | 0x30);
    ++Index;

    SmcVersion[Index + 1] = ((Temp & 0x0F) | 0x30);
    ++Index;
  }

  SmcVersion[Index] = 0x2E;
  ++Index;

  Temp = SmcRevision[1];

  if (Temp < 0x10) {
    SmcVersion[Index] = (Temp + 0x30);
    ++Index;
  } else {
    SmcVersion[Index] = ((Temp >> 4) | 0x30);
    ++Index;

    SmcVersion[Index] = ((Temp & 0x0F) | 0x30);
    ++Index;
  }

  Temp = SmcRevision[2];

  if ((Temp & 0xF0) != 0) {
    if (Temp >= 0xA0) {
      SmcVersion[Index] = ((Temp >> 4) + 0x37);
      ++Index;
    } else {
      SmcVersion[Index] = ((Temp >> 4) | 0x30);
      ++Index;
    }

    Temp &= 0x0F;

    if (Temp >= 0x0A) {
      SmcVersion[Index] = (Temp + 0x37);
      ++Index;
    } else {
      SmcVersion[Index] = (Temp | 0x30);
      ++Index;
    }
  } else {
    if (Temp >= 0x0A) {
      SmcVersion[Index] = (Temp + 0x37);
      ++Index;
    } else {
      SmcVersion[Index] = (Temp + 0x30);
      ++Index;
    }
  }

  Temp = SmcRevision[3];

  if (Temp < 0x10) {
    SmcVersion[Index] = (Temp + 0x30);
    ++Index;
  } else {
    SmcVersion[Index] = ((Temp >> 4) | 0x30);
    ++Index;

    SmcVersion[Index] = ((Temp & 0x0F) | 0x30);
    ++Index;
  }

  Temp = SmcRevision[4];

  if (Temp < 0x10) {
    SmcVersion[Index] = (Temp + 0x30);
    ++Index;
  } else {
    SmcVersion[Index] = ((Temp >> 4) | 0x30);
    ++Index;

    SmcVersion[Index] = ((Temp & 0x0F) | 0x30);
    ++Index;
  }

  Temp = SmcRevision[5];

  if (Temp < 0x10) {
    SmcVersion[Index] = (Temp + 0x30);
    ++Index;
  } else {
    SmcVersion[Index] = ((Temp >> 4) | 0x30);
    ++Index;

    SmcVersion[Index] = ((Temp & 0x0F) | 0x30);
    ++Index;
  }
}

OC_SMBIOS_UPDATE_MODE
OcSmbiosGetUpdateMode (
  IN CONST CHAR8  *UpdateMode
  )
{
  if (AsciiStrCmp (UpdateMode, "TryOverwrite") == 0) {
    return OcSmbiosUpdateTryOverwrite;
  }

  if (AsciiStrCmp (UpdateMode, "Create") == 0) {
    return OcSmbiosUpdateCreate;
  }

  if (AsciiStrCmp (UpdateMode, "Overwrite") == 0) {
    return OcSmbiosUpdateOverwrite;
  }

  if (AsciiStrCmp (UpdateMode, "Custom") == 0) {
    return OcSmbiosUpdateCustom;
  }

  DEBUG ((DEBUG_INFO, "OCSMB: Invalid SMBIOS update mode %a\n", UpdateMode));
  return OcSmbiosUpdateCreate;
}

EFI_STATUS
OcSmbiosCreate (
  IN OUT OC_SMBIOS_TABLE        *SmbiosTable,
  IN     OC_SMBIOS_DATA         *Data,
  IN     OC_SMBIOS_UPDATE_MODE  Mode,
  IN     OC_CPU_INFO            *CpuInfo
  )
{
  EFI_STATUS                      Status;

  APPLE_SMBIOS_STRUCTURE_POINTER  MemoryArray;
  APPLE_SMBIOS_STRUCTURE_POINTER  MemoryArrayAddress;
  APPLE_SMBIOS_STRUCTURE_POINTER  MemoryDeviceInfo;
  APPLE_SMBIOS_STRUCTURE_POINTER  MemoryDeviceAddress;
  SMBIOS_HANDLE                   MemoryArrayHandle;
  SMBIOS_HANDLE                   MemoryDeviceHandle;
  UINT16                          NumMemoryArrays;
  UINT16                          NumMemoryArrayMapped;
  UINT16                          NumMemoryDevices;
  UINT16                          NumMemoryDeviceMapped;
  UINT16                          MemoryArrayNo;
  UINT16                          MemoryArrayMappedNo;
  UINT16                          MemoryDeviceNo;
  UINT16                          MemoryDeviceMappedNo;
  UINT16                          MemoryArrayNewIndex;
  UINT16                          MemoryArrayMappedNewIndex;
  UINT16                          MemoryDeviceNewIndex;
  UINT16                          MemoryDeviceMappedNewIndex;
  OC_SMBIOS_MAPPING               *Mapping;
  UINT16                          MappingNum;

  BOOLEAN                         HasAppleSMBIOS;

  ASSERT (Data != NULL);

  Mapping = AllocatePool (OC_SMBIOS_MAX_MAPPING * sizeof (*Mapping));
  if (Mapping == NULL) {
    DEBUG ((DEBUG_WARN, "OCSMB: Cannot allocate mapping table\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  MappingNum = 0;

  HasAppleSMBIOS = PatchBiosInformation (SmbiosTable, Data);
  PatchSystemInformation (SmbiosTable, Data);
  PatchBaseboardInformation (SmbiosTable, Data);
  PatchSystemEnclosure (SmbiosTable, Data);
  PatchProcessorInformation (SmbiosTable, Data, CpuInfo);
  PatchSystemPorts (SmbiosTable, Data);
  PatchSystemSlots (SmbiosTable, Data);

  //
  // Create new memory tables if custom memory is desired.
  // Otherwise we'll patch the existing memory information.
  //
  if (Data->HasCustomMemory) {
    CreateMemoryArray (
      SmbiosTable,
      Data,
      &MemoryArrayHandle
      );

      //
      // Generate new memory device tables (type 17) for this memory array.
      //
      for (MemoryDeviceNo = 0; MemoryDeviceNo < Data->MemoryDevicesCount; MemoryDeviceNo++) {
        CreateMemoryDevice (
          SmbiosTable,
          Data,
          &Data->MemoryDevices[MemoryDeviceNo],
          MemoryArrayHandle,
          MemoryDeviceNo + 1,
          &MemoryDeviceHandle
          );
      }

  } else {
    NumMemoryArrays       = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY);
    NumMemoryArrayMapped  = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS);
    NumMemoryDevices      = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_MEMORY_DEVICE);
    NumMemoryDeviceMapped = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS);

    MemoryArrayNewIndex        = 1;
    MemoryArrayMappedNewIndex  = 1;
    MemoryDeviceNewIndex       = 1;
    MemoryDeviceMappedNewIndex = 1;

    for (MemoryArrayNo = 0; MemoryArrayNo < NumMemoryArrays; MemoryArrayNo++) {
      MemoryArray = SmbiosGetOriginalStructure (SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, MemoryArrayNo + 1);

      //
      // We want to exclude any non-system memory tables, such as system ROM flash areas.
      //
      if (MemoryArray.Raw == NULL
        || MemoryArray.Standard.Type16->Use != MemoryArrayUseSystemMemory) {
        continue;
      }

      //
      // Generate new type 16 table for memory array.
      //
      PatchMemoryArray (
        SmbiosTable,
        Data,
        MemoryArray,
        MemoryArrayNewIndex,
        &MemoryArrayHandle
        );
      MemoryArrayNewIndex++;

      //
      // Generate new memory mapped address tables (type 19) for this memory array.
      //
      for (MemoryArrayMappedNo = 0; MemoryArrayMappedNo < NumMemoryArrayMapped; MemoryArrayMappedNo++) {
        MemoryArrayAddress = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, MemoryArrayMappedNo + 1);

        if (MemoryArrayAddress.Raw == NULL
          || MemoryArrayAddress.Standard.Type19->MemoryArrayHandle != MemoryArray.Standard.Type16->Hdr.Handle) {
          continue;
        }

        PatchMemoryMappedAddress (
          SmbiosTable,
          Data,
          MemoryArrayHandle,
          MemoryArrayAddress,
          MemoryArrayMappedNewIndex,
          Mapping,
          &MappingNum
          );
        MemoryArrayMappedNewIndex++;
      }

      //
      // Generate new memory device tables (type 17) for this memory array.
      //
      for (MemoryDeviceNo = 0; MemoryDeviceNo < NumMemoryDevices; MemoryDeviceNo++) {
        MemoryDeviceInfo = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_DEVICE, MemoryDeviceNo + 1);

        if (MemoryDeviceInfo.Raw == NULL
          || MemoryDeviceInfo.Standard.Type17->MemoryArrayHandle != MemoryArray.Standard.Type16->Hdr.Handle) {
          continue;
        }

        PatchMemoryDevice (
          SmbiosTable,
          Data,
          MemoryArrayHandle,
          MemoryDeviceInfo,
          MemoryDeviceNewIndex,
          &MemoryDeviceHandle
          );
        MemoryDeviceNewIndex++;

        //
        // Generate a memory device mapping table (type 20) for each occupied memory device.
        //
        for (MemoryDeviceMappedNo = 0; MemoryDeviceMappedNo < NumMemoryDeviceMapped; MemoryDeviceMappedNo++) {
          MemoryDeviceAddress = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, MemoryDeviceMappedNo + 1);

          if (MemoryDeviceAddress.Raw != NULL
            && SMBIOS_ACCESSIBLE (MemoryDeviceAddress, Standard.Type20->MemoryDeviceHandle)
            && MemoryDeviceAddress.Standard.Type20->MemoryDeviceHandle ==
              MemoryDeviceInfo.Standard.Type17->Hdr.Handle) {
              PatchMemoryMappedDevice (
                SmbiosTable,
                Data,
                MemoryDeviceAddress,
                MemoryDeviceMappedNewIndex,
                MemoryDeviceHandle,
                Mapping,
                MappingNum
                );
              MemoryDeviceMappedNewIndex++;
          }
        }
      }
    }
  }

  PatchPortableBatteryDevice (SmbiosTable, Data);
  PatchBootInformation (SmbiosTable, Data);
  CreateAppleProcessorType (SmbiosTable, Data, CpuInfo);
  CreateAppleProcessorSpeed (SmbiosTable, Data, CpuInfo);
  CreateOrPatchAppleFirmwareVolume (SmbiosTable, Data, HasAppleSMBIOS);
  CreateOrPatchApplePlatformFeature (SmbiosTable, Data, HasAppleSMBIOS);
  CreateOrPatchAppleSmcInformation (SmbiosTable, Data, HasAppleSMBIOS);
  CreateSmBiosEndOfTable (SmbiosTable, Data);

  FreePool (Mapping);

  Status = SmbiosTableApply (SmbiosTable, Mode);

  return Status;
}

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
  )
{
  EFI_STATUS                      Status;
  CONST CHAR8                     *SmProductName;
  CONST CHAR8                     *SmManufacturer;
  CONST CHAR8                     *SmBoard;
  CONST CHAR8                     *SmTmp;
  UINTN                           Index;
  UINTN                           TmpSize;
  UINT32                          MinCount;
  UINT32                          MaxCount;
  UINT8                           *UuidWalker;
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;

  SmProductName  = NULL;
  SmManufacturer = NULL;
  SmBoard        = NULL;

  Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_SYSTEM_INFORMATION, 1);
  if (Original.Raw != NULL) {
    if (SMBIOS_ACCESSIBLE (Original, Standard.Type1->ProductName)) {
      SmProductName = SmbiosGetString (Original, Original.Standard.Type1->ProductName);
      if (SmProductName != NULL && ProductName != NULL) {
        Status = AsciiStrCpyS (ProductName, OC_OEM_NAME_MAX, SmProductName);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCSMB: Failed to copy SMBIOS product name %a\n", SmProductName));
        }
      }
    }

    if (SerialNumber != NULL && SMBIOS_ACCESSIBLE (Original, Standard.Type1->SerialNumber)) {
      SmTmp = SmbiosGetString (Original, Original.Standard.Type1->SerialNumber);
      if (SmTmp != NULL) {
        Status = AsciiStrCpyS (SerialNumber, OC_OEM_SERIAL_MAX, SmTmp);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCSMB: Failed to copy SMBIOS product serial %a\n", SmTmp));
        }
      }
    }

    if (SystemUuid != NULL && SMBIOS_ACCESSIBLE (Original, Standard.Type1->Uuid)) {
      MinCount = 0;
      MaxCount = 0;
      UuidWalker = (UINT8 *) &Original.Standard.Type1->Uuid;
      for (Index = 0; Index < sizeof (Original.Standard.Type1->Uuid); ++Index) {
        if (UuidWalker[Index] == 0x00) {
          ++MinCount;
        } else if (UuidWalker[Index] == 0xFF) {
          ++MaxCount;
        }
      }

      if (MinCount < 4 && MaxCount < 4) {
        CopyGuid (SystemUuid, &Original.Standard.Type1->Uuid);
        //
        // Convert LE to RAW (assuming SMBIOS stores in LE format).
        //
        if (!UuidIsRawEncoded) {
          SystemUuid->Data1 = SwapBytes32 (SystemUuid->Data1);
          SystemUuid->Data2 = SwapBytes16 (SystemUuid->Data2);
          SystemUuid->Data3 = SwapBytes16 (SystemUuid->Data3);
        }
      } else {
        DEBUG ((
          DEBUG_WARN,
          "OCSMB: Ignoring UUID %g due to low entropy\n",
          &Original.Standard.Type1->Uuid
          ));
      }
    }
  }

  Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_BASEBOARD_INFORMATION, 1);
  if (Original.Raw != NULL) {
    if (SMBIOS_ACCESSIBLE (Original, Standard.Type2->Manufacturer)) {
      SmManufacturer = SmbiosGetString (Original, Original.Standard.Type2->Manufacturer);
    }
    if (SMBIOS_ACCESSIBLE (Original, Standard.Type2->ProductName)) {
      SmBoard = SmbiosGetString (Original, Original.Standard.Type2->ProductName);
    }
    if (Mlb != NULL && SMBIOS_ACCESSIBLE (Original, Standard.Type2->SerialNumber)) {
      SmTmp = SmbiosGetString (Original, Original.Standard.Type2->SerialNumber);
      if (SmTmp != NULL) {
        Status = AsciiStrCpyS (Mlb, OC_OEM_SERIAL_MAX, SmTmp);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "OCSMB: Failed to copy SMBIOS board serial %a\n", SmTmp));
        }
      }
    }
  }

  if (Mlb != NULL) {
    TmpSize = OC_OEM_SERIAL_MAX - 1;
    Status = gRT->GetVariable (
      L"MLB",
      &gAppleVendorVariableGuid,
      NULL,
      &TmpSize,
      Mlb
      );
    if (!EFI_ERROR (Status)) {
      ZeroMem (Mlb + TmpSize, OC_OEM_SERIAL_MAX - TmpSize);
      DEBUG ((DEBUG_INFO, "OCSMB: MLB from NVRAM took precedence: %a\n", Mlb));
    }
  }

  if (SerialNumber != NULL) {
    TmpSize = OC_OEM_SERIAL_MAX - 1;
    Status = gRT->GetVariable (
      L"SSN",
      &gAppleVendorVariableGuid,
      NULL,
      &TmpSize,
      SerialNumber
      );
    if (!EFI_ERROR (Status)) {
      ZeroMem (SerialNumber + TmpSize, OC_OEM_SERIAL_MAX - TmpSize);
      DEBUG ((DEBUG_INFO, "OCSMB: SSN from NVRAM took precedence: %a\n", SerialNumber));
    }
  }

  if (Rom != NULL) {
    TmpSize = OC_OEM_ROM_MAX;
    Status = gRT->GetVariable (
      L"ROM",
      &gAppleVendorVariableGuid,
      NULL,
      &TmpSize,
      Rom
      );
    if (!EFI_ERROR (Status) && TmpSize != OC_OEM_ROM_MAX) {
      ZeroMem (Rom, OC_OEM_ROM_MAX);
    }
  }

  if (SystemUuid != NULL) {
    TmpSize = sizeof (EFI_GUID);
    Status = gRT->GetVariable (
      L"system-id",
      &gAppleVendorVariableGuid,
      NULL,
      &TmpSize,
      SystemUuid
      );
    if (!EFI_ERROR (Status) && TmpSize == sizeof (EFI_GUID)) {
      DEBUG ((DEBUG_INFO, "OCSMB: UUID from NVRAM took precedence: %g\n", SystemUuid));
    } else if (TmpSize != sizeof (EFI_GUID)) {
      ZeroMem (SystemUuid, sizeof (EFI_GUID));
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "OCSMB: Current SMBIOS %a (%a made by %a)\n",
    SmProductName,
    SmBoard,
    SmManufacturer
    ));

  if (ProductName != NULL && SmProductName != NULL) {
    Status = AsciiStrCpyS (ProductName, OC_OEM_NAME_MAX, SmProductName);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCSMB: Failed to copy SMBIOS product name %a\n", SmProductName));
    }
  }

  if (UseVariableStorage) {
    if (SmProductName != NULL) {
      Status = gRT->SetVariable (
        OC_OEM_PRODUCT_VARIABLE_NAME,
        &gOcVendorVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        AsciiStrLen (SmProductName),
        (VOID *) SmProductName
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMB: Cannot write OEM product\n"));
      }
    }

    if (SmManufacturer != NULL && SmBoard != NULL) {
      Status = gRT->SetVariable (
        OC_OEM_VENDOR_VARIABLE_NAME,
        &gOcVendorVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        AsciiStrLen (SmManufacturer),
        (VOID *) SmManufacturer
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMB: Cannot write OEM board manufacturer - %r\n", Status));
      }

      Status = gRT->SetVariable (
        OC_OEM_BOARD_VARIABLE_NAME,
        &gOcVendorVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        AsciiStrLen (SmBoard),
        (VOID *) SmBoard
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCSMB: Cannot write OEM board - %r\n", Status));
      }
    }
  }
}

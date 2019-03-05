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

#include <Guid/SmBios.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcProtocolLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/AppleSmBios.h>
#include <IndustryStandard/Pci.h>

#include "DebugSmbios.h"
#include "SmbiosInternal.h"

#include <Macros.h>
#include <ProcessorInfo.h>

STATIC SMBIOS_TABLE_ENTRY_POINT        *mOriginalSmbios;
STATIC SMBIOS_TABLE_3_0_ENTRY_POINT    *mOriginalSmbios3;
STATIC APPLE_SMBIOS_STRUCTURE_POINTER  mOriginalTable;
STATIC UINT32                          mOriginalTableSize;

#define SMBIOS_OVERRIDE_STR(Table, Field, Original, Value, Index, Fallback, Hex) \
  do { \
    CONST CHAR8  *RealValue__ = (Value); \
    if (RealValue__ == NULL && ((Original).Raw) != NULL && (Original).Raw + (Original).Standard.Hdr->Length \
      >= ((UINT8 *)&((Original).Field) + sizeof (SMBIOS_TABLE_STRING))) { \
      RealValue__ = SmbiosGetString ((Original), ((Original).Field)); \
    } \
    (((Table)->CurrentPtr).Field) = SmbiosOverrideString ( \
      (Table), \
      RealValue__ != NULL ? RealValue__ : (Fallback), \
      (Index), \
      (Hex) \
      ); \
  } while (0)

#define SMBIOS_OVERRIDE_S(Table, Field, Original, Value, Index, Fallback) \
  SMBIOS_OVERRIDE_STR ((Table), Field, (Original), (Value), (Index), (Fallback), FALSE)

#define SMBIOS_OVERRIDE_H(Table, Field, Original, Value, Index, Fallback) \
  SMBIOS_OVERRIDE_STR ((Table), Field, (Original), (Value), (Index), (Fallback), TRUE)

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

/** Type 0

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchBiosInformation (
  IN OUT OC_SMBIOS_TABLE *Table,
  IN     OC_SMBIOS_DATA  *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original    = SmbiosGetOriginalStructure (SMBIOS_TYPE_BIOS_INFORMATION, 1);
  MinLength   = sizeof (*Original.Standard.Type0);
  StringIndex = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_BIOS_INFORMATION, MinLength, 1))) {
    return;
  }

  CONST CHAR8  *RealValue__ = Data->BIOSVendor;
  if (RealValue__ == NULL
    && ((Original).Raw) != NULL
    && (Original).Raw + (Original).Standard.Hdr->Length
    >= ((UINT8 *)&((Original).Standard.Type0->Vendor) + sizeof (SMBIOS_TABLE_STRING))) {
    RealValue__ = SmbiosGetString ((Original), ((Original).Standard.Type0->Vendor));
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

  SmbiosFinaliseStruct (Table);
}

/** Type 1

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
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
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
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
  OC_INLINE_STATIC_ASSERT (
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
  SMBIOS_OVERRIDE_V (Table, Standard.Type2->BoardType, Original, &Data->BoardType, NULL);

  //
  // Leave NumberOfContainedObjectHandles as 0, just like Apple does.
  //

  SmbiosFinaliseStruct (Table);
}

/** Type 3

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
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
  OC_INLINE_STATIC_ASSERT (
    sizeof (Original.Standard.Type3->ContainedElements) == sizeof (UINT8)*3,
    "Please remove this hack, the structure should have been fixed by now."
    );

  //
  // Another hack for omitted SKUNumber after flexible array.
  // Note, Apple just ignores this field and just writes Type3 as is.
  //
  MinLength += sizeof (SMBIOS_TABLE_STRING);
  OC_INLINE_STATIC_ASSERT (
    sizeof (*Original.Standard.Type3) == sizeof (UINT8) * 0x18,
    "The structure has changed, please fix this up."
    );

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_SYSTEM_ENCLOSURE, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type3->Manufacturer, Original, Data->ChassisManufacturer, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type3->Type, Original, &Data->ChassisType, NULL);
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

/** Type 4

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.

  @retval
**/
STATIC
VOID
PatchProcessorInformation (
  IN OUT OC_SMBIOS_TABLE *Table,
  IN     OC_SMBIOS_DATA  *Data,
  IN     CPU_INFO        *CpuInfo
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;
  UINT8                           TmpCount;

  Original      = SmbiosGetOriginalStructure (SMBIOS_TYPE_PROCESSOR_INFORMATION, 1);
  MinLength     = sizeof (*Original.Standard.Type4);
  StringIndex   = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_PROCESSOR_INFORMATION, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type4->Socket, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorType, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorFamily, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type4->ProcessorManufacture, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorId, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type4->ProcessorVersion, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type4->Voltage, Original, NULL, NULL);
  //
  // Models newer than Sandybridge require quad pumped bus value instead of a front side bus value.
  //
  if (CpuInfo->Model >= CPU_MODEL_SANDYBRIDGE) {
    Table->CurrentPtr.Standard.Type4->ExternalClock = OC_CPU_SNB_QPB_CLOCK;
  } else {
    Table->CurrentPtr.Standard.Type4->ExternalClock = (UINT16) DivU64x32 (CpuInfo->FSBFrequency, 1000000);
  }

  Table->CurrentPtr.Standard.Type4->MaxSpeed = (UINT16) DivU64x32 (CpuInfo->CPUFrequency, 1000000);
  if (Table->CurrentPtr.Standard.Type4->MaxSpeed % 100 != 0) {
    Table->CurrentPtr.Standard.Type4->MaxSpeed = (UINT16) (((Table->CurrentPtr.Standard.Type4->MaxSpeed + 50) / 100) * 100);
  }

  Table->CurrentPtr.Standard.Type4->CurrentSpeed  = Table->CurrentPtr.Standard.Type4->MaxSpeed;

  SMBIOS_OVERRIDE_V (Table, Standard.Type4->Status, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type4->ProcessorUpgrade, Original, NULL, NULL);

  Table->CurrentPtr.Standard.Type4->L1CacheHandle = OcSmbiosL1CacheHandle;
  Table->CurrentPtr.Standard.Type4->L2CacheHandle = OcSmbiosL2CacheHandle;
  Table->CurrentPtr.Standard.Type4->L3CacheHandle = OcSmbiosL3CacheHandle;

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

/** Type 7

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchCacheInformation (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT16                          NumberEntries;
  UINT16                          EntryNo;
  UINT8                           MinLength;
  UINT8                           StringIndex;
  UINT16                          CacheLevel;
  BOOLEAN                         CacheLevels[3];

  ZeroMem (CacheLevels, sizeof (CacheLevels));

  NumberEntries = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_CACHE_INFORMATION);

  for (EntryNo = 1; EntryNo <= NumberEntries; EntryNo++) {
    Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_CACHE_INFORMATION, EntryNo);
    if (Original.Raw == NULL || (UINT8 *) &Original.Standard.Type7->CacheConfiguration - Original.Raw
      + sizeof (Original.Standard.Type7->CacheConfiguration) > Original.Standard.Hdr->Length) {
      continue;
    }

    //
    // Check if already done with this cache type.
    // Only support L1 to L3 caches.
    //
    CacheLevel = Original.Standard.Type7->CacheConfiguration & 7U;
    if (CacheLevel <= 2 && !CacheLevels[CacheLevel]) {
      CacheLevels[CacheLevel] = TRUE;
    } else {
      continue;
    }

    MinLength     = sizeof (*Original.Standard.Type7);
    StringIndex   = 0;

    if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_CACHE_INFORMATION, MinLength, CacheLevel+1))) {
      continue;
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
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->MaximumCacheSize2, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type7->InstalledSize2, Original, NULL, NULL);

    SmbiosFinaliseStruct (Table);
  }
}

/** Type 8

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
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
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
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
          BITFIELD (Table->CurrentPtr.Standard.Type9->DevFuncNum, 3, 7),
          Table->CurrentPtr.Standard.Type9->BusNum,
          BITFIELD (Table->CurrentPtr.Standard.Type9->DevFuncNum, 2, 0),
          0
          );
      } else {
        PciAddress = EFI_PCI_ADDRESS (
          Table->CurrentPtr.Standard.Type9->BusNum,
          BITFIELD (Table->CurrentPtr.Standard.Type9->DevFuncNum, 3, 7),
          BITFIELD (Table->CurrentPtr.Standard.Type9->DevFuncNum, 2, 0),
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
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchMemoryArray (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original      = SmbiosGetOriginalStructure (SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, 1);
  MinLength     = sizeof (*Original.Standard.Type16);
  StringIndex   = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type16->Location, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->Use, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->MemoryErrorCorrection, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->MaximumCapacity, Original, NULL, NULL);
  //
  // Do not support memory error information.
  //
  Table->CurrentPtr.Standard.Type16->MemoryErrorInformationHandle = 0xFFFF;
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->NumberOfMemoryDevices, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type16->ExtendedMaximumCapacity, Original, NULL, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 17

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchMemoryDevice (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
  IN     APPLE_SMBIOS_STRUCTURE_POINTER  Original,
  IN     UINT16                          Index,
     OUT SMBIOS_HANDLE                   *Handle
  )
{
  UINT8    MinLength;
  UINT8    StringIndex;
  BOOLEAN  IsEmpty;

  *Handle       = OcSmbiosInvalidHandle;
  Original      = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_DEVICE, Index);
  MinLength     = sizeof (*Original.Standard.Type17);
  StringIndex   = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_MEMORY_DEVICE, MinLength, Index))) {
    return;
  }

  //
  // External handles are set below.
  //
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->TotalWidth, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->DataWidth, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->Size, Original, NULL, NULL);
  Table->CurrentPtr.Standard.Type17->FormFactor = Data->MemoryFormFactor;
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->DeviceSet, Original, NULL, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type17->DeviceLocator, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type17->BankLocator, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->MemoryType, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->TypeDetail, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type17->Speed, Original, NULL, NULL);

  //
  // Empty modules should have 0 Size according to the spec.
  // Original OC implementation relies on TotalWidth, which may be a workaround for some FW.
  //
  IsEmpty = Table->CurrentPtr.Standard.Type17->Size == 0
    || Table->CurrentPtr.Standard.Type17->TotalWidth == 0;

  if (!IsEmpty) {
    Table->CurrentPtr.Standard.Type17->MemoryArrayHandle = OcSmbiosPhysicalMemoryArrayHandle;
    Table->CurrentPtr.Standard.Type17->MemoryErrorInformationHandle = 0xFFFF;
  } else {
    //
    // Empty slots should have no physical memory array handle.
    //
    Table->CurrentPtr.Standard.Type17->MemoryArrayHandle = 0xFFFF;
    Table->CurrentPtr.Standard.Type17->MemoryErrorInformationHandle = 0xFFFF;
  }

  SMBIOS_OVERRIDE_S (Table, Standard.Type17->Manufacturer, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type17->SerialNumber, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type17->AssetTag, Original, NULL, &StringIndex, NULL);
  SMBIOS_OVERRIDE_S (Table, Standard.Type17->PartNumber, Original, NULL, &StringIndex, NULL);

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

/** Type 19

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchMemoryMappedAddress (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT16                          NumberEntries;
  UINT16                          EntryNo;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  NumberEntries = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS);

  for (EntryNo = 1; EntryNo <= NumberEntries; EntryNo++) {
    Original = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, EntryNo);
    if (Original.Raw == NULL) {
      continue;
    }

    MinLength     = sizeof (*Original.Standard.Type19);
    StringIndex   = 0;

    if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, MinLength, EntryNo))) {
      continue;
    }

    SMBIOS_OVERRIDE_V (Table, Standard.Type19->StartingAddress, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type19->EndingAddress, Original, NULL, NULL);
    Table->CurrentPtr.Standard.Type19->MemoryArrayHandle = OcSmbiosPhysicalMemoryArrayHandle;
    SMBIOS_OVERRIDE_V (Table, Standard.Type19->PartitionWidth, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type19->ExtendedStartingAddress, Original, NULL, NULL);
    SMBIOS_OVERRIDE_V (Table, Standard.Type19->ExtendedEndingAddress, Original, NULL, NULL);

    SmbiosFinaliseStruct (Table);
  }
}

/** Type 20

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchMemoryMappedDevice (
  IN OUT OC_SMBIOS_TABLE                 *Table,
  IN     OC_SMBIOS_DATA                  *Data,
  IN     APPLE_SMBIOS_STRUCTURE_POINTER  Original,
  IN     UINT16                          Index,
     OUT SMBIOS_HANDLE                   MemoryDeviceHandle
  )
{
  UINT8    MinLength;
  UINT8    StringIndex;

  Original      = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, Index);
  MinLength     = sizeof (*Original.Standard.Type20);
  StringIndex   = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, MinLength, Index))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type20->StartingAddress, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->EndingAddress, Original, NULL, NULL);
  Table->CurrentPtr.Standard.Type20->MemoryDeviceHandle = MemoryDeviceHandle;
  //
  // FIXME: This one should point to Original Type 19...
  //
  Table->CurrentPtr.Standard.Type20->MemoryArrayMappedAddressHandle = 0xFFFF;
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->PartitionRowPosition, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->InterleavePosition, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->InterleavedDataDepth, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->ExtendedStartingAddress, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type20->ExtendedEndingAddress, Original, NULL, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 22

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
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
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
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
  UINT8                           StringIndex;

  Original    = SmbiosGetOriginalStructure (SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, 1);
  MinLength   = sizeof (*Original.Standard.Type32);
  StringIndex = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Standard.Type32->Reserved, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Standard.Type32->BootStatus, Original, NULL, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 128

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchAppleFirmwareVolume (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original    = SmbiosGetOriginalStructure (APPLE_SMBIOS_TYPE_FIRMWARE_INFORMATION, 1);
  MinLength   = sizeof (*Original.Type128);
  StringIndex = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_FIRMWARE_INFORMATION, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Type128->FirmwareFeatures, Original, &Data->FirmwareFeatures, NULL);
  SMBIOS_OVERRIDE_V (Table, Type128->FirmwareFeaturesMask, Original, &Data->FirmwareFeaturesMask, NULL);
  SMBIOS_OVERRIDE_V (Table, Type128->ExtendedFirmwareFeatures, Original, &Data->ExtendedFirmwareFeatures, NULL);
  SMBIOS_OVERRIDE_V (Table, Type128->ExtendedFirmwareFeaturesMask, Original, &Data->ExtendedFirmwareFeaturesMask, NULL);

  SmbiosFinaliseStruct (Table);
}

/** Type 131

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.

  @retval
**/
STATIC
VOID
PatchAppleProcessorType (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN     CPU_INFO         *CpuInfo
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;
  APPLE_PROCESSOR_TYPE            *ProcessorType;

  Original    = SmbiosGetOriginalStructure (APPLE_SMBIOS_TYPE_PROCESSOR_TYPE, 1);
  MinLength   = sizeof (*Original.Type131);
  StringIndex = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_PROCESSOR_TYPE, MinLength, 1))) {
    return;
  }

  SMBIOS_OVERRIDE_V (Table, Type131->ProcessorType, Original, NULL, NULL);
  SMBIOS_OVERRIDE_V (Table, Type131->Reserved, Original, NULL, NULL);

  ProcessorType = &Table->CurrentPtr.Type131->ProcessorType;

  switch (CpuInfo->Model) {
    //
    // Yonah: https://en.wikipedia.org/wiki/Yonah_(microprocessor)#Models_and_brand_names
    //
    // Used by Apple:
    //   Core Duo, Core Solo
    //
    // NOT used by Apple:
    //   Pentium, Celeron
    //
    // All 0x0201.
    //
    case CPU_MODEL_DOTHAN: // 0x0D
    case CPU_MODEL_YONAH:  // 0x0E

      // IM41  (T2400/T2500), MM11 (Solo T1200 / Duo T2300/T2400),
      // MBP11 (L2400/T2400/T2500/T2600), MBP12 (T2600),
      // MB11  (T2400/T2500)
      ProcessorType->Type = AppleProcessorTypeCoreSolo; // 0x0201

      break;

    //
    // Merom:  https://en.wikipedia.org/wiki/Merom_(microprocessor)#Variants
    // Penryn: https://en.wikipedia.org/wiki/Penryn_(microprocessor)#Variants
    //
    // Used by Apple:
    //   Core 2 Extreme, Core 2 Duo (Merom),
    //   Core 2 Duo,                (Penryn),
    //   certain Clovertown (Merom) / Harpertown (Penryn) based models
    //
    // Not used by Apple:
    //   Merom:  Core 2 Solo, Pentium, Celeron M, Celeron
    //   Penryn: Core 2 Extreme, Core 2 Quad, Core 2 Solo, Pentium, Celeron
    //
    case CPU_MODEL_MEROM:  // 0x0F
    case CPU_MODEL_PENRYN: // 0x17

      if (CpuInfo->AppleMajorType == AppleProcessorMajorCore2) {
        // TODO: add check for models above. (by changing the following "if (0)")
        if (0) {
          // ONLY MBA31 (SU9400/SU9600) and MBA32 (SL9400/SL9600)
          ProcessorType->Type = AppleProcessorTypeCore2DuoType2; // 0x0302
        } else {
          // IM51 (T7200), IM61 (T7400), IM71 (T7300), IM81 (E8435), IM101 (E7600),
          // MM21 (unknown), MM31 (P7350),
          // MBP21 (T7600), MBP22 (unknown), MBP31 (T7700), MBP41 (T8300), MBP71 (P8600),
          // MBP51 (P8600), MBP52 (T9600), MBP53 (P8800), MBP54 (P8700), MBP55 (P7550),
          // MBA11 (P7500), MBA21 (SL9600),
          // MB21 (unknown), MB31 (T7500), MB41 (T8300), MB51 (P8600), MB52 (P7450), MB61 (P7550), MB71 (P8600)
          ProcessorType->Type = AppleProcessorTypeCore2DuoType1; // 0x0301
        }
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonPenryn) {
        // MP21 (2x X5365), MP31 (2x E5462) - 0x0402
        // FIXME: check when 0x0401 will be used.
        ProcessorType->Type = AppleProcessorTypeXeonPenrynType2; // 0x0402
      } else {
        // here stands for models not used by Apple (Merom/Penryn), putting 0x0301 as lowest
        ProcessorType->Type = AppleProcessorTypeCore2DuoType1;   // 0x0301
      }

      break;

    //
    // Nehalem:  https://en.wikipedia.org/wiki/Nehalem_(microarchitecture)#Server_and_desktop_processors
    // Westmere: https://en.wikipedia.org/wiki/Westmere_(microarchitecture)#Server_/_Desktop_processors
    //
    // Used by Apple:
    //   Gainestown (Xeon), Bloomfield (Xeon), Lynnfield (i5/i7)                   [Nehalem]
    //   Gulftown (aka Westmere-EP, Xeon), Clarkdale (i3/i5), Arrandale (i5/i7)    [Westmere]
    //
    // Not used by Apple:
    //   Beckton (Xeon), Jasper Forest (Xeon), Clarksfield (i7), Pentium, Celeron [Nehalem]
    //   Westmere-EX (Xeon E7), Pentium, Celeron                                  [Westmere]
    //
    case CPU_MODEL_NEHALEM:     // 0x1A
    case CPU_MODEL_NEHALEM_EX:  // 0x2E, not used by Apple
    case CPU_MODEL_FIELDS:      // 0x1E, Lynnfield, Clarksfield (part of Nehalem)
    case CPU_MODEL_WESTMERE:    // 0x2C
    case CPU_MODEL_WESTMERE_EX: // 0x2F, not used by Apple
    case CPU_MODEL_DALES_32NM:  // 0x25, Clarkdale, Arrandale (part of Westmere)

      if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonNehalem) {
        // MP41 & Xserve31 (2x E5520, CPU_MODEL_NEHALEM), MP51 (2x X5670, CPU_MODEL_WESTMERE)
        ProcessorType->Type = AppleProcessorTypeXeon;        // 0x0501
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI3) {
        // IM112 (i3-540, 0x0901, CPU_MODEL_DALES_32NM)
        ProcessorType->Type = AppleProcessorTypeCorei3Type1; // 0x0901
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI5) {
        // FIXME: no idea what it is on IM112 (i5-680)
        // MBP61, i5-640M, 0x0602, CPU_MODEL_DALES_32NM
        ProcessorType->Type = AppleProcessorTypeCorei5Type2; // 0x0602
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI7) {
        // FIXME: used by Apple, no idea what to use, assuming 0x0702 for now (based off 0x0602 on i5)
        ProcessorType->Type = AppleProcessorTypeCorei7Type2; // 0x0702
      } else {
        // here stands for Pentium and Celeron (Nehalem/Westmere), not used by Apple at all.
        // putting 0x0901 (i3) as lowest
        ProcessorType->Type = AppleProcessorTypeCorei3Type1; // 0x0901
      }

      break;

    //
    // Sandy Bridge:   https://en.wikipedia.org/wiki/Sandy_Bridge#List_of_Sandy_Bridge_processors
    // Sandy Bridge-E: https://en.wikipedia.org/wiki/Sandy_Bridge-E#Overview
    //
    // Used by Apple:
    //   Core i5/i7 / i3 (see NOTE below)
    //
    // NOTE: There seems to be one more i3-2100 used on IM121 (EDU),
    //       assuming it exists for now.
    //
    // Not used by Apple:
    //   Xeon v1 (E5/E3),
    //   SNB-E based Core i7 (and Extreme): 3970X, 3960X, 3930K, 3820,
    //   Pentium, Celeron
    //
    case CPU_MODEL_SANDYBRIDGE: // 0x2A
    case CPU_MODEL_JAKETOWN:    // 0x2D, SNB-E, not used by Apple

      if (CpuInfo->AppleMajorType == AppleProcessorMajorI3) {
        // FIXME: used by Apple on iMac12,1 (EDU, i3-2100), not confirmed yet
        ProcessorType->Type = AppleProcessorTypeCorei3Type3;   // 0x0903
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI5) {
        // NOTE: two values are used here. (0x0602 and 0x0603)
        // TODO: how to classify them. (by changing "if (0)")
        if (0) {
          // MM51 (i5-2415M), MM52 (i5-2520M), MBA41 (i5-2467M), MBA42 (i5-2557M)
          ProcessorType->Type = AppleProcessorTypeCorei5Type2; // 0x0602
        } else {
          // IM121 (i5-2400S), MBP81 (i5-2415M)
          ProcessorType->Type = AppleProcessorTypeCorei5Type3; // 0x0603
        }
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI7) {
        // IM122 (i7-2600), MBP82 (i7-2675QM), MBP83 (i7-2820QM)
        //
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        ProcessorType->Type = AppleProcessorTypeCorei7Type3;   // 0x0703
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonE5) { // see TODO below
        // for Sandy Xeon E5, not used by Apple
        // FIXME: is AppleProcessorMajorXeonE5, which seems to be for IVY-E only, compatible with SNB-E too?
        // TODO: write some decent code to check SNB-E based Xeon E5.
        ProcessorType->Type = AppleProcessorTypeXeonE5;        // 0x0A01
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Sandy Xeon E3
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for SNB based Xeon E3
        ProcessorType->Type = AppleProcessorTypeXeon;          // 0x0501
      } else {
        // here stands for Pentium and Celeron (Sandy), not used by Apple at all.
        // putting 0x0903 (i3) as lowest
        ProcessorType->Type = AppleProcessorTypeCorei3Type3;   // 0x0903
      }

      break;

    //
    // Ivy Bridge:   https://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture)#List_of_Ivy_Bridge_processors
    // Ivy Bridge-E: https://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture)#Models_and_steppings_2
    //
    // Used by Apple:
    //   Core i5/i7 / i3 (see NOTE below),
    //   Xeon E5 v2
    //
    // NOTE: There seems to be an iMac13,1 (EDU version), or rather iMac13,3, with CPU i3-3225,
    //       assuming it exists for now.
    //
    // Not used by Apple:
    //   Xeon v2 (E7/E3),
    //   IVY-E based Core i7 (and Extreme): 4960X, 4930K, 4820K,
    //   Pentium, Celeron
    //
    case CPU_MODEL_IVYBRIDGE:    // 0x3A
    case CPU_MODEL_IVYBRIDGE_EP: // 0x3E

      if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonE5) {
        // MP61 (E5-1620 v2)
        ProcessorType->Type = AppleProcessorTypeXeonE5;      // 0x0A01
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI5) {
        // IM131 (i5-3470S), IM132  (i5-3470S),
        // MBP92 (i5-3210M), MBP102 (i5-3210M)
        // MBA51 (i6-3317U), MBA52  (i5-3427U)
        ProcessorType->Type = AppleProcessorTypeCorei5Type4; // 0x0604
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI7) {
        // MM62  (i7-3615QM),
        // MBP91 (i7-3615QM), MBP101 (i7-3820QM)
        //
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        ProcessorType->Type = AppleProcessorTypeCorei7Type4; // 0x0704
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI3) {
        // FIXME: used by Apple (if iMac13,3 were existent, i3-3225), not confirmed yet
        // assuming it exists for now
        ProcessorType->Type = AppleProcessorTypeCorei3Type4; // 0x0904
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Ivy/Ivy-E E3/E7, not used by Apple
        // NOTE: Xeon E3/E7 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for IVY based Xeon E3/E7
        ProcessorType->Type = AppleProcessorTypeXeon;        // 0x0501
      } else {
        // here stands for Pentium and Celeron (Ivy), not used by Apple at all.
        // putting 0x0904 (i3) as lowest.
        ProcessorType->Type = AppleProcessorTypeCorei3Type4; // 0x0904
      }

      break;

    //
    // Haswell:   https://en.wikipedia.org/wiki/Haswell_(microarchitecture)#List_of_Haswell_processors
    // Haswell-E: basically the same page.
    //
    // Used by Apple:
    //   Core i5/i7
    //
    // Not used by Apple:
    //   Xeon v3 (E7/E5/E3),
    //   Core i3,
    //   Haswell-E based Core i7 Extreme: 5960X, 5930K, 5820K,
    //   Pentium, Celeron
    //
    case CPU_MODEL_HASWELL:     // 0x3C
    case CPU_MODEL_HASWELL_EP:  // 0x3F
    case CPU_MODEL_HASWELL_ULT: // 0x45

      if (CpuInfo->AppleMajorType == AppleProcessorMajorI5) {
        // IM141 (i5-4570R), IM142 (i5-4670), IM151 (i5-4690),
        // MM71  (i5-4260U),
        // MBA62 (i5-4250U)
        ProcessorType->Type = AppleProcessorTypeCorei5Type5; // 0x0605
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI7) {
        // MBP112 (i7-4770HQ), MBP113 (i7-4850HQ)
        //
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        ProcessorType->Type = AppleProcessorTypeCorei7Type5; // 0x0705
      } else if (CpuInfo->AppleMajorType ==  AppleProcessorMajorI3) {
        // for i3, not used by Apple, just for showing i3 in "About This Mac".
        ProcessorType->Type = AppleProcessorTypeCorei3Type5; // 0x0905
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonE5) { // see TODO below
        // for Haswell-E Xeon E5, not used by Apple
        // FIXME: is AppleProcessorMajorXeonE5, which seems to be for IVY-E only, compatible with Haswell-E too?
        // TODO: write some decent code to check Haswell-E based Xeon E5.
        ProcessorType->Type = AppleProcessorTypeXeonE5;      // 0x0A01
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Haswell/Haswell-E E3/E7, not used by Apple
        // NOTE: Xeon E3/E7 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for Haswell/Haswell-E based Xeon E3/E7
        ProcessorType->Type = AppleProcessorTypeXeon;        // 0x0501
      } else {
        // here stands for Pentium and Celeron (Haswell), not used by Apple at all.
        // putting 0x0905 (i3) as lowest.
        ProcessorType->Type = AppleProcessorTypeCorei3Type5; // 0x0905
      }

      break;

    //
    // Broadwell:   https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)#List_of_Broadwell_processors
    // Broadwell-E: https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)#"Broadwell-E"_HEDT_(14_nm)
    //
    // NOTE: support table for BDW-E is missing in XNU, thus a CPUID patch might be needed. (See Clover FakeCPUID)
    //
    // Used by Apple:
    //   Core i5/i7, Core M
    //
    // Not used by Apple:
    //   Broadwell-E: i7 6950X/6900K/6850K/6800K,
    //   Xeon v4 (E5/E3),
    //   Core i3,
    //   Pentium, Celeron
    //
    case CPU_MODEL_BROADWELL:     // 0x3D
    case CPU_MODEL_CRYSTALWELL:   // 0x46
    case CPU_MODEL_BRYSTALWELL:   // 0x47

      if (CpuInfo->AppleMajorType == AppleProcessorMajorM) {
        // MB81 (M 5Y51)
        ProcessorType->Type = AppleProcessorTypeCoreMType6;   // 0x0B06
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI5) {
        // IM161  (i5-5250U), IM162 (i5-5675R),
        // MBP121 (i5-5257U),
        // MBA71  (i5-5250U), MBA72 (unknown)
        ProcessorType->Type = AppleProcessorTypeCorei5Type6; // 0x0606
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI7) {
        // FIXME: 0x0706 is just an ideal value for i7, waiting for confirmation
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        ProcessorType->Type = AppleProcessorTypeCorei7Type6; // 0x0706
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI3) {
        // for i3, not used by Apple, just for showing i3 in "About This Mac".
        // FIXME: 0x0906 is just an ideal value for i3, waiting for confirmation
        ProcessorType->Type = AppleProcessorTypeCorei3Type6; // 0x0906
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonE5) { // see TODO below
        // for Broadwell-E Xeon E5, not used by Apple
        // FIXME: is AppleProcessorMajorXeonE5, which seems to be for IVY-E only, compatible with Broadwell-E too?
        // TODO: write some decent code to check Broadwell-E based Xeon E5.
        ProcessorType->Type = AppleProcessorTypeXeonE5;      // 0x0A01
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Broadwell E3, not used by Apple
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for Broadwell based Xeon E3
        ProcessorType->Type = AppleProcessorTypeXeon;        // 0x0501
      } else {
        // here stands for Pentium and Celeron (Broadwell), not used by Apple at all.
        // putting 0x0906 (i3) as lowest.
        ProcessorType->Type = AppleProcessorTypeCorei3Type5; // 0x0906
      }

      break;

    //
    // Skylake: https://en.wikipedia.org/wiki/Skylake_(microarchitecture)#List_of_Skylake_processor_models
    //
    // Used by Apple:
    //   Xeon W, Core m3, m5, m7, i5, i7
    //
    // Not used by Apple:
    //   Core i3,
    //   all high-end models (Core i9, i7 Extreme): see https://en.wikipedia.org/wiki/Skylake_(microarchitecture)#High-end_desktop_processors
    //   Xeon E3 v5,
    //   Pentium, Celeron
    //
    case CPU_MODEL_SKYLAKE:     // 0x4E
    case CPU_MODEL_SKYLAKE_DT:  // 0x5E
    case CPU_MODEL_SKYLAKE_W:   // 0x55, also SKL-X

      if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonW) {
        // IMP11 (Xeon W 2140B)
        ProcessorType->Type = AppleProcessorTypeXeonW;       // 0x0F01
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorM3) {
        // FIXME: we dont have any m3 (Skylake) dump!
        // using an ideal value (0x0C07), which is used on MB101 (m3-7Y32)
        ProcessorType->Type = AppleProcessorTypeCoreM3Type7; // 0x0C07
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorM5) {
        // MB91 (m5 6Y54)
        ProcessorType->Type = AppleProcessorTypeCoreM5Type7; // 0x0D07
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorM7) {
        // FIXME: we dont have any m7 (Skylake) dump!
        // using an ideal value (0x0E07)
        ProcessorType->Type = AppleProcessorTypeCoreM7Type7; // 0x0E07
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI5) {
        ProcessorType->Type = AppleProcessorTypeCorei5Type5; // 0x0605
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI7) {
        // FIXME: used by Apple, but not sure what to use...
        // 0x0707 is used on MBP133 (i7-6700HQ),
        // 0x0705 is not confirmed, just an ideal one comparing to 0x0605 (AppleProcessorTypeCorei5Type5)
        // using 0x0705 for now
        ProcessorType->Type = AppleProcessorTypeCorei7Type5; // 0x0705
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI3) {
        // for i3, not used by Apple, just for showing i3 in "About This Mac".
        ProcessorType->Type = AppleProcessorTypeCorei3Type5; // 0x0905
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI9) {
        // for i9 (SKL-X), not used by Apple, just for showing i9 in "About This Mac".
        // FIXME: i9 was not introdced in this era but later (MBP151, Coffee Lake),
        //        will AppleProcessorMajorI9 work here?
        // NOTE: using a mostly invalid value 0x1005 for now...
        ProcessorType->Type = AppleProcessorTypeCorei9Type5; // 0x1005
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Skylake E3 (there's no E5/E7 on Skylake), not used by Apple
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for Skylake based Xeon E3
        ProcessorType->Type = AppleProcessorTypeXeon;        // 0x0501
      } else {
        // here stands for Pentium and Celeron (Skylake), not used by Apple at all.
        // putting 0x0905 (i3) as lowest.
        ProcessorType->Type = AppleProcessorTypeCorei3Type5; // 0x0905
      }

      break;

    //
    // Kaby Lake:   https://en.wikipedia.org/wiki/Kaby_Lake#List_of_7th_generation_Kaby_Lake_processors
    // Coffee Lake: https://en.wikipedia.org/wiki/Coffee_Lake#List_of_8th_generation_Coffee_Lake_processors
    //
    // Used by Apple:
    //   Core m3    [Kaby],
    //   Core i5/i7 [Kaby/Coffee],
    //   Core i9    [Coffee],
    //
    // Not used by Apple:
    //   Core i3    [Kaby/Coffee],
    //   Xeon E3 v6 [Kaby],
    //   Xeon E     [Coffee],
    //   Pentium, Celeron
    //
    case CPU_MODEL_KABYLAKE:       // 0x8E
    case CPU_MODEL_COFFEELAKE:     // 0x9E

      if (CpuInfo->AppleMajorType == AppleProcessorMajorM3) {
        // MB101 (m3 7Y32)
        ProcessorType->Type = AppleProcessorTypeCoreM3Type7; // 0x0C07
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI5) {
        // Kaby has 0x9 stepping, and Coffee use 0xA / 0xB stepping.
        if (CpuInfo->Stepping == 9) {
          // IM181 (i5-7360U), IM182  (i5-7400), IM183 (i5-7600)
          // MBP141 (i5-7360U), MBP142 (i5-7267U)
          ProcessorType->Type = AppleProcessorTypeCorei5Type5; // 0x0605
        } else {
          // MM81 (i5-8500B)
          // MBP152 (i5-8259U)
          ProcessorType->Type = AppleProcessorTypeCorei5Type9; // 0x0609
        }
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI7) {
        // FIXME: used by Apple, but not sure what to use...
        // 0x0709 is used on MBP151 (i7-8850H),
        // 0x0705 is not confirmed, just an ideal one comparing to 0x0605 (AppleProcessorTypeCorei5Type5)
        // using 0x0705 for now
        ProcessorType->Type = AppleProcessorTypeCorei7Type5; // 0x0705
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI9) {
        // FIXME: find a dump from MBP151 with i9-8950HK,
        // for now using an ideal value (0x1009), comparing to 0x0709 (used on MBP151, i7-8850H)
        ProcessorType->Type = AppleProcessorTypeCorei9Type9; // 0x1009
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorI3) {
        // for i3, not used by Apple, just for showing i3 in "About This Mac".
        ProcessorType->Type = AppleProcessorTypeCorei3Type5; // 0x0905
      } else if (CpuInfo->AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Kaby Lake/Coffee Lake E3 (there's no E5/E7 on either), not used by Apple
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for KBL/CFL based Xeon E3
        ProcessorType->Type = AppleProcessorTypeXeon;        // 0x0501
      } else {
        // here stands for Pentium and Celeron (KBL/CFL), not used by Apple at all.
        // putting 0x0905 (i3) as lowest.
        ProcessorType->Type = AppleProcessorTypeCorei3Type5; // 0x0905
      }
      break;

    default:
      // NOTE: by default it should be unknown
      ProcessorType->Type = AppleProcessorTypeCorei5Type5; // 0x0605
      break;
  }

  SmbiosFinaliseStruct (Table);
}

/** Type 132

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.

  @retval
**/
STATIC
VOID
PatchAppleProcessorSpeed (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data,
  IN     CPU_INFO         *CpuInfo
  )
{
#ifndef OC_PROVIDE_APPLE_PROCESSOR_BUS_SPEED
  //
  // I believe this table is no longer used.
  // The code below may be inaccurate as well, since it expects KHz.
  //
  (VOID) Table;
  (VOID) Data;
  (VOID) CpuInfo;
#else
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original    = SmbiosGetOriginalStructure (APPLE_SMBIOS_TYPE_PROCESSOR_BUS_SPEED, 1);
  MinLength   = sizeof (*Original.Type132);
  StringIndex = 0;

  if (EFI_ERROR (SmbiosInitialiseStruct (Table, APPLE_SMBIOS_TYPE_PROCESSOR_BUS_SPEED, MinLength, 1))) {
    return;
  }

  if (CpuInfo->Model >= CPU_MODEL_SANDYBRIDGE) {
    Table->CurrentPtr.Type132->ProcessorBusSpeed = OC_CPU_SNB_QPB_CLOCK * 1000;
  } else {
    Table->CurrentPtr.Type132->ProcessorBusSpeed = (UINT16) DivU64x32 (CpuInfo->FSBFrequency, 1000);
  }

  SmbiosFinaliseStruct (Table);
#endif
}

/** Type 127

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
STATIC
VOID
PatchSmBiosEndOfTable (
  IN OUT OC_SMBIOS_TABLE  *Table,
  IN     OC_SMBIOS_DATA   *Data
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  Original;
  UINT8                           MinLength;
  UINT8                           StringIndex;

  Original    = SmbiosGetOriginalStructure (SMBIOS_TYPE_END_OF_TABLE, 1);
  MinLength   = sizeof (*Original.Standard.Type127);
  StringIndex = 0;

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
        DEBUG ((DEBUG_INFO, "LegacyRegionUnlock DMI anchor failure - %r\n", Status));
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
        DEBUG ((DEBUG_INFO, "LegacyRegionUnlock DMI table failure - %r\n", Status));
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
        DEBUG ((DEBUG_INFO, "LegacyRegionLock DMI table failure - %r\n", Status));
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
        DEBUG ((DEBUG_INFO, "LegacyRegionLock DMI anchor failure - %r\n", Status));
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Prepare new SMBIOS table based on host data.

  @param  SmbiosTable

  @retval EFI_SUCCESS if buffer is ready to be filled.
**/
STATIC
EFI_STATUS
SmbiosPrepareTable (
  IN OUT OC_SMBIOS_TABLE  *SmbiosTable
  )
{
  EFI_STATUS  Status;
  UINT32      BufferLen;

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
      DEBUG ((DEBUG_WARN, "SmbiosLookupHost entry is too small - %u/%u bytes\n",
        mOriginalSmbios->EntryPointLength, (UINT32) sizeof (SMBIOS_TABLE_ENTRY_POINT)));
      mOriginalSmbios = NULL;
    } else if (mOriginalSmbios->TableAddress == 0
      || mOriginalSmbios->TableLength == 0
      || mOriginalSmbios->TableLength > SMBIOS_TABLE_MAX_LENGTH) {
      DEBUG ((DEBUG_WARN, "SmbiosLookupHost entry has invalid table - %08X of %u bytes\n",
        mOriginalSmbios->TableAddress, mOriginalSmbios->TableLength));
      mOriginalSmbios = NULL;
    }
  } else {
    DEBUG ((DEBUG_WARN, "SmbiosLookupHost failed to lookup SMBIOSv1 - %r\n", Status));
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
      DEBUG ((DEBUG_INFO, "SmbiosLookupHost v3 entry is too small - %u/%u bytes\n",
        mOriginalSmbios3->EntryPointLength, (UINT32) sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT)));
      mOriginalSmbios3 = NULL;
    } else if (mOriginalSmbios3->TableAddress == 0
      || mOriginalSmbios3->TableMaximumSize == 0
      || mOriginalSmbios3->TableMaximumSize > SMBIOS_3_0_TABLE_MAX_LENGTH) {
      DEBUG ((DEBUG_INFO, "SmbiosLookupHost v3 entry has invalid table - %016LX of %u bytes\n",
        mOriginalSmbios3->TableAddress, mOriginalSmbios3->TableMaximumSize));
      mOriginalSmbios3 = NULL;
    }
  } else {
    DEBUG ((DEBUG_INFO, "SmbiosLookupHost failed to lookup SMBIOSv3 - %r\n", Status));
  }

  //
  // TODO: I think we may want to try harder.
  //

  //
  // Pad the table length to a page and calculate byte size.
  //
  if (mOriginalSmbios != NULL) {
    mOriginalTableSize = mOriginalSmbios->TableLength;
    mOriginalTable.Raw = (UINT8 *)(UINTN) mOriginalSmbios->TableAddress;
  } else if (mOriginalSmbios3 != NULL) {
    mOriginalTableSize = mOriginalSmbios3->TableMaximumSize;
    mOriginalTable.Raw = (UINT8 *)(UINTN) mOriginalSmbios3->TableAddress;
  } else {
    BufferLen = EFI_PAGE_SIZE;
  }

  if (mOriginalSmbios != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "Found DMI Anchor %08LX v%u.%u Table Address %08LX Length %04X\n",
      (UINT64) mOriginalSmbios,
      mOriginalSmbios->MajorVersion,
      mOriginalSmbios->MinorVersion,
      (UINT64) mOriginalSmbios->TableAddress,
      mOriginalSmbios->TableLength
      ));
  }

  if (mOriginalSmbios3 != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "Found DMI Anchor %08LX v%u.%u Table Address %08LX Length %04X\n",
      (UINT64) mOriginalSmbios3,
      mOriginalSmbios3->MajorVersion,
      mOriginalSmbios3->MinorVersion,
      mOriginalSmbios3->TableAddress,
      mOriginalSmbios3->TableMaximumSize
      ));
  }

  Status = SmbiosExtendTable (SmbiosTable, 1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "SmbiosLookupHost failed to initialise smbios table - %r\n", Status));
  }

  return Status;
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
    *TableAddress = (VOID *) TmpAddr;
    TmpAddr = (BASE_4GB - 1);
    Status = gBS->AllocatePages (AllocateMaxAddress, EfiReservedMemoryType, 1,  &TmpAddr);
    if (!EFI_ERROR (Status)) {
      *TableEntryPoint = (SMBIOS_TABLE_ENTRY_POINT *) TmpAddr;
      if (mOriginalSmbios3 != NULL) {
        TmpAddr = (BASE_4GB - 1);
        Status = gBS->AllocatePages (AllocateMaxAddress, EfiReservedMemoryType, 1, &TmpAddr);
        if (!EFI_ERROR (Status)) {
          *TableEntryPoint3 = (SMBIOS_TABLE_3_0_ENTRY_POINT *) TmpAddr;
          *TableAddress3 = *TableAddress;
        }
      }
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "SmbiosTableAllocate aborts as it cannot allocate SMBIOS pages with %d %d %d\n",
      *TableEntryPoint != NULL, *TableAddress != NULL, *TableEntryPoint3 != NULL));
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
    || Mode == OcSmbiosUpdateAuto
    );

  //
  // We check maximum table size during table extension, so this cast is valid.
  //
  TableLength = (UINT16) (SmbiosTable->CurrentPtr.Raw - SmbiosTable->Table);

  if (Mode == OcSmbiosUpdateAuto || Mode == OcSmbiosUpdateOverwrite) {
    Status = SmbiosHandleLegacyRegion (TRUE);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "SmbiosTableApply(%u) cannot handle legacy region - %r\n", Mode, Status));
      if (Mode == OcSmbiosUpdateOverwrite) {
        return Status;
      }

      //
      // Fallback to create mode.
      //
      Mode = OcSmbiosUpdateCreate;
    } else if (mOriginalSmbios == NULL || TableLength > mOriginalSmbios->TableLength
      || mOriginalSmbios->EntryPointLength < sizeof (SMBIOS_TABLE_ENTRY_POINT)) {
      DEBUG ((DEBUG_WARN, "SmbiosTableApply(%u) cannot update old SMBIOS (%p, %u, %u) with %u\n",
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
      DEBUG ((DEBUG_WARN, "SmbiosTableApply(%u) cannot fit SMBIOSv3 (%p, %u, %u) with %u\n",
        Mode, mOriginalSmbios3, mOriginalSmbios3->EntryPointLength, mOriginalSmbios3->TableMaximumSize, TableLength));
      Mode = OcSmbiosUpdateCreate;
    } else {
      Mode = OcSmbiosUpdateOverwrite;
    }
  }

  ASSERT (Mode != Mode == OcSmbiosUpdateAuto);

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
    TableAddress3 = mOriginalSmbios3 != NULL ? (VOID *)(UINTN) TableAddress : NULL;
  }

  CopyMem (TableAddress,
    SmbiosTable->Table,
    TableLength
    );

  if (TableAddress3 != NULL && TableAddress3 != TableAddress) {
    CopyMem (TableAddress3,
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
  TableEntryPoint->EntryPointStructureChecksum = CalculateCheckSum8 (
    (UINT8 *) TableEntryPoint,
    SMBIOS_ENTRY_POINT_CHECKSUM_SIZE
    );
  TableEntryPoint->IntermediateChecksum        = CalculateCheckSum8 (
    (UINT8 *) TableEntryPoint + SMBIOS_ENTRY_POINT_CHECKSUM_SIZE,
    TableEntryPoint->EntryPointLength - SMBIOS_ENTRY_POINT_CHECKSUM_SIZE
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
    TableEntryPoint3->TableAddress                = (UINT32)(UINTN) TableAddress;
    TableEntryPoint3->EntryPointStructureChecksum = CalculateCheckSum8 (
      (UINT8 *) TableEntryPoint3,
      TableEntryPoint3->EntryPointLength
      );
  }

  DEBUG ((
    DEBUG_INFO,
    "Patched %08LX v%d.%d Table Address %08LX Length %04X %X %X\n",
    (UINT64) TableEntryPoint,
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
        &gEfiSmbios3TableGuid,
        TableEntryPoint3
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "Failed to install v3 table - %r\n", Status));
      }
    }

    Status  = gBS->InstallConfigurationTable (
      Mode == OcSmbiosUpdateCustom ? &gOcCustomSmbiosTableGuid : &gEfiSmbiosTableGuid,
      TableEntryPoint
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to install v1 table - %r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**

  @retval EFI_SUCCESS               The smbios tables were generated successfully
**/
EFI_STATUS
CreateSmbios (
  IN OC_SMBIOS_DATA         *Data,
  IN OC_SMBIOS_UPDATE_MODE  Mode
  )
{
  EFI_STATUS                      Status;
  OC_SMBIOS_TABLE                 SmbiosTable;
  CPU_INFO                        CpuInfo;
  SMBIOS_HANDLE                   MemoryDeviceHandle;
  APPLE_SMBIOS_STRUCTURE_POINTER  MemoryDeviceInfo;
  APPLE_SMBIOS_STRUCTURE_POINTER  MemoryDeviceAddress;
  UINT16                          NumberMemoryDevices;
  UINT8                           MemoryDeviceNo;
  UINT8                           RamModuleNo;


  ASSERT (Data != NULL);

  Status = SmbiosPrepareTable (&SmbiosTable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OcCpuScanProcessor (&CpuInfo);

  PatchBiosInformation (&SmbiosTable, Data);
  PatchSystemInformation (&SmbiosTable, Data);
  PatchBaseboardInformation (&SmbiosTable, Data);
  PatchSystemEnclosure (&SmbiosTable, Data);
  PatchProcessorInformation (&SmbiosTable, Data, &CpuInfo);
  PatchCacheInformation (&SmbiosTable, Data);
  PatchSystemPorts (&SmbiosTable, Data);
  PatchSystemSlots (&SmbiosTable, Data);
  PatchMemoryArray (&SmbiosTable, Data);
  PatchMemoryMappedAddress (&SmbiosTable, Data);

  NumberMemoryDevices = SmbiosGetOriginalStructureCount (SMBIOS_TYPE_MEMORY_DEVICE);

  for (MemoryDeviceNo = 1, RamModuleNo = 1; MemoryDeviceNo <= NumberMemoryDevices; MemoryDeviceNo++) {
    MemoryDeviceInfo = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_DEVICE, MemoryDeviceNo);

    if (MemoryDeviceInfo.Raw == NULL) {
      continue;
    }

    //
    // For each memory device we must generate type 17
    //
    PatchMemoryDevice (
      &SmbiosTable,
      Data,
      MemoryDeviceInfo,
      MemoryDeviceNo,
      &MemoryDeviceHandle
    );

    MemoryDeviceAddress = SmbiosGetOriginalStructure (SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, RamModuleNo);
    if (MemoryDeviceAddress.Raw != NULL) {
      //
      // For each occupied memory device we must generate type 20
      //

      //
      // FIXME: While I believe it may be the case that Type20 and Type17 are sorted,
      // it is not guaranteed by the spec. Rework the matching to do a proper nested loop.
      //
      //ASSERT (MemoryDeviceAddress.Standard.Type20->MemoryDeviceHandle ==
      //    MemoryDeviceInfo.Standard.Type17->Hdr.Handle);

      if (MemoryDeviceAddress.Standard.Type20->MemoryDeviceHandle ==
          MemoryDeviceInfo.Standard.Type17->Hdr.Handle) {
        PatchMemoryMappedDevice (
          &SmbiosTable,
          Data,
          MemoryDeviceAddress,
          RamModuleNo,
          MemoryDeviceHandle
          );

        //
        // TODO: Type 130 - Apple Memory SPD Info
        //

        RamModuleNo++;
      }
    }
  }

  PatchPortableBatteryDevice (&SmbiosTable, Data);
  PatchBootInformation (&SmbiosTable, Data);
  PatchAppleProcessorType (&SmbiosTable, Data, &CpuInfo);
  PatchAppleProcessorSpeed (&SmbiosTable, Data, &CpuInfo);
  PatchAppleFirmwareVolume (&SmbiosTable, Data);
  PatchSmBiosEndOfTable (&SmbiosTable, Data);

  Status = SmbiosTableApply (&SmbiosTable, Mode);

  SmbiosTableFree (&SmbiosTable);

  return Status;
}

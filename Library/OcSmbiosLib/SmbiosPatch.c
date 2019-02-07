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
#include <Library/OcMiscLib.h>
#include <Library/OcProtocolLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/AppleSmBios.h>
#include <IndustryStandard/Pci.h>

#include "DebugSmbios.h"
#include "OcSmbiosInternal.h"

#include <Macros.h>
#include <ProcessorInfo.h>

//FIXME: REMOVE
#define APPLE_FIRMWARE_FEATURES_VARIABLE        "FirmwareFeatures"
#define APPLE_FIRMWARE_FEATURES_MASK_VARIABLE   "FirmwareFeaturesMask"

#define OPEN_CORE_BIOS_VERSION_VARIABLE          "BiosVersion"
#define OPEN_CORE_BIOS_DATE_VARIABLE             "BiosDate"
#define OPEN_CORE_PRODUCT_NAME_VARIABLE          "ProductName"
#define OPEN_CORE_PRODUCT_FAMILY_VARIABLE        "ProductFamily"
#define OPEN_CORE_SYSTEM_VERSION_VARIABLE        "SystemVersion"
#define OPEN_CORE_SYSTEM_SERIAL_VARIABLE         "SystemSerial"
#define OPEN_CORE_PRODUCT_ID_VARIABLE            "ProductId"
#define OPEN_CORE_BOARD_VERSION_VARIABLE         "BoardVersion"
#define OPEN_CORE_BASE_BOARD_SERIAL_VARIABLE     "BaseBoardSerial"
#define OPEN_CORE_MANUFACTURER_VARIABLE          "Manufacturer"
#define OPEN_CORE_PROCESSOR_SERIAL_VARIABLE      "ProcessorSerial"

#define OPEN_CORE_SYSTEM_SKU_VARIABLE            "SystemSKU"
#define OPEN_CORE_BASE_BOARD_ASSET_TAG_VARIABLE  "BaseBoardAssetTag"
#define OPEN_CORE_CHASSIS_ASSET_TAG_VARIABLE     "ChassisAssetTag"

#define OPEN_CORE_CPU_TYPE_VARIABLE              "CpuType"
#define OPEN_CORE_HARDWARE_SIGNATURE_VARIABLE    "HardwareSignature"
#define OPEN_CORE_HARDWARE_ADDRESS_VARIABLE      "HardwareAddress"
#define OPEN_CORE_ENCLOSURE_TYPE_VARIABLE        "EnclosureType"

#define OPEN_CORE_FIRMWARE_VENDOR_VARIABLE       "FirmwareVendor"
#define OPEN_CORE_FIRMWARE_REVISION_VARIABLE     "FirmwareRevision"

EFI_GUID gOpenCoreOverridesGuid;


SMBIOS_TABLE_ENTRY_POINT        *mSmbios;

APPLE_SMBIOS_STRUCTURE_POINTER   mOriginal;
APPLE_SMBIOS_STRUCTURE_POINTER   This;

UINT16  mTableLength;
UINT16  mMaxStructureSize;
UINT16  mNumberOfSmbiosStructures;

//
typedef enum {
  iMac            = 0x01,
  MacAir          = 0x02,
  MacMini         = 0x03,
  MacBook         = 0x04,
  MacBookPro      = 0x05,
  MacPro          = 0x06
} OC_MODEL_TYPE_DATA;

// SmBiosFinaliseStruct
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] Length                 Length of the table to finalise.

  @retval
**/
VOID
SmBiosFinaliseStruct (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  UINTN           Length
  )
{
  if (Length > mMaxStructureSize) {
    mMaxStructureSize = Length;
  }

  Length++;

  mTableLength += Length;
  mNumberOfSmbiosStructures++;

  (*Handle)++;

  *Table  += Length;

}

// PatchBiosInformation
/** Type 0

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchBiosInformation (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_BIOS_INFORMATION, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE0   *p = (SMBIOS_TABLE_TYPE0 *)*(Table);

    CHAR8   *Strings        = PTR_OFFSET(p, mOriginal.Standard.Type0->Hdr.Length, CHAR8 *);
    UINT8   StringsIndex    = 0;

    CopyMem (p, mOriginal.Standard.Type0, mOriginal.Standard.Type0->Hdr.Length);

    p->Hdr.Handle           = *(Handle);

    p->Vendor               = SmbiosSetOverrideString (&Strings, OPEN_CORE_MANUFACTURER_VARIABLE, &StringsIndex);
    p->BiosVersion          = SmbiosSetOverrideString (&Strings, OPEN_CORE_BIOS_VERSION_VARIABLE, &StringsIndex);
    p->BiosReleaseDate      = SmbiosSetOverrideString (&Strings, OPEN_CORE_BIOS_DATE_VARIABLE, &StringsIndex);

    // ensure checks for IODeviceTree:/rom@0 or IODeviceTree:/rom@e0000 are met.
    p->BiosSegment          = 0x0;

    SmBiosFinaliseStruct (Table, Handle, ((UINTN)Strings - (UINTN)p));

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugBiosInformation (This);

    DEBUG_CODE_END();
  }
}

// PatchSystemInformation
/** Type 1

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchSystemInformation (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_SYSTEM_INFORMATION, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE1  *p = (SMBIOS_TABLE_TYPE1 *)*(Table);

    CHAR8   *Strings       = PTR_OFFSET(p, mOriginal.Standard.Type1->Hdr.Length, CHAR8 *);
    UINT8   StringsIndex   = 0;

    EFI_GUID  *PlatformUuid;
    UINTN     PlatformUuidSize;

    CopyMem (p, mOriginal.Standard.Type1, mOriginal.Standard.Type1->Hdr.Length);

    p->Hdr.Handle    = *(Handle);

    p->Manufacturer  = SmbiosSetOverrideString (&Strings, OPEN_CORE_MANUFACTURER_VARIABLE, &StringsIndex);
    p->ProductName   = SmbiosSetOverrideString (&Strings, OPEN_CORE_PRODUCT_NAME_VARIABLE, &StringsIndex);
    p->Version       = SmbiosSetOverrideString (&Strings, OPEN_CORE_SYSTEM_VERSION_VARIABLE, &StringsIndex);
    p->SerialNumber  = SmbiosSetOverrideString (&Strings, OPEN_CORE_SYSTEM_SERIAL_VARIABLE, &StringsIndex);
    p->SKUNumber     = SmbiosSetOverrideString (&Strings, OPEN_CORE_SYSTEM_SKU_VARIABLE, &StringsIndex);
    p->Family        = SmbiosSetOverrideString (&Strings, OPEN_CORE_PRODUCT_FAMILY_VARIABLE, &StringsIndex);

    PlatformUuidSize = 0;
    PlatformUuid     = NULL;

    OcGetVariable (
      OPEN_CORE_HARDWARE_SIGNATURE_VARIABLE,
      &gOpenCoreOverridesGuid,
      (VOID **)&PlatformUuid,
      &PlatformUuidSize
      );

    if (PlatformUuid != NULL) {
      if (PlatformUuidSize != EFI_GUID_STRING_LENGTH) {

        DEBUG ((
          DEBUG_ERROR,
          "Variable %a is of incorrect size should be %d, but is %d",
          OPEN_CORE_HARDWARE_SIGNATURE_VARIABLE,
          EFI_GUID_STRING_LENGTH,
          PlatformUuidSize
          ));

      } else {

        // Ensure User Expected Value
        PlatformUuid->Data1 = SwapBytes32 (PlatformUuid->Data1);
        PlatformUuid->Data2 = SwapBytes16 (PlatformUuid->Data2);
        PlatformUuid->Data3 = SwapBytes16 (PlatformUuid->Data3);

        OcAsciiStrToGuid ((CHAR8 *)(UINTN)PlatformUuid, &p->Uuid);

      }

      FreePool (PlatformUuid);

    }

    SmBiosFinaliseStruct (Table, Handle, ((UINTN)Strings - (UINTN)p));

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugSystemInformation (This);

    DEBUG_CODE_END();
  }
}

// PatchBaseboardInformation
/** Type 2

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchBaseboardInformation (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  UINT8           MacModel
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_BASEBOARD_INFORMATION, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE2  *p = (SMBIOS_TABLE_TYPE2 *)*(Table);

    CHAR8   *Strings       = PTR_OFFSET(p, mOriginal.Standard.Type2->Hdr.Length, CHAR8 *);
    UINT8   StringsIndex   = 0;

    CopyMem (p, mOriginal.Standard.Type2, mOriginal.Standard.Type2->Hdr.Length);

    p->Hdr.Handle    = *(Handle);

    // FIXME: This requires the type 2 structure to come next

    p->ChassisHandle  = *(Handle) + 1;

    p->Manufacturer   = SmbiosSetOverrideString (&Strings, OPEN_CORE_MANUFACTURER_VARIABLE, &StringsIndex);
    p->ProductName    = SmbiosSetOverrideString (&Strings, OPEN_CORE_PRODUCT_ID_VARIABLE, &StringsIndex);

    This.Raw = (UINT8 *)p;

    if (MacModel == MacPro) {
      p->SerialNumber = SmbiosSetOverrideString (&Strings, OPEN_CORE_PROCESSOR_SERIAL_VARIABLE, &StringsIndex);
      p->BoardType    = BaseBoardTypeProcessorMemoryModule;

      p->FeatureFlag.Removable = 1;

    } else {
      p->SerialNumber = SmbiosSetOverrideString (&Strings, OPEN_CORE_BASE_BOARD_SERIAL_VARIABLE, &StringsIndex);
      p->BoardType    = BaseBoardTypeMotherBoard;
    }

    p->Version        = SmbiosSetOverrideString (&Strings, OPEN_CORE_BOARD_VERSION_VARIABLE, &StringsIndex);
    p->AssetTag       = SmbiosSetOverrideString (&Strings, OPEN_CORE_BASE_BOARD_ASSET_TAG_VARIABLE, &StringsIndex);

    p->LocationInChassis = SmbiosSetString (&Strings, "Part Component", &StringsIndex);

    SmBiosFinaliseStruct (Table, Handle, ((UINTN)Strings - (UINTN)p));

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugBaseboardInformation (This);

    DEBUG_CODE_END();
  }
}

// PatchSystemEnclosure
/** Type 3

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchSystemEnclosure (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  UINT8           MacModel
  )
{
  EFI_STATUS Status;

  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_SYSTEM_ENCLOSURE, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE3  *p = (SMBIOS_TABLE_TYPE3 *)*(Table);

    CHAR8   *Strings       = PTR_OFFSET(p, mOriginal.Standard.Type3->Hdr.Length, CHAR8 *);
    UINT8   StringsIndex   = 0;

    UINT8   *Type;
    UINTN   Length;

    CopyMem (p, mOriginal.Standard.Type3, mOriginal.Standard.Type3->Hdr.Length);

    p->Hdr.Handle   = *(Handle);

    p->Manufacturer = SmbiosSetOverrideString (&Strings, OPEN_CORE_MANUFACTURER_VARIABLE, &StringsIndex);
    p->Version      = SmbiosSetOverrideString (&Strings, OPEN_CORE_PRODUCT_NAME_VARIABLE, &StringsIndex);
    p->SerialNumber = SmbiosSetOverrideString (&Strings, OPEN_CORE_SYSTEM_SERIAL_VARIABLE, &StringsIndex);
    p->AssetTag     = SmbiosSetOverrideString (&Strings, OPEN_CORE_CHASSIS_ASSET_TAG_VARIABLE, &StringsIndex);

    Length = sizeof(p->Type);
    Type   = &p->Type;

    Status = OcGetVariable (
               OPEN_CORE_ENCLOSURE_TYPE_VARIABLE,
               &gOpenCoreOverridesGuid,
               (VOID **)&Type,
               &Length
               );

    if ((EFI_ERROR (Status)) && (Length != sizeof(p->Type))) {
      if (MacModel == MacPro) {
        p->Type = MiscChassisTypeTower;
      } else {
        p->Type = MiscChassisTypeDeskTop;
      }
    }

    {
      UINTN   Offset     = MultU64x32 (p->ContainedElementCount, p->ContainedElementRecordLength) + 0x15;
      UINT8   *SkuNumber = PTR_OFFSET(p, Offset, UINT8 *);

      *SkuNumber = SmbiosSetString (
                     &Strings,
                     SmbiosGetString (mOriginal, *(PTR_OFFSET(mOriginal.Raw, Offset, UINT8 *))),
                     &StringsIndex
                     );


    }

    SmBiosFinaliseStruct (Table, Handle, ((UINTN)Strings - (UINTN)p));

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugSystemEnclosure (This);

    DEBUG_CODE_END();
  }
}

// PatchProcessorInformation
/** Type 4

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] L1CacheHandle          The discovered L1 cache handle value.
  @param[in] L2CacheHandle          The discovered L2 cache handle value.
  @param[in] L2CacheHandle          The discovered L3 cache handle value.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.

  @retval
**/
VOID
PatchProcessorInformation (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  SMBIOS_HANDLE   L1CacheHandle,
  IN  SMBIOS_HANDLE   L2CacheHandle,
  IN  SMBIOS_HANDLE   L3CacheHandle,
  IN  CPU_INFO        *CpuInfo
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_PROCESSOR_INFORMATION, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE4  *p = (SMBIOS_TABLE_TYPE4 *)*(Table);

    CHAR8   *Strings       = PTR_OFFSET(p, mOriginal.Standard.Type4->Hdr.Length, CHAR8 *);
    UINT8   StringsIndex   = 0;

    CopyMem (p, mOriginal.Standard.Type4, mOriginal.Standard.Type4->Hdr.Length);

    p->Hdr.Handle = *(Handle);

    p->Socket = SmbiosSetString (
                  &Strings,
                  SmbiosGetString (mOriginal, mOriginal.Standard.Type4->Socket),
                  &StringsIndex
                  );

    p->ProcessorManufacture = SmbiosSetString (
                                &Strings,
                                SmbiosGetString (mOriginal, mOriginal.Standard.Type4->ProcessorManufacture),
                                &StringsIndex
                                );

    p->ProcessorVersion     = SmbiosSetString (&Strings, CpuInfo->BrandString, &StringsIndex);
    p->AssetTag             = p->ProcessorVersion;

    // Models newer than Sandybridge require quad pumped bus value instead of a front side bus value.
    if (CpuInfo->Model < CPU_MODEL_SANDYBRIDGE) {
      p->ExternalClock = (UINT16)DivU64x32 (CpuInfo->FSBFrequency, 1000000);
    } else {
      p->ExternalClock = 0x19;
    }

    p->MaxSpeed = (UINT16)DivU64x32 (CpuInfo->CPUFrequency, 1000000);

    if ((p->MaxSpeed % 100) != 0) {
      p->MaxSpeed = (UINT16)(((p->MaxSpeed + 50) / 100) * 100);
    }

    p->CurrentSpeed  = p->MaxSpeed;

    p->L1CacheHandle = L1CacheHandle;
    p->L2CacheHandle = L2CacheHandle;
    p->L3CacheHandle = L3CacheHandle;

    //

    p->SerialNumber  = SmbiosSetString (&Strings, "Unavailable" , &StringsIndex);
    p->PartNumber    = SmbiosSetString (&Strings, "Unavailable" , &StringsIndex);

    SmBiosFinaliseStruct (Table, Handle, ((UINTN)Strings - (UINTN)p));

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugProcessorInformation (mSmbios, This);

    DEBUG_CODE_END();
  }
}

// PatchCacheInformation
/** Type 7

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] L1CacheHandle          Pointer to location to store the discovered L1 cache handle value.
  @param[in] L2CacheHandle          Pointer to location to store the discovered L2 cache handle value.
  @param[in] L3CacheHandle          Pointer to location to store the discovered L3 cache handle value.

  @retval
**/
VOID
PatchCacheInformation (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  OUT SMBIOS_HANDLE   *L1CacheHandle,
  OUT SMBIOS_HANDLE   *L2CacheHandle,
  OUT SMBIOS_HANDLE   *L3CacheHandle
  )
{
  APPLE_SMBIOS_STRUCTURE_POINTER  OriginalProcessor;

  UINT8  NumberCacheEntries;
  UINT8  CacheEntryNo;

  OriginalProcessor  = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_PROCESSOR_INFORMATION, 1);
  NumberCacheEntries = SmbiosGetTableCount (mSmbios, SMBIOS_TYPE_CACHE_INFORMATION);

  for (CacheEntryNo = 1; CacheEntryNo <= NumberCacheEntries; CacheEntryNo++) {

    mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_CACHE_INFORMATION, CacheEntryNo);

    if (mOriginal.Raw != NULL) {

      SMBIOS_TABLE_TYPE7  *p = (SMBIOS_TABLE_TYPE7 *)*(Table);
      UINTN      TableLength = SmbiosGetTableLength (mOriginal);

      CopyMem (p, mOriginal.Standard.Type7, TableLength);

      p->Hdr.Handle = *(Handle);

      // Use the original type 4 cache handles to set correct new handles for type 4 creation.
      if (mOriginal.Standard.Type7->Hdr.Handle == OriginalProcessor.Standard.Type4->L1CacheHandle) {
        *L1CacheHandle = p->Hdr.Handle;
      } else if (mOriginal.Standard.Type7->Hdr.Handle == OriginalProcessor.Standard.Type4->L2CacheHandle) {
        *L2CacheHandle = p->Hdr.Handle;
      } else if (mOriginal.Standard.Type7->Hdr.Handle == OriginalProcessor.Standard.Type4->L3CacheHandle) {
        *L3CacheHandle = p->Hdr.Handle;
      }

      SmBiosFinaliseStruct (Table, Handle, --TableLength);

      DEBUG_CODE_BEGIN();

      This.Raw = (UINT8 *)p;

      SmbiosDebugCacheInformation (This);

      DEBUG_CODE_END();
    }
  }
}

// PatchSystemPorts
/** Type 8

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchSystemPorts (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  UINT8  NumberPortEntries;
  UINT8  PortNo;

  NumberPortEntries = SmbiosGetTableCount (mSmbios, SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION);

  for (PortNo = 1; PortNo <= NumberPortEntries; PortNo++) {

    mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, PortNo);

    if (mOriginal.Raw != NULL) {

      SMBIOS_TABLE_TYPE8  *p = (SMBIOS_TABLE_TYPE8 *)*(Table);

      UINTN           Length = SmbiosGetTableLength (mOriginal);

      CopyMem (p, mOriginal.Standard.Type8, Length);

      p->Hdr.Handle = *(Handle);

      SmBiosFinaliseStruct (Table, Handle, --Length);

      DEBUG_CODE_BEGIN();

      This.Raw = (UINT8 *)p;

      SmbiosDebugSystemPorts (This);

      DEBUG_CODE_END();
    }
  }
}

// PatchSystemSlots
/** Type 9

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchSystemSlots (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  EFI_STATUS  Status;

  UINT8  NumberSlotEntries;
  UINT8  SlotNo;

  PciRootBridgeIo = NULL;

  Status          = SafeLocateProtocol (
                      &gEfiPciRootBridgeIoProtocolGuid,
                      NULL,
                      (VOID **)&PciRootBridgeIo
                      );

  NumberSlotEntries = SmbiosGetTableCount (mSmbios, SMBIOS_TYPE_SYSTEM_SLOTS);

  for (SlotNo = 1; SlotNo <= NumberSlotEntries; SlotNo++) {

    mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_SYSTEM_SLOTS, SlotNo);

    if (mOriginal.Raw != NULL) {

      SMBIOS_TABLE_TYPE9  *p   = (SMBIOS_TABLE_TYPE9 *)*(Table);
      CHAR8   *Strings         = PTR_OFFSET(p, mOriginal.Standard.Type9->Hdr.Length, CHAR8 *);
      UINT8   StringsIndex     = 0;

      CHAR8   *SlotDesignation;
      UINT64  PciAddress;

      CopyMem (p, mOriginal.Standard.Type9, mOriginal.Standard.Type9->Hdr.Length);

      p->Hdr.Handle = *(Handle);

      // Correct firmware data
      if (mOriginal.Standard.Type9->BusNum != 0) {
        PciAddress = CALC_EFI_PCI_ADDRESS(BITFIELD(mOriginal.Standard.Type9->DevFuncNum, 3, 7),
                                          mOriginal.Standard.Type9->BusNum,
                                          BITFIELD(mOriginal.Standard.Type9->DevFuncNum, 2, 0),
                                          0);
      } else {
        PciAddress = CALC_EFI_PCI_ADDRESS(mOriginal.Standard.Type9->BusNum,
                                          BITFIELD(mOriginal.Standard.Type9->DevFuncNum, 3, 7),
                                          BITFIELD(mOriginal.Standard.Type9->DevFuncNum, 2, 0),
                                          0);
      }

      SlotDesignation = NULL;
      p->CurrentUsage = SlotUsageAvailable;

      if (!EFI_ERROR (Status)) {

        UINT8     SecSubBus = MAX_UINT8;

        PciRootBridgeIo->Pci.Read (
                               PciRootBridgeIo,
                               EfiPciWidthUint8,
                               (PciAddress + PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET),
                               1,
                               &SecSubBus
                               );

        if (SecSubBus != MAX_UINT8) {

          UINT16  ClassCode = MAX_UINT16;

          PciRootBridgeIo->Pci.Read (
                                 PciRootBridgeIo,
                                 EfiPciWidthUint16,
                                 CALC_EFI_PCI_ADDRESS (SecSubBus, 0, 0, 0) + 10,
                                 1,
                                 &ClassCode
                                 );

          if (ClassCode != MAX_UINT16) {
            p->CurrentUsage = SlotUsageInUse;
            if (ClassCode == 0x0280) {
              SlotDesignation = "AirPort";
            }
            // FIXME: Expand checks since not all nvme uses m.2 slot
            if (ClassCode == 0x0108) {
              SlotDesignation = "M.2";
            }
          }
        }
      }

      if (SlotDesignation == NULL) {
        SlotDesignation = SmbiosGetString (mOriginal, mOriginal.Standard.Type9->SlotDesignation);
      }

      p->SlotDesignation = SmbiosSetString (
                             &Strings,
                             SlotDesignation,
                             &StringsIndex
                             );

      SmBiosFinaliseStruct (Table, Handle, ((UINTN)Strings - (UINTN)p));

      DEBUG_CODE_BEGIN();

      This.Raw = (UINT8 *)p;

      SmbiosDebugSystemSlots (This);

      DEBUG_CODE_END();
    }
  }
}

// SmBiosInitType16
/** Type 16

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] MemoryArrayHandle      Pointer to location to store the discovered this structures handle value.
  @param[in] MemoryErrorInfoHandle  The discovered memory error information structure handle value.

  @retval
**/
VOID
SmBiosInitType16 (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  SMBIOS_HANDLE   *MemoryArrayHandle,
  IN  SMBIOS_HANDLE   MemoryErrorInfoHandle
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE16  *p = (SMBIOS_TABLE_TYPE16 *)*(Table);

    CopyMem (p, mOriginal.Standard.Type16, mOriginal.Standard.Type16->Hdr.Length);

    p->Hdr.Handle = *(Handle);

    p->MemoryErrorInformationHandle = MemoryErrorInfoHandle;

    *MemoryArrayHandle = p->Hdr.Handle;

    SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugPhysicalMemoryArray (This);

    DEBUG_CODE_END();
  }
}

// PatchMemoryDevice
/** Type 17

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchMemoryDevice (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  SMBIOS_HANDLE   MemoryArrayHandle,
  IN  SMBIOS_HANDLE   MemoryErrorHandle,
  IN  UINT8           SlotNo,
  IN  UINT8           MacModel
  )
{
  SMBIOS_TABLE_TYPE17  *p = (SMBIOS_TABLE_TYPE17 *)*(Table);

  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_MEMORY_DEVICE, SlotNo);

  if (mOriginal.Raw != NULL) {

    CHAR8   *Strings      = NULL;
    UINT8   StringsIndex  = 0;

    CopyMem (p, mOriginal.Standard.Type17, mOriginal.Standard.Type17->Hdr.Length);

    Strings       = PTR_OFFSET(p, p->Hdr.Length, CHAR8 *);
    p->Hdr.Handle = *(Handle);

    // Use the SODIMM form factor on everything but macpro family
    if (MacModel == MacPro) {
      p->FormFactor = MemoryFormFactorDimm;
    } else {
      p->FormFactor = MemoryFormFactorSodimm;
    }

    p->DeviceLocator = SmbiosSetString (
                         &Strings,
                         SmbiosGetString (mOriginal, mOriginal.Standard.Type17->DeviceLocator),
                         &StringsIndex
                         );

    p->BankLocator = SmbiosSetString (
                       &Strings,
                       SmbiosGetString (mOriginal, mOriginal.Standard.Type17->BankLocator),
                       &StringsIndex
                       );

    if (p->TotalWidth != 0) {

      CHAR8   *Vendor       = SmbiosGetString (mOriginal, mOriginal.Standard.Type17->Manufacturer);
      CHAR8   *SerialNumber = SmbiosGetString (mOriginal, mOriginal.Standard.Type17->SerialNumber);
      CHAR8   *PartNumber   = SmbiosGetString (mOriginal, mOriginal.Standard.Type17->PartNumber);
      CHAR8   *AssetTag     = SmbiosGetString (mOriginal, mOriginal.Standard.Type17->AssetTag);

      p->Manufacturer       = SmbiosSetStringHex (&Strings, Vendor, &StringsIndex);
      p->SerialNumber       = SmbiosSetStringHex (&Strings, SerialNumber, &StringsIndex);
      p->PartNumber         = SmbiosSetStringHex (&Strings, PartNumber, &StringsIndex);
      p->AssetTag           = SmbiosSetStringHex (&Strings, AssetTag, &StringsIndex);

    } else {

      // Empty slots should have no physical memory array handle
      MemoryArrayHandle     = 0xFFFF;

      p->Manufacturer       = 0;
      p->SerialNumber       = 0;
      p->PartNumber         = 0;
      p->AssetTag           = 0;

    }

    p->MemoryArrayHandle            = MemoryArrayHandle;
    p->MemoryErrorInformationHandle = MemoryErrorHandle;

    SmBiosFinaliseStruct (Table, Handle, ((UINTN)Strings - (UINTN)p));

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugMemoryDevice (This);

    DEBUG_CODE_END();
  }
}

// SmBiosInitType19
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] MemoryArrayHandle      The discovered handle of the memory arrary that this structure is part of.

  @retval
**/
VOID
SmBiosInitType19 (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  SMBIOS_HANDLE   MemoryArrayHandle
  )
{
  UINT8  NumberMappedArrayDevices;
  UINT8  MappedDeviceNo;

  NumberMappedArrayDevices = SmbiosGetTableCount (mSmbios, SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS);

  for (MappedDeviceNo = 1; MappedDeviceNo <= NumberMappedArrayDevices; MappedDeviceNo++) {

    mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, MappedDeviceNo);

    if (mOriginal.Raw != NULL) {

      SMBIOS_TABLE_TYPE19  *p = (SMBIOS_TABLE_TYPE19 *)*(Table);

      CopyMem (p, mOriginal.Standard.Type19, mOriginal.Standard.Type19->Hdr.Length);

      p->Hdr.Handle        = *(Handle);
      p->MemoryArrayHandle = MemoryArrayHandle;

      SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);

      DEBUG_CODE_BEGIN();

      This.Raw = (UINT8 *)p;

      SmbiosDebugType19Device (This);

      DEBUG_CODE_END();
    }
  }
}

// SmBiosInitType20
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
SmBiosInitType20 (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  SMBIOS_HANDLE   MemoryArrayHandle,
  IN  SMBIOS_HANDLE   MemoryDeviceHandle,
  IN  INT8            SlotNo
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, SlotNo);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE20  *p = (SMBIOS_TABLE_TYPE20 *)*(Table);

    CopyMem (p, mOriginal.Standard.Type20, mOriginal.Standard.Type20->Hdr.Length);

    p->Hdr.Handle = *(Handle);

    p->MemoryDeviceHandle             = MemoryDeviceHandle;
    p->MemoryArrayMappedAddressHandle = MemoryArrayHandle;

    SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugType20Device (This);

    DEBUG_CODE_END();
  }
}

// PatchPortableBatteryDevice
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchPortableBatteryDevice (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_PORTABLE_BATTERY, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE22  *p = (SMBIOS_TABLE_TYPE22 *)*(Table);
    UINTN       TableLength = SmbiosGetTableLength (mOriginal);

    CopyMem (p, mOriginal.Standard.Type22, TableLength);

    p->Hdr.Handle = *(Handle);

    SmBiosFinaliseStruct (Table, Handle, --TableLength);

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugPortableBatteryDevice (This);

    DEBUG_CODE_END();
  }
}

// PatchBootInformation
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
PatchBootInformation (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  mOriginal = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, 1);

  if (mOriginal.Raw != NULL) {

    SMBIOS_TABLE_TYPE32  *p = (SMBIOS_TABLE_TYPE32 *)*(Table);

    CopyMem (p, mOriginal.Standard.Type32, mOriginal.Standard.Type32->Hdr.Length);

    p->Hdr.Handle = *(Handle);

    SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);

    DEBUG_CODE_BEGIN();

    This.Raw = (UINT8 *)p;

    SmbiosDebugBootInformation (This);

    DEBUG_CODE_END();
  }
}

// CreateAppleFirmwareVolume
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.

  @retval
**/
VOID
CreateAppleFirmwareVolume (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  APPLE_SMBIOS_TABLE_TYPE128  *p = (APPLE_SMBIOS_TABLE_TYPE128 *)*(Table);
  EFI_STATUS  Status;

  UINT32    *Data;
  UINTN     Length;

  p->Hdr.Type   = APPLE_SMBIOS_TYPE_FIRMWARE_INFORMATION;
  p->Hdr.Length = sizeof (APPLE_SMBIOS_TABLE_TYPE128);
  p->Hdr.Handle = *(Handle);

  Length = sizeof (UINT32);
  Data   = &p->FirmwareFeatures;

  Status = OcGetVariable (
             APPLE_FIRMWARE_FEATURES_VARIABLE,
             &gOpenCoreOverridesGuid,
             (VOID **)&Data,
             &Length
             );

  if ((EFI_ERROR (Status)) && (Length != sizeof(UINT32))) {
    p->FirmwareFeatures = 0x80001417;
  }

  Length = sizeof(UINT32);
  Data   = &p->FirmwareFeaturesMask;

  Status = OcGetVariable (
             APPLE_FIRMWARE_FEATURES_MASK_VARIABLE,
             &gOpenCoreOverridesGuid,
             (VOID **)&Data,
             &Length
             );

  if ((EFI_ERROR (Status)) || (Length != sizeof(UINT32))) {
    p->FirmwareFeaturesMask = 0xC003FF37;
  }

  SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);

  DEBUG_CODE_BEGIN();

  This.Raw = (UINT8 *)p;

  SmbiosDebugAppleFirmwareVolume (This);

  DEBUG_CODE_END();
}

// SmBiosInitType130
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.

  @retval
**/
VOID
SmBiosInitType130 (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  CPU_INFO        *CpuInfo
  )
{
  APPLE_SMBIOS_TABLE_TYPE130  *p = (APPLE_SMBIOS_TABLE_TYPE130 *)*(Table);

  p->Hdr.Type   = APPLE_SMBIOS_TYPE_MEMORY_SPD_DATA;
  p->Hdr.Length = sizeof(APPLE_SMBIOS_TABLE_TYPE130);
  p->Hdr.Handle = *(Handle);

  SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);
}

// CreateAppleProcessorType
/**

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.
  @param[in] CpuInfo                Pointer to a valid pico cpu info structure.

  @retval
**/
VOID
CreateAppleProcessorType (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle,
  IN  CPU_INFO        *CpuInfo
  )
{
  APPLE_SMBIOS_TABLE_TYPE131  *p = (APPLE_SMBIOS_TABLE_TYPE131 *)*(Table);
  EFI_STATUS  Status;
  UINT16      *CpuType;
  UINTN       Length;

  p->Hdr.Type   = APPLE_SMBIOS_TYPE_PROCESSOR_TYPE;
  p->Hdr.Length = sizeof (APPLE_SMBIOS_TABLE_TYPE131);
  p->Hdr.Handle = *(Handle);

  CpuType = &p->ProcessorType.Type;
  Length  = sizeof (p->ProcessorType);

  Status  = OcGetVariable (
              OPEN_CORE_CPU_TYPE_VARIABLE,
              &gOpenCoreOverridesGuid,
              (VOID **)&CpuType,
              &Length
              );

  if ((EFI_ERROR (Status)) || (Length != sizeof (p->ProcessorType.Type))) {

    APPLE_SMBIOS_STRUCTURE_POINTER  Original;

    Original = SmbiosGetTableFromType (mSmbios, SMBIOS_TYPE_PROCESSOR_INFORMATION, 1);

    switch (CpuInfo->Model) {

      case CPU_MODEL_WESTMERE:
      case CPU_MODEL_WESTMERE_EX:

        if (Original.Standard.Type4->ProcessorFamily == ProcessorFamilyIntelXeon) {
          p->ProcessorType.Type = AppleProcessorTypeXeon;
        } else {
          p->ProcessorType.Type = AppleProcessorTypeCorei7;
        }

        break;

      case CPU_MODEL_SANDYBRIDGE:

        if (Original.Standard.Type4->ProcessorFamily == ProcessorFamilyIntelCoreI7) {
          p->ProcessorType.Type = AppleProcessorTypeGen2Corei7;
        } else {
          p->ProcessorType.Type = AppleProcessorTypeGen2Corei5;
        }

        break;

      case CPU_MODEL_IVYBRIDGE:

        if (Original.Standard.Type4->ProcessorFamily == ProcessorFamilyIntelCoreI7) {
          p->ProcessorType.Type = AppleProcessorTypeGen3Corei7m;
        } else {
          p->ProcessorType.Type = AppleProcessorTypeGen3Corei5m;
        }

        break;

      case CPU_MODEL_IVYBRIDGE_E5:

        p->ProcessorType.Type =  AppleProcessorTypeGen3Xeon;

        break;

      case CPU_MODEL_HASWELL:
      case CPU_MODEL_HASWELL_MB:
      case CPU_MODEL_HASWELL_ULT:
      case CPU_MODEL_HASWELL_ULX:
      case CPU_MODEL_SKYLAKE:
      case CPU_MODEL_KABYLAKE:
      case CPU_MODEL_KABYLAKE_U:

        if (Original.Standard.Type4->ProcessorFamily == ProcessorFamilyIntelCoreI7) {
          p->ProcessorType.Type = AppleProcessorTypeGen3Corei7;
        } else {
          p->ProcessorType.Type = AppleProcessorTypeGen3Corei5;
        }

        break;

      default:

          p->ProcessorType.Type = AppleProcessorTypeGen3Corei5;

        break;
    }
  }

  SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);

  DEBUG_CODE_BEGIN();

  This.Raw = (UINT8 *)p;

  SmbiosDebugAppleProcessorType (This);

  DEBUG_CODE_END();
}

// CreateSmBiosEndOfTable
/** Type 127

  @param[in] Table                  Pointer to location containing the current address within the buffer.
  @param[in] Handle                 Pointer to tocation containing the current handle value.

  @retval
**/
VOID
CreateSmBiosEndOfTable (
  IN  UINT8           **Table,
  IN  SMBIOS_HANDLE   *Handle
  )
{
  SMBIOS_TABLE_TYPE127 *p = (SMBIOS_TABLE_TYPE127 *)*(Table);

  p->Hdr.Type   = SMBIOS_TYPE_END_OF_TABLE;
  p->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE127);
  p->Hdr.Handle = *(Handle);

  SmBiosFinaliseStruct (Table, Handle, p->Hdr.Length + 1);
}

// SmbiosGetModelOverride
/**

  @retval
**/
UINT8
SmbiosGetModelOverride (
  )
{
  EFI_STATUS  Status;
  CHAR8       *ProductName;
  UINTN       Length;
  UINT8       MacModel;

  MacModel    = 0;

  ProductName = NULL;
  Length      = 0;
  Status      = OcGetVariable (
                  OPEN_CORE_PRODUCT_NAME_VARIABLE,
                  &gOpenCoreOverridesGuid,
                  (VOID **)&ProductName,
                  &Length
                  );

  if (!EFI_ERROR (Status) && Length > 0) {
    if (CompareMem (ProductName, "MacPro", 6) == 0) {
      MacModel = MacPro;
    }
    FreePool ((VOID *)ProductName);
  }
  return MacModel;
}

// CreateSmBios
/**

  @retval EFI_SUCCESS               The smbios tables were generated successfully
**/
EFI_STATUS
CreateSmBios (
  VOID
  )
{
  EFI_STATUS    Status;

  DEBUG_FUNCTION_ENTRY (DEBUG_VERBOSE);

  mSmbios = NULL;
  Status  = EfiGetSystemConfigurationTable (
              &gEfiSmbiosTableGuid,
              (VOID **)&mSmbios);

  if (mSmbios != NULL) {

    UINT8   *Buffer;
    UINT32  BufferLen;

    // Pad the table length to a page and calculate byte size.
    BufferLen = EFI_PAGES_TO_SIZE(EFI_SIZE_TO_PAGES(mSmbios->TableLength));

    DEBUG ((
      DEBUG_INFO,
      "Found DMI Anchor 0x%08X v%d.%d Table Address 0x%08X Length 0x%04X 0x%04X\n",
      mSmbios,
      mSmbios->MajorVersion,
      mSmbios->MinorVersion,
      mSmbios->TableAddress,
      mSmbios->TableLength,
      BufferLen
      ));

    Buffer = AllocateZeroPool (BufferLen);

    Status = EFI_OUT_OF_RESOURCES;

    if (Buffer != NULL) {

      CPU_INFO  CpuInfo;
      UINT8     *Table;

      if (!EFI_ERROR (Status)) {
        Status = OcCpuScanProcessor (&CpuInfo);

        if (!EFI_ERROR (Status)) {

          SMBIOS_HANDLE   Handle;
          SMBIOS_HANDLE   MemoryArrayHandle;
          SMBIOS_HANDLE   MemoryErrorInfoHandle;

          SMBIOS_HANDLE   L1CacheHandle;
          SMBIOS_HANDLE   L2CacheHandle;
          SMBIOS_HANDLE   L3CacheHandle;

          UINT8   NumberMemoryDevices;
          UINT8   MemoryDeviceNo;
          UINT8   RamModuleNo;

          UINT8   MacModel;

          Handle = 0;
          Table  = Buffer;

          mTableLength      = 0;
          mMaxStructureSize = 0;

          L1CacheHandle     = 0xFFFF;
          L2CacheHandle     = 0xFFFF;
          L3CacheHandle     = 0xFFFF;

          MacModel          = SmbiosGetModelOverride ();

          //

          PatchBiosInformation (&Table, &Handle);
          PatchSystemInformation (&Table, &Handle);
          PatchBaseboardInformation (&Table, &Handle, MacModel);
          PatchSystemEnclosure (&Table, &Handle, MacModel);

          // Combine the following
          PatchCacheInformation (&Table, &Handle, &L1CacheHandle, &L2CacheHandle, &L3CacheHandle);
          PatchProcessorInformation (&Table, &Handle, L1CacheHandle, L2CacheHandle, L3CacheHandle, &CpuInfo);

          PatchSystemPorts (&Table, &Handle);
          PatchSystemSlots (&Table, &Handle);

          MemoryErrorInfoHandle = 0xFFFF;

          SmBiosInitType16 (&Table, &Handle, &MemoryArrayHandle, MemoryErrorInfoHandle);
          SmBiosInitType19 (&Table, &Handle, MemoryArrayHandle);

          NumberMemoryDevices = SmbiosGetTableCount (mSmbios, SMBIOS_TYPE_MEMORY_DEVICE);

          for (MemoryDeviceNo = 1, RamModuleNo = 1; MemoryDeviceNo <= NumberMemoryDevices; MemoryDeviceNo++) {

            APPLE_SMBIOS_STRUCTURE_POINTER  MemoryDeviceInfo;
            APPLE_SMBIOS_STRUCTURE_POINTER  MemoryDeviceAddress;

            MemoryDeviceInfo = SmbiosGetTableFromType (mSmbios, 17, MemoryDeviceNo);

            if (MemoryDeviceInfo.Raw != NULL) {

              MemoryDeviceAddress = SmbiosGetTableFromType (mSmbios, 20, RamModuleNo);

              if (MemoryDeviceAddress.Raw != NULL) {

                // For each occupied memory device we must generate type 20
                if (MemoryDeviceAddress.Standard.Type20->MemoryDeviceHandle ==
                    MemoryDeviceInfo.Standard.Type17->Hdr.Handle)
                {
                  SmBiosInitType20 (
                    &Table,
                    &Handle,
                    MemoryArrayHandle,
                    Handle + 1,
                    RamModuleNo++
                    );

                  // TODO: Type 130 - Apple Memory SPD Info
                }
              }

              // For each memory device we must generate type 17
              PatchMemoryDevice (
                &Table,
                &Handle,
                MemoryArrayHandle,
                MemoryErrorInfoHandle,
                MemoryDeviceNo,
                MacModel
              );

            }
          }

          PatchPortableBatteryDevice (&Table, &Handle);

          PatchBootInformation (&Table, &Handle);

          CreateAppleProcessorType (&Table, &Handle, &CpuInfo);

          CreateAppleFirmwareVolume (&Table, &Handle);

          CreateSmBiosEndOfTable (&Table, &Handle);

          if (((UINTN)mSmbios) < BASE_1MB) {
            // Enable write access to DMI anchor
            Status = LegacyRegionUnlock (
                       (UINT32)(UINTN)(mSmbios) & 0xFFFF8000ULL,
                       EFI_PAGE_SIZE
                       );
          }

          if (((UINTN)mSmbios->TableAddress) < BASE_1MB) {
            // Enable write access to DMI table
            Status = LegacyRegionUnlock (
                       (UINT32)(UINTN)(mSmbios->TableAddress) & 0xFFFF8000ULL,
                       EFI_PAGES_TO_SIZE(EFI_SIZE_TO_PAGES(mSmbios->TableLength))
                       );
          }

          CopyMem ((VOID *)(UINTN)mSmbios->TableAddress,
                   Buffer,
                   mTableLength);

          mSmbios->TableLength              = mTableLength;
          mSmbios->MaxStructureSize         = mMaxStructureSize;
          mSmbios->NumberOfSmbiosStructures = mNumberOfSmbiosStructures;

          // Fix checksums
          mSmbios->EntryPointStructureChecksum = 0;
          mSmbios->EntryPointStructureChecksum = CalculateCheckSum8 ((UINT8 *)mSmbios, 0x10);
          mSmbios->IntermediateChecksum = 0;
          mSmbios->IntermediateChecksum = CalculateCheckSum8 ((UINT8 *)mSmbios + 0x10, mSmbios->EntryPointLength - 0x10);

          DEBUG ((
            DEBUG_INFO,
            "Patched 0x%08X v%d.%d Table Address 0x%08X Length 0x%04X 0x%X 0x%X\n",
            mSmbios,
            mSmbios->MajorVersion,
            mSmbios->MinorVersion,
            mSmbios->TableAddress,
            mSmbios->TableLength,
            mSmbios->EntryPointStructureChecksum,
            mSmbios->IntermediateChecksum
            ));

          if (((UINTN)mSmbios->TableAddress) < BASE_1MB) {
            // Lock write access To DMI table
            Status = LegacyRegionLock (
                       (UINT32)(UINTN)(mSmbios->TableAddress) & 0xFFFF8000ULL,
                       EFI_PAGES_TO_SIZE(EFI_SIZE_TO_PAGES(mSmbios->TableLength))
                       );
          }

          if (((UINTN)mSmbios) < BASE_1MB) {
            // Lock write access To DMI anchor
            Status = LegacyRegionLock (
                       (UINT32)(UINTN)(mSmbios) & 0xFFFF8000ULL,
                       EFI_PAGE_SIZE
                       );
          }

          FreePool (Buffer);

        }
      }
    }
  }

  DEBUG_FUNCTION_RETURN (DEBUG_VERBOSE);

  return Status;
}

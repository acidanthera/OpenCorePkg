/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/OcMainLib.h>

#include <Protocol/DataHub.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMacInfoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcDataHubLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <IndustryStandard/AppleSmBios.h>
#include <IndustryStandard/AppleFeatures.h>

#include <Guid/AppleVariable.h>

STATIC CHAR8    mCurrentSmbiosProductName[OC_OEM_NAME_MAX];

STATIC
VOID
OcPlatformUpdateDataHub (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo,
  IN MAC_INFO_DATA       *MacInfo
  )
{
  EFI_STATUS            Status;
  OC_DATA_HUB_DATA      Data;
  EFI_GUID              Uuid;
  UINT64                StartupPowerEvents;
  UINT64                InitialTSC;
  EFI_DATA_HUB_PROTOCOL *DataHub;

  DataHub = OcDataHubInstallProtocol (FALSE);
  if (DataHub == NULL) {
    DEBUG ((DEBUG_WARN, "OC: Failed to install Data Hub\n"));
    return;
  }

  ZeroMem (&Data, sizeof (Data));

  if (MacInfo == NULL) {
    //
    // Manual mode, read data from DataHub.
    //
    if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.PlatformName)[0] != '\0') {
      Data.PlatformName = OC_BLOB_GET (&Config->PlatformInfo.DataHub.PlatformName);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemProductName)[0] != '\0') {
      Data.SystemProductName = OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemProductName);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemSerialNumber)[0] != '\0') {
      Data.SystemSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemSerialNumber);
    }

    if (!EFI_ERROR (OcAsciiStrToRawGuid (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemUuid), &Uuid))) {
      Data.SystemUUID = &Uuid;
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.BoardProduct)[0] != '\0') {
      Data.BoardProduct = OC_BLOB_GET (&Config->PlatformInfo.DataHub.BoardProduct);
    }

    Data.BoardRevision      = &Config->PlatformInfo.DataHub.BoardRevision[0];
    Data.StartupPowerEvents = &Config->PlatformInfo.DataHub.StartupPowerEvents;
    Data.InitialTSC         = &Config->PlatformInfo.DataHub.InitialTSC;

    if (Config->PlatformInfo.DataHub.FSBFrequency != 0) {
      Data.FSBFrequency     = &Config->PlatformInfo.DataHub.FSBFrequency;
    }

    if (Config->PlatformInfo.DataHub.ARTFrequency != 0) {
      Data.ARTFrequency     = &Config->PlatformInfo.DataHub.ARTFrequency;
    }

    if (Config->PlatformInfo.DataHub.DevicePathsSupported != 0) {
      Data.DevicePathsSupported = &Config->PlatformInfo.DataHub.DevicePathsSupported;
    }

    if (Config->PlatformInfo.DataHub.SmcRevision[0] != 0
      || Config->PlatformInfo.DataHub.SmcRevision[1] != 0
      || Config->PlatformInfo.DataHub.SmcRevision[2] != 0
      || Config->PlatformInfo.DataHub.SmcRevision[3] != 0
      || Config->PlatformInfo.DataHub.SmcRevision[4] != 0
      || Config->PlatformInfo.DataHub.SmcRevision[5] != 0) {
      Data.SmcRevision      = &Config->PlatformInfo.DataHub.SmcRevision[0];
    }

    if (Config->PlatformInfo.DataHub.SmcBranch[0] != 0
      || Config->PlatformInfo.DataHub.SmcBranch[1] != 0
      || Config->PlatformInfo.DataHub.SmcBranch[2] != 0
      || Config->PlatformInfo.DataHub.SmcBranch[3] != 0
      || Config->PlatformInfo.DataHub.SmcBranch[4] != 0
      || Config->PlatformInfo.DataHub.SmcBranch[5] != 0
      || Config->PlatformInfo.DataHub.SmcBranch[6] != 0
      || Config->PlatformInfo.DataHub.SmcBranch[7] != 0) {
      Data.SmcBranch        = &Config->PlatformInfo.DataHub.SmcBranch[0];
    }

    if (Config->PlatformInfo.DataHub.SmcPlatform[0] != 0
      || Config->PlatformInfo.DataHub.SmcPlatform[1] != 0
      || Config->PlatformInfo.DataHub.SmcPlatform[2] != 0
      || Config->PlatformInfo.DataHub.SmcPlatform[3] != 0
      || Config->PlatformInfo.DataHub.SmcPlatform[4] != 0
      || Config->PlatformInfo.DataHub.SmcPlatform[5] != 0
      || Config->PlatformInfo.DataHub.SmcPlatform[6] != 0
      || Config->PlatformInfo.DataHub.SmcPlatform[7] != 0) {
      Data.SmcPlatform      = &Config->PlatformInfo.DataHub.SmcPlatform[0];
    }
  } else {
    //
    // Automatic mode read data from Generic & MacInfo.
    //
    Data.PlatformName = MacInfo->DataHub.PlatformName;
    Data.SystemProductName = MacInfo->DataHub.SystemProductName;

    if (MacInfo->Oem.SystemSerialNumber[0] != '\0') {
      Data.SystemSerialNumber = MacInfo->Oem.SystemSerialNumber;
    }

    if (!IsZeroGuid (&MacInfo->Oem.SystemUuid)) {
      CopyGuid (&Uuid, &MacInfo->Oem.SystemUuid);
      Data.SystemUUID = &Uuid;
    }

    Data.BoardProduct  = MacInfo->DataHub.BoardProduct;
    Data.BoardRevision = &MacInfo->DataHub.BoardRevision[0];
    StartupPowerEvents = 0;
    Data.StartupPowerEvents = &StartupPowerEvents;
    InitialTSC = 0;
    Data.InitialTSC = &InitialTSC;
    Data.DevicePathsSupported = &MacInfo->DataHub.DevicePathsSupported[0];

    Data.SmcRevision      = &MacInfo->DataHub.SmcRevision[0];
    Data.SmcBranch        = &MacInfo->DataHub.SmcBranch[0];
    Data.SmcPlatform      = &MacInfo->DataHub.SmcPlatform[0];
  }

  Status = UpdateDataHub (DataHub , &Data, CpuInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Failed to update Data Hub - %r\n", Status));
  }
}

STATIC
VOID
OcPlatformUpdateSmbios (
  IN     OC_GLOBAL_CONFIG       *Config,
  IN     OC_CPU_INFO            *CpuInfo,
  IN     MAC_INFO_DATA          *MacInfo,
  IN OUT OC_SMBIOS_TABLE        *SmbiosTable,
  IN     OC_SMBIOS_UPDATE_MODE  UpdateMode
  )
{
  EFI_STATUS       Status;
  OC_SMBIOS_DATA   Data;
  UINT32           PlatformFeature;
  EFI_GUID         Uuid;
  UINT8            SmcVersion[APPLE_SMBIOS_SMC_VERSION_SIZE];
  CONST CHAR8      *SystemMemoryStatus;
  UINT16           Index;

  OC_PLATFORM_MEMORY_DEVICE_ENTRY *MemoryEntry;

  ZeroMem (&Data, sizeof (Data));

  //
  // Forcibly use MemoryFormFactor on non-Automatic mode.
  //
  Data.ForceMemoryFormFactor = !Config->PlatformInfo.Automatic;

  if (MacInfo == NULL) {
    //
    // Manual mode, read data from SMBIOS.
    //
    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BIOSVendor)[0] != '\0') {
      Data.BIOSVendor = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BIOSVendor);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BIOSVersion)[0] != '\0') {
      Data.BIOSVersion = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BIOSVersion);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BIOSReleaseDate)[0] != '\0') {
      Data.BIOSReleaseDate = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BIOSReleaseDate);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemManufacturer)[0] != '\0') {
      Data.SystemManufacturer = OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemManufacturer);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemProductName)[0] != '\0') {
      Data.SystemProductName = OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemProductName);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemVersion)[0] != '\0') {
      Data.SystemVersion = OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemVersion);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemSerialNumber)[0] != '\0') {
      Data.SystemSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemSerialNumber);
    }

    if (Config->PlatformInfo.UseRawUuidEncoding) {
      Status = OcAsciiStrToRawGuid (
        OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemUuid),
        &Uuid
        );
    } else {    
      Status = AsciiStrToGuid (
        OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemUuid),
        &Uuid
        );
    }

    if (!EFI_ERROR (Status)) {
      Data.SystemUUID         = &Uuid;
    }

    if (Config->PlatformInfo.Smbios.BoardType != 0) {
      Data.BoardType = &Config->PlatformInfo.Smbios.BoardType;
    }

    if (Config->PlatformInfo.Smbios.ChassisType != 0) {
      Data.ChassisType = &Config->PlatformInfo.Smbios.ChassisType;
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemSKUNumber)[0] != '\0') {
      Data.SystemSKUNumber = OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemSKUNumber);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemFamily)[0] != '\0') {
      Data.SystemFamily = OC_BLOB_GET (&Config->PlatformInfo.Smbios.SystemFamily);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardManufacturer)[0] != '\0') {
      Data.BoardManufacturer = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardManufacturer);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardProduct)[0] != '\0') {
      Data.BoardProduct = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardProduct);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardVersion)[0] != '\0') {
      Data.BoardVersion = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardVersion);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardSerialNumber)[0] != '\0') {
      Data.BoardSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardSerialNumber);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardAssetTag)[0] != '\0') {
      Data.BoardAssetTag = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardAssetTag);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardLocationInChassis)[0] != '\0') {
      Data.BoardLocationInChassis = OC_BLOB_GET (&Config->PlatformInfo.Smbios.BoardLocationInChassis);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisManufacturer)[0] != '\0') {
      Data.ChassisManufacturer = OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisManufacturer);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisVersion)[0] != '\0') {
      Data.ChassisVersion = OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisVersion);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisSerialNumber)[0] != '\0') {
      Data.ChassisSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisSerialNumber);
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisAssetTag)[0] != '\0') {
      Data.ChassisAssetTag = OC_BLOB_GET (&Config->PlatformInfo.Smbios.ChassisAssetTag);
    }

    Data.FirmwareFeatures     = Config->PlatformInfo.Smbios.FirmwareFeatures;
    Data.FirmwareFeaturesMask = Config->PlatformInfo.Smbios.FirmwareFeaturesMask;

    if (Config->PlatformInfo.Smbios.ProcessorType != 0) {
      Data.ProcessorType = &Config->PlatformInfo.Smbios.ProcessorType;
    }

    if (Config->PlatformInfo.Smbios.PlatformFeature != 0xFFFFFFFFU) {
      Data.PlatformFeature    = &Config->PlatformInfo.Smbios.PlatformFeature;
    }

    if (Config->PlatformInfo.Smbios.SmcVersion[0] != '\0') {
      Data.SmcVersion = &Config->PlatformInfo.Smbios.SmcVersion[0];
    }
  } else {
    //
    // Automatic mode read data from Generic & MacInfo.
    //
    if (Config->PlatformInfo.Generic.SpoofVendor) {
      Data.BIOSVendor          = OC_SMBIOS_VENDOR_NAME;
      Data.SystemManufacturer  = OC_SMBIOS_VENDOR_NAME;
      Data.ChassisManufacturer = OC_SMBIOS_VENDOR_NAME;
      Data.BoardManufacturer   = OC_SMBIOS_VENDOR_NAME;
    }

    Data.BIOSVersion = MacInfo->Smbios.BIOSVersion;
    Data.BIOSReleaseDate = MacInfo->Smbios.BIOSReleaseDate;
    Data.SystemProductName = MacInfo->Smbios.SystemProductName;
    Data.SystemVersion = MacInfo->Smbios.SystemVersion;

    if (MacInfo->Oem.SystemSerialNumber[0] != '\0') {
      Data.SystemSerialNumber = MacInfo->Oem.SystemSerialNumber;
    }

    if (!IsZeroGuid (&MacInfo->Oem.SystemUuid)) {
      CopyGuid (&Uuid, &MacInfo->Oem.SystemUuid);
      if (!Config->PlatformInfo.UseRawUuidEncoding) {
        //
        // Change from RAW to LE. 
        //
        Uuid.Data1 = SwapBytes32 (Uuid.Data1);
        Uuid.Data2 = SwapBytes16 (Uuid.Data2);
        Uuid.Data3 = SwapBytes16 (Uuid.Data3);
      }
      Data.SystemUUID = &Uuid;
    }

    Data.BoardType              = MacInfo->Smbios.BoardType;
    Data.ChassisType            = MacInfo->Smbios.ChassisType;
    Data.SystemSKUNumber        = MacInfo->Smbios.SystemSKUNumber;
    Data.SystemFamily           = MacInfo->Smbios.SystemFamily;
    Data.BoardProduct           = MacInfo->Smbios.BoardProduct;
    Data.BoardVersion           = MacInfo->Smbios.BoardVersion;

    if (MacInfo->Oem.Mlb[0] != '\0') {
      Data.BoardSerialNumber = MacInfo->Oem.Mlb;
    }

    Data.BoardAssetTag          = MacInfo->Smbios.BoardAssetTag;
    Data.BoardLocationInChassis = MacInfo->Smbios.BoardLocationInChassis;
    Data.ChassisVersion         = MacInfo->Smbios.ChassisVersion;

    if (MacInfo->Oem.SystemSerialNumber[0] != '\0') {
      Data.ChassisSerialNumber = MacInfo->Oem.SystemSerialNumber;
    }

    Data.ChassisAssetTag      = MacInfo->Smbios.ChassisAssetTag;
    Data.MemoryFormFactor     = MacInfo->Smbios.MemoryFormFactor;
    Data.FirmwareFeatures     = MacInfo->Smbios.FirmwareFeatures;
    Data.FirmwareFeaturesMask = MacInfo->Smbios.FirmwareFeaturesMask;

    //
    // Adopt to arbitrary hardware specifics. See description in NVRAM handling code.
    //
    if (Config->PlatformInfo.Generic.AdviseFeatures) {
      Data.FirmwareFeatures     |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE
        | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT
        | FW_FEATURE_SUPPORTS_APFS;
      Data.FirmwareFeaturesMask |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE
        | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT
        | FW_FEATURE_SUPPORTS_APFS;
    }

    //
    // Permit overriding the CPU type for those that want it.
    //
    if (Config->PlatformInfo.Generic.ProcessorType != 0) {
      Data.ProcessorType = &Config->PlatformInfo.Generic.ProcessorType;
    }

    Data.PlatformFeature = MacInfo->Smbios.PlatformFeature;
    if (Data.PlatformFeature != NULL) {
      PlatformFeature = *Data.PlatformFeature;
    } else {
      PlatformFeature = 0;
    }

    if (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemMemoryStatus)[0] != '\0') {
      SystemMemoryStatus = OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemMemoryStatus);

      if (AsciiStrCmp (SystemMemoryStatus, "Upgradable") == 0) {
        PlatformFeature &= ~PT_FEATURE_HAS_SOLDERED_SYSTEM_MEMORY;
        Data.PlatformFeature = &PlatformFeature;
      } else if (AsciiStrCmp (SystemMemoryStatus, "Soldered") == 0) {
        PlatformFeature |= PT_FEATURE_HAS_SOLDERED_SYSTEM_MEMORY;
        Data.PlatformFeature = &PlatformFeature;
      } else if (AsciiStrCmp (SystemMemoryStatus, "Auto") != 0) {
        DEBUG ((DEBUG_WARN, "OC: Invalid SMBIOS system memory status %a\n", SystemMemoryStatus));
      }
    }
    //
    // Override default BIOSVersion.
    //
    if (Config->PlatformInfo.Generic.MaxBIOSVersion) {
      Data.BIOSVersion = "9999.999.999.999.999";
    }

    if (MacInfo->DataHub.SmcRevision != NULL) {
      OcSmbiosGetSmcVersion (MacInfo->DataHub.SmcRevision, SmcVersion);
      Data.SmcVersion = SmcVersion;
    }
  }

  //
  // Inject custom memory info.
  //
  if (Config->PlatformInfo.CustomMemory) {
    Data.HasCustomMemory = TRUE;

    if (Config->PlatformInfo.Memory.Devices.Count <= MAX_UINT16) {
      Data.MemoryDevicesCount   = (UINT16) Config->PlatformInfo.Memory.Devices.Count;
    } else {
      Data.MemoryDevicesCount   = MAX_UINT16;
    }
    
    Data.MemoryErrorCorrection  = &Config->PlatformInfo.Memory.ErrorCorrection;
    Data.MemoryMaxCapacity      = &Config->PlatformInfo.Memory.MaxCapacity;
    Data.MemoryDataWidth        = &Config->PlatformInfo.Memory.DataWidth;
    Data.MemoryFormFactor       = &Config->PlatformInfo.Memory.FormFactor;
    Data.MemoryTotalWidth       = &Config->PlatformInfo.Memory.TotalWidth;
    Data.MemoryType             = &Config->PlatformInfo.Memory.Type;
    Data.MemoryTypeDetail       = &Config->PlatformInfo.Memory.TypeDetail;

    if (Data.MemoryDevicesCount) {
      Data.MemoryDevices = AllocateZeroPool (sizeof (OC_SMBIOS_MEMORY_DEVICE_DATA) * Data.MemoryDevicesCount);
      if (Data.MemoryDevices == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to allocate custom memory devices\n"));
        Data.MemoryDevicesCount = 0;
      }

      for (Index = 0; Index < Data.MemoryDevicesCount; ++Index) {
        MemoryEntry = Config->PlatformInfo.Memory.Devices.Values[Index];

        Data.MemoryDevices[Index].AssetTag      = OC_BLOB_GET (&MemoryEntry->AssetTag);
        Data.MemoryDevices[Index].BankLocator   = OC_BLOB_GET (&MemoryEntry->BankLocator);
        Data.MemoryDevices[Index].DeviceLocator = OC_BLOB_GET (&MemoryEntry->DeviceLocator);
        Data.MemoryDevices[Index].Manufacturer  = OC_BLOB_GET (&MemoryEntry->Manufacturer);
        Data.MemoryDevices[Index].PartNumber    = OC_BLOB_GET (&MemoryEntry->PartNumber);
        Data.MemoryDevices[Index].SerialNumber  = OC_BLOB_GET (&MemoryEntry->SerialNumber);
        Data.MemoryDevices[Index].Size          = &MemoryEntry->Size;
        Data.MemoryDevices[Index].Speed         = &MemoryEntry->Speed;
      }
    }
  }

  if (Data.SystemProductName != NULL) {
    DEBUG ((DEBUG_INFO, "OC: New SMBIOS: %a model %a\n", Data.SystemManufacturer, Data.SystemProductName));
    Status = AsciiStrCpyS (mCurrentSmbiosProductName, sizeof (mCurrentSmbiosProductName), Data.SystemProductName);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OC: Failed to copy new SMBIOS product name %a\n", Data.SystemProductName));
    }
  }

  Status = OcSmbiosCreate (SmbiosTable, &Data, UpdateMode, CpuInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Failed to update SMBIOS - %r\n", Status));
  }

  if (Data.MemoryDevices != NULL) {
    FreePool (Data.MemoryDevices);
  }
}

STATIC
VOID
OcPlatformUpdateNvram (
  IN OC_GLOBAL_CONFIG    *Config,
  IN MAC_INFO_DATA       *MacInfo
  )
{
  EFI_STATUS   Status;
  CONST CHAR8  *Bid;
  UINTN        BidSize;
  CONST CHAR8  *Mlb;
  UINTN        MlbSize;
  CONST CHAR8  *Ssn;
  UINTN        SsnSize;
  CONST  UINT8 *Rom;
  UINTN        RomSize;
  EFI_GUID     Uuid;
  UINT64       ExFeatures;
  UINT64       ExFeaturesMask;
  UINT32       Features;
  UINT32       FeaturesMask;

  if (MacInfo == NULL) {
    Bid            = OC_BLOB_GET (&Config->PlatformInfo.Nvram.Bid);
    BidSize        = Config->PlatformInfo.Nvram.Bid.Size - 1;
    Mlb            = OC_BLOB_GET (&Config->PlatformInfo.Nvram.Mlb);
    MlbSize        = Config->PlatformInfo.Nvram.Mlb.Size - 1;
    Ssn            = OC_BLOB_GET (&Config->PlatformInfo.Nvram.SystemSerialNumber);
    SsnSize        = Config->PlatformInfo.Nvram.SystemSerialNumber.Size - 1;
    Rom            = &Config->PlatformInfo.Nvram.Rom[0];
    RomSize        = sizeof (Config->PlatformInfo.Nvram.Rom);
    Status = OcAsciiStrToRawGuid (OC_BLOB_GET (&Config->PlatformInfo.Nvram.SystemUuid), &Uuid);
    if (EFI_ERROR (Status)) {
      ZeroMem (&Uuid, sizeof (Uuid));
    }
    ExFeatures     = Config->PlatformInfo.Nvram.FirmwareFeatures;
    ExFeaturesMask = Config->PlatformInfo.Nvram.FirmwareFeaturesMask;
  } else {
    Bid            = MacInfo->Smbios.BoardProduct;
    BidSize        = AsciiStrLen (Bid);
    Mlb            = MacInfo->Oem.Mlb;
    MlbSize        = AsciiStrLen (Mlb);
    Ssn            = MacInfo->Oem.SystemSerialNumber;
    SsnSize        = AsciiStrLen (Ssn);
    Rom            = &MacInfo->Oem.Rom[0];
    RomSize        = OC_OEM_ROM_MAX;
    CopyGuid (&Uuid, &MacInfo->Oem.SystemUuid);
    ExFeatures     = MacInfo->Smbios.FirmwareFeatures;
    ExFeaturesMask = MacInfo->Smbios.FirmwareFeaturesMask;

    //
    // Adopt to arbitrary hardware specifics. This bit allows the use
    // of legacy Windows installation in boot selector preference pane.
    // We need it because Windows systems with EFI partition not being 1st
    // are recognised as legacy. See:
    // https://github.com/acidanthera/bugtracker/issues/327
    // https://sourceforge.net/p/cloverefiboot/tickets/435
    //
    if (Config->PlatformInfo.Generic.AdviseFeatures) {
      ExFeatures     |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE
        | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT
        | FW_FEATURE_SUPPORTS_APFS;
      ExFeaturesMask |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE
        | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT
        | FW_FEATURE_SUPPORTS_APFS;
    }
  }

  Features       = (UINT32) ExFeatures;
  FeaturesMask   = (UINT32) ExFeaturesMask;

  if (Bid[0] != '\0') {
    Status = gRT->SetVariable (
      L"HW_BID",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      BidSize,
      (CHAR8 *) Bid
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting HW_BID %a - %r\n",
      Bid,
      Status
      ));
  }

  if (Rom[0] != 0 || Rom[1] != 0 || Rom[2] != 0 || Rom[3] != 0 || Rom[4] != 0 || Rom[5] != 0) {
    Status = gRT->SetVariable (
      L"HW_ROM",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      RomSize,
      (UINT8 *) Rom
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting HW_ROM %02x:%02x:%02x:%02x:%02x:%02x - %r\n",
      Rom[0], Rom[1], Rom[2], Rom[3], Rom[4], Rom[5],
      Status
      ));

    Status = gRT->SetVariable (
      L"ROM",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      RomSize,
      (UINT8 *) Rom
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting ROM %02x:%02x:%02x:%02x:%02x:%02x - %r\n",
      Rom[0], Rom[1], Rom[2], Rom[3], Rom[4], Rom[5],
      Status
      ));
  }

  if (Mlb[0] != '\0') {
    Status = gRT->SetVariable (
      L"HW_MLB",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      MlbSize,
      (CHAR8 *) Mlb
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting HW_MLB %a - %r\n",
      Mlb,
      Status
      ));

    Status = gRT->SetVariable (
      L"MLB",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      MlbSize,
      (CHAR8 *) Mlb
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting MLB %a - %r\n",
      Mlb,
      Status
      ));
  }

  if (Ssn[0] != '\0') {
    Status = gRT->SetVariable (
      L"HW_SSN",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      SsnSize,
      (CHAR8 *) Ssn
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting HW_SSN %a - %r\n",
      Ssn,
      Status
      ));

    Status = gRT->SetVariable (
      L"SSN",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      SsnSize,
      (CHAR8 *) Ssn
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting SSN %a - %r\n",
      Ssn,
      Status
      ));
  }

  //
  // system-id is only visible in BS scope and may be used by EfiBoot
  // in macOS 11.0 to generate x86legacy ApECID from the first 8 bytes.
  //
  if (!IsZeroGuid (&Uuid)) {
    Status = gRT->SetVariable (
      L"system-id",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof (Uuid),
      &Uuid
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting system-id %g - %r\n",
      &Uuid,
      Status
      ));
  }

  if (ExFeatures != 0 || ExFeaturesMask != 0) {
    Status = gRT->SetVariable (
      L"FirmwareFeatures",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      sizeof (Features),
      &Features
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting FirmwareFeatures %08x - %r\n",
      Features,
      Status
      ));

    Status = gRT->SetVariable (
      L"ExtendedFirmwareFeatures",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      sizeof (ExFeatures),
      &ExFeatures
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting ExtendedFirmwareFeatures %016Lx - %r\n",
      ExFeatures,
      Status
      ));

    Status = gRT->SetVariable (
      L"FirmwareFeaturesMask",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      sizeof (FeaturesMask),
      &FeaturesMask
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting FirmwareFeaturesMask %08x - %r\n",
      FeaturesMask,
      Status
      ));

    Status = gRT->SetVariable (
      L"ExtendedFirmwareFeaturesMask",
      &gAppleVendorVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      sizeof (ExFeaturesMask),
      &ExFeaturesMask
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Setting ExtendedFirmwareFeaturesMask %016Lx - %r\n",
      ExFeaturesMask,
      Status
      ));
  }
}

VOID
OcLoadPlatformSupport (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  )
{
  OC_SMBIOS_UPDATE_MODE  SmbiosUpdateMode;
  MAC_INFO_DATA          InfoData;
  MAC_INFO_DATA          *UsedMacInfo;
  EFI_STATUS             Status;
  OC_SMBIOS_TABLE        SmbiosTable;
  BOOLEAN                ExposeOem;
  BOOLEAN                UseOemSerial;
  BOOLEAN                UseOemUuid;
  BOOLEAN                UseOemMlb;
  BOOLEAN                UseOemRom;

  if (Config->PlatformInfo.Automatic) {
    GetMacInfo (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemProductName), &InfoData);
    UsedMacInfo  = &InfoData;
    UseOemSerial = AsciiStrCmp (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber), "OEM") == 0;
    UseOemUuid   = AsciiStrCmp (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemUuid), "OEM") == 0;
    UseOemMlb    = AsciiStrCmp (OC_BLOB_GET (&Config->PlatformInfo.Generic.Mlb), "OEM") == 0;
    UseOemRom    = AsciiStrCmp ((CHAR8 *) Config->PlatformInfo.Generic.Rom, "OEM") == 0;
  } else {
    UsedMacInfo  = NULL;
    UseOemSerial = FALSE;
    UseOemUuid   = FALSE;
    UseOemMlb    = FALSE;
    UseOemRom    = FALSE;
  }

  ExposeOem = (Config->Misc.Security.ExposeSensitiveData & OCS_EXPOSE_OEM_INFO) != 0;

  Status = OcSmbiosTablePrepare (&SmbiosTable);
  if (!EFI_ERROR (Status)) {
    OcSmbiosExtractOemInfo (
      &SmbiosTable,
      mCurrentSmbiosProductName,
      UseOemSerial ? InfoData.Oem.SystemSerialNumber : NULL,
      UseOemUuid ? &InfoData.Oem.SystemUuid : NULL,
      UseOemMlb ? InfoData.Oem.Mlb : NULL,
      UseOemRom ? InfoData.Oem.Rom : NULL,
      Config->PlatformInfo.UseRawUuidEncoding,
      ExposeOem
      );
  }

  DEBUG ((
    DEBUG_INFO,
    "OC: PlatformInfo auto %d OEM SN %d OEM UUID %d OEM MLB %d OEM ROM %d - %r\n",
    Config->PlatformInfo.Automatic,
    UseOemSerial,
    UseOemUuid,
    UseOemMlb,
    UseOemRom,
    Status
    ));

  if (Config->PlatformInfo.Automatic) {
    if (!UseOemSerial) {
      AsciiStrCpyS (
        InfoData.Oem.SystemSerialNumber,
        OC_OEM_SERIAL_MAX,
        OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber)
        );
    }

    if (!UseOemUuid) {
      OcAsciiStrToRawGuid (
        OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemUuid),
        &InfoData.Oem.SystemUuid
        );
    }

    if (!UseOemMlb) {
      AsciiStrCpyS (
        InfoData.Oem.Mlb,
        OC_OEM_SERIAL_MAX,
        OC_BLOB_GET (&Config->PlatformInfo.Generic.Mlb)
        );
    }

    if (!UseOemRom) {
      CopyMem (InfoData.Oem.Rom, Config->PlatformInfo.Generic.Rom, OC_OEM_ROM_MAX);
    }
  }

  if (!EFI_ERROR (Status)) {
    if (Config->PlatformInfo.UpdateSmbios) {
      SmbiosUpdateMode = OcSmbiosGetUpdateMode (
        OC_BLOB_GET (&Config->PlatformInfo.UpdateSmbiosMode)
        );
      OcPlatformUpdateSmbios (
        Config,
        CpuInfo,
        UsedMacInfo,
        &SmbiosTable,
        SmbiosUpdateMode
        );
    }

    OcSmbiosTableFree (&SmbiosTable);
  } else {
    DEBUG ((DEBUG_WARN, "OC: Unable to obtain SMBIOS - %r\n", Status));
  }

  if (Config->PlatformInfo.UpdateDataHub) {
    OcPlatformUpdateDataHub (Config, CpuInfo, UsedMacInfo);
  }

  if (Config->PlatformInfo.UpdateNvram) {
    OcPlatformUpdateNvram (Config, UsedMacInfo);
  }
}

BOOLEAN
OcPlatformIs64BitSupported (
  IN UINT32     KernelVersion
  )
{
#if defined(MDE_CPU_IA32)
  return FALSE;
#elif defined(MDE_CPU_X64)
  return IsMacModel64BitCompatible (mCurrentSmbiosProductName, KernelVersion);
#else
#error "Unsupported architecture"
#endif
}

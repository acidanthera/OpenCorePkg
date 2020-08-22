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

#include <OpenCore.h>

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

STATIC CONST CHAR8     *mCurrentSmbiosProductName;

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

    if (!EFI_ERROR (AsciiStrToGuid (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemUuid), &Uuid))) {
      Data.SystemUUID         = &Uuid;
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
  
    if (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber)[0] != '\0') {
      Data.SystemSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber);
    }

    if (!EFI_ERROR (AsciiStrToGuid (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemUuid), &Uuid))) {
      Data.SystemUUID         = &Uuid;
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
  EFI_GUID         Uuid;
  UINT8            SmcVersion[APPLE_SMBIOS_SMC_VERSION_SIZE];

  ZeroMem (&Data, sizeof (Data));

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

    if (!EFI_ERROR (AsciiStrToGuid (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemUuid), &Uuid))) {
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

    if (Config->PlatformInfo.Smbios.MemoryFormFactor != 0) {
      Data.MemoryFormFactor = &Config->PlatformInfo.Smbios.MemoryFormFactor;
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

    if (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber)[0] != '\0') {
      Data.SystemSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber);
    }

    if (!EFI_ERROR (AsciiStrToGuid (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemUuid), &Uuid))) {
      Data.SystemUUID = &Uuid;
    }

    Data.BoardType              = MacInfo->Smbios.BoardType;
    Data.ChassisType            = MacInfo->Smbios.ChassisType;
    Data.SystemSKUNumber        = MacInfo->Smbios.SystemSKUNumber;
    Data.SystemFamily           = MacInfo->Smbios.SystemFamily;
    Data.BoardProduct           = MacInfo->Smbios.BoardProduct;
    Data.BoardVersion           = MacInfo->Smbios.BoardVersion;

    if (OC_BLOB_GET (&Config->PlatformInfo.Generic.Mlb)[0] != '\0') {
      Data.BoardSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.Generic.Mlb);
    }

    Data.BoardAssetTag          = MacInfo->Smbios.BoardAssetTag;
    Data.BoardLocationInChassis = MacInfo->Smbios.BoardLocationInChassis;
    Data.ChassisVersion         = MacInfo->Smbios.ChassisVersion;

    if (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber)[0] != '\0') {
      Data.ChassisSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemSerialNumber);
    }

    Data.ChassisAssetTag      = MacInfo->Smbios.ChassisAssetTag;
    Data.MemoryFormFactor     = MacInfo->Smbios.MemoryFormFactor;
    Data.FirmwareFeatures     = MacInfo->Smbios.FirmwareFeatures;
    Data.FirmwareFeaturesMask = MacInfo->Smbios.FirmwareFeaturesMask;

    //
    // Adopt to arbitrary hardware specifics. See description in NVRAM handling code.
    //
    if (Config->PlatformInfo.Generic.AdviseWindows) {
      Data.FirmwareFeatures     |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT;
      Data.FirmwareFeaturesMask |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT;
    }

    Data.ProcessorType        = NULL;
    Data.PlatformFeature      = MacInfo->Smbios.PlatformFeature;

    if (MacInfo->DataHub.SmcRevision != NULL) {
      OcSmbiosGetSmcVersion (MacInfo->DataHub.SmcRevision, SmcVersion);
      Data.SmcVersion           = SmcVersion;
    }
  }

  DEBUG ((DEBUG_INFO, "OC: New SMBIOS: %a model %a\n", Data.SystemManufacturer, Data.SystemProductName));
  mCurrentSmbiosProductName = Data.SystemProductName;

  Status = OcSmbiosCreate (SmbiosTable, &Data, UpdateMode, CpuInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Failed to update SMBIOS - %r\n", Status));
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
  CONST  UINT8 *Rom;
  UINTN        RomSize;
  UINT64       ExFeatures;
  UINT64       ExFeaturesMask;
  UINT32       Features;
  UINT32       FeaturesMask;

  if (MacInfo == NULL) {
    Bid            = OC_BLOB_GET (&Config->PlatformInfo.Nvram.Bid);
    BidSize        = Config->PlatformInfo.Nvram.Bid.Size - 1;
    Mlb            = OC_BLOB_GET (&Config->PlatformInfo.Nvram.Mlb);
    MlbSize        = Config->PlatformInfo.Nvram.Mlb.Size - 1;
    Rom            = &Config->PlatformInfo.Nvram.Rom[0];
    RomSize        = sizeof (Config->PlatformInfo.Nvram.Rom);
    ExFeatures     = Config->PlatformInfo.Nvram.FirmwareFeatures;
    ExFeaturesMask = Config->PlatformInfo.Nvram.FirmwareFeaturesMask;
  } else {
    Bid            = MacInfo->Smbios.BoardProduct;
    BidSize        = AsciiStrLen (Bid);
    Mlb            = OC_BLOB_GET (&Config->PlatformInfo.Generic.Mlb);
    MlbSize        = Config->PlatformInfo.Generic.Mlb.Size - 1;
    Rom            = &Config->PlatformInfo.Generic.Rom[0];
    RomSize        = sizeof (Config->PlatformInfo.Generic.Rom);
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
    if (Config->PlatformInfo.Generic.AdviseWindows) {
      ExFeatures     |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT;
      ExFeaturesMask |= FW_FEATURE_SUPPORTS_CSM_LEGACY_MODE | FW_FEATURE_SUPPORTS_UEFI_WINDOWS_BOOT;
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
  CONST CHAR8            *SmbiosUpdateStr;
  OC_SMBIOS_UPDATE_MODE  SmbiosUpdateMode;
  MAC_INFO_DATA          InfoData;
  MAC_INFO_DATA          *UsedMacInfo;
  EFI_STATUS             Status;
  OC_SMBIOS_TABLE        SmbiosTable;
  BOOLEAN                ExposeOem;

  if (Config->PlatformInfo.Automatic) {
    GetMacInfo (OC_BLOB_GET (&Config->PlatformInfo.Generic.SystemProductName), &InfoData);
    UsedMacInfo = &InfoData;
  } else {
    UsedMacInfo = NULL;
  }

  if (Config->PlatformInfo.UpdateDataHub) {
    OcPlatformUpdateDataHub (Config, CpuInfo, UsedMacInfo);
  }

  ExposeOem = (Config->Misc.Security.ExposeSensitiveData & OCS_EXPOSE_OEM_INFO) != 0;

  if (ExposeOem || Config->PlatformInfo.UpdateSmbios) {
    Status = OcSmbiosTablePrepare (&SmbiosTable);
    if (!EFI_ERROR (Status)) {
      if (ExposeOem) {
        OcSmbiosExposeOemInfo (&SmbiosTable);
      }

      mCurrentSmbiosProductName = OcSmbiosGetProductName (&SmbiosTable);
      DEBUG ((DEBUG_INFO, "OC: Current SMBIOS: %a model %a\n", OcSmbiosGetManufacturer (&SmbiosTable), mCurrentSmbiosProductName));

      if (Config->PlatformInfo.UpdateSmbios) {
        SmbiosUpdateStr  = OC_BLOB_GET (&Config->PlatformInfo.UpdateSmbiosMode);

        if (AsciiStrCmp (SmbiosUpdateStr, "TryOverwrite") == 0) {
          SmbiosUpdateMode = OcSmbiosUpdateTryOverwrite;
        } else if (AsciiStrCmp (SmbiosUpdateStr, "Create") == 0) {
          SmbiosUpdateMode = OcSmbiosUpdateCreate;
        } else if (AsciiStrCmp (SmbiosUpdateStr, "Overwrite") == 0) {
          SmbiosUpdateMode = OcSmbiosUpdateOverwrite;
        } else if (AsciiStrCmp (SmbiosUpdateStr, "Custom") == 0) {
          SmbiosUpdateMode = OcSmbiosUpdateCustom;
        } else {
          DEBUG ((DEBUG_WARN, "OC: Invalid SMBIOS update mode %a\n", SmbiosUpdateStr));
          SmbiosUpdateMode = OcSmbiosUpdateCreate;
        }

        OcPlatformUpdateSmbios (Config, CpuInfo, UsedMacInfo, &SmbiosTable, SmbiosUpdateMode);
      }

      OcSmbiosTableFree (&SmbiosTable);
    } else {
      DEBUG ((DEBUG_WARN, "OC: Unable to obtain SMBIOS - %r\n", Status));
    }
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

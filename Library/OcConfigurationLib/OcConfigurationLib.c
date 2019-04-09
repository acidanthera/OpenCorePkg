/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/OcConfigurationLib.h>

OC_ARRAY_STRUCTORS (OC_ACPI_ADD_ARRAY)
OC_STRUCTORS       (OC_ACPI_BLOCK_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_ACPI_BLOCK_ARRAY)
OC_STRUCTORS       (OC_ACPI_PATCH_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_ACPI_PATCH_ARRAY)
OC_STRUCTORS       (OC_ACPI_QUIRKS, ())
OC_STRUCTORS       (OC_ACPI_CONFIG, ())

OC_MAP_STRUCTORS   (OC_DEV_PROP_ADD_MAP)
OC_STRUCTORS       (OC_DEV_PROP_BLOCK_ENTRY, ())
OC_MAP_STRUCTORS   (OC_DEV_PROP_BLOCK_MAP)
OC_STRUCTORS       (OC_DEV_PROP_QUIRKS, ())
OC_STRUCTORS       (OC_DEV_PROP_CONFIG, ())

OC_STRUCTORS       (OC_KERNEL_ADD_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_ADD_ARRAY)
OC_STRUCTORS       (OC_KERNEL_BLOCK_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_BLOCK_ARRAY)
OC_STRUCTORS       (OC_KERNEL_PATCH_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_PATCH_ARRAY)
OC_STRUCTORS       (OC_KERNEL_QUIRKS, ())
OC_STRUCTORS       (OC_KERNEL_CONFIG, ())

OC_STRUCTORS       (OC_MISC_DEBUG, ())
OC_STRUCTORS       (OC_MISC_CONFIG, ())

OC_MAP_STRUCTORS   (OC_NVRAM_ADD_MAP)
OC_STRUCTORS       (OC_NVRAM_BLOCK_ENTRY, ())
OC_MAP_STRUCTORS   (OC_NVRAM_BLOCK_MAP)
OC_STRUCTORS       (OC_NVRAM_CONFIG, ())

OC_STRUCTORS       (OC_PLATFORM_GENERIC_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_DATA_HUB_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_NVRAM_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_SMBIOS_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_CONFIG, ())

OC_ARRAY_STRUCTORS (OC_UEFI_DRIVER_ARRAY)
OC_STRUCTORS       (OC_UEFI_QUIRKS, ())
OC_STRUCTORS       (OC_UEFI_CONFIG, ())

OC_STRUCTORS       (OC_GLOBAL_CONFIG, ())

//
// ACPI configuration support
//

STATIC
OC_SCHEMA
mAcpiAddSchema = OC_SCHEMA_STRING (NULL);

STATIC
OC_SCHEMA
mAcpiBlockSchemaEntry[] = {
  OC_SCHEMA_STRING_IN    ("Comment",        OC_ACPI_BLOCK_ENTRY, Comment),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled",        OC_ACPI_BLOCK_ENTRY, Enabled),
  OC_SCHEMA_DATAF_IN     ("OemTableId",     OC_ACPI_BLOCK_ENTRY, OemTableId),
  OC_SCHEMA_INTEGER_IN   ("TableLength",    OC_ACPI_BLOCK_ENTRY, TableLength),
  OC_SCHEMA_DATAF_IN     ("TableSignature", OC_ACPI_BLOCK_ENTRY, TableSignature),
};

STATIC
OC_SCHEMA
mAcpiBlockSchema = OC_SCHEMA_DICT (NULL, mAcpiBlockSchemaEntry);

STATIC
OC_SCHEMA
mAcpiPatchSchemaEntry[] = {
  OC_SCHEMA_STRING_IN    ("Comment",        OC_ACPI_PATCH_ENTRY, Comment),
  OC_SCHEMA_INTEGER_IN   ("Count",          OC_ACPI_PATCH_ENTRY, Count),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled",        OC_ACPI_PATCH_ENTRY, Enabled),
  OC_SCHEMA_DATA_IN      ("Find",           OC_ACPI_PATCH_ENTRY, Find),
  OC_SCHEMA_INTEGER_IN   ("Limit",          OC_ACPI_PATCH_ENTRY, Limit),
  OC_SCHEMA_DATA_IN      ("Mask",           OC_ACPI_PATCH_ENTRY, Mask),
  OC_SCHEMA_DATAF_IN     ("OemTableId",     OC_ACPI_PATCH_ENTRY, OemTableId),
  OC_SCHEMA_DATA_IN      ("Replace",        OC_ACPI_PATCH_ENTRY, Replace),
  OC_SCHEMA_DATA_IN      ("ReplaceMask",    OC_ACPI_PATCH_ENTRY, ReplaceMask),
  OC_SCHEMA_INTEGER_IN   ("Skip",           OC_ACPI_PATCH_ENTRY, Skip),
  OC_SCHEMA_INTEGER_IN   ("TableLength",    OC_ACPI_PATCH_ENTRY, TableLength),
  OC_SCHEMA_DATAF_IN     ("TableSignature", OC_ACPI_PATCH_ENTRY, TableSignature),
};

STATIC
OC_SCHEMA
mAcpiPatchSchema = OC_SCHEMA_DICT (NULL, mAcpiPatchSchemaEntry);

STATIC
OC_SCHEMA
mAcpiQuirksSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("FadtEnableReset",  OC_GLOBAL_CONFIG, Acpi.Quirks.FadtEnableReset),
  OC_SCHEMA_BOOLEAN_IN ("IgnoreForWindows", OC_GLOBAL_CONFIG, Acpi.Quirks.IgnoreForWindows),
  OC_SCHEMA_BOOLEAN_IN ("NormalizeHeaders", OC_GLOBAL_CONFIG, Acpi.Quirks.NormalizeHeaders),
  OC_SCHEMA_BOOLEAN_IN ("RebaseRegions",    OC_GLOBAL_CONFIG, Acpi.Quirks.RebaseRegions)
};

STATIC
OC_SCHEMA
mAcpiConfigurationSchema[] = {
  OC_SCHEMA_ARRAY_IN   ("Add",    OC_GLOBAL_CONFIG, Acpi.Add,    &mAcpiAddSchema),
  OC_SCHEMA_ARRAY_IN   ("Block",  OC_GLOBAL_CONFIG, Acpi.Block,  &mAcpiBlockSchema),
  OC_SCHEMA_ARRAY_IN   ("Patch",  OC_GLOBAL_CONFIG, Acpi.Patch,  &mAcpiPatchSchema),
  OC_SCHEMA_DICT       ("Quirks", mAcpiQuirksSchema),
};



//
// Device properties support
//

STATIC
OC_SCHEMA
mDevicePropertiesAddEntrySchema = OC_SCHEMA_MDATA (NULL);


STATIC
OC_SCHEMA
mDevicePropertiesAddSchema = OC_SCHEMA_MAP (NULL, &mDevicePropertiesAddEntrySchema);

STATIC
OC_SCHEMA
mDevicePropertiesBlockEntrySchema = OC_SCHEMA_STRING (NULL);

STATIC
OC_SCHEMA
mDevicePropertiesBlockSchema = OC_SCHEMA_ARRAY (NULL, &mDevicePropertiesBlockEntrySchema);

STATIC
OC_SCHEMA
mDevicePropertiesQuirksSchema[] = {
  OC_SCHEMA_BOOLEAN_IN  ("ReinstallProtocol",  OC_GLOBAL_CONFIG, DeviceProperties.Quirks.ReinstallProtocol),
};

STATIC
OC_SCHEMA
mDevicePropertiesSchema[] = {
  OC_SCHEMA_MAP_IN      ("Add",                OC_GLOBAL_CONFIG, DeviceProperties.Add, &mDevicePropertiesAddSchema),
  OC_SCHEMA_MAP_IN      ("Block",              OC_GLOBAL_CONFIG, DeviceProperties.Block, &mDevicePropertiesBlockSchema),
  OC_SCHEMA_DICT        ("Quirks",             mDevicePropertiesQuirksSchema)
};

//
// Kernel space configuration support
//

STATIC
OC_SCHEMA
mKernelAddSchemaEntry[] = {
  OC_SCHEMA_STRING_IN    ("BundleName",     OC_KERNEL_ADD_ENTRY, BundleName),
  OC_SCHEMA_STRING_IN    ("Comment",        OC_KERNEL_ADD_ENTRY, Comment),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled",        OC_KERNEL_ADD_ENTRY, Enabled),
  OC_SCHEMA_STRING_IN    ("ExecutablePath", OC_KERNEL_ADD_ENTRY, ExecutablePath),
  OC_SCHEMA_STRING_IN    ("MatchKernel",    OC_KERNEL_ADD_ENTRY, MatchKernel),
  OC_SCHEMA_STRING_IN    ("PlistPath",      OC_KERNEL_ADD_ENTRY, PlistPath),
};

STATIC
OC_SCHEMA
mKernelAddSchema = OC_SCHEMA_DICT (NULL, mKernelAddSchemaEntry);

STATIC
OC_SCHEMA
mKernelBlockSchemaEntry[] = {
  OC_SCHEMA_STRING_IN    ("Comment",        OC_KERNEL_BLOCK_ENTRY, Comment),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled",        OC_KERNEL_BLOCK_ENTRY, Enabled),
  OC_SCHEMA_STRING_IN    ("Identifier",     OC_KERNEL_BLOCK_ENTRY, Identifier),
  OC_SCHEMA_STRING_IN    ("MatchKernel",    OC_KERNEL_BLOCK_ENTRY, MatchKernel),
};

STATIC
OC_SCHEMA
mKernelBlockSchema = OC_SCHEMA_DICT (NULL, mKernelBlockSchemaEntry);

STATIC
OC_SCHEMA
mKernelPatchSchemaEntry[] = {
  OC_SCHEMA_STRING_IN    ("Base",           OC_KERNEL_PATCH_ENTRY, Base),
  OC_SCHEMA_STRING_IN    ("Comment",        OC_KERNEL_PATCH_ENTRY, Comment),
  OC_SCHEMA_INTEGER_IN   ("Count",          OC_KERNEL_PATCH_ENTRY, Count),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled",        OC_KERNEL_PATCH_ENTRY, Enabled),
  OC_SCHEMA_DATA_IN      ("Find",           OC_KERNEL_PATCH_ENTRY, Find),
  OC_SCHEMA_STRING_IN    ("Identifier",     OC_KERNEL_PATCH_ENTRY, Identifier),
  OC_SCHEMA_INTEGER_IN   ("Limit",          OC_KERNEL_PATCH_ENTRY, Limit),
  OC_SCHEMA_DATA_IN      ("Mask",           OC_KERNEL_PATCH_ENTRY, Mask),
  OC_SCHEMA_STRING_IN    ("MatchKernel",    OC_KERNEL_PATCH_ENTRY, MatchKernel),
  OC_SCHEMA_DATA_IN      ("Replace",        OC_KERNEL_PATCH_ENTRY, Replace),
  OC_SCHEMA_DATA_IN      ("ReplaceMask",    OC_KERNEL_PATCH_ENTRY, ReplaceMask),
  OC_SCHEMA_INTEGER_IN   ("Skip",           OC_KERNEL_PATCH_ENTRY, Skip)
};

STATIC
OC_SCHEMA
mKernelPatchSchema = OC_SCHEMA_DICT (NULL, mKernelPatchSchemaEntry);

STATIC
OC_SCHEMA
mKernelQuirksSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("AppleCpuPmCfgLock", OC_GLOBAL_CONFIG, Kernel.Quirks.AppleCpuPmCfgLock),
  OC_SCHEMA_BOOLEAN_IN ("ExternalDiskIcons", OC_GLOBAL_CONFIG, Kernel.Quirks.ExternalDiskIcons),
  OC_SCHEMA_BOOLEAN_IN ("ThirdPartyTrim",    OC_GLOBAL_CONFIG, Kernel.Quirks.ThirdPartyTrim),
  OC_SCHEMA_BOOLEAN_IN ("XhciPortLimit",     OC_GLOBAL_CONFIG, Kernel.Quirks.XhciPortLimit),
};

STATIC
OC_SCHEMA
mKernelConfigurationSchema[] = {
  OC_SCHEMA_ARRAY_IN   ("Add",    OC_GLOBAL_CONFIG, Kernel.Add, &mKernelAddSchema),
  OC_SCHEMA_ARRAY_IN   ("Block",  OC_GLOBAL_CONFIG, Kernel.Block, &mKernelBlockSchema),
  OC_SCHEMA_ARRAY_IN   ("Patch",  OC_GLOBAL_CONFIG, Kernel.Patch, &mKernelPatchSchema),
  OC_SCHEMA_DICT       ("Quirks", mKernelQuirksSchema),
};

//
// Misc configuration support
//

STATIC
OC_SCHEMA
mMiscConfigurationDebugSchema[] = {
  OC_SCHEMA_INTEGER_IN ("Delay",            OC_GLOBAL_CONFIG, Misc.Debug.Delay),
  OC_SCHEMA_INTEGER_IN ("Target",           OC_GLOBAL_CONFIG, Misc.Debug.Target)
};


STATIC
OC_SCHEMA
mMiscConfigurationSchema[] = {
  OC_SCHEMA_DICT       ("Debug",            mMiscConfigurationDebugSchema),
};

//
// Nvram configuration support
//

STATIC
OC_SCHEMA
mNvramAddEntrySchema = OC_SCHEMA_MDATA (NULL);

STATIC
OC_SCHEMA
mNvramAddSchema = OC_SCHEMA_MAP (NULL, &mNvramAddEntrySchema);

STATIC
OC_SCHEMA
mNvramBlockEntrySchema = OC_SCHEMA_STRING (NULL);

STATIC
OC_SCHEMA
mNvramBlockSchema = OC_SCHEMA_ARRAY (NULL, &mNvramBlockEntrySchema);

STATIC
OC_SCHEMA
mNvramConfigurationSchema[] = {
  OC_SCHEMA_MAP_IN   ("Add",    OC_GLOBAL_CONFIG, Nvram.Add, &mNvramAddSchema),
  OC_SCHEMA_MAP_IN   ("Block",  OC_GLOBAL_CONFIG, Nvram.Block, &mNvramBlockSchema)
};

//
// Platform info configuration support
//
STATIC
OC_SCHEMA
mPlatformConfigurationDataHubSchema[] = {
  OC_SCHEMA_INTEGER_IN ("ARTFrequency",         OC_GLOBAL_CONFIG, PlatformInfo.DataHub.ARTFrequency),
  OC_SCHEMA_STRING_IN  ("BoardProduct",         OC_GLOBAL_CONFIG, PlatformInfo.DataHub.BoardProduct),
  OC_SCHEMA_DATAF_IN   ("BoardRevision",        OC_GLOBAL_CONFIG, PlatformInfo.DataHub.BoardRevision),
  OC_SCHEMA_DATAF_IN   ("DevicePathsSupported", OC_GLOBAL_CONFIG, PlatformInfo.DataHub.DevicePathsSupported),
  OC_SCHEMA_INTEGER_IN ("FSBFrequency",         OC_GLOBAL_CONFIG, PlatformInfo.DataHub.FSBFrequency),
  OC_SCHEMA_INTEGER_IN ("InitialTSC",           OC_GLOBAL_CONFIG, PlatformInfo.DataHub.InitialTSC),
  OC_SCHEMA_STRING_IN  ("PlatformName",         OC_GLOBAL_CONFIG, PlatformInfo.DataHub.PlatformName),
  OC_SCHEMA_DATAF_IN   ("SmcBranch",            OC_GLOBAL_CONFIG, PlatformInfo.DataHub.SmcBranch),
  OC_SCHEMA_DATAF_IN   ("SmcPlatform",          OC_GLOBAL_CONFIG, PlatformInfo.DataHub.SmcPlatform),
  OC_SCHEMA_DATAF_IN   ("SmcRevision",          OC_GLOBAL_CONFIG, PlatformInfo.DataHub.SmcRevision),
  OC_SCHEMA_INTEGER_IN ("StartupPowerEvents",   OC_GLOBAL_CONFIG, PlatformInfo.DataHub.StartupPowerEvents),
  OC_SCHEMA_STRING_IN  ("SystemProductName",    OC_GLOBAL_CONFIG, PlatformInfo.DataHub.SystemProductName),
  OC_SCHEMA_STRING_IN  ("SystemSerialNumber",   OC_GLOBAL_CONFIG, PlatformInfo.DataHub.SystemSerialNumber),
  OC_SCHEMA_STRING_IN  ("SystemUUID",           OC_GLOBAL_CONFIG, PlatformInfo.DataHub.SystemUuid),
};

STATIC
OC_SCHEMA
mPlatformConfigurationGenericSchema[] = {
  OC_SCHEMA_STRING_IN ("MLB",                OC_GLOBAL_CONFIG, PlatformInfo.Generic.Mlb),
  OC_SCHEMA_DATAF_IN  ("ROM",                OC_GLOBAL_CONFIG, PlatformInfo.Generic.Rom),
  OC_SCHEMA_STRING_IN ("SystemProductName",  OC_GLOBAL_CONFIG, PlatformInfo.Generic.SystemProductName),
  OC_SCHEMA_STRING_IN ("SystemSerialNumber", OC_GLOBAL_CONFIG, PlatformInfo.Generic.SystemSerialNumber),
  OC_SCHEMA_STRING_IN ("SystemUUID",         OC_GLOBAL_CONFIG, PlatformInfo.Generic.SystemUuid),
};

STATIC
OC_SCHEMA
mPlatformConfigurationNvramSchema[] = {
  OC_SCHEMA_STRING_IN ("BID",                OC_GLOBAL_CONFIG, PlatformInfo.Nvram.Bid),
  OC_SCHEMA_STRING_IN ("MLB",                OC_GLOBAL_CONFIG, PlatformInfo.Nvram.Mlb),
  OC_SCHEMA_DATAF_IN  ("ROM",                OC_GLOBAL_CONFIG, PlatformInfo.Nvram.Rom)
};

STATIC
OC_SCHEMA
mPlatformConfigurationSmbiosSchema[] = {
  OC_SCHEMA_STRING_IN  ("BIOSReleaseDate",        OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BIOSReleaseDate),
  OC_SCHEMA_STRING_IN  ("BIOSVendor",             OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BIOSVendor),
  OC_SCHEMA_STRING_IN  ("BIOSVersion",            OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BIOSVersion),
  OC_SCHEMA_STRING_IN  ("BoardAssetTag",          OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BoardAssetTag),
  OC_SCHEMA_STRING_IN  ("BoardLocationInChassis", OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BoardLocationInChassis),
  OC_SCHEMA_STRING_IN  ("BoardManufacturer",      OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BoardManufacturer),
  OC_SCHEMA_STRING_IN  ("BoardProduct",           OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BoardProduct),
  OC_SCHEMA_STRING_IN  ("BoardSerialNumber",      OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BoardSerialNumber),
  OC_SCHEMA_INTEGER_IN ("BoardType",              OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BoardType),
  OC_SCHEMA_STRING_IN  ("BoardVersion",           OC_GLOBAL_CONFIG, PlatformInfo.Smbios.BoardVersion),
  OC_SCHEMA_STRING_IN  ("ChassisAssetTag",        OC_GLOBAL_CONFIG, PlatformInfo.Smbios.ChassisAssetTag),
  OC_SCHEMA_STRING_IN  ("ChassisManufacturer",    OC_GLOBAL_CONFIG, PlatformInfo.Smbios.ChassisManufacturer),
  OC_SCHEMA_STRING_IN  ("ChassisSerialNumber",    OC_GLOBAL_CONFIG, PlatformInfo.Smbios.ChassisSerialNumber),
  OC_SCHEMA_INTEGER_IN ("ChassisType",            OC_GLOBAL_CONFIG, PlatformInfo.Smbios.ChassisType),
  OC_SCHEMA_STRING_IN  ("ChassisVersion",         OC_GLOBAL_CONFIG, PlatformInfo.Smbios.ChassisVersion),
  OC_SCHEMA_DATAF_IN   ("FirmwareFeatures",       OC_GLOBAL_CONFIG, PlatformInfo.Smbios.FirmwareFeatures),
  OC_SCHEMA_DATAF_IN   ("FirmwareFeaturesMask",   OC_GLOBAL_CONFIG, PlatformInfo.Smbios.FirmwareFeaturesMask),
  OC_SCHEMA_INTEGER_IN ("MemoryFormFactor",       OC_GLOBAL_CONFIG, PlatformInfo.Smbios.MemoryFormFactor),
  OC_SCHEMA_INTEGER_IN ("PlatformFeature",        OC_GLOBAL_CONFIG, PlatformInfo.Smbios.PlatformFeature),
  OC_SCHEMA_INTEGER_IN ("ProcessorType",          OC_GLOBAL_CONFIG, PlatformInfo.Smbios.ProcessorType),
  OC_SCHEMA_STRING_IN  ("SystemFamily",           OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SystemFamily),
  OC_SCHEMA_STRING_IN  ("SystemManufacturer",     OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SystemManufacturer),
  OC_SCHEMA_STRING_IN  ("SystemProductName",      OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SystemProductName),
  OC_SCHEMA_STRING_IN  ("SystemSKUNumber",        OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SystemSKUNumber),
  OC_SCHEMA_STRING_IN  ("SystemSerialNumber",     OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SystemSerialNumber),
  OC_SCHEMA_STRING_IN  ("SystemUUID",             OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SystemUuid),
  OC_SCHEMA_STRING_IN  ("SystemVersion",          OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SystemVersion),
};

STATIC
OC_SCHEMA
mPlatformConfigurationSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("Automatic",        OC_GLOBAL_CONFIG, PlatformInfo.Automatic),
  OC_SCHEMA_DICT       ("DataHub",          mPlatformConfigurationDataHubSchema),
  OC_SCHEMA_DICT       ("Generic",          mPlatformConfigurationGenericSchema),
  OC_SCHEMA_DICT       ("PlatformNVRAM",    mPlatformConfigurationNvramSchema),
  OC_SCHEMA_DICT       ("SMBIOS",           mPlatformConfigurationSmbiosSchema),
  OC_SCHEMA_BOOLEAN_IN ("UpdateDataHub",    OC_GLOBAL_CONFIG, PlatformInfo.UpdateDataHub),
  OC_SCHEMA_BOOLEAN_IN ("UpdateNVRAM",      OC_GLOBAL_CONFIG, PlatformInfo.UpdateNvram),
  OC_SCHEMA_BOOLEAN_IN ("UpdateSMBIOS",     OC_GLOBAL_CONFIG, PlatformInfo.UpdateSmbios),
  OC_SCHEMA_STRING_IN  ("UpdateSMBIOSMode", OC_GLOBAL_CONFIG, PlatformInfo.UpdateSmbiosMode)
};

//
// Uefi configuration support
//

STATIC
OC_SCHEMA
mUefiDriversSchema = OC_SCHEMA_STRING (NULL);

STATIC
OC_SCHEMA
mUefiQuirksSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("DisableWatchDog",        OC_GLOBAL_CONFIG, Uefi.Quirks.DisableWatchDog),
  OC_SCHEMA_BOOLEAN_IN ("IgnoreInvalidFlexRatio", OC_GLOBAL_CONFIG, Uefi.Quirks.IgnoreInvalidFlexRatio),
  OC_SCHEMA_BOOLEAN_IN ("ProvideConsoleGop",      OC_GLOBAL_CONFIG, Uefi.Quirks.ProvideConsoleGop),
  OC_SCHEMA_BOOLEAN_IN ("ReleaseUsbOwnership",    OC_GLOBAL_CONFIG, Uefi.Quirks.ReleaseUsbOwnership)
};

STATIC
OC_SCHEMA
mUefiConfigurationSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("ConnectDrivers", OC_GLOBAL_CONFIG, Uefi.ConnectDrivers),
  OC_SCHEMA_ARRAY_IN   ("Drivers",        OC_GLOBAL_CONFIG, Uefi.Drivers, &mUefiDriversSchema),
  OC_SCHEMA_DICT       ("Quirks",         mUefiQuirksSchema)
};

//
// Root configuration
//

STATIC
OC_SCHEMA
mRootConfigurationNodes[] = {
  OC_SCHEMA_DICT    ("ACPI",             mAcpiConfigurationSchema),
  OC_SCHEMA_DICT    ("DeviceProperties", mDevicePropertiesSchema),
  OC_SCHEMA_DICT    ("Kernel",           mKernelConfigurationSchema),
  OC_SCHEMA_DICT    ("Misc",             mMiscConfigurationSchema),
  OC_SCHEMA_DICT    ("NVRAM",            mNvramConfigurationSchema),
  OC_SCHEMA_DICT    ("PlatformInfo",     mPlatformConfigurationSchema),
  OC_SCHEMA_DICT    ("UEFI",             mUefiConfigurationSchema)
};

STATIC
OC_SCHEMA_INFO
mRootConfigurationInfo = {
  .Dict = {mRootConfigurationNodes, ARRAY_SIZE (mRootConfigurationNodes)}
};

EFI_STATUS
OcConfigurationInit (
  OUT OC_GLOBAL_CONFIG   *Config,
  IN  VOID               *Buffer,
  IN  UINT32             Size
  )
{
  BOOLEAN  Success;

  OC_GLOBAL_CONFIG_CONSTRUCT (Config, sizeof (*Config));
  Success = ParseSerialized (Config, &mRootConfigurationInfo, Buffer, Size);

  if (!Success) {
    OC_GLOBAL_CONFIG_DESTRUCT (Config, sizeof (*Config));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Free configuration structure.

  @param[in,out]  Config   Configuration structure.
**/
VOID
OcConfigurationFree (
  IN OUT OC_GLOBAL_CONFIG   *Config
  )
{
  OC_GLOBAL_CONFIG_DESTRUCT (Config, sizeof (*Config));
}

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

OC_STRUCTORS       (OC_ACPI_ADD_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_ACPI_ADD_ARRAY)
OC_STRUCTORS       (OC_ACPI_BLOCK_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_ACPI_BLOCK_ARRAY)
OC_STRUCTORS       (OC_ACPI_PATCH_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_ACPI_PATCH_ARRAY)
OC_STRUCTORS       (OC_ACPI_QUIRKS, ())
OC_STRUCTORS       (OC_ACPI_CONFIG, ())

OC_STRUCTORS       (OC_BOOTER_WL_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_BOOTER_WL_ARRAY)
OC_STRUCTORS       (OC_BOOTER_QUIRKS, ())
OC_STRUCTORS       (OC_BOOTER_CONFIG, ())

OC_MAP_STRUCTORS   (OC_DEV_PROP_ADD_MAP)
OC_STRUCTORS       (OC_DEV_PROP_BLOCK_ENTRY, ())
OC_MAP_STRUCTORS   (OC_DEV_PROP_BLOCK_MAP)
OC_STRUCTORS       (OC_DEV_PROP_CONFIG, ())

OC_STRUCTORS       (OC_KERNEL_ADD_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_ADD_ARRAY)
OC_STRUCTORS       (OC_KERNEL_BLOCK_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_BLOCK_ARRAY)
OC_STRUCTORS       (OC_KERNEL_EMULATE, ())
OC_STRUCTORS       (OC_KERNEL_PATCH_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_PATCH_ARRAY)
OC_STRUCTORS       (OC_KERNEL_QUIRKS, ())
OC_STRUCTORS       (OC_KERNEL_CONFIG, ())

OC_ARRAY_STRUCTORS (OC_MISC_BLESS_ARRAY)
OC_STRUCTORS       (OC_MISC_BOOT, ())
OC_STRUCTORS       (OC_MISC_DEBUG, ())
OC_STRUCTORS       (OC_MISC_SECURITY, ())
OC_STRUCTORS       (OC_MISC_TOOLS_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_MISC_TOOLS_ARRAY)
OC_STRUCTORS       (OC_MISC_CONFIG, ())

OC_MAP_STRUCTORS   (OC_NVRAM_ADD_MAP)
OC_STRUCTORS       (OC_NVRAM_BLOCK_ENTRY, ())
OC_MAP_STRUCTORS   (OC_NVRAM_BLOCK_MAP)
OC_STRUCTORS       (OC_NVRAM_LEGACY_ENTRY, ())
OC_MAP_STRUCTORS   (OC_NVRAM_LEGACY_MAP)
OC_STRUCTORS       (OC_NVRAM_CONFIG, ())

OC_STRUCTORS       (OC_PLATFORM_GENERIC_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_DATA_HUB_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_NVRAM_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_SMBIOS_CONFIG, ())
OC_STRUCTORS       (OC_PLATFORM_CONFIG, ())

OC_ARRAY_STRUCTORS (OC_UEFI_DRIVER_ARRAY)
OC_STRUCTORS       (OC_UEFI_APFS, ())
OC_STRUCTORS       (OC_UEFI_AUDIO, ())
OC_STRUCTORS       (OC_UEFI_INPUT, ())
OC_STRUCTORS       (OC_UEFI_OUTPUT, ())
OC_STRUCTORS       (OC_UEFI_PROTOCOLS, ())
OC_STRUCTORS       (OC_UEFI_QUIRKS, ())
OC_STRUCTORS       (OC_UEFI_CONFIG, ())

OC_STRUCTORS       (OC_GLOBAL_CONFIG, ())

//
// ACPI configuration support
//

STATIC
OC_SCHEMA
mAcpiAddSchemaEntry[] = {
  OC_SCHEMA_STRING_IN    ("Comment",        OC_ACPI_ADD_ENTRY, Comment),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled",        OC_ACPI_ADD_ENTRY, Enabled),
  OC_SCHEMA_STRING_IN    ("Path",           OC_ACPI_ADD_ENTRY, Path)
};

STATIC
OC_SCHEMA
mAcpiAddSchema = OC_SCHEMA_DICT (NULL, mAcpiAddSchemaEntry);

STATIC
OC_SCHEMA
mAcpiBlockSchemaEntry[] = {
  OC_SCHEMA_BOOLEAN_IN   ("All",            OC_ACPI_BLOCK_ENTRY, All),
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
  OC_SCHEMA_BOOLEAN_IN ("NormalizeHeaders", OC_GLOBAL_CONFIG, Acpi.Quirks.NormalizeHeaders),
  OC_SCHEMA_BOOLEAN_IN ("RebaseRegions",    OC_GLOBAL_CONFIG, Acpi.Quirks.RebaseRegions),
  OC_SCHEMA_BOOLEAN_IN ("ResetHwSig",       OC_GLOBAL_CONFIG, Acpi.Quirks.ResetHwSig),
  OC_SCHEMA_BOOLEAN_IN ("ResetLogoStatus",  OC_GLOBAL_CONFIG, Acpi.Quirks.ResetLogoStatus),
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
// Apple bootloader configuration
//

STATIC
OC_SCHEMA
mBooterWhitelistEntrySchema[] = {
  OC_SCHEMA_INTEGER_IN   ("Address", OC_BOOTER_WL_ENTRY, Address),
  OC_SCHEMA_STRING_IN    ("Comment", OC_BOOTER_WL_ENTRY, Comment),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled", OC_BOOTER_WL_ENTRY, Enabled)
};

STATIC
OC_SCHEMA
mBooterWhitelistSchema = OC_SCHEMA_DICT (NULL, mBooterWhitelistEntrySchema);

STATIC
OC_SCHEMA
mBooterQuirksSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("AvoidRuntimeDefrag",     OC_GLOBAL_CONFIG, Booter.Quirks.AvoidRuntimeDefrag),
  OC_SCHEMA_BOOLEAN_IN ("DevirtualiseMmio",       OC_GLOBAL_CONFIG, Booter.Quirks.DevirtualiseMmio),
  OC_SCHEMA_BOOLEAN_IN ("DisableSingleUser",      OC_GLOBAL_CONFIG, Booter.Quirks.DisableSingleUser),
  OC_SCHEMA_BOOLEAN_IN ("DisableVariableWrite",   OC_GLOBAL_CONFIG, Booter.Quirks.DisableVariableWrite),
  OC_SCHEMA_BOOLEAN_IN ("DiscardHibernateMap",    OC_GLOBAL_CONFIG, Booter.Quirks.DiscardHibernateMap),
  OC_SCHEMA_BOOLEAN_IN ("EnableSafeModeSlide",    OC_GLOBAL_CONFIG, Booter.Quirks.EnableSafeModeSlide),
  OC_SCHEMA_BOOLEAN_IN ("EnableWriteUnprotector", OC_GLOBAL_CONFIG, Booter.Quirks.EnableWriteUnprotector),
  OC_SCHEMA_BOOLEAN_IN ("ForceExitBootServices",  OC_GLOBAL_CONFIG, Booter.Quirks.ForceExitBootServices),
  OC_SCHEMA_BOOLEAN_IN ("ProtectMemoryRegions",   OC_GLOBAL_CONFIG, Booter.Quirks.ProtectMemoryRegions),
  OC_SCHEMA_BOOLEAN_IN ("ProtectSecureBoot",      OC_GLOBAL_CONFIG, Booter.Quirks.ProtectSecureBoot),
  OC_SCHEMA_BOOLEAN_IN ("ProtectUefiServices",    OC_GLOBAL_CONFIG, Booter.Quirks.ProtectUefiServices),
  OC_SCHEMA_BOOLEAN_IN ("ProvideCustomSlide",     OC_GLOBAL_CONFIG, Booter.Quirks.ProvideCustomSlide),
  OC_SCHEMA_BOOLEAN_IN ("RebuildAppleMemoryMap",  OC_GLOBAL_CONFIG, Booter.Quirks.RebuildAppleMemoryMap),
  OC_SCHEMA_BOOLEAN_IN ("SetupVirtualMap",        OC_GLOBAL_CONFIG, Booter.Quirks.SetupVirtualMap),
  OC_SCHEMA_BOOLEAN_IN ("SignalAppleOS",          OC_GLOBAL_CONFIG, Booter.Quirks.SignalAppleOS),
  OC_SCHEMA_BOOLEAN_IN ("SyncRuntimePermissions", OC_GLOBAL_CONFIG, Booter.Quirks.SyncRuntimePermissions),
};

STATIC
OC_SCHEMA
mBooterConfigurationSchema[] = {
  OC_SCHEMA_ARRAY_IN   ("MmioWhitelist",   OC_GLOBAL_CONFIG, Booter.MmioWhitelist, &mBooterWhitelistSchema),
  OC_SCHEMA_DICT       ("Quirks",          mBooterQuirksSchema),
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
mDevicePropertiesSchema[] = {
  OC_SCHEMA_MAP_IN      ("Add",                OC_GLOBAL_CONFIG, DeviceProperties.Add, &mDevicePropertiesAddSchema),
  OC_SCHEMA_MAP_IN      ("Block",              OC_GLOBAL_CONFIG, DeviceProperties.Block, &mDevicePropertiesBlockSchema)
};

//
// Kernel space configuration support
//

STATIC
OC_SCHEMA
mKernelAddSchemaEntry[] = {
  OC_SCHEMA_STRING_IN    ("BundlePath",     OC_KERNEL_ADD_ENTRY, BundlePath),
  OC_SCHEMA_STRING_IN    ("Comment",        OC_KERNEL_ADD_ENTRY, Comment),
  OC_SCHEMA_BOOLEAN_IN   ("Enabled",        OC_KERNEL_ADD_ENTRY, Enabled),
  OC_SCHEMA_STRING_IN    ("ExecutablePath", OC_KERNEL_ADD_ENTRY, ExecutablePath),
  OC_SCHEMA_STRING_IN    ("MaxKernel",      OC_KERNEL_ADD_ENTRY, MaxKernel),
  OC_SCHEMA_STRING_IN    ("MinKernel",      OC_KERNEL_ADD_ENTRY, MinKernel),
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
  OC_SCHEMA_STRING_IN    ("MaxKernel",      OC_KERNEL_BLOCK_ENTRY, MaxKernel),
  OC_SCHEMA_STRING_IN    ("MinKernel",      OC_KERNEL_BLOCK_ENTRY, MinKernel),
};

STATIC
OC_SCHEMA
mKernelBlockSchema = OC_SCHEMA_DICT (NULL, mKernelBlockSchemaEntry);

STATIC
OC_SCHEMA
mKernelEmulateSchema[] = {
  OC_SCHEMA_DATAF_IN   ("Cpuid1Data",       OC_GLOBAL_CONFIG, Kernel.Emulate.Cpuid1Data),
  OC_SCHEMA_DATAF_IN   ("Cpuid1Mask",       OC_GLOBAL_CONFIG, Kernel.Emulate.Cpuid1Mask),
};

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
  OC_SCHEMA_STRING_IN    ("MaxKernel",      OC_KERNEL_PATCH_ENTRY, MaxKernel),
  OC_SCHEMA_STRING_IN    ("MinKernel",      OC_KERNEL_PATCH_ENTRY, MinKernel),
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
  OC_SCHEMA_BOOLEAN_IN ("AppleCpuPmCfgLock",       OC_GLOBAL_CONFIG, Kernel.Quirks.AppleCpuPmCfgLock),
  OC_SCHEMA_BOOLEAN_IN ("AppleXcpmCfgLock",        OC_GLOBAL_CONFIG, Kernel.Quirks.AppleXcpmCfgLock),
  OC_SCHEMA_BOOLEAN_IN ("AppleXcpmExtraMsrs",      OC_GLOBAL_CONFIG, Kernel.Quirks.AppleXcpmExtraMsrs),
  OC_SCHEMA_BOOLEAN_IN ("AppleXcpmForceBoost",     OC_GLOBAL_CONFIG, Kernel.Quirks.AppleXcpmForceBoost),
  OC_SCHEMA_BOOLEAN_IN ("CustomSMBIOSGuid",        OC_GLOBAL_CONFIG, Kernel.Quirks.CustomSmbiosGuid),
  OC_SCHEMA_BOOLEAN_IN ("DisableIoMapper",         OC_GLOBAL_CONFIG, Kernel.Quirks.DisableIoMapper),
  OC_SCHEMA_BOOLEAN_IN ("DummyPowerManagement",    OC_GLOBAL_CONFIG, Kernel.Quirks.DummyPowerManagement),
  OC_SCHEMA_BOOLEAN_IN ("ExternalDiskIcons",       OC_GLOBAL_CONFIG, Kernel.Quirks.ExternalDiskIcons),
  OC_SCHEMA_BOOLEAN_IN ("IncreasePciBarSize",      OC_GLOBAL_CONFIG, Kernel.Quirks.IncreasePciBarSize),
  OC_SCHEMA_BOOLEAN_IN ("LapicKernelPanic",        OC_GLOBAL_CONFIG, Kernel.Quirks.LapicKernelPanic),
  OC_SCHEMA_BOOLEAN_IN ("PanicNoKextDump",         OC_GLOBAL_CONFIG, Kernel.Quirks.PanicNoKextDump),
  OC_SCHEMA_BOOLEAN_IN ("PowerTimeoutKernelPanic", OC_GLOBAL_CONFIG, Kernel.Quirks.PowerTimeoutKernelPanic),
  OC_SCHEMA_BOOLEAN_IN ("ThirdPartyDrives",        OC_GLOBAL_CONFIG, Kernel.Quirks.ThirdPartyDrives),
  OC_SCHEMA_BOOLEAN_IN ("XhciPortLimit",           OC_GLOBAL_CONFIG, Kernel.Quirks.XhciPortLimit),
};

STATIC
OC_SCHEMA
mKernelConfigurationSchema[] = {
  OC_SCHEMA_ARRAY_IN   ("Add",     OC_GLOBAL_CONFIG, Kernel.Add, &mKernelAddSchema),
  OC_SCHEMA_ARRAY_IN   ("Block",   OC_GLOBAL_CONFIG, Kernel.Block, &mKernelBlockSchema),
  OC_SCHEMA_DICT       ("Emulate", mKernelEmulateSchema),
  OC_SCHEMA_ARRAY_IN   ("Patch",   OC_GLOBAL_CONFIG, Kernel.Patch, &mKernelPatchSchema),
  OC_SCHEMA_DICT       ("Quirks",  mKernelQuirksSchema),
};

//
// Misc configuration support
//

STATIC
OC_SCHEMA
mMiscBlessOverrideSchema = OC_SCHEMA_STRING (NULL);

STATIC
OC_SCHEMA
mMiscConfigurationBootSchema[] = {
  OC_SCHEMA_INTEGER_IN ("ConsoleAttributes",   OC_GLOBAL_CONFIG, Misc.Boot.ConsoleAttributes),
  OC_SCHEMA_STRING_IN  ("HibernateMode",       OC_GLOBAL_CONFIG, Misc.Boot.HibernateMode),
  OC_SCHEMA_BOOLEAN_IN ("HideAuxiliary",       OC_GLOBAL_CONFIG, Misc.Boot.HideAuxiliary),
  OC_SCHEMA_BOOLEAN_IN ("HideSelf",            OC_GLOBAL_CONFIG, Misc.Boot.HideSelf),
  OC_SCHEMA_INTEGER_IN ("PickerAttributes",    OC_GLOBAL_CONFIG, Misc.Boot.PickerAttributes),
  OC_SCHEMA_BOOLEAN_IN ("PickerAudioAssist",   OC_GLOBAL_CONFIG, Misc.Boot.PickerAudioAssist),
  OC_SCHEMA_STRING_IN  ("PickerMode",          OC_GLOBAL_CONFIG, Misc.Boot.PickerMode),
  OC_SCHEMA_BOOLEAN_IN ("PollAppleHotKeys",    OC_GLOBAL_CONFIG, Misc.Boot.PollAppleHotKeys),
  OC_SCHEMA_BOOLEAN_IN ("ShowPicker",          OC_GLOBAL_CONFIG, Misc.Boot.ShowPicker),
  OC_SCHEMA_INTEGER_IN ("TakeoffDelay",        OC_GLOBAL_CONFIG, Misc.Boot.TakeoffDelay),
  OC_SCHEMA_INTEGER_IN ("Timeout",             OC_GLOBAL_CONFIG, Misc.Boot.Timeout),
};

STATIC
OC_SCHEMA
mMiscConfigurationDebugSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("AppleDebug",       OC_GLOBAL_CONFIG, Misc.Debug.AppleDebug),
  OC_SCHEMA_BOOLEAN_IN ("DisableWatchDog",  OC_GLOBAL_CONFIG, Misc.Debug.DisableWatchDog),
  OC_SCHEMA_INTEGER_IN ("DisplayDelay",     OC_GLOBAL_CONFIG, Misc.Debug.DisplayDelay),
  OC_SCHEMA_INTEGER_IN ("DisplayLevel",     OC_GLOBAL_CONFIG, Misc.Debug.DisplayLevel),
  OC_SCHEMA_INTEGER_IN ("Target",           OC_GLOBAL_CONFIG, Misc.Debug.Target)
};

STATIC
OC_SCHEMA
mMiscConfigurationSecuritySchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("AllowNvramReset",      OC_GLOBAL_CONFIG, Misc.Security.AllowNvramReset),
  OC_SCHEMA_BOOLEAN_IN ("AllowSetDefault",      OC_GLOBAL_CONFIG, Misc.Security.AllowSetDefault),
  OC_SCHEMA_BOOLEAN_IN ("AuthRestart",          OC_GLOBAL_CONFIG, Misc.Security.AuthRestart),
  OC_SCHEMA_BOOLEAN_IN ("EnablePassword",       OC_GLOBAL_CONFIG, Misc.Security.EnablePassword),
  OC_SCHEMA_INTEGER_IN ("ExposeSensitiveData",  OC_GLOBAL_CONFIG, Misc.Security.ExposeSensitiveData),
  OC_SCHEMA_INTEGER_IN ("HaltLevel",            OC_GLOBAL_CONFIG, Misc.Security.HaltLevel),
  OC_SCHEMA_DATAF_IN   ("PasswordHash",         OC_GLOBAL_CONFIG, Misc.Security.PasswordHash),
  OC_SCHEMA_DATA_IN    ("PasswordSalt",         OC_GLOBAL_CONFIG, Misc.Security.PasswordSalt),
  OC_SCHEMA_INTEGER_IN ("ScanPolicy",           OC_GLOBAL_CONFIG, Misc.Security.ScanPolicy),
  OC_SCHEMA_STRING_IN  ("Vault",                OC_GLOBAL_CONFIG, Misc.Security.Vault),
};

STATIC
OC_SCHEMA
mMiscToolsSchemaEntry[] = {
  OC_SCHEMA_STRING_IN  ("Arguments", OC_MISC_TOOLS_ENTRY, Arguments),
  OC_SCHEMA_BOOLEAN_IN ("Auxiliary", OC_MISC_TOOLS_ENTRY, Auxiliary),
  OC_SCHEMA_STRING_IN  ("Comment",   OC_MISC_TOOLS_ENTRY, Comment),
  OC_SCHEMA_BOOLEAN_IN ("Enabled",   OC_MISC_TOOLS_ENTRY, Enabled),
  OC_SCHEMA_STRING_IN  ("Name",      OC_MISC_TOOLS_ENTRY, Name),
  OC_SCHEMA_STRING_IN  ("Path",      OC_MISC_TOOLS_ENTRY, Path),
};

STATIC
OC_SCHEMA
mMiscToolsSchema = OC_SCHEMA_DICT (NULL, mMiscToolsSchemaEntry);

STATIC
OC_SCHEMA
mMiscConfigurationSchema[] = {
  OC_SCHEMA_ARRAY_IN   ("BlessOverride",    OC_GLOBAL_CONFIG, Misc.BlessOverride, &mMiscBlessOverrideSchema),
  OC_SCHEMA_DICT       ("Boot",             mMiscConfigurationBootSchema),
  OC_SCHEMA_DICT       ("Debug",            mMiscConfigurationDebugSchema),
  OC_SCHEMA_ARRAY_IN   ("Entries",          OC_GLOBAL_CONFIG, Misc.Entries, &mMiscToolsSchema),
  OC_SCHEMA_DICT       ("Security",         mMiscConfigurationSecuritySchema),
  OC_SCHEMA_ARRAY_IN   ("Tools",            OC_GLOBAL_CONFIG, Misc.Tools, &mMiscToolsSchema),
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
mNvramLegacyEntrySchema = OC_SCHEMA_STRING (NULL);

STATIC
OC_SCHEMA
mNvramLegacySchema = OC_SCHEMA_ARRAY (NULL, &mNvramLegacyEntrySchema);

STATIC
OC_SCHEMA
mNvramConfigurationSchema[] = {
  OC_SCHEMA_MAP_IN     ("Add",             OC_GLOBAL_CONFIG, Nvram.Add, &mNvramAddSchema),
  OC_SCHEMA_MAP_IN     ("Block",           OC_GLOBAL_CONFIG, Nvram.Block, &mNvramBlockSchema),
  OC_SCHEMA_BOOLEAN_IN ("LegacyEnable",    OC_GLOBAL_CONFIG, Nvram.LegacyEnable),
  OC_SCHEMA_BOOLEAN_IN ("LegacyOverwrite", OC_GLOBAL_CONFIG, Nvram.LegacyOverwrite),
  OC_SCHEMA_MAP_IN     ("LegacySchema",    OC_GLOBAL_CONFIG, Nvram.Legacy, &mNvramLegacySchema),
  OC_SCHEMA_BOOLEAN_IN ("WriteFlash",      OC_GLOBAL_CONFIG, Nvram.WriteFlash),
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
  OC_SCHEMA_INTEGER_IN ("DevicePathsSupported", OC_GLOBAL_CONFIG, PlatformInfo.DataHub.DevicePathsSupported),
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
  OC_SCHEMA_BOOLEAN_IN ("AdviseWindows",      OC_GLOBAL_CONFIG, PlatformInfo.Generic.AdviseWindows),
  OC_SCHEMA_STRING_IN  ("MLB",                OC_GLOBAL_CONFIG, PlatformInfo.Generic.Mlb),
  OC_SCHEMA_DATAF_IN   ("ROM",                OC_GLOBAL_CONFIG, PlatformInfo.Generic.Rom),
  OC_SCHEMA_BOOLEAN_IN ("SpoofVendor",        OC_GLOBAL_CONFIG, PlatformInfo.Generic.SpoofVendor),
  OC_SCHEMA_STRING_IN  ("SystemProductName",  OC_GLOBAL_CONFIG, PlatformInfo.Generic.SystemProductName),
  OC_SCHEMA_STRING_IN  ("SystemSerialNumber", OC_GLOBAL_CONFIG, PlatformInfo.Generic.SystemSerialNumber),
  OC_SCHEMA_STRING_IN  ("SystemUUID",         OC_GLOBAL_CONFIG, PlatformInfo.Generic.SystemUuid),
};

STATIC
OC_SCHEMA
mPlatformConfigurationNvramSchema[] = {
  OC_SCHEMA_STRING_IN ("BID",                  OC_GLOBAL_CONFIG, PlatformInfo.Nvram.Bid),
  OC_SCHEMA_DATAF_IN  ("FirmwareFeatures",     OC_GLOBAL_CONFIG, PlatformInfo.Nvram.FirmwareFeatures),
  OC_SCHEMA_DATAF_IN  ("FirmwareFeaturesMask", OC_GLOBAL_CONFIG, PlatformInfo.Nvram.FirmwareFeaturesMask),
  OC_SCHEMA_STRING_IN ("MLB",                  OC_GLOBAL_CONFIG, PlatformInfo.Nvram.Mlb),
  OC_SCHEMA_DATAF_IN  ("ROM",                  OC_GLOBAL_CONFIG, PlatformInfo.Nvram.Rom)
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
  OC_SCHEMA_DATAF_IN   ("SmcVersion",             OC_GLOBAL_CONFIG, PlatformInfo.Smbios.SmcVersion),
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
  OC_SCHEMA_INTEGER_IN ("ExitBootServicesDelay",  OC_GLOBAL_CONFIG, Uefi.Quirks.ExitBootServicesDelay),
  OC_SCHEMA_BOOLEAN_IN ("IgnoreInvalidFlexRatio", OC_GLOBAL_CONFIG, Uefi.Quirks.IgnoreInvalidFlexRatio),
  OC_SCHEMA_BOOLEAN_IN ("ReleaseUsbOwnership",    OC_GLOBAL_CONFIG, Uefi.Quirks.ReleaseUsbOwnership),
  OC_SCHEMA_BOOLEAN_IN ("RequestBootVarFallback", OC_GLOBAL_CONFIG, Uefi.Quirks.RequestBootVarFallback),
  OC_SCHEMA_BOOLEAN_IN ("RequestBootVarRouting",  OC_GLOBAL_CONFIG, Uefi.Quirks.RequestBootVarRouting),
  OC_SCHEMA_BOOLEAN_IN ("UnblockFsConnect",       OC_GLOBAL_CONFIG, Uefi.Quirks.UnblockFsConnect)
};

STATIC
OC_SCHEMA
mUefiProtocolsSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("AppleAudio",              OC_GLOBAL_CONFIG, Uefi.Protocols.AppleAudio),
  OC_SCHEMA_BOOLEAN_IN ("AppleBootPolicy",         OC_GLOBAL_CONFIG, Uefi.Protocols.AppleBootPolicy),
  OC_SCHEMA_BOOLEAN_IN ("AppleDebugLog",           OC_GLOBAL_CONFIG, Uefi.Protocols.AppleDebugLog),
  OC_SCHEMA_BOOLEAN_IN ("AppleEvent",              OC_GLOBAL_CONFIG, Uefi.Protocols.AppleEvent),
  OC_SCHEMA_BOOLEAN_IN ("AppleImageConversion",    OC_GLOBAL_CONFIG, Uefi.Protocols.AppleImageConversion),
  OC_SCHEMA_BOOLEAN_IN ("AppleKeyMap",             OC_GLOBAL_CONFIG, Uefi.Protocols.AppleKeyMap),
  OC_SCHEMA_BOOLEAN_IN ("AppleSmcIo",              OC_GLOBAL_CONFIG, Uefi.Protocols.AppleSmcIo),
  OC_SCHEMA_BOOLEAN_IN ("AppleUserInterfaceTheme", OC_GLOBAL_CONFIG, Uefi.Protocols.AppleUserInterfaceTheme),
  OC_SCHEMA_BOOLEAN_IN ("DataHub",                 OC_GLOBAL_CONFIG, Uefi.Protocols.DataHub),
  OC_SCHEMA_BOOLEAN_IN ("DeviceProperties",        OC_GLOBAL_CONFIG, Uefi.Protocols.DeviceProperties),
  OC_SCHEMA_BOOLEAN_IN ("FirmwareVolume",          OC_GLOBAL_CONFIG, Uefi.Protocols.FirmwareVolume),
  OC_SCHEMA_BOOLEAN_IN ("HashServices",            OC_GLOBAL_CONFIG, Uefi.Protocols.HashServices),
  OC_SCHEMA_BOOLEAN_IN ("OSInfo",                  OC_GLOBAL_CONFIG, Uefi.Protocols.OSInfo),
  OC_SCHEMA_BOOLEAN_IN ("UnicodeCollation",        OC_GLOBAL_CONFIG, Uefi.Protocols.UnicodeCollation)
};

STATIC
OC_SCHEMA
mUefiApfsSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("EnableJumpstart",      OC_GLOBAL_CONFIG, Uefi.Apfs.EnableJumpstart),
  OC_SCHEMA_BOOLEAN_IN ("HideVerbose",          OC_GLOBAL_CONFIG, Uefi.Apfs.HideVerbose),
  OC_SCHEMA_BOOLEAN_IN ("JumpstartHotPlug",     OC_GLOBAL_CONFIG, Uefi.Apfs.JumpstartHotPlug),
  OC_SCHEMA_INTEGER_IN ("MinDate",              OC_GLOBAL_CONFIG, Uefi.Apfs.MinDate),
  OC_SCHEMA_INTEGER_IN ("MinVersion",           OC_GLOBAL_CONFIG, Uefi.Apfs.MinVersion),
};

STATIC
OC_SCHEMA
mUefiAudioSchema[] = {
  OC_SCHEMA_INTEGER_IN ("AudioCodec",         OC_GLOBAL_CONFIG, Uefi.Audio.AudioCodec),
  OC_SCHEMA_STRING_IN  ("AudioDevice",        OC_GLOBAL_CONFIG, Uefi.Audio.AudioDevice),
  OC_SCHEMA_INTEGER_IN ("AudioOut",           OC_GLOBAL_CONFIG, Uefi.Audio.AudioOut),
  OC_SCHEMA_BOOLEAN_IN ("AudioSupport",       OC_GLOBAL_CONFIG, Uefi.Audio.AudioSupport),
  OC_SCHEMA_INTEGER_IN ("MinimumVolume",      OC_GLOBAL_CONFIG, Uefi.Audio.MinimumVolume),
  OC_SCHEMA_BOOLEAN_IN ("PlayChime",          OC_GLOBAL_CONFIG, Uefi.Audio.PlayChime),
  OC_SCHEMA_INTEGER_IN ("VolumeAmplifier",    OC_GLOBAL_CONFIG, Uefi.Audio.VolumeAmplifier),
};

STATIC
OC_SCHEMA
mUefiInputSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("KeyFiltering",       OC_GLOBAL_CONFIG, Uefi.Input.KeyFiltering),
  OC_SCHEMA_INTEGER_IN ("KeyForgetThreshold", OC_GLOBAL_CONFIG, Uefi.Input.KeyForgetThreshold),
  OC_SCHEMA_INTEGER_IN ("KeyMergeThreshold",  OC_GLOBAL_CONFIG, Uefi.Input.KeyMergeThreshold),
  OC_SCHEMA_BOOLEAN_IN ("KeySupport",         OC_GLOBAL_CONFIG, Uefi.Input.KeySupport),
  OC_SCHEMA_STRING_IN  ("KeySupportMode",     OC_GLOBAL_CONFIG, Uefi.Input.KeySupportMode),
  OC_SCHEMA_BOOLEAN_IN ("KeySwap",            OC_GLOBAL_CONFIG, Uefi.Input.KeySwap),
  OC_SCHEMA_BOOLEAN_IN ("PointerSupport",     OC_GLOBAL_CONFIG, Uefi.Input.PointerSupport),
  OC_SCHEMA_STRING_IN  ("PointerSupportMode", OC_GLOBAL_CONFIG, Uefi.Input.PointerSupportMode),
  OC_SCHEMA_INTEGER_IN ("TimerResolution",    OC_GLOBAL_CONFIG, Uefi.Input.TimerResolution)
};

STATIC
OC_SCHEMA
mUefiOutputSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("ClearScreenOnModeSwitch",OC_GLOBAL_CONFIG, Uefi.Output.ClearScreenOnModeSwitch),
  OC_SCHEMA_STRING_IN  ("ConsoleMode",            OC_GLOBAL_CONFIG, Uefi.Output.ConsoleMode),
  OC_SCHEMA_STRING_IN  ("DirectGopCacheMode",     OC_GLOBAL_CONFIG, Uefi.Output.DirectGopCacheMode),
  OC_SCHEMA_BOOLEAN_IN ("DirectGopRendering",     OC_GLOBAL_CONFIG, Uefi.Output.DirectGopRendering),
  OC_SCHEMA_BOOLEAN_IN ("IgnoreTextInGraphics",   OC_GLOBAL_CONFIG, Uefi.Output.IgnoreTextInGraphics),
  OC_SCHEMA_BOOLEAN_IN ("ProvideConsoleGop",      OC_GLOBAL_CONFIG, Uefi.Output.ProvideConsoleGop),
  OC_SCHEMA_BOOLEAN_IN ("ReconnectOnResChange",   OC_GLOBAL_CONFIG, Uefi.Output.ReconnectOnResChange),
  OC_SCHEMA_BOOLEAN_IN ("ReplaceTabWithSpace",    OC_GLOBAL_CONFIG, Uefi.Output.ReplaceTabWithSpace),
  OC_SCHEMA_STRING_IN  ("Resolution",             OC_GLOBAL_CONFIG, Uefi.Output.Resolution),
  OC_SCHEMA_BOOLEAN_IN ("SanitiseClearScreen",    OC_GLOBAL_CONFIG, Uefi.Output.SanitiseClearScreen),
  OC_SCHEMA_STRING_IN  ("TextRenderer",           OC_GLOBAL_CONFIG, Uefi.Output.TextRenderer),
};

STATIC
OC_SCHEMA
mUefiConfigurationSchema[] = {
  OC_SCHEMA_DICT       ("APFS",           mUefiApfsSchema),
  OC_SCHEMA_DICT       ("Audio",          mUefiAudioSchema),
  OC_SCHEMA_BOOLEAN_IN ("ConnectDrivers", OC_GLOBAL_CONFIG, Uefi.ConnectDrivers),
  OC_SCHEMA_ARRAY_IN   ("Drivers",        OC_GLOBAL_CONFIG, Uefi.Drivers, &mUefiDriversSchema),
  OC_SCHEMA_DICT       ("Input",          mUefiInputSchema),
  OC_SCHEMA_DICT       ("Output",         mUefiOutputSchema),
  OC_SCHEMA_DICT       ("Protocols",      mUefiProtocolsSchema),
  OC_SCHEMA_DICT       ("Quirks",         mUefiQuirksSchema)
};

//
// Root configuration
//

STATIC
OC_SCHEMA
mRootConfigurationNodes[] = {
  OC_SCHEMA_DICT    ("ACPI",             mAcpiConfigurationSchema),
  OC_SCHEMA_DICT    ("Booter",           mBooterConfigurationSchema),
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

VOID
OcConfigurationFree (
  IN OUT OC_GLOBAL_CONFIG   *Config
  )
{
  OC_GLOBAL_CONFIG_DESTRUCT (Config, sizeof (*Config));
}

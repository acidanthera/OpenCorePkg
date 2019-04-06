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

OC_MAP_STRUCTORS (OC_DEVICE_PROP_MAP)
OC_STRUCTORS (OC_KERNEL_ADD_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_ADD_ARRAY)
OC_STRUCTORS (OC_KERNEL_BLOCK_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_BLOCK_ARRAY)
OC_STRUCTORS (OC_KERNEL_PATCH_ENTRY, ())
OC_ARRAY_STRUCTORS (OC_KERNEL_PATCH_ARRAY)
OC_STRUCTORS (OC_KERNEL_QUIRKS, ())
OC_STRUCTORS (OC_KERNEL_CONFIG, ())
OC_ARRAY_STRUCTORS (OC_UEFI_DRIVER_ARRAY)
OC_STRUCTORS (OC_UEFI_QUIRKS, ())
OC_STRUCTORS (OC_UEFI_CONFIG, ())
OC_STRUCTORS (OC_GLOBAL_CONFIG, ())

//
// Device properties support
//

STATIC
OC_SCHEMA
mDevicePropertiesEntrySchema = OC_SCHEMA_MDATA (NULL);


STATIC
OC_SCHEMA
mDevicePropertiesSchema = OC_SCHEMA_MAP (NULL, OC_ASSOC, &mDevicePropertiesEntrySchema);


//
// Kernel space configuration support
//

STATIC
OC_SCHEMA
mKernelAddSchema[] = {
  OC_SCHEMA_STRING_IN    ("BundleName",     OC_KERNEL_ADD_ENTRY, BundleName),
  OC_SCHEMA_BOOLEAN_IN   ("Disabled",       OC_KERNEL_ADD_ENTRY, Disabled),
  OC_SCHEMA_STRING_IN    ("ExecutablePath", OC_KERNEL_ADD_ENTRY, ExecutablePath),
  OC_SCHEMA_STRING_IN    ("MatchKernel",    OC_KERNEL_ADD_ENTRY, MatchKernel),
  OC_SCHEMA_STRING_IN    ("PlistPath",      OC_KERNEL_ADD_ENTRY, PlistPath),
};

STATIC
OC_SCHEMA
mKernelBlockSchema[] = {
  OC_SCHEMA_BOOLEAN_IN   ("Disabled",       OC_KERNEL_BLOCK_ENTRY, Disabled),
  OC_SCHEMA_STRING_IN    ("Identifier",     OC_KERNEL_BLOCK_ENTRY, Identifier),
  OC_SCHEMA_STRING_IN    ("MatchKernel",    OC_KERNEL_BLOCK_ENTRY, MatchKernel),
};

STATIC
OC_SCHEMA
mKernelPatchSchema[] = {
  OC_SCHEMA_STRING_IN    ("Base",           OC_KERNEL_PATCH_ENTRY, Base),
  OC_SCHEMA_INTEGER_IN   ("Count",          OC_KERNEL_PATCH_ENTRY, Count),
  OC_SCHEMA_BOOLEAN_IN   ("Disabled",       OC_KERNEL_PATCH_ENTRY, Disabled),
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
mKernelQuirksSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("AppleCpuPmCfgLock", OC_GLOBAL_CONFIG, Kernel.Quirks.AppleCpuPmCfgLock),
  OC_SCHEMA_BOOLEAN_IN ("ThirdPartyTrim",    OC_GLOBAL_CONFIG, Kernel.Quirks.ThirdPartyTrim),
  OC_SCHEMA_BOOLEAN_IN ("XhciPortLimit",     OC_GLOBAL_CONFIG, Kernel.Quirks.XhciPortLimit),
};

STATIC
OC_SCHEMA
mKernelConfigurationSchema[] = {
  OC_SCHEMA_ARRAY_IN   ("Add",    OC_GLOBAL_CONFIG, Kernel.Add, mKernelAddSchema),
  OC_SCHEMA_ARRAY_IN   ("Block",  OC_GLOBAL_CONFIG, Kernel.Block, mKernelBlockSchema),
  OC_SCHEMA_ARRAY_IN   ("Patch",  OC_GLOBAL_CONFIG, Kernel.Patch, mKernelPatchSchema),
  OC_SCHEMA_ARRAY_IN   ("Quirks", OC_GLOBAL_CONFIG, Kernel.Quirks, mKernelQuirksSchema),
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
  OC_SCHEMA_BOOLEAN_IN ("ProvideConsoleGop",      OC_GLOBAL_CONFIG, Uefi.Quirks.ProvideConsoleGop)
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
  OC_SCHEMA_MAP_IN  ("DeviceProperties", OC_GLOBAL_CONFIG, DeviceProperties, &mDevicePropertiesSchema),
  OC_SCHEMA_DICT    ("KernelSpace",      mKernelConfigurationSchema),
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

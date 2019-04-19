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

#ifndef OC_CONFIGURATION_LIB_H
#define OC_CONFIGURATION_LIB_H

#include <Library/DebugLib.h>
#include <Library/OcSerializeLib.h>

/**
  ACPI section
**/

///
/// ACPI added tables.
///
#define OC_ACPI_ADD_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
  OC_DECLARE (OC_ACPI_ADD_ARRAY)

///
/// ACPI table blocks.
///
#define OC_ACPI_BLOCK_ENTRY_FIELDS(_, __) \
  _(BOOLEAN                     , All              ,     , FALSE   , () ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE   , () ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT8                       , OemTableId       , [8] , {0}     , () ) \
  _(UINT32                      , TableLength      ,     , 0       , () ) \
  _(UINT8                       , TableSignature   , [4] , {0}     , () )
  OC_DECLARE (OC_ACPI_BLOCK_ENTRY)

#define OC_ACPI_BLOCK_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_ACPI_BLOCK_ENTRY, _, __)
  OC_DECLARE (OC_ACPI_BLOCK_ARRAY)

///
/// ACPI patches.
///
#define OC_ACPI_PATCH_ENTRY_FIELDS(_, __) \
  _(UINT32                      , Count            ,     , 0                           , ()                   ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                       , ()                   ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_DATA                     , Find             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(UINT32                      , Limit            ,     , 0                           , ()                   ) \
  _(OC_DATA                     , Mask             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA                     , Replace          ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA                     , ReplaceMask      ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(UINT8                       , OemTableId       , [8] , {0}                         , ()                   ) \
  _(UINT32                      , TableLength      ,     , 0                           , ()                   ) \
  _(UINT8                       , TableSignature   , [4] , {0}                         , ()                   ) \
  _(UINT32                      , Skip             ,     , 0                           , ()                   )
  OC_DECLARE (OC_ACPI_PATCH_ENTRY)

#define OC_ACPI_PATCH_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_ACPI_PATCH_ENTRY, _, __)
  OC_DECLARE (OC_ACPI_PATCH_ARRAY)

///
/// ACPI quirks.
///
#define OC_ACPI_QUIRKS_FIELDS(_, __) \
  _(BOOLEAN                     , FadtEnableReset     ,     , FALSE  , ()) \
  _(BOOLEAN                     , IgnoreForWindows    ,     , FALSE  , ()) \
  _(BOOLEAN                     , NormalizeHeaders    ,     , FALSE  , ()) \
  _(BOOLEAN                     , RebaseRegions       ,     , FALSE  , ())
  OC_DECLARE (OC_ACPI_QUIRKS)

#define OC_ACPI_CONFIG_FIELDS(_, __) \
  _(OC_ACPI_ADD_ARRAY         , Add              ,     , OC_CONSTR2 (OC_ACPI_ADD_ARRAY, _, __)     , OC_DESTR (OC_ACPI_ADD_ARRAY)) \
  _(OC_ACPI_BLOCK_ARRAY       , Block            ,     , OC_CONSTR2 (OC_ACPI_BLOCK_ARRAY, _, __)   , OC_DESTR (OC_ACPI_BLOCK_ARRAY)) \
  _(OC_ACPI_PATCH_ARRAY       , Patch            ,     , OC_CONSTR2 (OC_ACPI_PATCH_ARRAY, _, __)   , OC_DESTR (OC_ACPI_PATCH_ARRAY)) \
  _(OC_ACPI_QUIRKS            , Quirks           ,     , OC_CONSTR2 (OC_ACPI_QUIRKS, _, __)        , OC_DESTR (OC_ACPI_QUIRKS))
  OC_DECLARE (OC_ACPI_CONFIG)

/**
  DeviceProperties section
**/

///
/// Device properties is an associative map of devices with their property key value maps.
///
#define OC_DEV_PROP_ADD_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_ASSOC, _, __)
  OC_DECLARE (OC_DEV_PROP_ADD_MAP)

#define OC_DEV_PROP_BLOCK_ENTRY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
  OC_DECLARE (OC_DEV_PROP_BLOCK_ENTRY)

#define OC_DEV_PROP_BLOCK_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_DEV_PROP_BLOCK_ENTRY, _, __)
  OC_DECLARE (OC_DEV_PROP_BLOCK_MAP)

#define OC_DEV_PROP_QUIRKS_FIELDS(_, __) \
  OC_DECLARE (OC_DEV_PROP_QUIRKS)

#define OC_DEV_PROP_CONFIG_FIELDS(_, __) \
  _(OC_DEV_PROP_ADD_MAP       , Add              ,     , OC_CONSTR2 (OC_DEV_PROP_ADD_MAP, _, __)   , OC_DESTR (OC_DEV_PROP_ADD_MAP)) \
  _(OC_DEV_PROP_BLOCK_MAP     , Block            ,     , OC_CONSTR2 (OC_DEV_PROP_BLOCK_MAP, _, __) , OC_DESTR (OC_DEV_PROP_BLOCK_MAP)) \
  _(BOOLEAN                   , ReinstallProtocol,     , FALSE  , ())
  OC_DECLARE (OC_DEV_PROP_CONFIG)

/**
  KernelSpace section
**/

///
/// KernelSpace kext adds.
///
#define OC_KERNEL_ADD_ENTRY_FIELDS(_, __) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                       , ()                   ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MatchKernel      ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , BundlePath       ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , ExecutablePath   ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , PlistPath        ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT8 *                     , ImageData        ,     , NULL                        , OcFreePointer        ) \
  _(UINT32                      , ImageDataSize    ,     , 0                           , ()                   ) \
  _(CHAR8 *                     , PlistData        ,     , NULL                        , OcFreePointer        ) \
  _(UINT32                      , PlistDataSize    ,     , 0                           , ()                   )
  OC_DECLARE (OC_KERNEL_ADD_ENTRY)

#define OC_KERNEL_ADD_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_KERNEL_ADD_ENTRY, _, __)
  OC_DECLARE (OC_KERNEL_ADD_ARRAY)

///
/// KernelSpace kext blocks.
///
#define OC_KERNEL_BLOCK_ENTRY_FIELDS(_, __) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                       , ()                   ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Identifier       ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MatchKernel      ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) )
  OC_DECLARE (OC_KERNEL_BLOCK_ENTRY)

#define OC_KERNEL_BLOCK_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_KERNEL_BLOCK_ENTRY, _, __)
  OC_DECLARE (OC_KERNEL_BLOCK_ARRAY)

///
/// KernelSpace patches.
///
#define OC_KERNEL_PATCH_ENTRY_FIELDS(_, __) \
  _(OC_STRING                   , Base             ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT32                      , Count            ,     , 0                           , ()                   ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                       , ()                   ) \
  _(OC_DATA                     , Find             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_STRING                   , Identifier       ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_DATA                     , Mask             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_STRING                   , MatchKernel      ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_DATA                     , Replace          ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA                     , ReplaceMask      ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(UINT32                      , Limit            ,     , 0                           , ()                   ) \
  _(UINT32                      , Skip             ,     , 0                           , ()                   )
  OC_DECLARE (OC_KERNEL_PATCH_ENTRY)

#define OC_KERNEL_PATCH_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_KERNEL_PATCH_ENTRY, _, __)
  OC_DECLARE (OC_KERNEL_PATCH_ARRAY)

///
/// KernelSpace quirks.
///
#define OC_KERNEL_QUIRKS_FIELDS(_, __) \
  _(BOOLEAN                     , AppleCpuPmCfgLock           ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleXcpmCfgLock            ,     , FALSE  , ()) \
  _(BOOLEAN                     , ExternalDiskIcons           ,     , FALSE  , ()) \
  _(BOOLEAN                     , ThirdPartyTrim              ,     , FALSE  , ()) \
  _(BOOLEAN                     , XhciPortLimit               ,     , FALSE  , ())
  OC_DECLARE (OC_KERNEL_QUIRKS)

#define OC_KERNEL_CONFIG_FIELDS(_, __) \
  _(OC_KERNEL_ADD_ARRAY         , Add              ,     , OC_CONSTR2 (OC_KERNEL_ADD_ARRAY, _, __)     , OC_DESTR (OC_KERNEL_ADD_ARRAY)) \
  _(OC_KERNEL_BLOCK_ARRAY       , Block            ,     , OC_CONSTR2 (OC_KERNEL_BLOCK_ARRAY, _, __)   , OC_DESTR (OC_KERNEL_BLOCK_ARRAY)) \
  _(OC_KERNEL_PATCH_ARRAY       , Patch            ,     , OC_CONSTR2 (OC_KERNEL_PATCH_ARRAY, _, __)   , OC_DESTR (OC_KERNEL_PATCH_ARRAY)) \
  _(OC_KERNEL_QUIRKS            , Quirks           ,     , OC_CONSTR2 (OC_KERNEL_QUIRKS, _, __)        , OC_DESTR (OC_KERNEL_QUIRKS))
  OC_DECLARE (OC_KERNEL_CONFIG)

/**
  Misc section
**/

#define OC_MISC_BOOT_FIELDS(_, __) \
  _(BOOLEAN                     , HideSelf                    ,     , FALSE                       , ())                   \
  _(BOOLEAN                     , ReinstallProtocol           ,     , FALSE                       , ())                   \
  _(BOOLEAN                     , ShowPicker                  ,     , FALSE                       , ())                   \
  _(UINT32                      , Timeout                     ,     , 0                           , ())                   \
  _(OC_STRING                   , Resolution                  ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , ConsoleMode                 ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING))
  OC_DECLARE (OC_MISC_BOOT)

#define OC_MISC_DEBUG_FIELDS(_, __) \
  _(UINT32                      , Delay                       ,     , 0            , ()) \
  _(UINT64                      , DisplayLevel                ,     , 0            , ()) \
  _(BOOLEAN                     , ExposeBootPath              ,     , FALSE        , ()) \
  _(UINT32                      , Target                      ,     , 0            , ())
  OC_DECLARE (OC_MISC_DEBUG)

#define OC_MISC_SECURITY_FIELDS(_, __) \
  _(BOOLEAN                     , RequireVault                ,     , TRUE         , ()) \
  _(BOOLEAN                     , RequireSignature            ,     , TRUE         , ()) \
  _(UINT64                      , HaltLevel                   ,     , 0x80000000   , ())
  OC_DECLARE (OC_MISC_SECURITY)

#define OC_MISC_CONFIG_FIELDS(_, __) \
  _(OC_MISC_BOOT               , Boot            ,     , OC_CONSTR2 (OC_MISC_BOOT, _, __)         , OC_DESTR (OC_MISC_BOOT)) \
  _(OC_MISC_DEBUG              , Debug           ,     , OC_CONSTR2 (OC_MISC_DEBUG, _, __)        , OC_DESTR (OC_MISC_DEBUG)) \
  _(OC_MISC_SECURITY           , Security        ,     , OC_CONSTR2 (OC_MISC_SECURITY, _, __)     , OC_DESTR (OC_MISC_SECURITY))
  OC_DECLARE (OC_MISC_CONFIG)

/**
  NVRAM section
**/

///
/// NVRAM values is an associative map of GUIDS with their property key value maps.
///
#define OC_NVRAM_ADD_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_ASSOC, _, __)
  OC_DECLARE (OC_NVRAM_ADD_MAP)

#define OC_NVRAM_BLOCK_ENTRY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
  OC_DECLARE (OC_NVRAM_BLOCK_ENTRY)

#define OC_NVRAM_BLOCK_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_NVRAM_BLOCK_ENTRY, _, __)
  OC_DECLARE (OC_NVRAM_BLOCK_MAP)

#define OC_NVRAM_CONFIG_FIELDS(_, __) \
  _(OC_NVRAM_ADD_MAP           , Add               ,     , OC_CONSTR2 (OC_NVRAM_ADD_MAP, _, __)        , OC_DESTR (OC_NVRAM_ADD_MAP)) \
  _(OC_NVRAM_BLOCK_MAP         , Block             ,     , OC_CONSTR2 (OC_NVRAM_BLOCK_MAP, _, __)      , OC_DESTR (OC_NVRAM_BLOCK_MAP))
  /* _(OC_NVRAM_QUIRKS         , Quirks            ,     , OC_CONSTR2 (OC_NVRAM_QUIRKS, _, __)         , OC_DESTR (OC_NVRAM_QUIRKS)) */
  OC_DECLARE (OC_NVRAM_CONFIG)

/**
  Uefi section
**/

///
/// Drivers is a sorted array of strings containing driver paths.
///
#define OC_UEFI_DRIVER_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
  OC_DECLARE (OC_UEFI_DRIVER_ARRAY)

///
/// Quirks is a set of hacks for different firmwares.
///
#define OC_UEFI_QUIRKS_FIELDS(_, __) \
  _(BOOLEAN                     , DisableWatchDog             ,     , FALSE  , ()) \
  _(BOOLEAN                     , IgnoreInvalidFlexRatio      ,     , FALSE  , ()) \
  _(BOOLEAN                     , ReleaseUsbOwnership         ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProvideConsoleControl       ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProvideConsoleGop           ,     , FALSE  , ())
  OC_DECLARE (OC_UEFI_QUIRKS)

/**
  Platform information configuration
**/

#define OC_PLATFORM_GENERIC_CONFIG_FIELDS(_, __) \
  _(OC_STRING                   , SystemProductName  ,     , OC_STRING_CONSTR ("MacPro6,1", _, __)        , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemSerialNumber ,     , OC_STRING_CONSTR ("OPENCORE_SN1", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemUuid         ,     , OC_STRING_CONSTR ("", _, __)                 , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Mlb                ,     , OC_STRING_CONSTR ("OPENCORE_MLB_SN11", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT8                       , Rom                , [6] , {0}                                          , () )
  OC_DECLARE (OC_PLATFORM_GENERIC_CONFIG)

#define OC_PLATFORM_DATA_HUB_CONFIG_FIELDS(_, __) \
  _(OC_STRING                   , PlatformName        ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemProductName   ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemSerialNumber  ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemUuid          ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , BoardProduct        ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(UINT8                       , BoardRevision       , [1] , {0}                              , () )                   \
  _(UINT64                      , StartupPowerEvents  ,     , 0                                , () )                   \
  _(UINT64                      , InitialTSC          ,     , 0                                , () )                   \
  _(UINT64                      , FSBFrequency        ,     , 0                                , () )                   \
  _(UINT64                      , ARTFrequency        ,     , 0                                , () )                   \
  _(UINT8                       , DevicePathsSupported, [1] , {0}                              , () )                   \
  _(UINT8                       , SmcRevision         , [6] , {0}                              , () )                   \
  _(UINT8                       , SmcBranch           , [8] , {0}                              , () )                   \
  _(UINT8                       , SmcPlatform         , [8] , {0}                              , () )
  OC_DECLARE (OC_PLATFORM_DATA_HUB_CONFIG)

#define OC_PLATFORM_NVRAM_CONFIG_FIELDS(_, __) \
  _(OC_STRING                   , Bid                   ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Mlb                   ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(UINT8                       , Rom                   , [6] , {0}                              , ()                   ) \
  _(UINT64                      , FirmwareFeatures      ,     , 0                                , ()                   ) \
  _(UINT64                      , FirmwareFeaturesMask  ,     , 0                                , ()                   )
  OC_DECLARE (OC_PLATFORM_NVRAM_CONFIG)

#define OC_PLATFORM_SMBIOS_CONFIG_FIELDS(_, __) \
  _(OC_STRING                    , BIOSVendor            ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , BIOSVersion           ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , BIOSReleaseDate       ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , SystemManufacturer    ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , SystemProductName     ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , SystemVersion         ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , SystemSerialNumber    ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , SystemUuid            ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , SystemSKUNumber       ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , SystemFamily          ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , BoardManufacturer     ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , BoardProduct          ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , BoardVersion          ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , BoardSerialNumber     ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , BoardAssetTag         ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(UINT8                        , BoardType             ,  , 0                                , ()                   ) \
  _(OC_STRING                    , BoardLocationInChassis,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , ChassisManufacturer   ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(UINT8                        , ChassisType           ,  , 0                                , ()                   ) \
  _(OC_STRING                    , ChassisVersion        ,  , OC_STRING_CONSTR ("", _, __)     , ()                   ) \
  _(OC_STRING                    , ChassisSerialNumber   ,  , OC_STRING_CONSTR ("", _, __)     , ()                   ) \
  _(OC_STRING                    , ChassisAssetTag       ,  , OC_STRING_CONSTR ("", _, __)     , ()                   ) \
  _(UINT32                       , PlatformFeature       ,  , 0                                , ()                   ) \
  _(UINT64                       , FirmwareFeatures      ,  , 0                                , ()                   ) \
  _(UINT64                       , FirmwareFeaturesMask  ,  , 0                                , ()                   ) \
  _(UINT16                       , ProcessorType         ,  , 0                                , ()                   ) \
  _(UINT8                        , MemoryFormFactor      ,  , 0                                , ()                   )
  OC_DECLARE (OC_PLATFORM_SMBIOS_CONFIG)

#define OC_PLATFORM_CONFIG_FIELDS(_, __) \
  _(BOOLEAN                     , Automatic        ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , UpdateDataHub    ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , UpdateNvram      ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , UpdateSmbios     ,     , FALSE                                           , ()) \
  _(OC_STRING                   , UpdateSmbiosMode ,     , OC_STRING_CONSTR ("Auto", _, __)                , OC_DESTR (OC_STRING) ) \
  _(OC_PLATFORM_GENERIC_CONFIG  , Generic          ,     , OC_CONSTR2 (OC_PLATFORM_GENERIC_CONFIG, _, __)  , OC_DESTR (OC_PLATFORM_GENERIC_CONFIG)) \
  _(OC_PLATFORM_DATA_HUB_CONFIG , DataHub          ,     , OC_CONSTR2 (OC_PLATFORM_DATA_HUB_CONFIG, _, __) , OC_DESTR (OC_PLATFORM_DATA_HUB_CONFIG)) \
  _(OC_PLATFORM_NVRAM_CONFIG    , Nvram            ,     , OC_CONSTR2 (OC_PLATFORM_NVRAM_CONFIG, _, __)    , OC_DESTR (OC_PLATFORM_NVRAM_CONFIG)) \
  _(OC_PLATFORM_SMBIOS_CONFIG   , Smbios           ,     , OC_CONSTR2 (OC_PLATFORM_SMBIOS_CONFIG, _, __)   , OC_DESTR (OC_PLATFORM_SMBIOS_CONFIG))
  OC_DECLARE (OC_PLATFORM_CONFIG)

///
/// Uefi contains firmware tweaks and extra drivers.
///
#define OC_UEFI_CONFIG_FIELDS(_, __) \
  _(BOOLEAN                     , ConnectDrivers   ,     , FALSE                                    , ()) \
  _(OC_UEFI_DRIVER_ARRAY        , Drivers          ,     , OC_CONSTR2 (OC_UEFI_DRIVER_ARRAY, _, __) , OC_DESTR (OC_UEFI_DRIVER_ARRAY)) \
  _(OC_UEFI_QUIRKS              , Quirks           ,     , OC_CONSTR2 (OC_UEFI_QUIRKS, _, __)       , OC_DESTR (OC_UEFI_QUIRKS))
  OC_DECLARE (OC_UEFI_CONFIG)

/**
  Root configuration
**/

#define OC_GLOBAL_CONFIG_FIELDS(_, __) \
  _(OC_ACPI_CONFIG              , Acpi              ,     , OC_CONSTR1 (OC_ACPI_CONFIG, _, __)      , OC_DESTR (OC_ACPI_CONFIG)) \
  _(OC_DEV_PROP_CONFIG          , DeviceProperties  ,     , OC_CONSTR1 (OC_DEV_PROP_CONFIG, _, __)  , OC_DESTR (OC_DEV_PROP_CONFIG)) \
  _(OC_KERNEL_CONFIG            , Kernel            ,     , OC_CONSTR1 (OC_KERNEL_CONFIG, _, __)    , OC_DESTR (OC_KERNEL_CONFIG)) \
  _(OC_MISC_CONFIG              , Misc              ,     , OC_CONSTR1 (OC_MISC_CONFIG, _, __)      , OC_DESTR (OC_MISC_CONFIG)) \
  _(OC_NVRAM_CONFIG             , Nvram             ,     , OC_CONSTR1 (OC_NVRAM_CONFIG, _, __)     , OC_DESTR (OC_NVRAM_CONFIG)) \
  _(OC_PLATFORM_CONFIG          , PlatformInfo      ,     , OC_CONSTR1 (OC_PLATFORM_CONFIG, _, __)  , OC_DESTR (OC_PLATFORM_CONFIG)) \
  _(OC_UEFI_CONFIG              , Uefi              ,     , OC_CONSTR1 (OC_UEFI_CONFIG, _, __)      , OC_DESTR (OC_UEFI_CONFIG))
  OC_DECLARE (OC_GLOBAL_CONFIG)

/**
  Initialize configuration with plist data.

  @param[out]  Config   Configuration structure.
  @param[in]   Buffer   Configuration buffer in plist format.
  @param[in]   Size     Configuration buffer size.

  @retval  EFI_SUCCESS on success
**/
EFI_STATUS
OcConfigurationInit (
  OUT OC_GLOBAL_CONFIG   *Config,
  IN  VOID               *Buffer,
  IN  UINT32             Size
  );

/**
  Free configuration structure.

  @param[in,out]  Config   Configuration structure.
**/
VOID
OcConfigurationFree (
  IN OUT OC_GLOBAL_CONFIG   *Config
  );

#endif // OC_CONFIGURATION_LIB_H

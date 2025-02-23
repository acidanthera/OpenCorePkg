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
#include <Library/OcBootManagementLib.h>

/**
  ACPI section
**/

///
/// ACPI added tables.
///
#define OC_ACPI_ADD_ENTRY_FIELDS(_, __) \
  _(BOOLEAN                     , Enabled          ,     , FALSE   , () ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Path             ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) )
OC_DECLARE (OC_ACPI_ADD_ENTRY)

#define OC_ACPI_ADD_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_ACPI_ADD_ENTRY, _, __)
OC_DECLARE (OC_ACPI_ADD_ARRAY)

///
/// ACPI table deletion.
///
#define OC_ACPI_DELETE_ENTRY_FIELDS(_, __) \
  _(BOOLEAN                     , All              ,     , FALSE   , () ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE   , () ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT8                       , OemTableId       , [8] , {0}     , () ) \
  _(UINT32                      , TableLength      ,     , 0       , () ) \
  _(UINT8                       , TableSignature   , [4] , {0}     , () )
OC_DECLARE (OC_ACPI_DELETE_ENTRY)

#define OC_ACPI_DELETE_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_ACPI_DELETE_ENTRY, _, __)
OC_DECLARE (OC_ACPI_DELETE_ARRAY)

///
/// ACPI patches.
///
#define OC_ACPI_PATCH_ENTRY_FIELDS(_, __) \
  _(UINT32                      , Count            ,     , 0                           , ()                   ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                       , ()                   ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_DATA                     , Find             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_STRING                   , Base             ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT32                      , BaseSkip         ,     , 0                           , ()                   ) \
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
  _(BOOLEAN                     , NormalizeHeaders    ,     , FALSE  , ()) \
  _(BOOLEAN                     , RebaseRegions       ,     , FALSE  , ()) \
  _(BOOLEAN                     , ResetHwSig          ,     , FALSE  , ()) \
  _(BOOLEAN                     , ResetLogoStatus     ,     , FALSE  , ()) \
  _(BOOLEAN                     , SyncTableIds        ,     , FALSE  , ())
OC_DECLARE (OC_ACPI_QUIRKS)

#define OC_ACPI_CONFIG_FIELDS(_, __) \
  _(OC_ACPI_ADD_ARRAY         , Add              ,     , OC_CONSTR2 (OC_ACPI_ADD_ARRAY, _, __)     , OC_DESTR (OC_ACPI_ADD_ARRAY)) \
  _(OC_ACPI_DELETE_ARRAY      , Delete           ,     , OC_CONSTR2 (OC_ACPI_DELETE_ARRAY, _, __)  , OC_DESTR (OC_ACPI_DELETE_ARRAY)) \
  _(OC_ACPI_PATCH_ARRAY       , Patch            ,     , OC_CONSTR2 (OC_ACPI_PATCH_ARRAY, _, __)   , OC_DESTR (OC_ACPI_PATCH_ARRAY)) \
  _(OC_ACPI_QUIRKS            , Quirks           ,     , OC_CONSTR2 (OC_ACPI_QUIRKS, _, __)        , OC_DESTR (OC_ACPI_QUIRKS))
OC_DECLARE (OC_ACPI_CONFIG)

/**
  Apple bootloader section
**/

#define OC_BOOTER_WL_ENTRY_FIELDS(_, __) \
  _(UINT64                      , Address          ,     , 0       , () ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE   , () ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) )
OC_DECLARE (OC_BOOTER_WL_ENTRY)

#define OC_BOOTER_WL_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_BOOTER_WL_ENTRY, _, __)
OC_DECLARE (OC_BOOTER_WL_ARRAY)

///
/// Bootloader patches.
///
#define OC_BOOTER_PATCH_ENTRY_FIELDS(_, __) \
  _(OC_STRING                   , Arch             ,     , OC_STRING_CONSTR ("Any", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT32                      , Count            ,     , 0                           , ()                   ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                       , ()                   ) \
  _(OC_DATA                     , Find             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_STRING                   , Identifier       ,     , OC_STRING_CONSTR ("Any", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_DATA                     , Mask             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA                     , Replace          ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA                     , ReplaceMask      ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(UINT32                      , Limit            ,     , 0                           , ()                   ) \
  _(UINT32                      , Skip             ,     , 0                           , ()                   )
OC_DECLARE (OC_BOOTER_PATCH_ENTRY)

#define OC_BOOTER_PATCH_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_BOOTER_PATCH_ENTRY, _, __)
OC_DECLARE (OC_BOOTER_PATCH_ARRAY)

///
/// Apple bootloader quirks.
///
#define OC_BOOTER_QUIRKS_FIELDS(_, __) \
  _(BOOLEAN                     , AllowRelocationBlock      ,     , FALSE  , ()) \
  _(BOOLEAN                     , AvoidRuntimeDefrag        ,     , FALSE  , ()) \
  _(BOOLEAN                     , ClearTaskSwitchBit        ,     , FALSE  , ()) \
  _(BOOLEAN                     , DevirtualiseMmio          ,     , FALSE  , ()) \
  _(BOOLEAN                     , DisableSingleUser         ,     , FALSE  , ()) \
  _(BOOLEAN                     , DisableVariableWrite      ,     , FALSE  , ()) \
  _(BOOLEAN                     , DiscardHibernateMap       ,     , FALSE  , ()) \
  _(BOOLEAN                     , EnableSafeModeSlide       ,     , FALSE  , ()) \
  _(BOOLEAN                     , EnableWriteUnprotector    ,     , FALSE  , ()) \
  _(BOOLEAN                     , FixupAppleEfiImages       ,     , FALSE  , ()) \
  _(BOOLEAN                     , ForceBooterSignature      ,     , FALSE  , ()) \
  _(BOOLEAN                     , ForceExitBootServices     ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProtectMemoryRegions      ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProtectSecureBoot         ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProtectUefiServices       ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProvideCustomSlide        ,     , FALSE  , ()) \
  _(UINT8                       , ProvideMaxSlide           ,     , 0      , ()) \
  _(BOOLEAN                     , RebuildAppleMemoryMap     ,     , FALSE  , ()) \
  _(INT8                        , ResizeAppleGpuBars        ,     , -1     , ()) \
  _(BOOLEAN                     , SetupVirtualMap           ,     , FALSE  , ()) \
  _(BOOLEAN                     , SignalAppleOS             ,     , FALSE  , ()) \
  _(BOOLEAN                     , SyncRuntimePermissions    ,     , FALSE  , ())
OC_DECLARE (OC_BOOTER_QUIRKS)

///
/// Apple bootloader section.
///
#define OC_BOOTER_CONFIG_FIELDS(_, __) \
  _(OC_BOOTER_WL_ARRAY          , MmioWhitelist    ,     , OC_CONSTR2 (OC_BOOTER_WL_ARRAY, _, __)      , OC_DESTR (OC_BOOTER_WL_ARRAY)) \
  _(OC_BOOTER_PATCH_ARRAY       , Patch            ,     , OC_CONSTR2 (OC_BOOTER_PATCH_ARRAY, _, __)   , OC_DESTR (OC_BOOTER_PATCH_ARRAY)) \
  _(OC_BOOTER_QUIRKS            , Quirks           ,     , OC_CONSTR2 (OC_BOOTER_QUIRKS, _, __)        , OC_DESTR (OC_BOOTER_QUIRKS))
OC_DECLARE (OC_BOOTER_CONFIG)

/**
  DeviceProperties section
**/

///
/// Device properties is an associative map of devices with their property key value maps.
///
#define OC_DEV_PROP_ADD_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_ASSOC, _, __)
OC_DECLARE (OC_DEV_PROP_ADD_MAP)

#define OC_DEV_PROP_DELETE_ENTRY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
OC_DECLARE (OC_DEV_PROP_DELETE_ENTRY)

#define OC_DEV_PROP_DELETE_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_DEV_PROP_DELETE_ENTRY, _, __)
OC_DECLARE (OC_DEV_PROP_DELETE_MAP)

#define OC_DEV_PROP_CONFIG_FIELDS(_, __) \
  _(OC_DEV_PROP_ADD_MAP       , Add              ,     , OC_CONSTR2 (OC_DEV_PROP_ADD_MAP, _, __)    , OC_DESTR (OC_DEV_PROP_ADD_MAP)) \
  _(OC_DEV_PROP_DELETE_MAP    , Delete           ,     , OC_CONSTR2 (OC_DEV_PROP_DELETE_MAP, _, __) , OC_DESTR (OC_DEV_PROP_DELETE_MAP))
OC_DECLARE (OC_DEV_PROP_CONFIG)

/**
  KernelSpace section
**/

///
/// KernelSpace kext adds.
///
#define OC_KERNEL_ADD_ENTRY_FIELDS(_, __) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                            ,  ()                   ) \
  _(OC_STRING                   , Arch             ,     , OC_STRING_CONSTR ("Any", _, __)  ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __)     ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MaxKernel        ,     , OC_STRING_CONSTR ("", _, __)     ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MinKernel        ,     , OC_STRING_CONSTR ("", _, __)     ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Identifier       ,     , OC_STRING_CONSTR ("", _, __)     ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , BundlePath       ,     , OC_STRING_CONSTR ("", _, __)     ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , ExecutablePath   ,     , OC_STRING_CONSTR ("", _, __)     ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , PlistPath        ,     , OC_STRING_CONSTR ("", _, __)     ,  OC_DESTR (OC_STRING) ) \
  _(UINT8 *                     , ImageData        ,     , NULL                             ,  OcFreePointer        ) \
  _(UINT32                      , ImageDataSize    ,     , 0                                ,  ()                   ) \
  _(CHAR8 *                     , PlistData        ,     , NULL                             ,  OcFreePointer        ) \
  _(UINT32                      , PlistDataSize    ,     , 0                                ,  ()                   )
OC_DECLARE (OC_KERNEL_ADD_ENTRY)

#define OC_KERNEL_ADD_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_KERNEL_ADD_ENTRY, _, __)
OC_DECLARE (OC_KERNEL_ADD_ARRAY)

///
/// KernelSpace kext blocks.
///
#define OC_KERNEL_BLOCK_ENTRY_FIELDS(_, __) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                                ,  ()                   ) \
  _(OC_STRING                   , Arch             ,     , OC_STRING_CONSTR ("Any", _, __)      ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __)         ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Identifier       ,     , OC_STRING_CONSTR ("", _, __)         ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MaxKernel        ,     , OC_STRING_CONSTR ("", _, __)         ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MinKernel        ,     , OC_STRING_CONSTR ("", _, __)         ,  OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Strategy         ,     , OC_STRING_CONSTR ("Disable", _, __)  ,  OC_DESTR (OC_STRING) )
OC_DECLARE (OC_KERNEL_BLOCK_ENTRY)

#define OC_KERNEL_BLOCK_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_KERNEL_BLOCK_ENTRY, _, __)
OC_DECLARE (OC_KERNEL_BLOCK_ARRAY)

///
/// Kernel emulation preferences.
///
#define OC_KERNEL_EMULATE_FIELDS(_, __) \
  _(UINT32                      , Cpuid1Data          , [4] , {0}                         , () ) \
  _(UINT32                      , Cpuid1Mask          , [4] , {0}                         , () ) \
  _(BOOLEAN                     , DummyPowerManagement,     , FALSE                       , () ) \
  _(OC_STRING                   , MaxKernel           ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MinKernel           ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) )
OC_DECLARE (OC_KERNEL_EMULATE)

///
/// KernelSpace forced loaded kexts.
///
#define OC_KERNEL_FORCE_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_KERNEL_ADD_ENTRY, _, __)
OC_DECLARE (OC_KERNEL_FORCE_ARRAY)

///
/// KernelSpace patches.
///
#define OC_KERNEL_PATCH_ENTRY_FIELDS(_, __) \
  _(OC_STRING                   , Arch             ,     , OC_STRING_CONSTR ("Any", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Base             ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT32                      , Count            ,     , 0                           , ()                   ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE                       , ()                   ) \
  _(OC_DATA                     , Find             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_STRING                   , Identifier       ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_DATA                     , Mask             ,     , OC_EDATA_CONSTR (_, __)     , OC_DESTR (OC_DATA)   ) \
  _(OC_STRING                   , MaxKernel        ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , MinKernel        ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
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
  _(INT64                       , SetApfsTrimTimeout          ,     , -1     , ()) \
  _(BOOLEAN                     , AppleCpuPmCfgLock           ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleXcpmCfgLock            ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleXcpmExtraMsrs          ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleXcpmForceBoost         ,     , FALSE  , ()) \
  _(BOOLEAN                     , CustomPciSerialDevice       ,     , FALSE  , ()) \
  _(BOOLEAN                     , CustomSmbiosGuid            ,     , FALSE  , ()) \
  _(BOOLEAN                     , DisableIoMapper             ,     , FALSE  , ()) \
  _(BOOLEAN                     , DisableIoMapperMapping      ,     , FALSE  , ()) \
  _(BOOLEAN                     , DisableLinkeditJettison     ,     , FALSE  , ()) \
  _(BOOLEAN                     , DisableRtcChecksum          ,     , FALSE  , ()) \
  _(BOOLEAN                     , ExtendBTFeatureFlags        ,     , FALSE  , ()) \
  _(BOOLEAN                     , ExternalDiskIcons           ,     , FALSE  , ()) \
  _(BOOLEAN                     , ForceAquantiaEthernet       ,     , FALSE  , ()) \
  _(BOOLEAN                     , ForceSecureBootScheme       ,     , FALSE  , ()) \
  _(BOOLEAN                     , IncreasePciBarSize          ,     , FALSE  , ()) \
  _(BOOLEAN                     , LapicKernelPanic            ,     , FALSE  , ()) \
  _(BOOLEAN                     , LegacyCommpage              ,     , FALSE  , ()) \
  _(BOOLEAN                     , PanicNoKextDump             ,     , FALSE  , ()) \
  _(BOOLEAN                     , PowerTimeoutKernelPanic     ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProvideCurrentCpuInfo       ,     , FALSE  , ()) \
  _(BOOLEAN                     , ThirdPartyDrives            ,     , FALSE  , ()) \
  _(BOOLEAN                     , XhciPortLimit               ,     , FALSE  , ())
OC_DECLARE (OC_KERNEL_QUIRKS)

///
/// KernelSpace operation scheme.
///
#define OC_KERNEL_SCHEME_FIELDS(_, __) \
  _(OC_STRING                   , KernelArch       ,     , OC_STRING_CONSTR ("Auto", _, __), OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , KernelCache      ,     , OC_STRING_CONSTR ("Auto", _, __), OC_DESTR (OC_STRING)) \
  _(BOOLEAN                     , CustomKernel     ,     , FALSE  , ()) \
  _(BOOLEAN                     , FuzzyMatch       ,     , FALSE  , ())
OC_DECLARE (OC_KERNEL_SCHEME)

#define OC_KERNEL_CONFIG_FIELDS(_, __) \
  _(OC_KERNEL_ADD_ARRAY         , Add              ,     , OC_CONSTR2 (OC_KERNEL_ADD_ARRAY, _, __)     , OC_DESTR (OC_KERNEL_ADD_ARRAY)) \
  _(OC_KERNEL_BLOCK_ARRAY       , Block            ,     , OC_CONSTR2 (OC_KERNEL_BLOCK_ARRAY, _, __)   , OC_DESTR (OC_KERNEL_BLOCK_ARRAY)) \
  _(OC_KERNEL_EMULATE           , Emulate          ,     , OC_CONSTR2 (OC_KERNEL_EMULATE, _, __)       , OC_DESTR (OC_KERNEL_EMULATE)) \
  _(OC_KERNEL_FORCE_ARRAY       , Force            ,     , OC_CONSTR2 (OC_KERNEL_FORCE_ARRAY, _, __)   , OC_DESTR (OC_KERNEL_FORCE_ARRAY)) \
  _(OC_KERNEL_PATCH_ARRAY       , Patch            ,     , OC_CONSTR2 (OC_KERNEL_PATCH_ARRAY, _, __)   , OC_DESTR (OC_KERNEL_PATCH_ARRAY)) \
  _(OC_KERNEL_QUIRKS            , Quirks           ,     , OC_CONSTR2 (OC_KERNEL_QUIRKS, _, __)        , OC_DESTR (OC_KERNEL_QUIRKS)) \
  _(OC_KERNEL_SCHEME            , Scheme           ,     , OC_CONSTR2 (OC_KERNEL_SCHEME, _, __)        , OC_DESTR (OC_KERNEL_SCHEME))
OC_DECLARE (OC_KERNEL_CONFIG)

/**
  Misc section
**/

#define OC_MISC_BLESS_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
OC_DECLARE (OC_MISC_BLESS_ARRAY)

#define OC_MISC_BOOT_FIELDS(_, __) \
  _(OC_STRING                   , PickerMode                  ,     , OC_STRING_CONSTR ("Builtin", _, __) , OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , HibernateMode               ,     , OC_STRING_CONSTR ("None", _, __)    , OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , InstanceIdentifier          ,     , OC_STRING_CONSTR ("", _, __)        , OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , LauncherOption              ,     , OC_STRING_CONSTR ("Disabled", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , LauncherPath                ,     , OC_STRING_CONSTR ("Default", _, __) , OC_DESTR (OC_STRING) ) \
  _(UINT32                      , ConsoleAttributes           ,     , 0                                   , ())                   \
  _(UINT32                      , PickerAttributes            ,     , 0                                   , ())                   \
  _(OC_STRING                   , PickerVariant               ,     , OC_STRING_CONSTR ("Auto", _, __)    , OC_DESTR (OC_STRING)) \
  _(UINT32                      , TakeoffDelay                ,     , 0                                   , ())                   \
  _(UINT32                      , Timeout                     ,     , 0                                   , ())                   \
  _(BOOLEAN                     , PickerAudioAssist           ,     , FALSE                               , ())                   \
  _(BOOLEAN                     , HibernateSkipsPicker        ,     , FALSE                               , ())                   \
  _(BOOLEAN                     , HideAuxiliary               ,     , FALSE                               , ())                   \
  _(BOOLEAN                     , PollAppleHotKeys            ,     , FALSE                               , ())                   \
  _(BOOLEAN                     , ShowPicker                  ,     , FALSE                               , ())
OC_DECLARE (OC_MISC_BOOT)

#define OC_MISC_DEBUG_FIELDS(_, __) \
  _(UINT64                      , DisplayLevel                ,     , 0            , ()) \
  _(UINT32                      , DisplayDelay                ,     , 0            , ()) \
  _(UINT32                      , Target                      ,     , 0            , ()) \
  _(BOOLEAN                     , AppleDebug                  ,     , FALSE        , ()) \
  _(BOOLEAN                     , ApplePanic                  ,     , FALSE        , ()) \
  _(BOOLEAN                     , DisableWatchDog             ,     , FALSE        , ()) \
  _(BOOLEAN                     , SysReport                   ,     , FALSE        , ()) \
  _(OC_STRING                   , LogModules                  ,     , OC_STRING_CONSTR ("*", _, __) , OC_DESTR (OC_STRING))
OC_DECLARE (OC_MISC_DEBUG)

#define OCS_EXPOSE_BOOT_PATH    1U
#define OCS_EXPOSE_VERSION_VAR  2U
#define OCS_EXPOSE_VERSION_UI   4U
#define OCS_EXPOSE_OEM_INFO     8U
#define OCS_EXPOSE_VERSION      (OCS_EXPOSE_VERSION_VAR | OCS_EXPOSE_VERSION_UI)
#define OCS_EXPOSE_ALL_BITS     (\
  OCS_EXPOSE_BOOT_PATH  | OCS_EXPOSE_VERSION_VAR | \
  OCS_EXPOSE_VERSION_UI | OCS_EXPOSE_OEM_INFO)

typedef enum {
  OcsVaultOptional = 0,
  OcsVaultBasic    = 1,
  OcsVaultSecure   = 2,
} OCS_VAULT_MODE;

#define OC_MISC_SECURITY_FIELDS(_, __) \
  _(OC_STRING                   , Vault                       ,      , OC_STRING_CONSTR ("Secure", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , DmgLoading                  ,      , OC_STRING_CONSTR ("Signed", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT32                      , ScanPolicy                  ,      , OC_SCAN_DEFAULT_POLICY  , ()) \
  _(UINT32                      , ExposeSensitiveData         ,      , OCS_EXPOSE_VERSION      , ()) \
  _(BOOLEAN                     , AllowSetDefault             ,      , FALSE                   , ()) \
  _(BOOLEAN                     , AuthRestart                 ,      , FALSE                   , ()) \
  _(BOOLEAN                     , BlacklistAppleUpdate        ,      , FALSE                   , ()) \
  _(BOOLEAN                     , EnablePassword              ,      , FALSE                   , ()) \
  _(UINT8                       , PasswordHash                , [64] , {0}                     , ()) \
  _(OC_DATA                     , PasswordSalt                ,      , OC_EDATA_CONSTR (_, __) , OC_DESTR (OC_DATA)) \
  _(OC_STRING                   , SecureBootModel             ,      , OC_STRING_CONSTR ("Default", _, __), OC_DESTR (OC_STRING) ) \
  _(UINT64                      , ApECID                      ,      , 0                       , ()) \
  _(UINT64                      , HaltLevel                   ,      , 0x80000000              , ())
OC_DECLARE (OC_MISC_SECURITY)

#define OC_MISC_TOOLS_ENTRY_FIELDS(_, __) \
  _(OC_STRING                   , Arguments       ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment         ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Flavour         ,     , OC_STRING_CONSTR ("Auto", _, __), OC_DESTR (OC_STRING) ) \
  _(BOOLEAN                     , Auxiliary       ,     , FALSE                       , ()                   ) \
  _(BOOLEAN                     , Enabled         ,     , FALSE                       , ()                   ) \
  _(BOOLEAN                     , FullNvramAccess ,     , FALSE                       , ()                   ) \
  _(BOOLEAN                     , RealPath        ,     , FALSE                       , ()                   ) \
  _(BOOLEAN                     , TextMode        ,     , FALSE                       , ()                   ) \
  _(OC_STRING                   , Name            ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Path            ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) )
OC_DECLARE (OC_MISC_TOOLS_ENTRY)

#define OC_MISC_TOOLS_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_MISC_TOOLS_ENTRY, _, __)
OC_DECLARE (OC_MISC_TOOLS_ARRAY)

///
/// Reference:
/// https://github.com/acidanthera/bugtracker/issues/1954#issuecomment-1084220743
///
#define OC_SERIAL_PCI_DEVICE_INFO_MAX_SIZE  41U

///
/// Reference:
/// https://github.com/acidanthera/audk/blob/master/MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
/// https://github.com/acidanthera/audk/blob/master/MdeModulePkg/MdeModulePkg.dec
///
#define OC_MISC_SERIAL_CUSTOM_FIELDS(_, __) \
  _(UINT32                      , BaudRate                 ,      , 115200                           , ()) \
  _(UINT32                      , ClockRate                ,      , 1843200                          , ()) \
  _(BOOLEAN                     , DetectCable              ,      , FALSE                            , ()) \
  _(UINT32                      , ExtendedTxFifoSize       ,      , 64                               , ()) \
  _(UINT8                       , FifoControl              ,      , 7                                , ()) \
  _(UINT8                       , LineControl              ,      , 3                                , ()) \
  _(OC_DATA                     , PciDeviceInfo            ,      , OC_DATA_CONSTR ({0xFF}, _, __)   , OC_DESTR (OC_DATA)) \
  _(UINT8                       , RegisterAccessWidth      ,      , 8                                , ()) \
  _(UINT64                      , RegisterBase             ,      , 0x03F8                           , ()) \
  _(UINT32                      , RegisterStride           ,      , 1                                , ()) \
  _(BOOLEAN                     , UseHardwareFlowControl   ,      , FALSE                            , ()) \
  _(BOOLEAN                     , UseMmio                  ,      , FALSE                            , ())
OC_DECLARE (OC_MISC_SERIAL_CUSTOM)

#define OC_MISC_SERIAL_FIELDS(_, __) \
  _(OC_MISC_SERIAL_CUSTOM              , Custom    ,     , OC_CONSTR3 (OC_MISC_SERIAL_CUSTOM, _, __)         , OC_DESTR (OC_MISC_SERIAL_CUSTOM)) \
  _(BOOLEAN                            , Init      ,     , FALSE                                             , ()) \
  _(BOOLEAN                            , Override  ,     , FALSE                                             , ())
OC_DECLARE (OC_MISC_SERIAL)

#define OC_MISC_CONFIG_FIELDS(_, __) \
  _(OC_MISC_BLESS_ARRAY        , BlessOverride   ,     , OC_CONSTR2 (OC_MISC_BLESS_ARRAY, _, __)  , OC_DESTR (OC_MISC_BLESS_ARRAY)) \
  _(OC_MISC_BOOT               , Boot            ,     , OC_CONSTR2 (OC_MISC_BOOT, _, __)         , OC_DESTR (OC_MISC_BOOT)) \
  _(OC_MISC_DEBUG              , Debug           ,     , OC_CONSTR2 (OC_MISC_DEBUG, _, __)        , OC_DESTR (OC_MISC_DEBUG)) \
  _(OC_MISC_SECURITY           , Security        ,     , OC_CONSTR2 (OC_MISC_SECURITY, _, __)     , OC_DESTR (OC_MISC_SECURITY)) \
  _(OC_MISC_SERIAL             , Serial          ,     , OC_CONSTR2 (OC_MISC_SERIAL, _, __)       , OC_DESTR (OC_MISC_SERIAL)) \
  _(OC_MISC_TOOLS_ARRAY        , Entries         ,     , OC_CONSTR2 (OC_MISC_TOOLS_ARRAY, _, __)  , OC_DESTR (OC_MISC_TOOLS_ARRAY)) \
  _(OC_MISC_TOOLS_ARRAY        , Tools           ,     , OC_CONSTR2 (OC_MISC_TOOLS_ARRAY, _, __)  , OC_DESTR (OC_MISC_TOOLS_ARRAY))
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

#define OC_NVRAM_DELETE_ENTRY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
OC_DECLARE (OC_NVRAM_DELETE_ENTRY)

#define OC_NVRAM_DELETE_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_NVRAM_DELETE_ENTRY, _, __)
OC_DECLARE (OC_NVRAM_DELETE_MAP)

#define OC_NVRAM_LEGACY_ENTRY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
OC_DECLARE (OC_NVRAM_LEGACY_ENTRY)

#define OC_NVRAM_LEGACY_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_NVRAM_LEGACY_ENTRY, _, __)
OC_DECLARE (OC_NVRAM_LEGACY_MAP)

#define OC_NVRAM_CONFIG_FIELDS(_, __) \
  _(OC_NVRAM_ADD_MAP           , Add               ,     , OC_CONSTR2 (OC_NVRAM_ADD_MAP, _, __)        , OC_DESTR (OC_NVRAM_ADD_MAP)) \
  _(OC_NVRAM_DELETE_MAP        , Delete            ,     , OC_CONSTR2 (OC_NVRAM_DELETE_MAP, _, __)     , OC_DESTR (OC_NVRAM_DELETE_MAP)) \
  _(OC_NVRAM_LEGACY_MAP        , Legacy            ,     , OC_CONSTR2 (OC_NVRAM_LEGACY_MAP, _, __)     , OC_DESTR (OC_NVRAM_LEGACY_MAP)) \
  _(BOOLEAN                    , LegacyOverwrite   ,     , FALSE                                       , () ) \
  _(BOOLEAN                    , WriteFlash        ,     , FALSE                                       , () )
OC_DECLARE (OC_NVRAM_CONFIG)

/**
  Platform information configuration
**/

#define OC_PLATFORM_GENERIC_CONFIG_FIELDS(_, __) \
  _(OC_STRING                   , SystemProductName  ,     , OC_STRING_CONSTR ("", _, __)                 , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemSerialNumber ,     , OC_STRING_CONSTR ("", _, __)                 , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemUuid         ,     , OC_STRING_CONSTR ("", _, __)                 , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Mlb                ,     , OC_STRING_CONSTR ("", _, __)                 , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemMemoryStatus ,     , OC_STRING_CONSTR ("Auto", _, __)             , OC_DESTR (OC_STRING) ) \
  _(UINT16                      , ProcessorType      ,     , 0                                            , () )                   \
  _(UINT8                       , Rom                , [6] , {0}                                          , () )                   \
  _(BOOLEAN                     , SpoofVendor        ,     , FALSE                                        , () )                   \
  _(BOOLEAN                     , AdviseFeatures     ,     , FALSE                                        , () )                   \
  _(BOOLEAN                     , MaxBIOSVersion     ,     , FALSE                                        , () )
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
  _(UINT32                      , DevicePathsSupported,     , 0                                , () )                   \
  _(UINT8                       , SmcRevision         , [6] , {0}                              , () )                   \
  _(UINT8                       , SmcBranch           , [8] , {0}                              , () )                   \
  _(UINT8                       , SmcPlatform         , [8] , {0}                              , () )
OC_DECLARE (OC_PLATFORM_DATA_HUB_CONFIG)

#define OC_PLATFORM_MEMORY_DEVICE_ENTRY_FIELDS(_, __) \
  _(UINT32                      , Size                ,     , 0                                  , () )                   \
  _(UINT16                      , Speed               ,     , 0                                  , () )                   \
  _(OC_STRING                   , DeviceLocator       ,     , OC_STRING_CONSTR ("Unknown", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , BankLocator         ,     , OC_STRING_CONSTR ("Unknown", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Manufacturer        ,     , OC_STRING_CONSTR ("Unknown", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SerialNumber        ,     , OC_STRING_CONSTR ("Unknown", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , AssetTag            ,     , OC_STRING_CONSTR ("Unknown", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , PartNumber          ,     , OC_STRING_CONSTR ("Unknown", _, __), OC_DESTR (OC_STRING) )
OC_DECLARE (OC_PLATFORM_MEMORY_DEVICE_ENTRY)

#define OC_PLATFORM_MEMORY_DEVICES_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_PLATFORM_MEMORY_DEVICE_ENTRY, _, __)
OC_DECLARE (OC_PLATFORM_MEMORY_DEVICES_ARRAY)

#define OC_PLATFORM_MEMORY_CONFIG_FIELDS(_, __) \
  _(UINT8                           , FormFactor     ,     , 0x2                                                 , () ) \
  _(UINT8                           , Type           ,     , 0x2                                                 , () ) \
  _(UINT16                          , TypeDetail     ,     , 0x4                                                 , () ) \
  _(UINT16                          , TotalWidth     ,     , 0xFFFF                                              , () ) \
  _(UINT16                          , DataWidth      ,     , 0xFFFF                                              , () ) \
  _(UINT8                           , ErrorCorrection,     , 0x3                                                 , () ) \
  _(UINT64                          , MaxCapacity    ,     , 0                                                   , () ) \
  _(OC_PLATFORM_MEMORY_DEVICES_ARRAY, Devices        ,     , OC_CONSTR3 (OC_PLATFORM_MEMORY_DEVICES_ARRAY, _, __), OC_DESTR (OC_PLATFORM_MEMORY_DEVICES_ARRAY))
OC_DECLARE (OC_PLATFORM_MEMORY_CONFIG)

#define OC_PLATFORM_NVRAM_CONFIG_FIELDS(_, __) \
  _(OC_STRING                   , Bid                   ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Mlb                   ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemSerialNumber    ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , SystemUuid            ,     , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
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
  _(OC_STRING                    , ChassisVersion        ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , ChassisSerialNumber   ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                    , ChassisAssetTag       ,  , OC_STRING_CONSTR ("", _, __)     , OC_DESTR (OC_STRING) ) \
  _(UINT32                       , PlatformFeature       ,  , 0xFFFFFFFFU                      , ()                   ) \
  _(UINT64                       , FirmwareFeatures      ,  , 0                                , ()                   ) \
  _(UINT64                       , FirmwareFeaturesMask  ,  , 0                                , ()                   ) \
  _(UINT8                        , SmcVersion            , [16] , {0}                          , ()                   ) \
  _(UINT16                       , ProcessorType         ,  , 0                                , ()                   )
OC_DECLARE (OC_PLATFORM_SMBIOS_CONFIG)

#define OC_PLATFORM_CONFIG_FIELDS(_, __) \
  _(BOOLEAN                     , Automatic          ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , CustomMemory       ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , UpdateDataHub      ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , UpdateNvram        ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , UpdateSmbios       ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , UseRawUuidEncoding ,     , FALSE                                           , ()) \
  _(OC_STRING                   , UpdateSmbiosMode   ,     , OC_STRING_CONSTR ("Create", _, __)              , OC_DESTR (OC_STRING) ) \
  _(OC_PLATFORM_GENERIC_CONFIG  , Generic            ,     , OC_CONSTR2 (OC_PLATFORM_GENERIC_CONFIG, _, __)  , OC_DESTR (OC_PLATFORM_GENERIC_CONFIG)) \
  _(OC_PLATFORM_DATA_HUB_CONFIG , DataHub            ,     , OC_CONSTR2 (OC_PLATFORM_DATA_HUB_CONFIG, _, __) , OC_DESTR (OC_PLATFORM_DATA_HUB_CONFIG)) \
  _(OC_PLATFORM_MEMORY_CONFIG   , Memory             ,     , OC_CONSTR2 (OC_PLATFORM_MEMORY_CONFIG, _, __)   , OC_DESTR (OC_PLATFORM_MEMORY_CONFIG)) \
  _(OC_PLATFORM_NVRAM_CONFIG    , Nvram              ,     , OC_CONSTR2 (OC_PLATFORM_NVRAM_CONFIG, _, __)    , OC_DESTR (OC_PLATFORM_NVRAM_CONFIG)) \
  _(OC_PLATFORM_SMBIOS_CONFIG   , Smbios             ,     , OC_CONSTR2 (OC_PLATFORM_SMBIOS_CONFIG, _, __)   , OC_DESTR (OC_PLATFORM_SMBIOS_CONFIG))
OC_DECLARE (OC_PLATFORM_CONFIG)

/**
  Uefi section
**/

///
/// Array of driver names to unload.
///
#define OC_UEFI_UNLOAD_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_STRING, _, __)
OC_DECLARE (OC_UEFI_UNLOAD_ARRAY)

///
/// Drivers is an ordered array of drivers to load.
///
#define OC_UEFI_DRIVER_ENTRY_FIELDS(_, __) \
  _(OC_STRING                   , Arguments          ,     , OC_STRING_CONSTR ("", _, __)                    , OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment            ,     , OC_STRING_CONSTR ("", _, __)                    , OC_DESTR (OC_STRING) ) \
  _(BOOLEAN                     , Enabled            ,     , FALSE                                           , ()) \
  _(BOOLEAN                     , LoadEarly          ,     , FALSE                                           , ()) \
  _(OC_STRING                   , Path               ,     , OC_STRING_CONSTR ("", _, __)                    , OC_DESTR (OC_STRING) )
OC_DECLARE (OC_UEFI_DRIVER_ENTRY)

#define OC_UEFI_DRIVER_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_UEFI_DRIVER_ENTRY, _, __)
OC_DECLARE (OC_UEFI_DRIVER_ARRAY)

///
/// APFS is a set of options for APFS file system support.
///
#define OC_UEFI_APFS_FIELDS(_, __) \
  _(UINT64                      , MinVersion         ,     , 0                             , ()) \
  _(UINT32                      , MinDate            ,     , 0                             , ()) \
  _(BOOLEAN                     , EnableJumpstart    ,     , FALSE                         , ()) \
  _(BOOLEAN                     , GlobalConnect      ,     , FALSE                         , ()) \
  _(BOOLEAN                     , HideVerbose        ,     , FALSE                         , ()) \
  _(BOOLEAN                     , JumpstartHotPlug   ,     , FALSE                         , ())
OC_DECLARE (OC_UEFI_APFS)

///
/// AppleInput is a set of options to configure OpenCore's reverse engingeered then customised implementation of the AppleEvent protocol.
///
#define OC_UEFI_APPLEINPUT_FIELDS(_, __) \
  _(OC_STRING                   , AppleEvent                    ,     , OC_STRING_CONSTR ("Auto", _, __)  , OC_DESTR (OC_STRING) ) \
  _(BOOLEAN                     , CustomDelays                  ,     , FALSE                             , ()) \
  _(UINT16                      , KeyInitialDelay               ,     , 50                                , ()) \
  _(UINT16                      , KeySubsequentDelay            ,     , 5                                 , ()) \
  _(BOOLEAN                     , GraphicsInputMirroring        ,     , FALSE                             , ()) \
  _(UINT32                      , PointerPollMin                ,     , 0                                 , ()) \
  _(UINT32                      , PointerPollMax                ,     , 0                                 , ()) \
  _(UINT32                      , PointerPollMask               ,     , ((UINT32) (-1))                   , ()) \
  _(UINT16                      , PointerSpeedDiv               ,     , 1                                 , ()) \
  _(UINT16                      , PointerSpeedMul               ,     , 1                                 , ()) \
  _(UINT16                      , PointerDwellClickTimeout      ,     , 0                                 , ()) \
  _(UINT16                      , PointerDwellDoubleClickTimeout,     , 0                                 , ()) \
  _(UINT16                      , PointerDwellRadius            ,     , 0                                 , ())
OC_DECLARE (OC_UEFI_APPLEINPUT)

///
/// Audio is a set of options for sound configuration.
///
#define OC_UEFI_AUDIO_FIELDS(_, __) \
  _(OC_STRING                   , AudioDevice        ,     , OC_STRING_CONSTR ("", _, __)      , OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , PlayChime          ,     , OC_STRING_CONSTR ("Auto", _, __)  , OC_DESTR (OC_STRING)) \
  _(UINT32                      , SetupDelay         ,     , 0                                 , ()) \
  _(BOOLEAN                     , AudioSupport       ,     , FALSE                             , ()) \
  _(UINT64                      , AudioOutMask       ,     , MAX_UINT64                        , ()) \
  _(UINT8                       , AudioCodec         ,     , 0                                 , ()) \
  _(INT8                        , MaximumGain        ,     , -15                               , ()) \
  _(INT8                        , MinimumAssistGain  ,     , -30                               , ()) \
  _(INT8                        , MinimumAudibleGain ,     , -128                              , ()) \
  _(BOOLEAN                     , ResetTrafficClass  ,     , FALSE                             , ()) \
  _(BOOLEAN                     , DisconnectHda      ,     , FALSE                             , ())
OC_DECLARE (OC_UEFI_AUDIO)

///
/// Input is a set of options to support advanced input.
///
#define OC_UEFI_INPUT_FIELDS(_, __) \
  _(OC_STRING                   , KeySupportMode     ,     , OC_STRING_CONSTR ("Auto", _, __)  , OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , PointerSupportMode ,     , OC_STRING_CONSTR ("", _, __)      , OC_DESTR (OC_STRING)) \
  _(UINT32                      , TimerResolution    ,     , 0                                 , ()) \
  _(UINT8                       , KeyForgetThreshold ,     , 0                                 , ()) \
  _(BOOLEAN                     , KeySupport         ,     , FALSE                             , ()) \
  _(BOOLEAN                     , KeyFiltering       ,     , FALSE                             , ()) \
  _(BOOLEAN                     , KeySwap            ,     , FALSE                             , ()) \
  _(BOOLEAN                     , PointerSupport     ,     , FALSE                             , ())
OC_DECLARE (OC_UEFI_INPUT)

///
/// Output is a set of options to support advanced output.
///
#define OC_UEFI_OUTPUT_FIELDS(_, __) \
  _(OC_STRING                   , ConsoleMode                 ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , ConsoleFont                 ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , Resolution                  ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , InitialMode                 ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , TextRenderer                ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING)) \
  _(OC_STRING                   , GopPassThrough              ,     , OC_STRING_CONSTR ("Disabled", _, __), OC_DESTR (OC_STRING)) \
  _(BOOLEAN                     , IgnoreTextInGraphics        ,     , FALSE  , ()) \
  _(BOOLEAN                     , ClearScreenOnModeSwitch     ,     , FALSE  , ()) \
  _(BOOLEAN                     , ProvideConsoleGop           ,     , FALSE  , ()) \
  _(BOOLEAN                     , ReplaceTabWithSpace         ,     , FALSE  , ()) \
  _(BOOLEAN                     , ReconnectOnResChange        ,     , FALSE  , ()) \
  _(BOOLEAN                     , SanitiseClearScreen         ,     , FALSE  , ()) \
  _(INT8                        , UIScale                     ,     , -1     , ()) \
  _(BOOLEAN                     , UgaPassThrough              ,     , FALSE  , ()) \
  _(BOOLEAN                     , DirectGopRendering          ,     , FALSE  , ()) \
  _(BOOLEAN                     , ForceResolution             ,     , FALSE  , ()) \
  _(BOOLEAN                     , GopBurstMode                ,     , FALSE  , ()) \
  _(BOOLEAN                     , ReconnectGraphicsOnConnect  ,     , FALSE  , ())
OC_DECLARE (OC_UEFI_OUTPUT)

///
/// Prefer own protocol implementation for these protocols.
///
#define OC_UEFI_PROTOCOL_OVERRIDES_FIELDS(_, __) \
  _(BOOLEAN                     , AppleAudio                  ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleBootPolicy             ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleDebugLog               ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleEg2Info                ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleFramebufferInfo        ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleImageConversion        ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleImg4Verification       ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleKeyMap                 ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleRtcRam                 ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleSecureBoot             ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleSmcIo                  ,     , FALSE  , ()) \
  _(BOOLEAN                     , AppleUserInterfaceTheme     ,     , FALSE  , ()) \
  _(BOOLEAN                     , DataHub                     ,     , FALSE  , ()) \
  _(BOOLEAN                     , DeviceProperties            ,     , FALSE  , ()) \
  _(BOOLEAN                     , FirmwareVolume              ,     , FALSE  , ()) \
  _(BOOLEAN                     , HashServices                ,     , FALSE  , ()) \
  _(BOOLEAN                     , OSInfo                      ,     , FALSE  , ()) \
  _(BOOLEAN                     , PciIo                       ,     , FALSE  , ()) \
  _(BOOLEAN                     , UnicodeCollation            ,     , FALSE  , ())
OC_DECLARE (OC_UEFI_PROTOCOL_OVERRIDES)

///
/// Quirks is a set of hacks for different types of firmware.
///
#define OC_UEFI_QUIRKS_FIELDS(_, __) \
  _(UINT32                      , ExitBootServicesDelay       ,     , 0      , ()) \
  _(UINT32                      , TscSyncTimeout              ,     , 0      , ()) \
  _(BOOLEAN                     , ActivateHpetSupport         ,     , FALSE  , ()) \
  _(BOOLEAN                     , DisableSecurityPolicy       ,     , FALSE  , ()) \
  _(BOOLEAN                     , EnableVectorAcceleration    ,     , FALSE  , ()) \
  _(BOOLEAN                     , EnableVmx                   ,     , FALSE  , ()) \
  _(BOOLEAN                     , ForgeUefiSupport            ,     , FALSE  , ()) \
  _(BOOLEAN                     , IgnoreInvalidFlexRatio      ,     , FALSE  , ()) \
  _(INT8                        , ResizeGpuBars               ,     , -1     , ()) \
  _(BOOLEAN                     , ResizeUsePciRbIo            ,     , FALSE  , ()) \
  _(BOOLEAN                     , ReleaseUsbOwnership         ,     , FALSE  , ()) \
  _(BOOLEAN                     , ReloadOptionRoms            ,     , FALSE  , ()) \
  _(BOOLEAN                     , RequestBootVarRouting       ,     , FALSE  , ()) \
  _(BOOLEAN                     , ShimRetainProtocol          ,     , FALSE  , ()) \
  _(BOOLEAN                     , UnblockFsConnect            ,     , FALSE  , ()) \
  _(BOOLEAN                     , ForceOcWriteFlash           ,     , FALSE  , ())
OC_DECLARE (OC_UEFI_QUIRKS)

///
/// Reserved memory entries adds.
///
#define OC_UEFI_RSVD_ENTRY_FIELDS(_, __) \
  _(UINT64                      , Address          ,     , 0       , () ) \
  _(UINT64                      , Size             ,     , 0       , () ) \
  _(BOOLEAN                     , Enabled          ,     , FALSE   , () ) \
  _(OC_STRING                   , Type             ,     , OC_STRING_CONSTR ("Reserved", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING                   , Comment          ,     , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) )
OC_DECLARE (OC_UEFI_RSVD_ENTRY)

#define OC_UEFI_RSVD_ARRAY_FIELDS(_, __) \
  OC_ARRAY (OC_UEFI_RSVD_ENTRY, _, __)
OC_DECLARE (OC_UEFI_RSVD_ARRAY)

///
/// Uefi contains firmware tweaks and extra drivers.
///
#define OC_UEFI_CONFIG_FIELDS(_, __) \
  _(BOOLEAN                     , ConnectDrivers    ,     , FALSE                                          , ()) \
  _(OC_UEFI_APFS                , Apfs              ,     , OC_CONSTR2 (OC_UEFI_APFS, _, __)               , OC_DESTR (OC_UEFI_APFS)) \
  _(OC_UEFI_APPLEINPUT          , AppleInput        ,     , OC_CONSTR2 (OC_UEFI_APPLEINPUT, _, __)         , OC_DESTR (OC_UEFI_APPLEINPUT)) \
  _(OC_UEFI_AUDIO               , Audio             ,     , OC_CONSTR2 (OC_UEFI_AUDIO, _, __)              , OC_DESTR (OC_UEFI_AUDIO)) \
  _(OC_UEFI_DRIVER_ARRAY        , Drivers           ,     , OC_CONSTR2 (OC_UEFI_DRIVER_ARRAY, _, __)       , OC_DESTR (OC_UEFI_DRIVER_ARRAY)) \
  _(OC_UEFI_INPUT               , Input             ,     , OC_CONSTR2 (OC_UEFI_INPUT, _, __)              , OC_DESTR (OC_UEFI_INPUT)) \
  _(OC_UEFI_OUTPUT              , Output            ,     , OC_CONSTR2 (OC_UEFI_OUTPUT, _, __)             , OC_DESTR (OC_UEFI_OUTPUT)) \
  _(OC_UEFI_PROTOCOL_OVERRIDES  , ProtocolOverrides ,     , OC_CONSTR2 (OC_UEFI_PROTOCOL_OVERRIDES, _, __) , OC_DESTR (OC_UEFI_PROTOCOL_OVERRIDES)) \
  _(OC_UEFI_QUIRKS              , Quirks            ,     , OC_CONSTR2 (OC_UEFI_QUIRKS, _, __)             , OC_DESTR (OC_UEFI_QUIRKS)) \
  _(OC_UEFI_RSVD_ARRAY          , ReservedMemory    ,     , OC_CONSTR2 (OC_UEFI_RSVD_ARRAY, _, __)         , OC_DESTR (OC_UEFI_RSVD_ARRAY)) \
  _(OC_UEFI_UNLOAD_ARRAY        , Unload            ,     , OC_CONSTR2 (OC_UEFI_UNLOAD_ARRAY, _, __)       , OC_DESTR (OC_UEFI_UNLOAD_ARRAY))
OC_DECLARE (OC_UEFI_CONFIG)

/**
  Root configuration
**/

#define OC_GLOBAL_CONFIG_FIELDS(_, __) \
  _(OC_ACPI_CONFIG              , Acpi              ,     , OC_CONSTR1 (OC_ACPI_CONFIG, _, __)      , OC_DESTR (OC_ACPI_CONFIG)) \
  _(OC_BOOTER_CONFIG            , Booter            ,     , OC_CONSTR1 (OC_BOOTER_CONFIG, _, __)    , OC_DESTR (OC_BOOTER_CONFIG)) \
  _(OC_DEV_PROP_CONFIG          , DeviceProperties  ,     , OC_CONSTR1 (OC_DEV_PROP_CONFIG, _, __)  , OC_DESTR (OC_DEV_PROP_CONFIG)) \
  _(OC_KERNEL_CONFIG            , Kernel            ,     , OC_CONSTR1 (OC_KERNEL_CONFIG, _, __)    , OC_DESTR (OC_KERNEL_CONFIG)) \
  _(OC_MISC_CONFIG              , Misc              ,     , OC_CONSTR1 (OC_MISC_CONFIG, _, __)      , OC_DESTR (OC_MISC_CONFIG)) \
  _(OC_NVRAM_CONFIG             , Nvram             ,     , OC_CONSTR1 (OC_NVRAM_CONFIG, _, __)     , OC_DESTR (OC_NVRAM_CONFIG)) \
  _(OC_PLATFORM_CONFIG          , PlatformInfo      ,     , OC_CONSTR1 (OC_PLATFORM_CONFIG, _, __)  , OC_DESTR (OC_PLATFORM_CONFIG)) \
  _(OC_UEFI_CONFIG              , Uefi              ,     , OC_CONSTR1 (OC_UEFI_CONFIG, _, __)      , OC_DESTR (OC_UEFI_CONFIG))
OC_DECLARE (OC_GLOBAL_CONFIG)

/**
  Initialize configuration with plist data.

  @param[out]     Config      Configuration structure.
  @param[in]      Buffer      Configuration buffer in plist format.
  @param[in]      Size        Configuration buffer size.
  @param[in,out]  ErrorCount  Errors detected duing initialisation. Optional.

  @retval  EFI_SUCCESS on success
**/
EFI_STATUS
OcConfigurationInit (
  OUT  OC_GLOBAL_CONFIG  *Config,
  IN       VOID          *Buffer,
  IN       UINT32        Size,
  IN  OUT  UINT32        *ErrorCount  OPTIONAL
  );

/**
  Free configuration structure.

  @param[in,out]  Config   Configuration structure.
**/
VOID
OcConfigurationFree (
  IN OUT OC_GLOBAL_CONFIG  *Config
  );

#endif // OC_CONFIGURATION_LIB_H

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

#include <Library/OcSerializeLib.h>

/**
  DeviceProperties section
**/

///
/// Device properties is an associative map of devices with their property key value maps.
///
#define OC_DEVICE_PROP_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_ASSOC, _, __)
  OC_DECLARE (OC_DEVICE_PROP_MAP)

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
/// Uefi contains firmware tweaks and extra drivers.
///
#define OC_UEFI_CONFIG_FIELDS(_, __) \
  _(OC_UEFI_DRIVER_ARRAY        , Drivers          ,     , OC_CONSTR2 (OC_UEFI_DRIVER_ARRAY, _, __) , OC_DESTR (OC_UEFI_DRIVER_ARRAY))
  OC_DECLARE (OC_UEFI_CONFIG)

/**
  Root configuration
**/

#define OC_GLOBAL_CONFIG_FIELDS(_, __) \
  _(OC_DEVICE_PROP_MAP          , DeviceProperties  ,     , OC_CONSTR1 (OC_DEVICE_PROP_MAP, _, __)  , OC_DESTR (OC_DEVICE_PROP_MAP)) \
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

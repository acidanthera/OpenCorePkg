/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_USER_UTILITIES_OCVALIDATE_H
#define OC_USER_UTILITIES_OCVALIDATE_H

#include <Library/OcConfigurationLib.h>
#include <Library/OcMainLib.h>

/**
  OpenCore Configuration checker.
**/
typedef
UINT32
(*CONFIG_CHECK) (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration ACPI Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in ACPI Section.
**/
UINT32
CheckACPI (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration Booter Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in Booter Section.
**/
UINT32
CheckBooter (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration DeviceProperties Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in DeviceProperties.
**/
UINT32
CheckDeviceProperties (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration Kernel Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in Kernel Section.
**/
UINT32
CheckKernel (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration Misc Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in Misc Section.
**/
UINT32
CheckMisc (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration NVRAM Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in NVRAM Section.
**/
UINT32
CheckNvram (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration PlatformInfo Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in PlatformInfo Section.
**/
UINT32
CheckPlatformInfo (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration UEFI Section.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected in UEFI Section.
**/
UINT32
CheckUEFI (
  IN  OC_GLOBAL_CONFIG  *Config
  );

/**
  Validate OpenCore Configuration overall, by calling each checker above.

  @param[in]  Config   Configuration structure.

  @return     Number of errors detected overall.
**/
UINT32
CheckConfig (
  IN  OC_GLOBAL_CONFIG  *Config
  );

#endif // OC_USER_UTILITIES_OCVALIDATE_H

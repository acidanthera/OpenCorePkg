/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OPEN_CORE_H
#define OPEN_CORE_H

#include <Library/OcConfigurationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcStorageLib.h>

#define OPEN_CORE_VERSION          L"0.1"

#define OPEN_CORE_IMAGE_PATH       L"EFI\\OC\\OpenCore.efi"

#define OPEN_CORE_ROOT_PATH        L"EFI\\OC\\"

#define OPEN_CORE_CONFIG_PATH      L"config.plist"

#define OPEN_CORE_ACPI_PATH        L"ACPI\\Custom\\"

#define OPEN_CORE_UEFI_DRIVER_PATH L"Drivers\\"

#define OPEN_CORE_KEXT_PATH        L"Kexts\\"

#define OPEN_CORE_NVRAM_ATTR       (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

/**
  Load ACPI compatibility support like custom tables.

  @param[in]  Storage   OpenCore storage.
  @param[in]  Config    OpenCore configuration.
**/
VOID
OcLoadAcpiSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  );

/**
  Load device properties compatibility support.

  @param[in]  Config    OpenCore configuration.
**/
VOID
OcLoadDevPropsSupport (
  IN OC_GLOBAL_CONFIG    *Config
  );

/**
  Load Kernel compatibility support like kexts.

  @param[in]  Storage   OpenCore storage.
  @param[in]  Config    OpenCore configuration.
**/
VOID
OcLoadKernelSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  );

/**
  Cleanup Kernel compatibility support on failure.
**/
VOID
OcUnloadKernelSupport (
  VOID
  );

/**
  Load NVRAM compatibility support.

  @param[in]  Config    OpenCore configuration.
**/
VOID
OcLoadNvramSupport (
  IN OC_GLOBAL_CONFIG    *Config
  );

/**
  Load platform compatibility support like DataHub or SMBIOS.

  @param[in]  Config    OpenCore configuration.
  @param[in]  CpuInfo   CPU information.
**/
VOID
OcLoadPlatformSupport (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  );

/**
  Load UEFI compatibility support like drivers.

  @param[in]  Storage   OpenCore storage.
  @param[in]  Config    OpenCore configuration.
  @param[in]  CpuInfo   CPU information.
**/
VOID
OcLoadUefiSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  );

#endif // OPEN_CORE_H

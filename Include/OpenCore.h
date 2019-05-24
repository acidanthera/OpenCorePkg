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
#include <Library/OcCryptoLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcStorageLib.h>

#include <Protocol/OcBootstrap.h>

/**
  OpenCore version reported to log and NVRAM.
  OPEN_CORE_VERSION must follow X.Y.Z format, where X.Y.Z are single digits.
**/
#define OPEN_CORE_VERSION          "0.0.3"

/**
  OpenCore build type reported to log and NVRAM.
**/
#if defined (OC_RELEASE_TARGET)
#define OPEN_CORE_TARGET           "REL" ///< Release.
#elif defined (OC_DEBUG_TARGET)
#define OPEN_CORE_TARGET           "DBG" ///< Debug with compiler optimisations.
#elif defined (OC_NOOPT_TARGET)
#define OPEN_CORE_TARGET           "NPT" ///< Debug with no compiler optimisations.
#else
#define OPEN_CORE_TARGET           "UNK" ///< This one should not happen in public builds.
#endif

#define OPEN_CORE_IMAGE_PATH       L"EFI\\OC\\OpenCore.efi"

/**
  Multiple boards, namely ASUS P8H61-M and P8H61-M LX2 will not
  open directories with trailing slash. It is irrelevant whether
  front slash is present for them.

  This means L"EFI\\OC\\" and L"\\EFI\\OC\\" will both fail to open,
  while L"EFI\\OC" and L"\\EFI\\OC" will open fine.

  We do not open any directories except root path and dmg, so the
  hack lives here.
**/
#define OPEN_CORE_ROOT_PATH        L"EFI\\OC"

#define OPEN_CORE_CONFIG_PATH      L"config.plist"

#define OPEN_CORE_LOG_PATH         L"opencore.log"

#define OPEN_CORE_ACPI_PATH        L"ACPI\\Custom\\"

#define OPEN_CORE_UEFI_DRIVER_PATH L"Drivers\\"

#define OPEN_CORE_KEXT_PATH        L"Kexts\\"

#define OPEN_CORE_NVRAM_ATTR       (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define OPEN_CORE_INT_NVRAM_ATTR   EFI_VARIABLE_BOOTSERVICE_ACCESS

/**
  Obtain cryptographic key if it was installed.

  @param[in]  Bootstrap  bootstrap protocol.

  @return public key or NULL.
**/
RSA_PUBLIC_KEY *
OcGetVaultKey (
  IN  OC_BOOTSTRAP_PROTOCOL *Bootstrap
  );

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

/**
  Load early miscellaneous support like configuration.

  @param[in]  Storage   OpenCore storage.
  @param[out] Config    OpenCore configuration.
  @param[in]  VaultKey  Vault key.

  @retval EFI_SUCCESS when allowed to continue.
**/
EFI_STATUS
OcMiscEarlyInit (
  IN  OC_STORAGE_CONTEXT *Storage,
  OUT OC_GLOBAL_CONFIG   *Config,
  IN  RSA_PUBLIC_KEY     *VaultKey  OPTIONAL
  );

/**
  Load late miscellaneous support like boot screen config.

  @param[in]  Config     OpenCore configuration.
  @param[in]  LoadPath   OpenCore loading path.
  @param[out] LoadHandle OpenCore loading handle.

  @retval EFI_SUCCESS on success, informational.
**/
EFI_STATUS
OcMiscLateInit (
  IN  OC_GLOBAL_CONFIG          *Config,
  IN  EFI_DEVICE_PATH_PROTOCOL  *LoadPath  OPTIONAL,
  OUT EFI_HANDLE                *LoadHandle OPTIONAL
  );

/**
  Load miscellaneous support after UEFI quirks.

  @param[in]  Config     OpenCore configuration.
**/
VOID
OcMiscUefiQuirksLoaded (
  IN OC_GLOBAL_CONFIG   *Config
  );

#endif // OPEN_CORE_H

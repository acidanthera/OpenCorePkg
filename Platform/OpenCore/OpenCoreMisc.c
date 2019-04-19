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

#include <Guid/OcLogVariable.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
VOID
OcStoreLoadPath (
  IN EFI_DEVICE_PATH_PROTOCOL  *LoadPath OPTIONAL
  )
{
  EFI_STATUS  Status;
  CHAR16      *DevicePath;
  CHAR8       OutPath[256];

  if (LoadPath != NULL) {
    DevicePath = ConvertDevicePathToText (LoadPath, FALSE, FALSE);
    if (DevicePath != NULL) {
      AsciiSPrint (OutPath, sizeof (OutPath), "%s", DevicePath);
      FreePool (DevicePath);
    } else {
      LoadPath = NULL;
    }
  }

  if (LoadPath == NULL) {
    AsciiSPrint (OutPath, sizeof (OutPath), "Unknown");
  }

  Status = gRT->SetVariable (
    OC_LOG_VARIABLE_PATH,
    &gOcLogVariableGuid,
    OPEN_CORE_NVRAM_ATTR,
    AsciiStrSize (OutPath),
    OutPath
    );

  DEBUG ((
    EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
    "OC: Setting NVRAM %g:%a = %a - %r\n",
    &gOcLogVariableGuid,
    OC_LOG_VARIABLE_PATH,
    OutPath,
    Status
    ));
}

EFI_STATUS
OcMiscEarlyInit (
  IN  OC_STORAGE_CONTEXT *Storage,
  OUT OC_GLOBAL_CONFIG   *Config,
  IN  RSA_PUBLIC_KEY     *VaultKey  OPTIONAL
  )
{
  EFI_STATUS                Status;
  CHAR8                     *ConfigData;
  UINT32                    ConfigDataSize;

  ConfigData = OcStorageReadFileUnicode (
    Storage,
    OPEN_CORE_CONFIG_PATH,
    &ConfigDataSize
    );

  if (ConfigData != NULL) {
    DEBUG ((DEBUG_INFO, "OC: Loaded configuration of %u bytes\n", ConfigDataSize));

    Status = OcConfigurationInit (Config, ConfigData, ConfigDataSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to parse configuration!\n"));
      CpuDeadLoop ();
      return EFI_UNSUPPORTED; ///< Should be unreachable.
    }

    FreePool (ConfigData);
  } else {
    DEBUG ((DEBUG_ERROR, "OC: Failed to load configuration!\n"));
    CpuDeadLoop ();
    return EFI_UNSUPPORTED; ///< Should be unreachable.
  }

  //
  // Sanity check that the configuration is adequate.
  //
  if (!Storage->HasVault && Config->Misc.Security.RequireVault) {
    DEBUG ((DEBUG_ERROR, "OC: Configuration requires vault but no vault provided!\n"));
    CpuDeadLoop ();
    return EFI_SECURITY_VIOLATION; ///< Should be unreachable.
  }

  if (VaultKey == NULL && Config->Misc.Security.RequireSignature) {
    DEBUG ((DEBUG_ERROR, "OC: Configuration requires signed vault but no public key provided!\n"));
    CpuDeadLoop ();
    return EFI_SECURITY_VIOLATION; ///< Should be unreachable.
  }

  OcConfigureLogProtocol (
    Config->Misc.Debug.Target,
    Config->Misc.Debug.Delay,
    (UINTN) Config->Misc.Debug.DisplayLevel,
    (UINTN) Config->Misc.Security.HaltLevel
    );

  DEBUG ((
    DEBUG_INFO,
    "OC: OpenCore is now loading (Vault: %d/%d, Sign %d/%d)...\n",
    Storage->HasVault,
    Config->Misc.Security.RequireVault,
    VaultKey != NULL,
    Config->Misc.Security.RequireSignature
    ));

  return EFI_SUCCESS;
}

EFI_STATUS
OcMiscLateInit (
  IN  OC_GLOBAL_CONFIG          *Config,
  IN  EFI_DEVICE_PATH_PROTOCOL  *LoadPath  OPTIONAL,
  OUT EFI_HANDLE                *LoadHandle OPTIONAL
  )
{
  EFI_STATUS   Status;
  UINT32       Width;
  UINT32       Height;
  UINT32       Bpp;
  BOOLEAN      SetMax;

  if (Config->Misc.Debug.ExposeBootPath) {
    OcStoreLoadPath (LoadPath);
  }

  if (Config->Misc.Boot.ReinstallProtocol) {
    if (OcAppleBootPolicyInstallProtocol (TRUE) == NULL) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to reinstall boot policy protocol\n"));
    }
  }

  Status = EFI_SUCCESS;

  if (LoadHandle != NULL) {
    *LoadHandle = NULL;
    //
    // Do not disclose self entry unless asked.
    //
    if (LoadPath != NULL && Config->Misc.Boot.HideSelf) {
      Status = gBS->LocateDevicePath (
        &gEfiSimpleFileSystemProtocolGuid,
        &LoadPath,
        LoadHandle
        );
      DEBUG ((DEBUG_INFO, "OC: LoadHandle is %p - %r\n", *LoadHandle, Status));
    }
  }

  ParseScreenResolution (
    OC_BLOB_GET (&Config->Misc.Boot.Resolution),
    &Width,
    &Height,
    &Bpp,
    &SetMax
    );

  DEBUG ((
    DEBUG_INFO,
    "OC: Requested resolution is %u:%u@%u (max: %d)\n",
    Width,
    Height,
    Bpp,
    SetMax
    ));

  if (SetMax || (Width > 0 && Height > 0)) {
    Status = SetConsoleResolution (Width, Height, Bpp);
    DEBUG ((
      DEBUG_INFO,
      "OC: Changed resolution to %u:%u@%u (max: %d) - %r\n",
      Width,
      Height,
      Bpp,
      SetMax,
      Status
      ));
  }

  return Status;
}

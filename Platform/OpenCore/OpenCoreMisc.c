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

#include <Guid/OcVariables.h>

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
    &gOcVendorVariableGuid,
    OPEN_CORE_NVRAM_ATTR,
    AsciiStrSize (OutPath),
    OutPath
    );

  DEBUG ((
    EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
    "OC: Setting NVRAM %g:%a = %a - %r\n",
    &gOcVendorVariableGuid,
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
  EFI_TIME                  BootTime;

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

  if (Config->Misc.Debug.DisableWatchDog) {
    //
    // boot.efi kills watchdog only in FV2 UI.
    //
    gBS->SetWatchdogTimer (0, 0, 0, NULL);
  }

  OcConfigureLogProtocol (
    Config->Misc.Debug.Target,
    Config->Misc.Debug.DisplayDelay,
    (UINTN) Config->Misc.Debug.DisplayLevel,
    (UINTN) Config->Misc.Security.HaltLevel,
    OPEN_CORE_LOG_PATH,
    Storage->FileSystem
    );

  DEBUG ((
    DEBUG_INFO,
    "OC: OpenCore is now loading (Vault: %d/%d, Sign %d/%d)...\n",
    Storage->HasVault,
    Config->Misc.Security.RequireVault,
    VaultKey != NULL,
    Config->Misc.Security.RequireSignature
    ));

  Status = gRT->GetTime (&BootTime, NULL);
  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OC: Boot timestamp: - %04u.%02u.%02u %02u:%02u:%02u\n",
      BootTime.Year,
      BootTime.Month,
      BootTime.Day,
      BootTime.Hour,
      BootTime.Minute,
      BootTime.Second
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OC: Boot timestamp: - %r\n",
      Status
      ));
  }

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

  if ((Config->Misc.Security.ExposeSensitiveData & OCS_EXPOSE_BOOT_PATH) != 0) {
    OcStoreLoadPath (LoadPath);
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
    "OC: Requested resolution is %ux%u@%u (max: %d) from %a\n",
    Width,
    Height,
    Bpp,
    SetMax,
    OC_BLOB_GET (&Config->Misc.Boot.Resolution)
    ));

  if (SetMax || (Width > 0 && Height > 0)) {
    Status = SetConsoleResolution (Width, Height, Bpp);
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Changed resolution to %ux%u@%u (max: %d) from %a - %r\n",
      Width,
      Height,
      Bpp,
      SetMax,
      OC_BLOB_GET (&Config->Misc.Boot.Resolution),
      Status
      ));
  }

  ParseConsoleMode (
    OC_BLOB_GET (&Config->Misc.Boot.ConsoleMode),
    &Width,
    &Height,
    &SetMax
    );

  DEBUG ((
    DEBUG_INFO,
    "OC: Requested console mode is %ux%u (max: %d) from %a\n",
    Width,
    Height,
    SetMax,
    OC_BLOB_GET (&Config->Misc.Boot.ConsoleMode)
    ));

  if (SetMax || (Width > 0 && Height > 0)) {
    Status = SetConsoleMode (Width, Height);
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Changed console mode to %ux%u (max: %d) from %a - %r\n",
      Width,
      Height,
      SetMax,
      OC_BLOB_GET (&Config->Misc.Boot.ConsoleMode),
      Status
      ));
  }

  return Status;
}

VOID
OcMiscBoot (
  IN  OC_STORAGE_CONTEXT        *Storage,
  IN  OC_GLOBAL_CONFIG          *Config,
  IN  OC_IMAGE_START            StartImage,
  IN  BOOLEAN                   CustomBootGuid,
  IN  EFI_HANDLE                LoadHandle OPTIONAL
  )
{
  EFI_STATUS         Status;
  OC_PICKER_CONTEXT  *Context;
  UINTN              ContextSize;

  if (!OcOverflowMulAddUN (
    sizeof (OC_PICKER_ENTRY),
    Config->Misc.Tools.Count,
    sizeof (OC_PICKER_CONTEXT),
    &ContextSize))
  {
    Context = AllocateZeroPool (ContextSize);
  } else {
    Context = NULL;
  }

  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to allocate boot picker context!\n"));
    return;
  }

  Context->ScanPolicy     = Config->Misc.Security.ScanPolicy;
  Context->LoadPolicy     = OC_LOAD_DEFAULT_POLICY;
  Context->TimeoutSeconds = Config->Misc.Boot.Timeout;
  Context->StartImage     = StartImage;
  Context->ShowPicker     = Config->Misc.Boot.ShowPicker;
  Context->CustomBootGuid = CustomBootGuid;
  Context->ExcludeHandle  = LoadHandle;

  Status = OcRunSimpleBootPicker (
    Context
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to show boot menu!\n"));
  }
}

VOID
OcMiscUefiQuirksLoaded (
  IN OC_GLOBAL_CONFIG   *Config
  )
{
  if (((Config->Misc.Security.ScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) == 0
    && (Config->Misc.Security.ScanPolicy & OC_SCAN_FILE_SYSTEM_BITS) != 0)
    || ((Config->Misc.Security.ScanPolicy & OC_SCAN_DEVICE_LOCK) == 0
    && (Config->Misc.Security.ScanPolicy & OC_SCAN_DEVICE_BITS) != 0)) {
    DEBUG ((DEBUG_ERROR, "OC: Invalid ScanPolicy %X\n", Config->Misc.Security.ScanPolicy));
    CpuDeadLoop ();
  }

  //
  // Inform drivers about our scan policy.
  //
  gRT->SetVariable (
    OC_SCAN_POLICY_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    OPEN_CORE_INT_NVRAM_ATTR,
    sizeof (Config->Misc.Security.ScanPolicy),
    &Config->Misc.Security.ScanPolicy
    );

  OcConsoleControlSetBehaviour (
    ParseConsoleControlBehaviour (
      OC_BLOB_GET (&Config->Misc.Boot.ConsoleBehaviourUi)
      )
    );
}

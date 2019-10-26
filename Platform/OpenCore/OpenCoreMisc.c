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

#include <Protocol/OcInterface.h>

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

STATIC
EFI_STATUS
EFIAPI
OcToolLoadEntry (
  IN  VOID                        *Context,
  IN  OC_BOOT_ENTRY               *ChosenEntry,
  OUT VOID                        **Data,
  OUT UINT32                      *DataSize,
  OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath OPTIONAL
  )
{
  CHAR16              ToolPath[64];
  OC_STORAGE_CONTEXT  *Storage;

  UnicodeSPrint (
    ToolPath,
    sizeof (ToolPath),
    OPEN_CORE_TOOL_PATH "%s",
    ChosenEntry->PathName
    );

  Storage = (OC_STORAGE_CONTEXT *) Context;

  *Data = OcStorageReadFileUnicode (
    Storage,
    ToolPath,
    DataSize
    );
  if (*Data == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "OC: Tool %s cannot be found!\n",
      ToolPath
      ));
    return EFI_NOT_FOUND;
  }

  if (DevicePath != NULL) {
    *DevicePath = Storage->DummyDevicePath;
  }

  return EFI_SUCCESS;
}

CONST CHAR8 *
OcMiscGetVersionString (
  VOID
  )
{
  UINT32  Month;

  /**
    Force the assertions in case we forget about them.
  **/
  OC_STATIC_ASSERT (
    L_STR_LEN (OPEN_CORE_VERSION) == 5,
    "OPEN_CORE_VERSION must follow X.Y.Z format, where X.Y.Z are single digits."
    );

  OC_STATIC_ASSERT (
    L_STR_LEN (OPEN_CORE_TARGET) == 3,
    "OPEN_CORE_TARGET must XYZ format, where XYZ is build target."
    );

  STATIC CHAR8 mOpenCoreVersion[] = {
    /* [2]:[0]    = */ OPEN_CORE_TARGET
    /* [3]        = */ "-"
    /* [6]:[4]    = */ "XXX"
    /* [7]        = */ "-"
    /* [12]:[8]   = */ "YYYY-"
    /* [15]:[13]  = */ "MM-"
    /* [17]:[16]  = */ "DD"
  };

  STATIC BOOLEAN mOpenCoreVersionReady;

  if (!mOpenCoreVersionReady) {

    mOpenCoreVersion[4] = OPEN_CORE_VERSION[0];
    mOpenCoreVersion[5] = OPEN_CORE_VERSION[2];
    mOpenCoreVersion[6] = OPEN_CORE_VERSION[4];

    mOpenCoreVersion[8]  = __DATE__[7];
    mOpenCoreVersion[9]  = __DATE__[8];
    mOpenCoreVersion[10] = __DATE__[9];
    mOpenCoreVersion[11] = __DATE__[10];

    Month =
      (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n') ?  1 :
      (__DATE__[0] == 'F' && __DATE__[1] == 'e' && __DATE__[2] == 'b') ?  2 :
      (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ?  3 :
      (__DATE__[0] == 'A' && __DATE__[1] == 'p' && __DATE__[2] == 'r') ?  4 :
      (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ?  5 :
      (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ?  6 :
      (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ?  7 :
      (__DATE__[0] == 'A' && __DATE__[1] == 'u' && __DATE__[2] == 'g') ?  8 :
      (__DATE__[0] == 'S' && __DATE__[1] == 'e' && __DATE__[2] == 'p') ?  9 :
      (__DATE__[0] == 'O' && __DATE__[1] == 'c' && __DATE__[2] == 't') ? 10 :
      (__DATE__[0] == 'N' && __DATE__[1] == 'o' && __DATE__[2] == 'v') ? 11 :
      (__DATE__[0] == 'D' && __DATE__[1] == 'e' && __DATE__[2] == 'c') ? 12 : 0;

    mOpenCoreVersion[13] = Month < 10 ? '0' : '1';
    mOpenCoreVersion[14] = '0' + (Month % 10);
    mOpenCoreVersion[16] = __DATE__[4] >= '0' ? __DATE__[4] : '0';
    mOpenCoreVersion[17] = __DATE__[5];

    mOpenCoreVersionReady = TRUE;
  }

  return mOpenCoreVersion;
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
    OPEN_CORE_LOG_PREFIX_PATH,
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
      "OC: Boot timestamp - %04u.%02u.%02u %02u:%02u:%02u\n",
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
      "OC: Boot timestamp - %r\n",
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
  EFI_STATUS   HibernateStatus;
  UINT32       Width;
  UINT32       Height;
  UINT32       Bpp;
  BOOLEAN      SetMax;
  CONST CHAR8  *HibernateMode;
  UINT32       HibernateMask;

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
    Status = SetConsoleResolution (
      Width,
      Height,
      Bpp,
      OcShouldReconnectConsoleOnResolutionChange (Config)
      );
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

  HibernateMode = OC_BLOB_GET (&Config->Misc.Boot.HibernateMode);

  if (AsciiStrCmp (HibernateMode, "None") == 0) {
    HibernateMask = HIBERNATE_MODE_NONE;
  } else if (AsciiStrCmp (HibernateMode, "Auto") == 0) {
    HibernateMask = HIBERNATE_MODE_RTC | HIBERNATE_MODE_NVRAM;
  } else if (AsciiStrCmp (HibernateMode, "RTC") == 0) {
    HibernateMask = HIBERNATE_MODE_RTC;
  } else if (AsciiStrCmp (HibernateMode, "NVRAM") == 0) {
    HibernateMask = HIBERNATE_MODE_NVRAM;
  } else {
    DEBUG ((DEBUG_INFO, "OC: Invalid HibernateMode: %a\n", HibernateMode));
    HibernateMask = HIBERNATE_MODE_NONE;
  }

  DEBUG ((DEBUG_INFO, "OC: Translated HibernateMode %a to %u\n", HibernateMode, HibernateMask));

  HibernateStatus = OcActivateHibernateWake (HibernateMask);
  DEBUG ((DEBUG_INFO, "OC: Hibernation detection status is %r\n", HibernateStatus));
  (VOID) HibernateStatus;

  return Status;
}

VOID
OcMiscBoot (
  IN  OC_STORAGE_CONTEXT        *Storage,
  IN  OC_GLOBAL_CONFIG          *Config,
  IN  OC_PRIVILEGE_CONTEXT      *Privilege OPTIONAL,
  IN  OC_IMAGE_START            StartImage,
  IN  BOOLEAN                   CustomBootGuid,
  IN  EFI_HANDLE                LoadHandle OPTIONAL
  )
{
  EFI_STATUS             Status;
  OC_PICKER_CONTEXT      *Context;
  OC_PICKER_CMD          PickerCommand;
  UINTN                  ContextSize;
  UINT32                 Index;
  UINT32                 EntryIndex;
  OC_INTERFACE_PROTOCOL  *Interface;
  UINTN                  BlessOverrideSize;
  CHAR16                 **BlessOverride;

  //
  // Do not use our boot picker unless asked.
  //
  if (!Config->Misc.Boot.UsePicker) {
    DEBUG ((DEBUG_INFO, "OC: Handing off to external boot controller\n"));

    Status = gBS->LocateProtocol (
      &gOcInterfaceProtocolGuid,
      NULL,
      (VOID **) &Interface
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OC: Missing external GUI protocol - %r\n", Status));
      return;
    }

    if (Interface->Revision != OC_INTERFACE_REVISION) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Incompatible external GUI protocol - %u vs %u\n",
        Interface->Revision,
        OC_INTERFACE_REVISION
        ));
      return;
    }
  } else {
    Interface = NULL;
  }
  //
  // Due to the file size and sanity guarantees OcXmlLib makes,
  // adding Counts cannot overflow.
  //
  if (!OcOverflowMulAddUN (
    sizeof (OC_PICKER_ENTRY),
    Config->Misc.Entries.Count + Config->Misc.Tools.Count,
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

  if (Config->Misc.BlessOverride.Count > 0) {
    if (!OcOverflowMulUN (
      Config->Misc.BlessOverride.Count,
      sizeof (*BlessOverride),
      &BlessOverrideSize))
    {
      BlessOverride = AllocateZeroPool (BlessOverrideSize);
    } else {
      BlessOverride = NULL;
    }

    if (BlessOverride == NULL) {
      FreePool (Context);
      DEBUG ((DEBUG_ERROR, "OC: Failed to allocate bless overrides!\n"));
      return;
    }

    for (Index = 0; Index < Config->Misc.BlessOverride.Count; ++Index) {
      BlessOverride[Index] = AsciiStrCopyToUnicode (
                               OC_BLOB_GET (
                                 Config->Misc.BlessOverride.Values[Index]
                                 ),
                               0
                               );
      if (BlessOverride[Index] == NULL) {
        for (EntryIndex = 0; EntryIndex < Index; ++EntryIndex) {
          FreePool (BlessOverride[EntryIndex]);
        }
        FreePool (BlessOverride);
        FreePool (Context);
        DEBUG ((DEBUG_ERROR, "OC: Failed to allocate bless overrides!\n"));
        return;
      }
    }

    Context->NumCustomBootPaths = Config->Misc.BlessOverride.Count;
    Context->CustomBootPaths    = BlessOverride;
  }

  Context->ScanPolicy         = Config->Misc.Security.ScanPolicy;
  Context->LoadPolicy         = OC_LOAD_DEFAULT_POLICY;
  Context->TimeoutSeconds     = Config->Misc.Boot.Timeout;
  Context->StartImage         = StartImage;
  Context->CustomBootGuid     = CustomBootGuid;
  Context->ExcludeHandle      = LoadHandle;
  Context->CustomEntryContext = Storage;
  Context->CustomRead         = OcToolLoadEntry;
  Context->PrivilegeContext   = Privilege;
  Context->RequestPrivilege   = OcShowSimplePasswordRequest;
  Context->BalloonAllocator   = OcGetBallooningHandler (Config);

  if ((Config->Misc.Security.ExposeSensitiveData & OCS_EXPOSE_VERSION_UI) != 0) {
    Context->TitleSuffix      = OcMiscGetVersionString ();
  }

  if (Config->Misc.Boot.ShowPicker) {
    PickerCommand = Context->PickerCommand = OcPickerShowPicker;
  } else {
    PickerCommand = Context->PickerCommand = OcPickerDefault;
  }

  for (Index = 0, EntryIndex = 0; Index < Config->Misc.Entries.Count; ++Index) {
    if (Config->Misc.Entries.Values[Index]->Enabled) {
      Context->CustomEntries[EntryIndex].Name      = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Name);
      Context->CustomEntries[EntryIndex].Path      = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Path);
      Context->CustomEntries[EntryIndex].Arguments = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Arguments);
      ++EntryIndex;
    }
  }

  Context->AbsoluteEntryCount = EntryIndex;
  //
  // Due to the file size and sanity guarantees OcXmlLib makes,
  // EntryIndex cannot overflow.
  //
  for (Index = 0; Index < Config->Misc.Tools.Count; ++Index) {
    if (Config->Misc.Tools.Values[Index]->Enabled) {
      Context->CustomEntries[EntryIndex].Name      = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Name);
      Context->CustomEntries[EntryIndex].Path      = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Path);
      Context->CustomEntries[EntryIndex].Arguments = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Arguments);
      ++EntryIndex;
    }
  }

  Context->AllCustomEntryCount = EntryIndex;
  Context->PollAppleHotKeys    = Config->Misc.Boot.PollAppleHotKeys;

  OcLoadPickerHotKeys (Context);

  Context->ShowNvramReset = Config->Misc.Security.AllowNvramReset;
  if (!Config->Misc.Security.AllowNvramReset && Context->PickerCommand == OcPickerResetNvram) {
    Context->PickerCommand = PickerCommand;
  }

  if (Interface != NULL) {
    Status = Interface->ShowInteface (Interface, Storage, Context);
  } else {
    Status = OcRunSimpleBootPicker (Context);
  }
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

  //
  // Regardless of the mode ensure our cursor is disabled as we do not need it.
  // This is a bit ugly, but works for most platforms we have:
  // - Firstly disable it on platforms that start with it for whatever reason.
  //   Generally Insyde laptops are happy with that.
  // - Secondly change the mode, on APTIO it may reenable the cursor in Text mode.
  // - Thirdly disable it again to ensure it is definitely disabled.
  //

  OcConsoleDisableCursor ();
  OcConsoleControlSetBehaviour (
    ParseConsoleControlBehaviour (
      OC_BLOB_GET (&Config->Misc.Boot.ConsoleBehaviourUi)
      )
    );
  OcConsoleDisableCursor ();
}

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
#include <Guid/GlobalVariable.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleBootCompatLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcAppleEventLib.h>
#include <Library/OcAppleImageConversionLib.h>
#include <Library/OcInputLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcAppleUserInterfaceThemeLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcDataHubLib.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/OcDriverConnectionLib.h>
#include <Library/OcFirmwareVolumeLib.h>
#include <Library/OcHashServicesLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcSmcLib.h>
#include <Library/OcOSInfoLib.h>
#include <Library/OcUnicodeCollationEngLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>

STATIC EFI_EVENT mOcExitBootServicesEvent;

STATIC
VOID
OcLoadDrivers (
  IN  OC_STORAGE_CONTEXT  *Storage,
  IN  OC_GLOBAL_CONFIG    *Config,
  OUT EFI_HANDLE          **DriversToConnect  OPTIONAL
  )
{
  EFI_STATUS  Status;
  VOID        *Driver;
  UINT32      DriverSize;
  UINT32      Index;
  CHAR16      DriverPath[64];
  EFI_HANDLE  ImageHandle;
  EFI_HANDLE  *DriversToConnectIterator;
  VOID        *DriverBinding;

  DriversToConnectIterator = NULL;
  if (DriversToConnect != NULL) {
    *DriversToConnect = NULL;
  }

  DEBUG ((DEBUG_INFO, "OC: Got %u drivers\n", Config->Uefi.Drivers.Count));

  for (Index = 0; Index < Config->Uefi.Drivers.Count; ++Index) {
    DEBUG ((
      DEBUG_INFO,
      "OC: Driver %a at %u is being loaded...\n",
      OC_BLOB_GET (Config->Uefi.Drivers.Values[Index]),
      Index
      ));

    UnicodeSPrint (
      DriverPath,
      sizeof (DriverPath),
      OPEN_CORE_UEFI_DRIVER_PATH "%a",
      OC_BLOB_GET (Config->Uefi.Drivers.Values[Index])
      );

    Driver = OcStorageReadFileUnicode (Storage, DriverPath, &DriverSize);
    if (Driver == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Driver %a at %u cannot be found!\n",
        OC_BLOB_GET (Config->Uefi.Drivers.Values[Index]),
        Index
        ));
      //
      // TODO: This should cause security violation if configured!
      //
      continue;
    }

    //
    // TODO: Use AppleLoadedImage!!
    //
    ImageHandle = NULL;
    Status = gBS->LoadImage (
      FALSE,
      gImageHandle,
      NULL,
      Driver,
      DriverSize,
      &ImageHandle
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Driver %a at %u cannot be loaded - %r!\n",
        OC_BLOB_GET (Config->Uefi.Drivers.Values[Index]),
        Index,
        Status
        ));
      FreePool (Driver);
      continue;
    }

    Status = gBS->StartImage (
      ImageHandle,
      NULL,
      NULL
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Driver %a at %u cannot be started - %r!\n",
        OC_BLOB_GET (Config->Uefi.Drivers.Values[Index]),
        Index,
        Status
        ));
      gBS->UnloadImage (ImageHandle);
    }

    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Driver %a at %u is successfully loaded!\n",
        OC_BLOB_GET (Config->Uefi.Drivers.Values[Index]),
        Index
        ));

      if (DriversToConnect != NULL) {
        Status = gBS->HandleProtocol (
          ImageHandle,
          &gEfiDriverBindingProtocolGuid,
          (VOID **) &DriverBinding
          );

        if (!EFI_ERROR (Status)) {
          if (*DriversToConnect == NULL) {
            //
            // Allocate enough entries for the drivers to connect.
            //
            *DriversToConnect = AllocatePool (
              (Config->Uefi.Drivers.Count + 1 - Index) * sizeof (**DriversToConnect)
              );

            if (*DriversToConnect != NULL) {
              DriversToConnectIterator = *DriversToConnect;
            } else {
              DEBUG ((DEBUG_ERROR, "OC: Failed to allocate memory for drivers to connect\n"));
              FreePool (Driver);
              return;
            }
          }

          *DriversToConnectIterator = ImageHandle;
          ++DriversToConnectIterator;

          DEBUG ((
            DEBUG_INFO,
            "OC: Driver %a at %u needs connection.\n",
            OC_BLOB_GET (Config->Uefi.Drivers.Values[Index]),
            Index
            ));
        }
      }
    }

    FreePool (Driver);
  }

  //
  // Driver connection list should be null-terminated.
  //
  if (*DriversToConnectIterator != NULL) {
    *DriversToConnectIterator = NULL;
  }
}

STATIC
VOID
EFIAPI
OcExitBootServicesHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS         Status;
  OC_GLOBAL_CONFIG   *Config;

  Config = (OC_GLOBAL_CONFIG *) Context;

  if (Config->Uefi.Quirks.ReleaseUsbOwnership) {
    Status = ReleaseUsbOwnership ();
    DEBUG ((DEBUG_INFO, "OC: ReleaseUsbOwnership status - %r\n", Status));
  }

  //
  // FIXME: This is a very ugly hack for (at least) ASUS Z87-Pro.
  // This board results in still waiting for root devices due to firmware
  // performing some timer(?) actions in parallel to ExitBootServices.
  // Some day we should figure out what exactly happens there.
  // It is not the first time I face this, check AptioInputFix timer code:
  // https://github.com/acidanthera/AptioFixPkg/blob/e54c185/Platform/AptioInputFix/Timer/AIT.c#L72-L73
  // Roughly 5 seconds is good enough.
  //
  if (Config->Uefi.Quirks.ExitBootServicesDelay > 0) {
    gBS->Stall (Config->Uefi.Quirks.ExitBootServicesDelay);
  }

  if (Config->Uefi.Input.TimerResolution != 0) {
    Status = OcAppleGenericInputTimerQuirkExit ();
    DEBUG ((
      DEBUG_INFO,
      "OC: OcAppleGenericInputTimerQuirkExit status - %r\n",
      Status
      ));
  }

  if (Config->Uefi.Input.PointerSupport) {
    Status = OcAppleGenericInputPointerExit ();
    DEBUG ((DEBUG_INFO,
      "OC: OcAppleGenericInputPointerExit status - %r\n",
      Status
      ));
  }

  if (Config->Uefi.Input.KeySupport) {
    Status = OcAppleGenericInputKeycodeExit ();
    DEBUG ((
      DEBUG_INFO,
      "OC: OcAppleGenericInputKeycodeExit status - %r\n",
      Status
      ));
  }
}

STATIC
VOID
OcReinstallProtocols (
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  if (OcAppleBootPolicyInstallProtocol (Config->Uefi.Protocols.AppleBootPolicy) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install boot policy protocol\n"));
  }

  if (OcDataHubInstallProtocol (Config->Uefi.Protocols.DataHub) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install data hub protocol\n"));
  }

  if (OcDevicePathPropertyInstallProtocol (Config->Uefi.Protocols.DeviceProperties) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install device properties protocol\n"));
  }

  if (OcAppleImageConversionInstallProtocol (Config->Uefi.Protocols.AppleImageConversion) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install image conversion protocol\n"));
  }

  if (OcSmcIoInstallProtocol (Config->Uefi.Protocols.AppleSmcIo, Config->Misc.Security.AuthRestart) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install smc i/o protocol\n"));
  }

  if (OcAppleUserInterfaceThemeInstallProtocol (Config->Uefi.Protocols.AppleUserInterfaceTheme) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install user interface theme protocol\n"));
  }

  if (OcUnicodeCollationEngInstallProtocol (Config->Uefi.Protocols.UnicodeCollation) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install unicode collation protocol\n"));
  }

  if (OcHashServicesInstallProtocol (Config->Uefi.Protocols.HashServices) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install hash services protocol\n"));
  }

  if (OcAppleKeyMapInstallProtocols (Config->Uefi.Protocols.AppleKeyMap) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install key map protocols\n"));
  }

  if (OcAppleEventInstallProtocol (Config->Uefi.Protocols.AppleEvent) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install key event protocol\n"));
  }

  if (OcFirmwareVolumeInstallProtocol (Config->Uefi.Protocols.FirmwareVolume) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install firmware volume protocol\n"));
  }

  if (OcOSInfoInstallProtocol (Config->Uefi.Protocols.OSInfo) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install os info protocol\n"));
  }
}

STATIC
BOOLEAN
OcLoadUefiInputSupport (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  BOOLEAN               ExitBs;
  EFI_STATUS            Status;
  UINT32                TimerResolution;
  CONST CHAR8           *PointerSupportStr;
  OC_INPUT_POINTER_MODE PointerMode;
  OC_INPUT_KEY_MODE     KeyMode;
  CONST CHAR8           *KeySupportStr;

  ExitBs = FALSE;

  TimerResolution = Config->Uefi.Input.TimerResolution;
  if (TimerResolution != 0) {
    Status = OcAppleGenericInputTimerQuirkInit (TimerResolution);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to initialize timer quirk\n"));
    } else {
      ExitBs = TRUE;
    }
  }

  if (Config->Uefi.Input.PointerSupport) {
    PointerSupportStr = OC_BLOB_GET (&Config->Uefi.Input.PointerSupportMode);
    PointerMode = OcInputPointerModeMax;
    if (AsciiStrCmp (PointerSupportStr, "ASUS") == 0) {
      PointerMode = OcInputPointerModeAsus;
    } else {
      DEBUG ((DEBUG_WARN, "OC: Invalid input pointer mode %a\n", PointerSupportStr));
    }

    if (PointerMode != OcInputPointerModeMax) {
      Status = OcAppleGenericInputPointerInit (PointerMode);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "OC: Failed to initialize pointer\n"));
      } else {
        ExitBs = TRUE;
      }
    }
  }

  if (Config->Uefi.Input.KeySupport) {
    KeySupportStr = OC_BLOB_GET (&Config->Uefi.Input.KeySupportMode);
    KeyMode = OcInputKeyModeMax;
    if (AsciiStrCmp (KeySupportStr, "Auto") == 0) {
      KeyMode = OcInputKeyModeAuto;
    } else if (AsciiStrCmp (KeySupportStr, "V1") == 0) {
      KeyMode = OcInputKeyModeV1;
    } else if (AsciiStrCmp (KeySupportStr, "V2") == 0) {
      KeyMode = OcInputKeyModeV2;
    } else if (AsciiStrCmp (KeySupportStr, "AMI") == 0) {
      KeyMode = OcInputKeyModeAmi;
    } else {
      DEBUG ((DEBUG_WARN, "OC: Invalid input key mode %a\n", KeySupportStr));
    }

    if (KeyMode != OcInputKeyModeMax) {
      Status = OcAppleGenericInputKeycodeInit (
                 KeyMode,
                 Config->Uefi.Input.KeyForgetThreshold,
                 Config->Uefi.Input.KeyMergeThreshold,
                 Config->Uefi.Input.KeySwap
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "OC: Failed to initialize keycode\n"));
      } else {
        ExitBs = TRUE;
      }
    }
  }

  return ExitBs;
}

STATIC
VOID
OcLoadUefiOutputSupport (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  EFI_STATUS           Status;
  CONST CHAR8          *AsciiRenderer;
  OC_CONSOLE_RENDERER  Renderer;
  UINT32               Width;
  UINT32               Height;
  UINT32               Bpp;
  BOOLEAN              SetMax;

  if (Config->Uefi.Output.ProvideConsoleGop) {
    OcProvideConsoleGop ();
  }

  OcParseScreenResolution (
    OC_BLOB_GET (&Config->Uefi.Output.Resolution),
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
    OC_BLOB_GET (&Config->Uefi.Output.Resolution)
    ));

  if (SetMax || (Width > 0 && Height > 0)) {
    Status = OcSetConsoleResolution (
      Width,
      Height,
      Bpp
      );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Changed resolution to %ux%u@%u (max: %d) from %a - %r\n",
      Width,
      Height,
      Bpp,
      SetMax,
      OC_BLOB_GET (&Config->Uefi.Output.Resolution),
      Status
      ));
  }

  if (Config->Uefi.Output.DirectGopRendering) {
    OcUseDirectGop ();
  }

  if (Config->Uefi.Output.ReconnectOnResChange) {
    OcReconnectConsole ();
  }

  AsciiRenderer = OC_BLOB_GET (&Config->Uefi.Output.TextRenderer);

  if (AsciiRenderer[0] == '\0' || AsciiStrCmp (AsciiRenderer, "BuiltinGraphics") == 0) {
    Renderer = OcConsoleRendererBuiltinGraphics;
  } else if (AsciiStrCmp (AsciiRenderer, "SystemGraphics") == 0) {
    Renderer = OcConsoleRendererSystemGraphics;
  } else if (AsciiStrCmp (AsciiRenderer, "SystemText") == 0) {
    Renderer = OcConsoleRendererSystemText;
  } else if (AsciiStrCmp (AsciiRenderer, "SystemGeneric") == 0) {
    Renderer = OcConsoleRendererSystemGeneric;
  } else {
    DEBUG ((DEBUG_WARN, "OC: Requested unknown renderer %a\n", AsciiRenderer));
    Renderer = OcConsoleRendererBuiltinGraphics;
  }

  OcSetupConsole (
    Renderer,
    Config->Uefi.Output.Scale,
    Config->Uefi.Output.IgnoreTextInGraphics,
    Config->Uefi.Output.SanitiseClearScreen,
    Config->Uefi.Output.ClearScreenOnModeSwitch,
    Config->Uefi.Output.ReplaceTabWithSpace
    );

  if (Config->Uefi.Output.ProvideEarlyConsole) {
    OcConsoleControlSetMode (EfiConsoleControlScreenText);
  }

  OcParseConsoleMode (
    OC_BLOB_GET (&Config->Uefi.Output.ConsoleMode),
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
    OC_BLOB_GET (&Config->Uefi.Output.ConsoleMode)
    ));

  if (SetMax || (Width > 0 && Height > 0)) {
    Status = OcSetConsoleMode (Width, Height);
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Changed console mode to %ux%u (max: %d) from %a - %r\n",
      Width,
      Height,
      SetMax,
      OC_BLOB_GET (&Config->Uefi.Output.ConsoleMode),
      Status
      ));
  }
}

VOID
OcLoadBooterUefiSupport (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  OC_ABC_SETTINGS  AbcSettings;
  UINT32           Index;
  UINT32           NextIndex;

  ZeroMem (&AbcSettings, sizeof (AbcSettings));

  AbcSettings.AvoidRuntimeDefrag     = Config->Booter.Quirks.AvoidRuntimeDefrag;
  AbcSettings.DevirtualiseMmio       = Config->Booter.Quirks.DevirtualiseMmio;
  AbcSettings.DisableSingleUser      = Config->Booter.Quirks.DisableSingleUser;
  AbcSettings.DisableVariableWrite   = Config->Booter.Quirks.DisableVariableWrite;
  AbcSettings.ProtectSecureBoot      = Config->Booter.Quirks.ProtectSecureBoot;
  AbcSettings.DiscardHibernateMap    = Config->Booter.Quirks.DiscardHibernateMap;
  AbcSettings.EnableSafeModeSlide    = Config->Booter.Quirks.EnableSafeModeSlide;
  AbcSettings.EnableWriteUnprotector = Config->Booter.Quirks.EnableWriteUnprotector;
  AbcSettings.ForceExitBootServices  = Config->Booter.Quirks.ForceExitBootServices;
  AbcSettings.ProtectCsmRegion       = Config->Booter.Quirks.ProtectCsmRegion;
  AbcSettings.ProvideCustomSlide     = Config->Booter.Quirks.ProvideCustomSlide;
  AbcSettings.SetupVirtualMap        = Config->Booter.Quirks.SetupVirtualMap;
  AbcSettings.ShrinkMemoryMap        = Config->Booter.Quirks.ShrinkMemoryMap;
  AbcSettings.SignalAppleOS          = Config->Booter.Quirks.SignalAppleOS;

  if (AbcSettings.DevirtualiseMmio && Config->Booter.MmioWhitelist.Count > 0) {
    AbcSettings.MmioWhitelist = AllocatePool (
      Config->Booter.MmioWhitelist.Count * sizeof (AbcSettings.MmioWhitelist[0])
      );

    if (AbcSettings.MmioWhitelist != NULL) {
      NextIndex = 0;
      for (Index = 0; Index < Config->Booter.MmioWhitelist.Count; ++Index) {
        if (Config->Booter.MmioWhitelist.Values[Index]->Enabled) {
          AbcSettings.MmioWhitelist[NextIndex] = Config->Booter.MmioWhitelist.Values[Index]->Address;
          ++NextIndex;
        }
      }
      AbcSettings.MmioWhitelistSize = NextIndex;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Failed to allocate %u slots for mmio addresses\n",
        (UINT32) Config->Booter.MmioWhitelist.Count
        ));
    }

  }

  OcAbcInitialize (&AbcSettings);
}

VOID
OcLoadUefiSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *DriversToConnect;
  UINTN       Index;
  UINTN       Index2;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  BOOLEAN     BootOrderChanged;
  BOOLEAN     AgiExitBs;

  OcReinstallProtocols (Config);

  AgiExitBs = OcLoadUefiInputSupport (Config);

  //
  // Setup Apple bootloader specific UEFI features.
  //
  OcLoadBooterUefiSupport (Config);

  if (Config->Uefi.Quirks.IgnoreInvalidFlexRatio) {
    OcCpuCorrectFlexRatio (CpuInfo);
  }

  //
  // Inform platform support whether we want Boot#### routing or not.
  //
  gRT->SetVariable (
    OC_BOOT_REDIRECT_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    OPEN_CORE_INT_NVRAM_ATTR,
    sizeof (Config->Uefi.Quirks.RequestBootVarRouting),
    &Config->Uefi.Quirks.RequestBootVarRouting
    );

  gRT->SetVariable (
    OC_BOOT_FALLBACK_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    OPEN_CORE_INT_NVRAM_ATTR,
    sizeof (Config->Uefi.Quirks.RequestBootVarFallback),
    &Config->Uefi.Quirks.RequestBootVarFallback
    );

  if (Config->Uefi.Quirks.RequestBootVarFallback) {
    Status = GetVariable2 (
      EFI_BOOT_ORDER_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      (VOID **) &BootOrder,
      &BootOrderSize
      );

    //
    // Deduplicate BootOrder variable contents.
    //
    if (!EFI_ERROR (Status) && BootOrderSize > 0 && BootOrderSize % sizeof (BootOrder[0]) == 0) {
      BootOrderChanged = FALSE;

      for (Index = 1; Index < BootOrderSize / sizeof (BootOrder[0]); ++Index) {
        for (Index2 = 0; Index2 < Index; ++Index2) {
          if (BootOrder[Index] == BootOrder[Index2]) {
            //
            // Found duplicate.
            //
            BootOrderChanged = TRUE;
            CopyMem (
              &BootOrder[Index],
              &BootOrder[Index + 1],
              BootOrderSize - sizeof (BootOrder[0]) * (Index + 1)
              );
            BootOrderSize -= sizeof (BootOrder[0]);
            --Index;
            break;
          }
        }

        if (BootOrderChanged) {
          DEBUG ((DEBUG_INFO, "OC: Performed BootOrder deduplication\n"));
          gRT->SetVariable (
            EFI_BOOT_ORDER_VARIABLE_NAME,
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
            BootOrderSize,
            BootOrder
            );
        }
      }
    }
  }

  if (Config->Uefi.Quirks.ReleaseUsbOwnership
    || Config->Uefi.Quirks.ExitBootServicesDelay > 0
    || AgiExitBs) {
    gBS->CreateEvent (
      EVT_SIGNAL_EXIT_BOOT_SERVICES,
      TPL_NOTIFY,
      OcExitBootServicesHandler,
      Config,
      &mOcExitBootServicesEvent
      );
  }

  if (Config->Uefi.Quirks.UnblockFsConnect) {
    OcUnblockUnmountedPartitions ();
  }

  OcMiscUefiQuirksLoaded (Config);

  if (Config->Uefi.ConnectDrivers) {
    OcLoadDrivers (Storage, Config, &DriversToConnect);
    DEBUG ((DEBUG_INFO, "OC: Connecting drivers...\n"));
    if (DriversToConnect != NULL) {
      OcRegisterDriversToHighestPriority (DriversToConnect);
      //
      // DriversToConnect is not freed as it is owned by OcRegisterDriversToHighestPriority.
      //
    }
    OcConnectDrivers ();
    DEBUG ((DEBUG_INFO, "OC: Connecting drivers done...\n"));
  } else {
    OcLoadDrivers (Storage, Config, NULL);
  }

  OcLoadUefiOutputSupport (Config);
}

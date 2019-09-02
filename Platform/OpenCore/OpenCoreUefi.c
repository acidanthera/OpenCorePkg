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

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleBootCompatLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcAppleEventLib.h>
#include <Library/OcAppleImageConversionLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcAppleUserInterfaceThemeLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcDataHubLib.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/OcFirmwareVolumeLib.h>
#include <Library/OcHashServicesLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcUnicodeCollationEngLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>

STATIC EFI_EVENT mOcExitBootServicesEvent;

STATIC
VOID
OcLoadDrivers (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS  Status;
  VOID        *Driver;
  UINT32      DriverSize;
  UINT32      Index;
  CHAR16      DriverPath[64];
  EFI_HANDLE  ImageHandle;

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
    }

    FreePool (Driver);
  }
}

STATIC
VOID
OcConnectDrivers (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;
  VOID        *DriverBinding;

  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                 );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiDevicePathProtocolGuid,
      &DriverBinding
      );

    if (EFI_ERROR (Status)) {
      //
      // Calling ConnectController on non-driver results in freezes on APTIO IV.
      //
      continue;
    }

    gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
  }

  FreePool (HandleBuffer);
}

STATIC
VOID
OcProvideConsoleGop (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Gop;

  Gop = NULL;
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, &Gop);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OC: Missing GOP on ConsoleOutHandle, will install - %r\n", Status));
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, &Gop);

    if (!EFI_ERROR (Status)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
        &gST->ConsoleOutHandle,
        &gEfiGraphicsOutputProtocolGuid,
        Gop,
        NULL
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OC: Failed to install GOP on ConsoleOutHandle - %r\n", Status));
      }
    } else {
      DEBUG ((DEBUG_WARN, "OC: Missing GOP entirely - %r\n", Status));
    }
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

  if (OcConsoleControlInstallProtocol (Config->Uefi.Protocols.ConsoleControl) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install console control protocol\n"));
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
}

VOID
OcLoadBooterUefiSupport (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  OC_ABC_SETTINGS  AbcSettings;

  ZeroMem (&AbcSettings, sizeof (AbcSettings));

  AbcSettings.AvoidRuntimeDefrag     = Config->Booter.Quirks.AvoidRuntimeDefrag;
  AbcSettings.DevirtualiseMmio       = Config->Booter.Quirks.DevirtualiseMmio;
  AbcSettings.DisableSingleUser      = Config->Booter.Quirks.DisableSingleUser;
  AbcSettings.DisableVariableWrite   = Config->Booter.Quirks.DisableVariableWrite;
  AbcSettings.DiscardHibernateMap    = Config->Booter.Quirks.DiscardHibernateMap;
  AbcSettings.EnableSafeModeSlide    = Config->Booter.Quirks.EnableSafeModeSlide;
  AbcSettings.EnableWriteUnprotector = Config->Booter.Quirks.EnableWriteUnprotector;
  AbcSettings.ForceExitBootServices  = Config->Booter.Quirks.ForceExitBootServices;
  AbcSettings.ProtectCsmRegion       = Config->Booter.Quirks.ProtectCsmRegion;
  AbcSettings.ProvideCustomSlide     = Config->Booter.Quirks.ProvideCustomSlide;
  AbcSettings.SetupVirtualMap        = Config->Booter.Quirks.SetupVirtualMap;
  AbcSettings.ShrinkMemoryMap        = Config->Booter.Quirks.ShrinkMemoryMap;

  OcAbcInitialize (&AbcSettings);
}

VOID
OcLoadUefiSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  )
{
  OcReinstallProtocols (Config);

  //
  // Setup Apple bootloader specific UEFI features.
  //
  OcLoadBooterUefiSupport (Config);

  if (Config->Uefi.Quirks.IgnoreInvalidFlexRatio) {
    OcCpuCorrectFlexRatio (CpuInfo);
  }

  if (Config->Uefi.Quirks.ProvideConsoleGop) {
    OcProvideConsoleGop ();
  }

  OcConsoleControlConfigure (
    Config->Uefi.Quirks.IgnoreTextInGraphics,
    Config->Uefi.Quirks.SanitiseClearScreen,
    Config->Uefi.Quirks.ClearScreenOnModeSwitch,
    Config->Uefi.Quirks.ReplaceTabWithSpace
    );

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

  //
  // Inform allocations when we want to use lower memory only.
  //
  gRT->SetVariable (
    OC_AVOID_HIGH_ALLOC_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    OPEN_CORE_INT_NVRAM_ATTR,
    sizeof (Config->Uefi.Quirks.AvoidHighAlloc),
    &Config->Uefi.Quirks.AvoidHighAlloc
    );

  if (Config->Uefi.Quirks.ReleaseUsbOwnership
    || Config->Uefi.Quirks.ExitBootServicesDelay > 0) {
    gBS->CreateEvent (
      EVT_SIGNAL_EXIT_BOOT_SERVICES,
      TPL_NOTIFY,
      OcExitBootServicesHandler,
      Config,
      &mOcExitBootServicesEvent
      );
  }

  OcMiscUefiQuirksLoaded (Config);

  OcLoadDrivers (Storage, Config);

  DEBUG ((DEBUG_INFO, "OC: Connecting drivers...\n"));

  if (Config->Uefi.ConnectDrivers) {
    OcConnectDrivers ();
  }
}

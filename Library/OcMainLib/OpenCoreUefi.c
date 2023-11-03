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

#include <Library/OcMainLib.h>

#include <Guid/AppleVariable.h>
#include <Guid/OcVariable.h>
#include <Guid/GlobalVariable.h>

#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAfterBootCompatLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcAppleEventLib.h>
#include <Library/OcAppleImageConversionLib.h>
#include <Library/OcAudioLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcInputLib.h>
#include <Library/OcApfsLib.h>
#include <Library/OcAppleImg4Lib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcAppleSecureBootLib.h>
#include <Library/OcAppleUserInterfaceThemeLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcDataHubLib.h>
#include <Library/OcLogAggregatorLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/OcDriverConnectionLib.h>
#include <Library/OcFirmwareVolumeLib.h>
#include <Library/OcHashServicesLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcRtcLib.h>
#include <Library/OcSmcLib.h>
#include <Library/OcOSInfoLib.h>
#include <Library/OcUnicodeCollationEngGenericLib.h>
#include <Library/OcPciIoLib.h>
#include <Library/OcVariableLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Security.h>
#include <Protocol/Security2.h>
#include <Protocol/SimplePointer.h>

#define OC_EXIT_BOOT_SERVICES_HANDLER_MAX  5

STATIC EFI_EVENT_NOTIFY  mOcExitBootServicesHandlers[OC_EXIT_BOOT_SERVICES_HANDLER_MAX+1];
STATIC VOID              *mOcExitBootServicesContexts[OC_EXIT_BOOT_SERVICES_HANDLER_MAX];
STATIC UINTN             mOcExitBootServicesIndex;

VOID
OcScheduleExitBootServices (
  IN EFI_EVENT_NOTIFY  Handler,
  IN VOID              *Context
  )
{
  if (mOcExitBootServicesIndex + 1 == OC_EXIT_BOOT_SERVICES_HANDLER_MAX) {
    ASSERT (FALSE);
    return;
  }

  mOcExitBootServicesHandlers[mOcExitBootServicesIndex] = Handler;
  mOcExitBootServicesContexts[mOcExitBootServicesIndex] = Context;
  ++mOcExitBootServicesIndex;
}

VOID
OcLoadDrivers (
  IN  OC_STORAGE_CONTEXT  *Storage,
  IN  OC_GLOBAL_CONFIG    *Config,
  OUT EFI_HANDLE          **DriversToConnect  OPTIONAL,
  IN  BOOLEAN             LoadEarly
  )
{
  EFI_STATUS                 Status;
  VOID                       *Driver;
  UINT32                     DriverSize;
  UINT32                     Index;
  CHAR16                     DriverPath[OC_STORAGE_SAFE_PATH_MAX];
  EFI_HANDLE                 ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_HANDLE                 *DriversToConnectIterator;
  VOID                       *DriverBinding;
  BOOLEAN                    SkipDriver;
  OC_UEFI_DRIVER_ENTRY       *DriverEntry;
  CONST CHAR8                *DriverComment;
  CHAR8                      *DriverFileName;
  CONST CHAR8                *DriverArguments;
  CONST CHAR8                *UnescapedArguments;

  ASSERT (!LoadEarly || DriversToConnect == NULL);

  DriversToConnectIterator = NULL;
  if (DriversToConnect != NULL) {
    *DriversToConnect = NULL;
  }

  DEBUG ((DEBUG_INFO, "OC: Got %u drivers\n", Config->Uefi.Drivers.Count));

  for (Index = 0; Index < Config->Uefi.Drivers.Count; ++Index) {
    DriverEntry     = Config->Uefi.Drivers.Values[Index];
    DriverComment   = OC_BLOB_GET (&DriverEntry->Comment);
    DriverFileName  = OC_BLOB_GET (&DriverEntry->Path);
    DriverArguments = OC_BLOB_GET (&DriverEntry->Arguments);

    SkipDriver = (  !DriverEntry->Enabled
                 || (DriverFileName == NULL)
                 || (DriverFileName[0] == '\0')
                 || (LoadEarly != DriverEntry->LoadEarly)
                    );

    //
    // Avoid showing skipped lines at early load since this can be several
    // lines and is basically duplicate info; this also avoids too much
    // traffic in early log, which can be problematic on some machines.
    //
    if (!LoadEarly || !SkipDriver) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Driver %a at %u (%a) is %a\n",
        DriverFileName,
        Index,
        DriverComment,
        SkipDriver ? "skipped!" : "being loaded..."
        ));
    }

    //
    // Skip disabled drivers.
    //
    if (SkipDriver) {
      continue;
    }

    Status = OcUnicodeSafeSPrint (
               DriverPath,
               sizeof (DriverPath),
               OPEN_CORE_UEFI_DRIVER_PATH "%a",
               DriverFileName
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Driver %s%a does not fit path!\n",
        OPEN_CORE_UEFI_DRIVER_PATH,
        DriverFileName
        ));
      continue;
    }

    Driver = OcStorageReadFileUnicode (Storage, DriverPath, &DriverSize);
    if (Driver == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Driver %a at %u cannot be found!\n",
        DriverFileName,
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
    Status      = gBS->LoadImage (
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
        DriverFileName,
        Index,
        Status
        ));
      FreePool (Driver);
      continue;
    }

    if ((DriverArguments != NULL) && (DriverArguments[0] != '\0')) {
      Status = gBS->HandleProtocol (
                      ImageHandle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&LoadedImage
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "OC: Failed to locate loaded image for driver %a at %u - %r!\n",
          DriverFileName,
          Index,
          Status
          ));
        gBS->UnloadImage (ImageHandle);
        FreePool (Driver);
        continue;
      }

      UnescapedArguments = XmlUnescapeString (DriverArguments);
      if (UnescapedArguments == NULL) {
        DEBUG ((
          DEBUG_ERROR,
          "OC: Cannot unescape arguments for driver %a at %u\n",
          DriverFileName,
          Index
          ));
        gBS->UnloadImage (ImageHandle);
        FreePool (Driver);
        continue;
      }

      if (!OcAppendArgumentsToLoadedImage (LoadedImage, &UnescapedArguments, 1, TRUE)) {
        DEBUG ((
          DEBUG_ERROR,
          "OC: Unable to apply arguments to driver %a at %u - %r!\n",
          DriverFileName,
          Index,
          Status
          ));
        gBS->UnloadImage (ImageHandle);
        FreePool (Driver);
        FreePool ((CHAR8 *)UnescapedArguments);
        continue;
      }

      FreePool ((CHAR8 *)UnescapedArguments);
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
        DriverFileName,
        Index,
        Status
        ));
      gBS->UnloadImage (ImageHandle);
    }

    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Driver %a at %u is successfully loaded!\n",
        DriverFileName,
        Index
        ));

      if (DriversToConnect != NULL) {
        Status = gBS->HandleProtocol (
                        ImageHandle,
                        &gEfiDriverBindingProtocolGuid,
                        (VOID **)&DriverBinding
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
            DriverFileName,
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
  if (DriversToConnectIterator != NULL) {
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
  EFI_STATUS        Status;
  OC_GLOBAL_CONFIG  *Config;

  Config = (OC_GLOBAL_CONFIG *)Context;

  //
  // Printing from ExitBootServices is dangerous, as it may cause
  // memory reallocation, which can make ExitBootServices fail.
  // Only do that on error, which is not expected.
  //

  if (Config->Uefi.Quirks.ReleaseUsbOwnership) {
    Status = ReleaseUsbOwnership ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OC: ReleaseUsbOwnership - %r\n", Status));
    }
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
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  CONST CHAR8  *AppleEventMode;
  BOOLEAN      InstallAppleEvent;
  BOOLEAN      OverrideAppleEvent;

  if (OcAudioInstallProtocols (
        Config->Uefi.ProtocolOverrides.AppleAudio,
        Config->Uefi.Audio.DisconnectHda
        ) == NULL)
  {
    DEBUG ((DEBUG_INFO, "OC: Disabling audio in favour of firmware implementation\n"));
  }

  if (OcAppleBootPolicyInstallProtocol (Config->Uefi.ProtocolOverrides.AppleBootPolicy) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install boot policy protocol\n"));
  }

  if (OcDataHubInstallProtocol (Config->Uefi.ProtocolOverrides.DataHub) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install data hub protocol\n"));
  }

  if (OcDevicePathPropertyInstallProtocol (Config->Uefi.ProtocolOverrides.DeviceProperties) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install device properties protocol\n"));
  }

  if (OcAppleImageConversionInstallProtocol (Config->Uefi.ProtocolOverrides.AppleImageConversion) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install image conversion protocol\n"));
  }

  if (OcAppleDebugLogInstallProtocol (Config->Uefi.ProtocolOverrides.AppleDebugLog) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install debug log protocol\n"));
  }

  if (OcSmcIoInstallProtocol (Config->Uefi.ProtocolOverrides.AppleSmcIo, Config->Misc.Security.AuthRestart) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install smc i/o protocol\n"));
  }

  if (OcAppleUserInterfaceThemeInstallProtocol (Config->Uefi.ProtocolOverrides.AppleUserInterfaceTheme) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install user interface theme protocol\n"));
  }

  if (OcUnicodeCollationEngInstallProtocol (Config->Uefi.ProtocolOverrides.UnicodeCollation) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install unicode collation protocol\n"));
  }

  if (OcHashServicesInstallProtocol (Config->Uefi.ProtocolOverrides.HashServices) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install hash services protocol\n"));
  }

  if (OcAppleKeyMapInstallProtocols (Config->Uefi.ProtocolOverrides.AppleKeyMap) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install key map protocols\n"));
  }

  if (OcPciIoInstallProtocol (Config->Uefi.ProtocolOverrides.PciIo) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install cpuio/pcirootbridgeio overrides\n"));
  }

  InstallAppleEvent  = TRUE;
  OverrideAppleEvent = FALSE;

  AppleEventMode = OC_BLOB_GET (&Config->Uefi.AppleInput.AppleEvent);

  if (AsciiStrCmp (AppleEventMode, "Auto") == 0) {
  } else if (AsciiStrCmp (AppleEventMode, "OEM") == 0) {
    InstallAppleEvent = FALSE;
  } else if (AsciiStrCmp (AppleEventMode, "Builtin") == 0) {
    OverrideAppleEvent = TRUE;
  } else {
    DEBUG ((DEBUG_INFO, "OC: Invalid AppleInput AppleEvent setting %a, using Auto\n", AppleEventMode));
  }

  if (  (OcAppleEventInstallProtocol (
           InstallAppleEvent,
           OverrideAppleEvent,
           Config->Uefi.AppleInput.CustomDelays,
           Config->Uefi.AppleInput.KeyInitialDelay,
           Config->Uefi.AppleInput.KeySubsequentDelay,
           Config->Uefi.AppleInput.GraphicsInputMirroring,
           Config->Uefi.AppleInput.PointerPollMin,
           Config->Uefi.AppleInput.PointerPollMax,
           Config->Uefi.AppleInput.PointerPollMask,
           Config->Uefi.AppleInput.PointerSpeedDiv,
           Config->Uefi.AppleInput.PointerSpeedMul,
           Config->Uefi.AppleInput.PointerDwellClickTimeout,
           Config->Uefi.AppleInput.PointerDwellDoubleClickTimeout,
           Config->Uefi.AppleInput.PointerDwellRadius
           ) == NULL)
     && InstallAppleEvent)
  {
    DEBUG ((DEBUG_INFO, "OC: Failed to install apple event protocol\n"));
  }

  if (OcFirmwareVolumeInstallProtocol (Config->Uefi.ProtocolOverrides.FirmwareVolume) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install firmware volume protocol\n"));
  }

  if (OcOSInfoInstallProtocol (Config->Uefi.ProtocolOverrides.OSInfo) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install os info protocol\n"));
  }

  if (OcAppleRtcRamInstallProtocol (Config->Uefi.ProtocolOverrides.AppleRtcRam) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install rtc ram protocol\n"));
  }

  if (OcAppleFbInfoInstallProtocol (Config->Uefi.ProtocolOverrides.AppleFramebufferInfo) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install fb info protocol\n"));
  }

  if (OcAppleEg2InfoInstallProtocol (Config->Uefi.ProtocolOverrides.AppleEg2Info) == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Failed to install eg2 info protocol\n"));
  }
}

STATIC
VOID
OcLoadAppleSecureBoot (
  IN OC_GLOBAL_CONFIG  *Config,
  IN OC_CPU_INFO       *CpuInfo
  )
{
  EFI_STATUS                  Status;
  APPLE_SECURE_BOOT_PROTOCOL  *SecureBoot;
  CONST CHAR8                 *SecureBootModel;
  CONST CHAR8                 *RealSecureBootModel;
  CONST CHAR8                 *DmgLoading;
  UINT8                       SecureBootPolicy;

  SecureBootModel = OC_BLOB_GET (&Config->Misc.Security.SecureBootModel);
  if ((AsciiStrCmp (SecureBootModel, OC_SB_MODEL_DEFAULT) == 0) || (SecureBootModel[0] == '\0')) {
    SecureBootModel = OcGetDefaultSecureBootModel (Config, CpuInfo);
  }

  RealSecureBootModel = OcAppleImg4GetHardwareModel (SecureBootModel);

  if (AsciiStrCmp (SecureBootModel, OC_SB_MODEL_DISABLED) == 0) {
    SecureBootPolicy = AppleImg4SbModeDisabled;
  } else if (  (Config->Misc.Security.ApECID != 0)
            && (  (RealSecureBootModel == NULL)
               || (AsciiStrCmp (RealSecureBootModel, OC_SB_MODEL_LEGACY) != 0)))
  {
    //
    // Note, for x86legacy it is always medium policy.
    //
    SecureBootPolicy = AppleImg4SbModeFull;
  } else {
    SecureBootPolicy = AppleImg4SbModeMedium;
  }

  //
  // We blindly trust DMG contents after signature verification
  // essentially skipping secure boot in this case.
  // Do not allow enabling one but not the other.
  //
  if (SecureBootPolicy != AppleImg4SbModeDisabled) {
    DmgLoading = OC_BLOB_GET (&Config->Misc.Security.DmgLoading);
    //
    // Check against all valid values in case more are added.
    //
    if (  (AsciiStrCmp (DmgLoading, "Signed") != 0)
       && (AsciiStrCmp (DmgLoading, "Disabled") != 0)
       && (DmgLoading[0] != '\0'))
    {
      DEBUG ((DEBUG_ERROR, "OC: Cannot use Secure Boot with Any DmgLoading!\n"));
      CpuDeadLoop ();
      return;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "OC: Loading Apple Secure Boot with %a (level %u)\n",
    SecureBootModel,
    SecureBootPolicy
    ));

  if (SecureBootPolicy != AppleImg4SbModeDisabled) {
    if (RealSecureBootModel == NULL) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to find SB model %a\n", SecureBootModel));
      return;
    }

    //
    // This is what Apple does at least.
    // I believe no ECID is invalid for macOS 12.
    //
    if (  (AsciiStrCmp (RealSecureBootModel, OC_SB_MODEL_LEGACY) == 0)
       && (Config->Misc.Security.ApECID == 0))
    {
      DEBUG ((DEBUG_INFO, "OC: Discovered x86legacy with zero ECID, using system-id\n"));
      OcGetLegacySecureBootECID (Config, &Config->Misc.Security.ApECID);
    }

    //
    // Forcibly disable single user mode in Apple Secure Boot mode.
    // Previously EfiBoot correctly removed the -s argument from command-line,
    // but for some reason it does not now.
    //
    Config->Booter.Quirks.DisableSingleUser = TRUE;

    Status = OcAppleImg4BootstrapValues (RealSecureBootModel, Config->Misc.Security.ApECID);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to bootstrap IMG4 NVRAM values - %r\n", Status));
      return;
    }

    Status = OcAppleSecureBootBootstrapValues (RealSecureBootModel, Config->Misc.Security.ApECID);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to bootstrap SB NVRAM values - %r\n", Status));
      return;
    }
  }

  //
  // Provide protocols even in disabled state.
  //
  if (OcAppleImg4VerificationInstallProtocol (Config->Uefi.ProtocolOverrides.AppleImg4Verification) == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install im4m protocol\n"));
    return;
  }

  //
  // TODO: Do we need to make Windows policy configurable?
  //
  SecureBoot = OcAppleSecureBootInstallProtocol (
                 Config->Uefi.ProtocolOverrides.AppleSecureBoot,
                 SecureBootPolicy,
                 0,
                 FALSE
                 );
  if (SecureBoot == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install secure boot protocol\n"));
  }
}

STATIC
EFI_STATUS
EFIAPI
OcSecurityFileAuthentication (
  IN  CONST EFI_SECURITY_ARCH_PROTOCOL  *This,
  IN  UINT32                            AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL    *File
  )
{
  DEBUG ((DEBUG_VERBOSE, "OC: Security V1 %u\n", AuthenticationStatus));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcSecurity2FileAuthentication (
  IN CONST EFI_SECURITY2_ARCH_PROTOCOL  *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL     *File OPTIONAL,
  IN VOID                               *FileBuffer,
  IN UINTN                              FileSize,
  IN BOOLEAN                            BootPolicy
  )
{
  DEBUG ((DEBUG_VERBOSE, "OC: Security V2 %u\n", BootPolicy));
  return EFI_SUCCESS;
}

STATIC
VOID
OcInstallPermissiveSecurityPolicy (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_SECURITY_ARCH_PROTOCOL   *Security;
  EFI_SECURITY2_ARCH_PROTOCOL  *Security2;

  DEBUG ((DEBUG_INFO, "OC: Installing DISABLING secure boot policy overrides\n"));

  Status = gBS->LocateProtocol (
                  &gEfiSecurityArchProtocolGuid,
                  NULL,
                  (VOID **)&Security
                  );

  DEBUG ((DEBUG_INFO, "OC: Security arch protocol - %r\n", Status));

  if (!EFI_ERROR (Status)) {
    Security->FileAuthenticationState = OcSecurityFileAuthentication;
  }

  Status = gBS->LocateProtocol (
                  &gEfiSecurity2ArchProtocolGuid,
                  NULL,
                  (VOID **)&Security2
                  );

  DEBUG ((DEBUG_INFO, "OC: Security2 arch protocol - %r\n", Status));

  if (!EFI_ERROR (Status)) {
    Security2->FileAuthentication = OcSecurity2FileAuthentication;
  }
}

VOID
OcLoadBooterUefiSupport (
  IN OC_GLOBAL_CONFIG  *Config,
  IN OC_CPU_INFO       *CpuInfo,
  IN UINT8             *Signature
  )
{
  OC_ABC_SETTINGS        AbcSettings;
  UINT32                 Index;
  UINT32                 NextIndex;
  OC_BOOTER_PATCH_ENTRY  *UserPatch;
  OC_BOOTER_PATCH        *Patch;

  ZeroMem (&AbcSettings, sizeof (AbcSettings));

  AbcSettings.AvoidRuntimeDefrag     = Config->Booter.Quirks.AvoidRuntimeDefrag;
  AbcSettings.DevirtualiseMmio       = Config->Booter.Quirks.DevirtualiseMmio;
  AbcSettings.DisableSingleUser      = Config->Booter.Quirks.DisableSingleUser;
  AbcSettings.DisableVariableWrite   = Config->Booter.Quirks.DisableVariableWrite;
  AbcSettings.ProtectSecureBoot      = Config->Booter.Quirks.ProtectSecureBoot;
  AbcSettings.DiscardHibernateMap    = Config->Booter.Quirks.DiscardHibernateMap;
  AbcSettings.AllowRelocationBlock   = Config->Booter.Quirks.AllowRelocationBlock;
  AbcSettings.EnableSafeModeSlide    = Config->Booter.Quirks.EnableSafeModeSlide;
  AbcSettings.EnableWriteUnprotector = Config->Booter.Quirks.EnableWriteUnprotector;
  AbcSettings.ForceExitBootServices  = Config->Booter.Quirks.ForceExitBootServices;
  AbcSettings.ForceBooterSignature   = Config->Booter.Quirks.ForceBooterSignature;
  CopyMem (AbcSettings.BooterSignature, Signature, sizeof (AbcSettings.BooterSignature));
  AbcSettings.ProtectMemoryRegions   = Config->Booter.Quirks.ProtectMemoryRegions;
  AbcSettings.ProvideCustomSlide     = Config->Booter.Quirks.ProvideCustomSlide;
  AbcSettings.ProvideMaxSlide        = Config->Booter.Quirks.ProvideMaxSlide;
  AbcSettings.ProtectUefiServices    = Config->Booter.Quirks.ProtectUefiServices;
  AbcSettings.RebuildAppleMemoryMap  = Config->Booter.Quirks.RebuildAppleMemoryMap;
  AbcSettings.ResizeUsePciRbIo       = Config->Uefi.Quirks.ResizeUsePciRbIo;
  AbcSettings.ResizeAppleGpuBars     = Config->Booter.Quirks.ResizeAppleGpuBars;
  AbcSettings.SetupVirtualMap        = Config->Booter.Quirks.SetupVirtualMap;
  AbcSettings.SignalAppleOS          = Config->Booter.Quirks.SignalAppleOS;
  AbcSettings.SyncRuntimePermissions = Config->Booter.Quirks.SyncRuntimePermissions;

  //
  // Handle MmioWhitelist patches.
  //
  if (AbcSettings.DevirtualiseMmio && (Config->Booter.MmioWhitelist.Count > 0)) {
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
        (UINT32)Config->Booter.MmioWhitelist.Count
        ));
    }
  }

  //
  // Handle customised booter patches.
  //
  if (Config->Booter.Patch.Count > 0) {
    AbcSettings.BooterPatches = AllocateZeroPool (
                                  Config->Booter.Patch.Count * sizeof (AbcSettings.BooterPatches[0])
                                  );

    if (AbcSettings.BooterPatches != NULL) {
      NextIndex = 0;
      for (Index = 0; Index < Config->Booter.Patch.Count; ++Index) {
        Patch     = &AbcSettings.BooterPatches[NextIndex];
        UserPatch = Config->Booter.Patch.Values[Index];

        if (!UserPatch->Enabled) {
          continue;
        }

        //
        // Ignore patch if:
        // - There is nothing to replace.
        // - Find and replace mismatch in size.
        // - Mask and ReplaceMask mismatch in size when are available.
        //
        if (  (UserPatch->Replace.Size == 0)
           || (UserPatch->Find.Size != UserPatch->Replace.Size)
           || ((UserPatch->Mask.Size > 0) && (UserPatch->Find.Size != UserPatch->Mask.Size))
           || ((UserPatch->ReplaceMask.Size > 0) && (UserPatch->Find.Size != UserPatch->ReplaceMask.Size)))
        {
          DEBUG ((DEBUG_ERROR, "OC: Booter patch %u is borked\n", Index));
          continue;
        }

        //
        // Also, ignore patch on mismatched architecture.
        //
        Patch->Arch = OC_BLOB_GET (&UserPatch->Arch);
        if ((Patch->Arch[0] != '\0') && (AsciiStrCmp (Patch->Arch, "Any") != 0)) {
 #if defined (MDE_CPU_X64)
          if (AsciiStrCmp (Patch->Arch, "x86_64") != 0) {
            continue;
          }

 #elif defined (MDE_CPU_IA32)
          if (AsciiStrCmp (Patch->Arch, "i386") != 0) {
            continue;
          }

 #else
          #error "Unsupported architecture"
 #endif
        }

        //
        // Here we simply receive Identifier from user,
        // and it will be parsed by the internal patch function.
        //
        Patch->Identifier = OC_BLOB_GET (&UserPatch->Identifier);

        Patch->Find    = OC_BLOB_GET (&UserPatch->Find);
        Patch->Replace = OC_BLOB_GET (&UserPatch->Replace);

        Patch->Comment = OC_BLOB_GET (&UserPatch->Comment);

        if (UserPatch->Mask.Size > 0) {
          Patch->Mask = OC_BLOB_GET (&UserPatch->Mask);
        }

        if (UserPatch->ReplaceMask.Size > 0) {
          Patch->ReplaceMask = OC_BLOB_GET (&UserPatch->ReplaceMask);
        }

        Patch->Size  = UserPatch->Replace.Size;
        Patch->Count = UserPatch->Count;
        Patch->Skip  = UserPatch->Skip;
        Patch->Limit = UserPatch->Limit;

        ++NextIndex;
      }

      AbcSettings.BooterPatchesSize = NextIndex;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Failed to allocate %u slots for user booter patches\n",
        (UINT32)Config->Booter.Patch.Count
        ));
    }
  }

  AbcSettings.ExitBootServicesHandlers        = mOcExitBootServicesHandlers;
  AbcSettings.ExitBootServicesHandlerContexts = mOcExitBootServicesContexts;

  OcAbcInitialize (&AbcSettings, CpuInfo);
}

VOID
OcReserveMemory (
  IN OC_GLOBAL_CONFIG  *Config
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  ReservedAddress;
  EFI_MEMORY_TYPE       RsvdMemoryType;
  CHAR8                 *RsvdMemoryTypeStr;

  for (Index = 0; Index < Config->Uefi.ReservedMemory.Count; ++Index) {
    if (!Config->Uefi.ReservedMemory.Values[Index]->Enabled) {
      continue;
    }

    if (  ((Config->Uefi.ReservedMemory.Values[Index]->Address & (BASE_4KB - 1)) != 0)
       || ((Config->Uefi.ReservedMemory.Values[Index]->Size & (BASE_4KB - 1)) != 0))
    {
      Status = EFI_INVALID_PARAMETER;
    } else {
      RsvdMemoryTypeStr = OC_BLOB_GET (&Config->Uefi.ReservedMemory.Values[Index]->Type);

      Status = OcDescToMemoryType (RsvdMemoryTypeStr, &RsvdMemoryType);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OC: Invalid ReservedMemory Type: %a\n", RsvdMemoryTypeStr));
        RsvdMemoryType = EfiReservedMemoryType;
      }

      ReservedAddress = Config->Uefi.ReservedMemory.Values[Index]->Address;
      Status          = gBS->AllocatePages (
                               AllocateAddress,
                               RsvdMemoryType,
                               (UINTN)EFI_SIZE_TO_PAGES (Config->Uefi.ReservedMemory.Values[Index]->Size),
                               &ReservedAddress
                               );
    }

    DEBUG ((
      DEBUG_INFO,
      "OC: Reserving region %Lx of %Lx size - %r\n",
      Config->Uefi.ReservedMemory.Values[Index]->Address,
      Config->Uefi.ReservedMemory.Values[Index]->Size,
      Status
      ));
  }
}

VOID
OcLoadUefiSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo,
  IN UINT8               *Signature
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *DriversToConnect;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleCount;
  EFI_EVENT   Event;
  BOOLEAN     AccelEnabled;

  OcReinstallProtocols (Config);

  OcImageLoaderInit (Config->Booter.Quirks.ProtectUefiServices, Config->Booter.Quirks.FixupAppleEfiImages);

  OcLoadAppleSecureBoot (Config, CpuInfo);

  OcLoadUefiInputSupport (Config);

  //
  // Setup Apple bootloader specific UEFI features.
  //
  OcLoadBooterUefiSupport (Config, CpuInfo, Signature);

  if (Config->Uefi.Quirks.ActivateHpetSupport) {
    ActivateHpetSupport ();
  }

  if (Config->Uefi.Quirks.IgnoreInvalidFlexRatio) {
    OcCpuCorrectFlexRatio (CpuInfo);
  }

  if (Config->Uefi.Quirks.EnableVmx) {
    Status = OcCpuEnableVmx ();
    DEBUG ((EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO, "OC: Enable VMX - %r\n", Status));
  }

  if (Config->Uefi.Quirks.TscSyncTimeout > 0) {
    OcCpuCorrectTscSync (CpuInfo, Config->Uefi.Quirks.TscSyncTimeout);
  }

  DEBUG ((
    DEBUG_INFO,
    "OC: RequestBootVarRouting %d\n",
    Config->Uefi.Quirks.RequestBootVarRouting
    ));

  //
  // Inform platform support whether we want Boot#### routing or not.
  //
  OcSetSystemVariable (
    OC_BOOT_REDIRECT_VARIABLE_NAME,
    OPEN_CORE_INT_NVRAM_ATTR,
    sizeof (Config->Uefi.Quirks.RequestBootVarRouting),
    &Config->Uefi.Quirks.RequestBootVarRouting,
    NULL
    );

  if (Config->Uefi.Quirks.UnblockFsConnect) {
    OcUnblockUnmountedPartitions ();
  }

  if (Config->Uefi.Quirks.DisableSecurityPolicy) {
    OcInstallPermissiveSecurityPolicy ();
  }

  OcForgeUefiSupport (Config->Uefi.Quirks.ForgeUefiSupport, FALSE);

  if (Config->Uefi.Quirks.ReloadOptionRoms) {
    OcReloadOptionRoms ();
  }

  if (Config->Uefi.Quirks.EnableVectorAcceleration) {
    AccelEnabled = TryEnableAccel ();
    DEBUG ((DEBUG_INFO, "OC: AVX enabled - %u\n", AccelEnabled));
  }

  if (  (Config->Uefi.Quirks.ResizeGpuBars >= 0)
     && (Config->Uefi.Quirks.ResizeGpuBars < PciBarTotal))
  {
    DEBUG ((DEBUG_INFO, "OC: Setting GPU BARs to %d\n", Config->Uefi.Quirks.ResizeGpuBars));
    ResizeGpuBars (Config->Uefi.Quirks.ResizeGpuBars, TRUE, Config->Uefi.Quirks.ResizeUsePciRbIo);
  }

  OcMiscUefiQuirksLoaded (Config);

  //
  // Reserve requested memory regions
  //
  OcReserveMemory (Config);

  if (Config->Uefi.ConnectDrivers) {
    OcLoadDrivers (Storage, Config, &DriversToConnect, FALSE);
    DEBUG ((DEBUG_INFO, "OC: Connecting drivers...\n"));
    if (DriversToConnect != NULL) {
      OcRegisterDriversToHighestPriority (DriversToConnect);
      //
      // DriversToConnect is not freed as it is owned by OcRegisterDriversToHighestPriority.
      //
    }

    if (Config->Uefi.Output.ReconnectGraphicsOnConnect) {
      DEBUG ((DEBUG_INFO, "OC: Disconnecting graphics drivers...\n"));
      OcDisconnectGraphicsDrivers ();
      DEBUG ((DEBUG_INFO, "OC: Disconnecting graphics drivers done...\n"));
    }

    OcConnectDrivers ();
    DEBUG ((DEBUG_INFO, "OC: Connecting drivers done...\n"));
  } else {
    OcLoadDrivers (Storage, Config, NULL, FALSE);
  }

  DEBUG_CODE_BEGIN ();
  HandleCount  = 0;
  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiSimplePointerProtocolGuid,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  DEBUG ((DEBUG_INFO, "OC: Found %u pointer devices - %r\n", HandleCount, Status));
  if (!EFI_ERROR (Status)) {
    FreePool (HandleBuffer);
  }

  DEBUG_CODE_END ();

  if (Config->Uefi.Apfs.EnableJumpstart) {
    OcApfsConfigure (
      Config->Uefi.Apfs.MinVersion,
      Config->Uefi.Apfs.MinDate,
      Config->Misc.Security.ScanPolicy,
      Config->Uefi.Apfs.GlobalConnect,
      Config->Uefi.Quirks.UnblockFsConnect,
      Config->Uefi.Apfs.HideVerbose
      );

    OcApfsConnectDevices (
      Config->Uefi.Apfs.JumpstartHotPlug
      );
  }

  OcLoadUefiOutputSupport (Storage, Config);

  OcLoadUefiAudioSupport (Storage, Config);

  gBS->CreateEvent (
         EVT_SIGNAL_EXIT_BOOT_SERVICES,
         TPL_CALLBACK,
         OcExitBootServicesHandler,
         Config,
         &Event
         );
}

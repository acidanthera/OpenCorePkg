/** @file
  Methods wrapping calls to legacy NVRAM protocol from OpenCore.

  Copyright (C) 2022, mikebeaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Protocol/OcVariableRuntime.h>
#include <Protocol/OcFirmwareRuntime.h>
#include <Library/OcDirectResetLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC
EFI_STATUS
InternalLocateVariableRuntimeProtocol (
  OC_VARIABLE_RUNTIME_PROTOCOL  **OcVariableRuntimeProtocol
  )
{
  EFI_STATUS  Status;

  ASSERT (OcVariableRuntimeProtocol != NULL);

  Status = gBS->LocateProtocol (
                  &gOcVariableRuntimeProtocolGuid,
                  NULL,
                  (VOID **)OcVariableRuntimeProtocol
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCVAR: Locate emulated NVRAM protocol - %r\n", Status));
    return Status;
  }

  if ((*OcVariableRuntimeProtocol)->Revision != OC_VARIABLE_RUNTIME_PROTOCOL_REVISION) {
    DEBUG ((
      DEBUG_WARN,
      "OCVAR: Emulated NVRAM protocol incompatible revision %d != %d\n",
      (*OcVariableRuntimeProtocol)->Revision,
      OC_VARIABLE_RUNTIME_PROTOCOL_REVISION
      ));
    return EFI_UNSUPPORTED;
  }

  return Status;
}

//
// TODO: We should normally strictly avoid passing OpenCore config structures outside of OcMainLib, but the
// emulated NVRAM protocol is so tightly tied to the legacy NVRAM map that we (currently?) do so.
//
VOID
OcLoadLegacyNvram (
  IN OC_STORAGE_CONTEXT   *Storage,
  IN OC_NVRAM_LEGACY_MAP  *LegacyMap,
  IN BOOLEAN              LegacyOverwrite,
  IN BOOLEAN              RequestBootVarRouting
  )
{
  EFI_STATUS                    Status;
  OC_VARIABLE_RUNTIME_PROTOCOL  *OcVariableRuntimeProtocol;
  OC_FIRMWARE_RUNTIME_PROTOCOL  *FwRuntime;
  OC_FWRT_CONFIG                FwrtConfig;

  Status = InternalLocateVariableRuntimeProtocol (&OcVariableRuntimeProtocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // It is not really required to support boot var routing with emulated NVRAM (since there are
  // no firmware NVRAM boot vars used outside of OpenCore to avoid trashing), but having working
  // support is more convenient when switching back and forth between emulated and non-emulated
  // NVRAM, i.e. one less thing to have to remember to switch, since with this code everything
  // works as expected with or without RequestBootVarRouting. (Without it, boot entries do not
  // restore to the right place when RequestBootVarRouting is enabled.)
  // OpenRuntime.efi must be loaded early, but after OpenVariableRuntimeDxe.efi, for this to work.
  //
  if (RequestBootVarRouting) {
    Status = gBS->LocateProtocol (
                    &gOcFirmwareRuntimeProtocolGuid,
                    NULL,
                    (VOID **)&FwRuntime
                    );

    if (!EFI_ERROR (Status) && (FwRuntime->Revision == OC_FIRMWARE_RUNTIME_REVISION)) {
      FwRuntime->GetCurrent (&FwrtConfig);
      if (FwrtConfig.BootVariableRedirect) {
        DEBUG ((DEBUG_INFO, "OCVAR: Found FW NVRAM, redirect already present %d\n", FwrtConfig.BootVariableRedirect));
        FwRuntime = NULL;
      } else {
        FwrtConfig.BootVariableRedirect = TRUE;
        FwRuntime->SetOverride (&FwrtConfig);
        DEBUG ((DEBUG_INFO, "OCVAR: Found FW NVRAM, forcing redirect %d\n", FwrtConfig.BootVariableRedirect));
      }
    } else {
      FwRuntime = NULL;
      DEBUG ((DEBUG_INFO, "OCVAR: Missing FW NVRAM, going on...\n"));
    }
  } else {
    FwRuntime = NULL;
  }

  DEBUG ((DEBUG_INFO, "OCVAR: Loading NVRAM from storage...\n"));

  Status = OcVariableRuntimeProtocol->LoadNvram (Storage, LegacyMap, LegacyOverwrite);

  if (EFI_ERROR (Status)) {
    //
    // Make this INFO rather then WARN, since it is expected on first boot with emulated NVRAM.
    //
    DEBUG ((DEBUG_INFO, "OCVAR: Emulated NVRAM load failed - %r\n", Status));
  }

  if (FwRuntime != NULL) {
    DEBUG ((DEBUG_INFO, "OCVAR: Restoring FW NVRAM...\n"));
    FwRuntime->SetOverride (NULL);
  }
}

VOID
EFIAPI
OcSaveLegacyNvram (
  VOID
  )
{
  EFI_STATUS                    Status;
  OC_VARIABLE_RUNTIME_PROTOCOL  *OcVariableRuntimeProtocol;

  Status = InternalLocateVariableRuntimeProtocol (&OcVariableRuntimeProtocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  DEBUG ((DEBUG_INFO, "OCVAR: Saving NVRAM to storage...\n"));

  Status = OcVariableRuntimeProtocol->SaveNvram ();

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCVAR: Emulated NVRAM save failed - %r\n", Status));
  }
}

VOID
EFIAPI
OcResetLegacyNvram (
  VOID
  )
{
  EFI_STATUS                    Status;
  OC_VARIABLE_RUNTIME_PROTOCOL  *OcVariableRuntimeProtocol;

  Status = InternalLocateVariableRuntimeProtocol (&OcVariableRuntimeProtocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  DEBUG ((DEBUG_INFO, "OCVAR: Resetting NVRAM storage...\n"));

  Status = OcVariableRuntimeProtocol->ResetNvram ();

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCVAR: Emulated NVRAM reset failed - %r\n", Status));
  }

  DirectResetCold ();
}

VOID
EFIAPI
OcSwitchToFallbackLegacyNvram (
  VOID
  )
{
  EFI_STATUS                    Status;
  OC_VARIABLE_RUNTIME_PROTOCOL  *OcVariableRuntimeProtocol;

  Status = InternalLocateVariableRuntimeProtocol (&OcVariableRuntimeProtocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  DEBUG ((DEBUG_INFO, "OCVAR: Switching to fallback NVRAM storage...\n"));

  Status = OcVariableRuntimeProtocol->SwitchToFallback ();

  if (EFI_ERROR (Status)) {
    DEBUG ((
      Status == EFI_ALREADY_STARTED ? DEBUG_INFO : DEBUG_WARN,
      "OCVAR: Emulated NVRAM switch to fallback failed - %r\n",
      Status
      ));
  }
}

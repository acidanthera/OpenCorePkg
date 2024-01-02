/** @file
  Reset system.

  Copyright (c) 2020-2024, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDirectResetLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Guid/GlobalVariable.h>

EFI_STATUS
EFIAPI
OcResetToFirmwareSettingsSupported (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      OsIndications;
  UINT32      Attr;
  UINTN       DataSize;

  DataSize = sizeof (OsIndications);
  Status   = gRT->GetVariable (
                    EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME,
                    &gEfiGlobalVariableGuid,
                    &Attr,
                    &DataSize,
                    &OsIndications
                    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCRST: Failed to acquire firmware features - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  if ((OsIndications & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) == 0) {
    DEBUG ((DEBUG_WARN, "OCRST: Firmware do not support EFI_OS_INDICATIONS_BOOT_TO_FW_UI - %r\n", Status));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcResetSystem (
  IN CHAR16  *Mode
  )
{
  EFI_STATUS      Status;
  EFI_RESET_TYPE  ResetMode;
  UINT64          OsIndications;
  UINT32          Attr;
  UINTN           DataSize;

  if (OcStriCmp (Mode, L"firmware") == 0) {
    DEBUG ((DEBUG_INFO, "OCRST: Entering firmware...\n"));
    Status = OcResetToFirmwareSettingsSupported ();
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DataSize = sizeof (OsIndications);
    Status   = gRT->GetVariable (
                      EFI_OS_INDICATIONS_VARIABLE_NAME,
                      &gEfiGlobalVariableGuid,
                      &Attr,
                      &DataSize,
                      &OsIndications
                      );
    if (!EFI_ERROR (Status)) {
      OsIndications |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
    } else {
      OsIndications = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
    }

    Status = gRT->SetVariable (
                    EFI_OS_INDICATIONS_VARIABLE_NAME,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof (OsIndications),
                    &OsIndications
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCRST: Failed to set EFI_OS_INDICATIONS_BOOT_TO_FW_UI - %r\n", Status));
      return EFI_ABORTED;
    }

    Mode = L"coldreset";
  }

  if (OcStriCmp (Mode, L"coldreset") == 0) {
    DEBUG ((DEBUG_INFO, "OCRST: Perform cold reset...\n"));
    ResetMode = EfiResetCold;
  } else if (OcStriCmp (Mode, L"warmreset") == 0) {
    DEBUG ((DEBUG_INFO, "OCRST: Perform warm reset...\n"));
    ResetMode = EfiResetWarm;
  } else if (OcStriCmp (Mode, L"shutdown") == 0) {
    DEBUG ((DEBUG_INFO, "OCRST: Perform shutdown...\n"));
    ResetMode = EfiResetShutdown;
  } else {
    DEBUG ((DEBUG_INFO, "OCRST: Unknown argument %s, defaulting to cold reset...\n", Mode));
    ResetMode = EfiResetCold;
  }

  gRT->ResetSystem (
         ResetMode,
         EFI_SUCCESS,
         0,
         NULL
         );

  DEBUG ((DEBUG_INFO, "OCRST: Failed to reset, trying direct\n"));

  DirectResetCold ();

  DEBUG ((DEBUG_INFO, "OCRST: Failed to reset directly, entering dead loop\n"));

  CpuDeadLoop ();

  return EFI_SUCCESS; ///< Unreachable
}

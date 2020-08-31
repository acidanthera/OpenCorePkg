/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/AppleVariable.h>

#include <IndustryStandard/AppleHibernate.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcRtcLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetRecoveryInitiator (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathSize;

  Status = GetVariable2 (
    APPLE_RECOVERY_BOOT_INITIATOR_VARIABLE_NAME,
    &gAppleVendorVariableGuid,
    (VOID **) &DevicePath,
    &DevicePathSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCB: No recovery initiator found - %r\n", Status));
    return NULL;
  }

  //
  // Also delete recovery initiator just in case.
  //
  gRT->SetVariable (
    APPLE_RECOVERY_BOOT_INITIATOR_VARIABLE_NAME,
    &gAppleVendorVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    0,
    NULL
    );

  if (!IsDevicePathValid (DevicePath, DevicePathSize)) {
    DEBUG ((DEBUG_INFO, "OCB: Recovery initiator (%u) is invalid\n", (UINT32) DevicePathSize));
    FreePool (DevicePath);
    return NULL;
  }

  DebugPrintDevicePath (DEBUG_INFO, "OC: Recovery initiator", DevicePath);
  return DevicePath;
}

EFI_STATUS
OcHandleRecoveryRequest (
  OUT EFI_DEVICE_PATH_PROTOCOL  **Initiator  OPTIONAL
  )
{
  EFI_STATUS             Status;
  CHAR8                  *RecoveryBootMode;
  UINTN                  RecoveryBootModeSize;

  //
  // Provide basic support for recovery-boot-mode variable, which is meant
  // to perform one-time recovery boot. In general BootOrder and BootNext
  // are set to the recovery path, but this is not the case for secure-boot.
  //
  RecoveryBootModeSize = 0;
  Status = gRT->GetVariable (
    APPLE_RECOVERY_BOOT_MODE_VARIABLE_NAME,
    &gAppleBootVariableGuid,
    NULL,
    &RecoveryBootModeSize,
    NULL
    );

  //
  // Initialise right away.
  //
  if (Initiator != NULL) {
    *Initiator = NULL;
  }

  //
  // No need for recovery, this is a normal boot.
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_FOUND;
  }

  //
  // Debug recovery-boot-mode contents.
  //
  DEBUG_CODE_BEGIN ();
  RecoveryBootMode = AllocatePool (RecoveryBootModeSize + 1);
  if (RecoveryBootMode != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Failed to allocate recovery-boot-mode %u\n",
      (UINT32) (RecoveryBootModeSize + 1)
      ));

    Status = gRT->GetVariable (
      APPLE_RECOVERY_BOOT_MODE_VARIABLE_NAME,
      &gAppleBootVariableGuid,
      NULL,
      &RecoveryBootModeSize,
      RecoveryBootMode
      );
    if (!EFI_ERROR (Status)) {
      //
      // Ensure null-termination.
      //
      RecoveryBootMode[RecoveryBootModeSize] = '\0';
      DEBUG ((
        DEBUG_INFO,
        "OCB: recovery-boot-mode %u = %a - %r\n",
        (UINT32) RecoveryBootModeSize,
        RecoveryBootMode,
        Status
        ));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Failed to obtain recovery-boot-mode %u - %r\n",
        (UINT32) RecoveryBootModeSize,
        Status
        ));
    }

    FreePool (RecoveryBootMode);
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Failed to allocate recovery-boot-mode %u\n",
      (UINT32) (RecoveryBootModeSize + 1)
      ));
  }
  DEBUG_CODE_END ();

  if (Initiator != NULL) {
    *Initiator = InternalGetRecoveryInitiator ();
  }

  //
  // Delete recovery-boot-mode.
  // Failing to do so will result in bootloop at least when SecureBoot is enabled.
  // In other cases EfiBoot may try to chainload into recovery, but it is unreliable.
  //
  gRT->SetVariable (
    APPLE_RECOVERY_BOOT_MODE_VARIABLE_NAME,
    &gAppleBootVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    0,
    NULL
    );

  return EFI_SUCCESS;
}
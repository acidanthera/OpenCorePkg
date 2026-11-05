/** @file
  Apple Firmware Password protocol implementation.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OcAppleFirmwarePasswordLib.h>
#include <Library/OcMiscLib.h>

STATIC
EFI_STATUS
EFIAPI
AppleFirmwarePasswordIsSet (
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL  *This
  )
{
  if ((This == NULL) || (This->Signature != APPLE_FIRMWARE_PASSWORD_PROTOCOL_SIGNATURE)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
AppleFirmwarePasswordCheck (
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL  *This,
  IN     CONST CHAR16                      *Password
  )
{
  if ((This == NULL) || (Password == NULL) || (This->Signature != APPLE_FIRMWARE_PASSWORD_PROTOCOL_SIGNATURE)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
AppleFirmwarePasswordGetMode (
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL   *This,
  OUT    APPLE_FIRMWARE_PASSWORD_MODE_INFO  *Info
  )
{
  if ((This == NULL) || (Info == NULL) || (This->Signature != APPLE_FIRMWARE_PASSWORD_PROTOCOL_SIGNATURE)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Info, sizeof (*Info));

  return EFI_SUCCESS;
}

STATIC APPLE_FIRMWARE_PASSWORD_PROTOCOL  mAppleFirmwarePasswordProtocol = {
  APPLE_FIRMWARE_PASSWORD_PROTOCOL_SIGNATURE,
  APPLE_FIRMWARE_PASSWORD_PROTOCOL_REVISION,
  AppleFirmwarePasswordIsSet,
  AppleFirmwarePasswordCheck,
  AppleFirmwarePasswordGetMode
};

APPLE_FIRMWARE_PASSWORD_PROTOCOL *
OcAppleFirmwarePasswordInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                        Status;
  APPLE_FIRMWARE_PASSWORD_PROTOCOL  *FwPass;
  UINTN                             Index;
  UINTN                             HandleCount;
  EFI_HANDLE                        *HandleBuffer;
  EFI_HANDLE                        NewHandle;

  FwPass       = NULL;
  HandleBuffer = NULL;
  HandleCount  = 0;
  Index        = 0;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleFirmwarePasswordProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCAFWP: Uninstall failed - %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateHandleBuffer (ByProtocol, &gAppleFirmwarePasswordProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (!EFI_ERROR (Status) && (HandleCount != 0) && (HandleBuffer != NULL)) {
      for (Index = 0; Index < HandleCount; Index++) {
        FwPass = NULL;
        Status = OcHandleProtocolFallback (HandleBuffer[Index], &gAppleFirmwarePasswordProtocolGuid, (VOID **)&FwPass);
        if (!EFI_ERROR (Status) && (FwPass != NULL) && (FwPass->Signature == APPLE_FIRMWARE_PASSWORD_PROTOCOL_SIGNATURE)) {
          FreePool (HandleBuffer);
          return FwPass;
        }
      }

      FreePool (HandleBuffer);
    }
  }

  NewHandle = NULL;
  Status    = gBS->InstallMultipleProtocolInterfaces (
                     &NewHandle,
                     &gAppleFirmwarePasswordProtocolGuid,
                     &mAppleFirmwarePasswordProtocol,
                     NULL
                     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleFirmwarePasswordProtocol;
}

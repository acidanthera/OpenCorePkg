/** @file
  Apple Firmware Password protocol implementation.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OcAppleFirmwarePasswordLib.h>

STATIC
EFI_STATUS
EFIAPI
AppleFirmwarePasswordIsSet (
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleFirmwarePasswordCheck (
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL  *This,
  IN     CONST CHAR16                      *Password
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleFirmwarePasswordGetMode (
  IN     APPLE_FIRMWARE_PASSWORD_PROTOCOL   *This,
  OUT    APPLE_FIRMWARE_PASSWORD_MODE_INFO  *Info
  )
{
  return EFI_UNSUPPORTED;
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
  VOID
  )
{
  EFI_STATUS                        Status;
  APPLE_FIRMWARE_PASSWORD_PROTOCOL  *FwPass;
  EFI_HANDLE                        NewHandle;

  FwPass = NULL;

  Status = gBS->LocateProtocol (
                  &gAppleFirmwarePasswordProtocolGuid,
                  NULL,
                  (VOID **)&FwPass
                  );
  if (!EFI_ERROR (Status)) {
    return FwPass;
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

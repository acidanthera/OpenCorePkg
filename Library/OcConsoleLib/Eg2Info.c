/** @file
  AppleEg2Info protocol implementation.

  Copyright (c) 2020, vit9696. All rights reserved.
  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "OcConsoleLibInternal.h"

#include <Guid/AppleVariable.h>

#include <Protocol/AppleEg2Info.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC UINT32  mRotation = AppleDisplayRotate0;

STATIC
EFI_STATUS
EFIAPI
AppleEg2Unknown1 (
  IN   APPLE_EG2_INFO_PROTOCOL  *This,
  IN   EFI_HANDLE               GpuHandle
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2GetPlatformInfo (
  IN   APPLE_EG2_INFO_PROTOCOL  *This,
  IN   EFI_HANDLE               GpuHandle,
  OUT  VOID                     *Data,
  OUT  UINTN                    *Size
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2StartupDisplay (
  IN   APPLE_EG2_INFO_PROTOCOL  *This,
  IN   EFI_HANDLE               GpuHandle,
  OUT  VOID                     *Unk1,
  OUT  VOID                     *Unk2,
  OUT  VOID                     *Unk3
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2GetHibernation (
  IN   APPLE_EG2_INFO_PROTOCOL  *This,
  OUT  BOOLEAN                  *Hibernated
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2GetRotation (
  IN   APPLE_EG2_INFO_PROTOCOL  *This,
  OUT  UINT32                   *Rotation
  )
{
  if ((This == NULL) || (Rotation == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Rotation = mRotation;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2DisableDisplayPolling (
  IN APPLE_EG2_INFO_PROTOCOL  *This,
  IN BOOLEAN                  Disable
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
APPLE_EG2_INFO_PROTOCOL
  mAppleEg2Info = {
  APPLE_EG2_INFO_PROTOCOL_REVISION,
  NULL,
  AppleEg2Unknown1,
  AppleEg2GetPlatformInfo,
  AppleEg2StartupDisplay,
  AppleEg2GetHibernation,
  AppleEg2GetRotation,
  AppleEg2DisableDisplayPolling
};

APPLE_EG2_INFO_PROTOCOL *
OcAppleEg2InfoInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS               Status;
  APPLE_EG2_INFO_PROTOCOL  *Protocol;
  EFI_HANDLE               NewHandle;
  UINTN                    Size;
  UINT32                   Attributes;
  UINT32                   Rotation;

  DEBUG ((DEBUG_VERBOSE, "OcAppleEg2InfoInstallProtocol\n"));

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleEg2InfoProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCEG: Uninstall failed - %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
                    &gAppleEg2InfoProtocolGuid,
                    NULL,
                    (VOID *)&Protocol
                    );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Attributes = 0;
  Size       = sizeof (Rotation);
  Status     = gRT->GetVariable (
                      APPLE_FORCE_DISPLAY_ROTATION_VARIABLE_NAME,
                      &gAppleBootVariableGuid,
                      &Attributes,
                      &Size,
                      &Rotation
                      );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCEG: Discovered rotate NVRAM override to %u\n", Rotation));
    if (Rotation == 90) {
      mRotation = AppleDisplayRotate90;
    } else if (Rotation == 180) {
      mRotation = AppleDisplayRotate180;
    } else if (Rotation == 270) {
      mRotation = AppleDisplayRotate270;
    }
  }

  NewHandle = NULL;
  Status    = gBS->InstallMultipleProtocolInterfaces (
                     &NewHandle,
                     &gAppleEg2InfoProtocolGuid,
                     (VOID *)&mAppleEg2Info,
                     NULL
                     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleEg2Info;
}

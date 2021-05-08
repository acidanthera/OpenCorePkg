/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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

STATIC UINT32 mRotation = AppleDisplayRotate0;

STATIC
EFI_STATUS
EFIAPI
AppleEg2Unknown1 (
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  IN   EFI_HANDLE                   GpuHandle
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2GetPlatformInfo (
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  IN   EFI_HANDLE                   GpuHandle,
  OUT  VOID                         *Data,
  OUT  UINTN                        *Size
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2StartupDisplay (
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  IN   EFI_HANDLE                   GpuHandle,
  OUT  VOID                         *Unk1,
  OUT  VOID                         *Unk2,
  OUT  VOID                         *Unk3
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2GetHibernation (
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  OUT  BOOLEAN                      *Hibernated
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AppleEg2GetRotation (
  IN   APPLE_EG2_INFO_PROTOCOL      *This,
  OUT  UINT32                       *Rotation
  )
{
  if (This == NULL || Rotation == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Rotation = mRotation;
  return EFI_SUCCESS;
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
  AppleEg2GetRotation
};

APPLE_EG2_INFO_PROTOCOL *
OcAppleEg2InfoInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                      Status;
  APPLE_EG2_INFO_PROTOCOL         *Protocol;
  EFI_HANDLE                      NewHandle;
  UINTN                           Size;
  UINT32                          Attributes;
  UINT32                          Rotation;

  DEBUG ((DEBUG_VERBOSE, "OcAppleFbInfoInstallProtocol\n"));

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleEg2InfoProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCOS: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleEg2InfoProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Attributes = 0;
  Size = sizeof (Rotation);
  Status = gRT->GetVariable (
    APPLE_FORCE_DISPLAY_ROTATION_VARIABLE_NAME,
    &gAppleBootVariableGuid,
    &Attributes,
    &Size,
    &Rotation
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCOS: Discovered rotate NVRAM override to %u\n", Rotation));
    if (Rotation == 90) {
      mRotation = AppleDisplayRotate90;
    } else if (Rotation == 180) {
      mRotation = AppleDisplayRotate180;
    } else if (Rotation == 270) {
      mRotation = AppleDisplayRotate270;
    }
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
     &NewHandle,
     &gAppleEg2InfoProtocolGuid,
     (VOID *) &mAppleEg2Info,
     NULL
     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleEg2Info;
}

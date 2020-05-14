/** @file
AppleUserInterfaceTheme protocol

Copyright (c) 2016-2018, vit9696. All rights reserved.<BR>
Portions copyright (c) 2018, savvas. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/AppleVariable.h>
#include <Library/DebugLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcAppleUserInterfaceThemeLib.h>
#include <Protocol/UserInterfaceTheme.h>

STATIC UINT32 mCurrentColor;

STATIC
EFI_STATUS
EFIAPI
UserInterfaceThemeGetColor (
  OUT UINT32  *Color
  )
{
  if (Color == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Color = mCurrentColor;
  return EFI_SUCCESS;
}

STATIC EFI_USER_INTERFACE_THEME_PROTOCOL mAppleUserInterfaceThemeProtocol = {
  USER_THEME_INTERFACE_PROTOCOL_REVISION,
  UserInterfaceThemeGetColor
};

EFI_USER_INTERFACE_THEME_PROTOCOL *
OcAppleUserInterfaceThemeInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                         Status;
  UINT32                             Color;
  UINTN                              DataSize;
  EFI_USER_INTERFACE_THEME_PROTOCOL  *EfiUiInterface;
  EFI_HANDLE                         NewHandle;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gEfiUserInterfaceThemeProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCUT: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gEfiUserInterfaceThemeProtocolGuid,
      NULL,
      (VOID **) &EfiUiInterface
      );
    if (!EFI_ERROR (Status)) {
      return EfiUiInterface;
    }
  }

  //
  // Default color is black
  //
  mCurrentColor = APPLE_COLOR_SYRAH_BLACK;

  DataSize = sizeof (Color);
  Status = gRT->GetVariable (
    APPLE_DEFAULT_BACKGROUND_COLOR_VARIABLE_NAME,
    &gAppleVendorVariableGuid,
    0,
    &DataSize,
    &Color
    );
  if (!EFI_ERROR (Status)) {
    mCurrentColor = Color;
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gEfiUserInterfaceThemeProtocolGuid,
    &mAppleUserInterfaceThemeProtocol,
    NULL
    );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleUserInterfaceThemeProtocol;
}

/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "OcConsoleLibInternal.h"

#include <Protocol/AppleFramebufferInfo.h>
#include <Protocol/GraphicsOutput.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
EFI_STATUS
EFIAPI
AppleFramebufferGetInfo (
  IN   APPLE_FRAMEBUFFER_INFO_PROTOCOL  *This,
  OUT  EFI_PHYSICAL_ADDRESS             *FramebufferBase,
  OUT  UINT32                           *FramebufferSize,
  OUT  UINT32                           *ScreenRowBytes,
  OUT  UINT32                           *ScreenWidth,
  OUT  UINT32                           *ScreenHeight,
  OUT  UINT32                           *ScreenDepth
  )
{
  EFI_STATUS                            Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE     *Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  if (This == NULL
    || FramebufferBase == NULL
    || FramebufferSize == NULL
    || ScreenRowBytes == NULL
    || ScreenWidth == NULL
    || ScreenHeight == NULL
    || ScreenDepth == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Mode = GraphicsOutput->Mode;
  Info = Mode->Info;

  if (Info == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // This is a bit inaccurate as it assumes 32-bit BPP, but will do for most cases.
  //
  *FramebufferBase = Mode->FrameBufferBase;
  *FramebufferSize = (UINT32) Mode->FrameBufferSize;
  *ScreenRowBytes  = (UINT32) (Info->PixelsPerScanLine * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  *ScreenWidth     = Info->HorizontalResolution;
  *ScreenHeight    = Info->VerticalResolution;
  *ScreenDepth     = DEFAULT_COLOUR_DEPTH;

  return EFI_SUCCESS;
}

STATIC
APPLE_FRAMEBUFFER_INFO_PROTOCOL
mAppleFramebufferInfo = {
  AppleFramebufferGetInfo
};

APPLE_FRAMEBUFFER_INFO_PROTOCOL *
OcAppleFbInfoInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                      Status;
  APPLE_FRAMEBUFFER_INFO_PROTOCOL *Protocol;

  DEBUG ((DEBUG_VERBOSE, "OcAppleFbInfoInstallProtocol\n"));

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleFramebufferInfoProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCOS: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleFramebufferInfoProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
     &gImageHandle,
     &gAppleFramebufferInfoProtocolGuid,
     (VOID *) &mAppleFramebufferInfo,
     NULL
     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleFramebufferInfo;
}

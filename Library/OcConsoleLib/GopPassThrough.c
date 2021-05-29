/** @file
  Copyright (C) 2020, vit9696. All rights reserved.
  Copyright (C) 2021, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "OcConsoleLibInternal.h"

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OcMiscLib.h>

STATIC
EFI_STATUS
EFIAPI
OcGopDrawQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  if (ModeNumber != 0) {
    DEBUG ((DEBUG_VERBOSE, "OCC: OcGopDrawQueryMode invalid ModeNumber - %u\n", ModeNumber));
    return EFI_INVALID_PARAMETER;
  }

  if (SizeOfInfo == NULL || Info == NULL) {
    DEBUG ((DEBUG_VERBOSE, "OCC: OcGopDrawQueryMode got invalid parameter SizeOfInfo or Info!\n"));
    return EFI_INVALID_PARAMETER;
  }

  *SizeOfInfo = This->Mode->SizeOfInfo;
  *Info       = AllocateCopyPool (This->Mode->SizeOfInfo, This->Mode->Info);
  if (*Info == NULL) {
    DEBUG ((DEBUG_VERBOSE, "OCC: OcGopDrawQueryMode failed to allocate memory for GOP Info!\n"));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcGopDrawSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  if (ModeNumber != 0) {
    DEBUG ((DEBUG_VERBOSE, "OCC: OcGopDrawSetMode unsupported ModeNumber - %u\n", ModeNumber));
    return EFI_UNSUPPORTED;
  }

  //
  // Assuming 0 is the only mode that is accepted, which is already set.
  //
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcGopDrawBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer    OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  )
{
  OC_GOP_PROTOCOL  *OcGopDraw;

  OcGopDraw = BASE_CR (This, OC_GOP_PROTOCOL, GraphicsOutput);

  return OcGopDraw->Uga->Blt (
    OcGopDraw->Uga,
    (EFI_UGA_PIXEL *) BltBuffer,
    (EFI_UGA_BLT_OPERATION) BltOperation,
    SourceX,
    SourceY,
    DestinationX,
    DestinationY,
    Width,
    Height,
    Delta
    );
}

EFI_STATUS
OcProvideGopPassThrough (
  IN BOOLEAN  ForAll
  )
{
  EFI_STATUS                       Status;
  UINTN                            HandleCount;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            Index;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL            *UgaDraw;
  OC_GOP_PROTOCOL                  *OcGopDraw;
  APPLE_FRAMEBUFFER_INFO_PROTOCOL  *FramebufferInfo;
  EFI_PHYSICAL_ADDRESS             FramebufferBase;
  UINT32                           FramebufferSize;
  UINT32                           ScreenRowBytes;
  UINT32                           ScreenWidth;
  UINT32                           ScreenHeight;
  UINT32                           ScreenDepth;
  UINT32                           HorizontalResolution;
  UINT32                           VerticalResolution;
  EFI_GRAPHICS_PIXEL_FORMAT        PixelFormat;
  UINT32                           ColorDepth;
  UINT32                           RefreshRate;
  BOOLEAN                          HasAppleFramebuffer;

  //
  // We should not proxy UGA when there is no AppleFramebuffer,
  // but on systems where there is nothing, it is the only option.
  // REF: https://github.com/acidanthera/bugtracker/issues/1498
  //
  Status = gBS->LocateProtocol (
    &gAppleFramebufferInfoProtocolGuid,
    NULL,
    (VOID *) &FramebufferInfo
    );
  HasAppleFramebuffer = !EFI_ERROR (Status);

  DEBUG_CODE_BEGIN ();
  HandleCount = (UINT32) OcCountProtocolInstances (&gEfiGraphicsOutputProtocolGuid);
  DEBUG ((DEBUG_INFO, "OCC: Found %u handles with GOP draw\n", HandleCount));

  HandleCount = (UINT32) OcCountProtocolInstances (&gAppleFramebufferInfoProtocolGuid);
  DEBUG ((DEBUG_INFO, "OCC: Found %u handles with Apple Framebuffer info\n", HandleCount));

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Failed to locate AppleFramebufferInfo protocol - %r\n", Status));
  } else {
    Status = FramebufferInfo->GetInfo (
      FramebufferInfo,
      &FramebufferBase,
      &FramebufferSize,
      &ScreenRowBytes,
      &ScreenWidth,
      &ScreenHeight,
      &ScreenDepth
      );
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCC: AppleFramebufferInfo - Got Base %Lx, Size %u, RowBytes %u, Width %u, Height %u, Depth %u\n",
        FramebufferBase,
        FramebufferSize,
        ScreenRowBytes,
        ScreenWidth,
        ScreenHeight,
        ScreenDepth
        ));
    } else {
      DEBUG ((DEBUG_INFO, "OCC: AppleFramebufferInfo failed to retrieve info - %r\n", Status));
    }
  }
  DEBUG_CODE_END ();
  
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiUgaDrawProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Failed to find handles with UGA - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCC: Found %u handles with UGA for GOP check\n", (UINT32) HandleCount));
  for (Index = 0; Index < HandleCount; ++Index) {
    DEBUG ((DEBUG_INFO, "OCC: Trying handle %u - %p\n", (UINT32) Index, HandleBuffer[Index]));

    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiUgaDrawProtocolGuid,
      (VOID **) &UgaDraw
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCC: No UGA protocol - %r\n", Status));
      continue;
    }

    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiGraphicsOutputProtocolGuid,
      (VOID **) &GraphicsOutput
      );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCC: Skipping GOP proxying as it is already present on handle %u - %p\n", (UINT32) Index, HandleBuffer[Index]));
      continue;
    }
    
    FramebufferBase = 0;
    FramebufferSize = 0;
    ScreenRowBytes  = 0;
    PixelFormat     = PixelBltOnly;

    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gAppleFramebufferInfoProtocolGuid,
      (VOID **) &FramebufferInfo
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCC: Failed to retrieve AppleFramebufferInfo protocol on handle %u - %p (%r)\n",
        (UINT32) Index,
        HandleBuffer[Index],
        Status
        ));
      if (HasAppleFramebuffer || !ForAll) {
        continue;
      }
    } else {
      DEBUG ((
        DEBUG_INFO,
        "OCC: Got AppleFramebufferInfo protocol on handle %u - %p\n",
        (UINT32) Index,
        HandleBuffer[Index]
        ));

      Status = FramebufferInfo->GetInfo (
        FramebufferInfo,
        &FramebufferBase,
        &FramebufferSize,
        &ScreenRowBytes,
        &ScreenWidth,
        &ScreenHeight,
        &ScreenDepth
        );
      if (!EFI_ERROR (Status)) {
        PixelFormat = PixelRedGreenBlueReserved8BitPerColor;  ///< or PixelBlueGreenRedReserved8BitPerColor?

        DEBUG ((
          DEBUG_INFO,
          "OCC: AppleFramebufferInfo - Got Base %Lx, Size %u, RowBytes %u, Width %u, Height %u, Depth %u on handle %u - %p\n",
          FramebufferBase,
          FramebufferSize,
          ScreenRowBytes,
          ScreenWidth,
          ScreenHeight,
          ScreenDepth,
          (UINT32) Index,
          HandleBuffer[Index]
          ));
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCC: Failed to get info from AppleFramebufferInfo protocol on handle %u - %p (%r)\n",
          (UINT32) Index,
          HandleBuffer[Index],
          Status
          ));
        if (HasAppleFramebuffer) {
          continue;
        }
      }
    }

    Status = UgaDraw->GetMode (
      UgaDraw,
      &HorizontalResolution,
      &VerticalResolution,
      &ColorDepth,
      &RefreshRate
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCC: UGA->GetMode returns error - %r\n", Status));
      continue;
    }

    OcGopDraw = AllocateZeroPool (sizeof (*OcGopDraw));
    if (OcGopDraw == NULL) {
      DEBUG ((DEBUG_INFO, "OCC: Failed to allocate GOP protocol\n"));
      continue;
    }

    OcGopDraw->Uga                          = UgaDraw;
    OcGopDraw->GraphicsOutput.QueryMode     = OcGopDrawQueryMode;
    OcGopDraw->GraphicsOutput.SetMode       = OcGopDrawSetMode;
    OcGopDraw->GraphicsOutput.Blt           = OcGopDrawBlt;
    OcGopDraw->GraphicsOutput.Mode          = AllocateZeroPool (sizeof (*OcGopDraw->GraphicsOutput.Mode));
    if (OcGopDraw->GraphicsOutput.Mode == NULL) {
      FreePool (OcGopDraw);
      continue;
    }
    //
    // Only Mode 0 is supported, so there is only one mode supported in total.
    //
    OcGopDraw->GraphicsOutput.Mode->MaxMode = 1;
    //
    // Again, only Mode 0 is supported.
    //
    OcGopDraw->GraphicsOutput.Mode->Mode    = 0;
    OcGopDraw->GraphicsOutput.Mode->Info    = AllocateZeroPool (sizeof (*OcGopDraw->GraphicsOutput.Mode->Info));
    if (OcGopDraw->GraphicsOutput.Mode->Info == NULL) {
      FreePool (OcGopDraw->GraphicsOutput.Mode);
      FreePool (OcGopDraw);
      continue;
    }
    OcGopDraw->GraphicsOutput.Mode->Info->Version                       = 0;
    OcGopDraw->GraphicsOutput.Mode->Info->HorizontalResolution          = HorizontalResolution;
    OcGopDraw->GraphicsOutput.Mode->Info->VerticalResolution            = VerticalResolution;
    OcGopDraw->GraphicsOutput.Mode->Info->PixelFormat                   = PixelFormat;
    //
    // No pixel mask is needed (i.e. all zero) in PixelInformation, 
    // plus AllocateZeroPool already assigns zero for it.
    // Skip.
    //------------------------------------------------------------------------------
    // ScreenRowBytes is PixelsPerScanLine * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
    // so here to divide it back.
    //
    OcGopDraw->GraphicsOutput.Mode->Info->PixelsPerScanLine             = ScreenRowBytes / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    OcGopDraw->GraphicsOutput.Mode->SizeOfInfo      = sizeof (*OcGopDraw->GraphicsOutput.Mode->Info);
    OcGopDraw->GraphicsOutput.Mode->FrameBufferBase = FramebufferBase;
    OcGopDraw->GraphicsOutput.Mode->FrameBufferSize = FramebufferSize;

    Status = gBS->InstallMultipleProtocolInterfaces (
      &HandleBuffer[Index],
      &gEfiGraphicsOutputProtocolGuid,
      &OcGopDraw->GraphicsOutput,
      NULL
      );
    if (EFI_ERROR (Status)) {
      FreePool (OcGopDraw->GraphicsOutput.Mode->Info);
      FreePool (OcGopDraw->GraphicsOutput.Mode);
      FreePool (OcGopDraw);
    }

    DEBUG ((
      DEBUG_INFO,
      "OCC: Installed GOP protocol - %r (Handle %u - %p, Resolution %ux%u, FramebufferBase %Lx, PixelFormat %d)\n",
      Status,
      (UINT32) Index,
      HandleBuffer[Index],
      HorizontalResolution,
      VerticalResolution,
      FramebufferBase,
      PixelFormat
      ));
  }

  FreePool (HandleBuffer);

  return Status;
}

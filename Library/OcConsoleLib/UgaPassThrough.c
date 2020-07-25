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

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC
EFI_STATUS
EFIAPI
OcUgaDrawGetMode (
  IN  EFI_UGA_DRAW_PROTOCOL *This,
  OUT UINT32                *HorizontalResolution,
  OUT UINT32                *VerticalResolution,
  OUT UINT32                *ColorDepth,
  OUT UINT32                *RefreshRate
  )
{
  OC_UGA_PROTOCOL                       *OcUgaDraw;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  DEBUG ((DEBUG_INFO, "OCC: OcUgaDrawGetMode %p\n", This));

  if (This == NULL
    || HorizontalResolution == NULL
    || VerticalResolution == NULL
    || ColorDepth == NULL
    || RefreshRate == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OcUgaDraw = BASE_CR (This, OC_UGA_PROTOCOL, Uga);
  Info      = OcUgaDraw->GraphicsOutput->Mode->Info;

  DEBUG ((
    DEBUG_INFO,
    "OCC: OcUgaDrawGetMode Info is %ux%u (%u)\n",
    Info->HorizontalResolution,
    Info->VerticalResolution,
    Info->PixelFormat
    ));

  if (Info->HorizontalResolution == 0 || Info->VerticalResolution == 0) {
    return EFI_NOT_STARTED;
  }

  *HorizontalResolution = Info->HorizontalResolution;
  *VerticalResolution = Info->VerticalResolution;
  *ColorDepth  = DEFAULT_COLOUR_DEPTH;
  *RefreshRate = DEFAULT_REFRESH_RATE;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcUgaDrawSetMode (
  IN  EFI_UGA_DRAW_PROTOCOL *This,
  IN  UINT32                HorizontalResolution,
  IN  UINT32                VerticalResolution,
  IN  UINT32                ColorDepth,
  IN  UINT32                RefreshRate
  )
{
  EFI_STATUS        Status;
  OC_UGA_PROTOCOL   *OcUgaDraw;

  OcUgaDraw = BASE_CR (This, OC_UGA_PROTOCOL, Uga);

  DEBUG ((
    DEBUG_INFO,
    "OCC: OcUgaDrawSetMode %p %ux%u@%u/%u\n",
    This,
    HorizontalResolution,
    VerticalResolution,
    ColorDepth,
    RefreshRate
    ));

  OcUgaDraw = BASE_CR (This, OC_UGA_PROTOCOL, Uga);

  Status = OcSetConsoleResolutionForProtocol (
    OcUgaDraw->GraphicsOutput,
    HorizontalResolution,
    VerticalResolution,
    ColorDepth
    );

  DEBUG ((DEBUG_INFO, "OCC: UGA SetConsoleResolutionOnHandle attempt - %r\n", Status));

  if (EFI_ERROR (Status)) {
    Status = OcSetConsoleResolutionForProtocol (
      OcUgaDraw->GraphicsOutput,
      0,
      0,
      0
      );
    DEBUG ((DEBUG_INFO, "OCC: UGA SetConsoleResolutionOnHandle max attempt - %r\n", Status));
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OcUgaDrawBlt (
  IN  EFI_UGA_DRAW_PROTOCOL                   *This,
  IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta      OPTIONAL
  )
{
  OC_UGA_PROTOCOL   *OcUgaDraw;

  OcUgaDraw = BASE_CR (This, OC_UGA_PROTOCOL, Uga);

  return OcUgaDraw->GraphicsOutput->Blt (
    OcUgaDraw->GraphicsOutput,
    (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) BltBuffer,
    (EFI_GRAPHICS_OUTPUT_BLT_OPERATION) BltOperation,
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
OcProvideUgaPassThrough (
  VOID
  )
{
  EFI_STATUS                     Status;
  UINTN                          HandleCount;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          Index;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL          *UgaDraw;
  OC_UGA_PROTOCOL                *OcUgaDraw;

  //
  // For now we do not need this but for launching 10.4, but as a side note:
  // MacPro5,1 has 2 GOP protocols:
  // - for GPU
  // - for ConsoleOutput
  // and 1 UGA protocol:
  // - for unknown handle
  //
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiUgaDrawProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Found %u handles with UGA draw\n", (UINT32) HandleCount));
    FreePool (HandleBuffer);
  } else {
    DEBUG ((DEBUG_INFO, "OCC: Found NO handles with UGA draw\n"));
  }

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Found %u handles with GOP for UGA check\n", (UINT32) HandleCount));

    for (Index = 0; Index < HandleCount; ++Index) {
      DEBUG ((DEBUG_INFO, "OCC: Trying handle %u - %p\n", (UINT32) Index, HandleBuffer[Index]));

      Status = gBS->HandleProtocol (
        HandleBuffer[Index],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID **) &GraphicsOutput
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCC: No GOP protocol - %r\n", Status));
        continue;
      }

      Status = gBS->HandleProtocol (
        HandleBuffer[Index],
        &gEfiUgaDrawProtocolGuid,
        (VOID **) &UgaDraw
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCC: No UGA protocol - %r\n", Status));

        OcUgaDraw = AllocateZeroPool (sizeof (*OcUgaDraw));
        if (OcUgaDraw == NULL) {
          DEBUG ((DEBUG_INFO, "OCC: Failed to allocate UGA protocol\n"));
          continue;
        }

        OcUgaDraw->GraphicsOutput = GraphicsOutput;
        OcUgaDraw->Uga.GetMode = OcUgaDrawGetMode;
        OcUgaDraw->Uga.SetMode = OcUgaDrawSetMode;
        OcUgaDraw->Uga.Blt = OcUgaDrawBlt;

        Status = gBS->InstallMultipleProtocolInterfaces (
          &HandleBuffer[Index],
          &gEfiUgaDrawProtocolGuid,
          &OcUgaDraw->Uga,
          NULL
          );

        DEBUG ((DEBUG_INFO, "OCC: Installed UGA protocol - %r\n", Status));
      } else {
        DEBUG ((DEBUG_INFO, "OCC: Has UGA protocol, skip\n"));
        continue;
      }
    }

    FreePool (HandleBuffer);
  } else {
    DEBUG ((DEBUG_INFO, "OCC: Failed to find handles with GOP\n"));
  }

  return Status;
}

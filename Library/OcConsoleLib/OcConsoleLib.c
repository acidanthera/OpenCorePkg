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

#include "OcConsoleLibInternal.h"

#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/OcForceResolution.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_STATUS
OcSetConsoleResolutionForProtocol (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput,
  IN  UINT32                          Width  OPTIONAL,
  IN  UINT32                          Height OPTIONAL,
  IN  UINT32                          Bpp    OPTIONAL
  )
{
  EFI_STATUS                            Status;

  UINT32                                MaxMode;
  UINT32                                ModeIndex;
  INT64                                 ModeNumber;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  BOOLEAN                               SetMax;

  SetMax = Width == 0 && Height == 0;

  DEBUG ((
    DEBUG_INFO,
    "OCC: Requesting %ux%u@%u (max: %d) resolution, curr %u, total %u\n",
    Width,
    Height,
    Bpp,
    SetMax,
    (UINT32) GraphicsOutput->Mode->Mode,
    (UINT32) GraphicsOutput->Mode->MaxMode
    ));

  DEBUG ((
    DEBUG_INFO,
    "OCC: Current FB at 0x%LX (0x%X), format %d, res %ux%u scan %u\n",
    GraphicsOutput->Mode->FrameBufferBase,
    (UINT32) GraphicsOutput->Mode->FrameBufferSize,
    GraphicsOutput->Mode->Info != NULL ? GraphicsOutput->Mode->Info->PixelFormat : -1,
    GraphicsOutput->Mode->Info != NULL ? GraphicsOutput->Mode->Info->HorizontalResolution : 0,
    GraphicsOutput->Mode->Info != NULL ? GraphicsOutput->Mode->Info->VerticalResolution : 0,
    GraphicsOutput->Mode->Info != NULL ? GraphicsOutput->Mode->Info->PixelsPerScanLine : 0
    ));

  //
  // Find the resolution we need.
  //
  ModeNumber = -1;
  MaxMode    = GraphicsOutput->Mode->MaxMode;
  for (ModeIndex = 0; ModeIndex < MaxMode; ++ModeIndex) {
    Status = GraphicsOutput->QueryMode (
      GraphicsOutput,
      ModeIndex,
      &SizeOfInfo,
      &Info
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCC: Mode %u failure - %r\n", ModeIndex, Status));
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCC: Mode %u - %ux%u:%u\n",
      ModeIndex,
      Info->HorizontalResolution,
      Info->VerticalResolution,
      Info->PixelFormat
      ));

    if (!SetMax) {
      //
      // Custom resolution is requested.
      //
      if (Info->HorizontalResolution == Width
        && Info->VerticalResolution == Height
        && (Bpp == 0 || Bpp == 24 || Bpp == 32)
        && (Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor
          || Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor
          || (Info->PixelFormat == PixelBitMask
            && (Info->PixelInformation.RedMask  == 0xFF000000U
              || Info->PixelInformation.RedMask == 0xFF0000U
              || Info->PixelInformation.RedMask == 0xFF00U
              || Info->PixelInformation.RedMask == 0xFFU))
          || Info->PixelFormat == PixelBltOnly)) {
        ModeNumber = ModeIndex;
        FreePool (Info);
        break;
      }
    } else if (Info->HorizontalResolution > Width
      || (Info->HorizontalResolution == Width && Info->VerticalResolution > Height)) {
      Width = Info->HorizontalResolution;
      Height   = Info->VerticalResolution;
      ModeNumber = ModeIndex;
    }

    FreePool (Info);
  }

  if (ModeNumber < 0) {
    DEBUG ((DEBUG_WARN, "OCC: No compatible mode for %ux%u@%u (max: %u) resolution\n", Width, Height, Bpp, SetMax));
    return EFI_NOT_FOUND;
  }

  if (ModeNumber == GraphicsOutput->Mode->Mode) {
    DEBUG ((DEBUG_INFO, "OCC: Current mode matches desired mode %u\n", (UINT32) ModeNumber));
    return EFI_ALREADY_STARTED;
  }

  //
  // Current graphics mode is not set, or is not set to the mode found above.
  // Set the new graphics mode.
  //
  DEBUG ((
    DEBUG_INFO,
    "OCC: Setting mode %u with %ux%u resolution\n",
    (UINT32) ModeNumber,
    Width,
    Height
    ));

  Status = GraphicsOutput->SetMode (GraphicsOutput, (UINT32) ModeNumber);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "OCC: Failed to set mode %u (prev %u) with %ux%u resolution\n",
      (UINT32) ModeNumber,
      (UINT32) GraphicsOutput->Mode->Mode,
      Width,
      Height
      ));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCC: Changed resolution mode to %u\n", (UINT32) GraphicsOutput->Mode->Mode));

  return Status;
}

EFI_STATUS
OcSetConsoleModeForProtocol (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut,
  IN  UINT32                           Width,
  IN  UINT32                           Height
  )
{
  EFI_STATUS  Status;
  UINT32      MaxMode;
  UINT32      ModeIndex;
  INT64       ModeNumber;
  UINTN       Columns;
  UINTN       Rows;
  BOOLEAN     SetMax;

  SetMax = Width == 0 && Height == 0;

  DEBUG ((
    DEBUG_INFO,
    "OCC: Requesting %ux%u (max: %d) console mode, curr %u, max %u\n",
    Width,
    Height,
    SetMax,
    (UINT32) TextOut->Mode->Mode,
    (UINT32) TextOut->Mode->MaxMode
    ));

  //
  // Find the resolution we need.
  //
  ModeNumber = -1;
  MaxMode    = TextOut->Mode->MaxMode;
  for (ModeIndex = 0; ModeIndex < MaxMode; ++ModeIndex) {
    Status = TextOut->QueryMode (
      TextOut,
      ModeIndex,
      &Columns,
      &Rows
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCC: Mode %u - %ux%u\n",
      ModeIndex,
      (UINT32) Columns,
      (UINT32) Rows
      ));

    if (!SetMax) {
      //
      // Custom mode is requested.
      //
      if (Columns == Width && Rows == Height) {
        ModeNumber = ModeIndex;
        break;
      }
    } else if ((UINT32) Columns > Width
      || ((UINT32) Columns == Width && (UINT32) Rows > Height)) {
      Width      = (UINT32) Columns;
      Height     = (UINT32) Rows;
      ModeNumber = ModeIndex;
    }
  }

  if (ModeNumber < 0) {
    DEBUG ((DEBUG_WARN, "OCC: No compatible mode for %ux%u (max: %u) console mode\n", Width, Height, SetMax));
    return EFI_NOT_FOUND;
  }

  if (ModeNumber == TextOut->Mode->Mode) {
    //
    // This does not seem to affect systems anyhow, but for safety reasons
    // we should refresh console mode after changing GOP resolution.
    //
    DEBUG ((
      DEBUG_INFO,
      "OCC: Current console mode matches desired mode %u, forcing update\n",
      (UINT32) ModeNumber
      ));
  }

  //
  // Current graphics mode is not set, or is not set to the mode found above.
  // Set the new graphics mode.
  //
  DEBUG ((
    DEBUG_INFO,
    "OCC: Setting mode %u (prev %u) with %ux%u console mode\n",
    (UINT32) ModeNumber,
    (UINT32) TextOut->Mode->Mode,
    Width,
    Height
    ));

  Status = TextOut->SetMode (TextOut, (UINTN) ModeNumber);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "OCC: Failed to set mode %u with %ux%u console mode\n",
      (UINT32) ModeNumber,
      Width,
      Height
      ));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCC: Changed console mode to %u\n", (UINT32) TextOut->Mode->Mode));

  return EFI_SUCCESS;
}

EFI_STATUS
OcSetConsoleResolution (
  IN  UINT32              Width  OPTIONAL,
  IN  UINT32              Height OPTIONAL,
  IN  UINT32              Bpp    OPTIONAL,
  IN  BOOLEAN             Force
  )
{
  EFI_STATUS                    Result;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  OC_FORCE_RESOLUTION_PROTOCOL  *OcForceResolution;

  //
  // Force resolution if specified.
  //
  if (Force) {
    Result = gBS->LocateProtocol (
      &gOcForceResolutionProtocolGuid,
      NULL,
      (VOID **) &OcForceResolution
      );

    if (!EFI_ERROR (Result)) {
      Result = OcForceResolution->SetResolution (OcForceResolution, Width, Height);
      if (EFI_ERROR (Result)) {
        DEBUG ((DEBUG_WARN, "OCC: Failed to force resolution - %r\n", Result));
      }
    } else {
      DEBUG ((DEBUG_INFO, "OCC: Missing OcForceResolution protocol - %r\n", Result));
    }
  }

#ifdef OC_CONSOLE_CHANGE_ALL_RESOLUTIONS
  EFI_STATUS                    Status;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Index;

  Result = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (!EFI_ERROR (Result)) {
    Result = EFI_NOT_FOUND;

    DEBUG ((DEBUG_INFO, "OCC: Found %u handles with GOP\n", (UINT32) HandleCount));

    for (Index = 0; Index < HandleCount; ++Index) {
      DEBUG ((DEBUG_INFO, "OCC: Trying handle %u - %p\n", (UINT32) Index, HandleBuffer[Index]));

      Status = gBS->HandleProtocol (
        HandleBuffer[Index],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID **) &GraphicsOutput
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OCC: Missing GOP on console - %r\n", Status));
        continue;
      }

      Result = OcSetConsoleResolutionForProtocol (GraphicsOutput, Width, Height, Bpp);
    }

    FreePool (HandleBuffer);
  } else {
    DEBUG ((DEBUG_INFO, "OCC: Failed to find handles with GOP - %r\n", Result));
  }
#else
  Result = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );

  if (EFI_ERROR (Result)) {
    DEBUG ((DEBUG_WARN, "OCC: Missing GOP on ConOut - %r\n", Result));
    return Result;
  }

  Result = OcSetConsoleResolutionForProtocol (GraphicsOutput, Width, Height, Bpp);
#endif

  return Result;
}

EFI_STATUS
OcSetConsoleMode (
  IN  UINT32                       Width,
  IN  UINT32                       Height
  )
{
  return OcSetConsoleModeForProtocol (gST->ConOut, Width, Height);
}

VOID
OcSetupConsole (
  IN OC_CONSOLE_RENDERER          Renderer,
  IN BOOLEAN                      IgnoreTextOutput,
  IN BOOLEAN                      SanitiseClearScreen,
  IN BOOLEAN                      ClearScreenOnModeSwitch,
  IN BOOLEAN                      ReplaceTabWithSpace
  )
{
  if (Renderer == OcConsoleRendererBuiltinGraphics) {
    OcUseBuiltinTextOutput (EfiConsoleControlScreenGraphics);
  } else if (Renderer == OcConsoleRendererBuiltinText) {
    OcUseBuiltinTextOutput (EfiConsoleControlScreenText);
  } else {
    OcUseSystemTextOutput (
      Renderer,
      IgnoreTextOutput,
      SanitiseClearScreen,
      ClearScreenOnModeSwitch,
      ReplaceTabWithSpace
      );
  }
}

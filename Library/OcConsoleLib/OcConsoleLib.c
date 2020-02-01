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
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>

VOID
OcConsoleDisableCursor (
  VOID
  )
{
  gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
  if (gST->ConOut->Mode->CursorVisible) {
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  }
}

EFI_STATUS
SetConsoleResolutionForProtocol (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput,
  IN  UINT32                          Width,
  IN  UINT32                          Height,
  IN  UINT32                          Bpp    OPTIONAL,
  IN  BOOLEAN                         Reconnect
  )
{
  EFI_STATUS                            Status;

  UINT32                                MaxMode;
  UINT32                                ModeIndex;
  INT64                                 ModeNumber;
  UINTN                                 SizeOfInfo;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  UINTN                                 Index;
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
      if (Info->HorizontalResolution == Width && Info->VerticalResolution == Height
        && (Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor
          || Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
        && (Bpp == 0 || Bpp == 24 || Bpp == 32)) {
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
    return EFI_SUCCESS;
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

  //
  // On some firmwares When we change mode on GOP, we need to reconnect the drivers
  // which produce simple text out. Otherwise, they won't produce text based on the
  // new resolution.
  //
  // Needy reports that boot.efi seems to work fine without this block of code.
  // However, I believe that UEFI specification does not provide any standard way
  // to inform TextOut protocol about resolution change, which means the firmware
  // may not be aware of the change, especially when custom GOP is used.
  // We can move this to quirks if it causes problems, but I believe the code below
  // is legit.
  //
  // Note: on APTIO IV boards this block of code may result in black screen when launching
  // OpenCore from Shell, thus it is optional.
  //

  if (!Reconnect) {
    return Status;
  }

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiSimpleTextOutProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; ++Index) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }

    for (Index = 0; Index < HandleCount; ++Index) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    FreePool (HandleBuffer);

    //
    // It is implementation defined, which console mode is used by ConOut.
    // Assume the implementation chooses most sensible value based on GOP resolution.
    // If it does not, there is a separate ConsoleMode param, which expands to SetConsoleMode.
    //
  } else {
    DEBUG ((DEBUG_WARN, "OCC: Failed to find any text output handles\n"));
  }

  return Status;
}

EFI_STATUS
SetConsoleModeForProtocol (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut,
  IN  UINT32                           Width,
  IN  UINT32                           Height
  )
{
  EFI_STATUS                            Status;

  UINT32                                MaxMode;
  UINT32                                ModeIndex;
  INT64                                 ModeNumber;
  UINTN                                 Columns;
  UINTN                                 Rows;
  BOOLEAN                               SetMax;

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
SetConsoleResolution (
  IN  UINT32              Width,
  IN  UINT32              Height,
  IN  UINT32              Bpp    OPTIONAL,
  IN  BOOLEAN             Reconnect
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;

#ifdef OC_CONSOLE_CHANGE_ALL_RESOLUTIONS
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Index;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (!EFI_ERROR (Status)) {
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

      Status = SetConsoleResolutionForProtocol (GraphicsOutput, Width, Height, Bpp, Reconnect);
    }

    FreePool (HandleBuffer);
  } else {
    DEBUG ((DEBUG_INFO, "OCC: Failed to find handles with GOP\n"));
  }
#else
  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCC: Missing GOP on ConOut - %r\n", Status));
    return Status;
  }

  Status = SetConsoleResolutionForProtocol (GraphicsOutput, Width, Height, Bpp, Reconnect);
#endif

  return Status;
}

EFI_STATUS
SetConsoleMode (
  IN  UINT32                       Width,
  IN  UINT32                       Height
  )
{
  return SetConsoleModeForProtocol (gST->ConOut, Width, Height);
}

VOID
OcProvideConsoleGop (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Gop;

  Gop = NULL;
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, &Gop);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: Installing GOP (%r) on ConsoleOutHandle...\n", Status));
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, &Gop);

    if (!EFI_ERROR (Status)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
        &gST->ConsoleOutHandle,
        &gEfiGraphicsOutputProtocolGuid,
        Gop,
        NULL
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OCC: Failed to install GOP on ConsoleOutHandle - %r\n", Status));
      }
    } else {
      DEBUG ((DEBUG_WARN, "OCC: Missing GOP entirely - %r\n", Status));
    }
  }
}

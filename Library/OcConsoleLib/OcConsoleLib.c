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

#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC EFI_CONSOLE_CONTROL_SCREEN_MODE mConsoleMode = EfiConsoleControlScreenText;

STATIC
EFI_CONSOLE_CONTROL_PROTOCOL
mOriginalConsoleControlProtocol;

STATIC
EFI_STATUS
EFIAPI
ConsoleControlGetMode (
  IN   EFI_CONSOLE_CONTROL_PROTOCOL    *This,
  OUT  EFI_CONSOLE_CONTROL_SCREEN_MODE *Mode,
  OUT  BOOLEAN                         *GopUgaExists OPTIONAL,
  OUT  BOOLEAN                         *StdInLocked  OPTIONAL
  )
{
  *Mode = mConsoleMode;

  if (GopUgaExists) {
    *GopUgaExists = TRUE;
  }

  if (StdInLocked) {
    *StdInLocked  = FALSE;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsoleControlSetMode (
  IN EFI_CONSOLE_CONTROL_PROTOCOL     *This,
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode
  )
{
  mConsoleMode = Mode;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsoleControlLockStdIn (
  IN EFI_CONSOLE_CONTROL_PROTOCOL *This,
  IN CHAR16                       *Password
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_CONSOLE_CONTROL_PROTOCOL
mConsoleControlProtocol = {
  ConsoleControlGetMode,
  ConsoleControlSetMode,
  ConsoleControlLockStdIn
};

EFI_STATUS
ConfigureConsoleControl (
  IN BOOLEAN              WrapExisting
  )
{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;
  EFI_HANDLE                    NewHandle;

  Status = gBS->LocateProtocol (
    &gEfiConsoleControlProtocolGuid,
    NULL,
    (VOID *) &ConsoleControl
    );

  //
  // Native implementation exists, ignore.
  //
  if (!EFI_ERROR (Status)) {
    if (WrapExisting) {
      CopyMem (
        &mOriginalConsoleControlProtocol,
        ConsoleControl,
        sizeof (mOriginalConsoleControlProtocol)
        );
      CopyMem (
        &ConsoleControl,
        &mConsoleControlProtocol,
        sizeof (mOriginalConsoleControlProtocol)
        );
    }

    return EFI_SUCCESS;
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gEfiConsoleControlProtocolGuid,
    &mConsoleControlProtocol,
    NULL
    );

  return Status;
}

VOID
ParseScreenResolution (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT UINT32              *Bpp,
  OUT BOOLEAN             *Max
  )
{
  UINT32  TmpWidth;
  UINT32  TmpHeight;

  *Width  = 0;
  *Height = 0;
  *Bpp    = 0;
  *Max    = FALSE;

  if (AsciiStrCmp (String, "Max") == 0) {
    *Max = TRUE;
    return;
  }

  if (String[0] == '\0' || *String < '0' || *String > '9') {
    return;
  }

  TmpWidth = TmpHeight = 0;

  while (*String >= '0' && *String <= '9') {
    if (OcOverflowMulAddU32 (TmpWidth, 10, *String++ - '0', &TmpWidth)) {
      return;
    }
  }

  if (*String++ != 'x' || *String < '0' || *String > '9') {
    return;
  }

  while (*String >= '0' && *String <= '9') {
    if (OcOverflowMulAddU32 (TmpHeight, 10, *String++ - '0', &TmpHeight)) {
      return;
    }
  }

  if (*String != '\0' && *String != '@') {
    return;
  }

  *Width  = TmpWidth;
  *Height = TmpHeight;

  if (*String++ != '@') {
    return;
  }

  TmpWidth = 0;
  while (*String >= '0' && *String <= '9') {
    if (OcOverflowMulAddU32 (TmpWidth, 10, *String++ - '0', &TmpWidth)) {
      return;
    }
  }

  if (*String != '\0') {
    return;
  }

  *Bpp = TmpWidth;
}

EFI_STATUS
SetConsoleResolution (
  IN  UINT32              Width,
  IN  UINT32              Height,
  IN  UINT32              Bpp    OPTIONAL
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
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  BOOLEAN                               SetMax;

  Status = gBS->HandleProtocol (
    &gEfiGraphicsOutputProtocolGuid,
    gST->ConsoleOutHandle,
    (VOID **) &GraphicsOutput
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCC: Missing GOP on console - %r\n", Status));
    return Status;
  }

  SetMax       = Width == 0 && Height == 0;
  ModeNumber   = -1;

  DEBUG ((DEBUG_INFO, "OCC: Requesting %ux%u@%u (max: %d) resolution\n", Width, Height, Bpp, SetMax));

  //
  // Find the resolution we need.
  //
  MaxMode = GraphicsOutput->Mode->MaxMode;
  for (ModeIndex = 0; ModeIndex < MaxMode; ++ModeIndex) {
    Status = GraphicsOutput->QueryMode (
      GraphicsOutput,
      ModeIndex,
      &SizeOfInfo,
      &Info
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCC: Mode %u has %ux%u:%u resolution\n",
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
      "OCC: Failed to set mode %u with %ux%u resolution\n",
      (UINT32) ModeNumber,
      Width,
      Height
      ));
    return Status;
  }

  //
  // On some firmwares When we change mode on GOP, we need to reconnect the drivers
  // which produce simple text out. Otherwise, they won't produce text based on the
  // new resolution.
  //
  // Vit: Needy reports that boot.efi seems to work fine without this block of code.
  // Should we try to stay safe and have it? Or is it in fact mad design wise?
  // Best to later check actual behaviour, firmware code, and EDK II...
  //

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

    Status = gST->ConOut->SetMode (gST->ConOut, 0);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCS: Mode set on ConOut failed after reconnect\n"));
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCS: Failed to find any text output handles\n"));
  }

  return Status;
}

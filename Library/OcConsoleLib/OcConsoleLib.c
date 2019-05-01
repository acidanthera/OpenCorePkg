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
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>

//
// Current reported console mode.
//
STATIC
EFI_CONSOLE_CONTROL_SCREEN_MODE
mConsoleMode = EfiConsoleControlScreenText;

//
// Stick to previously set console mode.
//
STATIC
BOOLEAN
mForceConsoleMode = FALSE;

//
// Original text output function.
//
STATIC
EFI_TEXT_STRING
mOriginalOutputString;

//
// Original console control protocol functions.
//
STATIC
EFI_CONSOLE_CONTROL_PROTOCOL
mOriginalConsoleControlProtocol;

STATIC
EFI_STATUS
EFIAPI
ControlledOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  )
{
  if (mConsoleMode == EfiConsoleControlScreenText) {
    return mOriginalOutputString (This, String);
  }

  return EFI_UNSUPPORTED;
}

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
  EFI_STATUS  Status;

  if (mOriginalConsoleControlProtocol.GetMode != NULL && !mForceConsoleMode) {
    Status = mOriginalConsoleControlProtocol.GetMode (
      This,
      Mode,
      GopUgaExists,
      StdInLocked
      );

    if (!EFI_ERROR (Status)) {
      mConsoleMode = *Mode;
    }

    return Status;
  }

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
  DEBUG ((DEBUG_INFO, "OCC: Setting cc mode %d -> %d\n", mConsoleMode, Mode));

  mConsoleMode = Mode;

  if (mOriginalConsoleControlProtocol.SetMode != NULL && !mForceConsoleMode) {
    return mOriginalConsoleControlProtocol.SetMode (
      This,
      Mode
      );
  }

  //
  // Disable and hide flashing cursor.
  //
  if (Mode == EfiConsoleControlScreenGraphics && mOriginalOutputString != NULL) {
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  }

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
  if (mOriginalConsoleControlProtocol.LockStdIn != NULL) {
    return mOriginalConsoleControlProtocol.LockStdIn (
      This,
      Password
      );
  }

  return EFI_DEVICE_ERROR;
}

STATIC
EFI_CONSOLE_CONTROL_PROTOCOL
mConsoleControlProtocol = {
  ConsoleControlGetMode,
  ConsoleControlSetMode,
  ConsoleControlLockStdIn
};

EFI_STATUS
ConsoleControlConfigure (
  IN BOOLEAN                      IgnoreTextOutput
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

  DEBUG ((
    DEBUG_INFO,
    "OCC: Configuring console (%r) ignore %d\n",
    Status,
    IgnoreTextOutput
    ));

  if (IgnoreTextOutput) {
    mOriginalOutputString     = gST->ConOut->OutputString;
    gST->ConOut->OutputString = ControlledOutputString;
  }

  //
  // Native implementation exists, ignore.
  //
  if (!EFI_ERROR (Status)) {
    CopyMem (
      &mOriginalConsoleControlProtocol,
      ConsoleControl,
      sizeof (mOriginalConsoleControlProtocol)
      );
    CopyMem (
      ConsoleControl,
      &mConsoleControlProtocol,
      sizeof (mOriginalConsoleControlProtocol)
      );

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

EFI_STATUS
ConsoleControlSetBehaviour (
  IN OC_CONSOLE_CONTROL_BEHAVIOUR  Behaviour
  )
{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

  DEBUG ((
    DEBUG_INFO,
    "OCC: Configuring behaviour %u\n",
    Behaviour
    ));

  if (Behaviour == OcConsoleControlDefault) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (
    &gEfiConsoleControlProtocolGuid,
    NULL,
    (VOID *) &ConsoleControl
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Behaviour == OcConsoleControlText) {
    return ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenText);
  }

  if (Behaviour == OcConsoleControlGraphics) {
    return ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenGraphics);
  }

  //
  // We are setting a forced mode, do not let console changes.
  //
  mForceConsoleMode = TRUE;

  if (Behaviour == OcConsoleControlForceText) {
    mConsoleMode = EfiConsoleControlScreenText;
  } else {
    mConsoleMode = EfiConsoleControlScreenGraphics;
  }

  //
  // Ensure that the mode is changed if original protocol is available.
  //
  if (mOriginalConsoleControlProtocol.SetMode != NULL) {
    Status = mOriginalConsoleControlProtocol.SetMode (
      ConsoleControl,
      mConsoleMode
      );
  }

  return Status;
}

/**
  Parse resolution string.

  @param[in]   String   Resolution in WxH@Bpp or WxH format.
  @param[out]  Width    Parsed width or 0.
  @param[out]  Height   Parsed height or 0.
  @param[out]  Bpp      Parsed Bpp or 0, optional to force WxH format.
  @param[out]  Max      Set to TRUE when String equals to Max.
**/
STATIC
VOID
ParseResolution (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT UINT32              *Bpp OPTIONAL,
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

  if (*String == '\0' || *String < '0' || *String > '9') {
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

  if (*String != '\0' && (*String != '@' || Bpp == NULL)) {
    return;
  }

  *Width  = TmpWidth;
  *Height = TmpHeight;

  if (*String == '\0') {
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

VOID
ParseScreenResolution (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT UINT32              *Bpp,
  OUT BOOLEAN             *Max
  )
{
  ASSERT (String != NULL);
  ASSERT (Width != NULL);
  ASSERT (Height != NULL);
  ASSERT (Bpp != NULL);
  ASSERT (Max != NULL);

  ParseResolution (String, Width, Height, Bpp, Max);
}

VOID
ParseConsoleMode (
  IN  CONST CHAR8         *String,
  OUT UINT32              *Width,
  OUT UINT32              *Height,
  OUT BOOLEAN             *Max
  )
{
  ASSERT (String != NULL);
  ASSERT (Width != NULL);
  ASSERT (Height != NULL);
  ASSERT (Max != NULL);

  ParseResolution (String, Width, Height, NULL, Max);
}

OC_CONSOLE_CONTROL_BEHAVIOUR
ParseConsoleControlBehaviour (
  IN  CONST CHAR8        *Behaviour
  )
{
  if (Behaviour[0] == '\0') {
    return OcConsoleControlDefault;
  }

  if (AsciiStrCmp (Behaviour, "Graphics") == 0) {
    return OcConsoleControlGraphics;
  }

  if (AsciiStrCmp (Behaviour, "Text") == 0) {
    return OcConsoleControlText;
  }

  if (AsciiStrCmp (Behaviour, "ForceGraphics") == 0) {
    return OcConsoleControlForceGraphics;
  }

  if (AsciiStrCmp (Behaviour, "ForceText") == 0) {
    return OcConsoleControlForceText;
  }

  return OcConsoleControlDefault;
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
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCC: Missing GOP on console - %r\n", Status));
    return Status;
  }

  SetMax = Width == 0 && Height == 0;

  DEBUG ((
    DEBUG_INFO,
    "OCC: Requesting %ux%u@%u (max: %d) resolution, curr %u\n",
    Width,
    Height,
    Bpp,
    SetMax,
    (UINT32) GraphicsOutput->Mode->Mode
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
  // Vit: Needy reports that boot.efi seems to work fine without this block of code.
  // However, I believe that UEFI specification does not provide any standard way
  // to inform TextOut protocol about resolution change, which means the firmware
  // may not be aware of the change, especially when custom GOP is used.
  // We can move this to quirks if it causes problems, but I believe the code below
  // is legit.
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

    //
    // It is implementation defined, which console mode is used by ConOut.
    // Assume the implementation chooses most sensible value based on GOP resolution.
    // If it does not, there is a separate ConsoleMode param, which expands to SetConsoleMode.
    //
  } else {
    DEBUG ((DEBUG_WARN, "OCS: Failed to find any text output handles\n"));
  }

  return Status;
}

EFI_STATUS
SetConsoleMode (
  IN  UINT32              Width,
  IN  UINT32              Height
  )
{
  EFI_STATUS                            Status;

  UINT32                                MaxMode;
  UINT32                                ModeIndex;
  INT64                                 ModeNumber;
  UINTN                                 Columns;
  UINTN                                 Rows;
  BOOLEAN                               SetMax;

  SetMax       = Width == 0 && Height == 0;

  DEBUG ((
    DEBUG_INFO,
    "OCC: Requesting %ux%u (max: %d) console mode, curr %u\n",
    Width,
    Height,
    SetMax,
    (UINT32) gST->ConOut->Mode->Mode
    ));

  //
  // Find the resolution we need.
  //
  ModeNumber = -1;
  MaxMode    = gST->ConOut->Mode->MaxMode;
  for (ModeIndex = 0; ModeIndex < MaxMode; ++ModeIndex) {
    Status = gST->ConOut->QueryMode (
      gST->ConOut,
      ModeIndex,
      &Columns,
      &Rows
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCC: Mode %u has %ux%u console mode\n",
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

  if (ModeNumber == gST->ConOut->Mode->Mode) {
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
    (UINT32) gST->ConOut->Mode->Mode,
    Width,
    Height
    ));

  Status = gST->ConOut->SetMode (gST->ConOut, (UINTN) ModeNumber);
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

  DEBUG ((DEBUG_INFO, "OCC: Changed console mode to %u\n", (UINT32) gST->ConOut->Mode->Mode));

  return EFI_SUCCESS;
}

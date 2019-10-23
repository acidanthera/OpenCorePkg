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
// Disable text output in graphics mode.
//
STATIC
BOOLEAN
mIgnoreTextInGraphics = FALSE;

//
// Clear screen when switching from graphics mode to text mode.
//
STATIC
BOOLEAN
mClearScreenOnModeSwitch = FALSE;

//
// Replace tab character with a space in output.
//
STATIC
BOOLEAN
mReplaceTabWithSpace = FALSE;

//
// Original text output function.
//
STATIC
EFI_TEXT_STRING
mOriginalOutputString;

//
// Original clear screen function.
//
STATIC
EFI_TEXT_CLEAR_SCREEN
mOriginalClearScreen;

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
  UINTN       Index;
  UINTN       Length;
  CHAR16      *StringCopy;
  EFI_STATUS  Status;

  if (mConsoleMode == EfiConsoleControlScreenText || !mIgnoreTextInGraphics) {
    if (mReplaceTabWithSpace) {
      Length = StrLen(String);
      if (Length == MAX_UINTN) {
        return EFI_UNSUPPORTED;
      }

      StringCopy = AllocatePool((Length + 1) * sizeof(CHAR16));
      if (StringCopy == NULL) {
        return mOriginalOutputString (This, String);
      }
      for (Index = 0; Index < Length; Index++) {
        if (String[Index] == '\t') {
          StringCopy[Index] = ' ';
        }
        else {
          StringCopy[Index] = String[Index];
        }
      }
      StringCopy[Length] = 0;
      Status = mOriginalOutputString (This, StringCopy);
      FreePool(StringCopy);
      return Status;
    }
    else {
      return mOriginalOutputString (This, String);
    }
  }

  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
ControlledClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  EFI_STATUS                            Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  UINT32                                Mode;

  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );

  if (!EFI_ERROR (Status)) {
    Mode = GraphicsOutput->Mode->Mode;
  } else {
    GraphicsOutput = NULL;
  }

  //
  // On APTIO V with large resolution (e.g. 2K or 4K) ClearScreen
  // invocation resets resolution to 1024x768. We restore it here.
  // This is probably the safest approach. Other ways include:
  // - Change SetMode function to some No-op in GOP.
  // - Cleanup the screen manually.
  // None of these are tested as I am tired and do not want to risk.
  //
  Status = mOriginalClearScreen (This);

  if (GraphicsOutput != NULL) {
    GraphicsOutput->SetMode (GraphicsOutput, Mode);
  }

  return Status;
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
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;

  DEBUG ((DEBUG_INFO, "OCC: Setting cc mode %d -> %d\n", mConsoleMode, Mode));

  mConsoleMode = Mode;

  if (mClearScreenOnModeSwitch && Mode == EfiConsoleControlScreenText) {
    Status = gBS->HandleProtocol (
      gST->ConsoleOutHandle,
      &gEfiGraphicsOutputProtocolGuid,
      (VOID **) &GraphicsOutput
      );

    if (!EFI_ERROR (Status)) {
      Background.Red   = 0;
      Background.Green = 0;
      Background.Blue  = 0;

      Status = GraphicsOutput->Blt (
        GraphicsOutput,
        &Background,
        EfiBltVideoFill,
        0,
        0,
        0,
        0,
        GraphicsOutput->Mode->Info->HorizontalResolution,
        GraphicsOutput->Mode->Info->VerticalResolution,
        0
        );
    }

  }

  if (mOriginalConsoleControlProtocol.SetMode != NULL && !mForceConsoleMode) {
    return mOriginalConsoleControlProtocol.SetMode (
      This,
      Mode
      );
  }

  //
  // Disable and hide flashing cursor when switching to graphics from text.
  // This may be the case when we formerly handled text input or output.
  // mOriginalOutputString check is here to signalise that the problem only exists
  // on platforms that require IgnoreTextOutput quirk, and this is the extension
  // of its implementation.
  //
  if (Mode == EfiConsoleControlScreenGraphics && mOriginalOutputString != NULL) {
    OcConsoleDisableCursor ();
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

/**
  Locate Console Control protocol.

  @retval Console control protocol instance or NULL.
**/
EFI_CONSOLE_CONTROL_PROTOCOL *
OcConsoleControlInstallProtocol (
  IN BOOLEAN  Reinstall
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
    "OCC: Install console control %d - %r\n",
    Reinstall,
    Status
    ));

  //
  // Native implementation exists, overwrite on force.
  //

  if (!EFI_ERROR (Status)) {
    if (Reinstall) {
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
    }

    return ConsoleControl;
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gEfiConsoleControlProtocolGuid,
    &mConsoleControlProtocol,
    NULL
    );

  return &mConsoleControlProtocol;
}

VOID
OcConsoleControlConfigure (
  IN BOOLEAN                      IgnoreTextOutput,
  IN BOOLEAN                      SanitiseClearScreen,
  IN BOOLEAN                      ClearScreenOnModeSwitch,
  IN BOOLEAN                      ReplaceTabWithSpace
  )
{
  DEBUG ((
    DEBUG_INFO,
    "OCC: Configuring console ignore %d san clear %d clear switch %d replace tab %ds\n",
    IgnoreTextOutput,
    SanitiseClearScreen,
    ClearScreenOnModeSwitch,
    ReplaceTabWithSpace
    ));

  mIgnoreTextInGraphics    = IgnoreTextOutput;
  mClearScreenOnModeSwitch = ClearScreenOnModeSwitch;
  mReplaceTabWithSpace     = ReplaceTabWithSpace;

  if (IgnoreTextOutput || ReplaceTabWithSpace) {
    mOriginalOutputString     = gST->ConOut->OutputString;
    gST->ConOut->OutputString = ControlledOutputString;
  }

  if (SanitiseClearScreen) {
    mOriginalClearScreen      = gST->ConOut->ClearScreen;
    gST->ConOut->ClearScreen  = ControlledClearScreen;
  }
}

EFI_STATUS
OcConsoleControlSetBehaviour (
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
  *Max    = FALSE;

  if (Bpp != NULL) {
    *Bpp    = 0;
  }

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
  IN  UINT32              Bpp    OPTIONAL,
  IN  BOOLEAN             Reconnect
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
    "OCC: Requesting %ux%u (max: %d) console mode, curr %u, max %u\n",
    Width,
    Height,
    SetMax,
    (UINT32) gST->ConOut->Mode->Mode,
    (UINT32) gST->ConOut->Mode->MaxMode
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

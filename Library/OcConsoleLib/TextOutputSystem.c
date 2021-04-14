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

//
// Current reported console mode.
//
STATIC
EFI_CONSOLE_CONTROL_SCREEN_MODE
mConsoleMode = EfiConsoleControlScreenText;

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

//
// EFI background colours.
//
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mEfiBackgroundColors[8] = {
  //
  // B    G    R   reserved
  //
  {0x00, 0x00, 0x00, 0x00},  // BLACK
  {0x98, 0x00, 0x00, 0x00},  // LIGHTBLUE
  {0x00, 0x98, 0x00, 0x00},  // LIGHGREEN
  {0x98, 0x98, 0x00, 0x00},  // LIGHCYAN
  {0x00, 0x00, 0x98, 0x00},  // LIGHRED
  {0x98, 0x00, 0x98, 0x00},  // MAGENTA
  {0x00, 0x98, 0x98, 0x00},  // BROWN
  {0x98, 0x98, 0x98, 0x00},  // LIGHTGRAY
};

STATIC
EFI_STATUS
EFIAPI
ControlledOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  )
{
  UINTN       Index;
  UINTN       Size;
  CHAR16      *StringCopy;
  EFI_STATUS  Status;

  if (mConsoleMode == EfiConsoleControlScreenText || !mIgnoreTextInGraphics) {
    if (mReplaceTabWithSpace) {
      Size = StrSize (String);

      StringCopy = AllocatePool (Size);
      if (StringCopy == NULL) {
        return mOriginalOutputString (This, String);
      }

      Size /= sizeof (CHAR16);
      for (Index = 0; Index < Size; Index++) {
        if (String[Index] == L'\t') {
          StringCopy[Index] = L' ';
        } else {
          StringCopy[Index] = String[Index];
        }
      }
      Status = mOriginalOutputString (This, StringCopy);
      FreePool(StringCopy);
      return Status;
    }

    return mOriginalOutputString (This, String);
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
  UINT32                                Width;
  UINT32                                Height;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  Status = OcHandleProtocolFallback (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );

  if (!EFI_ERROR (Status)) {
    Status = GraphicsOutput->QueryMode (
      GraphicsOutput,
      GraphicsOutput->Mode->Mode,
      &SizeOfInfo,
      &Info
      );
    if (!EFI_ERROR (Status)) {
      Width  = Info->HorizontalResolution;
      Height = Info->VerticalResolution;
      FreePool (Info);
    } else {
      GraphicsOutput = NULL;
    }
  } else {
    GraphicsOutput = NULL;
  }

  //
  // On APTIO V with large resolution (e.g. 2K or 4K) ClearScreen
  // invocation resets resolution to 1024x768. To avoid the glitches
  // we clear the screen manually, doing with other methods results
  // in screen flashes and other problems.
  //

  if (GraphicsOutput != NULL) {
    Status = GraphicsOutput->Blt (
      GraphicsOutput,
      &mEfiBackgroundColors[BitFieldRead32 ((UINT32) This->Mode->Attribute, 4, 6)],
      EfiBltVideoFill,
      0,
      0,
      0,
      0,
      Width,
      Height,
      0
      );

    This->SetCursorPosition (This, 0, 0);
  } else {
    Status = mOriginalClearScreen (This);
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

  if (mOriginalConsoleControlProtocol.GetMode != NULL) {
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

  if (GopUgaExists != NULL) {
    *GopUgaExists = TRUE;
  }

  if (StdInLocked != NULL) {
    *StdInLocked = FALSE;
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

  DEBUG ((DEBUG_INFO, "OCC: Setting cc mode %d -> %d\n", mConsoleMode, Mode));

  mConsoleMode = Mode;

  if (mClearScreenOnModeSwitch && Mode == EfiConsoleControlScreenText) {
    Status = OcHandleProtocolFallback (
      gST->ConsoleOutHandle,
      &gEfiGraphicsOutputProtocolGuid,
      (VOID **) &GraphicsOutput
      );

    if (!EFI_ERROR (Status)) {
      GraphicsOutput->Blt (
        GraphicsOutput,
        &mEfiBackgroundColors[BitFieldRead32 ((UINT32) gST->ConOut->Mode->Attribute, 4, 6)],
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

  if (mOriginalConsoleControlProtocol.SetMode != NULL) {
    return mOriginalConsoleControlProtocol.SetMode (
      This,
      Mode
      );
  }

  //
  // Disable and hide flashing cursor when switching to graphics from text.
  // This may be the case when we formerly handled text input or output.
  //
  if (Mode == EfiConsoleControlScreenGraphics) {
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
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
OcUseSystemTextOutput (
  IN OC_CONSOLE_RENDERER          Renderer,
  IN BOOLEAN                      IgnoreTextOutput,
  IN BOOLEAN                      SanitiseClearScreen,
  IN BOOLEAN                      ClearScreenOnModeSwitch,
  IN BOOLEAN                      ReplaceTabWithSpace
  )
{
  DEBUG ((
    DEBUG_INFO,
    "OCC: Configuring system console ignore %d san clear %d clear switch %d replace tab %d\n",
    IgnoreTextOutput,
    SanitiseClearScreen,
    ClearScreenOnModeSwitch,
    ReplaceTabWithSpace
    ));

  if (Renderer == OcConsoleRendererSystemGraphics) {
    OcConsoleControlSetMode (EfiConsoleControlScreenGraphics);
    OcConsoleControlInstallProtocol (&mConsoleControlProtocol, NULL, &mConsoleMode);
  } else if (Renderer == OcConsoleRendererSystemText) {
    OcConsoleControlSetMode (EfiConsoleControlScreenText);
    OcConsoleControlInstallProtocol (&mConsoleControlProtocol, NULL, &mConsoleMode);
  } else {
    OcConsoleControlInstallProtocol (&mConsoleControlProtocol, &mOriginalConsoleControlProtocol, &mConsoleMode);
  }

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

  return EFI_SUCCESS;
}

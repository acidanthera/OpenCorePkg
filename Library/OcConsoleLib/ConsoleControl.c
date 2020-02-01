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

      StringCopy = AllocatePool ((Length + 1) * sizeof(CHAR16));
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
  UINT32                                Width;
  UINT32                                Height;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  Status = gBS->HandleProtocol (
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
    STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mGraphicsEfiColors[8] = {
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

    Status = GraphicsOutput->Blt (
      GraphicsOutput,
      &mGraphicsEfiColors[BitFieldRead32 ((UINT32) This->Mode->Attribute, 4, 6)],
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

VOID
OcConsoleControlSync (
  VOID
  )
{
  if (mOriginalOutputString != NULL) {
    mOriginalOutputString     = gST->ConOut->OutputString;
    gST->ConOut->OutputString = ControlledOutputString;
  }

  if (mOriginalClearScreen != NULL) {
    mOriginalClearScreen      = gST->ConOut->ClearScreen;
    gST->ConOut->ClearScreen  = ControlledClearScreen;
  }
}

EFI_STATUS
ConsoleControlSetBehaviourForHandle (
  IN EFI_HANDLE                    Handle,
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

  if (Handle != NULL) {
    Status = gBS->HandleProtocol (
      Handle,
      &gEfiConsoleControlProtocolGuid,
      (VOID *) &ConsoleControl
      );
  } else {
    Status = gBS->LocateProtocol (
      &gEfiConsoleControlProtocolGuid,
      NULL,
      (VOID *) &ConsoleControl
      );
  }

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

EFI_STATUS
OcConsoleControlSetBehaviour (
  IN OC_CONSOLE_CONTROL_BEHAVIOUR  Behaviour
  )
{
  return ConsoleControlSetBehaviourForHandle (NULL, Behaviour);
}

/* CrScreenshotDxe.c

Copyright (c) 2016, Nikolaj Schlej, All rights reserved.

Redistribution and use in source and binary forms,
with or without modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OcPngLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>

#include <Protocol/AppleEvent.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/OcBootstrap.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleFileSystem.h>

//
// Keyboard protocol arrival event.
//
STATIC EFI_EVENT mProtocolNotification;

STATIC
EFI_STATUS
EFIAPI
ShowStatus (
  IN UINT8 Red,
  IN UINT8 Green,
  IN UINT8 Blue
  )
{
  //
  // Determines the size of status square.
  //
  #define STATUS_SQUARE_SIDE 5

  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Square[STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE];
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Backup[STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE];
  UINTN                          Index;

  Status = OcHandleProtocolFallback (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSCR: Graphics output protocol not found for status - %r\n", Status));
    return EFI_UNSUPPORTED;
  }

  //
  // Set square color.
  //
  for (Index = 0; Index < STATUS_SQUARE_SIDE * STATUS_SQUARE_SIDE; ++Index) {
    Square[Index].Blue     = Blue;
    Square[Index].Green    = Green;
    Square[Index].Red      = Red;
    Square[Index].Reserved = 0x00;
  }

  //
  // Backup current image.
  //
  GraphicsOutput->Blt (
    GraphicsOutput,
    Backup,
    EfiBltVideoToBltBuffer,
    0,
    0,
    0,
    0,
    STATUS_SQUARE_SIDE,
    STATUS_SQUARE_SIDE,
    0
    );

  //
  // Draw the status square.
  //
  GraphicsOutput->Blt (
    GraphicsOutput,
    Square,
    EfiBltBufferToVideo,
    0,
    0,
    0,
    0,
    STATUS_SQUARE_SIDE,
    STATUS_SQUARE_SIDE,
    0
    );

  //
  // Wait 500 ms.
  //
  gBS->Stall (500*1000);

  //
  // Restore the backup.
  //
  GraphicsOutput->Blt (
    GraphicsOutput,
    Backup,
    EfiBltBufferToVideo,
    0,
    0,
    0,
    0,
    STATUS_SQUARE_SIDE,
    STATUS_SQUARE_SIDE,
    0
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
TakeScreenshot (
  IN EFI_KEY_DATA *KeyData
  )
{
  EFI_FILE_PROTOCOL             *Fs;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Image;
  UINTN                         ImageSize;         ///< Size in pixels
  VOID                          *PngFile;
  UINTN                         PngFileSize;       ///< Size in bytes
  EFI_STATUS                    Status;
  UINT32                        ScreenWidth;
  UINT32                        ScreenHeight;
  CHAR16                        FileName[16];
  EFI_TIME                      Time;
  UINTN                         Index;
  UINT8                         Temp;

  Status = FindWritableOcFileSystem (&Fs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSCR: Can't find writable FS - %r\n", Status));
    ShowStatus (0xFF, 0xFF, 0x00); ///< Yellow
    return EFI_SUCCESS;
  }

  Status = OcHandleProtocolFallback (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSCR: Graphics output protocol not found for screen - %r\n", Status));
    Fs->Close (Fs);
    return EFI_SUCCESS;
  }

  //
  // Set screen width, height and image size in pixels.
  //
  ScreenWidth  = GraphicsOutput->Mode->Info->HorizontalResolution;
  ScreenHeight = GraphicsOutput->Mode->Info->VerticalResolution;
  ImageSize    = (UINTN) ScreenWidth * ScreenHeight;

  if (ImageSize == 0) {
    DEBUG ((DEBUG_INFO, "OCSCR: Empty screen size\n"));
    Fs->Close (Fs);
    return EFI_SUCCESS;
  }

  //
  // Get current time.
  //
  Status = gRT->GetTime (
    &Time,
    NULL
    );
  if (!EFI_ERROR (Status)) {
    //
    // Set file name to current day and time
    //
    UnicodeSPrint (
      FileName,
      sizeof (FileName),
      L"%02d%02d%02d%02d.png",
      Time.Day,
      Time.Hour,
      Time.Minute,
      Time.Second
      );
  } else {
    //
    // Set file name to scrnshot.png
    //
    StrCpyS (FileName, sizeof (FileName), L"scrnshot.png");
  }

  //
  // Allocate memory for screenshot.
  //
  Status = gBS->AllocatePool (
    EfiBootServicesData,
    ImageSize * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
    (VOID **) &Image
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "CRSCR: gBS->AllocatePool returned %r\n", Status));
    ShowStatus (0xFF, 0x00, 0x00); ///< Red
    Fs->Close (Fs);
    return EFI_SUCCESS;
  }

  //
  // Take screenshot.
  //
  Status = GraphicsOutput->Blt (
    GraphicsOutput,
    Image,
    EfiBltVideoToBltBuffer,
    0,
    0,
    0,
    0,
    ScreenWidth,
    ScreenHeight,
    0
    );
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_INFO, "CRSCR: GraphicsOutput->Blt returned %r\n", Status));
    gBS->FreePool (Image);
    ShowStatus (0xFF, 0x00, 0x00); ///< Red
    Fs->Close (Fs);
    return EFI_SUCCESS;
  }

  //
  // Convert BGR to RGBA with Alpha set to 0xFF.
  //
  for (Index = 0; Index < ImageSize; ++Index) {
    Temp                  = Image[Index].Blue;
    Image[Index].Blue     = Image[Index].Red;
    Image[Index].Red      = Temp;
    Image[Index].Reserved = 0xFF;
  }

  Status = OcEncodePng (
    Image,
    ScreenWidth,
    ScreenHeight,
    &PngFile,
    &PngFileSize
    );
  gBS->FreePool (Image);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "CRSCR: OcEncodePng returned %r\n", Status));
    ShowStatus (0xFF, 0x00, 0x00); ///< Red
    Fs->Close (Fs);
    return EFI_SUCCESS;
  }

  //
  // Write PNG image into the file.
  //
  Status = SetFileData (Fs, FileName, PngFile, (UINT32) PngFileSize);
  gBS->FreePool (PngFile);
  Fs->Close (Fs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "CRSCR: OcEncodePng returned %r\n", Status));
    ShowStatus (0xFF, 0x00, 0x00); ///< Red
    return EFI_SUCCESS;
  }

  //
  // Show success.
  //
  ShowStatus (0x00, 0xFF, 0x00); ///< Green

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
AppleEventKeyHandler (
  IN APPLE_EVENT_INFORMATION  *Information,
  IN VOID                     *NotifyContext
  )
{
  //
  // Mark the context argument as used.
  //
  (VOID) NotifyContext;

  //
  // Ignore invalid information if it happened to arrive.
  //
  if (Information == NULL || (Information->EventType & APPLE_EVENT_TYPE_KEY_UP) == 0) {
    return;
  }

  if (Information->EventData.KeyData->AppleKeyCode == AppleHidUsbKbUsageKeyF10) {
    //
    // Take a screenshot for F10.
    //
    TakeScreenshot (NULL);
  }
}

STATIC
EFI_STATUS
InstallKeyHandler (
  VOID
  )
{
  EFI_STATUS                         Status;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuffer;
  UINTN                              Index;
  EFI_KEY_DATA                       SimpleTextInExKeyStroke;
  EFI_HANDLE                         SimpleTextInExHandle;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleTextInEx;
  APPLE_EVENT_HANDLE                 AppleEventHandle;
  APPLE_EVENT_PROTOCOL               *AppleEvent;
  BOOLEAN                            Installed;

  Installed = FALSE;

  //
  // Locate compatible protocols, firstly try AppleEvent otherwise try SimpleTextInEx.
  // This is because we want key swap to take precedence.
  //
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gAppleEventProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; ++Index) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gAppleEventProtocolGuid, (VOID **) &AppleEvent);

      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_INFO,
          "CRSCR: gBS->HandleProtocol[%u] AppleEvent returned %r\n",
          (UINT32) Index,
          Status
          ));
        continue;
      }

      if (AppleEvent->Revision < APPLE_EVENT_PROTOCOL_REVISION_MINIMUM) {
        DEBUG ((
          DEBUG_INFO,
          "CRSCR: AppleEvent[%u] has outdated revision %u, expected %u\n",
          (UINT32) Index,
          (UINT32) AppleEvent->Revision,
          (UINT32) APPLE_EVENT_PROTOCOL_REVISION_MINIMUM
          ));
        continue;
      }

      //
      // Register key handler, which will later determine the combination.
      //
      Status = AppleEvent->RegisterHandler (
        APPLE_EVENT_TYPE_KEY_UP,
        AppleEventKeyHandler,
        &AppleEventHandle,
        NULL
        );
      if (!EFI_ERROR (Status)) {
        Installed = TRUE;
      }

      DEBUG ((
        DEBUG_INFO,
        "CRSCR: AppleEvent->RegisterHandler[%u] returned %r\n",
        (UINT32) Index,
        Status
        ));
    }

    gBS->FreePool (HandleBuffer);
  } else {
    DEBUG ((
      DEBUG_INFO,
      "CRSCR: gBS->LocateHandleBuffer AppleEvent returned %r\n",
      Status
      ));
  }

  if (!Installed) {
    Status = gBS->LocateHandleBuffer (
      ByProtocol,
      &gEfiSimpleTextInputExProtocolGuid,
      NULL,
      &HandleCount,
      &HandleBuffer
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "CRSCR: gBS->LocateHandleBuffer SimpleTextInEx returned %r\n",
        Status
        ));
      return EFI_UNSUPPORTED;
    }

    //
    // Register key notification function for F10.
    //
    SimpleTextInExKeyStroke.Key.ScanCode            = SCAN_F10;
    SimpleTextInExKeyStroke.Key.UnicodeChar         = 0;
    SimpleTextInExKeyStroke.KeyState.KeyShiftState  = 0;
    SimpleTextInExKeyStroke.KeyState.KeyToggleState = 0;

    for (Index = 0; Index < HandleCount; ++Index) {
      Status = gBS->HandleProtocol (
        HandleBuffer[Index],
        &gEfiSimpleTextInputExProtocolGuid,
        (VOID **) &SimpleTextInEx
        );

      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_INFO,
          "CRSCR: gBS->HandleProtocol[%u] SimpleTextInputEx returned %r\n",
          (UINT32) Index,
          Status
          ));
        continue;
      }

      Status = SimpleTextInEx->RegisterKeyNotify (
        SimpleTextInEx,
        &SimpleTextInExKeyStroke,
        TakeScreenshot,
        &SimpleTextInExHandle
        );

      if (!EFI_ERROR (Status)) {
        Installed = TRUE;
      }

      DEBUG ((
        DEBUG_INFO,
        "CRSCR: SimpleTextInEx->RegisterKeyNotify[%u] returned %r\n",
        (UINT32) Index,
        Status
        ));
    }

    gBS->FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
InstallKeyHandlerWrapper (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  InstallKeyHandler ();
  gBS->CloseEvent (mProtocolNotification);
}

EFI_STATUS
EFIAPI
CrScreenshotDxeEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  Status = InstallKeyHandler ();
  if (EFI_ERROR (Status)) {
    Status = gBS->CreateEvent (
      EVT_NOTIFY_SIGNAL,
      TPL_CALLBACK,
      InstallKeyHandlerWrapper,
      NULL,
      &mProtocolNotification
      );

    if (!EFI_ERROR (Status)) {
      gBS->RegisterProtocolNotify (
        &gEfiSimpleTextInputExProtocolGuid,
        mProtocolNotification,
        &Registration
        );

      gBS->RegisterProtocolNotify (
        &gAppleEventProtocolGuid,
        mProtocolNotification,
        &Registration
        );
    } else {
      DEBUG ((
        DEBUG_INFO,
        "CRSCR: Cannot create event for keyboard protocol arrivals %r\n",
        Status
        ));
    }
  }

  return EFI_SUCCESS;
}

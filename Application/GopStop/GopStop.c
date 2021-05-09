/** @file
  Test graphics output protocol.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Guid/AppleFile.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>

STATIC
VOID
AnalyzeGopHandle (
  IN  EFI_HANDLE        Handle,
  IN  UINTN             GopIndex,
  IN  UINTN             HandleCount,
  OUT CHAR8             *Report
  )
{
  EFI_STATUS                            Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *Gop;
  UINT32                                Index;
  CHAR8                                 Tmp[256];
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINTN                                 InfoSize;
  UINTN                                 Width;
  UINTN                                 Height;
  INTN                                  NewMode;

  Status = gBS->HandleProtocol (
    Handle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &Gop
    );

  //
  // GOP report:
  //
  AsciiSPrint (
    Tmp,
    sizeof (Tmp),
    "GOP #%u of total %u, %a - %r\n",
    (UINT32) (GopIndex + 1),
    (UINT32) HandleCount,
    gST->ConsoleOutHandle == Handle ? "console" : "auxiliary",
    Status
    );
  DEBUG ((DEBUG_WARN, "GSTT: %a", Tmp));
  AsciiStrCatS (Report, EFI_PAGE_SIZE, Tmp);

  if (EFI_ERROR (Status)) {
    return;
  }

  AsciiSPrint (
    Tmp,
    sizeof (Tmp),
    "Current mode %u, max mode %u, FB: %p, FBS: %LX\n",
    Gop->Mode->Mode,
    Gop->Mode->MaxMode,
    (UINT64) Gop->Mode->FrameBufferBase,
    (UINT32) Gop->Mode->FrameBufferSize
    );
  DEBUG ((DEBUG_WARN, "GSTT: %a", Tmp));
  AsciiStrCatS (Report, EFI_PAGE_SIZE, Tmp);

  Info = Gop->Mode->Info;
  AsciiSPrint (
    Tmp,
    sizeof (Tmp),
    "Current: %u x %u, pixel %d (%X %X %X %X), scan: %u\n",
    Info->HorizontalResolution,
    Info->VerticalResolution,
    Info->PixelFormat,
    Info->PixelInformation.RedMask,
    Info->PixelInformation.GreenMask,
    Info->PixelInformation.BlueMask,
    Info->PixelInformation.ReservedMask,
    Info->PixelsPerScanLine
    );
  DEBUG ((DEBUG_WARN, "GSTT: %a", Tmp));
  AsciiStrCatS (Report, EFI_PAGE_SIZE, Tmp);

  Width   = 0;
  Height  = 0;
  NewMode = -1;

  for (Index = 0; Index < Gop->Mode->MaxMode; ++Index) {
    Status = Gop->QueryMode (
      Gop,
      Index,
      &InfoSize,
      &Info
      );

    if (EFI_ERROR (Status)) {
      AsciiSPrint (
        Tmp,
        sizeof (Tmp),
        "%u: %r\n",
        Index,
        Status
        );
      DEBUG ((DEBUG_WARN, "GSTT: %a", Tmp));
      AsciiStrCatS (Report, EFI_PAGE_SIZE, Tmp);
      continue;
    }

    AsciiSPrint (
      Tmp,
      sizeof (Tmp),
      "%u: %u x %u, pixel %d (%X %X %X %X), scan: %u\n",
      Index,
      Info->HorizontalResolution,
      Info->VerticalResolution,
      Info->PixelFormat,
      Info->PixelInformation.RedMask,
      Info->PixelInformation.GreenMask,
      Info->PixelInformation.BlueMask,
      Info->PixelInformation.ReservedMask,
      Info->PixelsPerScanLine
      );
    DEBUG ((DEBUG_WARN, "GSTT: %a", Tmp));
    AsciiStrCatS (Report, EFI_PAGE_SIZE, Tmp);

    if (Info->HorizontalResolution > Width
      || (Info->HorizontalResolution == Width && Info->VerticalResolution > Height)) {
      Width   = Info->HorizontalResolution;
      Height  = Info->VerticalResolution;
      NewMode = (INTN) Index;
    }

    FreePool (Info);
  }

  if (NewMode >= 0) {
    Status = Gop->SetMode (
      Gop,
      (UINT32) NewMode
      );

    Info = Gop->Mode->Info;
    AsciiSPrint (
      Tmp,
      sizeof (Tmp),
      "New %u <-> %u: max mode %u, FB: %p, FBS: %LX - %r\n",
      (UINT32) NewMode,
      Gop->Mode->Mode,
      Gop->Mode->MaxMode,
      (UINT64) Gop->Mode->FrameBufferBase,
      (UINT32) Gop->Mode->FrameBufferSize,
      Status
      );
    DEBUG ((DEBUG_WARN, "GSTT: %a", Tmp));
    AsciiStrCatS (Report, EFI_PAGE_SIZE, Tmp);
    AsciiSPrint (
      Tmp,
      sizeof (Tmp),
      "New %u <-> %u: %u x %u, pixel %d (%X %X %X %X), scan: %u\n",
      (UINT32) NewMode,
      Gop->Mode->Mode,
      Info->HorizontalResolution,
      Info->VerticalResolution,
      Info->PixelFormat,
      Info->PixelInformation.RedMask,
      Info->PixelInformation.GreenMask,
      Info->PixelInformation.BlueMask,
      Info->PixelInformation.ReservedMask,
      Info->PixelsPerScanLine
      );
    DEBUG ((DEBUG_WARN, "GSTT: %a", Tmp));
    AsciiStrCatS (Report, EFI_PAGE_SIZE, Tmp);
  }
}

STATIC
VOID
RunGopTest (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  UINTN                         Index;
  UINT32                        ChunkX;
  UINT32                        ChunkY;
  UINT32                        ChunkW;
  UINT32                        ChunkH;
  UINT32                        ColorIndex;


  Status = gBS->HandleProtocol (
    Handle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &Gop
    );

  if (EFI_ERROR (Status)
    || Gop->Mode->Info->HorizontalResolution == 0
    || Gop->Mode->Info->VerticalResolution == 0) {
    return;
  }

  STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mGraphicsEfiColors[16] = {
    //
    // B    G    R   reserved
    //
    {0x00, 0x00, 0x00, 0x00},  //  0 - BLACK
    {0x98, 0x00, 0x00, 0x00},  //  1 - LIGHTBLUE
    {0x00, 0x98, 0x00, 0x00},  //  2 - LIGHGREEN
    {0x98, 0x98, 0x00, 0x00},  //  3 - LIGHCYAN
    {0x00, 0x00, 0x98, 0x00},  //  4 - LIGHRED
    {0x98, 0x00, 0x98, 0x00},  //  5 - MAGENTA
    {0x00, 0x98, 0x98, 0x00},  //  6 - BROWN
    {0x98, 0x98, 0x98, 0x00},  //  7 - LIGHTGRAY
    {0x30, 0x30, 0x30, 0x00},  //  8 - DARKGRAY - BRIGHT BLACK
    {0xff, 0x00, 0x00, 0x00},  //  9 - BLUE
    {0x00, 0xff, 0x00, 0x00},  // 10 - LIME
    {0xff, 0xff, 0x00, 0x00},  // 11 - CYAN
    {0x00, 0x00, 0xff, 0x00},  // 12 - RED
    {0xff, 0x00, 0xff, 0x00},  // 13 - FUCHSIA
    {0x00, 0xff, 0xff, 0x00},  // 14 - YELLOW
    {0xff, 0xff, 0xff, 0x00}   // 15 - WHITE
  };

  STATIC UINT32 mColorsTest[9] = {12, 10, 9, 15, 7, 0, 11, 5, 14};

  if (Gop->Mode->FrameBufferBase != 0) {
    //
    // 1. Fill screen with Red (#FF0000) in direct mode.
    //
    SetMem32 (
      (VOID *)(UINTN) Gop->Mode->FrameBufferBase,
      Gop->Mode->Info->VerticalResolution * Gop->Mode->Info->PixelsPerScanLine * sizeof (UINT32),
      *(UINT32 *) &mGraphicsEfiColors[mColorsTest[0]]
      );

    //
    // 2. Wait 5 seconds.
    // Note: Ensure that stall value is within UINT32 in nanoseconds.
    //
    for (Index = 0; Index < 5; ++Index) {
      gBS->Stall (SECONDS_TO_MICROSECONDS (1));
    }
  }

  //
  // 3. Fill screen with 4 Red (#FF0000) / Green (#00FF00) / Blue (#0000FF) / White (#FFFFFF) rectangles.
  // The user should visually ensure that the colours match and that rectangles equally split the screen in 4 parts.
  //
  ChunkW = Gop->Mode->Info->HorizontalResolution / 2;
  ChunkH = Gop->Mode->Info->VerticalResolution / 2;
  ColorIndex = 0;
  for (ChunkY = 0; ChunkY + ChunkH <= Gop->Mode->Info->VerticalResolution; ChunkY += ChunkH) {
    for (ChunkX = 0; ChunkX + ChunkW <= Gop->Mode->Info->HorizontalResolution; ChunkX += ChunkW) {
      Gop->Blt (
        Gop,
        &mGraphicsEfiColors[mColorsTest[ColorIndex]],
        EfiBltVideoFill,
        0,
        0,
        ChunkX,
        ChunkY,
        ChunkW,
        ChunkH,
        0
        );
      ++ColorIndex;
    }
  }

  //
  // 4. Wait 5 seconds.
  // Note: Ensure that stall value is within UINT32 in nanoseconds.
  //
  for (Index = 0; Index < 5; ++Index) {
    gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  }

  //
  // TODO:
  // 5. Fill screen with white text on black screen in normal mode.
  //    The screen should effectively be filled with 9 equal rectangles.
  //    From left to right top to bottom they should contain the following symbols: A B C D # F G H I.
  //    Instead of # there should be current GOP number. The user should visually ensure text and
  //    background colour, rectangle sizes, rectangle data.
  // 6. Wait 5 seconds.
  // 7. Fill screen with white text on black screen in HiDPI mode. This should repeat the previous test
  //    but the text should be twice bigger. The user should ensure all previous requirements and the
  //    fact that the text got bigger.
  // 8. Wait 5 seconds.
  // 9. Print all GOP reports one by one (on separate screens) with white text in black screen in
  //    normal mode. Wait 5 seconds after each. The user should screenshot these and later compare
  //    to the file if available.
  //

  //
  // 10. Fill screen with 9 rectangles of different colours. From left to right top to bottom they
  // should contain the following colours: Red (#FF0000), Green (#00FF00), Blue (#0000FF), White (#FFFFFF),
  // Light Grey (#989898), Black (#000000), Cyan (#00FFFF), Magenta (#FF00FF), Yellow (#FFFF00). The user should
  // visually ensure that the colours match and that rectangles equally split the screen in 9 parts.
  //
  ChunkW = Gop->Mode->Info->HorizontalResolution / 3;
  ChunkH = Gop->Mode->Info->VerticalResolution / 3;
  ColorIndex = 0;
  for (ChunkY = 0; ChunkY + ChunkH <= Gop->Mode->Info->VerticalResolution; ChunkY += ChunkH) {
    for (ChunkX = 0; ChunkX + ChunkW <= Gop->Mode->Info->HorizontalResolution; ChunkX += ChunkW) {
      Gop->Blt (
        Gop,
        &mGraphicsEfiColors[mColorsTest[ColorIndex]],
        EfiBltVideoFill,
        0,
        0,
        ChunkX,
        ChunkY,
        ChunkW,
        ChunkH,
        0
        );
      ++ColorIndex;
    }
  }

  //
  // 11. Wait 5 seconds.
  // Note: Ensure that stall value is within UINT32 in nanoseconds.
  //
  for (Index = 0; Index < 5; ++Index) {
    gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                           Status;
  UINTN                                HandleCount;
  EFI_HANDLE                           *HandleBuffer;
  UINTN                                Index;
  CHAR8                                (*Reports)[EFI_PAGE_SIZE];
  CHAR8                                *FinalReport;
  CHAR16                               Filename[64];
  EFI_TIME                             Date;

  //
  // 1. Disable watchdog timer.
  //
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //
  // 2. Gather all N available GOP protocols.
  //
  HandleCount = 0;
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  DEBUG ((
    DEBUG_WARN,
    "GSTT: Found %u handles with GOP protocol - %r\n",
    (UINT32) HandleCount,
    Status
    ));

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // 3. Allocate N buffer for GOP reports (PAGE_SIZE).
  //
  Reports = AllocateZeroPool (EFI_PAGE_SIZE * HandleCount);
  if (Reports == NULL) {
    DEBUG ((DEBUG_WARN, "GSTT: Cannot allocate memory for GOP reports\n"));
    FreePool (HandleBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // 4. Get GOP reports for every GOP and set maximum resolution.
  //
  for (Index = 0; Index < HandleCount; ++Index) {
    AnalyzeGopHandle (HandleBuffer[Index], Index, HandleCount, Reports[Index]);
  }

  //
  // 5. Save GOP reports to file to any writable fs (gop-date.txt).
  //
  FinalReport = AllocateZeroPool (EFI_PAGE_SIZE * HandleCount);
  if (FinalReport != NULL) {
    for (Index = 0; Index < HandleCount; ++Index) {
      AsciiStrCatS (FinalReport, EFI_PAGE_SIZE * HandleCount, Reports[Index]);
      AsciiStrCatS (FinalReport, EFI_PAGE_SIZE * HandleCount, "\n\n");
    }

    Status = gRT->GetTime (&Date, NULL);
    if (EFI_ERROR (Status)) {
      ZeroMem (&Date, sizeof (Date));
    }

    UnicodeSPrint (
      Filename,
      sizeof (Filename),
      L"gop-%04u-%02u-%02u-%02u%02u%02u.txt",
      (UINT32) Date.Year,
      (UINT32) Date.Month,
      (UINT32) Date.Day,
      (UINT32) Date.Hour,
      (UINT32) Date.Minute,
      (UINT32) Date.Second
      );

    SetFileData (NULL, Filename, FinalReport, (UINT32) AsciiStrLen (FinalReport));

    FreePool (FinalReport);
  } else {
    DEBUG ((DEBUG_WARN, "GSTT: Cannot allocate memory for final report\n"));
  }

  //
  // 6. Run tests in every GOP with non-zero resolution.
  //
  for (Index = 0; Index < HandleCount; ++Index) {
    RunGopTest (HandleBuffer[Index]);
  }

  FreePool (Reports);
  FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

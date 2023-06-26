/** @file
  Test implemented font pages.

  Copyright (c) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define APP_TITLE  L"  <<<Font Tester>>>  "

#define NROWS     (8)
#define NCOLUMNS  (16)
#define GAP       (8)

STATIC
VOID
PauseSeconds (
  UINTN  Seconds
  )
{
  UINTN  Index;

  //
  // Note: Ensure that stall value is within UINT32 in nanoseconds (required on some broken firmware).
  //
  for (Index = 0; Index < Seconds; ++Index) {
    gBS->Stall (SECONDS_TO_MICROSECONDS (1));
  }
}

STATIC
VOID
PrintFontPage (
  UINT8  PageNumber,
  UINTN  CursorColumn,
  UINTN  CursorRow
  )
{
  CHAR16  Char;
  CHAR16  String[2];
  UINTN   Row;
  UINTN   Column;

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_CYAN | EFI_DARKGRAY);
  gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumn, CursorRow);
  Print (L"Page %u:", PageNumber);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

  String[1] = CHAR_NULL;
  Char      = PageNumber << 7;

  for (Row = 0; Row < NROWS; Row++) {
    for (Column = 0; Column < NCOLUMNS; Column++) {
      String[0] = (Char < 32) ? L'_' : Char;

      gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumn + (Column * 2) + 1, CursorRow + Row + 1);
      gST->ConOut->OutputString (gST->ConOut, String);

      ++Char;
    }
  }
}

UINTN
CentrePos (
  UINTN  Width
  )
{
  EFI_STATUS  Status;
  UINTN       Columns;
  UINTN       Rows;

  Status = gST->ConOut->QueryMode (
                          gST->ConOut,
                          gST->ConOut->Mode->Mode,
                          &Columns,
                          &Rows
                          );

  if (EFI_ERROR (Status) || (Columns < Width)) {
    return 0;
  }

  //
  // Note: Subtract then divide can be different by one, and is less well centred.
  //
  return (Columns / 2) - (Width / 2);
}

VOID
Centre (
  CHAR16  *String
  )
{
  gST->ConOut->SetCursorPosition (
                 gST->ConOut,
                 CentrePos (StrLen (String)),
                 gST->ConOut->Mode->CursorRow
                 );

  gST->ConOut->OutputString (
                 gST->ConOut,
                 String
                 );
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN    Column0;
  UINTN    Column1;
  UINTN    Row0;
  UINTN    Row1;
  UINTN    Row2;
  INT32    OriginalAttribute;
  BOOLEAN  OriginalCursorVisible;

  OriginalAttribute     = gST->ConOut->Mode->Attribute;
  OriginalCursorVisible = gST->ConOut->Mode->CursorVisible;

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
  gST->ConOut->ClearScreen (gST->ConOut);

  Print (L"\r\n");
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BROWN | EFI_CYAN);
  Centre (APP_TITLE);

  Column0 = CentrePos (NCOLUMNS * 4 + GAP);
  Column1 = Column0 + NCOLUMNS * 2 + GAP;
  Row0    = gST->ConOut->Mode->CursorRow + 2;
  Row1    = Row0 + 10;
  Row2    = Row1 + 10;

  PrintFontPage (0, Column0, Row0);
  PrintFontPage (67, Column1, Row0);
  PrintFontPage (74, Column0, Row1);
  PrintFontPage (75, Column1, Row1);

  //
  // Give time for F10 screenshot.
  //
  PauseSeconds (2);

  gST->ConOut->SetCursorPosition (gST->ConOut, 0, Row2);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_RED | EFI_WHITE);
  Centre (L"Press any key...");
  WaitForKeyPress (L"");
  Centre (L"Done.");

  gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
  gST->ConOut->EnableCursor (gST->ConOut, OriginalCursorVisible);

  return EFI_SUCCESS;
}

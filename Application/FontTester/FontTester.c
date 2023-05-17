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
  CHAR16  String[(NCOLUMNS * 2) + 1];
  UINTN   Row;
  UINTN   Column;

  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_CYAN | EFI_DARKGRAY);
  gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumn, CursorRow);
  Print (L"Page %u:", PageNumber);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

  for (Column = 0; Column < NCOLUMNS; Column++) {
    String[Column * 2] = L' ';
  }

  String[NCOLUMNS * 2] = CHAR_NULL;
  Char                 = PageNumber << 7;

  for (Row = 0; Row < NROWS; Row++) {
    for (Column = 0; Column < NCOLUMNS; Column++) {
      if (Char < 32) {
        String[(Column * 2) + 1] = L'_';
      } else {
        String[(Column * 2) + 1] = Char;
      }

      ++Char;
    }

    gST->ConOut->SetCursorPosition (gST->ConOut, CursorColumn, CursorRow + Row + 2);
    gST->ConOut->OutputString (gST->ConOut, String);
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
  UINTN    Pos0;
  UINTN    Pos1;
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

  Pos0 = CentrePos (NCOLUMNS * 4 + GAP);
  Pos1 = Pos0 + NCOLUMNS * 2 + GAP;

  PrintFontPage (0, Pos0, 3);
  PrintFontPage (1, Pos1, 3);
  PrintFontPage (74, Pos0, 14);
  PrintFontPage (75, Pos1, 14);

  //
  // Give time for screenshot.
  //
  PauseSeconds (2);

  Print (L"\r\n\r\n\r\n");
  gST->ConOut->SetAttribute (gST->ConOut, EFI_BACKGROUND_RED | EFI_WHITE);
  Centre (L"Press any key...");
  WaitForKeyPress (L"");
  Centre (L"Done.\r\n");

  gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
  gST->ConOut->EnableCursor (gST->ConOut, OriginalCursorVisible);

  return EFI_SUCCESS;
}

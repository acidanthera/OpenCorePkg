/** @file
  Control MSR 0xE2 UI Input/Output in the widest sense

Copyright (c) 2020, Brumbaer. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ControlMsrE2.h"

UINTN           mArgumentFlags;

/*
  Read up to Length -1 Characters from keyboard.
  CR will exit
  LF will exit
  Del key is supported
  Esc any input will be cleared. If there isn't any ReadLine will exít
  When length - 1 characters are entered Readline will exit automatically.
*/
UINT32
ReadLine (
  OUT CHAR16   *Buffer,
  IN  UINT32   Length
  )
{
  EFI_STATUS     Status;
  UINTN          EventIndex;
  EFI_INPUT_KEY  Key;
  UINT32         Index;
  UINT32         Pos;
  INT32          StartRow;
  INT32          StartColumn;

  Pos = 0;

  STATIC CHAR16  Output[] = L"A";

  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  StartRow = gST->ConOut->Mode->CursorRow;
  StartColumn = gST->ConOut->Mode->CursorColumn;

  do {
    //
    // Read a key
    //
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    ZeroMem (&Key, sizeof (Key));
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_READY) {
        continue;
      }
      break;
    }

    switch (Key.UnicodeChar) {
      case CHAR_BACKSPACE:
        if (Pos > 0) {
          --Pos;
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
          gST->ConOut->OutputString (gST->ConOut, L" ");
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
        }
        break;

      case CHAR_ESC:
        if (Pos > 0) {
          Pos = 0;
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
          for (Index = 1; Index < Length; ++Index) {
            gST->ConOut->OutputString (gST->ConOut, L" ");
          }
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
        } else {
          Buffer[Pos] = 0;
          return Pos;
        }
        break;

      case CHAR_CARRIAGE_RETURN:
      case CHAR_LINEFEED:
        Buffer[Pos] = 0;
        gST->ConOut->EnableCursor (gST->ConOut, FALSE);
        return Pos;

      default:
        Buffer[Pos] = Key.UnicodeChar;
        ++Pos;
        Output[0] = Key.UnicodeChar;
        gST->ConOut->OutputString (gST->ConOut, Output);

        if (Pos == Length - 1) {
          Buffer[Pos] = 0;
          return Pos;
        }
        break;
    }
  } while (TRUE);

  return 0;
}

CHAR16
ReadAnyKey (
  VOID
  )
{
  CHAR16 Keys[2];
  ReadLine (Keys, 2);
  return Keys[0];
}

BOOLEAN
ReadYN (
  VOID
  )
{
  CHAR16 Key;
  do {
    Key = ReadAnyKey ();
  } while (Key != 'y' && Key != 'Y' && Key != 'n' && Key != 'N');

  return Key == 'y' || Key == 'Y';
}

#define TOKENLENGTH 32

EFI_STATUS
InterpretArguments (
  VOID
  )
{
  UINTN       Argc;
  CHAR16      **Argv;
  CHAR16      *Parameter;
  UINTN       ParameterCount;
  CHAR16      *Token;
  UINT16      TokenIndex;
  UINT32      Index;
  EFI_STATUS  Status;

  mArgumentFlags = ARG_VERIFY;

  Status = GetArguments (&Argc, &Argv);
  if (EFI_ERROR (Status)) {
    Print (L"Unable to get arguments - %r\n", Status);
    return Status;
  }

  ParameterCount = 0;

  for (Index = 1; Index < Argc; ++Index) {
    Token = AllocateCopyPool (StrSize (Argv[Index]), Argv[Index]);
    if (Token == NULL) {
      Print (L"Could not allocate memory for Token.\n");
      return EFI_OUT_OF_RESOURCES;
    }

    TokenIndex = 0;

    while (Token[TokenIndex] != '\0') {
      if (Token[TokenIndex] == ' ') {
        ++TokenIndex;
        continue;
      }

      Parameter = &Token[TokenIndex];

      do {
        ++TokenIndex;
      } while (Token[TokenIndex] != '\0' && Token[TokenIndex] != ' ');

      Token[TokenIndex] = '\0';

      if (OcStriCmp (Parameter, L"lock") == 0) {
        mArgumentFlags = ARG_LOCK;
      } else if (OcStriCmp (Parameter, L"unlock") == 0) {
        mArgumentFlags = ARG_UNLOCK;
      } else if (OcStriCmp (Parameter, L"interactive") == 0) {
        mArgumentFlags = ARG_INTERACTIVE;
      } else {
        Print (L"Unknown command line argument: %s\n", Parameter);
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      ++ParameterCount;
    }
    FreePool (Token);
  }

  if (ParameterCount > 1) {
    Print (L"interactive, unlock, lock are exclusive options. Use only one of them.\n\n");
    Status = EFI_INVALID_PARAMETER;
  }

  if (ParameterCount == 0) {
    Print (L"No option selected, verify only.\n");
    Print (L"Usage: ControlMsrE2 <unlock | lock | interactive>\n\n");
  }

  return Status;
}

VOID
ModifySearchString (
  IN OUT EFI_STRING *SearchString
  )
{
  BOOLEAN   Result;

  do {
    Print (L"\nCurrent search string: %s\n", *SearchString);
    Print (L"Do you want to change it ? ");
    Result = ReadYN ();
    Print (L"\n");
    if (Result) {
      Print (L"Enter search string:\n");

      CHAR16 *Buffer = AllocatePool (BUFFER_LENGTH * sizeof (CHAR16));

      if (Buffer != NULL) {
        if (ReadLine (Buffer, BUFFER_LENGTH) == 0) {
          Print (L"\nNo Input. Search string not changed.\n");
          FreePool (Buffer);
        } else {
          Print (L"\n");
          FreePool (*SearchString);
          *SearchString = Buffer;
        }
      } else {
        Print (L"Could not allocate memory. Search string can not be changed.\n");
      }
    }
  } while (Result);
}

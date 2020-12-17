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


UINTN           Flags;

/*
  Read up to length -1 Characters from keyboard.
  CR will exit
  LF will exit
  Del key is supported
  Esc any input will be cleared. If there isn't any ReadLine will exít
  When length - 1 characters are entered Readline will exit automatically.
*/
UINT32 ReadLine (
  OUT CHAR16   *buffer,
  IN  UINT32   length
  )
{
  EFI_STATUS     Status;
  UINTN          EventIndex;
  EFI_INPUT_KEY  Key;
  UINT32         Pos;
  INT32          StartRow;
  INT32          StartColumn;

  Pos = 0;

  STATIC CHAR16  Output[] = L"A";

  gST->ConOut->EnableCursor(gST->ConOut, TRUE);

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
          Pos--;
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
          gST->ConOut->OutputString (gST->ConOut, L" ");
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
        }
        break;

      case 0x1B:
        if (Pos > 0) {
          Pos = 0;
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
          for (UINT32 i = 1; i < length; i++) {
            gST->ConOut->OutputString (gST->ConOut, L" ");
          }
          gST->ConOut->SetCursorPosition (gST->ConOut, StartColumn + Pos, StartRow);
        } else {
          buffer[Pos] = 0;
          return Pos;
        }
        break;

      case CHAR_CARRIAGE_RETURN:
      case CHAR_LINEFEED:
        buffer[Pos] = 0;
        gST->ConOut->EnableCursor(gST->ConOut, FALSE);
        return Pos;

      default:
        buffer[Pos++] = Key.UnicodeChar;
        Output[0] = Key.UnicodeChar;
        gST->ConOut->OutputString (gST->ConOut, Output);

        if (Pos >= length - 1) {
          buffer[Pos] = 0;
          return Pos;
        }
        break;
    }
  } while (TRUE);

  return 0;
}

CHAR16 ReadAnyKey () {
  CHAR16 keys[2];
  ReadLine(keys, 2);
  return keys[0];
}

UINT32 ReadYN () {
  CHAR16 keys[2];
  do {
    ReadLine(keys, 2);
  } while (!OcStriStr (L"yn", keys));

  return keys[0] == 'y' || keys[0] == 'Y';
}

EFI_STATUS
PrintGuid (
  IN EFI_GUID *Guid
  )
/*++
Routine Description:
  This function prints a GUID to STDOUT.
Arguments:
  Guid    Pointer to a GUID to print.
Returns:
  EFI_SUCCESS             The GUID was printed.
  EFI_INVALID_PARAMETER   The input was NULL.
--*/
{
  if (Guid == NULL) {
    // Error (NULL, 0, 2000, "Invalid parameter", "PrintGuidToBuffer() called with a NULL value");
    return EFI_INVALID_PARAMETER;
  }

  Print (
    L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
    (unsigned) Guid->Data1,
    Guid->Data2,
    Guid->Data3,
    Guid->Data4[0],
    Guid->Data4[1],
    Guid->Data4[2],
    Guid->Data4[3],
    Guid->Data4[4],
    Guid->Data4[5],
    Guid->Data4[6],
    Guid->Data4[7]
    );
  return EFI_SUCCESS;
}

VOID PrintUINT8Str (
  IN UINT8 *String
  )
{
  AsciiPrint ((CHAR8*) String);
}

#define TOKENLENGTH 32;

EFI_STATUS InterpretArguments ()
{
  UINTN       Argc;
  CHAR16      **Argv;
  CHAR16      *Parameter;
  UINTN       ParameterCount;
  CHAR16      *Token;
  UINT16      TokenIndex;

  Flags = 0;

  if (EFI_ERROR(GetArguments (&Argc, &Argv)))
    Argc = 1;

  ParameterCount = 0;

  for (UINT32 i = 1; i < Argc; i++) {
    Token = AllocatePool (StrSize(Argv[i]));

    if (Token) {
      StrCpyS (Token, StrLen(Argv[i]) + 1, Argv[i]);

      TokenIndex = 0;

      while (Argv[i][TokenIndex]) {
        while (Argv[i][TokenIndex] == ' ') {
          TokenIndex++;
        }

        if (Argv[i][TokenIndex]) {
          Parameter = &Token[TokenIndex];

          while (Token[TokenIndex] != 0 && Token[TokenIndex] != ' ') {
            TokenIndex++;
          }
          Token[TokenIndex] = 0;

          if (!StrCmp (Parameter, L"check")) {
            Flags |= ARG_CHECK;
            ParameterCount++;
          } else if (!StrCmp (Parameter, L"lock")) {
            Flags |= ARG_LOCK;
            ParameterCount++;
          } else if (!StrCmp (Parameter, L"unlock")) {
            Flags |= ARG_UNLOCK;
            ParameterCount++;
          } else if (!StrCmp (Parameter, L"interactive")) {
            Flags |= ARG_INTERACTIVE;
            ParameterCount++;
          } else if (!StrCmp (Parameter, L"-v")) {
            Flags |= ARG_VERBOSE;
          } else {
            Print (L"Ignoring unknown command line argument: %s\n", Parameter);
          }
        }
      }  ///<  All Tokens parsed
      FreePool (Token);
    } else {
      Print (L"Couldn't allocate memory.\n");
      return EFI_OUT_OF_RESOURCES;
    }
  }  ///< All Arguments analysed

  if (ParameterCount == 0) {
    Flags |= ARG_UNLOCK;
    Print (L"No option selected, default to unlock.\n");
    Print (L"Usage: ControlMsrE2 <unlock | lock | interactive> [-v]\n\n");
  } else if (ParameterCount > 1) {
    Print (L"interactive, unlock, lock, check are exclusive options. Use only one of them.\n\n");
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STRING
ModifySearchString (
  IN EFI_STRING SearchString
  )
{
  UINT32 Flag;

  do {
    Print (L"\nCurrent search string: %s\n", SearchString);
    Print (L"Do you want to change it ? ");
    if ((Flag = ReadYN()) == TRUE) {
      Print (L"\nEnter search string: ");

      CHAR16 *Buffer = AllocatePool(BUFFER_LENGTH * sizeof(CHAR16));

      if (Buffer != NULL) {
        if (ReadLine (Buffer, BUFFER_LENGTH) == 0) {
          Print (L"\nNo Input. Search string not changed.\n");
          FreePool (Buffer);
        } else {
          FreePool (SearchString);
          SearchString = Buffer;
        }
      } else {
        Print (L"Could not allocate memory. Search string can not be changed.\n");
      }
    }
  } while (Flag);

  Print (L"\n");

  return SearchString;
}

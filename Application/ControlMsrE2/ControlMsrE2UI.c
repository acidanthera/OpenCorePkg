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


UINTN                     mFlags;

EFI_STRING AllocateStrFromAscii (CHAR8 *s) {
    EFI_STRING r = AllocatePool(sizeof(CHAR16) * (AsciiStrLen(s) + 1));
    
    CHAR16 *rp = r;
    
    for (CHAR8 *sp = s; *sp != 0; sp++, rp++)
        *rp = *sp;
    
    *rp = 0;
    return r;
}

UINT8 EFIAPI Char16IgnoreCaseCompare (CHAR16 a, CHAR16 b) {
    if (a >= 'A' && a <= 'Z') {
        a = a | 0x20;
    }
    if (b >= 'A' && b <= 'Z') {
        b = b | 0x20;
    }
    return a - b;
}

UINT8 EFIAPI StrContains (CHAR16* a, CHAR16* b) {
    
    while (*a != 0) {
        while (Char16IgnoreCaseCompare(*a, *b) != 0) {
            if (*a == 0) return FALSE;
            
            a++;
        }
        
        CHAR16* c = a;
        CHAR16* d = b;
        
        while (Char16IgnoreCaseCompare(*c, *d) == 0) {
            if (*c == 0) return TRUE;
            c++;
            d++;
        }
        if (*d == 0) return TRUE;
        if (*c == 0) return FALSE;
        
        a++;
    }
    return FALSE;
}

/*
 Read up to length -1 Characters from keyboard.
 CR will exit
 LF will exit
 Del key is supported
 Esc any input will be cleared. If there isn't any ReadLine will exít
 When length - 1 characters are entered Readline will exit automatically.
 */
UINT32 ReadLine (OUT CHAR16* buffer, IN UINT32 length) {
    
    EFI_STATUS     Status;
    UINTN          EventIndex;
    EFI_INPUT_KEY  Key;
    
    int pos = 0;
    STATIC CHAR16  Output[] = L"A";
    
    gST->ConOut->EnableCursor(gST->ConOut, TRUE);
    
    INT32 startRow = gST->ConOut->Mode->CursorRow;
    INT32 startColumn = gST->ConOut->Mode->CursorColumn;
    
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
                if (pos > 0) {
                    pos--;
                    gST->ConOut->SetCursorPosition (gST->ConOut, startColumn + pos, startRow);
                    gST->ConOut->OutputString (gST->ConOut, L" ");
                    gST->ConOut->SetCursorPosition (gST->ConOut, startColumn + pos, startRow);
                }
                break;
            case 0x1B:
                if (pos > 0) {
                    pos = 0;
                    gST->ConOut->SetCursorPosition (gST->ConOut, startColumn + pos, startRow);
                    for (int i = 1; i < length; i++) {
                        gST->ConOut->OutputString (gST->ConOut, L" ");
                    }
                    gST->ConOut->SetCursorPosition (gST->ConOut, startColumn + pos, startRow);
                }
                else {
                    buffer[pos] = 0;
                    return pos;
                }
                break;
            case CHAR_CARRIAGE_RETURN:
            case CHAR_LINEFEED:
                buffer[pos] = 0;
                gST->ConOut->EnableCursor(gST->ConOut, FALSE);
                return pos;
            default:
                buffer[pos++] = Key.UnicodeChar;
                Output[0] = Key.UnicodeChar;
                gST->ConOut->OutputString (gST->ConOut, Output);
                
                if (pos >= length - 1) {
                    buffer[pos] = 0;
                    return pos;
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
    } while (!StrContains (L"yn", keys));
    
    return keys[0] == 'y' || keys[0] == 'Y';
}

VOID PrintGuid (IN EFI_GUID* Guid) {
    
    Print (L"%08x-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
           Guid->Data1,
           Guid->Data2,
           Guid->Data3,
           Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3], Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]
           );
}

VOID PrintUINT8Str (UINT8 n[1]) {
    AsciiPrint ((CHAR8*)(&n[0]));
}

#define TOKENLENGTH 32;

EFI_STATUS InterpretArguments () {
    UINTN             Argc;
    CHAR16            **Argv;
    UINTN             ParameterCount;
    
    mFlags = 0;
    
    if (EFI_ERROR(GetArguments (&Argc, &Argv)))
        Argc = 1;
    
    ParameterCount = 0;
    
    for (UINT32 i = 1; i < Argc; i++) {
        
        CHAR16  token[StrLen(Argv[i]) + 1];
        StrCpyS (token, StrLen(Argv[i]) + 1, Argv[i]);
        
        UINT16 tokenIndex = 0;;
        
        while (Argv[i][tokenIndex]) {
            while (Argv[i][tokenIndex] == ' ') {
                tokenIndex++;
            }
            
            if (Argv[i][tokenIndex]) {
                CHAR16* s = &token[tokenIndex];
                
                while (token[tokenIndex] != 0 && token[tokenIndex] != ' ') {
                    tokenIndex++;
                }
                token[tokenIndex] = 0;
                
                if (!StrCmp (s, L"check")) {
                    mFlags |= ARG_CHECK;
                    ParameterCount++;
                }
                else if (!StrCmp (s, L"lock")) {
                    mFlags |= ARG_LOCK;
                    ParameterCount++;
                }
                else if (!StrCmp (s, L"unlock")) {
                    mFlags |= ARG_UNLOCK;
                    ParameterCount++;
                }
                else if (!StrCmp (s, L"interactive")) {
                    mFlags |= ARG_INTERACTIVE;
                    ParameterCount++;
                }
                else if (!StrCmp (s, L"-v")) {
                    mFlags |= ARG_VERBOSE;
                }
                else {
                    Print (L"Ignoring unknown command line argument: %s\n", s);
                }
            }
        } // All Tokens parsed
    } // All Arguments analysed
    
    if (ParameterCount == 0) {
        mFlags = mFlags | ARG_UNLOCK;
        Print (L"No option selected, default to unlock.\n");
        Print (L"Usage: ControlMsrE2 <unlock | lock | interactive> [-v]\n\n");
    }
    else if (ParameterCount > 1) {
        Print (L"interactive, unlock, lock, check are exclusive options. Use only one of them.\n\n");
        return EFI_INVALID_PARAMETER;
    }
    
    
    return EFI_SUCCESS;
}

EFI_STRING ModifySearchString (IN EFI_STRING SearchString) {
    int flag;
    
    do {
        Print (L"\nCurrent search string: %s\n", SearchString);
        Print (L"Do you want to change it ? ");
        if ((flag = ReadYN()) == TRUE) {
            Print (L"\nEnter search string: ");
            
            CHAR16 *Buffer = AllocatePool(BUFFER_LENGTH * sizeof(CHAR16));
            
            if (ReadLine (Buffer, BUFFER_LENGTH) == 0) {
                Print (L"\nNo Input. Search string not changed.\n");
                FreePool(Buffer);
            }
            else {
                FreePool(SearchString);
                SearchString = Buffer;
            }
        }
    } while (flag);
    
    Print (L"\n");
    
    return SearchString;
}

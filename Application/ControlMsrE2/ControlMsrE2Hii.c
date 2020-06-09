/** @file
 Control MSR 0xE2 Hii HiiDatabase related stuff
 
 Copyright (c) 2020, Brumbaer. All rights reserved.<BR>
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

#include "ControlMsrE2.h"

typedef struct {
    EFI_VARSTORE_ID     id;
    EFI_IFR_VARSTORE   *varstoreHeader;
} VarStoreContext;

typedef VOID EFIAPI OpCodeHandler (EFI_IFR_OP_HEADER* IfrHeader, UINT8* Stop, VOID* Context);

EFI_HII_PACKAGE_LIST_HEADER* HiiExportPackageLists (IN EFI_HII_HANDLE Handle) {
    
    EFI_STATUS Status:
    UINTN BufferSize = 0;

    Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, NULL);
    
    if (!EFI_ERROR(Status) && BufferSize > 0) {
        EFI_HII_PACKAGE_LIST_HEADER* result = (EFI_HII_PACKAGE_LIST_HEADER*) AllocatePool (BufferSize);
        
        Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, result);
        
        if (EFI_ERROR(Status)) {
            FreePool (result);
            return NULL;
        }
        else {
            return result;
        }
    }
    return NULL;
}

EFI_IFR_OP_HEADER* DoForEachOpCode (
                 IN EFI_IFR_OP_HEADER* Header,
                 IN UINT8 OpCode,
                 IN UINT8* Stop,
                 IN void* Context,
                 IN OpCodeHandler Handler
                 ) {
    
    do {
        if (Header->OpCode == EFI_IFR_END_OP)
            return Header;
        if (Header->OpCode == OpCode) {
            Handler (Header, Stop, Context);
            if ((Stop != NULL) && *Stop)
                return Header;
        }
        if (Header->Scope) {
            Header = DoForEachOpCode (PADD(Header, Header->Length), OpCode, Stop, Context, Handler);
        }
        
        Header = PADD(Header, Header->Length);
    } while ((Stop == NULL) || !(*Stop));
    
    return Header;
}

VOID HandleVarStore (
                EFI_IFR_OP_HEADER* IfrHeader,
                UINT8* Stop,
                void* Context
                ) {
    
    VarStoreContext *ctx = Context;
    EFI_IFR_VARSTORE* varStore = (EFI_IFR_VARSTORE*) IfrHeader;
    
    if (varStore->VarStoreId == ctx->id) {
        ctx->varstoreHeader = varStore;
        if (Stop)
            *Stop = TRUE;
    }
}

EFI_IFR_VARSTORE* EFIAPI GetVarStore (
                                      EFI_IFR_OP_HEADER* header,
                                      EFI_VARSTORE_ID id
                                      ) {
    
    UINT8 Stop = FALSE;
    VarStoreContext Context = { id, NULL };
    
    DoForEachOpCode (header, EFI_IFR_VARSTORE_OP, &Stop, &Context, HandleVarStore);
    
    return Context.varstoreHeader;
}

VOID HandleOneOf (
             EFI_IFR_OP_HEADER* IfrHeader,
             UINT8* Stop,
             VOID* Context) {
    
    
    OneOfContext* ctx = Context;
    UINT8 *Data;
    UINTN DataSize;
    EFI_STATUS Status;
    EFI_IFR_VARSTORE* ifrVarStore;
    EFI_IFR_ONE_OF* ifrOneOf = (EFI_IFR_ONE_OF*) IfrHeader;
    EFI_STRING s = HiiGetString (ctx->efiHandle, ifrOneOf->Question.Header.Prompt, "en-US");
    
    if ((ifrVarStore = GetVarStore (ctx->firstIfrHeader, ifrOneOf->Question.VarStoreId)) != NULL) {
        if (OcStriStr(s, ctx->searchText)) {
            UINT16 old = ctx->count;
            
            if (ctx->ifrOneOf == NULL) {
                ctx->ifrOneOf = ifrOneOf;
                ctx->ifrVarStore = ifrVarStore;
                ctx->count++;
            }
            else { // Skip identical Options
                if ((ctx->ifrOneOf->Question.VarStoreId != ifrOneOf->Question.VarStoreId) || (ctx->ifrOneOf->Question.VarStoreInfo.VarOffset != ifrOneOf->Question.VarStoreInfo.VarOffset)) {
                    ctx->ifrOneOf = ifrOneOf;
                    ctx->ifrVarStore = ifrVarStore;
                    ctx->count++;
                }
            }
            
            if (ctx->count == ctx->stopAt && Stop != NULL) {
                ctx->ifrOneOf = ifrOneOf;
                ctx->ifrVarStore = ifrVarStore;
                ctx->count = 1;
                *Stop = TRUE;
            }
            else if (old != ctx->count && ctx->stopAt == DONT_STOP_AT) {
                Print (L"%X. %02X %04X %04X /%s/ VarStore Name: ", ctx->count, ifrOneOf->Header.OpCode, ifrOneOf->Question.VarStoreInfo.VarName, ifrOneOf->Question.VarStoreId, s);
                
                PrintUINT8Str(ifrVarStore->Name);
                
                EFI_STRING s = AsciiStrCopyToUnicode((CHAR8*)ifrVarStore->Name, 0);
                
                DataSize = 0;
                if ((Status = gRT->GetVariable (s, &ifrVarStore->Guid, NULL, &DataSize, NULL)) == EFI_BUFFER_TOO_SMALL) {
                    if ((Data = AllocatePool (DataSize)) != NULL) {
                        if ((Status = gRT->GetVariable (s, &ifrVarStore->Guid, NULL, &DataSize, Data)) == EFI_SUCCESS) {
                            int varSize = sizeof (EFI_IFR_ONE_OF) - ifrOneOf->Header.Length;
                            varSize = 8 - (varSize / 3);

                            UINT8 *p = Data + ifrOneOf->Question.VarStoreInfo.VarOffset;
                            UINT64 value = (varSize == 1) ? *p : (varSize == 2) ? *(UINT16*)(p) : (varSize == 4) ? *(UINT32*)(p)  : *(UINT64*)(p);
                            
                            Print (L" Value: value %X", value);
                            
                            FreePool (Data);
                        }
                    } // Allocate
                } // GetVariable
               
                Print (L"\n");
            }
        }
    }
    
    FreePool(s);
}

VOID EFIAPI HandleOneVariable (OneOfContext* Context) {
    EFI_STATUS                    Status;
    UINTN                         DataSize;
    UINT8                         *Data;
    UINT32                        Attributes;
    
    EFI_STRING s = HiiGetString (Context->efiHandle, Context->ifrOneOf->Question.Header.Prompt, "en-US");
    Print (L"\nBIOS Option found: %s\n", s);
    FreePool(s);
    
    Print (L"In VarStore \"");
    PrintUINT8Str (Context->ifrVarStore->Name);
    Print (L"\" GUID: ");
    PrintGuid (&Context->ifrVarStore->Guid);
    
    int varSize = sizeof (EFI_IFR_ONE_OF) - Context->ifrOneOf->Header.Length;
    varSize = 8 - (varSize / 3);
    Print (L" Offset: %04X Size: %X ", Context->ifrOneOf->Question.VarStoreInfo.VarOffset, varSize);
    
    DataSize = 0;
    s = AsciiStrCopyToUnicode((CHAR8*)Context->ifrVarStore->Name, 0);
    if ((Status = gRT->GetVariable (s, &Context->ifrVarStore->Guid, &Attributes, &DataSize, NULL)) == EFI_BUFFER_TOO_SMALL) {
        if ((Data = AllocatePool (DataSize)) != NULL) {
            if ((Status = gRT->GetVariable (s, &Context->ifrVarStore->Guid, &Attributes, &DataSize, Data)) == EFI_SUCCESS) {
                
                UINT8 *p = Data + Context->ifrOneOf->Question.VarStoreInfo.VarOffset;
                UINT64 newValue;
                UINT64 value = (varSize == 1) ? *p : (varSize == 2) ? *(UINT16*)(p) : (varSize == 4) ? *(UINT32*)(p)  : *(UINT64*)(p);
                
                Print (L"Value: value %X\n", value);
                
                newValue = (IS_INTERACTIVE()) ? (value) ? 0 : 1 : (IS_LOCK()) ? 1 : (IS_UNLOCK()) ? 0 : value;
                
                if (newValue != value) {
                    if (IS_INTERACTIVE()) {
                        Print (L"Do you want to toggle the value y/n ?");
                    }
                    else if (IS_LOCK()) {
                        Print (L"Do you want to set the value y/n ?");
                    }
                    else {
                        Print (L"Do you want to clear the value y/n ?");
                    }
                    
                    if (ReadYN()) {
                        
                        switch (varSize) {
                            case 1:
                                *p = newValue;
                                break;
                            case 2:
                                *(UINT16*)(p) = newValue;
                                break;
                            case 4:
                                *(UINT32*)(p) = newValue;
                                break;
                            case 8:
                                *(UINT64*)(p) = newValue;
                                break;
                                
                            default:
                                break;
                        }
                        
                        if ((Status = gRT->SetVariable (s, &Context->ifrVarStore->Guid, Attributes, DataSize, Data)) == EFI_SUCCESS) {
                            Print (L"\nDone. You will have to reboot for the change to take effect.\n");
                        }
                        else {
                            Print (L"\nProblem writing variable.\n");
                        }
                    }
                    else {
                        Print (L"\n");
                    }
                }
                else {
                    Print (L"Value is as wanted already. No action required.\n");
                }
            }
            else
                Print (L"\nCouldn't read Data\n");
            FreePool(Data);
        }
        else {
            Print (L"\nCouldn't allocate memory\n");
        }
    }
    else
        Print (L"\nCouldn't find Variable.\n");
    
    FreePool(s);
}


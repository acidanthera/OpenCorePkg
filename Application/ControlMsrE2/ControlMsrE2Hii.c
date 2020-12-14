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

typedef struct VarStoreContext_ {
  EFI_VARSTORE_ID    Id;
  EFI_IFR_VARSTORE   *VarstoreHeader;
} VarStoreContext;

EFI_HII_PACKAGE_LIST_HEADER* HiiExportPackageLists (
  IN EFI_HII_HANDLE Handle
  )
{
  EFI_STATUS   Status;
  UINTN        BufferSize;

  BufferSize = 0;

  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, NULL);

  if ((Status == EFI_BUFFER_TOO_SMALL) && (BufferSize > 0)) {
    EFI_HII_PACKAGE_LIST_HEADER* result = (EFI_HII_PACKAGE_LIST_HEADER*) AllocatePool (BufferSize);
    
    if (result == NULL)
      return NULL;

    Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, result);

    if (EFI_ERROR(Status)) {
      FreePool (result);
      return NULL;
    } else {
      return result;
    }
  }
  return NULL;
}

EFI_IFR_OP_HEADER* DoForEachOpCode (
  IN     EFI_IFR_OP_HEADER   *Header,
  IN     UINT8               OpCode,
  IN OUT UINT8               *Stop     OPTIONAL,
  IN     void                *Context,
  IN     OP_CODE_HANDLER     Handler
  )
{
  while ((Stop == NULL) || !(*Stop)) {
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
  }

  return Header;
}

VOID HandleVarStore (
  IN     EFI_IFR_OP_HEADER   *IfrHeader,
  IN OUT UINT8               *Stop       OPTIONAL,
  IN OUT void                *Context
  )
{
  VarStoreContext *ctx = Context;
  EFI_IFR_VARSTORE* varStore = (EFI_IFR_VARSTORE*) IfrHeader;

  if (varStore->VarStoreId == ctx->Id) {
    ctx->VarstoreHeader = varStore;
    if (Stop)
      *Stop = TRUE;
  }
}

EFI_IFR_VARSTORE*  GetVarStore (
  IN EFI_IFR_OP_HEADER   *Header,
  IN EFI_VARSTORE_ID     Id
  )
{
  UINT8           Stop;
  VarStoreContext Context;

  Stop = FALSE;
  Context.Id = Id;
  Context.VarstoreHeader = NULL;

  DoForEachOpCode (Header, EFI_IFR_VARSTORE_OP, &Stop, &Context, HandleVarStore);

  return Context.VarstoreHeader;
}

VOID HandleOneOf (
  IN     EFI_IFR_OP_HEADER   *IfrHeader,
  IN OUT UINT8               *Stop       OPTIONAL,
  IN OUT VOID                *Context
  )
{
  ONE_OF_CONTEXT* ctx;
  UINT8 *Data;
  UINTN DataSize;
  EFI_STATUS Status;
  EFI_IFR_VARSTORE *IfrVarStore;
  EFI_IFR_ONE_OF *IfrOneOf;
  EFI_STRING s;

  ctx = Context;
  IfrOneOf = (EFI_IFR_ONE_OF*) IfrHeader;
  s = HiiGetString (ctx->EfiHandle, IfrOneOf->Question.Header.Prompt, "en-US");

  if ((s != NULL) && ((IfrVarStore = GetVarStore (ctx->FirstIfrHeader, IfrOneOf->Question.VarStoreId)) != NULL)) {
    
    if (OcStriStr(s, ctx->SearchText)) {
      UINT16 old = ctx->Count;
      
      if (ctx->IfrOneOf == NULL) {
        ctx->IfrOneOf = IfrOneOf;
        ctx->IfrVarStore = IfrVarStore;
        ctx->Count++;
      }
      else { // Skip identical Options
        if ((ctx->IfrOneOf->Question.VarStoreId != IfrOneOf->Question.VarStoreId) || (ctx->IfrOneOf->Question.VarStoreInfo.VarOffset != IfrOneOf->Question.VarStoreInfo.VarOffset)) {
          ctx->IfrOneOf = IfrOneOf;
          ctx->IfrVarStore = IfrVarStore;
          ctx->Count++;
        }
      }
      
      if (ctx->Count == ctx->StopAt && Stop != NULL) {
        ctx->IfrOneOf = IfrOneOf;
        ctx->IfrVarStore = IfrVarStore;
        ctx->Count = 1;
        *Stop = TRUE;
      }
      else if (old != ctx->Count && ctx->StopAt == DONT_STOP_AT) {
        Print (L"%X. %02X %04X %04X /%s/ VarStore Name: ", ctx->Count, IfrOneOf->Header.OpCode, IfrOneOf->Question.VarStoreInfo.VarName, IfrOneOf->Question.VarStoreId, s);
        
        PrintUINT8Str(IfrVarStore->Name);
        
        EFI_STRING VarStoreName = AsciiStrCopyToUnicode((CHAR8*) IfrVarStore->Name, 0);
        
        DataSize = 0;
        if ((Status = gRT->GetVariable (VarStoreName, (void*) &IfrVarStore->Guid, NULL, &DataSize, NULL)) == EFI_BUFFER_TOO_SMALL) {
          if ((Data = AllocatePool (DataSize)) != NULL) {
            if ((Status = gRT->GetVariable (VarStoreName, (void*) &IfrVarStore->Guid, NULL, &DataSize, Data)) == EFI_SUCCESS) {
              int varSize = sizeof (EFI_IFR_ONE_OF) - IfrOneOf->Header.Length;
              varSize = 8 - (varSize / 3);

              UINT8 *p = Data + IfrOneOf->Question.VarStoreInfo.VarOffset;
              UINT64 value = (varSize == 1) ? *p : (varSize == 2) ? *(UINT16*) (p) : (varSize == 4) ? *(UINT32*) (p)  : *(UINT64*) (p);
              
              Print (L" Value: value %X", value);
              
            }
            FreePool (Data);
          } // Allocate
        } // GetVariable
         
        Print (L"\n");
      }
    }
  }
  
  if (s != NULL) {
    FreePool(s);
  }
}

VOID  HandleOneVariable (
  IN OUT ONE_OF_CONTEXT* Context
  )
{
  EFI_STATUS          Status;
  UINTN             DataSize;
  UINT8             *Data;
  UINT32            Attributes;
  
  EFI_STRING s = HiiGetString (Context->EfiHandle, Context->IfrOneOf->Question.Header.Prompt, "en-US");
  if (s) {
    Print (L"\nBIOS Option found: %s\n", s);
    FreePool(s);
  }
  
  Print (L"In VarStore \"");
  PrintUINT8Str (Context->IfrVarStore->Name);
  Print (L"\" GUID: ");
  PrintGuid ((void*) &Context->IfrVarStore->Guid);
  
  int varSize = sizeof (EFI_IFR_ONE_OF) - Context->IfrOneOf->Header.Length;
  varSize = 8 - (varSize / 3);
  Print (L" Offset: %04X Size: %X ", Context->IfrOneOf->Question.VarStoreInfo.VarOffset, varSize);
  
  DataSize = 0;
  s = AsciiStrCopyToUnicode((CHAR8*) Context->IfrVarStore->Name, 0);

  if (s == NULL) {
    Print (L"\nCouldn't allocate memory\n");
  }
  else {
    if ((Status = gRT->GetVariable (s, (void*) &Context->IfrVarStore->Guid, &Attributes, &DataSize, NULL)) == EFI_BUFFER_TOO_SMALL) {
      if ((Data = AllocatePool (DataSize)) != NULL) {
        if ((Status = gRT->GetVariable (s, (void*) &Context->IfrVarStore->Guid, &Attributes, &DataSize, Data)) == EFI_SUCCESS) {
          
          UINT8 *p = Data + Context->IfrOneOf->Question.VarStoreInfo.VarOffset;
          UINT64 newValue;
          UINT64 value = (varSize == 1) ? *p : (varSize == 2) ? *(UINT16*) (p) : (varSize == 4) ? *(UINT32*) (p)  : *(UINT64*) (p);
          
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
                  *p = (UINT8)newValue;
                  break;
                case 2:
                  *(UINT16*) (p) = (UINT16)newValue;
                  break;
                case 4:
                  *(UINT32*) (p) = (UINT32)newValue;
                  break;
                case 8:
                  *(UINT64*) (p) = newValue;
                  break;
                  
                default:
                  break;
              }
              
              if ((Status = gRT->SetVariable (s, (void*) &Context->IfrVarStore->Guid, Attributes, DataSize, Data)) == EFI_SUCCESS) {
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
}

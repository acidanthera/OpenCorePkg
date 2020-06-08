/** @file
 Control CFGLock BIOS Option
 
 Copyright (c) 2020, Brumbaer. All rights reserved.<BR>
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
#include "ControlMsrE2.h"

#define CONTEXTS_MAX 8

EFI_GUID FormSetupClassGuid = EFI_HII_PLATFORM_SETUP_FORMSET_GUID;

EFI_STATUS
EFIAPI
UefiMain (
          IN EFI_HANDLE        ImageHandle,
          IN EFI_SYSTEM_TABLE  *SystemTable
          )
{
    EFI_HII_HANDLE * handles;
    UINT32           OptionsCount;
    UINT32           ContextsCount;
    OneOfContext     Contexts[CONTEXTS_MAX];
    
    EFI_STATUS Status = InterpretArguments();
    
    if (!EFI_ERROR(Status)) {
        Status = VerifyMSRE2 (ImageHandle, SystemTable);
    }
    
    if (!EFI_ERROR(Status)) {
        Print(L"\nBIOS Options:\n");

        EFI_STRING SearchString = AllocateStrFromAscii ("cfg");
        
        if (IS_INTERACTIVE()) {
            SearchString = ModifySearchString(SearchString);
        }
        
        handles = HiiGetHiiHandles (NULL);
        
        if (handles == NULL) {
            Print (L"Could not retrieve HiiHandles.\n");
            Status = EFI_OUT_OF_RESOURCES;
        }
        else {
            UINT32 ListHeaderCount;
            UINT32 ListHeaderIndex;
            
            ListHeaderCount = 0;
            for (EFI_HII_HANDLE* h = (EFI_HII_HANDLE*) handles; *h != NULL; h++) {
                ListHeaderCount++;
            }

            // Keep list alive 'til program finishes.
            // So that all lists can be searched, the results be displayed together.
            // And from all those one Option will be selected to be changed
            EFI_HII_PACKAGE_LIST_HEADER *ListHeaders[ListHeaderCount];
            
            OptionsCount = 0;
            ContextsCount = 0;
            ListHeaderIndex = 0;
            
            // For Each Handle
            for (EFI_HII_HANDLE* h = (EFI_HII_HANDLE*) handles; (*h != NULL) && (ContextsCount < CONTEXTS_MAX); h++, ListHeaderIndex++) {
                
                ListHeaders[ListHeaderIndex] = HiiExportPackageLists(*h);
                
                if (ListHeaders[ListHeaderIndex]) {
                    
                    if (IS_VERBOSE()) {
                        Print (L"Package List: ");
                        PrintGuid (&ListHeaders[ListHeaderIndex]->PackageListGuid);
                        Print (L"\n");
                    }
                    
                    // First package in list
                    EFI_HII_PACKAGE_HEADER* PkgHeader = PADD(ListHeaders[ListHeaderIndex], sizeof(EFI_HII_PACKAGE_LIST_HEADER));
                    
                    // For each package in list
                    do {
                        if (PkgHeader->Type == EFI_HII_PACKAGE_END) {
                            break;
                        }
                        
                        if (IS_VERBOSE()) {
                            Print (L"Package Type: %02X ", PkgHeader->Type);
                        }
                        
                        if (PkgHeader->Type == EFI_HII_PACKAGE_FORMS) {
                            
                            EFI_IFR_OP_HEADER* IfrHeader = PADD(PkgHeader, sizeof(EFI_HII_PACKAGE_HEADER));
                            
                            if (IfrHeader->OpCode == EFI_IFR_FORM_SET_OP) { // Form Definition must start with FORM_SET_OP
                                
                                // Print some Info
                                if (IS_VERBOSE()) {
                                    Print (L"Form: ");
                                    PrintGuid (&(((EFI_IFR_FORM_SET*) IfrHeader)->Guid));
                                }
                                
                                if (IfrHeader->Length >= 16 + sizeof(EFI_IFR_FORM_SET)) {
                                    if (IS_VERBOSE()) {
                                        Print (L" Class Guid: ");
                                        PrintGuid (PADD(IfrHeader, sizeof(EFI_IFR_FORM_SET)));
                                        Print(L"\n");
                                    }
                                    
                                    // Checkup for Setup Form
                                    if (CompareGuid (&FormSetupClassGuid, PADD(IfrHeader, sizeof(EFI_IFR_FORM_SET)))) {
                                        
                                        Contexts[ContextsCount] = (OneOfContext) {
                                            SearchString,
                                            *h,
                                            ListHeaders[ListHeaderIndex],
                                            PkgHeader,
                                            PADD(IfrHeader, IfrHeader->Length),
                                            NULL,
                                            NULL,
                                            DONT_STOP_AT,
                                            OptionsCount
                                        };
                                        
                                        DoForEachOpCode (Contexts[ContextsCount].firstIfrHeader, EFI_IFR_ONE_OF_OP, NULL, &Contexts[ContextsCount], HandleOneOf);
                                        
                                        if (Contexts[ContextsCount].count != OptionsCount) {
                                            OptionsCount = Contexts[ContextsCount].count;
                                            ContextsCount++;
                                        }
                                    }
                                }
                            }
                        }
                        PkgHeader = PADD(PkgHeader, PkgHeader->Length);
                    } while (ContextsCount < CONTEXTS_MAX); // For each package in list
                    
                    if (IS_VERBOSE()) {
                        Print(L"\n");
                    }
                } // ListHeader End
            }  // For Each Handle
            
            if (IS_VERBOSE()) {
                Print (L"ContextCount %x OptionsCount %x\n", ContextsCount, OptionsCount);
            }
            
            if (IS_INTERACTIVE() || IS_LOCK() || IS_UNLOCK()) {
                if (OptionsCount > 9 || ContextsCount == CONTEXTS_MAX) {
                    Print (L"Too many corresponding BIOS Options found. Try a different search string using interactive mode.\n");
                }
                else if (OptionsCount == 0) {
                    Print (L"No corresponding BIOS Options found. Try a different search string using interactive mode.\n");
                }
                else {
                    CHAR16 key = '1';
                    
                    if (OptionsCount > 1) {
                        do {
                            Print (L"\nEnter choice (1..%x) ? ", OptionsCount);
                            key = ReadAnyKey();
                        } while ((key < '1') && (key > '0' + OptionsCount) && (key != 0x1B));
                        
                        Print (L"\n");
                    }
                    
                    if (key != 0x1B) {
                        UINT8 stop;
                        UINT32 i;
                        UINT16 Index = key - '0';
                        
                        for (i = 0; i < ContextsCount; i++) {
                            if (Contexts[i].count >= Index) {
                                Contexts[i].count = (i == 0) ? 0 : (Contexts[i - 1].count);
                                Contexts[i].stopAt = Index;
                                Contexts[i].ifrOneOf = NULL;
                                
                                DoForEachOpCode (Contexts[i].firstIfrHeader, EFI_IFR_ONE_OF_OP, &stop, &Contexts[i], HandleOneOf);
                                
                                if (Contexts[i].ifrOneOf != NULL) {
                                    HandleOneVariable (&Contexts[i]);
                                }
                                break;
                            }
                        }
                    }
                }
            }
            
            for (ListHeaderIndex = 0; ListHeaderIndex < ListHeaderCount; ListHeaderIndex++) {
                if (ListHeaders[ListHeaderIndex] != NULL) {
                    FreePool (ListHeaders[ListHeaderIndex]);
                }
            }
            
            
        } // Handling Handles
        FreePool (SearchString);
    }
    
    Print (L"Press any key. \n");
    ReadAnyKey();
    return Status;
}

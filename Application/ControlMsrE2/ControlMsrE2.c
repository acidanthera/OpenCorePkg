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

EFI_STATUS WalkListHeaders (
  IN EFI_HII_HANDLE                *HiiHandles,
  IN EFI_HII_PACKAGE_LIST_HEADER   **ListHeaders,
  IN UINT32                        ListHeaderCount,
  IN EFI_STRING                    SearchString
  )
{
  EFI_STATUS               Status;
  UINT16                   OptionsCount;
  UINT16                   ContextsCount;
  ONE_OF_CONTEXT           Contexts[CONTEXTS_MAX];
  UINT32                   ListHeaderIndex;
  EFI_IFR_OP_HEADER        *IfrHeader;
  EFI_HII_PACKAGE_HEADER   *PkgHeader;
  UINT8                    Stop;
  UINT32                   ContextIndex;
  UINT16                   Index;
  CHAR16                   Key;

  Status = EFI_SUCCESS;

  OptionsCount = 0;
  ContextsCount = 0;

  //
  // For Each Handle
  //
  for (ListHeaderIndex = 0; (HiiHandles[ListHeaderIndex] != NULL) && (ContextsCount < CONTEXTS_MAX); ListHeaderIndex++) {
    ListHeaders[ListHeaderIndex] = HiiExportPackageLists(HiiHandles[ListHeaderIndex]);

    if (ListHeaders[ListHeaderIndex] != NULL) {

      if (IS_VERBOSE ()) {
        Print (L"Package List: ");
        PrintGuid (&ListHeaders[ListHeaderIndex]->PackageListGuid);
        Print (L"\n");
      }

      //
      // First package in list
      //
      PkgHeader = PADD (ListHeaders[ListHeaderIndex], sizeof (EFI_HII_PACKAGE_LIST_HEADER));

      //
      // For each package in list
      //
      while (ContextsCount < CONTEXTS_MAX) {
        if (IS_VERBOSE ()) {
          Print (L"Package Type: %02X ", PkgHeader->Type);
        }

        if (PkgHeader->Type == EFI_HII_PACKAGE_END) {
          break;
        } else if (PkgHeader->Type == EFI_HII_PACKAGE_FORMS) {
          IfrHeader = PADD (PkgHeader, sizeof(EFI_HII_PACKAGE_HEADER));

          if (IfrHeader->OpCode == EFI_IFR_FORM_SET_OP) { // Form Definition must start with FORM_SET_OP

            //
            // Print some Info
            //
            if (IS_VERBOSE ()) {
              Print (L"Form: ");
              PrintGuid ((void*) &(((EFI_IFR_FORM_SET*) IfrHeader)->Guid));
            }

            if (IfrHeader->Length >= 16 + sizeof(EFI_IFR_FORM_SET)) {
              if (IS_VERBOSE ()) {
                Print (L" Class Guid: ");
                PrintGuid (PADD(IfrHeader, sizeof(EFI_IFR_FORM_SET)));
                Print(L"\n");
              }

              //
              // Checkup for Setup Form
              //
              if (CompareGuid (&gEfiHiiPlatformSetupFormsetGuid, PADD(IfrHeader, sizeof(EFI_IFR_FORM_SET)))) {
                Contexts[ContextsCount].SearchText = SearchString;
                Contexts[ContextsCount].EfiHandle = HiiHandles[ListHeaderIndex];
                Contexts[ContextsCount].ListHeader = ListHeaders[ListHeaderIndex];
                Contexts[ContextsCount].PkgHeader = PkgHeader;
                Contexts[ContextsCount].FirstIfrHeader = PADD(IfrHeader, IfrHeader->Length);
                Contexts[ContextsCount].IfrVarStore = NULL;
                Contexts[ContextsCount].IfrOneOf = NULL;
                Contexts[ContextsCount].StopAt = DONT_STOP_AT;
                Contexts[ContextsCount].Count = OptionsCount;

                DoForEachOpCode (Contexts[ContextsCount].FirstIfrHeader, EFI_IFR_ONE_OF_OP, NULL, &Contexts[ContextsCount], HandleOneOf);

                if (Contexts[ContextsCount].Count != OptionsCount) {
                  OptionsCount = Contexts[ContextsCount].Count;
                  ContextsCount++;
                }
              }
            }
          }
        }
        PkgHeader = PADD (PkgHeader, PkgHeader->Length);
      }  ///< For each package in list

      if (IS_VERBOSE ()) {
        Print(L"\n");
      }
    }  ///< ListHeader End
  }  ///< For Each Handle

  if (IS_VERBOSE ()) {
    Print (L"Context Count: %x Options Count %x\n", ContextsCount, OptionsCount);
  }

  if (IS_INTERACTIVE () || IS_LOCK () || IS_UNLOCK ()) {
    if (OptionsCount > 9 || ContextsCount == CONTEXTS_MAX) {
      Print (L"Too many corresponding BIOS Options found. Try a different search string using interactive mode.\n");
    } else if (OptionsCount == 0) {
      Print (L"No corresponding BIOS Options found. Try a different search string using interactive mode.\n");
    } else {
      Key = '1';

      if (OptionsCount > 1) {
        do {
          Print (L"\nEnter choice (1..%x) ? ", OptionsCount);
          Key = ReadAnyKey ();
        } while ((Key < '1') && (Key > '0' + OptionsCount) && (Key != 0x1B));

        Print (L"\n");
      }

      if (Key != 0x1B) {
        Index = Key - '0';

        for (ContextIndex = 0; ContextIndex < ContextsCount; ContextIndex++) {
          if (Contexts[ContextIndex].Count >= Index) {
            Contexts[ContextIndex].Count = (ContextIndex == 0) ? 0 : (Contexts[ContextIndex - 1].Count);
            Contexts[ContextIndex].StopAt = Index;
            Contexts[ContextIndex].IfrOneOf = NULL;

            DoForEachOpCode (Contexts[ContextIndex].FirstIfrHeader, EFI_IFR_ONE_OF_OP, &Stop, &Contexts[ContextIndex], HandleOneOf);

            if (Contexts[ContextIndex].IfrOneOf != NULL) {
              HandleOneVariable (&Contexts[ContextIndex]);
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
  return Status;
}

EFI_STATUS SearchForString (
  IN EFI_STRING SearchString
  )
{
  EFI_HII_HANDLE                *HiiHandles;
  EFI_HII_PACKAGE_LIST_HEADER   **ListHeaders;
  EFI_STATUS                    Status;
  UINT32                        ListHeaderCount;

  Status = EFI_SUCCESS;

    if (IS_INTERACTIVE()) {
      SearchString = ModifySearchString(SearchString);
    }

    HiiHandles = HiiGetHiiHandles (NULL);

    if (HiiHandles == NULL) {
      Print (L"Could not retrieve HiiHandles.\n");
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      for (ListHeaderCount = 0; HiiHandles[ListHeaderCount] != NULL; ListHeaderCount++) {
        continue;
      }

      //
      // Keep list alive 'til program finishes.
      // So that all lists can be searched, the results be displayed together.
      // And from all those one Option will be selected to be changed
      //
      ListHeaders = AllocatePool (sizeof(*ListHeaders) * ListHeaderCount);

      if (ListHeaders != NULL) {
        Status = WalkListHeaders(HiiHandles, ListHeaders, ListHeaderCount, SearchString);
        FreePool (ListHeaders);
      } else {
        Print (L"Could not allocate memory.\n");
        Status = EFI_OUT_OF_RESOURCES;
      }
    }  ///< Handling HiiHandles
  return Status;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE    ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_STRING SearchString;

  Status = InterpretArguments();

  if (!EFI_ERROR(Status)) {
    Status = VerifyMSRE2 (ImageHandle, SystemTable);
  }

  if (!EFI_ERROR(Status)) {
    Print(L"\nBIOS Options:\n");

    SearchString = AsciiStrCopyToUnicode ("cfg", 0);

    if (SearchString != NULL) {
      Status = SearchForString (SearchString);
      FreePool (SearchString);
    }
  } else {
    Print (L"Could not allocate memory. Function not available.\n");
  }

  Print (L"Press any key. \n");
  ReadAnyKey();
  return Status;
}

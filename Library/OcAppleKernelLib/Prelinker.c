/** @file
  Library handling KEXT prelinking.
  Currently limited to Intel 64 architectures.
  
Copyright (c) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcAppleKernelLib.h>

#include "PrelinkedInternal.h"

/*STATIC
BOOLEAN
InternalPrelinkKextsWorker (
  IN     OC_MACHO_CONTEXT               *KernelContext,
  IN OUT OC_KEXT_REQUEST                *Request,
  IN     LIST_ENTRY                     *Dependencies,
  IN     CONST MACH_SEGMENT_COMMAND_64  *PrelinkTextSegment,
  IN     CONST CHAR8                    *PrelinkedPlist
  )
{
  UINTN                         Index;
  LIST_ENTRY                    *DependencyEntry;
  OC_DEPENDENCY_INFO_ENTRY      *DependencyInfo;
  OC_DEPENDENCY_INFO            *Info;
  OC_DEPENDENCY_DATA            DependencyData;
  UINT64                        LoadAddress;
  BOOLEAN                       Result;
  BOOLEAN                       OneLinked;
  BOOLEAN                       OneUnlinked;
  MACH_SEGMENT_COMMAND_64       *LinkEdit;
  UINTN                         ScratchSize;
  VOID                          *ScratchMemory;
  OC_DEPENDENCY_DATA            *CurrentData;
  OC_MACHO_CONTEXT              MachoContext;
  CONST MACH_HEADER_64          *MachHeader;
  CONST MACH_NLIST_64           *SymbolTable;
  UINT32                        NumSymbols;
  OC_SYMBOL_TABLE_64            *OcSymbolTable;
  OC_VTABLE_ARRAY               *Vtables;
  OC_VTABLE_EXPORT_ARRAY        *VtableExport;

  UINT32                        VtableOffset;
  CONST UINT64                  *VtableData;
  UINTN                         VtablesSize;

  ASSERT (KernelContext != NULL);
  ASSERT (NumRequests > 0);
  ASSERT (Requests != NULL);
  ASSERT (Dependencies != NULL);
  ASSERT (PrelinkTextSegment != NULL);
  ASSERT (PrelinkedPlist != NULL);

  ScratchSize = 0;

  if (Request->Private.Info == NULL) {
    return FALSE;
  }

  //
  // Retrieve the __LINKEDIT segment.
  //
  LinkEdit = MachoGetSegmentByName64 (
             Request->Private.MachoContext,
             "__LINKEDIT"
             );
  if (LinkEdit == NULL) {
    InternalInvalidateKextRequest (
      Dependencies,
      Request
      );
    return FALSE;
  }

    Request->Private.LinkEdit = LinkEdit;

    if (ScratchSize < LinkEdit->FileSize) {
      ScratchSize = LinkEdit->FileSize;
    }
  }

  if (ScratchSize == 0) {
    return FALSE;
  }

  ScratchMemory = AllocatePool (ScratchSize);
  if (ScratchMemory == NULL) {
    return FALSE;
  }

  VtableExport = (OC_VTABLE_EXPORT_ARRAY *)ScratchMemory;

  for (
    DependencyEntry = GetFirstNode (Dependencies);
    !IsNull (DependencyEntry, Dependencies);
    DependencyEntry = GetNextNode (Dependencies, DependencyEntry),
    InternalDestructDependencyArrays (&DependencyData)
    ) {
    DependencyInfo = OC_DEP_INFO_FROM_LINK (DependencyEntry);
    CurrentData    = &DependencyInfo->Data;
    //
    // Try to construct the dependency tree first.
    //
    Info   = &DependencyInfo->Info;
    Result = InternalConstructDependencyArrays (
               Info->NumDependencies,
               Info->Dependencies,
               &DependencyData
               );
    if (!Result) {
      //
      // Prelinked dependencies can only depend on other already prelinked
      // KEXTs.
      //
      DependencyEntry = InternalRemoveDependency (
                          Dependencies,
                          NumRequests,
                          Requests,
                          DependencyInfo
                          );
      continue;
    }
    //
    // Create a Mach-O Context for this KEXT.
    //
    Result = MachoInitializeContext (
               &MachoContext,
               Info->MachHeader,
               Info->MachoSize
               );
    if (!Result) {
      DependencyEntry = InternalRemoveDependency (
                          Dependencies,
                          NumRequests,
                          Requests,
                          DependencyInfo
                          );
      continue;
    }

    NumSymbols = MachoGetSymbolTable (
                   &MachoContext,
                   &SymbolTable,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL
                   );
    if (NumSymbols == 0) {
      DependencyEntry = InternalRemoveDependency (
                          Dependencies,
                          DependencyInfo
                          );
      continue;
    }

    OcSymbolTable = AllocatePool (
                      sizeof (*OcSymbolTable)
                        + (NumSymbols * sizeof (*OcSymbolTable->Symbols))
                      );
    if (OcSymbolTable == NULL) {
      FreePool (ScratchMemory);
      return FALSE;
    }

    InternalFillSymbolTable64 (
      &MachoContext,
      NumSymbols,
      SymbolTable,
      OcSymbolTable
      );

    Result = InternalPrepareCreateVtablesPrelinked64 (
               &MachoContext,
               VtableExport,
               ScratchSize
               );
    if (!Result) {
      DependencyEntry = InternalRemoveDependency (
                          Dependencies,
                          NumRequests,
                          Requests,
                          DependencyInfo
                          );
      FreePool (OcSymbolTable);
      continue;
    }

    MachHeader = MachoGetMachHeader64 (&MachoContext);
    ASSERT (MachHeader != NULL);

    Result      = TRUE;
    VtablesSize = 0;

    for (Index = 0; Index < VtableExport->NumSymbols; ++Index) {
      Result = MachoSymbolGetFileOffset64 (
                 &MachoContext,
                 VtableExport->Symbols[Index],
                 &VtableOffset
                 );
      if (!Result) {
        break;
      }

      VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
      if (!OC_ALIGNED (VtableData)) {
        Result = FALSE;
        break;
      }

      VtablesSize += InternalGetVtableSize64 (VtableData);
    }

    if (!Result) {
      DependencyEntry = InternalRemoveDependency (
                          Dependencies,
                          NumRequests,
                          Requests,
                          DependencyInfo
                          );
      FreePool (OcSymbolTable);
      continue;
    }

    Vtables = AllocatePool (sizeof (*Vtables) + VtablesSize);
    if (Vtables == NULL) {
      FreePool (OcSymbolTable);
      FreePool (ScratchMemory);
      return FALSE;
    }

    Result = InternalCreateVtablesPrelinked64 (
               &MachoContext,
               DependencyData.SymbolTable,
               VtableExport,
               GET_FIRST_OC_VTABLE (Vtables)
               );
    if (!Result) {
      DependencyEntry = InternalRemoveDependency (
                          Dependencies,
                          NumRequests,
                          Requests,
                          DependencyInfo
                          );
      FreePool (Vtables);
      FreePool (OcSymbolTable);
      continue;
    }

    CurrentData->SymbolTable = OcSymbolTable;
    CurrentData->Vtables     = Vtables;
  }

  do {
    OneLinked   = FALSE;
    OneUnlinked = FALSE;

    for (Index = 0; Index < NumRequests; ++Index) {
      Request        = &Requests[Index];
      DependencyInfo = Request->Private.Info;
      if ((DependencyInfo == NULL) || DependencyInfo->Prelinked) {
        continue;
      }

      Info   = &DependencyInfo->Info;
      Result = InternalConstructDependencyArrays (
                 Info->NumDependencies,
                 Info->Dependencies,
                 &DependencyData
                 );
      if (!Result) {
        //
        // The KEXT depends on another KEXT staged for prelinking which has not
        // been processed yet.  Postpone to another iteration.
        //
        OneUnlinked = TRUE;
        continue;
      }
      //
      // Retrieve a loading address for the KEXT.
      //
      LoadAddress = InternalGetNewPrelinkedKextLoadAddress (
                      KernelContext,
                      PrelinkTextSegment,
                      PrelinkedPlist
                      );
      if (LoadAddress == 0) {
        FreePool (ScratchMemory);
        return FALSE;
      }
      //
      // Prelink the KEXT.
      //
      Result = InternalPrelinkKext64 (
                 &Request->Private.MachoContext,
                 Request->Private.LinkEdit,
                 LoadAddress,
                 &DependencyData,
                 Request->Private.IsDependedOn,
                 &DependencyInfo->Data,
                 ScratchMemory
                 );
      if (Result) {
        DependencyInfo->Prelinked = TRUE;
        Request->Output.Linked    = TRUE;
        OneLinked                 = TRUE;
      } else {
        InternalInvalidateKextRequest (
          Dependencies,
          NumRequests,
          Requests,
          Request
          );
      }
    }
    //
    // Continue while the loop makes progress and there are still KEXTs to
    // link.
    //
  } while (OneLinked && OneUnlinked);

  FreePool (ScratchMemory);

  return TRUE;
}*/

EFI_STATUS
PrelinkedLinkExecutable (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN OUT OC_MACHO_CONTEXT   *Executable,
  IN     XML_NODE           *PlistRoot,
  IN     UINT64             LoadAddress
  )
{
  EFI_STATUS                Status;
  PRELINKED_KEXT            *Kext;

  Kext = InternalNewPrelinkedKext (Executable, PlistRoot);
  if (Kext == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InternalScanPrelinkedKext (Kext, Context);
  if (EFI_ERROR (Status)) {
    FreePool (Kext);
    return Status;
  }

  InternalFreePrelinkedKext (Kext);

  return EFI_UNSUPPORTED;

  /*if (Request.Private.Info != NULL) {
    KextAddress  = (UINTN) MachoGetMachHeader64 (Request.Input.MachoContext);
    KextAddress += Context->PrelinkedTextSegment->FileOffset;
    if (KextAddress == (UINTN)KextAddress) {
      Result = InternalResolveDependencies (
                 &Dependencies,
                 PrelinkedPlist,
                 Request.Private.Info,
                 PrelinkTextSegment->VirtualAddress,
                 (UINTN)KextAddress
                 );
    }

    if (!Result) {
      //
      // Only pass the KEXT Requests that already had their dependencies
      // resolved.
      //
      InternalRemoveDependency (
        &Dependencies,
        Index,
        Requests,
        Request.Private.Info
        );
    }
  }
  //
  // Link the KEXTs.
  //
  Result = InternalPrelinkKextsWorker (
             KernelContext,
             &Request,
             &Dependencies,
             PrelinkTextSegment,
             PrelinkedPlist
             );*/

  //
  // Free all resources.
  //

}

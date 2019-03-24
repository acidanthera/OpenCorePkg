/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/AppleKmodInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcXmlLib.h>
#include <Library/PrintLib.h>

#include "PrelinkedInternal.h"

/**
  Creates new uncached PRELINKED_KEXT from pool.

  @param[in] Prelinked  Initialises PRELINKED_KEXT from prelinked buffer.
  @param[in] KextPlist  Plist root node with Kext Information.
  @param[in] Identifier Abort on CFBundleIdentifier mismatch.

  @return allocated PRELINKED_KEXT or NULL.
**/
STATIC
PRELINKED_KEXT *
InternalCreatePrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Prelinked OPTIONAL,
  IN XML_NODE               *KextPlist,
  IN CONST CHAR8            *Identifier OPTIONAL
  )
{
  PRELINKED_KEXT  *NewKext;
  UINT32          FieldIndex;
  UINT32          FieldCount;
  CONST CHAR8     *KextPlistKey;
  XML_NODE        *KextPlistValue;
  CONST CHAR8     *KextIdentifier;
  XML_NODE        *BundleLibraries;
  CONST CHAR8     *CompatibleVersion;
  UINT64          VirtualBase;
  UINT64          VirtualKmod;
  UINT64          SourceBase;
  UINT64          SourceSize;
  UINT64          SourceEnd;
  BOOLEAN         Found;

  KextIdentifier    = NULL;
  BundleLibraries   = NULL;
  CompatibleVersion = NULL;
  VirtualBase       = 0;
  VirtualKmod       = 0;
  SourceBase        = 0;
  SourceSize        = 0;

  Found       = Identifier == NULL;

  FieldCount = PlistDictChildren (KextPlist);
  for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
    KextPlistKey = PlistKeyValue (PlistDictChild (KextPlist, FieldIndex, &KextPlistValue));
    if (KextPlistKey == NULL) {
      continue;
    }

    if (KextIdentifier == NULL && AsciiStrCmp (KextPlistKey, INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
      KextIdentifier = XmlNodeContent (KextPlistValue);
      if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_STRING) == NULL || KextIdentifier == NULL) {
        break;
      }
      if (!Found && AsciiStrCmp (KextIdentifier, Identifier) == 0) {
        Found = TRUE;
      }
    } else if (BundleLibraries == NULL && AsciiStrCmp (KextPlistKey, INFO_BUNDLE_LIBRARIES_KEY) == 0) {
      if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_DICT) == NULL) {
        break;
      }
      BundleLibraries = KextPlistValue;
    } else if (CompatibleVersion == NULL && AsciiStrCmp (KextPlistKey, INFO_BUNDLE_COMPATIBLE_VERSION_KEY) == 0) {
      if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_STRING) == NULL) {
        break;
      }
      CompatibleVersion = XmlNodeContent (KextPlistValue);
      if (CompatibleVersion == NULL) {
        break;
      }
    } else if (Prelinked != NULL && VirtualBase == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &VirtualBase, sizeof (VirtualBase), TRUE)) {
        break;
      }
    } else if (Prelinked != NULL && VirtualKmod == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_KMOD_INFO_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &VirtualKmod, sizeof (VirtualKmod), TRUE)) {
        break;
      }
    } else if (Prelinked != NULL && SourceBase == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_SOURCE_ADDR_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &SourceBase, sizeof (SourceBase), TRUE)) {
        break;
      }
    } else if (Prelinked != NULL && SourceSize == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_SIZE_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &SourceSize, sizeof (SourceSize), TRUE)) {
        break;
      }
    }

    if (KextIdentifier != NULL && BundleLibraries != NULL && CompatibleVersion != NULL
      && (Prelinked == NULL || (Prelinked != NULL && VirtualBase != 0 && VirtualKmod != 0 && SourceBase != 0 && SourceSize != 0))) {
      break;
    }
  }

  //
  // BundleLibraries, CompatibleVersion, and KmodInfo are optional and thus not checked.
  //
  if (!Found || KextIdentifier == NULL || SourceBase < VirtualBase
    || (Prelinked != NULL && (VirtualBase == 0 || SourceBase == 0 || SourceSize == 0 || SourceSize > MAX_UINT32))) {
    return NULL;
  }

  if (Prelinked != NULL) {
    SourceBase -= Prelinked->PrelinkedTextSegment->VirtualAddress;
    if (OcOverflowAddU64 (SourceBase, Prelinked->PrelinkedTextSegment->FileOffset, &SourceBase) ||
      OcOverflowAddU64 (SourceBase, SourceSize, &SourceEnd) ||
      SourceEnd > Prelinked->PrelinkedSize) {
      return NULL;
    }
  }

  //
  // Important to ZeroPool for dependency cleanup.
  //
  NewKext = AllocateZeroPool (sizeof (*NewKext));
  if (NewKext == NULL) {
    return NULL;
  }

  if (Prelinked != NULL
    && !MachoInitializeContext (&NewKext->Context.MachContext, &Prelinked->Prelinked[SourceBase], (UINT32)SourceSize)) {
    FreePool (NewKext);
    return NULL;
  }

  NewKext->Signature            = PRELINKED_KEXT_SIGNATURE;
  NewKext->Identifier           = KextIdentifier;
  NewKext->BundleLibraries      = BundleLibraries;
  NewKext->CompatibleVersion    = CompatibleVersion;
  NewKext->Context.VirtualBase  = VirtualBase;
  NewKext->Context.VirtualKmod  = VirtualKmod;

  return NewKext;
}

STATIC
EFI_STATUS
InternalScanCurrentPrelinkedKext (
  IN OUT PRELINKED_KEXT  *Kext
  )
{
  if (Kext->LinkEditSegment == NULL) {
    Kext->LinkEditSegment = MachoGetSegmentByName64 (
      &Kext->Context.MachContext,
      "__LINKEDIT"
      );

    if (Kext->LinkEditSegment == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  if (Kext->SymbolTable == NULL) {
    Kext->NumberOfSymbols = MachoGetSymbolTable (
                   &Kext->Context.MachContext,
                   &Kext->SymbolTable,
                   &Kext->StringTable,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL
                   );
    if (Kext->NumberOfSymbols == 0) {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalScanBuildLinkedSymbolTable (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  CONST MACH_HEADER_64  *MachHeader;
  BOOLEAN               ResolveSymbols;
  PRELINKED_KEXT_SYMBOL *SymbolTable;
  PRELINKED_KEXT_SYMBOL *WalkerBottom;
  PRELINKED_KEXT_SYMBOL *WalkerTop;
  UINT32                NumCxxSymbols;
  UINT32                NumDiscardedSyms;
  UINT32                Index;
  CONST MACH_NLIST_64   *Symbol;
  MACH_NLIST_64         SymbolScratch;
  CONST PRELINKED_KEXT_SYMBOL *ResolvedSymbol;
  CONST CHAR8           *Name;
  BOOLEAN               Result;

  if (Kext->LinkedSymbolTable != NULL) {
    return EFI_SUCCESS;
  }

  SymbolTable = AllocatePool (Kext->NumberOfSymbols * sizeof (*SymbolTable));
  if (SymbolTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MachHeader = MachoGetMachHeader64 (&Kext->Context.MachContext);
  ASSERT (MachHeader != NULL);
  //
  // KPIs declare undefined and indirect symbols even in prelinkedkernel.
  //
  ResolveSymbols = ((MachHeader->Flags & MACH_HEADER_FLAG_NO_UNDEFINED_REFERENCES) == 0);
  NumDiscardedSyms = 0;

  WalkerBottom = &SymbolTable[0];
  WalkerTop    = &SymbolTable[Kext->NumberOfSymbols - 1];

  NumCxxSymbols = 0;

  for (Index = 0; Index < Kext->NumberOfSymbols; ++Index) {
    Symbol = &Kext->SymbolTable[Index];
    Name   = MachoGetSymbolName64 (&Kext->Context.MachContext, Symbol);
    Result = MachoSymbolNameIsCxx (Name);

    if (ResolveSymbols) {
      //
      // Undefined symbols will be resolved via the KPI's dependencies and
      // hence do not need to be included (again).
      //
      if ((Symbol->Type & MACH_N_TYPE_TYPE) == MACH_N_TYPE_UNDF) {
        ++NumDiscardedSyms;
        continue;
      }
      //
      // Resolve indirect symbols via the KPI's dependencies (kernel).
      //
      if ((Symbol->Type & MACH_N_TYPE_TYPE) == MACH_N_TYPE_INDR) {
        Name = MachoGetIndirectSymbolName64 (&Kext->Context.MachContext, Symbol);
        if (Name == NULL) {
          return EFI_LOAD_ERROR;
        }

        CopyMem (&SymbolScratch, Symbol, sizeof (SymbolScratch));
        ResolvedSymbol = InternalOcGetSymbolName (
                           Context,
                           Kext,
                           (UINTN)Name,
                           OcGetSymbolFirstLevel
                           );
        if (ResolvedSymbol == NULL) {
          return EFI_NOT_FOUND;
        }
        SymbolScratch.Value = ResolvedSymbol->Value;
        Symbol = &SymbolScratch;
      }
    }

    if (!Result) {
      WalkerBottom->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerBottom->Value       = Symbol->Value;
      ++WalkerBottom;
    } else {
      WalkerTop->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerTop->Value       = Symbol->Value;
      --WalkerTop;

      ++NumCxxSymbols;
    }
  }
  //
  // Move the C++ symbols to the actual end of the non-C++ symbols as undefined
  // symbols got discarded.
  //
  if (NumDiscardedSyms > 0) {
    CopyMem (
      &SymbolTable[Kext->NumberOfSymbols - NumCxxSymbols - NumDiscardedSyms],
      &SymbolTable[Kext->NumberOfSymbols - NumCxxSymbols],
      (NumCxxSymbols * sizeof (*SymbolTable))
      );
    Kext->NumberOfSymbols -= NumDiscardedSyms;
  }

  Kext->LinkedSymbolTable  = SymbolTable;
  Kext->NumberOfCxxSymbols = NumCxxSymbols;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalScanBuildLinkedVtables (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  OC_VTABLE_EXPORT_ARRAY *VtableExport;
  BOOLEAN                Result;
  UINT32                 NumVtables;
  UINT32                 NumVtablesTemp;
  UINT32                 Index;
  UINT32                 VtableOffset;
  UINT32                 MaxSize;
  CONST UINT64           *VtableData;
  CONST MACH_HEADER_64   *MachHeader;
  UINT32                 MachSize;
  PRELINKED_VTABLE       *Vtables;

  VtableExport = Context->LinkBuffer;

  Result = InternalPrepareCreateVtablesPrelinked64 (
             &Kext->Context.MachContext,
             VtableExport,
             Context->LinkBufferSize
             );
  if (!Result) {
    return EFI_UNSUPPORTED;
  }

  MachHeader = MachoGetMachHeader64 (&Kext->Context.MachContext);
  ASSERT (MachHeader != NULL);

  MachSize = MachoGetFileSize (&Kext->Context.MachContext);
  ASSERT (MachSize != 0);

  NumVtables = 0;

  for (Index = 0; Index < VtableExport->NumSymbols; ++Index) {
    Result = MachoSymbolGetFileOffset64 (
                &Kext->Context.MachContext,
                VtableExport->Symbols[Index],
                &VtableOffset,
                &MaxSize
                );
    if (!Result || (MaxSize < (VTABLE_HEADER_SIZE_64 + VTABLE_ENTRY_SIZE_64))) {
      return Result;
    }

    VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
    if (!OC_ALIGNED (VtableData)) {
      return FALSE;
    }

    Result = InternalGetVtableEntries64 (
               VtableData,
               MaxSize,
               &NumVtablesTemp
               );
    if (!Result) {
      return FALSE;
    }

    NumVtables += NumVtablesTemp;
  }

  Vtables = AllocatePool (
              (VtableExport->NumSymbols * sizeof (*Vtables))
                + (NumVtables * sizeof (*Vtables->Entries))
              );
  if (Vtables == NULL) {
    return FALSE;
  }

  Result = InternalCreateVtablesPrelinked64 (Context, Kext, VtableExport, Vtables);
  if (!Result) {
    return EFI_UNSUPPORTED;
  }

  Kext->LinkedVtables   = Vtables;
  Kext->NumberOfVtables = VtableExport->NumSymbols;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalInsertPrelinkedKextDependency (
  IN OUT PRELINKED_KEXT     *Kext,
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     UINT32             DependencyIndex,
  IN OUT PRELINKED_KEXT     *DependencyKext
  )
{
  EFI_STATUS  Status;

  if (DependencyIndex >= ARRAY_SIZE (Kext->Dependencies)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InternalScanPrelinkedKext (DependencyKext, Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InternalScanBuildLinkedSymbolTable (DependencyKext, Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Context->LinkBuffer == NULL) {
    //
    // Allocate the LinkBuffer from the size cached during the initial
    // function recursion.
    //
    Context->LinkBuffer = AllocatePool (Context->LinkBufferSize);
    if (Context->LinkBuffer == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }
  } else if (Context->LinkBufferSize < DependencyKext->LinkEditSegment->FileSize) {
    FreePool (Context->LinkBuffer);

    Context->LinkBufferSize = (UINT32)DependencyKext->LinkEditSegment->FileSize;
    Context->LinkBuffer     = AllocatePool (Context->LinkBufferSize);
    if (Context->LinkBuffer == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }
  }

  Status = InternalScanBuildLinkedVtables (DependencyKext, Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Kext->Dependencies[DependencyIndex] = DependencyKext;

  return RETURN_SUCCESS;
}


PRELINKED_KEXT *
InternalNewPrelinkedKext (
  IN OC_MACHO_CONTEXT       *Context,
  IN XML_NODE               *KextPlist
  )
{
  PRELINKED_KEXT  *NewKext;

  NewKext = InternalCreatePrelinkedKext (NULL, KextPlist, NULL);
  if (NewKext == NULL) {
    return NULL;
  }

  CopyMem (&NewKext->Context.MachContext, Context, sizeof (NewKext->Context.MachContext));
  return NewKext;
}

VOID
InternalFreePrelinkedKext (
  IN PRELINKED_KEXT  *Kext
  )
{
  if (Kext->LinkedSymbolTable != NULL) {
    FreePool (Kext->LinkedSymbolTable);
    Kext->LinkedSymbolTable = NULL;
  }

  FreePool (Kext);
}

PRELINKED_KEXT *
InternalCachedPrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Identifier
  )
{
  PRELINKED_KEXT  *NewKext;
  LIST_ENTRY      *Kext;
  UINT32          Index;
  UINT32          KextCount;
  XML_NODE        *KextPlist;

  //
  // Find cached entry if any.
  //
  Kext = GetFirstNode (&Prelinked->PrelinkedKexts);
  while (!IsNull (&Prelinked->PrelinkedKexts, Kext)) {
    if (AsciiStrCmp (Identifier, GET_PRELINKED_KEXT_FROM_LINK (Kext)->Identifier) == 0) {
      return GET_PRELINKED_KEXT_FROM_LINK (Kext);
    }

    Kext = GetNextNode (&Prelinked->PrelinkedKexts, Kext);
  }

  //
  // Try with real entry.
  //
  NewKext   = NULL;
  KextCount = XmlNodeChildren (Prelinked->KextList);
  for (Index = 0; Index < KextCount; ++Index) {
    KextPlist = PlistNodeCast (XmlNodeChild (Prelinked->KextList, Index), PLIST_NODE_TYPE_DICT);

    if (KextPlist == NULL) {
      continue;
    }

    NewKext = InternalCreatePrelinkedKext (Prelinked, KextPlist, Identifier);
    if (NewKext != NULL) {
      break;
    }
  }

  if (NewKext == NULL) {
    return NULL;
  }

  InsertTailList (&Prelinked->PrelinkedKexts, &NewKext->Link);

  return NewKext;
}

PRELINKED_KEXT *
InternalCachedPrelinkedKernel (
  IN OUT PRELINKED_CONTEXT  *Prelinked
  )
{
  LIST_ENTRY               *Kext;
  PRELINKED_KEXT           *NewKext;
  MACH_SEGMENT_COMMAND_64  *Segment;

  //
  // First entry is prelinked kernel.
  //
  Kext = GetFirstNode (&Prelinked->PrelinkedKexts);
  if (!IsNull (&Prelinked->PrelinkedKexts, Kext)) {
    return GET_PRELINKED_KEXT_FROM_LINK (Kext);
  }

  NewKext = AllocateZeroPool (sizeof (*NewKext));
  if (NewKext == NULL) {
    return NULL;
  }

  if (!MachoInitializeContext (&NewKext->Context.MachContext, &Prelinked->Prelinked[0], Prelinked->PrelinkedSize)) {
    FreePool (NewKext);
    return NULL;
  }

  Segment = MachoGetSegmentByName64 (
    &NewKext->Context.MachContext,
    "__TEXT"
    );
  if (Segment == NULL || Segment->VirtualAddress < Segment->FileOffset) {
    FreePool (NewKext);
    return NULL;
  }

  NewKext->Signature            = PRELINKED_KEXT_SIGNATURE;
  NewKext->Identifier           = PRELINK_KERNEL_IDENTIFIER;
  NewKext->BundleLibraries      = NULL;
  NewKext->CompatibleVersion    = "0";
  NewKext->Context.VirtualBase  = Segment->VirtualAddress - Segment->FileOffset;
  NewKext->Context.VirtualKmod  = 0;

  InsertTailList (&Prelinked->PrelinkedKexts, &NewKext->Link);

  return NewKext;
}

EFI_STATUS
InternalScanPrelinkedKext (
  IN OUT PRELINKED_KEXT     *Kext,
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  EFI_STATUS      Status;
  UINT32          FieldCount;
  UINT32          FieldIndex;
  UINT32          DependencyIndex;
  CONST CHAR8     *DependencyId;
  PRELINKED_KEXT  *DependencyKext;

  Status = InternalScanCurrentPrelinkedKext (Kext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Find the biggest __LINKEDIT size down the first dependency tree walk to
  // possibly save a few re-allocations.
  //
  if ((Context->LinkBuffer == NULL)
   && (Context->LinkBufferSize < Kext->LinkEditSegment->FileSize)) {
    Context->LinkBufferSize = (UINT32)Kext->LinkEditSegment->FileSize;
  }

  //
  // Always add kernel dependency.
  //
  DependencyKext = InternalCachedPrelinkedKernel (Context);
  if (DependencyKext == NULL) {
    return EFI_NOT_FOUND;
  }

  if (DependencyKext != Kext) {
    Status = InternalInsertPrelinkedKextDependency (Kext, Context, 0, DependencyKext);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Kext->BundleLibraries != NULL) {
    DependencyIndex = 1;
    FieldCount = PlistDictChildren (Kext->BundleLibraries);

    for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
      DependencyId = PlistKeyValue (PlistDictChild (Kext->BundleLibraries, FieldIndex, NULL));
      if (DependencyId == NULL) {
        continue;
      }

      //
      // We still need to add KPI dependencies, as they may have indirect symbols,
      // which are not present in kernel (e.g. _IOLockLock).
      //
      DependencyKext = InternalCachedPrelinkedKext (Context, DependencyId);
      if (DependencyKext == NULL) {
        return EFI_NOT_FOUND;
      }

      Status = InternalInsertPrelinkedKextDependency (Kext, Context, DependencyIndex, DependencyKext);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ++DependencyIndex;
    }

    //
    // We do not need this anymore.
    // Additionally it may point to invalid memory on prelinked kexts.
    //
    Kext->BundleLibraries = NULL;
  }

  if (Context->LinkBuffer == NULL) {
    //
    // Allocate the LinkBuffer in case there are no dependencies.
    //
    Context->LinkBuffer = AllocatePool (Context->LinkBufferSize);
    if (Context->LinkBuffer == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

VOID
InternalUnlockContextKexts (
  IN PRELINKED_CONTEXT                *Context
  )
{
  LIST_ENTRY  *Kext;

  Kext = GetFirstNode (&Context->PrelinkedKexts);
  while (!IsNull (&Context->PrelinkedKexts, Kext)) {
    GET_PRELINKED_KEXT_FROM_LINK (Kext)->Processed = FALSE;
    Kext = GetNextNode (&Context->PrelinkedKexts, Kext);
  }
}


PRELINKED_KEXT *
InternalLinkPrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN OUT OC_MACHO_CONTEXT   *Executable,
  IN     XML_NODE           *PlistRoot,
  IN     UINT64             LoadAddress,
  IN     UINT64             KmodAddress
  )
{
  EFI_STATUS      Status;
  PRELINKED_KEXT  *Kext;

  Kext = InternalNewPrelinkedKext (Executable, PlistRoot);
  if (Kext == NULL) {
    return NULL;
  }

  Status = InternalScanPrelinkedKext (Kext, Context);
  if (EFI_ERROR (Status)) {
    InternalFreePrelinkedKext (Kext);
    return NULL;
  }

  //
  // Detach Identifier from temporary memory location.
  //
  Kext->Identifier = AllocateCopyPool (AsciiStrSize (Kext->Identifier), Kext->Identifier);
  if (Kext->Identifier == NULL) {
    InternalFreePrelinkedKext (Kext);
    return NULL;
  }

  Status = PrelinkedDependencyInsert (Context, (CHAR8 *) Kext->Identifier);
  if (EFI_ERROR (Status)) {
    FreePool ((CHAR8 *) Kext->Identifier);
    InternalFreePrelinkedKext (Kext);
    return NULL;
  }
  //
  // Also detach bundle compatible version if any.
  //
  if (Kext->CompatibleVersion != NULL) {
    Kext->CompatibleVersion = AllocateCopyPool (AsciiStrSize (Kext->CompatibleVersion), Kext->CompatibleVersion);
    if (Kext->CompatibleVersion == NULL) {
      InternalFreePrelinkedKext (Kext);
      return NULL;
    }

    Status = PrelinkedDependencyInsert (Context, (CHAR8 *) Kext->CompatibleVersion);
    if (EFI_ERROR (Status)) {
      FreePool ((CHAR8 *) Kext->CompatibleVersion);
      InternalFreePrelinkedKext (Kext);
      return NULL;
    }
  }
  //
  // Set virtual addresses.
  //
  Kext->Context.VirtualBase = LoadAddress;
  Kext->Context.VirtualKmod = KmodAddress;

  Status = InternalPrelinkKext64 (Context, Kext, LoadAddress);

  if (EFI_ERROR (Status)) {
    InternalFreePrelinkedKext (Kext);
    return NULL;
  }

  Kext->SymbolTable     = NULL;
  Kext->StringTable     = NULL;
  Kext->NumberOfSymbols = 0;

  return Kext;
}

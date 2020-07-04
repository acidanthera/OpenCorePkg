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

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcXmlLib.h>

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
  PRELINKED_KEXT           *NewKext;
  UINT32                   FieldIndex;
  UINT32                   FieldCount;
  CONST CHAR8              *KextPlistKey;
  XML_NODE                 *KextPlistValue;
  CONST CHAR8              *KextIdentifier;
  XML_NODE                 *BundleLibraries;
  XML_NODE                 *BundleLibraries64;
  CONST CHAR8              *CompatibleVersion;
  UINT64                   VirtualBase;
  UINT64                   VirtualKmod;
  UINT64                   SourceBase;
  UINT64                   SourceSize;
  UINT64                   CalculatedSourceSize;
  UINT64                   SourceEnd;
  MACH_SEGMENT_COMMAND_64  *BaseSegment;
  UINT32                   ContainerOffset;
  BOOLEAN                  Found;

  KextIdentifier    = NULL;
  BundleLibraries   = NULL;
  BundleLibraries64 = NULL;
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
    } else if (BundleLibraries64 == NULL && AsciiStrCmp (KextPlistKey, INFO_BUNDLE_LIBRARIES_64_KEY) == 0) {
      if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_DICT) == NULL) {
        break;
      }
      BundleLibraries64 = BundleLibraries = KextPlistValue;
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

    if (KextIdentifier != NULL && BundleLibraries64 != NULL && CompatibleVersion != NULL
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

  if (Prelinked != NULL && Prelinked->IsKernelCollection) {
    CalculatedSourceSize = KcGetKextSize (Prelinked, SourceBase);
    if (CalculatedSourceSize < MAX_UINT32 && CalculatedSourceSize > SourceSize) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: Patching invalid size %Lx with %Lx for %a\n",
        SourceSize,
        CalculatedSourceSize,
        KextIdentifier
        ));
      SourceSize = CalculatedSourceSize;
    }
  }

  if (Prelinked != NULL) {
    if (Prelinked->IsKernelCollection) {
      BaseSegment = Prelinked->RegionSegment;
    } else {
      BaseSegment = Prelinked->PrelinkedTextSegment;
    }

    SourceBase -= BaseSegment->VirtualAddress;
    if (OcOverflowAddU64 (SourceBase, BaseSegment->FileOffset, &SourceBase) ||
      OcOverflowAddU64 (SourceBase, SourceSize, &SourceEnd) ||
      SourceEnd > Prelinked->PrelinkedSize) {
      return NULL;
    }

    ContainerOffset = 0;
    if (Prelinked->IsKernelCollection) {
      ContainerOffset = (UINT32) SourceBase;
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
    && !MachoInitializeContext (&NewKext->Context.MachContext, &Prelinked->Prelinked[SourceBase], (UINT32)SourceSize, ContainerOffset)) {
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
VOID
InternalScanCurrentPrelinkedKextLinkInfo (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  if (Kext->LinkEditSegment == NULL) {
    DEBUG ((DEBUG_VERBOSE, "OCAK: Requesting __LINKEDIT for %a\n", Kext->Identifier));
    if (AsciiStrCmp (Kext->Identifier, PRELINK_KERNEL_IDENTIFIER) == 0) {
      Kext->LinkEditSegment = Context->LinkEditSegment;
    } else {
      Kext->LinkEditSegment = MachoGetSegmentByName64 (
        &Kext->Context.MachContext,
        "__LINKEDIT"
        );
    }    
  }

  if (Kext->SymbolTable == NULL) {
    DEBUG ((DEBUG_VERBOSE, "OCAK: Requesting SymbolTable for %a\n", Kext->Identifier));
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
  }
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
    if ((Symbol->Type & MACH_N_TYPE_STAB) != 0) {
      ++NumDiscardedSyms;
      continue;
    }

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
          FreePool (SymbolTable);
          return EFI_LOAD_ERROR;
        }

        CopyMem (&SymbolScratch, Symbol, sizeof (SymbolScratch));
        ResolvedSymbol = InternalOcGetSymbolName (
                           Context,
                           Kext,
                           Name,
                           OcGetSymbolFirstLevel
                           );
        if (ResolvedSymbol == NULL) {
          FreePool (SymbolTable);
          return EFI_NOT_FOUND;
        }
        SymbolScratch.Value = ResolvedSymbol->Value;
        Symbol = &SymbolScratch;
      }
    }

    if (!Result) {
      WalkerBottom->Value  = Symbol->Value;
      WalkerBottom->Name   = Kext->StringTable + Symbol->UnifiedName.StringIndex;
      WalkerBottom->Length = (UINT32)AsciiStrLen (WalkerBottom->Name);
      ++WalkerBottom;
    } else {
      WalkerTop->Value  = Symbol->Value;
      WalkerTop->Name   = Kext->StringTable + Symbol->UnifiedName.StringIndex;
      WalkerTop->Length = (UINT32)AsciiStrLen (WalkerTop->Name);
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

  Kext->NumberOfCxxSymbols = NumCxxSymbols;
  Kext->LinkedSymbolTable  = SymbolTable;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalScanBuildLinkedVtables (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  OC_PRELINKED_VTABLE_LOOKUP_ENTRY *VtableLookups;
  UINT32                           MaxSize;
  BOOLEAN                          Result;
  UINT32                           NumVtables;
  UINT32                           NumEntries;
  UINT32                           NumEntriesTemp;
  UINT32                           Index;
  UINT32                           VtableMaxSize;
  CONST UINT64                     *VtableData;
  PRELINKED_VTABLE                 *LinkedVtables;

  VOID                             *Tmp;

  if (Kext->LinkedVtables != NULL) {
    return EFI_SUCCESS;
  }

  VtableLookups = Context->LinkBuffer;
  MaxSize       = Context->LinkBufferSize;

  Result = InternalPrepareCreateVtablesPrelinked64 (
             Kext,
             MaxSize,
             &NumVtables,
             VtableLookups
             );
  if (!Result) {
    return EFI_UNSUPPORTED;
  }

  NumEntries = 0;

  for (Index = 0; Index < NumVtables; ++Index) {
    //
    // NOTE: KXLD locates the section via MACH_NLIST_64.Section. However, as we
    //       need to abort anyway when the value is out of its bounds, we can
    //       just locate it by address in the first place.
    //
    Tmp = MachoGetFilePointerByAddress64 (
            &Kext->Context.MachContext,
            VtableLookups[Index].Vtable.Value,
            &VtableMaxSize
            );
    if (Tmp == NULL || !OC_TYPE_ALIGNED (UINT64, Tmp)) {
      return EFI_UNSUPPORTED;
    }
    VtableData = (UINT64 *)Tmp;

    Result = InternalGetVtableEntries64 (
               VtableData,
               VtableMaxSize,
               &NumEntriesTemp
               );
    if (!Result) {
      return EFI_UNSUPPORTED;
    }

    VtableLookups[Index].Vtable.Data = VtableData;

    NumEntries += NumEntriesTemp;
  }

  LinkedVtables = AllocatePool (
                    (NumVtables * sizeof (*LinkedVtables))
                      + (NumEntries * sizeof (*LinkedVtables->Entries))
                    );
  if (LinkedVtables == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InternalCreateVtablesPrelinked64 (
             Context,
             Kext,
             NumVtables,
             VtableLookups,
             LinkedVtables
             );

  Kext->NumberOfVtables = NumVtables;
  Kext->LinkedVtables   = LinkedVtables;

  return EFI_SUCCESS;
}

STATIC
UINT32
InternalGetLinkBufferSize (
  IN OUT PRELINKED_KEXT  *Kext
  )
{
  UINT32 Size;
  //
  // LinkBuffer must be able to hold all symbols and for KEXTs to be prelinked
  // also the __LINKEDIT segment (however not both simultaneously/separately).
  //
  Size = Kext->NumberOfSymbols * sizeof (MACH_NLIST_64);
  
  if (Kext->LinkEditSegment != NULL) {
    Size = MAX ((UINT32) Kext->LinkEditSegment->FileSize, Size);
  }

  return Size;
}

STATIC
EFI_STATUS
InternalUpdateLinkBuffer (
  IN OUT PRELINKED_KEXT     *Kext,
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  UINT32 BufferSize;

  ASSERT (Kext != NULL);
  ASSERT (Context != NULL);

  if (Context->LinkBuffer == NULL) {
    //
    // Context->LinkBufferSize was updated recursively during initial dependency
    // walk to save reallocations.
    //
    ASSERT (Context->LinkBufferSize >= InternalGetLinkBufferSize (Kext));

    Context->LinkBuffer = AllocatePool (Context->LinkBufferSize);
    if (Context->LinkBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    BufferSize = InternalGetLinkBufferSize (Kext);

    if (Context->LinkBufferSize < BufferSize) {
      FreePool (Context->LinkBuffer);

      Context->LinkBufferSize = BufferSize;
      Context->LinkBuffer     = AllocatePool (Context->LinkBufferSize);
      if (Context->LinkBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }
  }

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
    DEBUG ((DEBUG_INFO, "OCAK: Kext %a has more than %u or more dependencies!", Kext->Identifier, DependencyIndex));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InternalScanPrelinkedKext (DependencyKext, Context, TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Kext->Dependencies[DependencyIndex] = DependencyKext;

  return EFI_SUCCESS;
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

  if (Kext->LinkedVtables != NULL) {
    FreePool (Kext->LinkedVtables);
    Kext->LinkedVtables = NULL;
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

  ASSERT (Prelinked->Prelinked != NULL);
  ASSERT (Prelinked->PrelinkedSize > 0);

  CopyMem (
    &NewKext->Context.MachContext,
    &Prelinked->PrelinkedMachContext,
    sizeof (NewKext->Context.MachContext)
    );

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

PRELINKED_KEXT *
InternalGetQuirkDependencyKext (
  IN     CONST CHAR8        *DependencyId,
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  PRELINKED_KEXT *DependencyKext;

  ASSERT (DependencyId != NULL);
  ASSERT (Context != NULL);

  DependencyKext = NULL;
  //
  // Some kexts, notably VoodooPS2 forks, link against IOHIDSystem.kext, which is a plist-only
  // dummy, macOS does not add to the prelinkedkernel. This cannot succeed as /S/L/E directory
  // is not accessible (and can be encrypted). Normally kext's Info.plist is to be fixed, but
  // we also put a hack here to let some common kexts work.
  //
  if (AsciiStrCmp (DependencyId, "com.apple.iokit.IOHIDSystem") == 0) {
    DependencyKext = InternalCachedPrelinkedKext (Context, "com.apple.iokit.IOHIDFamily");
    DEBUG ((
      DEBUG_WARN,
      "Dependency %a fallback to %a %a. Please fix your kext!\n",
      DependencyId,
      "com.apple.iokit.IOHIDFamily",
      DependencyKext != NULL ? "succeeded" : "failed"
      ));
  }

  return DependencyKext;
}

EFI_STATUS
InternalScanPrelinkedKext (
  IN OUT PRELINKED_KEXT     *Kext,
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     BOOLEAN            Dependency
  )
{
  EFI_STATUS      Status;
  UINT32          FieldCount;
  UINT32          FieldIndex;
  UINT32          DependencyIndex;
  CONST CHAR8     *DependencyId;
  PRELINKED_KEXT  *DependencyKext;
  //
  // __LINKEDIT may validly not be present, as seen for 10.7.5's
  // com.apple.kpi.unsupported.
  //
  InternalScanCurrentPrelinkedKextLinkInfo (Kext, Context);
  //
  // Find the biggest LinkBuffer size down the first dependency tree walk to
  // possibly save a few re-allocations.
  //
  if (Context->LinkBuffer == NULL) {
    Context->LinkBufferSize = MAX (
                                InternalGetLinkBufferSize (Kext),
                                Context->LinkBufferSize
                                );
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
      // In 11.0 KPIs just like plist-only kexts are not present in memory and their
      // _PrelinkExecutableLoadAddr / _PrelinkExecutableSourceAddr values equal to MAX_INT64.
      // Skip them early to improve performance.
      //
      if (Context->IsKernelCollection
        && AsciiStrnCmp (DependencyId, "com.apple.kpi.", L_STR_LEN ("com.apple.kpi.")) == 0) {
        DEBUG ((DEBUG_VERBOSE, "OCAK: Ignoring KPI %a for kext %a in KC mode\n", DependencyId, Kext->Identifier));
        continue;
      }

      //
      // We still need to add KPI dependencies, as they may have indirect symbols,
      // which are not present in kernel (e.g. _IOLockLock).
      //
      DependencyKext = InternalCachedPrelinkedKext (Context, DependencyId);
      if (DependencyKext == NULL) {
        DEBUG ((DEBUG_INFO, "OCAK: Dependency %a was not found for kext %a\n", DependencyId, Kext->Identifier));

        DependencyKext = InternalGetQuirkDependencyKext (DependencyId, Context);
        if (DependencyKext == NULL) {
          //
          // Skip missing dependencies.  PLIST-only dependencies, such as used
          // in macOS Catalina, will not be found in prelinkedkernel and are
          // assumed to be safe to ignore.  Any actual problems will be found
          // during linking.
          //
          continue;
        }
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

  //
  // Extend or allocate LinkBuffer in case there are no dependencies (kernel).
  //
  Status = InternalUpdateLinkBuffer (Kext, Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Collect data to enable linking against this KEXT.
  //
  if (Dependency) {
    Status = InternalScanBuildLinkedSymbolTable (Kext, Context);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = InternalScanBuildLinkedVtables (Kext, Context);
    if (EFI_ERROR (Status)) {
      return Status;
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

  Status = InternalScanPrelinkedKext (Kext, Context, FALSE);
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

  Status = PrelinkedDependencyInsert (Context, (VOID *)Kext->Identifier);
  if (EFI_ERROR (Status)) {
    FreePool ((VOID *)Kext->Identifier);
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

    Status = PrelinkedDependencyInsert (Context, (VOID *)Kext->CompatibleVersion);
    if (EFI_ERROR (Status)) {
      FreePool ((VOID *)Kext->CompatibleVersion);
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
  //
  // TODO: VTable and entry names are not updated after relocating StringTable,
  // which means that to avoid linkage issues for plugins we must either continue
  // re-constructing VTables (as done below) or implement some optimisations.
  // For example, we could recalculate addresses after linking has finished.
  // We could also store the name's offset and access via a StringTable pointer,
  // yet it was prone to errors and was already removed once.
  //
  if (Kext->LinkedVtables != NULL) {
    FreePool (Kext->LinkedVtables);
    Kext->LinkedVtables   = NULL;
    Kext->NumberOfVtables = 0;
  }

  return Kext;
}

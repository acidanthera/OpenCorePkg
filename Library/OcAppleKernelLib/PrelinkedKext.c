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
  MACH_SEGMENT_COMMAND_ANY *BaseSegment;
  UINT64                   KxldState;
  UINT64                   KxldOffset;
  UINT32                   KxldStateSize;
  UINT32                   ContainerOffset;
  BOOLEAN                  Found;
  BOOLEAN                  HasExe;
  BOOLEAN                  IsKpi;

  KextIdentifier    = NULL;
  BundleLibraries   = NULL;
  BundleLibraries64 = NULL;
  CompatibleVersion = NULL;
  VirtualBase       = 0;
  VirtualKmod       = 0;
  SourceBase        = 0;
  SourceSize        = 0;
  KxldState         = 0;
  KxldStateSize     = 0;

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
    } else if (Prelinked != NULL && KxldState == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_LINK_STATE_ADDR_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &KxldState, sizeof (KxldState), TRUE)) {
        break;
      }
    } else if (Prelinked != NULL && KxldStateSize == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_LINK_STATE_SIZE_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &KxldStateSize, sizeof (KxldStateSize), TRUE)) {
        break;
      }
    }

    if (KextIdentifier != NULL
      && BundleLibraries64 != NULL
      && CompatibleVersion != NULL
      && (Prelinked == NULL
        || (Prelinked != NULL
          && VirtualBase != 0
          && VirtualKmod != 0
          && SourceBase != 0
          && SourceSize != 0
          && KxldState != 0
          && KxldStateSize != 0))) {
      break;
    }
  }

  //
  // BundleLibraries, CompatibleVersion, and KmodInfo are optional and thus not checked.
  //
  if (!Found || KextIdentifier == NULL || SourceBase < VirtualBase) {
    return NULL;
  }

  //
  // KPIs on 10.6.8 may not have executables, but for all other types they are required.
  //
  if (Prelinked != NULL) {
    HasExe = VirtualBase != 0 && SourceBase != 0 && SourceSize != 0 && SourceSize <= MAX_UINT32;
    IsKpi  = VirtualBase == 0 && SourceBase == 0 && SourceSize == 0
      && KxldState != 0 && KxldStateSize != 0 && !Prelinked->IsKernelCollection;
    if (!IsKpi && !HasExe) {
      return NULL;
    }
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

  if (Prelinked != NULL && HasExe) {
    if (Prelinked->IsKernelCollection) {
      BaseSegment = (MACH_SEGMENT_COMMAND_ANY *) Prelinked->RegionSegment;
    } else {
      BaseSegment = Prelinked->PrelinkedTextSegment;
    }

    SourceBase -= Prelinked->Is32Bit ? BaseSegment->Segment32.VirtualAddress : BaseSegment->Segment64.VirtualAddress;
    if (OcOverflowAddU64 (SourceBase, Prelinked->Is32Bit ? BaseSegment->Segment32.FileOffset : BaseSegment->Segment64.FileOffset, &SourceBase)
      || OcOverflowAddU64 (SourceBase, SourceSize, &SourceEnd)
      || SourceEnd > Prelinked->PrelinkedSize) {
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
    && HasExe
    && !MachoInitializeContext (&NewKext->Context.MachContext, &Prelinked->Prelinked[SourceBase], (UINT32)SourceSize, ContainerOffset, Prelinked->Is32Bit)) {
    FreePool (NewKext);
    return NULL;
  }

  NewKext->Signature                  = PRELINKED_KEXT_SIGNATURE;
  NewKext->Identifier                 = KextIdentifier;
  NewKext->BundleLibraries            = BundleLibraries;
  NewKext->CompatibleVersion          = CompatibleVersion;
  NewKext->Context.VirtualBase        = VirtualBase;
  NewKext->Context.VirtualKmod        = VirtualKmod;
  NewKext->Context.IsKernelCollection = Prelinked != NULL ? Prelinked->IsKernelCollection : FALSE;
  NewKext->Context.Is32Bit            = Prelinked != NULL ? Prelinked->Is32Bit : FALSE;

  //
  // Provide pointer to 10.6.8 KXLD state.
  //
  if (Prelinked != NULL
    && KxldState != 0
    && KxldStateSize != 0
    && Prelinked->PrelinkedStateKextsAddress != 0
    && Prelinked->PrelinkedStateKextsAddress <= KxldState
    && Prelinked->PrelinkedStateKextsSize >= KxldStateSize) {
    KxldOffset = KxldState - Prelinked->PrelinkedStateKextsAddress;
    if (KxldOffset <= Prelinked->PrelinkedStateKextsSize - KxldStateSize) {
      NewKext->Context.KxldState = (UINT8 *)Prelinked->PrelinkedStateKexts + KxldOffset;
      NewKext->Context.KxldStateSize = KxldStateSize;
    }
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "OCAK: %a got KXLD %p %u\n",
    NewKext->Identifier,
    NewKext->Context.KxldState,
    NewKext->Context.KxldStateSize
    ));

  return NewKext;
}

STATIC
VOID
InternalScanCurrentPrelinkedKextLinkInfo (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  //
  // Prefer KXLD state when available.
  //
  if (Kext->Context.KxldState != NULL) {
    return;
  }

  if (Kext->LinkEditSegment == NULL && Kext->NumberOfSymbols == 0) {
    if (AsciiStrCmp (Kext->Identifier, PRELINK_KERNEL_IDENTIFIER) == 0) {
      Kext->LinkEditSegment = Context->LinkEditSegment;
    } else {
      Kext->LinkEditSegment = MachoGetSegmentByName (
        &Kext->Context.MachContext,
        "__LINKEDIT"
        );
    }    
    DEBUG ((
      DEBUG_VERBOSE,
      "OCAK: Requesting __LINKEDIT for %a - %p at %p\n",
      Kext->Identifier,
      Kext->LinkEditSegment,
      (UINT8 *) MachoGetMachHeader (&Kext->Context.MachContext) - Context->Prelinked
      ));
  }

  if (Kext->SymbolTable == NULL && Kext->NumberOfSymbols == 0) {
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
    DEBUG ((
      DEBUG_VERBOSE,
      "OCAK: Requesting SymbolTable for %a - %u\n",
      Kext->Identifier,
      Kext->NumberOfSymbols
      ));
  }
}

STATIC
EFI_STATUS
InternalScanBuildLinkedSymbolTable (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  CONST MACH_HEADER_ANY *MachHeader;
  BOOLEAN               ResolveSymbols;
  PRELINKED_KEXT_SYMBOL *SymbolTable;
  PRELINKED_KEXT_SYMBOL *WalkerBottom;
  PRELINKED_KEXT_SYMBOL *WalkerTop;
  UINT32                NumCxxSymbols;
  UINT32                NumDiscardedSyms;
  UINT32                Index;
  CONST MACH_NLIST_ANY  *Symbol;
  UINT8                 SymbolType;
  MACH_NLIST_ANY        SymbolScratch;
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

  MachHeader = MachoGetMachHeader (&Kext->Context.MachContext);
  ASSERT (MachHeader != NULL);
  //
  // KPIs declare undefined and indirect symbols even in prelinkedkernel.
  //
  ResolveSymbols = (((Context->Is32Bit ? MachHeader->Header32.Flags : MachHeader->Header64.Flags) & MACH_HEADER_FLAG_NO_UNDEFINED_REFERENCES) == 0);
  NumDiscardedSyms = 0;

  WalkerBottom = &SymbolTable[0];
  WalkerTop    = &SymbolTable[Kext->NumberOfSymbols - 1];

  NumCxxSymbols = 0;

  for (Index = 0; Index < Kext->NumberOfSymbols; ++Index) {
    if (Context->Is32Bit) {
      Symbol = (MACH_NLIST_ANY *)&(&Kext->SymbolTable->Symbol32)[Index];
    } else {
      Symbol = (MACH_NLIST_ANY *)&(&Kext->SymbolTable->Symbol64)[Index];
    }
    SymbolType = Context->Is32Bit ? Symbol->Symbol32.Type : Symbol->Symbol64.Type;
    if ((SymbolType & MACH_N_TYPE_STAB) != 0) {
      ++NumDiscardedSyms;
      continue;
    }

    //
    // Undefined symbols will be resolved via the KPI's dependencies and
    // hence do not need to be included (again).
    //
    if ((SymbolType & MACH_N_TYPE_TYPE) == MACH_N_TYPE_UNDF) {
      ++NumDiscardedSyms;
      continue;
    }

    Name   = MachoGetSymbolName (&Kext->Context.MachContext, Symbol);
    Result = MachoSymbolNameIsCxx (Name);

    if (ResolveSymbols) {
      //
      // Resolve indirect symbols via the KPI's dependencies (kernel).
      //
      if ((SymbolType & MACH_N_TYPE_TYPE) == MACH_N_TYPE_INDR) {
        Name = MachoGetIndirectSymbolName (&Kext->Context.MachContext, Symbol);
        if (Name == NULL) {
          FreePool (SymbolTable);
          return EFI_LOAD_ERROR;
        }

        CopyMem (&SymbolScratch, Symbol, Context->Is32Bit ? sizeof (SymbolScratch.Symbol32) : sizeof (SymbolScratch.Symbol64));
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
        if (Context->Is32Bit) {
          SymbolScratch.Symbol32.Value = (UINT32) ResolvedSymbol->Value;
        } else {
          SymbolScratch.Symbol64.Value = ResolvedSymbol->Value;
        }
        Symbol = &SymbolScratch;
      }
    }

    if (!Result) {
      WalkerBottom->Value  = Context->Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value;
      WalkerBottom->Name   = Kext->StringTable + (Context->Is32Bit ?
        Symbol->Symbol32.UnifiedName.StringIndex : Symbol->Symbol64.UnifiedName.StringIndex);
      WalkerBottom->Length = (UINT32)AsciiStrLen (WalkerBottom->Name);
      ++WalkerBottom;
    } else {
      WalkerTop->Value  = Context->Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value;
      WalkerTop->Name   = Kext->StringTable + (Context->Is32Bit ?
        Symbol->Symbol32.UnifiedName.StringIndex : Symbol->Symbol64.UnifiedName.StringIndex);
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
  UINT32                           ResultingSize;
  CONST VOID                       *VtableData;
  PRELINKED_VTABLE                 *LinkedVtables;

  if (Kext->LinkedVtables != NULL) {
    return EFI_SUCCESS;
  }

  VtableLookups = Context->LinkBuffer;
  MaxSize       = Context->LinkBufferSize;

  Result = InternalPrepareCreateVtablesPrelinked (
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
    VtableData = MachoGetFilePointerByAddress (
            &Kext->Context.MachContext,
            VtableLookups[Index].Vtable.Value,
            &VtableMaxSize
            );
    if (VtableData == NULL || (Context->Is32Bit ? !OC_TYPE_ALIGNED (UINT32, VtableData) : !OC_TYPE_ALIGNED (UINT64, VtableData))) {
      return EFI_UNSUPPORTED;
    }

    Result = InternalGetVtableEntries (
               Context->Is32Bit,
               VtableData,
               VtableMaxSize,
               &NumEntriesTemp
               );
    if (!Result) {
      return EFI_UNSUPPORTED;
    }

    VtableLookups[Index].Vtable.Data = VtableData;

    if (OcOverflowAddU32 (NumEntries, NumEntriesTemp, &NumEntries)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if (OcOverflowMulU32 (NumVtables, sizeof (*LinkedVtables), &ResultingSize)
    || OcOverflowMulAddU32 (NumEntries, sizeof (*LinkedVtables->Entries), ResultingSize, &ResultingSize)) {
    return EFI_OUT_OF_RESOURCES;
  }

  LinkedVtables = AllocatePool (ResultingSize);
  if (LinkedVtables == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InternalCreateVtablesPrelinked (
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
  Size = Kext->NumberOfSymbols * (Kext->Context.Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64));
  
  if (Kext->LinkEditSegment != NULL) {
    Size = MAX ((UINT32) (Kext->Context.Is32Bit ?
      Kext->LinkEditSegment->Segment32.FileSize : Kext->LinkEditSegment->Segment64.FileSize), Size);
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
  LIST_ENTRY                *Kext;
  PRELINKED_KEXT            *NewKext;
  MACH_SEGMENT_COMMAND_ANY  *Segment;

  UINT64                    VirtualAddress;
  UINT64                    FileOffset;
  UINT64                    KernelSize;
  UINT64                    KextsSize;
  UINT64                    KernelOffset;
  UINT64                    KextsOffset;

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

  Segment = MachoGetSegmentByName (
    &NewKext->Context.MachContext,
    "__TEXT"
    );
  if (Segment == NULL) {
    FreePool (NewKext);
    return NULL;
  }
  
  VirtualAddress = Prelinked->Is32Bit ? Segment->Segment32.VirtualAddress : Segment->Segment64.VirtualAddress;
  FileOffset     = Prelinked->Is32Bit ? Segment->Segment32.FileOffset : Segment->Segment64.FileOffset;

  if (VirtualAddress < FileOffset) {
    FreePool (NewKext);
    return NULL;
  }

  NewKext->Signature                  = PRELINKED_KEXT_SIGNATURE;
  NewKext->Identifier                 = PRELINK_KERNEL_IDENTIFIER;
  NewKext->BundleLibraries            = NULL;
  NewKext->CompatibleVersion          = "0";
  NewKext->Context.Is32Bit            = Prelinked->Is32Bit;
  NewKext->Context.VirtualBase        = VirtualAddress - FileOffset;
  NewKext->Context.VirtualKmod        = 0;
  NewKext->Context.IsKernelCollection = Prelinked->IsKernelCollection;

  if (!Prelinked->IsKernelCollection) {
    //
    // Find optional __PRELINK_STATE segment, present in 10.6.8
    //
    Prelinked->PrelinkedStateSegment = MachoGetSegmentByName (
      &Prelinked->PrelinkedMachContext,
      PRELINK_STATE_SEGMENT
      );

    if (Prelinked->PrelinkedStateSegment != NULL) {
      Prelinked->PrelinkedStateSectionKernel = MachoGetSectionByName (
        &Prelinked->PrelinkedMachContext,
        Prelinked->PrelinkedStateSegment,
        PRELINK_STATE_SECTION_KERNEL
        );
      Prelinked->PrelinkedStateSectionKexts = MachoGetSectionByName (
        &Prelinked->PrelinkedMachContext,
        Prelinked->PrelinkedStateSegment,
        PRELINK_STATE_SECTION_KEXTS
        );

      if (Prelinked->PrelinkedStateSectionKernel != NULL
        && Prelinked->PrelinkedStateSectionKexts != NULL) {
        KernelSize = Prelinked->Is32Bit ?
          Prelinked->PrelinkedStateSectionKernel->Section32.Size :
          Prelinked->PrelinkedStateSectionKernel->Section64.Size;
        KextsSize = Prelinked->Is32Bit ?
          Prelinked->PrelinkedStateSectionKexts->Section32.Size :
          Prelinked->PrelinkedStateSectionKexts->Section64.Size;
        KernelOffset = Prelinked->Is32Bit ?
          Prelinked->PrelinkedStateSectionKernel->Section32.Offset :
          Prelinked->PrelinkedStateSectionKernel->Section64.Offset;
        KextsOffset = Prelinked->Is32Bit ?
          Prelinked->PrelinkedStateSectionKexts->Section32.Offset :
          Prelinked->PrelinkedStateSectionKexts->Section64.Offset;

        if (KernelSize > 0 && KextsSize > 0) {
          Prelinked->PrelinkedStateKernelSize = (UINT32) KernelSize;
          Prelinked->PrelinkedStateKextsSize = (UINT32) KextsSize;
          Prelinked->PrelinkedStateKernel = AllocateCopyPool (
            Prelinked->PrelinkedStateKernelSize,
            &Prelinked->Prelinked[KernelOffset]
            );
          Prelinked->PrelinkedStateKexts = AllocateCopyPool (
            Prelinked->PrelinkedStateKextsSize,
            &Prelinked->Prelinked[KextsOffset]
            );
          if (Prelinked->PrelinkedStateKernel != NULL
            && Prelinked->PrelinkedStateKexts != NULL) {
            Prelinked->PrelinkedStateKextsAddress = Prelinked->Is32Bit ?
              Prelinked->PrelinkedStateSectionKexts->Section32.Address :
              Prelinked->PrelinkedStateSectionKexts->Section64.Address;
            NewKext->Context.KxldState = Prelinked->PrelinkedStateKernel;
            NewKext->Context.KxldStateSize = Prelinked->PrelinkedStateKernelSize;
          } else {
            DEBUG ((
              DEBUG_INFO,
              "OCAK: Ignoring unused PK state __kernel %p __kexts %p\n",
              Prelinked->PrelinkedStateSectionKernel,
              Prelinked->PrelinkedStateSectionKexts
              ));
            if (Prelinked->PrelinkedStateKernel != NULL) {
              FreePool (Prelinked->PrelinkedStateKernel);
            }
            if (Prelinked->PrelinkedStateKexts != NULL) {
              FreePool (Prelinked->PrelinkedStateKexts);
            }
            Prelinked->PrelinkedStateSectionKernel = NULL;
            Prelinked->PrelinkedStateSectionKexts = NULL;
            Prelinked->PrelinkedStateSegment = NULL;
          }
        }
      }
    }
  }

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
        DEBUG ((DEBUG_VERBOSE, "OCAK: Ignoring KPI %a for kext %a in KC/state mode\n", DependencyId, Kext->Identifier));
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
    if (Kext->Context.KxldState != NULL) {
      //
      // Use KXLD state as is for 10.6.8 kernel.
      //
      Status = InternalKxldStateBuildLinkedSymbolTable (
        Kext,
        Context
        );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = InternalKxldStateBuildLinkedVtables (
        Kext,
        Context
        );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } else {
      //
      // Use normal LINKEDIT building for newer kernels and all kexts.
      //
      Status = InternalScanBuildLinkedSymbolTable (Kext, Context);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = InternalScanBuildLinkedVtables (Kext, Context);
      if (EFI_ERROR (Status)) {
        return Status;
      }
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
  IN     UINT64             KmodAddress,
  IN     UINT64             FileOffset
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

  Status = InternalPrelinkKext (Context, Kext, LoadAddress, FileOffset);

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

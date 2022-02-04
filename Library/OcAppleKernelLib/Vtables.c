/** @file
  Library handling KEXT prelinking.

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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>

#include "PrelinkedInternal.h"

CONST PRELINKED_VTABLE *
InternalGetOcVtableByNameWorker (
  IN PRELINKED_CONTEXT     *Context,
  IN PRELINKED_KEXT        *Kext,
  IN CONST CHAR8           *Name
  )
{
  CONST PRELINKED_VTABLE *Vtable;

  UINTN                  Index;
  PRELINKED_KEXT         *Dependency;
  INTN                   Result;

  Kext->Processed = TRUE;

  for (
    Index = 0, Vtable = Kext->LinkedVtables;
    Index < Kext->NumberOfVtables;
    ++Index, Vtable = GET_NEXT_PRELINKED_VTABLE (Vtable)
    ) {
    Result = AsciiStrCmp (Vtable->Name, Name);
    if (Result == 0) {
      return Vtable;
    }
  }

  for (Index = 0; Index < ARRAY_SIZE (Kext->Dependencies); ++Index) {
    Dependency = Kext->Dependencies[Index];
    if (Dependency == NULL) {
      break;
    }

    if (Dependency->Processed) {
      continue;
    }

    Vtable = InternalGetOcVtableByName (Context, Dependency, Name);
    if (Vtable != NULL) {
      return Vtable;
    }
  }

  return NULL;
}

CONST PRELINKED_VTABLE *
InternalGetOcVtableByName (
  IN PRELINKED_CONTEXT     *Context,
  IN PRELINKED_KEXT        *Kext,
  IN CONST CHAR8           *Name
  )
{
  CONST PRELINKED_VTABLE *Vtable;

  Vtable = InternalGetOcVtableByNameWorker (Context, Kext, Name);

  InternalUnlockContextKexts (Context);

  return Vtable;
}

STATIC
VOID
InternalConstructVtablePrelinked (
  IN     PRELINKED_CONTEXT                      *Context,
  IN OUT PRELINKED_KEXT                         *Kext,
  IN     CONST OC_PRELINKED_VTABLE_LOOKUP_ENTRY *VtableLookup,
  OUT    PRELINKED_VTABLE                       *Vtable
  )
{
  CONST VOID                                    *VtableData;
  UINT64                                        Value;
  UINT32                                        Index;
  CONST PRELINKED_KEXT_SYMBOL                   *Symbol;

  ASSERT (Kext != NULL);
  ASSERT (VtableLookup != NULL);
  ASSERT (Vtable != NULL);

  VtableData   = VtableLookup->Vtable.Data;
  Vtable->Name = VtableLookup->Name;

  //
  // Initialize the VTable by entries.
  //

  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  // VTable bounds are verified in InternalGetVtableEntries() called earlier.
  // The buffer is allocated to be able to hold all entries.
  //
  for (
    Index = 0;
    (Value = VTABLE_ENTRY_X (Context->Is32Bit, VtableData, Index + VTABLE_HEADER_LEN)) != 0;
    ++Index
    ) {

    if (Context->IsKernelCollection) {
      Value = KcFixupValue (Value, Kext->Identifier);
    }

    //
    // If we can't find the symbol, it means that the virtual function was
    // defined inline.  There's not much I can do about this; it just means
    // I can't patch this function.
    //
    Symbol = InternalOcGetSymbolValue (Context, Kext, Value, OcGetSymbolOnlyCxx);

    if (Symbol != NULL) {
      Vtable->Entries[Index].Address = Value;
      Vtable->Entries[Index].Name    = Symbol->Name;
    } else {
      Vtable->Entries[Index].Address = 0;
      Vtable->Entries[Index].Name    = NULL;
    }
  }

  Vtable->NumEntries = Index;
}

BOOLEAN
InternalGetVtableEntries (
  IN  BOOLEAN       Is32Bit,
  IN  CONST VOID    *VtableData,
  IN  UINT32        MaxSize,
  OUT UINT32        *NumEntries
  )
{
  UINT32 Index;

  ASSERT (VtableData != NULL);
  ASSERT (NumEntries != NULL);
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  //
  MaxSize /= VTABLE_ENTRY_SIZE_X (Is32Bit);
  Index    = VTABLE_HEADER_LEN;
  do {
    if (Index >= MaxSize) {
      return FALSE;
    }
  } while (VTABLE_ENTRY_X (Is32Bit, VtableData, Index++) != 0);

  *NumEntries = (Index - VTABLE_HEADER_LEN);
  return TRUE;
}

BOOLEAN
InternalPrepareCreateVtablesPrelinked (
  IN  PRELINKED_KEXT                    *Kext,
  IN  UINT32                            MaxSize,
  OUT UINT32                            *NumVtables,
  OUT OC_PRELINKED_VTABLE_LOOKUP_ENTRY  *Vtables
  )
{
  UINT32                      VtableIndex;

  CONST PRELINKED_KEXT_SYMBOL *Symbol;
  UINT32                      Index;

  ASSERT (Kext != NULL);

  VtableIndex = 0;

  MaxSize /= sizeof (*Vtables);
  for (
    Index = (Kext->NumberOfSymbols - Kext->NumberOfCxxSymbols);
    Index < Kext->NumberOfSymbols;
    ++Index
    ) {
    Symbol = &Kext->LinkedSymbolTable[Index];
    if (MachoSymbolNameIsVtable (Symbol->Name)) {
      //
      // This seems to be valid for KC format as some symbols may be kernel imports?!
      // Observed with IOACPIFamily when injecting VirtualSMC:
      // __ZTV18IODTPlatformExpert                 (zero value)
      // __ZTV16IOPlatformDevice                   (zero value)
      // __ZTVN20IOACPIPlatformExpert9MetaClassE
      // __ZTVN20IOACPIPlatformDevice9MetaClassE
      // __ZTV20IOACPIPlatformExpert
      // __ZTV20IOACPIPlatformDevice
      //
      if (Symbol->Value == 0) {
        DEBUG ((DEBUG_VERBOSE, "OCAK: Skipping %a with NULL value\n", Symbol->Name));
        continue;
      }

      if (VtableIndex >= MaxSize) {
        return FALSE;
      }

      Vtables[VtableIndex].Name         = Symbol->Name;
      Vtables[VtableIndex].Vtable.Value = Symbol->Value;
      ++VtableIndex;
    }
  }

  *NumVtables = VtableIndex;

  return TRUE;
}

VOID
InternalCreateVtablesPrelinked (
  IN     PRELINKED_CONTEXT                       *Context,
  IN OUT PRELINKED_KEXT                          *Kext,
  IN     UINT32                                  NumVtables,
  IN     CONST OC_PRELINKED_VTABLE_LOOKUP_ENTRY  *VtableLookups,
  OUT    PRELINKED_VTABLE                        *VtableBuffer
  )
{
  UINT32              Index;

  ASSERT (Kext != NULL);

  for (Index = 0; Index < NumVtables; ++Index) {
    InternalConstructVtablePrelinked (
                Context,
                Kext,
                &VtableLookups[Index],
                VtableBuffer
                );
    VtableBuffer = GET_NEXT_PRELINKED_VTABLE (VtableBuffer);
  }
}

/**
 * kxld_vtable_patch
 */
STATIC
BOOLEAN
InternalPatchVtableSymbol (
  IN OUT OC_MACHO_CONTEXT              *MachoContext,
  IN     CONST PRELINKED_VTABLE_ENTRY  *ParentEntry,
  IN     CONST CHAR8                   *VtableName,
  OUT    MACH_NLIST_ANY                *Symbol
  )
{
  CONST CHAR8 *Name;
  INTN        Result;
  BOOLEAN     Success;
  CONST CHAR8 *ClassName;
  CHAR8       FunctionPrefix[SYM_MAX_NAME_LEN];

  ASSERT (Symbol != NULL);
  ASSERT (ParentEntry != NULL);
  //
  // It's possible for the patched parent entry not to have a symbol
  // (e.g. when the definition is inlined).  We can't patch this entry no
  // matter what, so we'll just skip it and die later if it's a problem
  // (which is not likely).
  //
  if (ParentEntry->Name == NULL) {
    return TRUE;
  }
  //
  // 1) If the symbol is defined locally, do not patch
  //
  if (MachoSymbolIsLocalDefined (MachoContext, Symbol)) {
    return TRUE;
  }

  Name = MachoGetSymbolName (MachoContext, Symbol);
  //
  // 2) If the child is a pure virtual function, do not patch.
  // In general, we want to proceed with patching when the symbol is
  // externally defined because pad slots fall into this category.
  // The pure virtual function symbol is special case, as the pure
  // virtual property itself overrides the parent's implementation.
  //
  if (MachoSymbolNameIsPureVirtual (Name)) {
    return TRUE;
  }
  //
  // 3) If the symbols are the same, do not patch
  //
  Result = AsciiStrCmp (Name, ParentEntry->Name);
  if (Result == 0) {
    return TRUE;
  }
  //
  // 4) If the parent vtable entry is a pad slot, and the child does not
  // match it, then the child was built against a newer version of the
  // libraries, so it is binary-incompatible.
  //
  if (MachoSymbolNameIsPadslot (ParentEntry->Name)) {
    return FALSE;
  }
  //
  // 5) If we are doing strict patching, we prevent kexts from declaring
  // virtual functions and not implementing them.  We can tell if a
  // virtual function is declared but not implemented because we resolve
  // symbols before patching; an unimplemented function will still be
  // undefined at this point.  We then look at whether the symbol has
  // the same class prefix as the vtable.  If it does, the symbol was
  // declared as part of the class and not inherited, which means we
  // should not patch it.
  //
  if (!MachoSymbolIsDefined (MachoContext, Symbol)) {
    ClassName = MachoGetClassNameFromVtableName (VtableName);

    Success = MachoGetFunctionPrefixFromClassName (
                ClassName,
                sizeof (FunctionPrefix),
                FunctionPrefix
                );
    if (!Success) {
      return FALSE;
    }

    Result = AsciiStrCmp (Name, FunctionPrefix);
    if (Result == 0) {
      //
      // The VTable's class declares a method without providing an
      // implementation.
      //
      return FALSE;
    }
  }
  //
  // 6) The child symbol is unresolved and different from its parent, so
  // we need to patch it up.  We do this by modifying the relocation
  // entry of the vtable entry to point to the symbol of the parent
  // vtable entry.  If that symbol does not exist (i.e. we got the data
  // from a link state object's vtable representation), then we create a
  // new symbol in the symbol table and point the relocation entry to
  // that.
  //
  // NOTE: The original logic has been altered significantly.  Instead of
  //       declaring a symbol as "replaced" and either changing the
  //       associated relocation's index to the parent's or adding a new symbol
  //       based on a match, the symbol is actually overwritten.  This looks
  //       fine for the rest of the control flow.  The symbol name is not
  //       changed for the symbol value is already resolved and nothing but a
  //       VTable Relocation should reference it.
  //
  InternalSolveSymbolValue (MachoContext->Is32Bit, ParentEntry->Address, Symbol);
  //
  // The C++ ABI requires that functions be aligned on a 2-byte boundary:
  // http://www.codesourcery.com/public/cxx-abi/abi.html#member-pointers
  // If the LSB of any virtual function's link address is 1, then the
  // compiler has violated that part of the ABI, and we're going to panic
  // in _ptmf2ptf() (in OSMetaClass.h). Better to panic here with some
  // context.
  //
  Name = ParentEntry->Name;
  if (!MachoSymbolNameIsPureVirtual (Name)
    && (((MachoContext->Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value) & 1U) != 0)) {
    DEBUG ((DEBUG_WARN, "OCAK: Prelink: Invalid VTable symbol\n"));
  }

  return TRUE;
}

STATIC
BOOLEAN
InternalInitializeVtableByEntriesAndRelocations (
  IN     PRELINKED_CONTEXT            *Context,
  IN     PRELINKED_KEXT               *Kext,
  IN     CONST PRELINKED_VTABLE       *SuperVtable,
  IN     CONST MACH_NLIST_ANY         *VtableSymbol,
  IN     CONST VOID                   *VtableData,
  IN     UINT32                       NumSolveSymbols,
  IN OUT MACH_NLIST_ANY               **SolveSymbols,
  OUT    PRELINKED_VTABLE             *Vtable
  )
{
  OC_MACHO_CONTEXT           *MachoContext;
  CONST CHAR8                *VtableName;
  CONST PRELINKED_KEXT_SYMBOL *OcSymbol;
  UINT32                     Index;
  PRELINKED_VTABLE_ENTRY     *VtableEntries;
  UINT64                     EntryValue;
  UINT32                     SolveSymbolIndex;
  BOOLEAN                    Result;
  MACH_NLIST_ANY             *Symbol;

  MachoContext  = &Kext->Context.MachContext;
  VtableEntries = Vtable->Entries;

  VtableName       = MachoGetSymbolName (MachoContext, VtableSymbol);
  SolveSymbolIndex = 0;
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  //
  for (Index = 0; Index < SuperVtable->NumEntries; ++Index) {
    EntryValue = VTABLE_ENTRY_X (Context->Is32Bit, VtableData, Index + VTABLE_HEADER_LEN);
    if (EntryValue != 0) {
      //
      // If we can't find a symbol, it means it is a locally-defined,
      // non-external symbol that has been stripped.  We don't patch over
      // locally-defined symbols, so we leave the symbol as NULL and just
      // skip it.  We won't be able to patch subclasses with this symbol,
      // but there isn't much we can do about that.
      //
      OcSymbol = InternalOcGetSymbolValue (
                   Context,
                   Kext,
                   EntryValue,
                   OcGetSymbolOnlyCxx
                   );
      if (OcSymbol != NULL) {
        VtableEntries[Index].Name    = OcSymbol->Name;
        VtableEntries[Index].Address = OcSymbol->Value;
        continue;
      }
    } else {
      if (SolveSymbolIndex >= NumSolveSymbols) {
        //
        // When no more symbols are left to resolve with, this marks the end.
        //
        break;
      }

      Symbol = SolveSymbols[SolveSymbolIndex];
      ++SolveSymbolIndex;
      //
      // The child entry can be NULL when a locally-defined, non-external
      // symbol is stripped.  We wouldn't patch this entry anyway, so we
      // just skip it.
      //
      if (Symbol != NULL) {
        Result = InternalPatchVtableSymbol (
                   MachoContext,
                   &SuperVtable->Entries[Index],
                   VtableName,
                   Symbol
                   );
        if (!Result) {
          DEBUG ((DEBUG_INFO, "OCAK: Failed to patch symbol %a for vtable %a\n", MachoGetSymbolName (MachoContext, Symbol), VtableName));
          return FALSE;
        }

        VtableEntries[Index].Name    = MachoGetSymbolName (MachoContext, Symbol);
        VtableEntries[Index].Address = Context->Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value;
        continue;
      }
    }

    VtableEntries[Index].Name    = NULL;
    VtableEntries[Index].Address = 0;
  }

  Vtable->Name       = VtableName;
  Vtable->NumEntries = Index;

  return TRUE;
}

STATIC
BOOLEAN
InternalInitializeVtablePatchData (
  IN OUT OC_MACHO_CONTEXT     *MachoContext,
  IN     CONST MACH_NLIST_ANY *VtableSymbol,
  IN OUT UINT32               *MaxSize,
  OUT    VOID                 **VtableDataPtr,
  OUT    UINT32               *NumEntries,
  OUT    UINT32               *NumSymbols,
  OUT    MACH_NLIST_ANY       **SolveSymbols
  )
{
  BOOLEAN               Result;
  UINT32                VtableOffset;
  UINT32                VtableMaxSize;
  CONST MACH_HEADER_ANY *MachHeader;
  VOID                  *VtableData;
  UINT32                SymIndex;
  UINT32                EntryOffset;
  UINT64                FileOffset;
  UINT32                MaxSymbols;
  MACH_NLIST_ANY        *Symbol;

  Result = MachoSymbolGetFileOffset (
             MachoContext,
             VtableSymbol,
             &VtableOffset,
             &VtableMaxSize
             );
  if (!Result) {
    return FALSE;
  }

  MachHeader = MachoGetMachHeader (MachoContext);
  ASSERT (MachHeader != NULL);

  VtableData = (VOID *)((UINTN) MachHeader + VtableOffset);
  if (MachoContext->Is32Bit ? !OC_TYPE_ALIGNED (UINT32, VtableData) : !OC_TYPE_ALIGNED (UINT64, VtableData)) {
    return FALSE;
  }
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  //
  MaxSymbols     = (*MaxSize / (MachoContext->Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64)));
  VtableMaxSize /= VTABLE_ENTRY_SIZE_X (MachoContext->Is32Bit);

  SymIndex = 0;

  for (
    EntryOffset = VTABLE_HEADER_LEN;
    EntryOffset < VtableMaxSize;
    ++EntryOffset
    ) {
    if (VTABLE_ENTRY_X (MachoContext->Is32Bit, VtableData, EntryOffset) == 0) {
      Result = OcOverflowAddU64 (
                 MachoContext->Is32Bit ? VtableSymbol->Symbol32.Value : VtableSymbol->Symbol64.Value,
                 (EntryOffset * VTABLE_ENTRY_SIZE_X (MachoContext->Is32Bit)),
                 &FileOffset
                 );
      if (Result) {
        return FALSE;
      }

      Result = MachoGetSymbolByExternRelocationOffset (
                 MachoContext,
                 FileOffset,
                 &Symbol
                 );
      if (!Result) {
        //
        // If the VTable entry is 0 and it is not referenced by a Relocation,
        // it is the end of the table.
        //
        *MaxSize      -= (SymIndex * (MachoContext->Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64)));
        *VtableDataPtr = VtableData;
        *NumEntries    = (EntryOffset - VTABLE_HEADER_LEN);
        *NumSymbols    = SymIndex;
        return TRUE;
      }

      if (SymIndex >= MaxSymbols) {
        return FALSE;
      }

      SolveSymbols[SymIndex] = Symbol;
      ++SymIndex;
    }
  }

  return FALSE;
}

BOOLEAN
InternalPatchByVtables (
  IN     PRELINKED_CONTEXT         *Context,
  IN OUT PRELINKED_KEXT            *Kext
  )
{
  OC_VTABLE_PATCH_ENTRY *Entries;
  OC_VTABLE_PATCH_ENTRY *EntryWalker;
  UINT32                MaxSize;

  OC_MACHO_CONTEXT        *MachoContext;
  CONST MACH_HEADER_ANY   *MachHeader;
  UINT32                  Index;
  UINT32                  NumTables;
  UINT32                  NumEntries;
  UINT32                  NumEntriesTemp;
  UINT32                  NumPatched;
  BOOLEAN                 Result;
  CONST MACH_NLIST_ANY    *Smcp;
  CONST CHAR8             *Name;
  CONST MACH_NLIST_ANY    *MetaClass;
  CONST PRELINKED_VTABLE  *SuperVtable;
  CONST PRELINKED_VTABLE  *MetaVtable;
  CONST VOID              *OcSymbolDummy;
  MACH_NLIST_ANY          *SymbolDummy;
  CHAR8                   ClassName[SYM_MAX_NAME_LEN];
  CHAR8                   SuperClassName[SYM_MAX_NAME_LEN];
  CHAR8                   VtableName[SYM_MAX_NAME_LEN];
  CHAR8                   SuperVtableName[SYM_MAX_NAME_LEN];
  CHAR8                   FinalSymbolName[SYM_MAX_NAME_LEN];
  BOOLEAN                 SuccessfulIteration;
  PRELINKED_VTABLE        *CurrentVtable;

  //
  // LinkBuffer is at least as big as __LINKEDIT, so it can store all symbols.
  //
  Entries = Context->LinkBuffer;
  MaxSize = Context->LinkBufferSize;

  MachoContext = &Kext->Context.MachContext;

  MachHeader = MachoGetMachHeader (MachoContext);
  ASSERT (MachHeader != NULL);
  //
  // Retrieve all SMCPs.
  //
  EntryWalker = Entries;
  NumTables   = 0;
  NumEntries  = 0;

  for (
    Index = 0;
    (Smcp = MachoGetSymbolByIndex (MachoContext, Index)) != NULL;
    ++Index
    ) {
    Name = MachoGetSymbolName (MachoContext, Smcp);
    if ((((Context->Is32Bit ? Smcp->Symbol32.Type : Smcp->Symbol64.Type) & MACH_N_TYPE_STAB) == 0)
     && MachoSymbolNameIsSmcp (MachoContext, Name)) {
      if (MaxSize < sizeof (*EntryWalker)) {
        return FALSE;
      }
      //
      // We walk over the super metaclass pointer symbols because classes
      // with them are the only ones that need patching.  Then we double the
      // number of vtables we're expecting, because every pointer will have a
      // class vtable and a MetaClass vtable.
      //
      Result = MachoGetVtableSymbolsFromSmcp (
                 MachoContext,
                 Name,
                 &EntryWalker->Vtable,
                 &EntryWalker->MetaVtable
                 );
      if (!Result) {
        return FALSE;
      }

      EntryWalker->Smcp = Smcp;

      Result = InternalInitializeVtablePatchData (
                 MachoContext,
                 EntryWalker->Vtable,
                 &MaxSize,
                 &EntryWalker->VtableData,
                 &NumEntriesTemp,
                 &EntryWalker->MetaSymsIndex,
                 EntryWalker->SolveSymbols
                 );
      if (!Result) {
        return FALSE;
      }

      NumEntries += NumEntriesTemp;

      Result = InternalInitializeVtablePatchData (
                 MachoContext,
                 EntryWalker->MetaVtable,
                 &MaxSize,
                 &EntryWalker->MetaVtableData,
                 &NumEntriesTemp,
                 &EntryWalker->NumSolveSymbols,
                 &EntryWalker->SolveSymbols[EntryWalker->MetaSymsIndex]
                 );
      if (!Result) {
        return FALSE;
      }

      NumEntries += NumEntriesTemp;

      EntryWalker->NumSolveSymbols += EntryWalker->MetaSymsIndex;
      ++NumTables;

      EntryWalker = GET_NEXT_OC_VTABLE_PATCH_ENTRY (EntryWalker);
    }
  }
  //
  // One structure contains two VTables, hence (NumTables * 2).
  //
  Kext->LinkedVtables = AllocatePool (
                          ((NumTables * 2) * sizeof (*Kext->LinkedVtables))
                            + (NumEntries * sizeof (*Kext->LinkedVtables->Entries))
                          );
  if (Kext->LinkedVtables == NULL) {
    return FALSE;
  }

  CurrentVtable = Kext->LinkedVtables;
  //
  // Patch via the previously retrieved SMCPs.
  //
  NumPatched   = 0;

  while (NumPatched < NumTables) {
    SuccessfulIteration = FALSE;

    for (
      Index = 0, EntryWalker = Entries;
      Index < NumTables;
      ++Index, EntryWalker = GET_NEXT_OC_VTABLE_PATCH_ENTRY (EntryWalker)
      ) {
      Smcp = EntryWalker->Smcp;
      if (Smcp == NULL) {
        continue;
      }

      Name = MachoGetSymbolName (MachoContext, Smcp);
      //
      // We walk over the super metaclass pointer symbols because classes
      // with them are the only ones that need patching.  Then we double the
      // number of vtables we're expecting, because every pointer will have a
      // class vtable and a MetaClass vtable.
      //
      ASSERT (MachoSymbolNameIsSmcp (MachoContext, Name));
      //
      // Get the class name from the smc pointer
      //
      Result = MachoGetClassNameFromSuperMetaClassPointer (
                 MachoContext,
                 Name,
                 sizeof (ClassName),
                 ClassName
                 );
      if (!Result) {
        return FALSE;
      }
      //
      // Get the vtable name from the class name
      //
      Result = MachoGetVtableNameFromClassName (
                 ClassName,
                 sizeof (VtableName),
                 VtableName
                 );
      if (!Result) {
        return FALSE;
      }
      //
      // Find the SMCP's meta class symbol
      //
      MetaClass = MachoGetMetaclassSymbolFromSmcpSymbol (
                    MachoContext,
                    Smcp
                    );
      if (MetaClass == NULL) {
        return FALSE;
      }
      //
      // Get the super class name from the super metaclass
      //
      Result = MachoGetClassNameFromMetaClassPointer (
                 MachoContext,
                 MachoGetSymbolName (MachoContext, MetaClass),
                 sizeof (SuperClassName),
                 SuperClassName
                 );
      if (!Result) {
        return FALSE;
      }

      Result = MachoGetVtableNameFromClassName (
        SuperClassName,
        sizeof (SuperVtableName),
        SuperVtableName
        );

      if (!Result) {
        return FALSE;
      }

      //
      // Get the super vtable if it's been patched
      //
      SuperVtable = InternalGetOcVtableByName (
                      Context,
                      Kext,
                      SuperVtableName
                      );
      if (SuperVtable == NULL) {
        continue;
      }
      //
      // Get the final symbol's name from the super vtable
      //
      Result = MachoGetFinalSymbolNameFromClassName (
                 SuperClassName,
                 sizeof (FinalSymbolName),
                 FinalSymbolName
                 );
      if (!Result) {
        return FALSE;
      }
      //
      // Verify that the final symbol does not exist.  First check
      // all the externally defined symbols, then check locally.
      //
      OcSymbolDummy = InternalOcGetSymbolName (
                        Context,
                        Kext,
                        FinalSymbolName,
                        OcGetSymbolAnyLevel
                        );
      if (OcSymbolDummy != NULL) {
        return FALSE;
      }

      SymbolDummy = MachoGetLocalDefinedSymbolByName (
                  MachoContext,
                  FinalSymbolName
                  );
      if (SymbolDummy != NULL) {
        return FALSE;
      }
      //
      // Patch the class's vtable
      //
      Result = InternalInitializeVtableByEntriesAndRelocations (
                 Context,
                 Kext,
                 SuperVtable,
                 EntryWalker->Vtable,
                 EntryWalker->VtableData,
                 EntryWalker->MetaSymsIndex,
                 EntryWalker->SolveSymbols,
                 CurrentVtable
                 );
      if (!Result) {
        DEBUG ((DEBUG_INFO, "OCAK: Failed to patch vtable for superclass %a\n", SuperClassName));
        return FALSE;
      }

      CurrentVtable = GET_NEXT_PRELINKED_VTABLE (CurrentVtable);
      //
      // Get the meta vtable name from the class name
      //
      Result = MachoGetMetaVtableNameFromClassName (
                 ClassName,
                 sizeof (VtableName),
                 VtableName
                 );
      if (!Result) {
        return FALSE;
      }

      MetaVtable = InternalGetOcVtableByName (
                     Context,
                     Kext,
                     VtableName
                     );
      if (MetaVtable != NULL) {
        return FALSE;
      }
      //
      // There is no way to look up a metaclass vtable at runtime, but
      // we know that every class's metaclass inherits directly from
      // OSMetaClass, so we just hardcode that vtable name here.
      //
      SuperVtable = InternalGetOcVtableByName (
                      Context,
                      Kext,
                      OS_METACLASS_VTABLE_NAME
                      );
      if (SuperVtable == NULL) {
        return FALSE;
      }
      //
      // meta_vtable_sym will be null when we don't support strict
      // patching and can't find the metaclass vtable. If that's the
      // case, we just reduce the expect number of vtables by 1.
      // Only i386 does not support strict patchting.
      //
      Result = InternalInitializeVtableByEntriesAndRelocations (
                 Context,
                 Kext,
                 SuperVtable,
                 EntryWalker->MetaVtable,
                 EntryWalker->MetaVtableData,
                 (EntryWalker->NumSolveSymbols - EntryWalker->MetaSymsIndex),
                 &EntryWalker->SolveSymbols[EntryWalker->MetaSymsIndex],
                 CurrentVtable
                 );
      if (!Result) {
        DEBUG ((DEBUG_INFO, "OCAK: Failed to patch meta vtable for superclass %a\n", SuperClassName));
        return FALSE;
      }

      CurrentVtable = GET_NEXT_PRELINKED_VTABLE (CurrentVtable);

      Kext->NumberOfVtables += 2;

      EntryWalker->Smcp = NULL;

      ++NumPatched;
      SuccessfulIteration = TRUE;
    }
    //
    // Exit when there are unpatched VTables left, but there are none patched
    // in a full iteration.
    //
    if (!SuccessfulIteration) {
      return FALSE;
    }
  }

  return TRUE;
}

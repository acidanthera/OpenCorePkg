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
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcAppleKernelLib.h>

#include "PrelinkedInternal.h"

CONST PRELINKED_VTABLE *
InternalGetOcVtableByName (
  IN PRELINKED_CONTEXT     *Context,
  IN CONST PRELINKED_KEXT  *Kext,
  IN CONST CHAR8           *Name,
  IN UINT32                RecursionLevel
  )
{
  CONST PRELINKED_VTABLE *Vtable;

  UINTN                  Index;
  UINT32                 Index2;
  PRELINKED_KEXT         *Dependency;
  INTN                   Result;

  Vtable = NULL;

  for (Index = 0; Vtable == NULL && Index < ARRAY_SIZE (Kext->Dependencies); ++Index) {
    Dependency = Kext->Dependencies[Index];
    if (Dependency == NULL) {
      break;
    }

    if (Dependency->Processed) {
      continue;
    }

    Dependency->Processed = TRUE;

    for (
      Index2 = 0, Vtable = Dependency->LinkedVtables;
      Index2 < Dependency->NumberOfVtables;
      ++Index2, Vtable = GET_NEXT_PRELINKED_VTABLE (Vtable)
      ) {
      Result = AsciiStrCmp (Vtable->Name, Name);
      if (Result == 0) {
        break;
      }
    }

    if (Index2 == Dependency->NumberOfVtables) {
      Vtable = InternalGetOcVtableByName (Context, Dependency, Name, RecursionLevel+1);
    }
  }

  if (RecursionLevel == 0) {
    InternalUnlockContextKexts (Context);
  }

  return Vtable;
}

STATIC
BOOLEAN
InternalConstructVtablePrelinked64 (
  IN     PRELINKED_CONTEXT         *Context,
  IN OUT PRELINKED_KEXT            *Kext,
  IN     CONST MACH_NLIST_64       *VtableSymbol,
  OUT    PRELINKED_VTABLE          *Vtable
  )
{
  OC_MACHO_CONTEXT            *MachoContext;
  BOOLEAN                     Result;
  CONST MACH_HEADER_64        *MachHeader;
  UINT32                      VtableOffset;
  UINT32                      MaxSize;
  CONST UINT64                *VtableData;
  UINT64                      Value;
  UINT32                      Index;
  CONST PRELINKED_KEXT_SYMBOL *Symbol;

  ASSERT (Kext != NULL);
  ASSERT (VtableSymbol != NULL);
  ASSERT (Vtable != NULL);

  MachoContext = &Kext->Context.MachContext;

  MachHeader = MachoGetMachHeader64 (MachoContext);
  ASSERT (MachHeader != NULL);

  Result = MachoSymbolGetFileOffset64 (
             MachoContext,
             VtableSymbol,
             &VtableOffset,
             &MaxSize
             );
  if (!Result || (MaxSize < (VTABLE_HEADER_SIZE_64 + VTABLE_ENTRY_SIZE_64))) {
    return FALSE;
  }

  VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
  if (!OC_ALIGNED (VtableData)) {
    return FALSE;
  }

  Vtable->Name = MachoGetSymbolName64 (MachoContext, VtableSymbol);
  Vtable->NumEntries = 0;

  //
  // Initialize the VTable by entries.
  //

  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  // VTable bounds are verified in InternalGetVtableEntries64() called earlier.
  //
  MaxSize /= VTABLE_ENTRY_SIZE_64;
  for (
    Index = 0;
    (Value = VtableData[Index + VTABLE_HEADER_LEN_64]) != 0;
    ++Index
    ) {
    Symbol = InternalOcGetSymbolValue (Context, Kext, Value, OcGetSymbolOnlyCxx);

    if (Symbol != NULL) {
      Vtable->Entries[Index].Address = Value;
      Vtable->Entries[Index].Name = (Kext->StringTable + Symbol->StringIndex);
    } else {
      //
      // If we can't find the symbol, it means that the virtual function was
      // defined inline.  There's not much I can do about this; it just means
      // I can't patch this function.
      //
      Vtable->Entries[Index].Address = 0;
      Vtable->Entries[Index].Name    = NULL;
    }

    ++Vtable->NumEntries;

    if ((Index + VTABLE_HEADER_LEN_64 + 1) >= MaxSize) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
InternalGetVtableEntries64 (
  IN  CONST UINT64  *VtableData,
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
  MaxSize /= VTABLE_ENTRY_SIZE_64;
  Index    = VTABLE_HEADER_LEN_64;
  do {
    if (Index >= MaxSize) {
      return FALSE;
    }
  } while (VtableData[Index++] != 0);

  *NumEntries = (Index - VTABLE_HEADER_LEN_64);
  return TRUE;
}

BOOLEAN
InternalPrepareCreateVtablesPrelinked64 (
  IN  OC_MACHO_CONTEXT          *MachoContext,
  OUT OC_VTABLE_EXPORT_ARRAY    *VtableExport,
  IN  UINT32                    VtableExportSize
  )
{
  UINT32              NumVtables;

  CONST MACH_NLIST_64 *SymbolTable;
  CONST MACH_NLIST_64 *Symbol;
  CONST CHAR8         *Name;
  UINT32              NumSymbols;
  UINT32              Index;

  ASSERT (MachoContext != NULL);

  NumVtables = 0;

  NumSymbols = MachoGetSymbolTable (
                 MachoContext,
                 &SymbolTable,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL
                 );
  for (Index = 0; Index < NumSymbols; ++Index) {
    Symbol = &SymbolTable[Index];
    Name = MachoGetSymbolName64 (MachoContext, Symbol);
    if (MachoSymbolNameIsVtable64 (Name)) {
      //
      // This should be accounted for via previous sanity checks.
      //
      ASSERT (VtableExportSize >= (sizeof (*VtableExport) + ((NumVtables + 1) * sizeof (*VtableExport->Symbols))));

      VtableExport->Symbols[NumVtables] = Symbol;
      ++NumVtables;
    }
  }

  VtableExport->NumSymbols = NumVtables;

  return TRUE;
}

BOOLEAN
InternalCreateVtablesPrelinked64 (
  IN     PRELINKED_CONTEXT      *Context,
  IN OUT PRELINKED_KEXT         *Kext,
  IN  OC_VTABLE_EXPORT_ARRAY    *VtableExport,
  OUT PRELINKED_VTABLE          *VtableBuffer
  )
{
  CONST MACH_NLIST_64 *Symbol;
  UINT32              Index;
  BOOLEAN             Result;

  ASSERT (Kext != NULL);

  for (Index = 0; Index < VtableExport->NumSymbols; ++Index) {
    Symbol = VtableExport->Symbols[Index];
    Result = InternalConstructVtablePrelinked64 (
                Context,
                Kext,
                Symbol,
                VtableBuffer
                );
    if (!Result) {
      return FALSE;
    }
      
    VtableBuffer = GET_NEXT_PRELINKED_VTABLE (VtableBuffer);
  }

  return TRUE;
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
  OUT    MACH_NLIST_64                 *Symbol
  )
{
  CONST CHAR8 *Name;
  INTN        Result;
  BOOLEAN     Success;
  CONST CHAR8 *ClassName;
  CHAR8       FunctionPrefix[SYM_MAX_NAME_LEN];

  ASSERT (ParentEntry != NULL);
  //
  // The child entry can be NULL when a locally-defined, non-external
  // symbol is stripped.  We wouldn't patch this entry anyway, so we
  // just skip it.
  //
  if (Symbol == NULL) {
    return TRUE;
  }
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

  Name = MachoGetSymbolName64 (MachoContext, Symbol);
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
  if (!MachoSymbolIsDefined (Symbol)) {
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
  InternalSolveSymbolValue64 (ParentEntry->Address, Symbol);
  //
  // The C++ ABI requires that functions be aligned on a 2-byte boundary:
  // http://www.codesourcery.com/public/cxx-abi/abi.html#member-pointers
  // If the LSB of any virtual function's link address is 1, then the
  // compiler has violated that part of the ABI, and we're going to panic
  // in _ptmf2ptf() (in OSMetaClass.h). Better to panic here with some
  // context.
  //
  Name = ParentEntry->Name;
  if (!MachoSymbolNameIsPureVirtual (Name) && ((Symbol->Value & 1U) != 0)) {
    DEBUG ((DEBUG_WARN, "Prelink: Invalid VTable symbol\n"));
  }

  return TRUE;
}

STATIC
BOOLEAN
InternalInitializeVtableByEntriesAndRelocations64 (
  IN OUT OC_MACHO_CONTEXT             *MachoContext,
  IN     CONST PRELINKED_VTABLE       *SuperVtable,
  IN     CONST MACH_NLIST_64          *VtableSymbol,
  IN     CONST UINT64                 *VtableData,
  IN     UINT32                       MaxSize
  )
{
  UINT32                     NumEntries;
  UINT32                     EntryOffset;
  UINT64                     EntryValue;
  MACH_NLIST_64              *Symbol;
  BOOLEAN                    Result;

  if (MaxSize < (VTABLE_HEADER_SIZE_64 + VTABLE_ENTRY_SIZE_64)) {
    return FALSE;
  }
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  //
  MaxSize /= VTABLE_ENTRY_SIZE_64;
  for (
    NumEntries = 0, EntryOffset = VTABLE_HEADER_LEN_64;
    TRUE;
    ++NumEntries, ++EntryOffset
    ) {
    EntryValue = VtableData[EntryOffset];
    //
    // If we can't find a symbol, it means it is a locally-defined,
    // non-external symbol that has been stripped.  We don't patch over
    // locally-defined symbols, so we leave the symbol as NULL and just
    // skip it.  We won't be able to patch subclasses with this symbol,
    // but there isn't much we can do about that.
    //
    if (EntryValue == 0) {
      Result = MachoGetSymbolByExternRelocationOffset64 (
                 MachoContext,
                 (VtableSymbol->Value + (EntryOffset * sizeof (*VtableData))),
                 &Symbol
                 );
      if (!Result) {
        //
        // When the VTable entry is 0 and it is not referenced by a Relocation,
        // it is the end of the table.
        //
        break;
      }

      Result = InternalPatchVtableSymbol (
                 MachoContext,
                 &SuperVtable->Entries[NumEntries],
                 MachoGetSymbolName64 (MachoContext, VtableSymbol),
                 Symbol
                 );
      if (!Result) {
        return FALSE;
      }
    }

    if ((EntryOffset + 1) >= MaxSize) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
InternalPatchByVtables64 (
  IN     PRELINKED_CONTEXT         *Context,
  IN OUT PRELINKED_KEXT            *Kext,
  IN     VOID                      *ScratchBuffer
  )
{
  OC_VTABLE_PATCH_ENTRY *Entries;

  OC_MACHO_CONTEXT     *MachoContext;
  CONST MACH_HEADER_64 *MachHeader;
  UINT32               Index;
  UINT32               NumTables;
  UINT32               NumPatched;
  BOOLEAN              Result;
  CONST MACH_NLIST_64  *Smcp;
  CONST CHAR8          *Name;
  UINT32               VtableOffset;
  UINT32               MaxSize;
  CONST MACH_NLIST_64  *VtableSymbol;
  CONST MACH_NLIST_64  *MetaVtableSymbol;
  CONST MACH_NLIST_64  *MetaClass;
  CONST UINT64         *VtableData;
  CONST PRELINKED_VTABLE *SuperVtable;
  CONST PRELINKED_VTABLE *MetaVtable;
  CONST VOID           *OcSymbolDummy;
  MACH_NLIST_64        *SymbolDummy;
  CHAR8                ClassName[SYM_MAX_NAME_LEN];
  CHAR8                SuperClassName[SYM_MAX_NAME_LEN];
  CHAR8                VtableName[SYM_MAX_NAME_LEN];
  CHAR8                SuperVtableName[SYM_MAX_NAME_LEN];
  CHAR8                FinalSymbolName[SYM_MAX_NAME_LEN];
  BOOLEAN              SuccessfulIteration;

  Entries = ScratchBuffer;

  MachoContext = &Kext->Context.MachContext;

  MachHeader = MachoGetMachHeader64 (MachoContext);
  ASSERT (MachHeader != NULL);
  //
  // Retrieve all SMCPs.
  //
  NumTables = 0;

  for (
    Index = 0;
    (Smcp = MachoGetSymbolByIndex64 (MachoContext, Index)) != NULL;
    ++Index
    ) {
    Name = MachoGetSymbolName64 (MachoContext, Smcp);
    if (MachoSymbolNameIsSmcp64 (MachoContext, Name)) {
      //
      // We walk over the super metaclass pointer symbols because classes
      // with them are the only ones that need patching.  Then we double the
      // number of vtables we're expecting, because every pointer will have a
      // class vtable and a MetaClass vtable.
      //
      Result = MachoGetVtableSymbolsFromSmcp64 (
                 MachoContext,
                 Name,
                 &VtableSymbol,
                 &MetaVtableSymbol
                 );
      if (!Result) {
        return FALSE;
      }

      Entries[NumTables].Smcp       = Smcp;
      Entries[NumTables].Vtable     = VtableSymbol;
      Entries[NumTables].MetaVtable = MetaVtableSymbol;
      ++NumTables;
    }
  }
  //
  // Patch via the previously retrieved SMCPs.
  //
  NumPatched   = 0;

  while (NumPatched < NumTables) {
    SuccessfulIteration = FALSE;

    for (Index = 0; Index < NumTables; ++Index) {
      Smcp = Entries[Index].Smcp;
      if (Smcp == NULL) {
        continue;
      }

      Name = MachoGetSymbolName64 (MachoContext, Smcp);
      //
      // We walk over the super metaclass pointer symbols because classes
      // with them are the only ones that need patching.  Then we double the
      // number of vtables we're expecting, because every pointer will have a
      // class vtable and a MetaClass vtable.
      //
      ASSERT (MachoSymbolNameIsSmcp64 (MachoContext, Name));
      VtableSymbol     = Entries[Index].Vtable;
      MetaVtableSymbol = Entries[Index].MetaVtable;
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
      MetaClass = MachoGetMetaclassSymbolFromSmcpSymbol64 (
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
                 MachoGetSymbolName64 (MachoContext, MetaClass),
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
                      SuperVtableName,
                      0
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
                        (UINTN)FinalSymbolName,
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
      Result = MachoSymbolGetFileOffset64 (
                 MachoContext,
                 VtableSymbol,
                 &VtableOffset,
                 &MaxSize
                 );
      if (!Result || (MaxSize < (VTABLE_HEADER_SIZE_64 + VTABLE_ENTRY_SIZE_64))) {
        return FALSE;
      }

      VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
      if (!OC_ALIGNED (VtableData)) {
        return FALSE;
      }

      Result = InternalInitializeVtableByEntriesAndRelocations64 (
                 MachoContext,
                 SuperVtable,
                 VtableSymbol,
                 VtableData,
                 MaxSize
                 );
      if (!Result) {
        return FALSE;
      }
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
                     VtableName,
                     0
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
                      OS_METACLASS_VTABLE_NAME,
                      0
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
      Result = MachoSymbolGetFileOffset64 (
                 MachoContext,
                 MetaVtableSymbol,
                 &VtableOffset,
                 &MaxSize
                 );
      if (!Result || (MaxSize < (VTABLE_HEADER_SIZE_64 + VTABLE_ENTRY_SIZE_64))) {
        return FALSE;
      }

      VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
      if (!OC_ALIGNED (VtableData)) {
        return FALSE;
      }

      Result = InternalInitializeVtableByEntriesAndRelocations64 (
                 MachoContext,
                 SuperVtable,
                 MetaVtableSymbol,
                 VtableData,
                 MaxSize
                 );
      if (!Result) {
        return FALSE;
      }

      Entries[Index].Smcp = NULL;

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

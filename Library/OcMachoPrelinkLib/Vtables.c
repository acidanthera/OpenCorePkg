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

#include "OcMachoPrelinkInternal.h"

STATIC
CONST OC_VTABLE *
InternalGetOcVtableByName (
  IN CONST OC_VTABLE_ARRAY  *Vtables,
  IN CONST CHAR8            *Name
  )
{
  CONST OC_VTABLE_ARRAY *VtableWalker;
  CONST OC_VTABLE       *Vtable;
  UINT32                Index;
  INTN                  Result;

  VtableWalker = Vtables;

  do {
    Vtable = GET_FIRST_OC_VTABLE (VtableWalker);

    for (Index = 0; Index < VtableWalker->NumVtables; ++Index) {
      Result = AsciiStrCmp (Vtable->Name, Name);
      if (Result == 0) {
        return Vtable;
      }

      Vtable = GET_NEXT_OC_VTABLE (Vtable);
    }

    VtableWalker = GET_OC_VTABLE_ARRAY_FROM_LINK (
                     GetNextNode (&Vtables->Link, &VtableWalker->Link)
                     );
  } while (!IsNull (&Vtables->Link, &VtableWalker->Link));

  return NULL;
}

STATIC
BOOLEAN
InternalConstructVtablePrelinked64 (
  IN OUT OC_MACHO_CONTEXT          *MachoContext,
  IN     CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  IN     CONST MACH_NLIST_64       *VtableSymbol,
  OUT    OC_VTABLE                 *Vtable
  )
{
  BOOLEAN                  Result;
  CONST MACH_HEADER_64     *MachHeader;
  UINT32                   VtableOffset;
  CONST UINT64             *VtableData;
  UINT64                   Value;
  CONST OC_SYMBOL_TABLE_64 *SymbolsWalker;
  CONST CHAR8              *StringTable;
  UINT32                   Index;
  UINT32                   Index2;
  UINT32                   CxxIndex;
  CONST OC_SYMBOL_64       *Symbol;

  ASSERT (MachoContext != NULL);
  ASSERT (DefinedSymbols != NULL);
  ASSERT (VtableSymbol != NULL);
  ASSERT (Vtable != NULL);

  MachHeader = MachoGetMachHeader64 (MachoContext);
  ASSERT (MachHeader != NULL);

  Result = MachoSymbolGetFileOffset64 (
             MachoContext,
             VtableSymbol,
             &VtableOffset
             );
  if (!Result) {
    return FALSE;
  }

  VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
  if (!OC_ALIGNED (VtableData)) {
    return FALSE;
  }

  Vtable->Name = MachoGetSymbolName64 (MachoContext, VtableSymbol);
  //
  // Initialize the VTable by entries.
  //
  SymbolsWalker = DefinedSymbols;

  do {
    StringTable = SymbolsWalker->StringTable;
    CxxIndex = (
                 SymbolsWalker->NumSymbols
                   - SymbolsWalker->NumCxxSymbols
               );
    //
    // Assumption: Not ARM (ARM requires an alignment to the function pointer
    //             retrieved from VtableData.
    //
    for (
      Index = VTABLE_HEADER_LEN_64;
      (Value = VtableData[Index]) != 0;
      ++Index
      ) {
      for (
        Index2 = CxxIndex;
        Index2 < SymbolsWalker->NumSymbols;
        ++Index2
        ) {
        Symbol = &SymbolsWalker->Symbols[Index2];

        if (Symbol->Value == Value) {
          Vtable->Entries[Index].Address = Value;
          Vtable->Entries[Index].Name    = (StringTable + Symbol->StringIndex);
          break;
        }
      }
      //
      // If we can't find the symbol, it means that the virtual function was
      // defined inline.  There's not much I can do about this; it just means
      // I can't patch this function.
      //
      if (Index == SymbolsWalker->NumCxxSymbols) {
        Vtable->Entries[Index].Address = 0;
        Vtable->Entries[Index].Name    = NULL;
      }
    }

    SymbolsWalker = GET_OC_SYMBOL_TABLE_64_FROM_LINK (
                      GetNextNode (
                        &DefinedSymbols->Link,
                        &SymbolsWalker->Link
                        )
                      );
  } while (!IsNull (&DefinedSymbols->Link, &SymbolsWalker->Link));

  return TRUE;
}

UINT32
InternalGetVtableSize64 (
  IN CONST UINT64  *VtableData
  )
{
  UINT32 Index;

  ASSERT (VtableData != NULL);
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  //
  for (Index = VTABLE_HEADER_LEN_64; VtableData[Index] != 0; ++Index) {
    ;
  }

  return (Index * VTABLE_ENTRY_SIZE_64);
}

BOOLEAN
InternalGetVtableSizeWithRelocs64 (
  IN OUT OC_MACHO_CONTEXT  *MachoContext,
  IN     CONST UINT64      *VtableData,
  OUT    UINT32            *VtableSize
  )
{
  UINT32        Size;
  MACH_NLIST_64 *Symbol;

  Size = InternalGetVtableSize64 (VtableData);

  Symbol = MachoGetSymbolByExternRelocationOffset64 (
             MachoContext,
             ((UINTN)VtableData + Size)
             );
  if (Symbol == NULL) {
    return FALSE;
  }

  *VtableSize = Size;

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
  BOOLEAN             Result;

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
                 NULL
                 );
  for (Index = 0; Index < NumSymbols; ++Index) {
    Symbol = &SymbolTable[Index];
    Name = MachoGetSymbolName64 (MachoContext, Symbol);
    if (MachoSymbolNameIsVtable64 (Name)) {
      Result = MachoIsSymbolValueInRange64 (
                 MachoContext,
                 VtableExport->Symbols[Index]
                 );
      if (!Result) {
        return FALSE;
      }

      ++NumVtables;

      if (VtableExportSize < (NumVtables * sizeof (*VtableExport))) {
        ASSERT (FALSE);
        return FALSE;
      }

      VtableExport->Symbols[NumVtables] = Symbol;
    }
  }

  VtableExport->NumSymbols = NumVtables;

  return TRUE;
}

BOOLEAN
InternalCreateVtablesPrelinked64 (
  IN  OC_MACHO_CONTEXT          *MachoContext,
  IN  CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  IN  OC_VTABLE_EXPORT_ARRAY    *VtableExport,
  OUT OC_VTABLE                 *VtableBuffer
  )
{
  CONST MACH_NLIST_64 *Symbol;
  UINT32              Index;
  BOOLEAN             Result;

  ASSERT (MachoContext != NULL);
  ASSERT (DefinedSymbols != NULL);

  for (Index = 0; Index < VtableExport->NumSymbols; ++Index) {
    Symbol = VtableExport->Symbols[Index];
    Result = InternalConstructVtablePrelinked64 (
                MachoContext,
                DefinedSymbols,
                Symbol,
                VtableBuffer
                );
    if (!Result) {
      return FALSE;
    }
      
    VtableBuffer = GET_NEXT_OC_VTABLE (VtableBuffer);
  }

  return TRUE;
}

/**
 * kxld_vtable_patch
 */
STATIC
BOOLEAN
InternalPatchVtableSymbol (
  IN OUT OC_MACHO_CONTEXT       *MachoContext,
  IN     CONST OC_VTABLE_ENTRY  *ParentEntry,
  IN     CONST CHAR8            *VtableName,
  OUT    MACH_NLIST_64          *Symbol
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
    return MachoIsSymbolValueInRange64 (MachoContext, Symbol);
  }
  //
  // 1) If the symbol is defined locally, do not patch
  //
  if (MachoSymbolIsLocalDefined (MachoContext, Symbol)) {
    return MachoIsSymbolValueInRange64 (MachoContext, Symbol);
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
    return MachoIsSymbolValueInRange64 (MachoContext, Symbol);
  }
  //
  // 3) If the symbols are the same, do not patch
  //
  Result = AsciiStrCmp (Name, ParentEntry->Name);
  if (Result == 0) {
    return MachoIsSymbolValueInRange64 (MachoContext, Symbol);
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
      ASSERT (FALSE);
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
  ASSERT (MachoSymbolNameIsPureVirtual (Name) || ((Symbol->Value & 1U) == 0));

  return TRUE;
}

STATIC
BOOLEAN
InternalInitializeVtableByEntriesAndRelocations64 (
  IN OUT OC_MACHO_CONTEXT             *MachoContext,
  IN     CONST OC_SYMBOL_TABLE_64     *DefinedSymbols,
  IN     CONST OC_VTABLE              *SuperVtable,
  IN     CONST MACH_NLIST_64          *VtableSymbol,
  IN     CONST UINT64                 *VtableData,
  OUT    OC_VTABLE                    *Vtable
  )
{
  UINT32                     NumEntries;
  UINT32                     Index;
  UINT32                     CxxIndex;
  UINT32                     EntryOffset;
  UINT64                     EntryValue;
  CONST OC_SYMBOL_TABLE_64   *SymbolsWalker;
  CONST OC_SYMBOL_64         *OcSymbol;
  CONST CHAR8                *StringTable;
  MACH_NLIST_64              *Symbol;
  BOOLEAN                    Result;
  CONST CHAR8                *Name;
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  //
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
    if (EntryValue != 0) {
      SymbolsWalker = DefinedSymbols;
      CxxIndex = (
                   SymbolsWalker->NumSymbols
                     - SymbolsWalker->NumCxxSymbols
                 );
      do {
        StringTable = SymbolsWalker->StringTable;
        //
        // Imported symbols are implicitely locally defined and hence do not
        // need to be patched.
        //
        for (
          Index = CxxIndex;
          Index < SymbolsWalker->NumSymbols;
          ++Index
          ) {
          OcSymbol = &SymbolsWalker->Symbols[Index];

          if (OcSymbol->Value == EntryValue) {
            Name = (StringTable + OcSymbol->StringIndex);
            Vtable->Entries[NumEntries].Name    = Name;
            Vtable->Entries[NumEntries].Address = OcSymbol->Value;
            break;
          }
        }

        if (Index != SymbolsWalker->NumSymbols) {
          break;
        }

        SymbolsWalker = GET_OC_SYMBOL_TABLE_64_FROM_LINK (
                          GetNextNode (
                            &DefinedSymbols->Link,
                            &SymbolsWalker->Link
                            )
                          );
        if (IsNull (&DefinedSymbols->Link, &SymbolsWalker->Link)) {
          ASSERT (FALSE);
          return FALSE;
        }
      } while (TRUE);
    } else if (NumEntries < SuperVtable->NumEntries) {
      Symbol = MachoGetSymbolByExternRelocationOffset64 (
                 MachoContext,
                 (VtableSymbol->Value + EntryOffset)
                 );

      if (Symbol == NULL) {
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

      Name = MachoGetSymbolName64 (MachoContext, Symbol);
      Vtable->Entries[NumEntries].Name    = Name;
      Vtable->Entries[NumEntries].Address = Symbol->Value;
    }
  }

  Vtable->NumEntries = NumEntries;

  return TRUE;
}

BOOLEAN
InternalPrepareVtableCreationNonPrelinked64 (
  IN OUT OC_MACHO_CONTEXT       *MachoContext,
  IN     UINT32                 NumSymbols,
  IN     CONST MACH_NLIST_64    *SymbolTable,
  OUT    OC_VTABLE_PATCH_ARRAY  *PatchData
  )
{
  CONST MACH_HEADER_64 *MachHeader;
  UINT32               Index;
  UINT32               NumTables;
  BOOLEAN              Result;
  CONST MACH_NLIST_64  *Smcp;
  CONST CHAR8          *Name;
  CONST MACH_NLIST_64  *VtableSymbol;
  CONST MACH_NLIST_64  *MetaVtableSymbol;

  MachHeader = MachoGetMachHeader64 (MachoContext);
  ASSERT (MachHeader != NULL);

  NumTables = 0;

  for (Index = 0; Index < NumSymbols; ++Index) {
    Smcp = &SymbolTable[Index];
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

      PatchData[NumTables].Entries->Smcp       = Smcp;
      PatchData[NumTables].Entries->Vtable     = VtableSymbol;
      PatchData[NumTables].Entries->MetaVtable = MetaVtableSymbol;
      ++NumTables;
    }
  }

  PatchData->NumEntries = NumTables;

  return TRUE;
}

BOOLEAN
InternalCreateVtablesNonPrelinked64 (
  IN OUT OC_MACHO_CONTEXT          *MachoContext,
  IN     CONST OC_DEPENDENCY_DATA  *DependencyData,
  IN     OC_VTABLE_PATCH_ARRAY     *PatchData,
  OUT    OC_VTABLE_ARRAY           *VtableArray
  )
{
  OC_VTABLE            *VtableBuffer;
  CONST MACH_HEADER_64 *MachHeader;
  UINT32               Index;
  UINT32               NumPatched;
  BOOLEAN              Result;
  CONST MACH_NLIST_64  *Smcp;
  CONST CHAR8          *Name;
  UINT32               VtableOffset;
  CONST MACH_NLIST_64  *VtableSymbol;
  CONST MACH_NLIST_64  *MetaVtableSymbol;
  CONST MACH_NLIST_64  *MetaClass;
  CONST UINT64         *VtableData;
  CONST OC_VTABLE      *SuperVtable;
  CONST OC_VTABLE      *MetaVtable;
  CONST OC_SYMBOL_64   *OcSymbolDummy;
  MACH_NLIST_64        *SymbolDummy;
  CHAR8                ClassName[SYM_MAX_NAME_LEN];
  CHAR8                SuperClassName[SYM_MAX_NAME_LEN];
  CHAR8                VtableName[SYM_MAX_NAME_LEN];
  CHAR8                SuperVtableName[SYM_MAX_NAME_LEN];
  CHAR8                FinalSymbolName[SYM_MAX_NAME_LEN];
  BOOLEAN              SuccessfulIteration;

  MachHeader = MachoGetMachHeader64 (MachoContext);
  ASSERT (MachHeader != NULL);

  VtableBuffer = GET_FIRST_OC_VTABLE (VtableArray);
  NumPatched   = 0;

  while (NumPatched < PatchData->NumEntries) {
    SuccessfulIteration = FALSE;

    for (Index = 0; Index < PatchData->NumEntries; ++Index) {
      Smcp = PatchData->Entries[Index].Smcp;
      Name = MachoGetSymbolName64 (MachoContext, Smcp);
      //
      // We walk over the super metaclass pointer symbols because classes
      // with them are the only ones that need patching.  Then we double the
      // number of vtables we're expecting, because every pointer will have a
      // class vtable and a MetaClass vtable.
      //
      ASSERT (MachoSymbolNameIsSmcp64 (MachoContext, Name));
      VtableSymbol     = PatchData->Entries[Index].Vtable;
      MetaVtableSymbol = PatchData->Entries[Index].MetaVtable;
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
      //
      // Get the super vtable if it's been patched
      //
      SuperVtable = InternalGetOcVtableByName (
                      DependencyData->Vtables,
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
      OcSymbolDummy = InternalOcGetSymbolByName (
                        DependencyData->SymbolTable,
                        FinalSymbolName,
                        TRUE
                        );
      if (OcSymbolDummy != NULL) {
        return FALSE;
      }

      Result = MachoGetLocalDefinedSymbolByName (
                  MachoContext,
                  FinalSymbolName,
                  &SymbolDummy
                  );
      if (!Result || (SymbolDummy != NULL)) {
        return FALSE;
      }
      //
      // Patch the class's vtable
      //
      Result = MachoSymbolGetFileOffset64 (
                 MachoContext,
                 VtableSymbol,
                 &VtableOffset
                 );
      if (!Result) {
        return FALSE;
      }

      VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
      if (!OC_ALIGNED (VtableData)) {
        return FALSE;
      }

      Result = InternalInitializeVtableByEntriesAndRelocations64 (
                 MachoContext,
                 DependencyData->SymbolTable,
                 SuperVtable,
                 VtableSymbol,
                 VtableData,
                 VtableBuffer
                 );
      if (!Result) {
        return FALSE;
      }

      VtableBuffer->Name = MachoGetSymbolName64 (MachoContext, VtableSymbol);
      //
      // Add the class's vtable to the set of patched vtables
      // This is done implicitely as we're operating on an array.
      //
      VtableBuffer = GET_NEXT_OC_VTABLE (VtableBuffer);
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
                     DependencyData->Vtables,
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
                      DependencyData->Vtables,
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
      Result = MachoSymbolGetFileOffset64 (
                 MachoContext,
                 MetaVtableSymbol,
                 &VtableOffset
                 );
      if (!Result) {
        return FALSE;
      }

      VtableData = (UINT64 *)((UINTN)MachHeader + VtableOffset);
      if (!OC_ALIGNED (VtableData)) {
        return FALSE;
      }

      Result = InternalInitializeVtableByEntriesAndRelocations64 (
                 MachoContext,
                 DependencyData->SymbolTable,
                 SuperVtable,
                 MetaVtableSymbol,
                 VtableData,
                 VtableBuffer
                 );
      if (!Result) {
        return FALSE;
      }

      VtableBuffer->Name = MachoGetSymbolName64 (MachoContext, MetaVtableSymbol);
      //
      // Add the MetaClass's vtable to the set of patched vtables
      // This is done implicitely as we're operating on an array.
      //
      VtableBuffer = GET_NEXT_OC_VTABLE (VtableBuffer);

      ++NumPatched;
      SuccessfulIteration = TRUE;
    }
    //
    // Exit when there are unpatched VTables left, but there are none patched
    // in a full iteration.
    //
    if (!SuccessfulIteration) {
      ASSERT (FALSE);
      return FALSE;
    }
  }

  VtableArray->NumVtables = (NumPatched * 2);

  return TRUE;
}

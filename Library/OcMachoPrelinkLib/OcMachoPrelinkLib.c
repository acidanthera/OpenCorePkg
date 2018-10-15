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

#include <Uefi.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMachoLib.h>

#define KXLD_WEAK_TEST_SYMBOL  "_gOSKextUnresolved"

#define OS_METACLASS_VTABLE_NAME "__ZTV11OSMetaClass"

#define X86_64_RIP_RELATIVE_LIMIT  0x80000000ULL

#define SYM_MAX_NAME_LEN  256U

#define VTABLE_ENTRY_SIZE_64   8U
#define VTABLE_HEADER_LEN_64   2U
#define VTABLE_HEADER_SIZE_64  (VTABLE_HEADER_LEN_64 * VTABLE_ENTRY_SIZE_64)

typedef struct {
  UINT32 StringIndex;  ///< index into the string table
  UINT64 Value;        ///< value of this symbol (or stab offset)
} OC_SYMBOL_64;

#define OC_SYMBOL_TABLE_64_SIGNATURE  SIGNATURE_32 ('O', 'S', '6', '4')

/**
  Gets the next element in a linked list of OC_SYMBOL_TABLE_64.

  @param[in] This  The current ListEntry.

**/
#define GET_OC_SYMBOL_TABLE_64_FROM_LINK(This)  \
  CR (                                          \
    (This),                                     \
    OC_SYMBOL_TABLE_64,                         \
    Link,                                       \
    OC_SYMBOL_TABLE_64_SIGNATURE                \
    )

typedef struct {
  ///
  /// These data are used to construct linked lists of dependency information
  /// for each KEXT.  It is declared hear for every dependency will
  /// eventually be part of a list and to save separate allocations per KEXT.
  ///
  UINT32       Signature;
  LIST_ENTRY   Link;
  BOOLEAN      IsIndirect;
  ///
  /// The String Table associated with this symbol table.
  ///
  CONST CHAR8  *StringTable;
  ///
  /// The number of symbols in the entire symbols buffer.
  ///
  UINTN        NumberOfSymbols;
  ///
  /// The number of C++ symbols at the end of the symbols buffer.
  ///
  UINTN        NumberOfCxxSymbols;
  OC_SYMBOL_64 Symbols[];  ///< The symbol buffer.
} OC_SYMBOL_TABLE_64;

typedef struct {
  CONST CHAR8 *Name;    ///< The symbol's name.
  UINT64      Address;  ///< The symbol's address.
} OC_VTABLE_ENTRY;

#define GET_NEXT_OC_VTABLE(This)  \
  ((OC_VTABLE *)(&(This)->Entries[(This)->NumberOfEntries]))

typedef struct {
  CONST CHAR8     *Name;            ///< The VTable's name.
  UINTN           NumberOfEntries;  ///< The number of VTable entries.
  OC_VTABLE_ENTRY Entries[];        ///< The VTable entries.
} OC_VTABLE;

#define OC_VTABLE_ARRAY_SIGNATURE  SIGNATURE_32 ('O', 'V', 'T', 'A')

/**
  Gets the next element in a linked list of OC_VTABLE_ARRAY.

  @param[in] This  The current ListEntry.

**/
#define GET_OC_VTABLE_ARRAY_FROM_LINK(This)  \
  CR (                                       \
    (This),                                  \
    OC_VTABLE_ARRAY,                         \
    Link,                                    \
    OC_VTABLE_ARRAY_SIGNATURE                \
    )

#define GET_FIRST_OC_VTABLE(This)  \
  ((OC_VTABLE *)((This) + 1))

typedef struct {
  ///
  /// These data are used to construct linked lists of dependency information
  /// for each KEXT.  It is declared hear for every dependency will
  /// eventually be part of a list and to save separate allocations per KEXT.
  ///
  UINT32     Signature;
  LIST_ENTRY Link;
  ///
  /// The number of VTables in the array.
  ///
  UINTN      NumberOfVtables;
} OC_VTABLE_ARRAY;

typedef union {
  struct {
    UINT32 Major      : 14;
    UINT32 Minor      : 7;
    UINT32 Revision   : 7;
    UINT32 Stage      : 4;
    UINT32 StageLevel : 8;
  }      Bits;
  UINT64 Value;
} OC_KEXT_VERSION;

typedef enum {
  OcKextVersionStageDevelopment = 1,
  OcKextVersionStageAlpha       = 3,
  OcKextVersionStageBeta        = 5,
  OcKextVersionStageCandidate   = 7,
  OcKextVersionStageRelease     = 9
} OC_KEXT_VERSION_STAGE;

typedef struct {
  OC_SYMBOL_TABLE_64 *SymbolTable;
  OC_VTABLE_ARRAY    *Vtables;
} OC_DEPENDENCY_DATA;

#define OC_DEPENDENCY_INFO_ENTRY_SIGNATURE  \
  SIGNATURE_32 ('O', 'D', 'I', 'E')

typedef struct OC_DEPENDENCY_INFO_ENTRY_ OC_DEPENDENCY_INFO_ENTRY;

typedef struct {
  CONST CHAR8              *Name;
  OC_KEXT_VERSION          Version;
  OC_KEXT_VERSION          CompatibleVersion;
  UINTN                    NumberOfDependencies;
  OC_DEPENDENCY_INFO_ENTRY *Dependencies[];
} OC_DEPENDENCY_INFO;

struct OC_DEPENDENCY_INFO_ENTRY_ {
  UINT32             Signature;
  LIST_ENTRY         Link;
  OC_DEPENDENCY_DATA Data;
  OC_DEPENDENCY_INFO Info;
};

typedef struct {
  CONST CHAR8             *Plist;
  CONST EFI_FILE_PROTOCOL *Directory;
  //
  // Private data.
  //
  CONST VOID              *Private;
} OC_KEXT_REQUEST;

//
// Symbols
//

/**
  Patches Symbol with Value and marks it as solved.

  @param[in]  Value   The value to solve Symbol with.
  @param[out] Symbol  The symbol to solve.

**/
STATIC
VOID
InternalSolveSymbolValue64 (
  IN  UINT64         Value,
  OUT MACH_NLIST_64  *Symbol
  )
{
  Symbol->Value   = Value;
  Symbol->Type    = (MACH_N_TYPE_ABS | MACH_N_TYPE_EXT);
  Symbol->Section = NO_SECT;
}

STATIC
CONST OC_SYMBOL_64 *
InternalOcGetSymbolByName (
  IN CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  IN CONST CHAR8               *Name,
  IN BOOLEAN                   CheckIndirect
  )
{
  CONST OC_SYMBOL_TABLE_64 *SymbolsWalker;
  CONST OC_SYMBOL_64       *Symbols;
  UINTN                    NumberOfSymbols;
  UINTN                    CxxIndex;
  UINTN                    Index;
  INTN                     Result;

  SymbolsWalker = DefinedSymbols;

  do {
    Symbols         = SymbolsWalker->Symbols;
    NumberOfSymbols = SymbolsWalker->NumberOfSymbols;

    if (SymbolsWalker->IsIndirect) {
      if (!CheckIndirect) {
        continue;
      }
      //
      // Only consider C++ symbols for indirect dependencies.
      //
      CxxIndex = (
                   SymbolsWalker->NumberOfSymbols
                     - SymbolsWalker->NumberOfCxxSymbols
                 );
      Symbols         = &Symbols[CxxIndex];
      NumberOfSymbols = SymbolsWalker->NumberOfCxxSymbols;
    }

    for (Index = 0; Index < NumberOfSymbols; ++Index) {
      Result = AsciiStrCmp (
                 Name,
                 (SymbolsWalker->StringTable + Symbols[Index].StringIndex)
                 );
      if (Result == 0) {
        return &Symbols[Index];
      }
    }

    SymbolsWalker = GET_OC_SYMBOL_TABLE_64_FROM_LINK (
                      GetNextNode (
                        &DefinedSymbols->Link,
                        &SymbolsWalker->Link
                        )
                      );
  } while (!IsNull (&DefinedSymbols->Link, &SymbolsWalker->Link));

  return NULL;
}

/**
  Worker function to solve Symbol against the specified DefinedSymbols.
  It does not consider Symbol might be a weak reference.

  @param[in]     DefinedSymbols  List of defined symbols of all dependencies.
  @param[in]     Name            The name of the symbol to resolve against.
  @param[in,out] Symbol          The symbol to be resolved.
  @param[in]     Strings         The String Table associated with Symbol.

  @retval  Returned is whether the symbol was solved successfully.

**/
STATIC
BOOLEAN
InternalSolveSymbolNonWeak64 (
  IN     CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  IN     CONST CHAR8               *Name,
  IN OUT MACH_NLIST_64             *Symbol,
  IN     CONST CHAR8               *Strings
  )
{
  INTN               Result;
  CONST OC_SYMBOL_64 *ResolveSymbol;

  if (Symbol->Type != MACH_N_TYPE_UNDF) {
    if (Symbol->Type != MACH_N_TYPE_INDR) {
      //
      // KXLD_WEAK_TEST_SYMBOL might have been resolved by the resolving code
      // at the end of InternalSolveSymbol64. 
      //
      Result = AsciiStrCmp (
                 (Strings + Symbol->UnifiedName.StringIndex),
                 KXLD_WEAK_TEST_SYMBOL
                 );
      if (Result == 0) {
        //
        // KXLD_WEAK_TEST_SYMBOL has been solved successfully already.
        //
        return TRUE;
      }
      //
      // Any other symbols must be undefined or indirect.
      //
      ASSERT (FALSE);
    }
  } else if (Symbol->Value != 0) {
    //
    // Common symbols are not supported.
    //
    ASSERT (FALSE);
    return FALSE;
  }

  ResolveSymbol = InternalOcGetSymbolByName (
                    DefinedSymbols,
                    Name,
                    FALSE
                    );
  if (ResolveSymbol != NULL) {
    InternalSolveSymbolValue64 (ResolveSymbol->Value, Symbol);
    return TRUE;
  }

  return FALSE;
}

/**
  Solves Symbol against the specified DefinedSymbols.

  @param[in]     DefinedSymbols   List of defined symbols of all dependencies.
  @param[in]     SymbolTable      The KEXT's Symbol Table.
  @param[in]     StringTable      The KEXT's String Table.
  @param[in]     DySymtab         The DYSYMTAB Load Command of the KEXT.
  @param[in]     Name             The name of the symbol to resolve against.
  @param[in,out] Symbol           The symbol to be resolved.

  @retval  Returned is whether the symbol was solved successfully.  For weak
           symbols, this includes solving with _gOSKextUnresolved.

**/
STATIC
BOOLEAN
InternalSolveSymbol64 (
  IN     CONST OC_SYMBOL_TABLE_64     *DefinedSymbols,
  IN     CONST MACH_NLIST_64          *SymbolTable,
  IN     CONST CHAR8                  *StringTable,
  IN     CONST MACH_DYSYMTAB_COMMAND  *DySymtab,
  IN     CONST CHAR8                  *Name,
  IN OUT MACH_NLIST_64                *Symbol
  )
{
  BOOLEAN             Success;
  CONST MACH_NLIST_64 *Symbols;
  UINTN               Index;
  INTN                Result;
  CONST MACH_NLIST_64 *WeakTestSymbol;

  ASSERT (Symbol != NULL);
  ASSERT ((Symbol->Type & MACH_N_TYPE_STAB) == 0);

  Success = InternalSolveSymbolNonWeak64 (
              DefinedSymbols,
              Name,
              Symbol,
              StringTable
              );
  if (Success) {
    return TRUE;
  }

  if (((Symbol->Type & MACH_N_TYPE_STAB) == 0)
   && ((Symbol->Descriptor & MACH_N_WEAK_DEF) != 0)) {
    //
    // KXLD_WEAK_TEST_SYMBOL is not going to defined or exposed by a KEXT
    // prelinked by this library, hence only check the undefined symbols region
    // for matches.
    //
    Symbols = &SymbolTable[DySymtab->UndefinedSymbolsIndex];

    for (Index = 0; Index < DySymtab->NumberOfUndefinedSymbols; ++Index) {
      WeakTestSymbol = &Symbols[Index];
      Result = AsciiStrCmp (
                 (StringTable + WeakTestSymbol->UnifiedName.StringIndex),
                 KXLD_WEAK_TEST_SYMBOL
                 );
      if (Result == 0) {
        if (WeakTestSymbol->Type == MACH_N_TYPE_UNDF) {
          Success = InternalSolveSymbolNonWeak64 (
                      DefinedSymbols,
                      Name,
                      Symbol,
                      StringTable
                      );
          if (!Success) {
            return FALSE;
          }
        }

        InternalSolveSymbolValue64 (WeakTestSymbol->Value, Symbol);
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Fills SymbolTable with the symbols provided in Symbols.  For performance
  reasons, the C++ symbols are continuously added to the top of the buffer.
  Their order is not preserved.  SymbolTable->SymbolTable is expected to be
  a valid buffer that can store at least NumberOfSymbols symbols.

  @param[in]     NumberOfSymbols  The number of symbols to copy.
  @param[in]     Symbols          The source symbol array.
  @param[in]     Strings          The String Table associated with Symbols.
  @param[in,out] SymbolTable      The desination Symbol List.  Must be able to
                                  hold at least NumberOfSymbols symbols.

**/
STATIC
VOID
InternalFillSymbolTable64 (
  IN     UINTN                NumberOfSymbols,
  IN     CONST MACH_NLIST_64  *Symbols,
  IN     CONST CHAR8          *Strings,
  IN OUT OC_SYMBOL_TABLE_64   *SymbolTable
  )
{
  OC_SYMBOL_64        *WalkerBottom;
  OC_SYMBOL_64        *WalkerTop;
  UINTN               NumberOfCxxSymbols;
  UINTN               Index;
  CONST MACH_NLIST_64 *Symbol;
  CONST CHAR8         *Name;
  BOOLEAN             Result;

  ASSERT (NumberOfSymbols > 0);
  ASSERT (Symbols != NULL);
  ASSERT (Strings != NULL);

  WalkerBottom = SymbolTable->Symbols;
  WalkerTop    = (WalkerBottom + (SymbolTable->NumberOfSymbols - 1));

  NumberOfCxxSymbols = 0;

  for (Index = 0; Index < NumberOfSymbols; ++Index) {
    Symbol = &Symbols[Index];
    Name   = (Strings + Symbol->UnifiedName.StringIndex);
    Result = MachoIsSymbolNameCxx (Name);

    if (!Result) {
      WalkerBottom->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerBottom->Value       = Symbol->Value;
      ++WalkerBottom;
    } else {
      WalkerTop->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerTop->Value       = Symbol->Value;
      --WalkerTop;

      ++NumberOfCxxSymbols;
    }
  }

  SymbolTable->NumberOfSymbols    = NumberOfSymbols;
  SymbolTable->NumberOfCxxSymbols = NumberOfCxxSymbols;
}

//
// VTables
//

/**
 * kxld_vtable_patch
 */
STATIC
BOOLEAN
InternalPatchVtableSymbol (
  IN  CONST MACH_HEADER_64         *MachHeader,
  IN  CONST MACH_NLIST_64          *SymbolTable,
  IN  CONST MACH_DYSYMTAB_COMMAND  *DySymtab,
  IN  CONST CHAR8                  *StringTable,
  IN  CONST OC_VTABLE_ENTRY        *ParentEntry,
  IN  CONST CHAR8                  *VtableName,
  OUT MACH_NLIST_64                *Symbol
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
    return TRUE;
  }
  //
  // 1) If the symbol is defined locally, do not patch
  //
  Success = MachoSymbolIsLocalDefined (
              MachHeader,
              SymbolTable,
              DySymtab,
              Symbol
              );
  if (!Success) {
    return TRUE;
  }

  Name = (StringTable + Symbol->UnifiedName.StringIndex);
  //
  // 2) If the child is a pure virtual function, do not patch.
  // In general, we want to proceed with patching when the symbol is 
  // externally defined because pad slots fall into this category.
  // The pure virtual function symbol is special case, as the pure
  // virtual property itself overrides the parent's implementation.
  //
  if (MachoIsSymbolNamePureVirtual (Name)) {
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
  if (MachoIsSymbolNamePadslot (ParentEntry->Name)) {
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
  ASSERT (MachoIsSymbolNamePureVirtual (Name) || ((Symbol->Value & 1U) == 0));

  return TRUE;
}

STATIC
BOOLEAN
InternalInitializeVtableByEntriesAndRelocations64 (
  IN  CONST MACH_HEADER_64         *MachHeader,
  IN  CONST OC_SYMBOL_TABLE_64     *DefinedSymbols,
  IN  MACH_NLIST_64                *Symbols,
  IN  CONST CHAR8                  *Strings,
  IN  CONST MACH_DYSYMTAB_COMMAND  *DySymtab,
  IN  CONST OC_VTABLE              *SuperVtable,
  IN  CONST MACH_NLIST_64          *VtableSymbol,
  IN  CONST UINT64                 *VtableData,
  OUT OC_VTABLE                    *Vtable
  )
{
  UINTN                      NumberOfEntries;
  UINTN                      Index;
  UINTN                      EntryOffset;
  UINT64                     EntryValue;
  CONST OC_SYMBOL_TABLE_64   *SymbolsWalker;
  CONST OC_SYMBOL_64         *OcSymbol;
  CONST CHAR8                *StringTable;
  MACH_NLIST_64              *Symbol;
  CONST MACH_RELOCATION_INFO *Relocation;
  BOOLEAN                    Result;
  CONST CHAR8                *Name;

  NumberOfEntries = 0;
  EntryOffset     = VTABLE_HEADER_LEN_64;
  //
  // Assumption: Not ARM (ARM requires an alignment to the function pointer
  //             retrieved from VtableData.
  //
  while (TRUE) {
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

      do {
        StringTable = SymbolsWalker->StringTable;
        //
        // Imported symbols are implicitely locally defined and hencefor do not
        // need to be patched.
        //
        for (
          Index = (
                    SymbolsWalker->NumberOfSymbols
                      - SymbolsWalker->NumberOfCxxSymbols
                  );
          Index < SymbolsWalker->NumberOfSymbols;
          ++Index
          ) {
          OcSymbol = &SymbolsWalker->Symbols[Index];

          if (OcSymbol->Value == EntryValue) {
            Name = (StringTable + OcSymbol->StringIndex);
            Vtable->Entries[NumberOfEntries].Name    = Name;
            Vtable->Entries[NumberOfEntries].Address = OcSymbol->Value;
            break;
          }
        }

        if (Index == SymbolsWalker->NumberOfSymbols) {
          ASSERT (FALSE);
          return FALSE;
        }

        SymbolsWalker = GET_OC_SYMBOL_TABLE_64_FROM_LINK (
                          GetNextNode (
                            &DefinedSymbols->Link,
                            &SymbolsWalker->Link
                            )
                          );
      } while (!IsNull (&DefinedSymbols->Link, &SymbolsWalker->Link));
    } else if (NumberOfEntries < SuperVtable->NumberOfEntries) {
      Relocation = (CONST MACH_RELOCATION_INFO *)(
                     (UINTN)MachHeader + DySymtab->ExternalRelocationsOffset
                     );
      Relocation = MachoGetRelocationByOffset (
                     MachHeader,
                     DySymtab->NumberOfExternalRelocations,
                     Relocation,
                     (VtableSymbol->Value + EntryOffset)
                     );
      if (Relocation != NULL) {
        Symbol = &Symbols[Relocation->SymbolNumber];
        Result = InternalPatchVtableSymbol (
                   MachHeader,
                   Symbols,
                   DySymtab,
                   Strings,
                   &SuperVtable->Entries[NumberOfEntries],
                   (Strings + VtableSymbol->UnifiedName.StringIndex),
                   Symbol
                   );
        if (!Result) {
          return FALSE;
        }

        Name = (Strings + Symbol->UnifiedName.StringIndex);
        Vtable->Entries[NumberOfEntries].Name    = Name;
        Vtable->Entries[NumberOfEntries].Address = Symbol->Value;
      } else {
        //
        // When the VTable entry is 0 and it is not referenced by a Relocation,
        // it is the end of the table.
        //
        break;
      }
    }

    ++EntryOffset;
    ++NumberOfEntries;
  }

  Vtable->NumberOfEntries = NumberOfEntries;

  return TRUE;
}

STATIC
VOID
InternalConstructVtablePrelinked64 (
  IN  CONST MACH_HEADER_64      *MachHeader,
  IN  CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  IN  CONST MACH_NLIST_64       *VtableSymbol,
  IN  CONST CHAR8               *Strings,
  OUT OC_VTABLE                 *Vtable
  )
{
  CONST MACH_SECTION_64    *Section;
  CONST UINT64             *VtableData;
  UINT64                   Value;
  CONST OC_SYMBOL_TABLE_64 *SymbolsWalker;
  CONST CHAR8              *StringTable;
  UINTN                    Index;
  UINTN                    CxxIndex;
  CONST OC_SYMBOL_64       *Symbol;

  ASSERT (VtableSymbol != NULL);
  ASSERT (Strings != NULL);
  ASSERT (Vtable != NULL);

  Section = MachoGetSectionByIndex64 (MachHeader, VtableSymbol->Section);
  ASSERT (Section != NULL);

  VtableData = (CONST UINT64 *)(
                 (UINTN)MachHeader
                   + (VtableSymbol->Value - Section->Address)
                   + Section->Offset
                 );
  Vtable->Name = (Strings + VtableSymbol->UnifiedName.StringIndex);
  //
  // Initialize the VTable by entries.
  //
  SymbolsWalker = DefinedSymbols;

  do {
    StringTable = SymbolsWalker->StringTable;
    CxxIndex = (
                 SymbolsWalker->NumberOfSymbols
                   - SymbolsWalker->NumberOfCxxSymbols
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
        Index = CxxIndex;
        Index < SymbolsWalker->NumberOfSymbols;
        ++Index
        ) {
        Symbol = &SymbolsWalker->Symbols[Index];

        if (Symbol->Value == Value) {
          Vtable->Entries[Index].Address = Symbol->Value;
          Vtable->Entries[Index].Name    = (StringTable + Symbol->StringIndex);
        }
      }
      //
      // If we can't find the symbol, it means that the virtual function was
      // defined inline.  There's not much I can do about this; it just means
      // I can't patch this function.
      //
      if (Index == SymbolsWalker->NumberOfCxxSymbols) {
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
}

STATIC
CONST OC_VTABLE *
InternalGetOcVtableByName (
  IN CONST OC_VTABLE_ARRAY  *Vtables,
  IN CONST CHAR8            *Name
  )
{
  CONST OC_VTABLE_ARRAY *VtableWalker;
  CONST OC_VTABLE       *Vtable;
  UINTN                 Index;
  INTN                  Result;

  VtableWalker = Vtables;

  do {
    Vtable = GET_FIRST_OC_VTABLE (VtableWalker);

    for (Index = 0; Index < VtableWalker->NumberOfVtables; ++Index) {
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
InternalCreateVtablesNonPrelinked64 (
  IN     CONST MACH_HEADER_64         *MachHeader,
  IN     CONST OC_DEPENDENCY_DATA     *DependencyData,
  IN     UINTN                        NumberOfVtables,
  IN     CONST MACH_NLIST_64          *VtableSymbols,
  IN OUT MACH_NLIST_64                *SymbolTable,
  IN     CONST CHAR8                  *StringTable,
  IN     UINTN                        NumberOfRelocations,
  IN     CONST MACH_RELOCATION_INFO   *Relocations,
  IN     CONST MACH_DYSYMTAB_COMMAND  *DySymtab,
  IN     UINTN                        NumberOfSymbols,
  OUT    OC_VTABLE                    *VtableBuffer
  )
{
  UINTN               Index;
  UINTN               NumberOfPatchedTables;
  BOOLEAN             Result;
  CONST MACH_NLIST_64 *SuperMetaclassPointer;
  CONST MACH_NLIST_64 *VtableSymbol;
  CONST MACH_NLIST_64 *MetaVtableSymbol;
  CONST MACH_NLIST_64 *MetaClass;
  CONST UINT64        *VtableData;
  CONST OC_VTABLE     *SuperVtable;
  CONST OC_VTABLE     *MetaVtable;
  CONST VOID          *SymbolDummy;
  CHAR8               ClassName[SYM_MAX_NAME_LEN];
  CHAR8               SuperClassName[SYM_MAX_NAME_LEN];
  CHAR8               VtableName[SYM_MAX_NAME_LEN];
  CHAR8               SuperVtableName[SYM_MAX_NAME_LEN];
  CHAR8               FinalSymbolName[SYM_MAX_NAME_LEN];
  BOOLEAN             SuccessfulIteration;

  NumberOfPatchedTables = 0;

  while (NumberOfPatchedTables < NumberOfVtables) {
    SuccessfulIteration = FALSE;

    for (Index = 0; Index < NumberOfVtables; ++Index) {
      SuperMetaclassPointer = &VtableSymbols[Index];
      //
      // We walk over the super metaclass pointer symbols because classes
      // with them are the only ones that need patching.  Then we double the
      // number of vtables we're expecting, because every pointer will have a
      // class vtable and a MetaClass vtable.
      //
      DEBUG_CODE (
        Result = MachoSymbolIsSmcp64 (SuperMetaclassPointer, StringTable);
        ASSERT (Result);
        );

      Result = MachoGetVtableSymbolsFromSmcp64 (
                 SymbolTable,
                 StringTable,
                 DySymtab,
                 SuperMetaclassPointer,
                 &VtableSymbol,
                 &MetaVtableSymbol
                 );
      if (!Result) {
        return FALSE;
      }
      //
      // Get the class name from the smc pointer 
      //
      Result = MachoGetClassNameFromSuperMetaClassPointer (
                 StringTable,
                 SuperMetaclassPointer,
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
                    MachHeader,
                    NumberOfSymbols,
                    SymbolTable,
                    StringTable,
                    NumberOfRelocations,
                    Relocations,
                    SuperMetaclassPointer
                    );
      if (MetaClass == NULL) {
        return FALSE;
      }
      //
      // Get the super class name from the super metaclass
      //
      Result = MachoGetClassNameFromMetaClassPointer (
                 StringTable,
                 MetaClass,
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
      SymbolDummy = InternalOcGetSymbolByName (
                      DependencyData->SymbolTable,
                      FinalSymbolName,
                      TRUE
                      );
      if (SymbolDummy == NULL) {
        SymbolDummy = MachoGetLocalDefinedSymbolByName (
                        SymbolTable,
                        StringTable,
                        DySymtab,
                        FinalSymbolName
                        );
      }
      if (SymbolDummy != NULL) {
        ASSERT (FALSE);
        continue;
      }
      //
      // Patch the class's vtable
      //
      VtableData = (CONST UINT64 *)((UINTN)MachHeader + VtableSymbol->Value);
      Result = InternalInitializeVtableByEntriesAndRelocations64 (
                 MachHeader,
                 DependencyData->SymbolTable,
                 SymbolTable,
                 StringTable,
                 DySymtab,
                 SuperVtable,
                 VtableSymbol,
                 VtableData,
                 VtableBuffer
                 );
      if (!Result) {
        return FALSE;
      }

      VtableBuffer->Name = (StringTable + VtableSymbol->UnifiedName.StringIndex);
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
      VtableData = (CONST UINT64 *)(
                     (UINTN)MachHeader + MetaVtableSymbol->Value
                     );
      Result = InternalInitializeVtableByEntriesAndRelocations64 (
                 MachHeader,
                 DependencyData->SymbolTable,
                 SymbolTable,
                 StringTable,
                 DySymtab,
                 SuperVtable,
                 MetaVtableSymbol,
                 VtableData,
                 VtableBuffer
                 );
      if (!Result) {
        return FALSE;
      }

      VtableBuffer->Name = (StringTable + MetaVtableSymbol->UnifiedName.StringIndex);
      //
      // Add the MetaClass's vtable to the set of patched vtables
      // This is done implicitely as we're operating on an array.
      //
      VtableBuffer = GET_NEXT_OC_VTABLE (VtableBuffer);

      ++NumberOfPatchedTables;
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

  return TRUE;
}

STATIC
UINTN
InternalCreateVtablesPrelinked64 (
  IN  CONST MACH_HEADER_64      *MachHeader,
  IN  CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  OUT OC_VTABLE                 *VtableBuffer
  )
{
  UINTN                     NumberOfVtables;

  CONST MACH_SYMTAB_COMMAND *Symtab;
  CONST CHAR8               *Strings;
  CONST MACH_NLIST_64       *Symbol;
  UINTN                     Index;

  ASSERT (MachHeader != NULL);
  ASSERT (MachHeader->Signature == MACH_HEADER_64_SIGNATURE);
  ASSERT (DefinedSymbols != NULL);

  NumberOfVtables = 0;

  Symtab = (CONST MACH_SYMTAB_COMMAND *)(
             MachoGetFirstCommand64 (MachHeader, MACH_LOAD_COMMAND_SYMTAB)
             );
  ASSERT (Symtab != NULL);

  Strings = (CONST CHAR8 *)((UINTN)MachHeader + Symtab->StringsOffset);
  Symbol  = (CONST MACH_NLIST_64 *)((UINTN)MachHeader + Symtab->SymbolsOffset);

  for (Index = 0; Index < Symtab->NumberOfSymbols; ++Index) {
    if (MachoSymbolIsVtable64 (Symbol, Strings)) {
      InternalConstructVtablePrelinked64 (
        MachHeader,
        DefinedSymbols,
        Symbol,
        Strings,
        VtableBuffer
        );
      VtableBuffer = GET_NEXT_OC_VTABLE (VtableBuffer);

      ++NumberOfVtables;
    }

    ++Symbol;
  }

  return NumberOfVtables;
}

//
// Relocations
//

/**
  Calculate the target address' displacement for the Intel 64 platform.
  Instruction will be patched with the resulting address.
  Logically matches XNU's calculate_displacement_x86_64.

  @param[in]     Target       The target address.
  @param[in]     Adjustment   Adjustment to be subtracted from the
                              displacement.
  @param[in,out] Instruction  Pointer to the instruction to be patched.

  @retval  Returned is whether the target instruction has been patched.

**/
STATIC
BOOLEAN
InternalCalculateDisplacementIntel64 (
  IN     UINT64  Target,
  IN     UINT64  Adjustment,
  IN OUT INT32   *Instruction
  )
{
  INT64  Displacement;
  UINT64 Difference;

  ASSERT (Instruction != NULL);

  Displacement = ((*Instruction + Target) - Adjustment);
  Difference   = ABS (Displacement);

  if (Difference >= X86_64_RIP_RELATIVE_LIMIT) {
    return FALSE;
  }

  *Instruction = (INT32)Displacement;

  return TRUE;
}

/**
  Calculate the target addresses for Relocation and NextRelocation.
  Logically matches XNU's calculate_targets.

  @param[in]  MachHeader       MACH-O header of the KEXT to relocate.
  @param[in]  LinkAddress      The address to be linked against.
  @param[in]  Vtables          List of all dependent VTables.
  @param[in]  NumberOfSymbols  The number of symbols in SymbolTable.
  @param[in]  SymbolTable      The KEXT's Symbol Table.
  @param[in]  StringTable      The KEXT's String Table.
  @param[in]  Relocation       The Relocation to be resolved.
  @param[in]  NextRelocation   The Relocation following Relocation.
  @param[out] Target           Relocation's target address.
  @param[out] PairTarget       NextRelocation's target address.
  @param[out] Vtable           The VTable described by the symbol referenced by
                               Relocation.  NULL, if there is none.

  @retval FALSE  The Relocation does not need to be preseved.
  @retval TRUE   The Relocation must be preseved.

**/
STATIC
BOOLEAN
InternalCalculateTargetsIntel64 (
  IN  CONST MACH_HEADER_64        *MachHeader,
  IN  UINT64                      LinkAddress,
  IN  CONST OC_VTABLE_ARRAY       *Vtables,
  IN  UINTN                       NumberOfSymbols,
  IN  CONST MACH_NLIST_64         *SymbolTable,
  IN  CONST CHAR8                 *StringTable,
  IN  CONST MACH_RELOCATION_INFO  *Relocation,
  IN  CONST MACH_RELOCATION_INFO  *NextRelocation  OPTIONAL,
  OUT UINT64                      *Target,
  OUT UINT64                      *PairTarget,
  OUT CONST OC_VTABLE             **Vtable  OPTIONAL
  )
{
  BOOLEAN               Success;

  UINT64                TargetAddress;
  CONST MACH_NLIST_64   *Symbol;
  CONST CHAR8           *Name;
  CONST MACH_SECTION_64 *Section;
  UINT64                PairAddress;
  UINT64                PairDummy;
  UINTN                 Index;
  CONST OC_VTABLE_ARRAY *VtablesWalker;
  INTN                  Result;
  CONST OC_VTABLE       *VtableWalker;

  ASSERT (Target != NULL);
  ASSERT (PairTarget != NULL);

  Section     = NULL;
  PairAddress = 0;
  //
  // Pull out the data from the relocation entries.  The target_type depends
  // on the Extern bit:
  //   Scattered -> Section Lookup by Address.
  //   Local (not extern) -> Section by Index
  //   Extern -> Symbolnum by Index
  //
  // Relocations referencing a symbol result in a Target of its resolved value.
  // Relocations referencing a section result in a Target of
  // (link_addr - base_addr), which should be LinkAddress aligned on the
  // section's boundary.
  //
  TargetAddress = LinkAddress;
  //
  // Scattered Relocations are only supported by i386.
  //
  ASSERT (((UINT32)Relocation->Address & MACH_RELOC_SCATTERED) == 0);

  if (Relocation->Extern != 0) {
    ASSERT (Relocation->SymbolNumber < NumberOfSymbols);

    Symbol = &SymbolTable[Relocation->SymbolNumber];
    Name   = (StringTable + Symbol->UnifiedName.StringIndex);
    //
    // If this symbol is a padslot that has already been replaced, then the
    // only way a relocation entry can still reference it is if there is a
    // vtable that has not been patched.  The vtable patcher uses the
    // MetaClass structure to find classes for patching, so an unpatched
    // vtable means that there is an OSObject-dervied class that is missing
    // its OSDeclare/OSDefine macros.
    //
    if (MachoIsSymbolNamePadslot (Name)) {
      return FALSE;
    }

    if ((Vtable != NULL) && MachoSymbolIsVtable64 (Symbol, StringTable)) {
      VtablesWalker = Vtables;
      VtableWalker  = GET_FIRST_OC_VTABLE (VtablesWalker);

      do {
        for (Index = 0; Index < VtablesWalker->NumberOfVtables; ++Index) {
          Result = AsciiStrCmp (VtableWalker->Name, Name);
          if (Result == 0) {
            *Vtable = VtableWalker;
            break;
          }
        }

        if (Index != VtablesWalker->NumberOfVtables) {
          break;
        }

        VtablesWalker = GET_OC_VTABLE_ARRAY_FROM_LINK (
                          GetNextNode (
                            &Vtables->Link,
                            &VtablesWalker->Link
                            )
                          );
        if (IsNull (&Vtables->Link, &VtablesWalker->Link)) {
          ASSERT (FALSE);
          break;
        }

        VtableWalker = GET_NEXT_OC_VTABLE (VtableWalker);
      } while (TRUE);
    }

    TargetAddress = Symbol->Value;
  } else {
    Section = MachoGetSectionByIndex64 (MachHeader, Relocation->SymbolNumber);
    ASSERT (Section != NULL);
  }

  if (Section != NULL) {
    TargetAddress = ALIGN_VALUE (
                      (Section->Address + LinkAddress),
                      Section->Alignment
                      );
    TargetAddress -= Section->Address;
  }

  if (MachoRelocationIsPairIntel64 (Relocation->Type)) {
    ASSERT (NextRelocation != NULL);
    ASSERT (MachoIsRelocationPairTypeIntel64 (NextRelocation->Type));
    //
    // As this relocation is the second one in a pair, it cannot be the start
    // of a pair itself.  Pass dummy data for the related arguments.  This call
    // shall never reach this very branch.
    //
    Success = InternalCalculateTargetsIntel64 (
                MachHeader,
                LinkAddress,
                Vtables,
                NumberOfSymbols,
                SymbolTable,
                StringTable,
                NextRelocation,
                NULL,
                &PairAddress,
                &PairDummy,
                NULL
                );
    if (!Success) {
      return FALSE;
    }
  }

  *Target     = TargetAddress;
  *PairTarget = PairAddress;

  return TRUE;
}

/**
  Returns whether the VTable entry at Offset is a direct pure virtual call.
  Logically macthes XNU's check_for_direct_pure_virtual_call.

  @param[in] Vtable  The current VTable to be inspected.
  @param[in] Offset  The VTable's entry offset.

**/
STATIC
BOOLEAN
InternalIsDirectPureVirtualCall64 (
  IN CONST OC_VTABLE  *Vtable,
  IN UINT64            Offset
  )
{
  UINTN                 Index;
  CONST OC_VTABLE_ENTRY *Entry;

  if ((Offset % sizeof (UINT64)) != 0) {
    ASSERT (FALSE);
    return FALSE;
  }

  Index = ((Offset - VTABLE_ENTRY_SIZE_64) / sizeof (UINT64));
  Entry = &Vtable->Entries[Index];

  if ((Index >= Vtable->NumberOfEntries) || (Entry->Name == NULL)) {
    return FALSE;
  }

  return MachoIsSymbolNamePureVirtual (Entry->Name);
}

/**
  Relocates Relocation against the specified Symtab's Symbol Table and
  LinkAddress.  This logically matches KXLD's x86_64_process_reloc.

  @param[in] MachHeader       MACH-O header of the KEXT to relocate.
  @param[in] LinkAddress      The address to be linked against.
  @param[in] Vtables          List of all dependent VTables.
  @param[in] NumberOfSymbols  The number of symbols in SymbolTable.
  @param[in] SymbolTable      The KEXT's Symbol Table.
  @param[in] StringTable      The KEXT's String Table.
  @param[in] RelocationBase   The Relocations base address.
  @param[in] Relocation       The Relocation to be processed.
  @param[in] NextRelocation   The Relocation following Relocation.

  @retval 0          The Relocation does not need to be preseved.
  @retval 1          The Relocation must be preseved.
  @retval 0 | BIT31  The Relocation does not need to be preseved.
                     The next Relocation shall be skipped.
  @retval 1 | BIT31  The Relocation must be preseved.
                     The next Relocation shall be skipped.
  @retval MAX_UINTN  The Relocation's target cannot be determined or it is a
                     direct pure virtual call.

**/
STATIC
UINTN 
InternalRelocateRelocationIntel64 (
  IN CONST MACH_HEADER_64        *MachHeader,
  IN UINT64                      LinkAddress,
  IN CONST OC_VTABLE_ARRAY       *Vtables,
  IN UINTN                       NumberOfSymbols,
  IN CONST MACH_NLIST_64         *SymbolTable,
  IN CONST CHAR8                 *StringTable,
  IN UINTN                       RelocationBase,
  IN CONST MACH_RELOCATION_INFO  *Relocation,
  IN CONST MACH_RELOCATION_INFO  *NextRelocation  OPTIONAL
  )
{
  UINTN           ReturnValue;

  UINT8           Type;
  INT32           *Instruction32Ptr;
  INT32           Instruction32;
  UINT64          *Instruction64Ptr;
  UINT64          Instruction64;
  UINT64          Target;
  BOOLEAN         IsPair;
  UINT64          PairTarget;
  CONST OC_VTABLE *Vtable;
  UINT64          LinkPc;
  UINT64          Adjustment;
  UINTN           Length;
  UINT32          Address;
  UINT8           *InstructionPtr;
  BOOLEAN         PcRelative;
  BOOLEAN         IsNormalLocal;
  BOOLEAN         Result;

  ASSERT (RelocationBase != 0);
  ASSERT (Relocation != NULL);
  //
  // Scattered Relocations are only supported by i386.
  //
  ASSERT (((UINT32)Relocation->Address & MACH_RELOC_SCATTERED) == 0);

  IsPair        = FALSE;
  Adjustment    = 0;
  IsNormalLocal = FALSE;

  if ((Relocation->Extern == 0)
   && (Relocation->SymbolNumber == MACH_RELOC_ABSOLUTE)) {
    //
    // A section-based relocation entry can be skipped for absolute 
    // symbols.
    //
    return FALSE;
  }

  Address    = Relocation->Address;
  Length     = Relocation->Size;
  Type       = Relocation->Type;
  PcRelative = (Relocation->PcRelative != 0);

  if (Relocation->Extern == 0) {
    IsNormalLocal = TRUE;
  }

  ASSERT ((Length == 2) || (Length == 3));

  LinkPc         = (Address + LinkAddress);
  InstructionPtr = (UINT8 *)(RelocationBase + Address);

  Vtable = NULL;
  Result = InternalCalculateTargetsIntel64 (
             MachHeader,
             LinkAddress,
             Vtables,
             NumberOfSymbols,
             SymbolTable,
             StringTable,
             Relocation,
             NextRelocation,
             &Target,
             &PairTarget,
             &Vtable
             );
  if (!Result) {
    return MAX_UINTN;
  }

  if (Length == 2) {
    Instruction32Ptr = (INT32 *)InstructionPtr;
    Instruction32    = *Instruction32Ptr;

    if ((Vtable != NULL)
     && InternalIsDirectPureVirtualCall64 (Vtable, Instruction32)) {
      return MAX_UINTN;
    }
    //
    // There are a number of different small adjustments for pc-relative
    // relocation entries.  The general case is to subtract the size of the
    // relocation (represented by the length parameter), and it applies to
    // the GOT types and external SIGNED types.  The non-external signed types
    // have a different adjustment corresponding to the specific type.
    //
    switch (Type) {
      case MachX8664RelocSigned:
        if (IsNormalLocal) {
          Adjustment = 0;    
          break;
        }
        // Fall through
      case MachX8664RelocSigned1:
        if (IsNormalLocal) {
          Adjustment = 1;
          break;
        }
        // Fall through
      case MachX8664RelocSigned2:
        if (IsNormalLocal) {
          Adjustment = 2;
          break;
        }
        // Fall through
      case MachX8664RelocSigned4:
        if (IsNormalLocal) {
          Adjustment = 4;
          break;
        }
        // Fall through
      case MachX8664RelocBranch:
      case MachX8664RelocGot:
      case MachX8664RelocGotLoad:
      {
        Adjustment = (1ULL << Length);
        break;
      }

      default:
      {
        break;
      }
    }
    //
    // Perform the actual relocation.  All of the 32-bit relocations are 
    // pc-relative except for SUBTRACTOR, so a good chunk of the logic is
    // stuck in calculate_displacement_x86_64.  The signed relocations are
    // a special case, because when they are non-external, the instruction
    // already contains the pre-relocation displacement, so we only need to
    // find the difference between how far the PC was relocated, and how
    // far the target is relocated.  Since the target variable already
    // contains the difference between the target's base and link
    // addresses, we add the difference between the PC's base and link
    // addresses to the adjustment variable.  This will yield the
    // appropriate displacement in calculate_displacement.
    //
    switch (Type) {
      case MachX8664RelocBranch:
      {
        ASSERT (PcRelative);
        Adjustment += LinkPc;
        break;
      }

      case MachX8664RelocSigned:
      case MachX8664RelocSigned1:
      case MachX8664RelocSigned2:
      case MachX8664RelocSigned4:
      {
        ASSERT (PcRelative);
        Adjustment += (IsNormalLocal ? LinkAddress : LinkPc);
        break;
      }

      case MachX8664RelocGot:
      case MachX8664RelocGotLoad:
      {
        ASSERT (PcRelative);
        Adjustment += LinkPc;
        Target      = PairTarget;
        IsPair      = TRUE;
        break;
      }

      case MachX8664RelocSubtractor:
      {
        ASSERT (!PcRelative);
        Instruction32 = (INT32)(Target - PairTarget);
        IsPair        = TRUE;
        break;
      }

      default:
      {
        return FALSE;
      }
    }

    if (PcRelative) {
      Result = InternalCalculateDisplacementIntel64 (
                 Target,
                 Adjustment,
                 &Instruction32
                 );
      if (!Result) {
        return FALSE;
      }
    }
        
    *Instruction32Ptr = Instruction32;
  } else {
    Instruction64Ptr = (UINT64 *)InstructionPtr;
    Instruction64    = *Instruction64Ptr;

    if ((Vtable != NULL)
     && InternalIsDirectPureVirtualCall64 (Vtable, Instruction64)) {
      return MAX_UINTN;
    }

    switch (Type) {
      case MachX8664RelocUnsigned:
      {
        ASSERT (!PcRelative);
        Instruction64 += Target;
        break;
      }

      case MachX8664RelocSubtractor:
      {
        ASSERT (!PcRelative);
        Instruction64 = (Target - PairTarget);
        IsPair        = TRUE;
        break;
      }

      default:
      {
        return FALSE;
      }
    }

    *Instruction64Ptr = Instruction64;
  }

  ReturnValue = (MachoPreserveRelocationIntel64 (Type) ? 1 : 0);

  if (IsPair) {
    ReturnValue |= BIT31;
  }

  return ReturnValue;
}

/**
  Relocates all MACH-O Relocations and copies the ones to be preserved after
  prelinking to TargetRelocations.

  @param[in]  MachHeader           MACH-O header of the KEXT to relocate.
  @param[in]  LinkAddress          The address to be linked against.
  @param[in]  Vtables              The patched VTables of this KEXT and its
                                   dependencies.
  @param[in]  NumberOfSymbols      The number of symbols in SymbolTable.
  @param[in]  SymbolTable          The KEXT's Symbol Table.
  @param[in]  StringTable          The KEXT's String Table.
  @param[in]  RelocationBase       The Relocations base address.
  @param[in]  SourceRelocations    The Relocations source buffer.
  @param[in]  NumberOfRelocations  On input, the number of source Relocations.
                                   On output, the number of Relocations to
                                   preserve.
  @param[out] TargetRelocations    The Relocations destination buffer.

  @retval  Returned is the number of preserved Relocations.

**/
STATIC
BOOLEAN
InternalRelocateAndCopyRelocations64 (
  IN  CONST MACH_HEADER_64        *MachHeader,
  IN  UINT64                      LinkAddress,
  IN  CONST OC_VTABLE_ARRAY       *Vtables,
  IN  UINTN                       NumberOfSymbols,
  IN  CONST MACH_NLIST_64         *SymbolTable,
  IN  CONST CHAR8                 *StringTable,
  IN  UINTN                       RelocationBase,
  IN  CONST MACH_RELOCATION_INFO  *SourceRelocations,
  IN  OUT UINTN                   *NumberOfRelocations,
  OUT MACH_RELOCATION_INFO        *TargetRelocations
  )
{
  UINT32                     PreservedRelocations;

  UINTN                      Index;
  CONST MACH_RELOCATION_INFO *NextRelocation;
  UINTN                      Result;
  MACH_RELOCATION_INFO       *Relocation;

  ASSERT (NumberOfSymbols > 0);
  ASSERT (SymbolTable != NULL);
  ASSERT (StringTable != NULL);
  ASSERT (RelocationBase != 0);
  ASSERT (SourceRelocations != NULL);
  ASSERT (NumberOfRelocations != NULL);
  ASSERT (NumberOfRelocations > 0);
  ASSERT (TargetRelocations != NULL);

  PreservedRelocations = 0;

  for (Index = 0; Index < *NumberOfRelocations; ++Index) {
    NextRelocation = &SourceRelocations[Index + 1];
    //
    // The last Relocation does not have a successor.
    //
    if (Index == (*NumberOfRelocations - 1)) {
      NextRelocation = NULL;
    }
    //
    // Relocate the relocation.
    //
    Result = InternalRelocateRelocationIntel64 (
               MachHeader,
               LinkAddress,
               Vtables,
               NumberOfSymbols,
               SymbolTable,
               StringTable,
               RelocationBase,
               &SourceRelocations[Index],
               NextRelocation
               );
    if (Result == MAX_UINTN) {
      return FALSE;
    }
    //
    // Copy the Relocation to the destination buffer if it shall be preserved.
    //
    if ((Result & ~(UINTN)BIT31) != 0) {
      Relocation = &TargetRelocations[PreservedRelocations];
      //
      // Scattered Relocations are only supported by i386.
      //
      ASSERT (((UINT32)Relocation->Address & MACH_RELOC_SCATTERED) == 0);

      CopyMem (Relocation, &SourceRelocations[Index], sizeof (*Relocation));

      if (Relocation->Extern != 0) {
        //
        // All relocation targets have been updated with symbol values.
        // Convert to a local relocation as only sliding is supported from now
        // on.
        //
        Relocation->Extern = 0;
        //
        // Assertion: The entire kext will be slid by the same offset.
        // This is asserted by KXLD as well.
        //
        Relocation->SymbolNumber = 1;
      }

      ++PreservedRelocations;
    }
    //
    // Skip the next Relocation as instructed by
    // InternalRelocateRelocationIntel64().
    //
    if ((Result & BIT31) != 0) {
      ++Index;
    }
  }

  *NumberOfRelocations = PreservedRelocations;

  return TRUE;
}

//
// MACH header
//

/**
  Strip superfluous Load Commands from the MACH-O header.  This includes the
  Code Signature Load Command which must be removed for the binary has been
  modified by the prelinking routines.

  @param[in,out] MachHeader  MACH-O header to strip the Load Commands from.

**/
STATIC
VOID
InternalStripLoadCommands64 (
  IN OUT MACH_HEADER_64  *MachHeader
  )
{
  STATIC CONST MACH_LOAD_COMMAND_TYPE LoadCommandsToStrip[] = {
    MACH_LOAD_COMMAND_CODE_SIGNATURE,
    MACH_LOAD_COMMAND_DYLD_INFO,
    MACH_LOAD_COMMAND_DYLD_INFO_ONLY,
    MACH_LOAD_COMMAND_FUNCTION_STARTS,
    MACH_LOAD_COMMAND_DATA_IN_CODE,
    MACH_LOAD_COMMAND_DYLIB_CODE_SIGN_DRS
  };

  UINTN             Index;
  UINTN             Index2;
  MACH_LOAD_COMMAND *LoadCommand;
  UINTN             SizeOfLeftCommands;
  //
  // Delete the Code Signature Load Command if existent as we modified the
  // binary, as well as linker metadata not needed for runtime operation.
  //
  LoadCommand        = MachHeader->Commands;
  SizeOfLeftCommands = MachHeader->CommandsSize;

  for (Index = 0; Index < MachHeader->NumberOfCommands; ++Index) {
    //
    // LC_UNIXTHREAD and LC_MAIN are technically stripped in KXLD, but they are
    // not supposed to be present in the first place.
    //
    ASSERT ((LoadCommand->Type != MACH_LOAD_COMMAND_UNIX_THREAD)
         && (LoadCommand->Type != MACH_LOAD_COMMAND_MAIN));

    SizeOfLeftCommands -= LoadCommand->Size;

    for (Index2 = 0; Index < ARRAY_SIZE (LoadCommandsToStrip); ++Index2) {
      if (LoadCommand->Type == LoadCommandsToStrip[Index2]) {
        if (Index != (MachHeader->NumberOfCommands - 1)) {
          //
          // If the current Load Command is not the last one, relocate the
          // subsequent ones.
          //
          CopyMem (
            LoadCommand,
            NEXT_MACH_LOAD_COMMAND (LoadCommand),
            SizeOfLeftCommands
            );
        }

        --MachHeader->NumberOfCommands;
        MachHeader->CommandsSize -= LoadCommand->Size;

        break;
      }
    }

    LoadCommand = NEXT_MACH_LOAD_COMMAND (LoadCommand);
  }
}

/**
  Prelinks the specified KEXT against the specified LinkAddress and the data
  of its dependencies.

  @param[in,out] MachHeader       MACH-O header of the KEXT to prelink.
  @param[in]     LinkAddress      The address this KEXT shall be linked against.
  @param[in]     DependencyData   List of data of all dependencies.
  @param[in]     NumberOfVtables  Number of entries in VtableSymbols.
  @param[in]     VtableSymbols    List of VTable symbols in this KEXT.
  @param[out]    OutSymbols       Buffer to output external symbols to.
  @param[out]    OutVtables       Buffer to output patched vtables into.

  @retval  Returned is whether the prelinking process has been successful.
           The state of the KEXT is undefined in case this routine fails.

**/
STATIC
BOOLEAN
InternalPrelinkKext64 (
  IN OUT MACH_HEADER_64      *MachHeader,
  IN     UINT64              LinkAddress,
  IN     OC_DEPENDENCY_DATA  *DependencyData,
  IN     UINTN               NumberOfVtables,
  IN     MACH_NLIST_64       *VtableSymbols,
  OUT    OC_SYMBOL_TABLE_64  *OutSymbols,
  OUT    OC_VTABLE           *OutVtables
  )
{
  MACH_SYMTAB_COMMAND        *Symtab;
  MACH_DYSYMTAB_COMMAND      *DySymtab;

  MACH_SEGMENT_COMMAND_64    *Segment;
  MACH_SECTION_64            *Section;

  UINTN                      Index;
  BOOLEAN                    Result;
  UINTN                      Size;

  MACH_NLIST_64              *Symbol;
  MACH_NLIST_64              *SymbolTable;
  CONST CHAR8                *StringTable;
  UINTN                      NumberOfSymbols;

  UINTN                      NumberOfRelocations;
  UINTN                      NumberOfRelocations2;
  UINTN                      RelocationBase;
  CONST MACH_RELOCATION_INFO *Relocations;
  MACH_RELOCATION_INFO       *TargetRelocation;
  MACH_SEGMENT_COMMAND_64    *FirstSegment;

  VOID                       *LinkEdit;
  UINTN                      SymbolTableOffset;
  UINTN                      SymbolTableSize;
  UINTN                      RelocationsOffset;
  UINTN                      RelocationsSize;
  UINTN                      StringTableOffset;
  UINTN                      StringTableSize;

  ASSERT (MachHeader != NULL);
  ASSERT (MachHeader->Signature == MACH_HEADER_64_SIGNATURE);
  ASSERT (LinkAddress != 0);
  ASSERT (DependencyData != NULL);
  ASSERT (DependencyData->SymbolTable != NULL);
  ASSERT (DependencyData->Vtables != NULL);
  //
  // Only perform actions when the kext is flag'd to be dynamically linked.
  //
  if ((MachHeader->Flags & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) == 0) {
    ASSERT (FALSE);
    return FALSE;
  }
  //
  // Strip superfluous Load Commands.
  //
  InternalStripLoadCommands64 (MachHeader);
  //
  // Retrieve the symbol tables required for most following operations.
  //
  DySymtab = (MACH_DYSYMTAB_COMMAND *)(
               MachoGetFirstCommand64 (MachHeader, MACH_LOAD_COMMAND_DYSYMTAB)
               );
  ASSERT (DySymtab != NULL);

  Symtab = (MACH_SYMTAB_COMMAND *)(
             MachoGetFirstCommand64 (MachHeader, MACH_LOAD_COMMAND_SYMTAB)
             );
  ASSERT (Symtab != NULL);

  StringTable = (CONST CHAR8 *)((UINTN)MachHeader + Symtab->StringsOffset);
  //
  // Solve indirect symbols.
  //
  SymbolTable = (MACH_NLIST_64 *)(
                  (UINTN)MachHeader + DySymtab->IndirectSymbolsOffset
                  );
  for (Index = 0; Index < DySymtab->NumberOfIndirectSymbols; ++Index) {
    Symbol = &SymbolTable[DySymtab->IndirectSymbolsOffset + Index];
    //
    // Indirect symbols are solved via their value as a String Table index.
    //
    Result = InternalSolveSymbol64 (
               DependencyData->SymbolTable,
               SymbolTable,
               StringTable,
               DySymtab,
               (StringTable + Symbol->Value),
               &SymbolTable[Index]
               );
    if (!Result) {
      ASSERT (FALSE);
      return FALSE;
    }
  }
  //
  // Solve undefined symbols.
  //
  SymbolTable = (MACH_NLIST_64 *)((UINTN)MachHeader + Symtab->SymbolsOffset);

  for (Index = 0; Index < DySymtab->NumberOfUndefinedSymbols; ++Index) {
    Symbol = &SymbolTable[DySymtab->UndefinedSymbolsIndex + Index];
    //
    // Undefined symbols are solved via their name.
    //
    Result = InternalSolveSymbol64 (
               DependencyData->SymbolTable,
               SymbolTable,
               StringTable,
               DySymtab,
               (StringTable + Symbol->UnifiedName.StringIndex),
               Symbol
               );
    if (!Result) {
      ASSERT (FALSE);
      return FALSE;
    }
  }
  //
  // Create and patch the KEXT's VTables.
  //
  Relocations = (CONST MACH_RELOCATION_INFO *)(
                  (UINTN)MachHeader + DySymtab->ExternalRelocationsOffset
                  );
  Result = InternalCreateVtablesNonPrelinked64 (
             MachHeader,
             DependencyData,
             NumberOfVtables,
             VtableSymbols,
             SymbolTable,
             StringTable,
             DySymtab->NumberOfExternalRelocations,
             Relocations,
             DySymtab,
             Symtab->NumberOfSymbols,
             OutVtables
             );
  if (!Result) {
    return FALSE;
  }
  //
  // Relocate local and external symbols.
  //
  for (Index = 0; Index < DySymtab->NumberOfLocalSymbols; ++Index) {
    Result = MachoRelocateSymbol64 (
               MachHeader,
               LinkAddress,
               &SymbolTable[DySymtab->LocalSymbolsIndex + Index]
               );
    if (!Result) {
      return FALSE;
    }
  }

  for (Index = 0; Index < DySymtab->NumberOfExternalSymbols; ++Index) {
    Result = MachoRelocateSymbol64 (
               MachHeader,
               LinkAddress,
               &SymbolTable[DySymtab->ExternalSymbolsIndex + Index]
               );
    if (!Result) {
      return FALSE;
    }
  }
  //
  // Prepare constructing a new __LINKEDIT section to...
  //   1. strip undefined symbols for they are not allowed in prelinked
  //      binaries,
  //   2. merge local and external relocations as only sliding is allowed from
  //      this point onwards,
  //   3. strip Code Signature because we modified the binary, as well as other
  //      linker metadata stripped by KXLD as well, probably for space reasons.
  //
  // Example original layout:
  //   Local relocations - Symbol Table - external relocations - String Table -
  //   Code Signature
  //
  // Example prelinked layout
  //   Symbol Table - relocations (external -> local) - String Table
  //
  NumberOfSymbols   = Symtab->NumberOfSymbols;
  NumberOfSymbols  -= DySymtab->NumberOfUndefinedSymbols;
  SymbolTableOffset = 0;
  SymbolTableSize   = (NumberOfSymbols * sizeof (MACH_NLIST_64));
  RelocationsOffset = (SymbolTableOffset + SymbolTableSize);
  StringTableSize   = Symtab->StringsSize;
  //
  // For the allocation, assume all relocations will be preserved to simplify
  // the code, the memory is only temporarily allocated anyway.
  //
  NumberOfRelocations  = DySymtab->NumberOfLocalRelocations;
  NumberOfRelocations += DySymtab->NumberOfExternalRelocations;
  RelocationsSize      = (NumberOfRelocations * sizeof (MACH_RELOCATION_INFO));

  LinkEdit = AllocatePool (SymbolTableSize + RelocationsSize + StringTableSize);
  if (LinkEdit == NULL) {
    ASSERT (FALSE);
    return FALSE;
  }

  FirstSegment = MachoGetFirstSegment64 (MachHeader);
  //
  // Copy the relocations to be reserved and adapt the symbol number they
  // reference in case it has been relocated above.
  //
  TargetRelocation = (MACH_RELOCATION_INFO *)(
                       (UINTN)LinkEdit + RelocationsOffset
                       );
  RelocationBase = ((UINTN)MachHeader + FirstSegment->FileOffset);
  //
  // Relocate and copy local and external relocations.
  //
  Relocations = (CONST MACH_RELOCATION_INFO *)(
                  (UINTN)MachHeader + DySymtab->LocalRelocationsOffset
                  );
  NumberOfRelocations = DySymtab->NumberOfLocalRelocations;
  Result = InternalRelocateAndCopyRelocations64 (
             MachHeader,
             LinkAddress,
             DependencyData->Vtables,
             Symtab->NumberOfSymbols,
             SymbolTable,
             StringTable,
             RelocationBase,
             Relocations,
             &NumberOfRelocations,
             &TargetRelocation[0]
             );
  if (!Result) {
    ASSERT (FALSE);
    FreePool (LinkEdit);
    return FALSE;
  }

  Relocations = (MACH_RELOCATION_INFO *)(
                  (UINTN)MachHeader + DySymtab->ExternalRelocationsOffset
                  );
  NumberOfRelocations2 = DySymtab->NumberOfExternalRelocations;
  Result = InternalRelocateAndCopyRelocations64 (
             MachHeader,
             LinkAddress,
             DependencyData->Vtables,
             Symtab->NumberOfSymbols,
             SymbolTable,
             StringTable,
             RelocationBase,
             Relocations,
             &NumberOfRelocations2,
             &TargetRelocation[NumberOfRelocations]
             );
  if (!Result) {
    ASSERT (FALSE);
    FreePool (LinkEdit);
    return FALSE;
  }
  NumberOfRelocations += NumberOfRelocations2;
  //
  // Expose the external Symbol Table if requested.
  //
  if (OutSymbols != NULL) {
    InternalFillSymbolTable64 (
      DySymtab->NumberOfExternalSymbols,
      &SymbolTable[DySymtab->ExternalSymbolsIndex],
      StringTable,
      OutSymbols
      );
  }
  //
  // Copy the entire symbol table excluding the area for undefined symbols.
  //
  Size = (DySymtab->UndefinedSymbolsIndex * sizeof (MACH_NLIST_64));

  if (DySymtab->UndefinedSymbolsIndex != 0) {
    CopyMem (
      (VOID *)((UINTN)LinkEdit + SymbolTableOffset),
      (VOID *)&SymbolTable[0],
      Size
      );
  }

  NumberOfSymbols  = DySymtab->UndefinedSymbolsIndex;
  NumberOfSymbols += DySymtab->NumberOfUndefinedSymbols;
  if (NumberOfSymbols != Symtab->NumberOfSymbols) {
    CopyMem (
      (VOID *)((UINTN)LinkEdit + SymbolTableOffset + Size),
      (VOID *)&SymbolTable[NumberOfSymbols],
      ((Symtab->NumberOfSymbols - NumberOfSymbols) * sizeof (MACH_NLIST_64))
      );
  }

  RelocationsSize = (NumberOfRelocations * sizeof (MACH_RELOCATION_INFO));
  //
  // Copy the String Table.  Don't strip it for the saved bytes are unlikely
  // worth the time required.
  //
  StringTableOffset = (RelocationsOffset + RelocationsSize);
  CopyMem (
    (VOID *)((UINTN)LinkEdit + StringTableOffset),
    StringTable,
    StringTableSize
    );
  //
  // Set up the tables with the new offsets and Symbol Table length.
  //
  Symtab->SymbolsOffset    = SymbolTableOffset;
  Symtab->NumberOfSymbols -= DySymtab->NumberOfUndefinedSymbols;
  Symtab->StringsOffset    = StringTableOffset;

  DySymtab->LocalRelocationsOffset   = RelocationsOffset;
  DySymtab->NumberOfLocalRelocations = NumberOfRelocations;
  //
  // Clear dynamic linker information.
  //
  DySymtab->LocalSymbolsIndex           = 0;
  DySymtab->NumberOfLocalSymbols        = 0;
  DySymtab->ExternalSymbolsIndex        = 0;
  DySymtab->NumberOfExternalRelocations = 0;
  DySymtab->UndefinedSymbolsIndex       = 0;
  DySymtab->NumberOfUndefinedSymbols    = 0;
  DySymtab->IndirectSymbolsOffset       = 0;
  DySymtab->NumberOfIndirectSymbols     = 0;
  //
  // Copy the new __LINKEDIT segment into the binary and fix its Load Command.
  //
  Segment = MachoGetSegmentByName64 (MachHeader, "__LINKEDIT");
  ASSERT (Segment != NULL);

  Segment->FileSize = (SymbolTableSize + RelocationsSize + StringTableSize);
  Segment->Size     = ALIGN_VALUE (Segment->FileSize, BASE_4KB);
  CopyMem (
    (VOID *)((UINTN)MachHeader + Segment->FileOffset),
    LinkEdit,
    Segment->FileSize
    );
  FreePool (LinkEdit);
  //
  // Adapt the link addresses of all Segments and their Sections.
  //
  for (
    Segment = FirstSegment;
    Segment != NULL;
    Segment = MachoGetNextSegment64 (MachHeader, Segment)
    ) {
    Section = Segment->Sections;

    for (Index = 0; Index < Segment->NumberOfSections; ++Index) {
      Section->Address = ALIGN_VALUE (
                           (Section->Address + LinkAddress),
                           Section->Alignment
                           );
      ++Section;
    }

    Segment->VirtualAddress += LinkAddress;
  }
  //
  // Adapt the MACH-O header to signal being prelinked.
  //
  MachHeader->Flags = MACH_HEADER_FLAG_NO_UNDEFINED_REFERENCES;

  return TRUE;
}

STATIC
INTN
EFIAPI
AsciiKextNameCmp (
  IN CONST CHAR8  *FirstString,
  IN CONST CHAR8  *SecondString
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (FirstString));
  ASSERT (AsciiStrSize (SecondString));

  while ((*FirstString != '<') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }

  if ((*FirstString == '<') && (*SecondString == '0')) {
    return 0;
  }

  return *FirstString - *SecondString;
}

#define OS_BUNDLE_LIBRARIES_STR           "OSBundleLibraries"
#define OS_BUNDLE_IDENTIFIER_STR          "CFBundleIdentifier"
#define OS_BUNDLE_VERSION_STR             "CFBundleVersion"
#define OS_BUNDLE_COMPATIBLE_VERSION_STR  "OSBundleCompatibleVersion"

STATIC
CONST CHAR8 *
InternalFindPrelinkedKextPlist (
  IN CONST CHAR8  *PrelinkedPlist,
  IN CONST CHAR8  *BundleId
  )
{
  CONST CHAR8 *Walker;
  CONST CHAR8 *Walker2;
  CONST CHAR8 *BundleIdKey;
  INTN        Result;
  UINTN       DictLevel;
  //
  // Iterate through all OS Bundle IDs.
  //
  BundleIdKey = "<key>" OS_BUNDLE_IDENTIFIER_STR "<";

  for (
    Walker = AsciiStrStr (PrelinkedPlist, BundleIdKey);
    Walker != NULL;
    Walker = AsciiStrStr (Walker, BundleIdKey)
    ) {
    Walker2 = AsciiStrStr (Walker, "<string>");
    ASSERT (Walker2 != NULL);
    Walker2 += (ARRAY_SIZE ("<string>") - 1);

    Result = AsciiKextNameCmp (Walker2, BundleId);
    if (Result == 0) {
      //
      // The OS Bundle ID is in the first level of the KEXT PLIST dictionary.
      //
      DictLevel = 1;

      for (
        Walker -= (ARRAY_SIZE ("<dict>") - 1);
        TRUE;
        --Walker
        ) {
        ASSERT (Walker >= PrelinkedPlist);
        //
        // Find the KEXT PLIST dictionary entry.
        //
        Result = AsciiStrnCmp (
                   Walker,
                   "</dict>",
                   (ARRAY_SIZE ("</dict>") - 1)
                   );
        if (Result == 0) {
          ++DictLevel;
          Walker -= ((ARRAY_SIZE ("<dict>") - 1) - 1);
          continue;
        }

        Result = AsciiStrnCmp (
                  Walker,
                  "<dict>",
                  (ARRAY_SIZE ("<dict>") - 1)
                  );
        if (Result == 0) {
          --DictLevel;

          if (DictLevel == 0) {
            //
            // Return the content within the KEXT PLIST dictionary.
            //
            return (Walker + (ARRAY_SIZE ("<dict>") - 1));
          }

          Walker -= ((ARRAY_SIZE ("<dict>") - 1) - 1);
          continue;
        }
      }
    }

    Walker = Walker2;
  }

  return NULL;
}

STATIC
UINT64
InternalGetNewPrelinkedKextLoadAddress (
  IN CONST MACH_HEADER_64           *KernelHeader,
  IN CONST MACH_SEGMENT_COMMAND_64  *PrelinkedKextsSegment,
  IN CONST CHAR8                    *PrelinkedPlist
  )
{
  UINT64               Address;

  CONST CHAR8          *Walker;
  CHAR8                *Walker2;
  UINT64               TempAddress;
  CONST MACH_HEADER_64 *KextHeader;

  Address = 0;

  for (
    Walker = AsciiStrStr (PrelinkedPlist, "<key>_PrelinkExecutableLoadAddr<");
    Walker != NULL;
    Walker = AsciiStrStr (Walker, "<key>_PrelinkExecutableLoadAddr<")
    ) {
    Walker += (ARRAY_SIZE ("<key>_PrelinkExecutableLoadAddr</key>") - 1);
    Walker  = AsciiStrStr (Walker, "<integer");
    ASSERT (Walker != NULL);
    Walker += (ARRAY_SIZE ("<integer") - 1);
    //
    // Skip "size" and any other attributes.
    //
    Walker = AsciiStrStr (Walker, ">");
    ASSERT (Walker != NULL);
    ++Walker;
    //
    // Temporarily terminate after the integer definition.
    //
    Walker2 = AsciiStrStr (Walker, "<");
    ASSERT (Walker2 != NULL);
    *Walker2 = '\0';
    //
    // Assert that the prelinked PLIST always uses hexadecimal integers.
    //
    while ((*Walker == ' ') || (*Walker == '\t')) {
      ++Walker;
    }
    ASSERT ((Walker[1] == 'x') || (Walker[1] == 'X'));
    TempAddress = AsciiStrHexToUint64 (Walker);
    ASSERT (TempAddress != 0);
    ASSERT (TempAddress != MAX_UINT64);
    Walker = (Walker2 + (ARRAY_SIZE ("0x0</integer>") - 1));
    //
    // Restore the previously replaced opening brace.
    //
    *Walker2 = '<';

    if (TempAddress > Address) {
      Address = TempAddress;
    }
  }

  ASSERT (Address != 0);

  Address   -= PrelinkedKextsSegment->VirtualAddress;
  Address   += ((UINTN)KernelHeader + PrelinkedKextsSegment->FileOffset);
  KextHeader = (CONST MACH_HEADER_64 *)Address;

  return ALIGN_VALUE (MachoGetLastAddress64 (KextHeader), BASE_4KB);
}

STATIC
CONST CHAR8 *
InternalKextCollectDependencies (
  IN  CONST CHAR8  **BundleLibrariesStr,
  OUT UINTN        *NumberOfDependencies
  )
{
  CONST CHAR8 *Dependencies;
  UINTN       NumDependencies;

  CONST CHAR8 *Walker;
  CHAR8       *DictEnd;

  Dependencies    = NULL;
  NumDependencies = 0;
  DictEnd         = NULL;

  Walker  = *BundleLibrariesStr;
  Walker += (ARRAY_SIZE (OS_BUNDLE_LIBRARIES_STR "</key>") - 1);
  //
  // Retrieve the opening of the OSBundleLibraries key.
  //
  Walker = AsciiStrStr (Walker, "<dict");
  ASSERT (Walker != NULL);
  Walker += (ARRAY_SIZE ("<dict") - 1);
  //
  // Verify the dict is not an empty single tag.
  //
  while ((*Walker == ' ') || (*Walker == '\t')) {
    ++Walker;
  }
  if (*Walker != '/') {
    ASSERT (*Walker == '>');
    ++Walker;
    //
    // Locate the closure of the OSBundleLibraries dict.  Dicts inside
    // OSBundleLibraries are not valid, so searching for the next match is
    // fine.
    //
    DictEnd = AsciiStrStr (Walker, "</dict>");
    ASSERT (DictEnd != NULL);
    //
    // Temporarily terminate the PLIST string at the beginning of the
    // OSBundleLibraries closing tag so that AsciiStrStr() calls do not exceed
    // its bounds.
    //
    DictEnd[0] = '\0';
    //
    // Cache the result to retrieve the dependencies from the prelinked PLIST
    // later.
    //
    Dependencies = AsciiStrStr (Walker, "<key>");
    ASSERT (Dependencies != NULL);

    for (
      Walker = Dependencies;
      Walker != NULL;
      Walker = AsciiStrStr (Walker, "<key>")
      ) {
      ++NumDependencies;
      //
      // A dependency must have a non-empty name to be valid.
      // A valid string-value for the version identifies is mandatory.
      //
      Walker += (ARRAY_SIZE ("<key>x</key><string>x</string>") - 1);
    }
    //
    // Restore the previously replaced opening brace.
    //
    DictEnd[0] = '<';
  }

  if (DictEnd != NULL) {
    *BundleLibrariesStr = (DictEnd + (ARRAY_SIZE ("</dict>") - 1));
  } else {
    *BundleLibrariesStr = Walker;
  }

  *NumberOfDependencies = NumDependencies;

  return Dependencies;
}

STATIC
UINT16
InternalStringToKextVersionWorker (
  IN CONST CHAR8  **VersionString
  )
{
  UINT16      Version;

  CONST CHAR8 *VersionStart;
  CHAR8       *VersionEnd;
  CONST CHAR8 *VersionReturn;

  VersionReturn = NULL;

  VersionStart = *VersionString;
  VersionEnd   = AsciiStrStr (VersionStart, ".");

  if (VersionEnd != NULL) {
    *VersionEnd = '\0';
  }

  Version = (UINT16)AsciiStrDecimalToUintn (VersionStart);

  if (VersionEnd != NULL) {
    *VersionEnd   = '.';
    VersionReturn = (VersionEnd + 1);
    ASSERT (*VersionReturn != '\0');
  }

  *VersionString = VersionReturn;

  return Version;
}

STATIC
UINT64
InternalStringToKextVersion (
  IN CONST CHAR8  *VersionString
  )
{
  OC_KEXT_VERSION KextVersion;
  CONST CHAR8     *Walker;

  KextVersion.Value = 0;

  Walker = VersionString;

  KextVersion.Bits.Major = InternalStringToKextVersionWorker (&Walker);
  ASSERT (KextVersion.Bits.Major != 0);
  ASSERT (Walker != NULL);

  KextVersion.Bits.Minor = InternalStringToKextVersionWorker (&Walker);
  ASSERT (KextVersion.Bits.Minor != 0);

  if (Walker == NULL) {
    return KextVersion.Value;
  }

  KextVersion.Bits.Revision = InternalStringToKextVersionWorker (&Walker);
  ASSERT (KextVersion.Bits.Revision != 0);

  if (Walker == NULL) {
    return KextVersion.Value;
  }

  switch (Walker[0]) {
    case 'd':
    {
      KextVersion.Bits.Stage = OcKextVersionStageDevelopment;
      break;
    }

    case 'a':
    {
      KextVersion.Bits.Stage = OcKextVersionStageAlpha;
      break;
    }

    case 'b':
    {
      KextVersion.Bits.Stage = OcKextVersionStageBeta;
      break;
    }

    case 'f':
    {
      KextVersion.Bits.Stage = OcKextVersionStageCandidate;

      if (Walker[1] == 'c') {
        ++Walker;
      }
      ASSERT ((Walker[1] >= '0') && (Walker[1] <= '9'));

      break;
    }

    default:
    {
      return 0;
    }
  }

  ++Walker;

  KextVersion.Bits.StageLevel = (UINT16)AsciiStrDecimalToUintn (Walker);
  ASSERT (KextVersion.Bits.StageLevel != 0);
  
  return KextVersion.Value;
}

STATIC
OC_DEPENDENCY_INFO_ENTRY *
InternalKextCollectInformation (
  IN CONST CHAR8  *Plist,
  IN UINT64       RequestedVersion  OPTIONAL
  )
{
  OC_DEPENDENCY_INFO_ENTRY *DependencyEntry;
  OC_DEPENDENCY_INFO       *DependencyInfo;

  CONST CHAR8              *Walker;
  INTN                     Result;

  OC_DEPENDENCY_INFO       DependencyHdr;
  CONST CHAR8              *Dependencies;

  ZeroMem (&DependencyHdr, sizeof (DependencyHdr));
  Dependencies = NULL;

  for (
    Walker = AsciiStrStr (Plist, "<key>");
    Walker != NULL;
    Walker = AsciiStrStr (Walker, "<key>")
    ) {
    if ((DependencyHdr.Name != NULL)
     && (DependencyHdr.Version.Value != 0)
     && (DependencyHdr.CompatibleVersion.Value != 0)
     && (Dependencies != NULL)
      ) {
      DependencyEntry = AllocatePool (
                          sizeof (*DependencyEntry)
                            + (DependencyHdr.NumberOfDependencies
                                * sizeof (*DependencyInfo->Dependencies))
                          );
      if (DependencyEntry != NULL) {
        DependencyEntry->Signature = OC_DEPENDENCY_INFO_ENTRY_SIGNATURE;
        DependencyEntry->Link.ForwardLink = NULL;
        CopyMem (
          &DependencyEntry->Info,
          &DependencyHdr,
          sizeof (DependencyEntry->Info)
          );
        DependencyEntry->Data.SymbolTable = NULL;
        DependencyEntry->Data.Vtables     = NULL;
        DependencyInfo = &DependencyEntry->Info;
        //
        // Temporarily store the Dependency strings there until resolution.
        //
        DependencyInfo->Dependencies[0] = (OC_DEPENDENCY_INFO_ENTRY *)(
                                            Dependencies
                                            );
      }

      return DependencyEntry;
    }

    Walker += (ARRAY_SIZE ("<key>") - 1);
    //
    // Retrieve the KEXT's name.
    //
    if (DependencyHdr.Name == NULL) {
      Result = AsciiStrnCmp (
                 Walker,
                 (OS_BUNDLE_IDENTIFIER_STR "<"),
                 (ARRAY_SIZE (OS_BUNDLE_IDENTIFIER_STR "<") - 1)
                 );
      if (Result == 0) {
        Walker += (ARRAY_SIZE (OS_BUNDLE_IDENTIFIER_STR "</key>") - 1);
        Walker  = AsciiStrStr (Walker, "<string>");
        ASSERT (Walker != NULL);
        Walker += (ARRAY_SIZE ("<string>") - 1);

        DependencyHdr.Name = Walker;

        Walker += (ARRAY_SIZE ("x</string>") - 1);
        continue;
      }
    }
    //
    // Retrieve the KEXT's version.
    //
    if (DependencyHdr.Version.Value == 0) {
      Result = AsciiStrnCmp (
                 Walker,
                 (OS_BUNDLE_COMPATIBLE_VERSION_STR "<"),
                 (ARRAY_SIZE (OS_BUNDLE_COMPATIBLE_VERSION_STR "<") - 1)
                 );
      if (Result == 0) {
        Walker += (ARRAY_SIZE (OS_BUNDLE_COMPATIBLE_VERSION_STR "</key>") - 1);
        Walker  = AsciiStrStr (Walker, "<string>");
        ASSERT (Walker != NULL);
        Walker += (ARRAY_SIZE ("<string>") - 1);

        DependencyHdr.Version.Value = InternalStringToKextVersion (Walker);
        if ((DependencyHdr.Version.Value == 0)
         || ((RequestedVersion != 0)
           && (RequestedVersion > DependencyHdr.Version.Value))
          ) {
          return NULL;
        }

        Walker += (ARRAY_SIZE ("x</string>") - 1);
        continue;
      }
    }
    //
    // Retrieve the KEXT's compatible version.
    //
    if (DependencyHdr.CompatibleVersion.Value == 0) {
      Result = AsciiStrnCmp (
                 Walker,
                 (OS_BUNDLE_COMPATIBLE_VERSION_STR "<"),
                 (ARRAY_SIZE (OS_BUNDLE_COMPATIBLE_VERSION_STR "<") - 1)
                 );
      if (Result == 0) {
        Walker += (ARRAY_SIZE (OS_BUNDLE_COMPATIBLE_VERSION_STR "</key>") - 1);
        Walker  = AsciiStrStr (Walker, "<string>");
        ASSERT (Walker != NULL);
        Walker += (ARRAY_SIZE ("<string>") - 1);

        DependencyHdr.CompatibleVersion.Value = InternalStringToKextVersion (
                                                  Walker
                                                  );
        if ((DependencyHdr.Version.Value == 0)
         || ((RequestedVersion != 0)
          && (RequestedVersion < DependencyHdr.CompatibleVersion.Value))
          ) {
          return NULL;
        }

        Walker += (ARRAY_SIZE ("x</string>") - 1);
        continue;
      }
    }
    //
    // Retrieve the KEXT's dependency information.
    //
    if (Dependencies == NULL) {
      Result = AsciiStrnCmp (
                 Walker,
                 (OS_BUNDLE_LIBRARIES_STR "<"),
                 (ARRAY_SIZE (OS_BUNDLE_LIBRARIES_STR "<") - 1)
                 );
      if (Result == 0) {
        Dependencies = InternalKextCollectDependencies (
                         &Walker,
                         &DependencyHdr.NumberOfDependencies
                         );
        continue;
      }
    }
  }

  return NULL;
}

STATIC
VOID
InternalRemoveDependency (
  IN     UINTN                  NumberOfRequests,
  IN OUT OC_KEXT_REQUEST        *Requests,
  IN     CONST OC_KEXT_REQUEST  *Request
  )
{
  UINTN                          Index;
  UINTN                          Index2;
  CONST OC_DEPENDENCY_INFO_ENTRY *RemoveEntry;
  OC_KEXT_REQUEST                *CurrentRequest;
  OC_DEPENDENCY_INFO_ENTRY       *Entry;
  CONST OC_DEPENDENCY_INFO       *Info;

  RemoveEntry = (CONST OC_DEPENDENCY_INFO_ENTRY *)Request->Private;

  for (Index = 0; Index < NumberOfRequests; ++Index) {
    CurrentRequest = &Requests[Index];
    Entry = (OC_DEPENDENCY_INFO_ENTRY *)CurrentRequest->Private;

    if (Entry == RemoveEntry) {
      continue;
    }

    Info  = (CONST OC_DEPENDENCY_INFO *)&Entry->Info;

    for (Index2 = 0; Index2 < Info->NumberOfDependencies; ++Index2) {
      if (Info->Dependencies[Index2] == RemoveEntry) {
        InternalRemoveDependency (NumberOfRequests, Requests, CurrentRequest);
      }
    }

    if (Entry->Data.SymbolTable != NULL) {
      FreePool (Entry->Data.SymbolTable);
    }

    if (Entry->Data.Vtables != NULL) {
      FreePool (Entry->Data.Vtables);
    }

    FreePool (Entry);
    CurrentRequest->Private = NULL;
  }
}

STATIC
VOID
InternalConstructDependencyArraysWorker (
  IN  UINTN                     NumberOfDependencies,
  IN  OC_DEPENDENCY_INFO_ENTRY  **Dependencies,
  OUT OC_DEPENDENCY_DATA        *DependencyData,
  IN  BOOLEAN                   IsTopLevel
  )
{
  OC_DEPENDENCY_INFO_ENTRY *Dependency;
  OC_DEPENDENCY_INFO       *Info;
  UINTN                    Index;

  for (Index = 0; Index < NumberOfDependencies; ++Index) {
    Dependency = Dependencies[Index];
    Info       = &Dependency->Info;

    if (Dependency->Data.SymbolTable->Link.ForwardLink != NULL) {
      //
      // This dependency already has been added to the lost and henceforth its
      // dependencies as well.
      //
      continue;
    }

    InternalConstructDependencyArraysWorker (
      Info->NumberOfDependencies,
      Info->Dependencies,
      DependencyData,
      FALSE
      );

    InsertHeadList (
      &DependencyData->SymbolTable->Link,
      &Dependency->Data.SymbolTable->Link
      );
    InsertHeadList (
      &DependencyData->Vtables->Link,
      &Dependency->Data.Vtables->Link
      );

    Dependency->Data.SymbolTable->IsIndirect = !IsTopLevel;
  }
}

STATIC
VOID
InternalConstructDependencyArrays (
  IN  UINTN                     NumberOfDependencies,
  IN  OC_DEPENDENCY_INFO_ENTRY  **Dependencies,
  OUT OC_DEPENDENCY_DATA        *DependencyData
  )
{
  OC_DEPENDENCY_INFO *Info;

  Info = &Dependencies[0]->Info;

  DependencyData->SymbolTable = Dependencies[0]->Data.SymbolTable;
  DependencyData->Vtables     = Dependencies[0]->Data.Vtables;

  InitializeListHead (&DependencyData->SymbolTable->Link);
  InitializeListHead (&DependencyData->Vtables->Link);
  DependencyData->SymbolTable->IsIndirect = FALSE;
  //
  // Process the first dependency's dependencies as it itself has already been
  // processed implicitly above.
  //
  InternalConstructDependencyArraysWorker (
    Info->NumberOfDependencies,
    Info->Dependencies,
    DependencyData,
    FALSE
    );
  InternalConstructDependencyArraysWorker (
    (NumberOfDependencies - 1),
    &Dependencies[1],
    DependencyData,
    TRUE
    );
}

STATIC
VOID
InternalDestructDependencyArraysWorker (
  OUT LIST_ENTRY  *ListEntry
  )
{
  do {
    //
    // Only clear ForwardLink as BackLink is not explicitly checked.
    //
    ListEntry->ForwardLink = NULL;
    ListEntry = ListEntry->BackLink;
  } while (ListEntry->ForwardLink != NULL);
}

STATIC
VOID
InternalDestructDependencyArrays (
  OUT CONST OC_DEPENDENCY_DATA *DependencyData
  )
{
  InternalDestructDependencyArraysWorker (
    &DependencyData->SymbolTable->Link
    );
  InternalDestructDependencyArraysWorker (
    &DependencyData->Vtables->Link
    );
}

VOID
OcPrelinkKexts64 (
  IN     CONST MACH_HEADER_64  *KernelHeader,
  IN     UINTN                 NumberOfRequests,
  IN OUT OC_KEXT_REQUEST       *Requests
  )
{
  UINTN                         Index;
  OC_KEXT_REQUEST               *Request;
  OC_DEPENDENCY_INFO_ENTRY      *DependencyInfo;
  OC_DEPENDENCY_INFO            *Info;
  OC_DEPENDENCY_DATA            DependencyData;
  BOOLEAN                       Result;
  UINT64                        LoadAddress;
  CONST MACH_SEGMENT_COMMAND_64 *PrelinkInfoSegment;
  CONST MACH_SECTION_64         *PrelinkInfoSection;
  CONST MACH_SEGMENT_COMMAND_64 *PrelinkTextSegment;
  //
  // Get the Prelinked PLIST data.
  //
  PrelinkInfoSegment = MachoGetSegmentByName64 (
                         KernelHeader,
                         "__PRELINK_INFO"
                         );
  ASSERT (PrelinkInfoSegment != NULL);

  PrelinkInfoSection = MachoGetSectionByName64 (
                         KernelHeader,
                         PrelinkInfoSegment,
                         "__info"
                         );
  ASSERT (PrelinkInfoSection != NULL);
  //
  // Get the segment containing the prelinked KEXT binaries.
  //
  PrelinkTextSegment = MachoGetSegmentByName64 (
                         KernelHeader,
                         "__PRELINK_TEXT"
                         );
  ASSERT (PrelinkTextSegment != NULL);
  //
  // Collect the KEXT information for all KEXTs to be prelinked.  Do not solve
  // dependencies yet so we can verify successful dependency against other
  // KEXTs to be prelinked in one go.
  //
  for (Index = 0; Index < NumberOfRequests; ++Index) {
    Request = &Requests[Index];
    Request->Private = InternalKextCollectInformation (Request->Plist, 0);
  }
  //
  // Resolve dependencies.
  //
  for (Index = 0; Index < NumberOfRequests; ++Index) {
    Request = &Requests[Index];
    if (Request->Private != NULL) {
      // resolve deps
      if (FALSE) {
        InternalRemoveDependency (Index, Requests, Request);
      }
    }
  }
  //
  // Link the KEXTs.
  //
  for (Index = 0; Index < NumberOfRequests; ++Index) {
    Request        = &Requests[Index];
    DependencyInfo = (OC_DEPENDENCY_INFO_ENTRY *)Request->Private;
    Info           = &DependencyInfo->Info;
    //
    // Retrieve a loading address for the KEXT.
    //
    LoadAddress = InternalGetNewPrelinkedKextLoadAddress (
                    KernelHeader,
                    PrelinkTextSegment,
                    (CONST CHAR8 *)(
                      (UINTN)KernelHeader + PrelinkInfoSection->Offset
                      )
                    );
    if (LoadAddress == 0) {
      InternalRemoveDependency (Index, Requests, Request);
      continue;
    }
    InternalConstructDependencyArrays (
      Info->NumberOfDependencies,
      Info->Dependencies,
      &DependencyData
      );
    Result = InternalPrelinkKext64 (
               NULL,
               LoadAddress,
               &DependencyData,
               0,
               NULL,
               NULL,
               NULL
               );
    InternalDestructDependencyArrays (&DependencyData);

    if (!Result) {
      InternalRemoveDependency (Index, Requests, Request);
    }
  }
}

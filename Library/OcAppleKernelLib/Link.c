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
#include <IndustryStandard/AppleKmodInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcAppleKernelLib.h>

#include "PrelinkedInternal.h"

#define TEXT_SEG_PROT (MACH_SEGMENT_VM_PROT_READ | MACH_SEGMENT_VM_PROT_EXECUTE)
#define DATA_SEG_PROT (MACH_SEGMENT_VM_PROT_READ | MACH_SEGMENT_VM_PROT_WRITE)

//
// Symbols
//

STATIC
CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolWorker (
  IN PRELINKED_CONTEXT                *Context,
  IN PRELINKED_KEXT                   *Kext,
  IN UINT64                           PredicateContext,
  IN PRELINKED_KEXT_SYMBOL_PREDICATE  Predicate,
  IN OC_GET_SYMBOL_LEVEL              SymbolLevel,
  IN UINT32                           RecursionLevel
  )
{
  CONST PRELINKED_KEXT_SYMBOL *Symbol;

  UINTN                       Index;
  PRELINKED_KEXT              *Dependency;
  CONST PRELINKED_KEXT_SYMBOL *Symbols;
  UINT32                      NumSymbols;
  UINT32                      CxxIndex;
  UINT32                      SymIndex;
  BOOLEAN                     Result;

  ASSERT (Kext != NULL);

  Symbol = NULL;

  if (SymbolLevel == OcGetSymbolOnlyCxx) {
    CxxIndex = (Kext->NumberOfSymbols - Kext->NumberOfCxxSymbols);
    Symbols  = &Kext->LinkedSymbolTable[CxxIndex];

    for (SymIndex = 0; SymIndex < Kext->NumberOfCxxSymbols; ++SymIndex) {
      Result = Predicate (Kext, &Symbols[SymIndex], PredicateContext);
      if (Result) {
        Symbol = &Symbols[SymIndex];
        break;
      }
    }
  }

  for (Index = 0; Symbol == NULL && Index < ARRAY_SIZE (Kext->Dependencies); ++Index) {
    Dependency = Kext->Dependencies[Index];
    if (Dependency == NULL) {
      break;
    }

    if (Dependency->Processed) {
      continue;
    }

    //
    // Block any 1+ level dependencies.
    //
    Dependency->Processed = TRUE;

    NumSymbols = Dependency->NumberOfSymbols;
    Symbols    = Dependency->LinkedSymbolTable;
    //
    // A value of 2 will only be passed from within this function, the value is
    // reserved to indicate the iteration of an indirect dependency.
    //
    if (SymbolLevel == OcGetSymbolOnlyCxx) {
      //
      // Only consider C++ symbols for indirect dependencies.
      //
      CxxIndex   = (NumSymbols - Dependency->NumberOfCxxSymbols);
      Symbols    = &Symbols[CxxIndex];
      NumSymbols = Dependency->NumberOfCxxSymbols;
    }

    for (SymIndex = 0; SymIndex < NumSymbols; ++SymIndex) {
      Result = Predicate (Dependency, &Symbols[SymIndex], PredicateContext);
      if (Result) {
        Symbol = &Symbols[SymIndex];
        break;
      }
    }

    if (Symbol == NULL && SymbolLevel == OcGetSymbolAnyLevel) {
      Symbol = InternalOcGetSymbolWorker (
                 Context,
                 Dependency,
                 PredicateContext,
                 Predicate,
                 OcGetSymbolOnlyCxx,
                 RecursionLevel + 1
                 );
    }
  }

  if (RecursionLevel == 0) {
    InternalUnlockContextKexts (Context);
  }

  return Symbol;
}

STATIC
BOOLEAN
InternalOcGetSymbolByNamePredicate (
  IN CONST PRELINKED_KEXT         *Kext,
  IN CONST PRELINKED_KEXT_SYMBOL  *Symbol,
  IN       UINT64                 Context
  )
{
  INTN Result;

  Result = AsciiStrCmp (
             (CHAR8 *)(UINTN)Context,
             (Kext->StringTable + Symbol->StringIndex)
             );
  return (Result == 0);
}

CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolByName (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN CONST CHAR8          *Name,
  IN OC_GET_SYMBOL_LEVEL  SymbolLevel
  )
{
  return InternalOcGetSymbolWorker (
           Context,
           Kext,
           (UINTN)Name,
           InternalOcGetSymbolByNamePredicate,
           SymbolLevel,
           0
           );
}

STATIC
BOOLEAN
InternalOcGetSymbolByValuePredicate (
  IN CONST PRELINKED_KEXT         *Kext,
  IN CONST PRELINKED_KEXT_SYMBOL  *Symbol,
  IN       UINT64                 Context
  )
{
  return (Symbol->Value == Context);
}

CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolByValue (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN UINT64               Value,
  IN OC_GET_SYMBOL_LEVEL  SymbolLevel
  )
{
  return InternalOcGetSymbolWorker (
           Context,
           Kext,
           Value,
           InternalOcGetSymbolByValuePredicate,
           SymbolLevel,
           0
           );
}

/**
  Patches Symbol with Value and marks it as solved.

  @param[in]  Value   The value to solve Symbol with.
  @param[out] Symbol  The symbol to solve.

**/
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

/**
  Worker function to solve Symbol against the specified DefinedSymbols.
  It does not consider Symbol might be a weak reference.

  @param[in,out] MachoContext    Context of the Mach-O.
  @param[in]     DefinedSymbols  List of defined symbols of all dependencies.
  @param[in]     Name            The name of the symbol to resolve against.
  @param[in,out] Symbol          The symbol to be resolved.

  @retval  Returned is whether the symbol was solved successfully.

**/
STATIC
BOOLEAN
InternalSolveSymbolNonWeak64 (
  IN     PRELINKED_CONTEXT         *Context,
  IN     PRELINKED_KEXT            *Kext,
  IN     CONST CHAR8               *Name,
  IN OUT MACH_NLIST_64             *Symbol
  )
{
  INTN                        Result;
  CONST PRELINKED_KEXT_SYMBOL *ResolveSymbol;

  if ((Symbol->Type & MACH_N_TYPE_TYPE) != MACH_N_TYPE_UNDF) {
    if ((Symbol->Type & MACH_N_TYPE_TYPE) != MACH_N_TYPE_INDR) {
      //
      // KXLD_WEAK_TEST_SYMBOL might have been resolved by the resolving code
      // at the end of InternalSolveSymbol64. 
      //
      Result = AsciiStrCmp (
                 MachoGetSymbolName64 (&Kext->Context.MachContext, Symbol),
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
      return FALSE;
    }
  } else if (Symbol->Value != 0) {
    //
    // Common symbols are not supported.
    //
    return FALSE;
  }
  //
  // Do not error when the referenced symbol cannot be found as some will be
  // patched by the VTable code.  This matches the KXLD behaviour.
  //
  ResolveSymbol = InternalOcGetSymbolByName (
                    Context,
                    Kext,
                    Name,
                    OcGetSymbolFirstLevel
                    );
  if (ResolveSymbol != NULL) {
    InternalSolveSymbolValue64 (ResolveSymbol->Value, Symbol);
  }

  return TRUE;
}

/**
  Solves Symbol against the specified DefinedSymbols.

  @param[in]     MachoContext     Context of the Mach-O.
  @param[in]     DefinedSymbols   List of defined symbols of all dependencies.
  @param[in]     Name             The name of the symbol to resolve against.
  @param[in,out] Symbol           The symbol to be resolved.
  @param[in,out] WeakTestValue    Points to the value of the Weak test symbol.
                                  If it is 0 on input, it will get assigned
                                  the value on the first resolution of a weak
                                  symbol.
  @param[in] UndefinedSymbols     Array of the Mach-O'S undefined symbols.
  @param[in] NumUndefinedSymbols  Number of symbols in UndefinedSymbols.

  @retval  Returned is whether the symbol was solved successfully.  For weak
           symbols, this includes solving with _gOSKextUnresolved.

**/
STATIC
BOOLEAN
InternalSolveSymbol64 (
  IN     PRELINKED_CONTEXT         *Context,
  IN     PRELINKED_KEXT            *Kext,
  IN     CONST CHAR8               *Name,
  IN OUT MACH_NLIST_64             *Symbol,
  IN OUT UINT64                    *WeakTestValue,
  IN     CONST MACH_NLIST_64       *UndefinedSymbols,
  IN     UINT32                    NumUndefinedSymbols
  )
{
  BOOLEAN             Success;
  UINT32              Index;
  INTN                Result;
  UINT64              Value;
  CONST MACH_NLIST_64 *WeakTestSymbol;

  ASSERT (Symbol != NULL);
  if (NumUndefinedSymbols != 0) {
    ASSERT (UndefinedSymbols != NULL);
  }

  Success = InternalSolveSymbolNonWeak64 (
              Context,
              Kext,
              Name,
              Symbol
              );
  if (Success) {
    return TRUE;
  }

  if (((Symbol->Type & MACH_N_TYPE_STAB) == 0)
   && ((Symbol->Descriptor & MACH_N_WEAK_DEF) != 0)) {
    //
    // KXLD_WEAK_TEST_SYMBOL is not going to be defined or exposed by a KEXT
    // prelinked by this library, hence only check the undefined symbols region
    // for matches.
    //
    Value = *WeakTestValue;

    if (Value == 0) {
      for (Index = 0; Index < NumUndefinedSymbols; ++Index) {
        WeakTestSymbol = &UndefinedSymbols[Index];
        Result = AsciiStrCmp (
                   MachoGetSymbolName64 (
                     &Kext->Context.MachContext,
                     WeakTestSymbol
                     ),
                   KXLD_WEAK_TEST_SYMBOL
                   );
        if (Result == 0) {
          if ((WeakTestSymbol->Type & MACH_N_TYPE_TYPE) == MACH_N_TYPE_UNDF) {
            Success = InternalSolveSymbolNonWeak64 (
                        Context,
                        Kext,
                        Name,
                        Symbol
                        );
            if (!Success) {
              return FALSE;
            }
          }

          *WeakTestValue = WeakTestSymbol->Value;
          break;
        }
      }

      if (Value != 0) {
        InternalSolveSymbolValue64 (Value, Symbol);
        return TRUE;
      }
    }
  }

  return FALSE;
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

  @param[in]  MachoContext     Mach-O context of the KEXT to relocate.
  @param[in]  LoadAddress      The address to be linked against.
  @param[in]  Vtables          List of all dependent VTables.
  @param[in]  Relocation       The Relocation to be resolved.
  @param[in]  NextRelocation   The Relocation following Relocation.
  @param[out] Target           Relocation's target address.
  @param[out] PairTarget       NextRelocation's target address.
  @param[out] Vtable           The VTable described by the symbol referenced by
                               Relocation.  NULL, if there is none.

  @returns  Whether the operation was completed successfully.

**/
STATIC
BOOLEAN
InternalCalculateTargetsIntel64 (
  IN     PRELINKED_CONTEXT           *Context,
  IN OUT PRELINKED_KEXT              *Kext,
  IN     UINT64                      LoadAddress,
  IN     CONST MACH_RELOCATION_INFO  *Relocation,
  IN     CONST MACH_RELOCATION_INFO  *NextRelocation  OPTIONAL,
  OUT    UINT64                      *Target,
  OUT    UINT64                      *PairTarget,
  OUT    CONST PRELINKED_VTABLE      **Vtable  OPTIONAL
  )
{
  BOOLEAN               Success;

  OC_MACHO_CONTEXT      *MachoContext;

  UINT64                TargetAddress;
  MACH_NLIST_64         *Symbol;
  CONST CHAR8           *Name;
  MACH_SECTION_64       *Section;
  UINT64                PairAddress;
  UINT64                PairDummy;

  ASSERT (Target != NULL);
  ASSERT (PairTarget != NULL);

  MachoContext = &Kext->Context.MachContext;

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
  // (link_addr - base_addr), which should be LoadAddress aligned on the
  // section's boundary.
  //
  if (Relocation->Extern != 0) {
    Symbol = MachoGetSymbolByIndex64 (
                MachoContext,
                Relocation->SymbolNumber
                );
    if (Symbol == NULL) {
      return FALSE;
    }

    Name = MachoGetSymbolName64 (MachoContext, Symbol);
    //
    // If this symbol is a padslot that has already been replaced, then the
    // only way a relocation entry can still reference it is if there is a
    // vtable that has not been patched.  The vtable patcher uses the
    // MetaClass structure to find classes for patching, so an unpatched
    // vtable means that there is an OSObject-dervied class that is missing
    // its OSDeclare/OSDefine macros.
    // - FIXME: This cannot currently be checked with the means of this
    //          library.  KXLD creates copies of patched VTable symbols, marks
    //          the originals patched and then updates the referencing reloc.
    //

    if ((Vtable != NULL) && MachoSymbolNameIsVtable64 (Name)) {
      *Vtable = InternalGetOcVtableByName (Context, Kext, Name, 0);
    }

    TargetAddress = Symbol->Value;
  } else {
    Section = MachoGetSectionByIndex64 (
                MachoContext,
                Relocation->SymbolNumber
                );
    if (Section == NULL) {
      return FALSE;
    }

    TargetAddress = ALIGN_VALUE (
                      (Section->Address + LoadAddress),
                      (1ULL << Section->Alignment)
                      );
    TargetAddress -= Section->Address;
  }

  if (MachoRelocationIsPairIntel64 ((UINT8)Relocation->Type)) {
    if (NextRelocation == NULL) {
      return FALSE;
    }
    //
    // As this relocation is the second one in a pair, it cannot be the start
    // of a pair itself.  Pass dummy data for the related arguments.  This call
    // shall never reach this very branch.
    //
    Success = InternalCalculateTargetsIntel64 (
                Context,
                Kext,
                LoadAddress,
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
  IN CONST PRELINKED_VTABLE  *Vtable,
  IN UINT64                  Offset
  )
{
  UINT64                       Index;
  CONST PRELINKED_VTABLE_ENTRY *Entry;

  if ((Offset % sizeof (UINT64)) != 0 || (Offset < VTABLE_ENTRY_SIZE_64)) {
    return FALSE;
  }

  Index = ((Offset - VTABLE_ENTRY_SIZE_64) / sizeof (UINT64));
  Entry = &Vtable->Entries[Index];

  if ((Index >= Vtable->NumEntries) || (Entry->Name == NULL)) {
    return FALSE;
  }

  return MachoSymbolNameIsPureVirtual (Entry->Name);
}

/**
  Relocates Relocation against the specified Symtab's Symbol Table and
  LoadAddress.  This logically matches KXLD's x86_64_process_reloc.

  @param[in] MachoContext     Mach-O context of the KEXT to relocate.
  @param[in] LoadAddress      The address to be linked against.
  @param[in] Vtables          List of all dependent VTables.
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
  IN PRELINKED_CONTEXT           *Context,
  IN PRELINKED_KEXT              *Kext,
  IN UINT64                      LoadAddress,
  IN UINTN                       RelocationBase,
  IN CONST MACH_RELOCATION_INFO  *Relocation,
  IN CONST MACH_RELOCATION_INFO  *NextRelocation  OPTIONAL
  )
{
  UINTN           ReturnValue;

  CONST MACH_HEADER_64 *MachHeader;
  UINT32          MachSize;
  UINT8           Type;
  INT32           Instruction32;
  UINT64          Instruction64;
  UINT64          Target;
  BOOLEAN         IsPair;
  UINT64          PairTarget;
  CONST PRELINKED_VTABLE *Vtable;
  UINT64          LinkPc;
  UINT64          Adjustment;
  UINT32          Length;
  UINT32          Address;
  UINT8           *InstructionPtr;
  BOOLEAN         PcRelative;
  BOOLEAN         IsNormalLocal;
  BOOLEAN         Result;
  BOOLEAN         InvalidPcRel;

  ASSERT (RelocationBase != 0);
  ASSERT (Relocation != NULL);

  IsPair        = FALSE;
  Adjustment    = 0;
  IsNormalLocal = FALSE;

  Address    = Relocation->Address;
  Length     = Relocation->Size;
  Type       = (UINT8)Relocation->Type;
  PcRelative = (Relocation->PcRelative != 0);

  if (Length < 2) {
    return MAX_UINTN;
  }

  if (Relocation->Extern == 0) {
    IsNormalLocal = TRUE;
  }

  LinkPc         = (Address + LoadAddress);
  InstructionPtr = (UINT8 *)(RelocationBase + Address);

  MachHeader = MachoGetMachHeader64 (&Kext->Context.MachContext);
  ASSERT (MachHeader != NULL);

  MachSize = MachoGetFileSize (&Kext->Context.MachContext);

  if (((UINTN)InstructionPtr < (UINTN)MachHeader)
   || (((UINTN)InstructionPtr + ((Length != 3) ? 4 : 8)) > ((UINTN)MachHeader + MachSize))) {
    return MAX_UINTN;
  }

  Vtable = NULL;
  Result = InternalCalculateTargetsIntel64 (
             Context,
             Kext,
             LoadAddress,
             Relocation,
             NextRelocation,
             &Target,
             &PairTarget,
             &Vtable
             );
  if (!Result) {
    return MAX_UINTN;
  }

  InvalidPcRel = FALSE;

  // Length == 2
  if (Length != 3) {
    CopyMem (&Instruction32, InstructionPtr, sizeof (Instruction32));

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
        InvalidPcRel = !PcRelative;
        Adjustment += LinkPc;
        break;
      }

      case MachX8664RelocSigned:
      case MachX8664RelocSigned1:
      case MachX8664RelocSigned2:
      case MachX8664RelocSigned4:
      {
        InvalidPcRel = !PcRelative;
        Adjustment += (IsNormalLocal ? LoadAddress : LinkPc);
        break;
      }

      case MachX8664RelocGot:
      case MachX8664RelocGotLoad:
      {
        InvalidPcRel = !PcRelative;
        Adjustment += LinkPc;
        Target      = PairTarget;
        IsPair      = TRUE;
        break;
      }

      case MachX8664RelocSubtractor:
      {
        InvalidPcRel = PcRelative;
        Instruction32 = (INT32)(Target - PairTarget);
        IsPair        = TRUE;
        break;
      }

      default:
      {
        return MAX_UINTN;
      }
    }

    if (PcRelative) {
      Result = InternalCalculateDisplacementIntel64 (
                 Target,
                 Adjustment,
                 &Instruction32
                 );
      if (!Result) {
        return MAX_UINTN;
      }
    }

    CopyMem (InstructionPtr, &Instruction32, sizeof (Instruction32));
  } else {
    CopyMem (&Instruction64, InstructionPtr, sizeof (Instruction64));

    if ((Vtable != NULL)
     && InternalIsDirectPureVirtualCall64 (Vtable, Instruction64)) {
      return MAX_UINTN;
    }

    switch (Type) {
      case MachX8664RelocUnsigned:
      {
        InvalidPcRel = PcRelative;
        Instruction64 += Target;
        break;
      }

      case MachX8664RelocSubtractor:
      {
        InvalidPcRel = PcRelative;
        Instruction64 = (Target - PairTarget);
        IsPair        = TRUE;
        break;
      }

      default:
      {
        return MAX_UINTN;
      }
    }

    CopyMem (InstructionPtr, &Instruction64, sizeof (Instruction64));
  }

  if (InvalidPcRel) {
    DEBUG ((DEBUG_WARN, "Prelink: Relocation has invalid PC relative flag.\n"));
  }

  ReturnValue = (MachoPreserveRelocationIntel64 (Type) ? 1 : 0);

  if (IsPair) {
    ReturnValue |= BIT31;
  }

  return ReturnValue;
}

/**
  Relocates all Mach-O Relocations and copies the ones to be preserved after
  prelinking to TargetRelocations.

  @param[in]  MachoContext         Mach-O context of the KEXT to relocate.
  @param[in]  LoadAddress          The address to be linked against.
  @param[in]  Vtables              The patched VTables of this KEXT and its
                                   dependencies.
  @param[in]  RelocationBase       The Relocations base address.
  @param[in]  SourceRelocations    The Relocations source buffer.
  @param[in]  NumRelocations       On input, the number of source Relocations.
                                   On output, the number of Relocations to
                                   preserve.
  @param[out] TargetRelocations    The Relocations destination buffer.

  @retval  Returned is the number of preserved Relocations.

**/
STATIC
BOOLEAN
InternalRelocateAndCopyRelocations64 (
  IN  PRELINKED_CONTEXT           *Context,
  IN  PRELINKED_KEXT              *Kext,
  IN  UINT64                      LoadAddress,
  IN  UINTN                       RelocationBase,
  IN  CONST MACH_RELOCATION_INFO  *SourceRelocations,
  IN  OUT UINT32                  *NumRelocations,
  OUT MACH_RELOCATION_INFO        *TargetRelocations
  )
{
  UINT32                     PreservedRelocations;

  UINT32                     Index;
  CONST MACH_RELOCATION_INFO *NextRelocation;
  UINTN                      Result;
  MACH_RELOCATION_INFO       *Relocation;

  ASSERT (Kext != NULL);
  ASSERT (RelocationBase != 0);
  ASSERT (SourceRelocations != NULL);
  ASSERT (NumRelocations != NULL);
  ASSERT (NumRelocations > 0);
  ASSERT (TargetRelocations != NULL);

  PreservedRelocations = 0;

  for (Index = 0; Index < *NumRelocations; ++Index) {
    //
    // Assertion: Not i386.  Scattered Relocations are only supported by i386.
    //            && ((UINT32)Relocation->Address & MACH_RELOC_SCATTERED) == 0
    //
    if ((SourceRelocations[Index].Extern == 0)
     && (SourceRelocations[Index].SymbolNumber == MACH_RELOC_ABSOLUTE)) {
      //
      // A section-based relocation entry can be skipped for absolute 
      // symbols.
      //
      continue;
    }

    NextRelocation = &SourceRelocations[Index + 1];
    //
    // The last Relocation does not have a successor.
    //
    if (Index == (*NumRelocations - 1)) {
      NextRelocation = NULL;
    }
    //
    // Relocate the relocation.
    //
    Result = InternalRelocateRelocationIntel64 (
               Context,
               Kext,
               LoadAddress,
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
        //            This is asserted by KXLD as well.
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

  *NumRelocations = PreservedRelocations;

  return TRUE;
}

//
// MACH header
//

/**
  Strip superfluous Load Commands from the Mach-O header.  This includes the
  Code Signature Load Command which must be removed for the binary has been
  modified by the prelinking routines.

  @param[in,out] MachHeader  Mach-O header to strip the Load Commands from.

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

  UINT32            Index;
  UINT32            Index2;
  MACH_LOAD_COMMAND *LoadCommand;
  UINT32            SizeOfLeftCommands;
  //
  // Delete the Code Signature Load Command if existent as we modified the
  // binary, as well as linker metadata not needed for runtime operation.
  //
  LoadCommand        = MachHeader->Commands;
  SizeOfLeftCommands = MachHeader->CommandsSize;

  for (Index = 0; Index < MachHeader->NumCommands; ++Index) {
    //
    // Assertion: LC_UNIXTHREAD and LC_MAIN are technically stripped in KXLD,
    //            but they are not supposed to be present in the first place.
    //
    if ((LoadCommand->CommandType == MACH_LOAD_COMMAND_UNIX_THREAD)
     || (LoadCommand->CommandType == MACH_LOAD_COMMAND_MAIN)) {
      DEBUG ((DEBUG_WARN, "Prelink: UNIX Thread and Main LCs are unsupported.\n"));
    }

    SizeOfLeftCommands -= LoadCommand->CommandSize;

    for (Index2 = 0; Index2 < ARRAY_SIZE (LoadCommandsToStrip); ++Index2) {
      if (LoadCommand->CommandType == LoadCommandsToStrip[Index2]) {
        if (Index != (MachHeader->NumCommands - 1)) {
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

        --MachHeader->NumCommands;
        MachHeader->CommandsSize -= LoadCommand->CommandSize;

        break;
      }
    }

    LoadCommand = NEXT_MACH_LOAD_COMMAND (LoadCommand);
  }
}

STATIC
BOOLEAN
InternalRelocateSymbols (
  IN     OC_MACHO_CONTEXT  *MachoContext,
  IN     UINT64            LoadAddress,
  IN     UINT32            NumSymbols,
  IN OUT MACH_NLIST_64     *Symbols,
  OUT    UINT32            *KmodInfoOffset
  )
{
  UINT32        Index;
  MACH_NLIST_64 *Symbol;
  CONST CHAR8   *SymbolName;
  BOOLEAN       Result;
  UINT32        KmodOffset;
  UINT32        MaxSize;

  ASSERT (MachoContext != NULL);
  ASSERT (Symbols != NULL);
  ASSERT (KmodInfoOffset != NULL);

  KmodOffset = *KmodInfoOffset;

  for (Index = 0; Index < NumSymbols; ++Index) {
    Symbol = &Symbols[Index];

    if (KmodOffset == 0) {
      SymbolName = MachoGetSymbolName64 (MachoContext, Symbol);
      ASSERT (SymbolName != NULL);

      if (AsciiStrCmp (SymbolName, "_kmod_info") == 0) {
        Result = MachoSymbolGetFileOffset64 (
                   MachoContext,
                   Symbol,
                   &KmodOffset,
                   &MaxSize
                   );
        if (!Result
         || (MaxSize < sizeof (KMOD_INFO_64_V1)
         || (KmodOffset % 4) != 0)) {
          return FALSE;
        }

        *KmodInfoOffset = KmodOffset;
      }
    }

    Result = MachoRelocateSymbol64 (
               MachoContext,
               LoadAddress,
               Symbol
               );
    if (!Result) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Prelinks the specified KEXT against the specified LoadAddress and the data
  of its dependencies.

  @param[in,out] MachoContext     Mach-O context of the KEXT to prelink.
  @param[in]     LinkEditSegment  __LINKEDIT segment of the KEXT to prelink.
  @param[in]     LoadAddress      The address this KEXT shall be linked
                                  against.

  @retval  Returned is whether the prelinking process has been successful.
           The state of the KEXT is undefined in case this routine fails.

**/
RETURN_STATUS
InternalPrelinkKext64 (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     PRELINKED_KEXT     *Kext,
  IN     UINT64             LoadAddress
  )
{
  OC_MACHO_CONTEXT           *MachoContext;
  MACH_SEGMENT_COMMAND_64    *LinkEditSegment;

  MACH_HEADER_64             *MachHeader;

  MACH_SEGMENT_COMMAND_64    *Segment;
  MACH_SECTION_64            *Section;

  UINT32                     Index;
  BOOLEAN                    Result;
  UINT32                     SymtabSize;
  UINT32                     SymtabSize2;

  MACH_SYMTAB_COMMAND        *Symtab;
  MACH_DYSYMTAB_COMMAND      *DySymtab;
  MACH_NLIST_64              *Symbol;
  CONST CHAR8                *SymbolName;
  CONST MACH_NLIST_64        *SymbolTable;
  CONST CHAR8                *StringTable;
  UINT32                     NumSymbols;
  CONST MACH_NLIST_64        *IndirectSymtab;
  UINT32                     NumIndirectSymbols;
  CONST MACH_NLIST_64        *LocalSymtab;
  UINT32                     NumLocalSymbols;
  CONST MACH_NLIST_64        *ExternalSymtab;
  UINT32                     NumExternalSymbols;
  CONST MACH_NLIST_64        *UndefinedSymtab;
  UINT32                     NumUndefinedSymbols;
  UINT64                     WeakTestValue;
  OC_VTABLE_PATCH_ARRAY      *PatchData;

  UINT32                     NumRelocations;
  UINT32                     NumRelocations2;
  UINTN                      RelocationBase;
  CONST MACH_RELOCATION_INFO *Relocations;
  MACH_RELOCATION_INFO       *TargetRelocation;
  MACH_SEGMENT_COMMAND_64    *FirstSegment;

  VOID                       *LinkEdit;
  UINT32                     LinkEditSize;
  UINT32                     SymbolTableOffset;
  UINT32                     SymbolTableSize;
  UINT32                     RelocationsOffset;
  UINT32                     RelocationsSize;
  UINT32                     StringTableOffset;
  UINT32                     StringTableSize;

  UINT32                     SegmentOffset;
  UINT32                     SegmentSize;

  UINT64                     SegmentVmSizes;
  UINT32                     KmodInfoOffset;
  KMOD_INFO_64_V1            *KmodInfo;

  ASSERT (Context != NULL);
  ASSERT (Kext != NULL);
  ASSERT (LoadAddress != 0);

  MachoContext    = &Kext->Context.MachContext;
  LinkEditSegment = Kext->LinkEditSegment;

  MachHeader = MachoGetMachHeader64 (MachoContext);
  //
  // Only perform actions when the kext is flag'd to be dynamically linked.
  //
  if ((MachHeader->Flags & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) == 0) {
    return RETURN_SUCCESS;
  }
  //
  // Retrieve the symbol tables required for most following operations.
  //
  NumSymbols = MachoGetSymbolTable (
                 MachoContext,
                 &SymbolTable,
                 &StringTable,
                 &LocalSymtab,
                 &NumLocalSymbols,
                 &ExternalSymtab,
                 &NumExternalSymbols,
                 &UndefinedSymtab,
                 &NumUndefinedSymbols
                 );
  //
  // Solve indirect symbols.
  //
  WeakTestValue      = 0;
  NumIndirectSymbols = MachoGetIndirectSymbolTable (
                         MachoContext,
                         &IndirectSymtab
                         );
  for (Index = 0; Index < NumIndirectSymbols; ++Index) {
    Symbol     = (MACH_NLIST_64 *)&IndirectSymtab[Index];
    SymbolName = MachoGetIndirectSymbolName64 (MachoContext, Symbol);
    if (SymbolName == NULL) {
      return RETURN_LOAD_ERROR;
    }

    Result = InternalSolveSymbol64 (
               Context,
               Kext,
               SymbolName,
               Symbol,
               &WeakTestValue,
               UndefinedSymtab,
               NumUndefinedSymbols
               );
    if (!Result) {
      return RETURN_LOAD_ERROR;
    }
  }
  //
  // Solve undefined symbols.
  //
  for (Index = 0; Index < NumUndefinedSymbols; ++Index) {
    Symbol = (MACH_NLIST_64 *)&UndefinedSymtab[Index];
    //
    // Undefined symbols are solved via their name.
    //
    Result = InternalSolveSymbol64 (
               Context,
               Kext,
               MachoGetSymbolName64 (MachoContext, Symbol),
               Symbol,
               &WeakTestValue,
               UndefinedSymtab,
               NumUndefinedSymbols
               );
    if (!Result) {
      return RETURN_LOAD_ERROR;
    }
  }
  //
  // Create and patch the KEXT's VTables.
  // ScratchMemory is at least as big as __LINKEDIT, so it can store all
  // symbols.
  //
  PatchData = (OC_VTABLE_PATCH_ARRAY *)Context->LinkBuffer;
  Result = InternalPrepareVtableCreationNonPrelinked64 (
             MachoContext,
             NumSymbols,
             SymbolTable,
             PatchData
             );
  if (!Result) {
    return RETURN_LOAD_ERROR;
  }

  Result = InternalPatchByVtables64 (Context, Kext, PatchData);
  if (!Result) {
    return RETURN_LOAD_ERROR;
  }
  //
  // Relocate local and external symbols.
  //
  KmodInfoOffset = 0;

  Result = InternalRelocateSymbols (
             MachoContext,
             LoadAddress,
             NumLocalSymbols,
             (MACH_NLIST_64 *)LocalSymtab,
             &KmodInfoOffset
             );
  if (!Result) {
    return RETURN_LOAD_ERROR;
  }

  Result = InternalRelocateSymbols (
             MachoContext,
             LoadAddress,
             NumExternalSymbols,
             (MACH_NLIST_64 *)ExternalSymtab,
             &KmodInfoOffset
             );
  if (!Result || (KmodInfoOffset == 0)) {
    return RETURN_LOAD_ERROR;
  }

  KmodInfo = (KMOD_INFO_64_V1 *)((UINTN)MachHeader + (UINTN)KmodInfoOffset);
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
  SymbolTableOffset = 0;
  SymbolTableSize   = (NumSymbols - NumUndefinedSymbols);
  SymbolTableSize  *= sizeof (MACH_NLIST_64);
  RelocationsOffset = (SymbolTableOffset + SymbolTableSize);
  StringTableSize   = MachoContext->Symtab->StringsSize;
  //
  // For the allocation, assume all relocations will be preserved to simplify
  // the code, the memory is only temporarily allocated anyway.
  //
  NumRelocations  = MachoContext->DySymtab->NumOfLocalRelocations;
  NumRelocations += MachoContext->DySymtab->NumExternalRelocations;
  RelocationsSize = (NumRelocations * sizeof (MACH_RELOCATION_INFO));

  LinkEdit = Context->LinkBuffer;

  FirstSegment = MachoGetNextSegment64 (MachoContext, NULL);
  if (FirstSegment == NULL) {
    return RETURN_UNSUPPORTED;
  }
  //
  // Copy the relocations to be reserved and adapt the symbol number they
  // reference in case it has been relocated above.
  //
  TargetRelocation = (MACH_RELOCATION_INFO *)(
                       (UINTN)LinkEdit + RelocationsOffset
                       );
  RelocationBase = ((UINTN)MachHeader + (UINTN)FirstSegment->FileOffset);
  //
  // Relocate and copy local and external relocations.
  //
  Relocations    = MachoContext->LocalRelocations;
  NumRelocations = MachoContext->DySymtab->NumOfLocalRelocations;
  Result = InternalRelocateAndCopyRelocations64 (
             Context,
             Kext,
             LoadAddress,
             RelocationBase,
             Relocations,
             &NumRelocations,
             &TargetRelocation[0]
             );
  if (!Result) {
    return RETURN_LOAD_ERROR;
  }

  Relocations     = MachoContext->ExternRelocations;
  NumRelocations2 = MachoContext->DySymtab->NumExternalRelocations;
  Result = InternalRelocateAndCopyRelocations64 (
             Context,
             Kext,
             LoadAddress,
             RelocationBase,
             Relocations,
             &NumRelocations2,
             &TargetRelocation[NumRelocations]
             );
  if (!Result) {
    return RETURN_LOAD_ERROR;
  }
  NumRelocations += NumRelocations2;
  //
  // Copy the entire symbol table excluding the area for undefined symbols.
  //
  SymtabSize = (UINT32)((UndefinedSymtab - SymbolTable) * sizeof (MACH_NLIST_64));
  if (SymtabSize != 0) {
    CopyMem (
      (VOID *)((UINTN)LinkEdit + SymbolTableOffset),
      SymbolTable,
      SymtabSize
      );
  }

  SymtabSize2  = (UINT32)(&SymbolTable[NumSymbols] - &UndefinedSymtab[NumUndefinedSymbols]);
  SymtabSize2 *= sizeof (MACH_NLIST_64);
  if (SymtabSize2 != 0) {
    CopyMem (
      (VOID *)((UINTN)LinkEdit + SymbolTableOffset + SymtabSize),
      (VOID *)&UndefinedSymtab[NumUndefinedSymbols],
      SymtabSize2
      );
  }

  NumSymbols -= NumUndefinedSymbols;
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
  Symtab   = MachoContext->Symtab;
  DySymtab = MachoContext->DySymtab;

  Symtab->SymbolsOffset = (UINT32)(LinkEditSegment->FileOffset + SymbolTableOffset);
  Symtab->NumSymbols    = NumSymbols;
  Symtab->StringsOffset = (UINT32)(LinkEditSegment->FileOffset + StringTableOffset);

  DySymtab->LocalRelocationsOffset = (UINT32)(LinkEditSegment->FileOffset + RelocationsOffset);
  DySymtab->NumOfLocalRelocations  = NumRelocations;
  //
  // Clear dynamic linker information.
  //
  DySymtab->LocalSymbolsIndex      = 0;
  DySymtab->NumLocalSymbols        = 0;
  DySymtab->NumExternalSymbols     = 0;
  DySymtab->ExternalSymbolsIndex   = 0;
  DySymtab->NumExternalRelocations = 0;
  DySymtab->UndefinedSymbolsIndex  = 0;
  DySymtab->NumUndefinedSymbols    = 0;
  DySymtab->IndirectSymbolsOffset  = 0;
  DySymtab->NumIndirectSymbols     = 0;
  //
  // Copy the new __LINKEDIT segment into the binary and fix its Load Command.
  //
  LinkEditSize = (SymbolTableSize + RelocationsSize + StringTableSize);
  
  CopyMem (
    (VOID *)((UINTN)MachHeader + (UINTN)LinkEditSegment->FileOffset),
    LinkEdit,
    LinkEditSize
    );
  ZeroMem (
    (VOID *)((UINTN)MachHeader + (UINTN)LinkEditSegment->FileOffset + LinkEditSize),
    (LinkEditSegment->FileSize - LinkEditSize)
    );

  LinkEditSegment->FileSize = LinkEditSize;
  LinkEditSegment->Size     = ALIGN_VALUE (LinkEditSize, BASE_4KB);
  //
  // Adapt the link addresses of all Segments and their Sections.
  //
  SegmentOffset = 0;
  SegmentSize   = 0;

  SegmentVmSizes = 0;

  Segment = NULL;
  while ((Segment = MachoGetNextSegment64 (MachoContext, Segment)) != NULL) {
    Section = NULL;
    while ((Section = MachoGetNextSection64 (MachoContext, Segment, Section)) != NULL) {
      Section->Address = ALIGN_VALUE (
                           (Section->Address + LoadAddress),
                           (1ULL << Section->Alignment)
                           );
    }

    Segment->VirtualAddress += LoadAddress;

    //
    // Logically equivalent to kxld_seg_set_vm_protections.
    // Assertion: Not i386 (strict protection).
    //
    if (AsciiStrnCmp (Segment->SegmentName, "__TEXT", ARRAY_SIZE (Segment->SegmentName)) == 0) {
      Segment->InitialProtection = TEXT_SEG_PROT;
      Segment->MaximumProtection = TEXT_SEG_PROT;
    } else {
      Segment->InitialProtection = DATA_SEG_PROT;
      Segment->MaximumProtection = DATA_SEG_PROT;
    }

    if (Segment->FileOffset > SegmentOffset) {
      SegmentOffset = (UINT32)Segment->FileOffset;
      SegmentSize   = (UINT32)Segment->FileSize;
    }

    SegmentVmSizes += Segment->Size;
  }
  //
  // Populate kmod information.
  //
  KmodInfo->Address = LoadAddress;
  //
  // This is a hack borrowed from XNU. Real header size is equal to:
  //   sizeof (*MachHeader) + MachHeader->CommandsSize (often aligned to 4096)
  // However, it cannot be set to this value unless it exists in a separate segment,
  // and presently it is not the case on macOS. When header is put to __TEXT (as usual),
  // XNU makes it read only, and this prevents __TEXT from gaining executable permission.
  // See OSKext::setVMAttributes.
  //
  KmodInfo->HdrSize = 0;
  KmodInfo->Size    = KmodInfo->HdrSize + SegmentVmSizes;
  //
  // Adapt the Mach-O header to signal being prelinked.
  //
  MachHeader->Flags = MACH_HEADER_FLAG_NO_UNDEFINED_REFERENCES;
  //
  // Strip superfluous Load Commands.
  //
  InternalStripLoadCommands64 (MachHeader);
  //
  // Reinitialize the Mach-O context to account for the changed __LINKEDIT
  // segment and file size.
  //
  MachoInitializeContext (
    MachoContext,
    MachHeader,
    (SegmentOffset + SegmentSize)
    );
  return RETURN_SUCCESS;
}

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

#include <IndustryStandard/AppleKmodInfo.h>
#include <IndustryStandard/AppleMachoImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>

#include <Library/OcFileLib.h>

#include "PrelinkedInternal.h"

#define TEXT_SEG_PROT (MACH_SEGMENT_VM_PROT_READ | MACH_SEGMENT_VM_PROT_EXECUTE)
#define DATA_SEG_PROT (MACH_SEGMENT_VM_PROT_READ | MACH_SEGMENT_VM_PROT_WRITE)

//
// Symbols
//

STATIC
CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolWorkerName (
  IN PRELINKED_KEXT                   *Kext,
  IN CONST CHAR8                      *LookupValue,
  IN UINT32                           LookupValueLength,
  IN OC_GET_SYMBOL_LEVEL              SymbolLevel
  )
{
  PRELINKED_KEXT              *Dependency;
  CONST PRELINKED_KEXT_SYMBOL *Symbols;
  CONST PRELINKED_KEXT_SYMBOL *SymbolsEnd;
  UINT32                      Index;
  UINT32                      NumSymbols;

  //
  // Block any 1+ level dependencies.
  //
  Kext->Processed = TRUE;

  if (Kext->LinkedSymbolTable != NULL) {
    NumSymbols = Kext->NumberOfSymbols;
    Symbols    = Kext->LinkedSymbolTable;

    if (SymbolLevel == OcGetSymbolOnlyCxx) {
      NumSymbols = Kext->NumberOfCxxSymbols;
      Symbols    = &Kext->LinkedSymbolTable[Kext->NumberOfSymbols - Kext->NumberOfCxxSymbols];
    }

    SymbolsEnd = &Symbols[NumSymbols];
    while (Symbols < SymbolsEnd) {
      //
      // Symbol names often start and end similarly due to C++ mangling (e.g. __ZN).
      // To optimise the lookup we compare their length check in the middle.
      // Please do not change this without careful profiling.
      //
      if (Symbols->Length == LookupValueLength) {
        if (Symbols->Name[LookupValueLength / 2] == LookupValue[LookupValueLength / 2]
          && Symbols->Name[(LookupValueLength / 2) + 1] == LookupValue[(LookupValueLength / 2) + 1]) {
          for (Index = 0; Index < LookupValueLength; ++Index) {
            if (Symbols->Name[Index] != LookupValue[Index]) {
              break;
            }
          }
          if (Index == LookupValueLength) {
            return Symbols;
          }
        }
      }
      Symbols++;
    }
  }

  if (SymbolLevel != OcGetSymbolFirstLevel) {
    for (Index = 0; Index < ARRAY_SIZE (Kext->Dependencies); ++Index) {
      Dependency = Kext->Dependencies[Index];
      if (Dependency == NULL) {
        return NULL;
      }

      if (Dependency->Processed) {
        continue;
      }

      Symbols = InternalOcGetSymbolWorkerName (
                 Dependency,
                 LookupValue,
                 LookupValueLength,
                 OcGetSymbolOnlyCxx
                 );
      if (Symbols != NULL) {
        return Symbols;
      }
    }
  }

  return NULL;
}

STATIC
CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolWorkerValue (
  IN PRELINKED_KEXT                   *Kext,
  IN UINT64                           LookupValue,
  IN OC_GET_SYMBOL_LEVEL              SymbolLevel
  )
{
  PRELINKED_KEXT              *Dependency;
  CONST PRELINKED_KEXT_SYMBOL *Symbols;
  CONST PRELINKED_KEXT_SYMBOL *SymbolsEnd;
  UINT32                      Index;
  UINT32                      NumSymbols;

  //
  // Block any 1+ level dependencies.
  //
  Kext->Processed = TRUE;

  if (Kext->LinkedSymbolTable != NULL) {
    NumSymbols = Kext->NumberOfSymbols;
    Symbols    = Kext->LinkedSymbolTable;

    if (SymbolLevel == OcGetSymbolOnlyCxx) {
      NumSymbols = Kext->NumberOfCxxSymbols;
      Symbols    = &Kext->LinkedSymbolTable[(Kext->NumberOfSymbols - Kext->NumberOfCxxSymbols) & ~15ULL];
    }
    //
    // WARN! Hot path! Do not change this code unless you have decent profiling data.
    // We are not allowed to use SIMD in UEFI, but we can still do better with larger iteration.
    // Up to 15 C symbols extra may get parsed, but it is fine, as they will not match.
    // Increasing the iteration block to more than 16 no longer pays off.
    // Note, lower loop is not on hot path.
    //
    SymbolsEnd = &Symbols[NumSymbols & ~15ULL];
    while (Symbols < SymbolsEnd) {
      #define MATCH(X) if (Symbols[X].Value == LookupValue) { return &Symbols[X]; }
      MATCH (0) MATCH (1) MATCH (2)  MATCH (3)  MATCH (4)  MATCH (5)  MATCH (6)  MATCH (7)
      MATCH (8) MATCH (9) MATCH (10) MATCH (11) MATCH (12) MATCH (13) MATCH (14) MATCH (15)
      #undef MATCH
      Symbols += 16;
    }
  }

  if (SymbolLevel != OcGetSymbolFirstLevel) {
    for (Index = 0; Index < ARRAY_SIZE (Kext->Dependencies); ++Index) {
      Dependency = Kext->Dependencies[Index];
      if (Dependency == NULL) {
        return NULL;
      }

      if (Dependency->Processed) {
        continue;
      }

      Symbols = InternalOcGetSymbolWorkerValue (
                 Dependency,
                 LookupValue,
                 OcGetSymbolOnlyCxx
                 );
      if (Symbols != NULL) {
        return Symbols;
      }
    }
  }

  return NULL;
}

CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolName (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN CONST CHAR8          *LookupValue,
  IN OC_GET_SYMBOL_LEVEL  SymbolLevel
  )
{
  CONST PRELINKED_KEXT_SYMBOL *Symbol;

  PRELINKED_KEXT              *Dependency;
  UINT32                      Index;
  UINT32                      LookupValueLength;

  Symbol = NULL;
  LookupValueLength = (UINT32)AsciiStrLen (LookupValue);

  //
  // Such symbols are illegit, but InternalOcGetSymbolWorkerName assumes Length > 0.
  //
  if (LookupValueLength == 0) {
    return NULL;
  }

  if ((SymbolLevel == OcGetSymbolOnlyCxx) && (Kext->LinkedSymbolTable != NULL)) {
    Symbol = InternalOcGetSymbolWorkerName (
      Kext,
      LookupValue,
      LookupValueLength,
      SymbolLevel
      );
  } else {
    for (Index = 0; Index < ARRAY_SIZE (Kext->Dependencies); ++Index) {
      Dependency = Kext->Dependencies[Index];
      if (Dependency == NULL) {
        break;
      }

      Symbol = InternalOcGetSymbolWorkerName (
                 Dependency,
                 LookupValue,
                 LookupValueLength,
                 SymbolLevel
                 );
      if (Symbol != NULL) {
        break;
      }
    }
  }

  InternalUnlockContextKexts (Context);

  return Symbol;
}

CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolValue (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN UINT64               LookupValue,
  IN OC_GET_SYMBOL_LEVEL  SymbolLevel
  )
{
  CONST PRELINKED_KEXT_SYMBOL *Symbol;

  PRELINKED_KEXT              *Dependency;
  UINT32                      Index;

  Symbol = NULL;

  if ((SymbolLevel == OcGetSymbolOnlyCxx) && (Kext->LinkedSymbolTable != NULL)) {
    Symbol = InternalOcGetSymbolWorkerValue (Kext, LookupValue, SymbolLevel);
  } else {
    for (Index = 0; Index < ARRAY_SIZE (Kext->Dependencies); ++Index) {
      Dependency = Kext->Dependencies[Index];
      if (Dependency == NULL) {
        break;
      }

      Symbol = InternalOcGetSymbolWorkerValue (
                 Dependency,
                 LookupValue,
                 SymbolLevel
                 );
      if (Symbol != NULL) {
        break;
      }
    }
  }

  InternalUnlockContextKexts (Context);

  return Symbol;
}

/**
  Patches Symbol with Value and marks it as solved.

  @param[in]  Is32Bit Symbol is 32-bit.
  @param[in]  Value   The value to solve Symbol with.
  @param[out] Symbol  The symbol to solve.

**/
VOID
InternalSolveSymbolValue (
  IN  BOOLEAN             Is32Bit,
  IN  UINT64              Value,
  OUT MACH_NLIST_ANY      *Symbol
  )
{
  if (Is32Bit) {
    Symbol->Symbol32.Value   = (UINT32) Value;
    Symbol->Symbol32.Type    = (MACH_N_TYPE_ABS | MACH_N_TYPE_EXT);
    Symbol->Symbol32.Section = NO_SECT;
  } else {
    Symbol->Symbol64.Value   = Value;
    Symbol->Symbol64.Type    = (MACH_N_TYPE_ABS | MACH_N_TYPE_EXT);
    Symbol->Symbol64.Section = NO_SECT;
  }
}

/**
  Worker function to solve Symbol against the specified DefinedSymbols.
  It does not consider Symbol might be a weak reference.

  @param[in,out] Context  Prelinking context.
  @param[in]     Kext     KEXT prelinking context.
  @param[in]     Name     The name of the symbol to resolve against.
  @param[in,out] Symbol   The symbol to be resolved.

  @retval  Returned is whether the symbol was solved successfully.

**/
STATIC
BOOLEAN
InternalSolveSymbolNonWeak (
  IN     PRELINKED_CONTEXT         *Context,
  IN     PRELINKED_KEXT            *Kext,
  IN     CONST CHAR8               *Name,
  IN OUT MACH_NLIST_ANY            *Symbol
  )
{
  INTN                        Result;
  CONST PRELINKED_KEXT_SYMBOL *ResolveSymbol;
  UINT8                       SymbolType;

  SymbolType = Context->Is32Bit ? Symbol->Symbol32.Type : Symbol->Symbol64.Type;

  if ((SymbolType & MACH_N_TYPE_TYPE) != MACH_N_TYPE_UNDF) {
    if ((SymbolType & MACH_N_TYPE_TYPE) != MACH_N_TYPE_INDR) {
      //
      // KXLD_WEAK_TEST_SYMBOL might have been resolved by the resolving code
      // at the end of InternalSolveSymbol. 
      //
      Result = AsciiStrCmp (
                 MachoGetSymbolName (&Kext->Context.MachContext, Symbol),
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
  } else if ((Context->Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value) != 0) {
    //
    // Common symbols are not supported.
    //
    return FALSE;
  }
  //
  // Do not error when the referenced symbol cannot be found as some will be
  // patched by the VTable code.  This matches the KXLD behaviour.
  //
  ResolveSymbol = InternalOcGetSymbolName (
                    Context,
                    Kext,
                    Name,
                    OcGetSymbolFirstLevel
                    );
  if (ResolveSymbol != NULL) {
    InternalSolveSymbolValue (Context->Is32Bit, ResolveSymbol->Value, Symbol);
  }

  return TRUE;
}

/**
  Solves Symbol against the specified DefinedSymbols.

  @param[in,out] Context          Prelinking context.
  @param[in]     Kext             KEXT prelinking context.
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
InternalSolveSymbol (
  IN     PRELINKED_CONTEXT         *Context,
  IN     PRELINKED_KEXT            *Kext,
  IN     CONST CHAR8               *Name,
  IN OUT MACH_NLIST_ANY            *Symbol,
  IN OUT UINT64                    *WeakTestValue,
  IN     CONST MACH_NLIST_ANY      *UndefinedSymbols,
  IN     UINT32                    NumUndefinedSymbols
  )
{
  BOOLEAN               Success;
  UINT32                Index;
  INTN                  Result;
  UINT64                Value;
  CONST MACH_NLIST_ANY  *WeakTestSymbol;

  ASSERT (Symbol != NULL);
  ASSERT (UndefinedSymbols != NULL || NumUndefinedSymbols == 0);
  //
  // STAB symbols are not considered undefined.
  //
  if (((Context->Is32Bit ? Symbol->Symbol32.Type : Symbol->Symbol64.Type) & MACH_N_TYPE_STAB) != 0) {
    return TRUE;
  }

  Success = InternalSolveSymbolNonWeak (
              Context,
              Kext,
              Name,
              Symbol
              );
  if (Success) {
    return TRUE;
  }

  if (((Context->Is32Bit ? Symbol->Symbol32.Descriptor : Symbol->Symbol64.Descriptor) & MACH_N_WEAK_DEF) != 0) {
    //
    // KXLD_WEAK_TEST_SYMBOL is not going to be defined or exposed by a KEXT
    // prelinked by this library, hence only check the undefined symbols region
    // for matches.
    //
    Value = *WeakTestValue;

    if (Value == 0) {
      //
      // Fallback to the primary symbol table if we do not have a separate undefined symbol table, but only on 32-bit.
      //
      if (NumUndefinedSymbols > 0) {
        for (Index = 0; Index < NumUndefinedSymbols; ++Index) {
          if (Context->Is32Bit) {
            WeakTestSymbol = (MACH_NLIST_ANY *)&(&UndefinedSymbols->Symbol32)[Index];
          } else {
            WeakTestSymbol = (MACH_NLIST_ANY *)&(&UndefinedSymbols->Symbol64)[Index];
          }
          Result = AsciiStrCmp (
                    MachoGetSymbolName (
                      &Kext->Context.MachContext,
                      WeakTestSymbol
                      ),
                    KXLD_WEAK_TEST_SYMBOL
                    );
          if (Result == 0) {
            if (((Context->Is32Bit ?
              WeakTestSymbol->Symbol32.Descriptor : WeakTestSymbol->Symbol64.Descriptor) & MACH_N_TYPE_TYPE) == MACH_N_TYPE_UNDF) {
              Success = InternalSolveSymbolNonWeak (
                          Context,
                          Kext,
                          Name,
                          Symbol
                          );
              if (!Success) {
                return FALSE;
              }
            }

            Value = Context->Is32Bit ? WeakTestSymbol->Symbol32.Value : WeakTestSymbol->Symbol64.Value;
            *WeakTestValue = Value;
            break;
          }
        }
      } else if (Context->Is32Bit) {
        Index = 0;
        while (TRUE) {
          WeakTestSymbol = MachoGetSymbolByIndex (&Kext->Context.MachContext, Index);
          if (WeakTestSymbol == NULL) {
            break;
          }

          Result = AsciiStrCmp (
                    MachoGetSymbolName (
                      &Kext->Context.MachContext,
                      WeakTestSymbol
                      ),
                    KXLD_WEAK_TEST_SYMBOL
                    );
          if (Result == 0) {
            if (((Context->Is32Bit ?
              WeakTestSymbol->Symbol32.Descriptor : WeakTestSymbol->Symbol64.Descriptor) & MACH_N_TYPE_TYPE) == MACH_N_TYPE_UNDF) {
              Success = InternalSolveSymbolNonWeak (
                          Context,
                          Kext,
                          Name,
                          Symbol
                          );
              if (!Success) {
                return FALSE;
              }
            }

            Value = Context->Is32Bit ? WeakTestSymbol->Symbol32.Value : WeakTestSymbol->Symbol64.Value;
            *WeakTestValue = Value;
            break;
          }

          ++Index;
        }
      }
    }

    if (Value != 0) {
      InternalSolveSymbolValue (Context->Is32Bit, Value, Symbol);
      return TRUE;
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

  @param[in,out] Context          Prelinking context.
  @param[in]     Kext             KEXT prelinking context.
  @param[in]     LoadAddress      The address to be linked against.
  @param[in]     Relocation       The Relocation to be resolved.
  @param[in]     NextRelocation   The Relocation following Relocation.
  @param[out]    Target           Relocation's target address.
  @param[out]    PairTarget       NextRelocation's target address.
  @param[out]    Vtable           The VTable described by the symbol referenced
                                  by Relocation.  NULL, if there is none.

  @returns  Whether the operation was completed successfully.

**/
STATIC
BOOLEAN
InternalCalculateTargets (
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
  MACH_NLIST_ANY        *Symbol;
  CONST CHAR8           *Name;
  MACH_SECTION_ANY      *Section;
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
    Symbol = MachoGetSymbolByIndex (
                MachoContext,
                Relocation->SymbolNumber
                );
    if (Symbol == NULL) {
      return FALSE;
    }

    Name = MachoGetSymbolName (MachoContext, Symbol);
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

    if ((Vtable != NULL) && MachoSymbolNameIsVtable (Name)) {
      *Vtable = InternalGetOcVtableByName (Context, Kext, Name);
    }

    TargetAddress = Context->Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value;
    if (TargetAddress == 0) {
      DEBUG ((DEBUG_INFO, "OCAK: Symbol %a has 0-value\n", Name));
    }
  } else {
    if ((Relocation->SymbolNumber == NO_SECT)
     || (Relocation->SymbolNumber > MAX_SECT)) {
      return FALSE;
    }

    Section = MachoGetSectionByIndex (
                MachoContext,
                (Relocation->SymbolNumber - 1)
                );
    if (Section == NULL) {
      return FALSE;
    }

    if (Context->Is32Bit) {
      TargetAddress = ALIGN_VALUE (
                        (Section->Section32.Address + LoadAddress),
                        LShiftU64 (1, Section->Section32.Alignment)
                        );
      TargetAddress -= Section->Section32.Address;
    } else {
      TargetAddress = ALIGN_VALUE (
                        (Section->Section64.Address + LoadAddress),
                        LShiftU64 (1, Section->Section64.Alignment)
                        );
      TargetAddress -= Section->Section64.Address;
    }
  }

  if (Context->Is32Bit ?
    MachoRelocationIsPairIntel32 ((UINT8) Relocation->Type) :
    MachoRelocationIsPairIntel64 ((UINT8) Relocation->Type)
    ) {
    if (NextRelocation == NULL) {
      return FALSE;
    }
    //
    // As this relocation is the second one in a pair, it cannot be the start
    // of a pair itself.  Pass dummy data for the related arguments.  This call
    // shall never reach this very branch.
    //
    Success = InternalCalculateTargets (
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
InternalIsDirectPureVirtualCall (
  IN BOOLEAN                 Is32Bit,
  IN CONST PRELINKED_VTABLE  *Vtable,
  IN UINT64                  Offset
  )
{
  UINT32                       Remainder;
  UINT64                       Index;
  CONST PRELINKED_VTABLE_ENTRY *Entry;

  DivU64x32Remainder (Offset, Is32Bit ? sizeof (UINT32) : sizeof (UINT64), &Remainder);
  if (Remainder != 0 || Offset < VTABLE_ENTRY_SIZE_X (Is32Bit)) {
    return FALSE;
  }

  Index = DivU64x32 (Offset - VTABLE_ENTRY_SIZE_X (Is32Bit), Is32Bit ? sizeof (UINT32) : sizeof (UINT64));
  if (Index >= Vtable->NumEntries) {
    return FALSE;
  }

  Entry = &Vtable->Entries[Index];
  if (Entry->Name == NULL) {
    return FALSE;
  }

  return MachoSymbolNameIsPureVirtual (Entry->Name);
}

/**
  Relocates Relocation against the specified Symtab's Symbol Table and
  LoadAddress. This logically matches KXLD's generic_process_reloc (for 32-bit)
  and x86_64_process_reloc (for 64-bit).

  @param[in,out] Context         Prelinking context.
  @param[in]     Kext            KEXT prelinking context.
  @param[in]     LoadAddress      The address to be linked against.
  @param[in]     RelocationBase  The Relocations base address.
  @param[in]     Relocation      The Relocation to be processed.
  @param[in]     NextRelocation  The Relocation following Relocation.

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
InternalRelocateRelocation (
  IN PRELINKED_CONTEXT           *Context,
  IN PRELINKED_KEXT              *Kext,
  IN UINT64                      LoadAddress,
  IN UINT64                      RelocationBase,
  IN CONST MACH_RELOCATION_INFO  *Relocation,
  IN CONST MACH_RELOCATION_INFO  *NextRelocation  OPTIONAL
  )
{
  UINTN                     ReturnValue;

  UINT8                     Type;
  INT32                     Instruction32;
  UINT64                    Instruction64;
  UINT64                    Target;
  BOOLEAN                   IsPair;
  UINT64                    PairTarget;
  CONST PRELINKED_VTABLE    *Vtable;
  UINT64                    LinkPc;
  UINT64                    Adjustment;
  UINT32                    Length;
  UINT32                    Address;
  UINT8                     *InstructionPtr;
  UINT32                    MaxSize;
  BOOLEAN                   PcRelative;
  BOOLEAN                   IsNormalLocal;
  BOOLEAN                   Result;
  BOOLEAN                   InvalidPcRel;

  MACH_SCATTERED_RELOCATION_INFO  *ScatteredRelocation;

  ASSERT (Relocation != NULL);

  IsPair        = FALSE;
  Adjustment    = 0;
  IsNormalLocal = FALSE;

  Address    = Relocation->Address;
  Length     = Relocation->Size;
  Type       = (UINT8) Relocation->Type;
  PcRelative = (Relocation->PcRelative != 0);

  InvalidPcRel        = FALSE;
  ScatteredRelocation = NULL;

  //
  // Scattered relocations apply only to 32-bit.
  //
  if (Context->Is32Bit) {
    if (((MACH_SCATTERED_RELOCATION_INFO *) Relocation)->Scattered) {
      ScatteredRelocation = (MACH_SCATTERED_RELOCATION_INFO *) Relocation;

      Address    = ScatteredRelocation->Address;
      Length     = ScatteredRelocation->Size;
      Type       = (UINT8) ScatteredRelocation->Type;
      PcRelative = (ScatteredRelocation->PcRelative != 0);
      Target     = LoadAddress;
    }
  }

  if (Length < 2) {
    return MAX_UINTN;
  }

  InstructionPtr = MachoGetFilePointerByAddress (
                     &Kext->Context.MachContext,
                     (RelocationBase + Address),
                     &MaxSize
                     );
  if ((InstructionPtr == NULL) || (MaxSize < ((Length != 3) ? 4U : 8U))) {
    return MAX_UINTN;
  }

  if (Relocation->Extern == 0) {
    IsNormalLocal = TRUE;
  }

  LinkPc = (Address + LoadAddress);

  Vtable = NULL;
  if (ScatteredRelocation == NULL) {
    Result = InternalCalculateTargets (
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
  }

  // Length == 2
  if (Length != 3) {
    CopyMem (&Instruction32, InstructionPtr, sizeof (Instruction32));

    if ((Vtable != NULL)
     && InternalIsDirectPureVirtualCall (Context->Is32Bit, Vtable, Instruction32)) {
      return MAX_UINTN;
    }

    if (Context->Is32Bit) {
      if (PcRelative) {
        Target = Target + Address - (LinkPc + RelocationBase);
      }

      switch (Type) {
        case MachGenericRelocVanilla:
          Instruction32 += (UINT32) Target;
          break;

        default:
          DEBUG ((DEBUG_ERROR, "OCAK: Non-vanilla 32-bit relocations are unsupported\n")); //FIXME: Implement paired relocs.
          return MAX_UINTN;
      }

    } else {
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
          Adjustment = LShiftU64 (1, Length);
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
    }

    CopyMem (InstructionPtr, &Instruction32, sizeof (Instruction32));
  } else {
    CopyMem (&Instruction64, InstructionPtr, sizeof (Instruction64));

    if ((Vtable != NULL)
     && InternalIsDirectPureVirtualCall (Context->Is32Bit, Vtable, Instruction64)) {
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
    DEBUG ((DEBUG_WARN, "OCAK: Relocation has invalid PC relative flag\n"));
  }

  if (Context->Is32Bit) {
    ReturnValue = 0;
  } else {
    ReturnValue = (MachoPreserveRelocationIntel64 (Type) ? 1 : 0);
  }

  if (IsPair) {
    ReturnValue |= BIT31;
  }

  return ReturnValue;
}

/**
  Relocates all Mach-O Relocations and copies the ones to be preserved after
  prelinking to TargetRelocations.

  @param[in,out] Context            Prelinking context.
  @param[in]     Kext               KEXT prelinking context.
  @param[in]     LoadAddress        The address to be linked against.
                                    dependencies.
  @param[in]     RelocationBase     The Relocations base address.
  @param[in]     SourceRelocations  The Relocations source buffer.
  @param[in]     NumRelocations     On input, the number of source Relocations.
                                    On output, the number of Relocations to
                                    preserve.
  @param[out]    TargetRelocations  The Relocations destination buffer.

  @retval  Returned is the number of preserved Relocations.

**/
STATIC
BOOLEAN
InternalRelocateAndCopyRelocations (
  IN  PRELINKED_CONTEXT           *Context,
  IN  PRELINKED_KEXT              *Kext,
  IN  UINT64                      LoadAddress,
  IN  UINT64                      RelocationBase,
  IN  CONST MACH_RELOCATION_INFO  *SourceRelocations,
  IN  OUT UINT32                  *NumRelocations,
  OUT MACH_RELOCATION_INFO        *TargetRelocations
  )
{
  UINT32                     PreservedRelocations;

  UINT32                     Index;
  UINT32                     SectionIndex;
  CONST MACH_RELOCATION_INFO *NextRelocation;
  UINTN                      Result;
  MACH_RELOCATION_INFO       *Relocation;
  MACH_SECTION               *Section32;

  ASSERT (Kext != NULL);
  ASSERT (NumRelocations != NULL);
  ASSERT (SourceRelocations != NULL || *NumRelocations == 0);

  //
  // Fallback to section-based relocations if needed for 32-bit.
  //
  if (Context->Is32Bit && *NumRelocations == 0) {
    SectionIndex = 0;
    while (TRUE) {
      Section32 = MachoGetSectionByIndex32 (&Kext->Context.MachContext, SectionIndex);
      if (Section32 == NULL) {
        break;
      }

      if (Section32->NumRelocations > 0) {
        Relocation = (MACH_RELOCATION_INFO *) ((UINTN) MachoGetMachHeader32 (&Kext->Context.MachContext) + Section32->RelocationsOffset);
        for (Index = 0; Index < Section32->NumRelocations; ++Index) {
          NextRelocation = &Relocation[Index + 1];
          //
          // The last Relocation does not have a successor.
          //
          if (Index == (Section32->NumRelocations - 1)) {
            NextRelocation = NULL;
          }

          //
          // Relocate the relocation.
          //
          Result = InternalRelocateRelocation (
                    Context,
                    Kext,
                    LoadAddress,
                    RelocationBase + Section32->Address,
                    &Relocation[Index],
                    NextRelocation
                    );
          if (Result == MAX_UINTN) {
            return FALSE;
          }
        }

        Section32->NumRelocations     = 0;
        Section32->RelocationsOffset  = 0;
      }

      ++SectionIndex;
    }

    return TRUE;
  }

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
    Result = InternalRelocateRelocation (
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
    // InternalRelocateRelocation().
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

STATIC
BOOLEAN
InternalRelocateSymbol (
  IN     OC_MACHO_CONTEXT  *MachoContext,
  IN     UINT64            LoadAddress,
  IN OUT MACH_NLIST_ANY    *Symbol,
  IN OUT UINT32            *KmodInfoOffset
  )
{
  BOOLEAN         Result;
  CONST CHAR8     *SymbolName;
  UINT32          KmodOffset;
  UINT32          MaxSize;

  KmodOffset = *KmodInfoOffset;

  if ((KmodOffset == 0) && (((MachoContext->Is32Bit ? Symbol->Symbol32.Type : Symbol->Symbol64.Type) & MACH_N_TYPE_STAB) == 0)) {
    SymbolName = MachoGetSymbolName (MachoContext, Symbol);
    ASSERT (SymbolName != NULL);

    if (AsciiStrCmp (SymbolName, "_kmod_info") == 0) {
      Result = MachoSymbolGetFileOffset (
                MachoContext,
                Symbol,
                &KmodOffset,
                &MaxSize
                );
      if (!Result
      || (MaxSize < (MachoContext->Is32Bit ? sizeof (KMOD_INFO_32_V1) : sizeof (KMOD_INFO_64_V1))
      || (KmodOffset % 4) != 0)) {
        return FALSE;
      }

      *KmodInfoOffset = KmodOffset;
    }
  }

  return MachoRelocateSymbol (
          MachoContext,
          LoadAddress,
          Symbol
          );
}

STATIC
BOOLEAN
InternalRelocateSymbols (
  IN     OC_MACHO_CONTEXT  *MachoContext,
  IN     UINT64            LoadAddress,
  IN     UINT32            NumSymbols,
  IN OUT MACH_NLIST_ANY    *Symbols,
  OUT    UINT32            *KmodInfoOffset
  )
{
  BOOLEAN         Result;
  UINT32          Index;
  MACH_NLIST_ANY  *Symbol;
  UINT8           SymbolType;

  ASSERT (MachoContext != NULL);
  ASSERT (Symbols != NULL || NumSymbols == 0);
  ASSERT (KmodInfoOffset != NULL);

  //
  // Fallback to standard symbol lookup if needed for 32-bit.
  //
  if (MachoContext->Is32Bit && NumSymbols == 0) {
    Index = 0;
    while (TRUE) {
      Symbol = MachoGetSymbolByIndex (MachoContext, Index);
      if (Symbol == NULL) {
        break;
      }

      SymbolType = Symbol->Symbol32.Type & MACH_N_TYPE_TYPE;
      if (SymbolType == MACH_N_TYPE_SECT || SymbolType == MACH_N_TYPE_EXT) {
        Result = InternalRelocateSymbol (
                  MachoContext,
                  LoadAddress,
                  Symbol,
                  KmodInfoOffset
                  );

        if (!Result) {
          return FALSE;
        }
      }

      ++Index;
    }

    return TRUE;
  }

  for (Index = 0; Index < NumSymbols; ++Index) {
    if (MachoContext->Is32Bit) {
      Symbol = (MACH_NLIST_ANY *)&(&Symbols->Symbol32)[Index];
    } else {
      Symbol = (MACH_NLIST_ANY *)&(&Symbols->Symbol64)[Index];
    }
    Result = InternalRelocateSymbol (
              MachoContext,
              LoadAddress,
              Symbol,
              KmodInfoOffset
              );

    if (!Result) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  process_symbol_pointers
**/
STATIC
BOOLEAN
InternalProcessSymbolPointers (
  IN OC_MACHO_CONTEXT             *MachoContext,
  IN CONST MACH_DYSYMTAB_COMMAND  *DySymtab,
  IN UINT64                       LoadAddress
  )
{
  CONST MACH_HEADER_ANY   *MachHeader;
  UINT32                  MachSize;
  CONST MACH_SECTION_ANY  *Section;
  UINT32                  NumSymbols;
  UINT32                  FirstSym;
  BOOLEAN                 Result;
  UINT32                  OffsetTop;
  CONST UINT32            *SymIndices;
  VOID                    *IndirectSymPtr;
  UINT32                  Index;
  CONST MACH_NLIST_ANY    *Symbol;

  VOID                  *Tmp;

  Section = MachoGetSegmentSectionByName (
              MachoContext,
              "__DATA",
              "__nl_symbol_ptr"
              );
  if (Section == NULL) {
    return TRUE;
  }

  if (MachoContext->Is32Bit) {
    NumSymbols = Section->Section32.Size / sizeof (UINT32);
    FirstSym   = Section->Section32.Reserved1;
  } else {
    NumSymbols = (UINT32)(Section->Section64.Size / sizeof (UINT64));
    FirstSym   = Section->Section64.Reserved1;
  }

  Result = OcOverflowAddU32 (FirstSym, NumSymbols, &OffsetTop);
  if (Result || (OffsetTop > DySymtab->NumIndirectSymbols)) {
    return FALSE;
  }

  MachSize = MachoGetFileSize (MachoContext);
  Result = OcOverflowMulAddU32 (
             DySymtab->NumIndirectSymbols,
             MachoContext->Is32Bit ? sizeof (UINT32) : sizeof (UINT64),
             DySymtab->IndirectSymbolsOffset,
             &OffsetTop
             );
  if (Result || (OffsetTop > MachSize)) {
    return FALSE;
  }

  MachHeader = MachoGetMachHeader (MachoContext);
  ASSERT (MachHeader != NULL);
  //
  // Iterate through the indirect symbol table and fill in the section of
  // symbol pointers.  There are three cases:
  //   1) A normal symbol - put its value directly in the table
  //   2) An INDIRECT_SYMBOL_LOCAL - symbols that are local and already have
  //      their offset from the start of the file in the section.  Simply
  //      add the file's link address to fill this entry.
  //   3) An INDIRECT_SYMBOL_ABS - prepopulated absolute symbols.  No
  //      action is required.
  //
  Tmp = (VOID *)((UINTN)MachHeader + DySymtab->IndirectSymbolsOffset);
  if (!OC_TYPE_ALIGNED (UINT32, Tmp)) {
    return FALSE;
  }
  SymIndices = (UINT32 *)Tmp + FirstSym;

  IndirectSymPtr = (VOID *)((UINTN)MachHeader + (MachoContext->Is32Bit ? Section->Section32.Offset : Section->Section64.Offset));
  if (MachoContext->Is32Bit ? !OC_TYPE_ALIGNED (UINT32, IndirectSymPtr) : !OC_TYPE_ALIGNED (UINT64, IndirectSymPtr)) {
    return FALSE;
  }

  for (Index = 0; Index < NumSymbols; ++Index) {
    if ((SymIndices[Index] & MACH_INDIRECT_SYMBOL_LOCAL) != 0) {
      if ((SymIndices[Index] & MACH_INDIRECT_SYMBOL_ABS) != 0) {
        continue;
      }

      if (MachoContext->Is32Bit) {
        ((UINT32 *) IndirectSymPtr)[Index] += (UINT32) LoadAddress;
      } else {
        ((UINT64 *) IndirectSymPtr)[Index] += LoadAddress;
      }
    } else {
      Symbol = MachoGetSymbolByIndex (MachoContext, SymIndices[Index]);
      if (Symbol == NULL) {
        return FALSE;
      }

      if (MachoContext->Is32Bit) {
        ((UINT32 *) IndirectSymPtr)[Index] += Symbol->Symbol32.Value;
      } else {
        ((UINT64 *) IndirectSymPtr)[Index] += Symbol->Symbol64.Value;
      }
    }
  }

  return TRUE;
}

/**
  Prelinks the specified KEXT against the specified LoadAddress and the data
  of its dependencies.

  @param[in,out] Context      Prelinking context.
  @param[in]     Kext         KEXT prelinking context.
  @param[in]     LoadAddress  The address this KEXT shall be linked against.
  @param[in]     FileOffset   The file offset of the first segment.

  @retval  Returned is whether the prelinking process has been successful.
           The state of the KEXT is undefined in case this routine fails.

**/
EFI_STATUS
InternalPrelinkKext (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     PRELINKED_KEXT     *Kext,
  IN     UINT64             LoadAddress,
  IN     UINT64             FileOffset
  )
{
  OC_MACHO_CONTEXT           *MachoContext;
  MACH_SEGMENT_COMMAND_ANY   *LinkEditSegment;
  UINT64                     LinkEditFileOffset;
  UINT64                     LinkEditFileSize;

  MACH_HEADER_ANY            *MachHeader;
  UINT32                     MachSize;
  BOOLEAN                    IsObject32;

  MACH_SEGMENT_COMMAND_ANY   *Segment;
  MACH_SECTION_ANY           *Section;

  UINT32                     Index;
  BOOLEAN                    Result;
  UINT32                     SymtabSize;
  UINT32                     SymtabSize2;

  MACH_SYMTAB_COMMAND        *Symtab;
  MACH_DYSYMTAB_COMMAND      *DySymtab;
  MACH_NLIST_ANY             *Symbol;
  CONST CHAR8                *SymbolName;
  CONST MACH_NLIST_ANY       *SymbolTable;
  CONST CHAR8                *StringTable;
  UINT32                     NumSymbols;
  CONST MACH_NLIST_ANY       *IndirectSymtab;
  UINT32                     NumIndirectSymbols;
  CONST MACH_NLIST_ANY       *LocalSymtab;
  UINT32                     NumLocalSymbols;
  CONST MACH_NLIST_ANY       *ExternalSymtab;
  UINT32                     NumExternalSymbols;
  CONST MACH_NLIST_ANY       *UndefinedSymtab;
  UINT32                     NumUndefinedSymbols;
  UINT64                     WeakTestValue;

  UINT32                     NumRelocations;
  UINT32                     NumRelocations2;
  CONST MACH_RELOCATION_INFO *Relocations;
  MACH_RELOCATION_INFO       *TargetRelocation;
  MACH_SEGMENT_COMMAND_ANY   *FirstSegment;

  VOID                       *LinkEdit;
  UINT32                     LinkEditSize;
  UINT32                     SymbolTableOffset;
  UINT32                     SymbolTableSize;
  UINT32                     RelocationsOffset;
  UINT32                     RelocationsSize;
  UINT32                     StringTableOffset;
  UINT32                     StringTableSize;
  MACH_NLIST                 *SymtabLinkEdit32;
  UINT32                     NumSymtabLinkEdit32Symbols;

  UINT32                     SegmentOffset;
  UINT32                     SegmentSize;
  UINT64                     LoadAddressOffset;

  UINT64                     SegmentVmSizes;
  UINT32                     KmodInfoOffset;
  KMOD_INFO_ANY              *KmodInfo;

  ASSERT (Context != NULL);
  ASSERT (Kext != NULL);
  ASSERT (LoadAddress != 0);

  //
  // ASSUMPTIONS:
  // If kext is 64-bit, it has a __LINKEDIT segment and DYSYMTAB.
  // If kext is 32-bit, it has a __LINKEDIT segment and DYSYMTAB if its not MH_OBJECT.
  //

  MachoContext    = &Kext->Context.MachContext;
  LinkEditSegment = Kext->LinkEditSegment;

  MachHeader = MachoGetMachHeader (MachoContext);
  MachSize   = MachoGetFileSize (MachoContext);

  IsObject32 = Context->Is32Bit && MachHeader->Header32.FileType == MachHeaderFileTypeObject; 

  //
  // Only perform actions when the kext is flag'd to be dynamically linked.
  //
  if (!IsObject32
    && ((Context->Is32Bit ? MachHeader->Header32.Flags : MachHeader->Header64.Flags) & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) == 0) {
    return EFI_SUCCESS;
  }

  if ((!IsObject32 && LinkEditSegment == NULL) || Kext->Context.VirtualKmod == 0) {
    return EFI_UNSUPPORTED;
  }

  if (OcOverflowAddU64 (LoadAddress, FileOffset, &LoadAddressOffset)) {
    return EFI_INVALID_PARAMETER;
  }
  if (Context->Is32Bit) {
    ASSERT (LoadAddressOffset < MAX_UINT32);
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
  if (NumSymbols == 0) {
    return EFI_UNSUPPORTED;
  }

  Symtab = MachoContext->Symtab;
  ASSERT (Symtab != NULL);

  DySymtab = MachoContext->DySymtab;
  if (!IsObject32) {
    ASSERT (DySymtab != NULL);
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
  SymbolTableSize = (NumSymbols * (Context->Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64)));
  StringTableSize = Symtab->StringsSize;

  //
  // For the allocation, assume all relocations will be preserved to simplify
  // the code, the memory is only temporarily allocated anyway.
  //
  if (!IsObject32) {
    NumRelocations  = DySymtab->NumOfLocalRelocations;
    NumRelocations += DySymtab->NumExternalRelocations;
  } else {
    NumRelocations  = 0;
  }
  RelocationsSize = (NumRelocations * sizeof (MACH_RELOCATION_INFO));

  LinkEdit        = Context->LinkBuffer;
  LinkEditSize    = (SymbolTableSize + RelocationsSize + StringTableSize);

  if (!IsObject32
    && LinkEditSize > (Context->Is32Bit ? LinkEditSegment->Segment32.FileSize : LinkEditSegment->Segment64.FileSize)) {
    return EFI_UNSUPPORTED;
  }

  SymbolTableSize  -= (NumUndefinedSymbols * (Context->Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64)));

  SymbolTableOffset = 0;
  RelocationsOffset = (SymbolTableOffset + SymbolTableSize);

  //
  // Solve indirect symbols.
  // For 32-bit objects, we will solve those at the same time as undefined symbols later.
  //
  if (!IsObject32) {
    WeakTestValue      = 0;
    NumIndirectSymbols = MachoGetIndirectSymbolTable (
                          MachoContext,
                          &IndirectSymtab
                          );
    for (Index = 0; Index < NumIndirectSymbols; ++Index) {
      Symbol     = (MACH_NLIST_ANY *) &IndirectSymtab[Index];
      SymbolName = MachoGetIndirectSymbolName (MachoContext, Symbol);
      if (SymbolName == NULL) {
        return EFI_LOAD_ERROR;
      }

      Result = InternalSolveSymbol (
                Context,
                Kext,
                SymbolName,
                Symbol,
                &WeakTestValue,
                UndefinedSymtab,
                NumUndefinedSymbols
                );
      if (!Result) {
        return EFI_LOAD_ERROR;
      }
    }
  }

  //
  // Solve undefined symbols. If on 32-bit, we may not have a DYSYMTAB so fallback to checking all symbols.
  // All non-undefined symbols will have their indexes stored in the new symtab in LinkBuffer for later use.
  //
  NumSymtabLinkEdit32Symbols = 0;
  if (IsObject32) {
    NumUndefinedSymbols = NumSymbols;
  }
  for (Index = 0; Index < NumUndefinedSymbols; ++Index) {
    if (Context->Is32Bit) {
      Symbol = (MACH_NLIST_ANY *) (!IsObject32 ? &(&UndefinedSymtab->Symbol32)[Index] : &(&SymbolTable->Symbol32)[Index]);
      if (IsObject32
        && (Symbol->Symbol32.Type & MACH_N_TYPE_TYPE) != MACH_N_TYPE_UNDF
        && (Symbol->Symbol32.Type & MACH_N_TYPE_TYPE) != MACH_N_TYPE_INDR) {
        continue;
      }
    } else {
      Symbol = (MACH_NLIST_ANY *) &(&UndefinedSymtab->Symbol64)[Index];
    }

    //
    // Undefined symbols are solved via their name.
    //
    SymbolName = MachoGetSymbolName (MachoContext, Symbol);
    Result = InternalSolveSymbol (
               Context,
               Kext,
               SymbolName,
               Symbol,
               &WeakTestValue,
               !IsObject32 ? UndefinedSymtab : NULL,
               !IsObject32 ? NumUndefinedSymbols : 0
               );
    if (!Result) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: Symbol %a was unresolved for kext %a\n",
        MachoGetSymbolName (MachoContext, Symbol),
        Kext->Identifier
        ));
      return EFI_LOAD_ERROR;
    } else {
      DEBUG ((
        DEBUG_VERBOSE,
        "OCAK: Symbol %a was resolved for kext %a to %Lx\n",
        MachoGetSymbolName (MachoContext, Symbol),
        Kext->Identifier,
        Context->Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value
        ));
    }
  }
  //
  // Create and patch the KEXT's VTables.
  //
  Result = InternalPatchByVtables (Context, Kext);
  if (!Result) {
    DEBUG ((DEBUG_INFO, "OCAK: Vtable patching failed for kext %a\n", Kext->Identifier));
    return EFI_LOAD_ERROR;
  }

  //
  // Relocate local and external symbols.
  // We only need to call InternalRelocateSymbols once if there is no DYSYMTAB.
  //
  KmodInfoOffset = 0;
  if (!IsObject32) {
      Result = InternalRelocateSymbols (
              MachoContext,
              LoadAddressOffset,
              NumLocalSymbols,
              (MACH_NLIST_ANY *) LocalSymtab,
              &KmodInfoOffset
              );
    if (!Result) {
      return EFI_LOAD_ERROR;
    }
  }

  Result = InternalRelocateSymbols (
             MachoContext,
             LoadAddressOffset,
             NumExternalSymbols,
             (MACH_NLIST_ANY *) ExternalSymtab,
             &KmodInfoOffset
             );
  if (!Result || (KmodInfoOffset == 0)) {
    return EFI_LOAD_ERROR;
  }

  KmodInfo     = (KMOD_INFO_ANY *)((UINTN) MachHeader + (UINTN)KmodInfoOffset);

  FirstSegment = MachoGetNextSegment (MachoContext, NULL);
  if (FirstSegment == NULL) {
    return EFI_UNSUPPORTED;
  }

  Result = InternalProcessSymbolPointers (MachoContext, DySymtab, LoadAddressOffset);
  if (!Result) {
    return EFI_LOAD_ERROR;
  }

  //
  // Copy the relocations to be reserved and adapt the symbol number they
  // reference in case it has been relocated above.
  //
  TargetRelocation = (MACH_RELOCATION_INFO *)(
                       (UINTN)LinkEdit + RelocationsOffset
                       );

  //
  // Relocate and copy local and external relocations.
  //
  Relocations    = MachoContext->LocalRelocations;
  NumRelocations = !IsObject32 ? DySymtab->NumOfLocalRelocations : 0;
  Result = InternalRelocateAndCopyRelocations (
             Context,
             Kext,
             LoadAddressOffset,
             Context->Is32Bit ? FirstSegment->Segment32.VirtualAddress : FirstSegment->Segment64.VirtualAddress,
             Relocations,
             &NumRelocations,
             &TargetRelocation[0]
             );
  if (!Result) {
    return EFI_LOAD_ERROR;
  }

  //
  // If there is no DYSYMTAB, the call to InternalRelocateAndCopyRelocations
  // above takes care of all relocations.
  //
  if (!IsObject32) {
    Relocations     = MachoContext->ExternRelocations;
    NumRelocations2 = DySymtab->NumExternalRelocations;
    Result = InternalRelocateAndCopyRelocations (
              Context,
              Kext,
              LoadAddressOffset,
              Context->Is32Bit ?
                FirstSegment->Segment32.VirtualAddress : FirstSegment->Segment64.VirtualAddress,
              Relocations,
              &NumRelocations2,
              &TargetRelocation[NumRelocations]
              );
    if (!Result) {
      return EFI_LOAD_ERROR;
    }
    NumRelocations += NumRelocations2;
    RelocationsSize = (NumRelocations * sizeof (MACH_RELOCATION_INFO));
  }

  if (!IsObject32) {
    //
    // Copy the entire symbol table excluding the area for undefined symbols.
    //
    SymtabSize = SymbolTableSize;
    if (NumUndefinedSymbols > 0) {
      SymtabSize = (UINT32)((UndefinedSymtab - SymbolTable) * (Context->Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64)));
    }

    CopyMem (
      (VOID *)((UINTN)LinkEdit + SymbolTableOffset),
      SymbolTable,
      SymtabSize
      );

    if (NumUndefinedSymbols > 0) {
      if (Context->Is32Bit) {
        SymtabSize2  = (UINT32)(&(&SymbolTable->Symbol32)[NumSymbols] - &(&UndefinedSymtab->Symbol32)[NumUndefinedSymbols]);
        SymtabSize2 *= sizeof (MACH_NLIST);

        CopyMem (
          (VOID *)((UINTN)LinkEdit + SymbolTableOffset + SymtabSize),
          (VOID *)&(&UndefinedSymtab->Symbol32)[NumUndefinedSymbols],
          SymtabSize2
          );
      } else {
        SymtabSize2  = (UINT32)(&(&SymbolTable->Symbol64)[NumSymbols] - &(&UndefinedSymtab->Symbol64)[NumUndefinedSymbols]);
        SymtabSize2 *= sizeof (MACH_NLIST_64);

        CopyMem (
          (VOID *)((UINTN)LinkEdit + SymbolTableOffset + SymtabSize),
          (VOID *)&(&UndefinedSymtab->Symbol64)[NumUndefinedSymbols],
          SymtabSize2
          );
      }

      NumSymbols -= NumUndefinedSymbols;
    }

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
    if (Context->Is32Bit) {
      LinkEditFileOffset = LinkEditSegment->Segment32.FileOffset;
      LinkEditFileSize   = LinkEditSegment->Segment32.FileSize;
    } else {
      LinkEditFileOffset = LinkEditSegment->Segment64.FileOffset;
      LinkEditFileSize   = LinkEditSegment->Segment64.FileSize;
    }
    
    Symtab->SymbolsOffset = (UINT32)(LinkEditFileOffset + SymbolTableOffset);
    Symtab->NumSymbols    = NumSymbols;
    Symtab->StringsOffset = (UINT32)(LinkEditFileOffset + StringTableOffset);

    //
    // Copy the new __LINKEDIT segment into the binary and fix its Load Command.
    //
    LinkEditSize = (SymbolTableSize + RelocationsSize + StringTableSize);

    CopyMem (
      (VOID *)((UINTN)MachHeader + (UINTN)LinkEditFileOffset),
      LinkEdit,
      LinkEditSize
      );
    ZeroMem (
      (VOID *)((UINTN)MachHeader + (UINTN)LinkEditFileOffset + LinkEditSize),
      (UINTN)(LinkEditFileSize - LinkEditSize)
      );

    LinkEditSize = MACHO_ALIGN (LinkEditSize);
    if (Context->Is32Bit) {
      LinkEditSegment->Segment32.FileSize = LinkEditSize;
      LinkEditSegment->Segment32.Size     = LinkEditSize;
    } else {
      LinkEditSegment->Segment64.FileSize = LinkEditSize;
      LinkEditSegment->Segment64.Size     = LinkEditSize;
    }

    DySymtab->LocalRelocationsOffset = (UINT32)(LinkEditFileOffset + RelocationsOffset);
    DySymtab->NumOfLocalRelocations  = NumRelocations;

    //
    // Clear dynamic linker information.
    //
    DySymtab->LocalSymbolsIndex         = 0;
    DySymtab->NumLocalSymbols           = 0;
    DySymtab->NumExternalSymbols        = 0;
    DySymtab->ExternalSymbolsIndex      = 0;
    DySymtab->NumExternalRelocations    = 0;
    DySymtab->ExternalRelocationsOffset = 0;
    DySymtab->UndefinedSymbolsIndex     = 0;
    DySymtab->NumUndefinedSymbols       = 0;
    DySymtab->IndirectSymbolsOffset     = 0;
    DySymtab->NumIndirectSymbols        = 0;

  } else {
    if (NumUndefinedSymbols > 0) {
      //
      // Copy the entire symbol table excluding undefined symbols.
      // Any symbols that point to locations before our kext are ones
      // that were previously undefined, and can be skipped.
      //
      SymtabLinkEdit32           = (MACH_NLIST *) LinkEdit;
      NumSymtabLinkEdit32Symbols = 0;
      for (Index = 0; Index < NumSymbols; ++Index) {
        Symbol = (MACH_NLIST_ANY *) &(&SymbolTable->Symbol32)[Index];
        if (Symbol->Symbol32.Type == (MACH_N_TYPE_ABS | MACH_N_TYPE_EXT)
          && Symbol->Symbol32.Section == NO_SECT
          && Symbol->Symbol32.Value < LoadAddress) {
          continue;
        }

        CopyMem (
          &SymtabLinkEdit32[NumSymtabLinkEdit32Symbols++],
          Symbol,
          sizeof (MACH_NLIST)
          );
      }

      SymtabSize  = (UINT32)(NumSymbols * sizeof (MACH_NLIST));
      SymtabSize2 = (UINT32)(NumSymtabLinkEdit32Symbols * sizeof (MACH_NLIST));

      //
      // Zero out the existing symbol table, and copy our non-undefined symbols back.
      //
      ZeroMem (
        (VOID *)((UINTN)MachHeader + Symtab->SymbolsOffset),
        SymtabSize
        );
      CopyMem (
        (VOID *)((UINTN)MachHeader + Symtab->SymbolsOffset),
        SymtabLinkEdit32,
        SymtabSize2
        );

      Symtab->NumSymbols = NumSymtabLinkEdit32Symbols;
    }
  }

  //
  // Adapt the link addresses of all Segments and their Sections.
  //
  SegmentOffset  = 0;
  SegmentSize    = 0;
  SegmentVmSizes = 0;

  Segment = NULL;
  while ((Segment = MachoGetNextSegment (MachoContext, Segment)) != NULL) {
    Section = NULL;
    while ((Section = MachoGetNextSection (MachoContext, Segment, Section)) != NULL) {
      if (Context->Is32Bit) {
        Section->Section32.Address = ALIGN_VALUE (
                                      (Section->Section32.Address + (UINT32) LoadAddressOffset),
                                      (UINT32)(1U << Section->Section32.Alignment)
                                      );
      } else {
        Section->Section64.Address = ALIGN_VALUE (
                                      (Section->Section64.Address + LoadAddressOffset),
                                      (UINT64)(1U << Section->Section64.Alignment)
                                      );
      }
    }

    if (Context->Is32Bit) {
      Segment->Segment32.VirtualAddress += (UINT32) LoadAddressOffset;

      //
      // Logically equivalent to kxld_seg_set_vm_protections.
      // Assertion: Not i386 (strict protection).
      //
      // MH_OBJECT has only one unnamed segment, so all protections need to be enabled.
      //
      if (AsciiStrnCmp (Segment->Segment32.SegmentName, "", ARRAY_SIZE (Segment->Segment32.SegmentName)) == 0) {
        Segment->Segment32.InitialProtection = TEXT_SEG_PROT | DATA_SEG_PROT;
        Segment->Segment32.MaximumProtection = TEXT_SEG_PROT | DATA_SEG_PROT;
      } else if (AsciiStrnCmp (Segment->Segment32.SegmentName, "__TEXT", ARRAY_SIZE (Segment->Segment32.SegmentName)) == 0) {
        Segment->Segment32.InitialProtection = TEXT_SEG_PROT;
        Segment->Segment32.MaximumProtection = TEXT_SEG_PROT;
      } else {
        Segment->Segment32.InitialProtection = DATA_SEG_PROT;
        Segment->Segment32.MaximumProtection = DATA_SEG_PROT;
      }

      if (Segment->Segment32.FileOffset > SegmentOffset) {
        SegmentOffset = Segment->Segment32.FileOffset;
        SegmentSize   = Segment->Segment32.FileSize;
      }

      SegmentVmSizes += Segment->Segment32.Size;
    } else {
      Segment->Segment64.VirtualAddress += LoadAddressOffset;

      //
      // Logically equivalent to kxld_seg_set_vm_protections.
      // Assertion: Not i386 (strict protection).
      //
      if (AsciiStrnCmp (Segment->Segment64.SegmentName, "__TEXT", ARRAY_SIZE (Segment->Segment64.SegmentName)) == 0) {
        Segment->Segment64.InitialProtection = TEXT_SEG_PROT;
        Segment->Segment64.MaximumProtection = TEXT_SEG_PROT;
      } else {
        Segment->Segment64.InitialProtection = DATA_SEG_PROT;
        Segment->Segment64.MaximumProtection = DATA_SEG_PROT;
      }

      if (Segment->Segment64.FileOffset > SegmentOffset) {
        SegmentOffset = (UINT32)Segment->Segment64.FileOffset;
        SegmentSize   = (UINT32)Segment->Segment64.FileSize;
      }

      SegmentVmSizes += Segment->Segment64.Size;
    }
  }

  if (Context->Is32Bit) {
    //
    // Populate kmod information.
    //
    KmodInfo->Kmod32.Address = (UINT32) LoadAddress;
    //
    // This is a hack borrowed from XNU. Real header size is equal to:
    //   sizeof (*MachHeader) + MachHeader->CommandsSize (often aligned to 4096)
    // However, it cannot be set to this value unless it exists in a separate segment,
    // and presently it is not the case on macOS. When header is put to __TEXT (as usual),
    // XNU makes it read only, and this prevents __TEXT from gaining executable permission.
    // See OSKext::setVMAttributes.
    //
    KmodInfo->Kmod32.HdrSize = IsObject32 ? (UINT32) FileOffset : 0;
    KmodInfo->Kmod32.Size    = IsObject32 ? MachSize : KmodInfo->Kmod32.HdrSize + (UINT32) SegmentVmSizes;
    //
    // Adapt the Mach-O header to signal being prelinked.
    //
    MachHeader->Header32.Flags = MACH_HEADER_FLAG_NO_UNDEFINED_REFERENCES;

    if (!IsObject32) {
      MachSize = SegmentOffset + SegmentSize;
    }
  } else {
    //
    // Populate kmod information.
    //
    KmodInfo->Kmod64.Address = LoadAddress;
    //
    // This is a hack borrowed from XNU. Real header size is equal to:
    //   sizeof (*MachHeader) + MachHeader->CommandsSize (often aligned to 4096)
    // However, it cannot be set to this value unless it exists in a separate segment,
    // and presently it is not the case on macOS. When header is put to __TEXT (as usual),
    // XNU makes it read only, and this prevents __TEXT from gaining executable permission.
    // See OSKext::setVMAttributes.
    //
    KmodInfo->Kmod64.HdrSize = 0;
    KmodInfo->Kmod64.Size    = KmodInfo->Kmod64.HdrSize + SegmentVmSizes;
    //
    // Adapt the Mach-O header to signal being prelinked.
    //
    MachHeader->Header64.Flags = MACH_HEADER_FLAG_NO_UNDEFINED_REFERENCES;

    MachSize = SegmentOffset + SegmentSize;
  }

  //
  // Reinitialize the Mach-O context to account for the changed __LINKEDIT
  // segment and file size.
  //
  if (!MachoInitializeContext (MachoContext, MachHeader, MachSize, MachoContext->ContainerOffset, Context->Is32Bit)) { 
    //
    // This should never failed under normal and abnormal conditions.
    //
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

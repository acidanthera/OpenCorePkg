/** @file
  Provides services for symbols.

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

#include "OcMachoLibInternal.h"

BOOLEAN
InternalSymbolIsSane (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab->NumSymbols > 0);

  ASSERT ((Symbol >= &Context->SymbolTable[0])
       && (Symbol < &Context->SymbolTable[Context->Symtab->NumSymbols]));
  //
  // Symbol->Section is implicitly verified by MachoGetSectionByIndex64() when
  // passed to it.
  //
  if (Symbol->UnifiedName.StringIndex >= Context->Symtab->StringsSize) {
    return FALSE;
  }

  return TRUE;
}

/**
  Returns whether the symbol's value is a valid address within the Mach-O
  referenced to by Context.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to verify the value of.

**/
BOOLEAN
MachoIsSymbolValueInRange64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;

  if (MachoSymbolIsLocalDefined (Context, Symbol)) {
    for (
      Segment = MachoGetNextSegment64 (Context, NULL);
      Segment != NULL;
      Segment = MachoGetNextSegment64 (Context, Segment)
      ) {
      if ((Symbol->Value >= Segment->VirtualAddress)
       && (Symbol->Value < (Segment->VirtualAddress + Segment->Size))) {
        return TRUE;
      }
    }

    return FALSE;
  }

  return TRUE;
}

/**
  Returns whether Symbol describes a section.

  @param[in] Symbol  Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsSection (
  IN CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Symbol != NULL);

  if ((Symbol->Type & MACH_N_TYPE_STAB) != 0) {
    switch (Symbol->Type) {
      //
      // Labeled as MACH_N_sect in stab.h
      //
      case MACH_N_FUN:
      case MACH_N_STSYM:
      case MACH_N_LCSYM:
      case MACH_N_BNSYM:
      case MACH_N_SLINE:
      case MACH_N_ENSYM:
      case MACH_N_SO:
      case MACH_N_SOL:
      case MACH_N_ENTRY:
      case MACH_N_ECOMM:
      case MACH_N_ECOML:
      //
      // These are labeled as NO_SECT in stab.h, but they are actually
      // section-based on OS X.  We must mark them as such so they get
      // relocated.
      //
      case MACH_N_RBRAC:
      case MACH_N_LBRAC:
      {
        return TRUE;
      }

      default:
      {
        break;
      }
    }
  } else if ((Symbol->Type & MACH_N_TYPE_TYPE) == MACH_N_TYPE_SECT) {
    return TRUE;
  }

  return FALSE;
}

/**
  Returns whether Symbol is defined.

  @param[in] Symbol  Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsDefined (
  IN CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Symbol != NULL);

  return ((Symbol->Type == MACH_RELOC_ABSOLUTE)
           || MachoSymbolIsSection (Symbol));
}

/**
  Returns whether Symbol is defined locally.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsLocalDefined (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  CONST MACH_NLIST_64         *UndefinedSymbols;
  CONST MACH_NLIST_64         *UndefinedSymbolsTop;
  CONST MACH_NLIST_64         *IndirectSymbols;
  CONST MACH_NLIST_64         *IndirectSymbolsTop;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  DySymtab = Context->DySymtab;
  ASSERT (Context->SymbolTable != NULL);

  if ((DySymtab == NULL) || (DySymtab->NumUndefinedSymbols == 0)) {
    return TRUE;
  }
  //
  // The symbol must have been declared locally prior to solving.  As there is
  // no information on whether the symbol has been solved explicitely, check
  // its storage location for Undefined or Indirect.
  //
  UndefinedSymbols    = &Context->SymbolTable[DySymtab->UndefinedSymbolsIndex];
  UndefinedSymbolsTop = &UndefinedSymbols[DySymtab->NumUndefinedSymbols];

  if ((Symbol >= UndefinedSymbols) && (Symbol < UndefinedSymbolsTop)) {
    return FALSE;
  }

  IndirectSymbols = Context->IndirectSymbolTable;
  IndirectSymbolsTop = &IndirectSymbols[DySymtab->NumIndirectSymbols];

  if ((Symbol >= IndirectSymbols) && (Symbol < IndirectSymbolsTop)) {
    return FALSE;
  }

  return MachoSymbolIsDefined (Symbol);
}

/**
  Retrieves a symbol by its index.

  @param[in] Context  Context of the Mach-O.
  @param[in] Index    Index of the symbol to locate.

  @retval NULL  NULL is returned on failure.

**/
MACH_NLIST_64 *
MachoGetSymbolByIndex64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index
  )
{
  MACH_NLIST_64 *Symbol;

  ASSERT (Context != NULL);

  if (!InternalRetrieveSymtabs64 (Context)) {
    return NULL;
  }

  ASSERT (Context->SymbolTable != NULL);

  if (Index < Context->Symtab->NumSymbols) {
    Symbol = &Context->SymbolTable[Index];
    if (InternalSymbolIsSane (Context, Symbol)) {
      return Symbol;
    }
  }

  return NULL;
}

/**
  Retrieves Symbol's name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to retrieve the name of.

  @retval NULL  NULL is returned on failure.

**/
CONST CHAR8 *
MachoGetSymbolName64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab->StringsSize > Symbol->UnifiedName.StringIndex);

  return (Context->StringTable + Symbol->UnifiedName.StringIndex);
}

/**
  Retrieves Symbol's name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Indirect symbol to retrieve the name of.

  @retval NULL  NULL is returned on failure.

**/
CONST CHAR8 *
MachoGetIndirectSymbolName64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (Context->SymbolTable != NULL);

  if ((Symbol->Type & MACH_N_TYPE_STAB) != 0
    || (Symbol->Type & MACH_N_TYPE_TYPE) != MACH_N_TYPE_INDR) {
    return NULL;
  }

  if (Context->Symtab->StringsSize <= Symbol->Value) {
    return NULL;
  }

  return (Context->StringTable + Symbol->Value);
}

/**
  Retrieves the symbol referenced by the Relocation targeting Address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Address to search for.
  @param[out]    Symbol   Buffer to output the symbol referenced by the
                          Relocation into.  The output is undefined when FALSE
                          is returned.  May be NULL.

  @returns  Whether the Relocation exists.

**/
BOOLEAN
MachoGetSymbolByExternRelocationOffset64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    MACH_NLIST_64     **Symbol
  )
{
  CONST MACH_RELOCATION_INFO *Relocation;

  ASSERT (Context != NULL);

  Relocation = InternalGetExternalRelocationByOffset (Context, Address);
  if (Relocation != NULL) {
    *Symbol = MachoGetSymbolByIndex64 (Context, Relocation->SymbolNumber);
    return TRUE;
  }

  return FALSE;
}

/**
  Retrieves a symbol by its name.

  @param[in] Context          Context of the Mach-O.
  @param[in] SymbolTable      Symbol Table of the Mach-O.
  @param[in] NumberOfSymbols  Number of symbols in SymbolTable.
  @param[in] Name             Name of the symbol to locate.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_NLIST_64 *
InternalGetSymbolByName (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     MACH_NLIST_64     *SymbolTable,
  IN     UINT32            NumberOfSymbols,
  IN     CONST CHAR8       *Name
  )
{
  UINT32       Index;
  CONST CHAR8  *TmpName;

  ASSERT (SymbolTable != NULL);
  ASSERT (Name != NULL);

  for (Index = 0; Index < NumberOfSymbols; ++Index) {
    if (!InternalSymbolIsSane (Context, &SymbolTable[Index])) {
      continue;
    }
    TmpName = MachoGetSymbolName64 (Context, &SymbolTable[Index]);
    if (TmpName != NULL && AsciiStrCmp (Name, TmpName) == 0) {
      return &SymbolTable[Index];
    }
  }

  return NULL;
}

/**
  Retrieves a locally defined symbol by its name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Name     Name of the symbol to locate.

**/
MACH_NLIST_64 *
MachoGetLocalDefinedSymbolByName (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *Name
  )
{
  MACH_NLIST_64               *SymbolTable;
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  MACH_NLIST_64               *Symbol;

  ASSERT (Context != NULL);
  ASSERT (Name != NULL);

  if (!InternalRetrieveSymtabs64 (Context)) {
    return NULL;
  }

  SymbolTable = Context->SymbolTable;
  ASSERT (SymbolTable != NULL);

  DySymtab = Context->DySymtab;

  if (DySymtab != NULL) {
    Symbol = InternalGetSymbolByName (
               Context,
               &SymbolTable[DySymtab->LocalSymbolsIndex],
               DySymtab->NumLocalSymbols,
               Name
               );
    if (Symbol == NULL) {
      Symbol = InternalGetSymbolByName (
                 Context,
                 &SymbolTable[DySymtab->ExternalSymbolsIndex],
                 DySymtab->NumExternalSymbols,
                 Name
                 );
    }
  } else {
    ASSERT (Context->Symtab != NULL);
    Symbol = InternalGetSymbolByName (
               Context,
               SymbolTable,
               Context->Symtab->NumSymbols,
               Name
               );
  }

  if ((Symbol != NULL) && !InternalSymbolIsSane (Context, Symbol)) {
    return NULL;
  }

  return Symbol;
}

/**
  Relocate Symbol to be against LinkAddress.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     LinkAddress  The address to be linked against.
  @param[in,out] Symbol       The symbol to be relocated.

  @returns  Whether the operation has been completed successfully.

**/
BOOLEAN
MachoRelocateSymbol64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            LinkAddress,
  IN OUT MACH_NLIST_64     *Symbol
  )
{
  CONST MACH_SECTION_64 *Section;
  UINT64                Value;
  BOOLEAN               Result;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  //
  // Symbols are relocated when they describe sections.
  //
  if (MachoSymbolIsSection (Symbol)) {
    Section = MachoGetSectionByAddress64 (Context, Symbol->Value);
    if (Section == NULL) {
      return FALSE;
    }

    Value = ALIGN_VALUE (
              (Section->Address + LinkAddress),
              (1ULL << Section->Alignment)
              );
    Value -= Section->Address;
    //
    // The overflow arithmetic functions are not used as an overflow within the
    // ALIGN_VALUE addition and a subsequent "underflow" of the section address
    // subtraction is valid, hence just verify whether the final result
    // overflew.
    //
    if (Value < LinkAddress) {
      return FALSE;
    }

    Result = OcOverflowAddU64 (Symbol->Value, Value, &Value);
    if (Result) {
      return FALSE;
    }

    Symbol->Value = Value;
  }

  return TRUE;
}

/**
  Retrieves the Mach-O file offset of the address pointed to by Symbol.

  @param[in,ouz] Context     Context of the Mach-O.
  @param[in]     Symbol      Symbol to retrieve the offset of.
  @param[out]    FileOffset  Pointer the file offset is returned into.
                             If FALSE is returned, the output is undefined.
  @param[out]    MaxSize     Maximum data safely available from FileOffset.

  @retval 0  0 is returned on failure.

**/
BOOLEAN
MachoSymbolGetFileOffset64 (
  IN OUT OC_MACHO_CONTEXT      *Context,
  IN     CONST  MACH_NLIST_64  *Symbol,
  OUT    UINT32                *FileOffset,
  OUT    UINT32                *MaxSize OPTIONAL
  )
{
  UINT64          Offset;
  MACH_SECTION_64 *Section;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  ASSERT (FileOffset != NULL);

  Section = MachoGetSectionByIndex64 (
    Context,
    Symbol->Section == NO_SECT ? 0 : Symbol->Section - 1
    );

  if ((Section == NULL) || (Symbol->Value < Section->Address)) {
    return FALSE;
  }

  Offset = (Symbol->Value - Section->Address);
  if (Offset > Section->Size) {
    return FALSE;
  }
  //
  // The check against Section->Size guarantees a 32-bit value for the current
  // library implementation.
  //
  if (Offset != (UINT32)Offset) {
    return FALSE;
  }

  *FileOffset = (Section->Offset + (UINT32)Offset);

  if (MaxSize != NULL) {
    *MaxSize = (UINT32)(Section->Size - Offset);
  }

  return TRUE;
}

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

#include "MachoX.h"

/**
  Returns whether Symbol describes a section type.

  @param[in] Symbol  Symbol to evaluate.

**/
STATIC
BOOLEAN
InternalSymbolIsSectionType (
  IN CONST MACH_NLIST_X  *Symbol
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
  Retrieves a symbol by its value.

  @param[in] Context  Context of the Mach-O.
  @param[in] Value    Value of the symbol to locate.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_NLIST_X *
MACH_X (InternalGetSymbolByValue) (
  IN OUT OC_MACHO_CONTEXT   *Context,
  IN     MACH_UINT_X        Value
  )
{
  UINT32 Index;

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab != NULL);

  for (Index = 0; Index < Context->Symtab->NumSymbols; ++Index) {
    if ((MACH_X (&Context->SymbolTable->Symbol))[Index].Value == Value) {
      return &(MACH_X (&Context->SymbolTable->Symbol))[Index];
    }
  }

  return NULL;
}

STATIC
BOOLEAN
InternalGetSymbolByExternRelocationOffset (
  IN OUT OC_MACHO_CONTEXT   *Context,
  IN     MACH_UINT_X        Address,
  OUT    MACH_NLIST_X       **Symbol
  )
{
  CONST MACH_RELOCATION_INFO *Relocation;

  ASSERT (Context != NULL);
  MACH_ASSERT_X (Context);

  Relocation = InternalGetExternRelocationByOffset (Context, Address);
  if (Relocation != NULL) {
    *Symbol = MACH_X (MachoGetSymbolByIndex) (Context, Relocation->SymbolNumber);
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
MACH_NLIST_X *
InternalGetLocalDefinedSymbolByNameWorker (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     MACH_NLIST_X      *SymbolTable,
  IN     UINT32            NumberOfSymbols,
  IN     CONST CHAR8       *Name
  )
{
  UINT32       Index;
  CONST CHAR8  *TmpName;

  ASSERT (SymbolTable != NULL);
  ASSERT (Name != NULL);

  for (Index = 0; Index < NumberOfSymbols; ++Index) {
    if (!MACH_X (InternalSymbolIsSane) (Context, &SymbolTable[Index])) {
      break;
    }

    if (!MACH_X (MachoSymbolIsDefined) (&SymbolTable[Index])) {
      continue;
    }

    TmpName = MACH_X (MachoGetSymbolName) (Context, &SymbolTable[Index]);
    if (AsciiStrCmp (Name, TmpName) == 0) {
      return &SymbolTable[Index];
    }
  }

  return NULL;
}

BOOLEAN
MACH_X (InternalSymbolIsSane) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_X   *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  MACH_ASSERT_X (Context);

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab->NumSymbols > 0);

  ASSERT (((Symbol >= &(MACH_X (&Context->SymbolTable->Symbol))[0])
        && (Symbol < &(MACH_X (&Context->SymbolTable->Symbol))[Context->Symtab->NumSymbols]))
       || ((Context->DySymtab != NULL)
        && (Symbol >= &(MACH_X (&Context->IndirectSymbolTable->Symbol))[0])
        && (Symbol < &(MACH_X (&Context->IndirectSymbolTable->Symbol))[Context->DySymtab->NumIndirectSymbols])));
  //
  // Symbol->Section is implicitly verified by MachoGetSectionByIndex() when
  // passed to it.
  //
  if (Symbol->UnifiedName.StringIndex >= Context->Symtab->StringsSize) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
MACH_X (InternalMachoSymbolGetDirectFileOffset) (
  IN OUT OC_MACHO_CONTEXT       *Context,
  IN     MACH_UINT_X            Address,
  OUT    UINT32                 *FileOffset,
  OUT    UINT32                 *MaxSize OPTIONAL
  )
{
  MACH_UINT_X               Offset;
  MACH_UINT_X               Base;
  MACH_UINT_X               Size;
  MACH_SEGMENT_COMMAND_X    *Segment;

  ASSERT (Context != NULL);
  ASSERT (FileOffset != NULL);
  MACH_ASSERT_X (Context);

  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    if ((Address >= Segment->VirtualAddress)
     && (Address < (Segment->VirtualAddress + Segment->Size))) {
      break;
    }
  }

  if (Segment == NULL) {
    return FALSE;
  }

  Offset = Address - Segment->VirtualAddress;
  Base   = Segment->FileOffset;
  Size   = Segment->Size;

  *FileOffset = MACH_X_TO_UINT32 (Base - Context->ContainerOffset + Offset);

  if (MaxSize != NULL) {
    *MaxSize = MACH_X_TO_UINT32 (Size - Offset);
  }

  return TRUE;
}

BOOLEAN
MACH_X (MachoIsSymbolValueInRange) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_X   *Symbol
  )
{
  CONST MACH_SEGMENT_COMMAND_X  *Segment;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  MACH_ASSERT_X (Context);

  if (MACH_X (MachoSymbolIsLocalDefined) (Context, Symbol)) {
    for (
      Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
      Segment != NULL;
      Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
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

BOOLEAN
MACH_X (MachoSymbolIsSection) (
  IN CONST MACH_NLIST_X   *Symbol
  )
{
  ASSERT (Symbol != NULL);
  return (InternalSymbolIsSectionType (Symbol) && (Symbol->Section != NO_SECT));
}

BOOLEAN
MACH_X (MachoSymbolIsDefined) (
  IN CONST MACH_NLIST_X   *Symbol
  )
{
  ASSERT (Symbol != NULL);

  return (((Symbol->Type & MACH_N_TYPE_STAB) == 0)
      && (((Symbol->Type & MACH_N_TYPE_TYPE) == MACH_N_TYPE_ABS)
       || InternalSymbolIsSectionType (Symbol)));
}

BOOLEAN
MACH_X (MachoSymbolIsLocalDefined) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_X   *Symbol
  )
{
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  CONST MACH_NLIST_X          *UndefinedSymbols;
  CONST MACH_NLIST_X          *UndefinedSymbolsTop;
  CONST MACH_NLIST_X          *IndirectSymbols;
  CONST MACH_NLIST_X          *IndirectSymbolsTop;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  MACH_ASSERT_X (Context);

  DySymtab = Context->DySymtab;
  ASSERT (Context->SymbolTable != NULL);

  if ((DySymtab == NULL) || (DySymtab->NumUndefinedSymbols == 0)) {
    return FALSE; // FIXME: Required under 32-bit during vtable parent resolution, if using MH_OBJECT.
  }
  //
  // The symbol must have been declared locally prior to solving.  As there is
  // no information on whether the symbol has been solved explicitely, check
  // its storage location for Undefined or Indirect.
  //
  UndefinedSymbols    = &(MACH_X (&Context->SymbolTable->Symbol))[DySymtab->UndefinedSymbolsIndex];
  UndefinedSymbolsTop = &UndefinedSymbols[DySymtab->NumUndefinedSymbols];

  if ((Symbol >= UndefinedSymbols) && (Symbol < UndefinedSymbolsTop)) {
    return FALSE;
  }

  IndirectSymbols = MACH_X (&Context->IndirectSymbolTable->Symbol);
  IndirectSymbolsTop = &IndirectSymbols[DySymtab->NumIndirectSymbols];

  if ((Symbol >= IndirectSymbols) && (Symbol < IndirectSymbolsTop)) {
    return FALSE;
  }

  return MACH_X (MachoSymbolIsDefined) (Symbol);
}

MACH_NLIST_X *
MACH_X (MachoGetSymbolByIndex) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index
  )
{
  MACH_NLIST_X  *Symbol;

  ASSERT (Context != NULL);
  MACH_ASSERT_X (Context);

  if (!InternalRetrieveSymtabs (Context)) {
    return NULL;
  }

  ASSERT (Context->SymbolTable != NULL);

  if (Index < Context->Symtab->NumSymbols) {
    Symbol = &(MACH_X (&Context->SymbolTable->Symbol))[Index];
    if (MACH_X (InternalSymbolIsSane) (Context, Symbol)) {
      return Symbol;
    }
  }

  return NULL;
}

CONST CHAR8 *
MACH_X (MachoGetSymbolName) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_X   *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  MACH_ASSERT_X (Context);

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab->StringsSize > Symbol->UnifiedName.StringIndex);

  return (Context->StringTable + Symbol->UnifiedName.StringIndex);
}

CONST CHAR8 *
MACH_X (MachoGetIndirectSymbolName) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_X   *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  MACH_ASSERT_X (Context);

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

BOOLEAN
MACH_X (MachoGetSymbolByExternRelocationOffset) (
  IN OUT OC_MACHO_CONTEXT   *Context,
  IN     MACH_UINT_X        Address,
  OUT    MACH_NLIST_X       **Symbol
  )
{
  if (Address >= MachoGetFileSize (Context)) {
    return FALSE;
  }

  return InternalGetSymbolByExternRelocationOffset (
           Context,
           Address,
           Symbol
           );
}

BOOLEAN
MACH_X (MachoGetSymbolByRelocationOffset) (
  IN OUT OC_MACHO_CONTEXT   *Context,
  IN     MACH_UINT_X        Address,
  OUT    MACH_NLIST_X       **Symbol
  )
{
  BOOLEAN                     Result;
  CONST MACH_RELOCATION_INFO  *Relocation;
  CONST MACH_UINT_X           *Data;
  MACH_NLIST_X                *Sym;
  MACH_UINT_X                 AddressTop;
  UINT32                      MaxSize;

  VOID                        *Tmp;

  ASSERT (Context != NULL);
  MACH_ASSERT_X (Context);

  Result = MACH_X (OcOverflowAddU) (Address, sizeof (MACH_UINT_X), &AddressTop);
  if (Result || AddressTop > MachoGetFileSize (Context)) {
    return FALSE;
  }

  Result = InternalGetSymbolByExternRelocationOffset (
             Context,
             Address,
             Symbol
             );
  if (Result) {
    return TRUE;
  }

  Relocation = InternalGetLocalRelocationByOffset (Context, Address);
  if (Relocation != NULL) {
    Sym = NULL;

    Tmp = (VOID *) MachoGetFilePointerByAddress (Context, Address, &MaxSize);
    if (Tmp == NULL || MaxSize < sizeof (MACH_UINT_X)) {
      return FALSE;
    }

    if (OC_TYPE_ALIGNED (MACH_UINT_X, Tmp)) {
      Data = (MACH_UINT_X *)Tmp;

      // FIXME: Only C++ symbols.
      Sym = MACH_X (InternalGetSymbolByValue) (Context, *Data);
      if ((Sym != NULL) && !MACH_X (InternalSymbolIsSane) (Context, Sym)) {
        Sym = NULL;
      }
    }

    *Symbol = Sym;
    return TRUE;
  }

  return FALSE;
}

MACH_NLIST_X *
MACH_X (MachoGetLocalDefinedSymbolByName) (
  IN OUT OC_MACHO_CONTEXT   *Context,
  IN     CONST CHAR8        *Name
  )
{
  MACH_NLIST_X                *SymbolTable;
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  MACH_NLIST_X                *Symbol;

  ASSERT (Context != NULL);
  ASSERT (Name != NULL);
  MACH_ASSERT_X (Context);

  if (!InternalRetrieveSymtabs (Context)) {
    return NULL;
  }

  ASSERT (Context->SymbolTable != NULL);
  SymbolTable = MACH_X (&Context->SymbolTable->Symbol);

  DySymtab = Context->DySymtab;

  if (DySymtab != NULL) {
    Symbol = InternalGetLocalDefinedSymbolByNameWorker (
               Context,
               &SymbolTable[DySymtab->LocalSymbolsIndex],
               DySymtab->NumLocalSymbols,
               Name
               );
    if (Symbol == NULL) {
      Symbol = InternalGetLocalDefinedSymbolByNameWorker (
                 Context,
                 &SymbolTable[DySymtab->ExternalSymbolsIndex],
                 DySymtab->NumExternalSymbols,
                 Name
                 );
    }
  } else {
    ASSERT (Context->Symtab != NULL);
    Symbol = InternalGetLocalDefinedSymbolByNameWorker (
               Context,
               SymbolTable,
               Context->Symtab->NumSymbols,
               Name
               );
  }

  return Symbol;
}

BOOLEAN
MACH_X (MachoRelocateSymbol) (
  IN OUT OC_MACHO_CONTEXT   *Context,
  IN     MACH_UINT_X        LinkAddress,
  IN OUT MACH_NLIST_X       *Symbol
  )
{
  CONST MACH_SECTION_X  *Section;
  MACH_UINT_X           Value;
  BOOLEAN               Result;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  MACH_ASSERT_X (Context);

  //
  // Symbols are relocated when they describe sections.
  //
  if (MACH_X (MachoSymbolIsSection) (Symbol)) {
    Section = MACH_X (MachoGetSectionByIndex) (Context, (Symbol->Section - 1));
    if (Section == NULL) {
      return FALSE;
    }

    Value = ALIGN_VALUE (
              (Section->Address + LinkAddress),
              (MACH_UINT_X)(1U << Section->Alignment)
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

    Result = MACH_X (OcOverflowAddU) (Symbol->Value, Value, &Value);
    if (Result) {
      return FALSE;
    }

    Symbol->Value = Value;
  }

  return TRUE;
}

BOOLEAN
MACH_X (MachoSymbolGetFileOffset) (
  IN OUT OC_MACHO_CONTEXT       *Context,
  IN     CONST MACH_NLIST_X     *Symbol,
  OUT    UINT32                 *FileOffset,
  OUT    UINT32                 *MaxSize OPTIONAL
  )
{
  MACH_UINT_X               Offset;
  MACH_UINT_X               Base;
  MACH_UINT_X               Size;
  MACH_SEGMENT_COMMAND_X    *Segment;
  MACH_SECTION_X            *Section;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  ASSERT (FileOffset != NULL);
  MACH_ASSERT_X (Context);

  if (Symbol->Section == NO_SECT) {
    return FALSE;
  }

  Section = MACH_X (MachoGetSectionByIndex) (
              Context,
              (Symbol->Section - 1)
              );
  if (Section == NULL || Section->Size == 0) {
    for (
      Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
      Segment != NULL;
      Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
      ) {
      if ((Symbol->Value >= Segment->VirtualAddress)
       && (Symbol->Value < (Segment->VirtualAddress + Segment->Size))) {
        break;
      }
    }

    if (Segment == NULL) {
      return FALSE;
    }

    Offset = Symbol->Value - Segment->VirtualAddress;
    Base   = Segment->FileOffset;
    Size   = Segment->Size;
  } else {
    if (Symbol->Value < Section->Address) {
      return FALSE;
    }

    Offset = Symbol->Value - Section->Address;
    Base   = Section->Offset;
    Size   = Section->Size;
    if (Offset > Section->Size) {
      return FALSE;
    }
  }

  *FileOffset = MACH_X_TO_UINT32 (Base - Context->ContainerOffset + Offset);

  if (MaxSize != NULL) {
    *MaxSize = MACH_X_TO_UINT32 (Size - Offset);
  }

  return TRUE;
}

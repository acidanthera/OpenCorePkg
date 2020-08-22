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

#include <Uefi.h>

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

  ASSERT (((Symbol >= &Context->SymbolTable[0])
        && (Symbol < &Context->SymbolTable[Context->Symtab->NumSymbols]))
       || ((Context->DySymtab != NULL)
        && (Symbol >= &Context->IndirectSymbolTable[0])
        && (Symbol < &Context->IndirectSymbolTable[Context->DySymtab->NumIndirectSymbols])));
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
  Returns whether Symbol describes a section type.

  @param[in] Symbol  Symbol to evaluate.

**/
STATIC
BOOLEAN
InternalSymbolIsSectionType (
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
  Returns whether Symbol describes a section.

  @param[in] Symbol  Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsSection (
  IN CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Symbol != NULL);
  return (InternalSymbolIsSectionType (Symbol) && (Symbol->Section != NO_SECT));
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

  return (((Symbol->Type & MACH_N_TYPE_STAB) == 0)
      && (((Symbol->Type & MACH_N_TYPE_TYPE) == MACH_N_TYPE_ABS)
       || InternalSymbolIsSectionType (Symbol)));
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
  Retrieves a symbol by its value.

  @param[in] Context  Context of the Mach-O.
  @param[in] Value    Value of the symbol to locate.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_NLIST_64 *
InternalGetSymbolByValue (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Value
  )
{
  UINT32 Index;

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab != NULL);

  for (Index = 0; Index < Context->Symtab->NumSymbols; ++Index) {
    if (Context->SymbolTable[Index].Value == Value) {
      return &Context->SymbolTable[Index];
    }
  }

  return NULL;
}

BOOLEAN
InternalGetSymbolByExternRelocationOffset64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    MACH_NLIST_64     **Symbol
  )
{
  CONST MACH_RELOCATION_INFO *Relocation;

  ASSERT (Context != NULL);

  Relocation = InternalGetExternRelocationByOffset (Context, Address);
  if (Relocation != NULL) {
    *Symbol = MachoGetSymbolByIndex64 (Context, Relocation->SymbolNumber);
    return TRUE;
  }

  return FALSE;
}

/**
  Retrieves the symbol referenced by the extern Relocation targeting Address.

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
  if (Address >= MachoGetFileSize (Context)) {
    return FALSE;
  }

  return InternalGetSymbolByExternRelocationOffset64 (
           Context,
           Address,
           Symbol
           );
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
MachoGetSymbolByRelocationOffset64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    MACH_NLIST_64     **Symbol
  )
{
  BOOLEAN                    Result;
  CONST MACH_RELOCATION_INFO *Relocation;
  CONST UINT64               *Data;
  MACH_NLIST_64              *Sym;
  UINT64                     AddressTop;

  VOID                       *Tmp;

  ASSERT (Context != NULL);

  Result = OcOverflowAddU64 (Address, sizeof (UINT64), &AddressTop);
  if (Result || AddressTop > MachoGetFileSize (Context)) {
    return FALSE;
  }

  Result = InternalGetSymbolByExternRelocationOffset64 (
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

    Tmp = (VOID *)((UINTN)Context->MachHeader + (UINTN)Address);

    if (OC_TYPE_ALIGNED (UINT64, Tmp)) {
      Data = (UINT64 *)Tmp;

      // FIXME: Only C++ symbols.
      Sym = InternalGetSymbolByValue (Context, *Data);
      if ((Sym != NULL) && !InternalSymbolIsSane (Context, Sym)) {
        Sym = NULL;
      }
    }

    *Symbol = Sym;
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
InternalGetLocalDefinedSymbolByNameWorker (
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
      break;
    }

    if (!MachoSymbolIsDefined (&SymbolTable[Index])) {
      continue;
    }

    TmpName = MachoGetSymbolName64 (Context, &SymbolTable[Index]);
    if (AsciiStrCmp (Name, TmpName) == 0) {
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
    Section = MachoGetSectionByIndex64 (Context, (Symbol->Section - 1));
    if (Section == NULL) {
      return FALSE;
    }

    Value = ALIGN_VALUE (
              (Section->Address + LinkAddress),
              (UINT64)(1U << Section->Alignment)
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

BOOLEAN
MachoSymbolGetFileOffset64 (
  IN OUT OC_MACHO_CONTEXT      *Context,
  IN     CONST  MACH_NLIST_64  *Symbol,
  OUT    UINT32                *FileOffset,
  OUT    UINT32                *MaxSize OPTIONAL
  )
{
  UINT64                   Offset;
  UINT64                   Base;
  UINT64                   Size;
  MACH_SEGMENT_COMMAND_64  *Segment;
  MACH_SECTION_64          *Section;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  ASSERT (FileOffset != NULL);

  if (Symbol->Section == NO_SECT) {
    return FALSE;
  }

  Section = MachoGetSectionByIndex64 (
              Context,
              (Symbol->Section - 1)
              );
  if (Section == NULL || Section->Size == 0) {
    for (
      Segment = MachoGetNextSegment64 (Context, NULL);
      Segment != NULL;
      Segment = MachoGetNextSegment64 (Context, Segment)
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

  *FileOffset = (UINT32) (Base - Context->ContainerOffset + Offset);

  if (MaxSize != NULL) {
    *MaxSize = (UINT32) (Size - Offset);
  }

  return TRUE;
}

BOOLEAN
MachoSymbolGetDirectFileOffset64 (
  IN OUT OC_MACHO_CONTEXT      *Context,
  IN     UINT64                Address,
  OUT    UINT32                *FileOffset,
  OUT    UINT32                *MaxSize OPTIONAL
  )
{
  UINT64                   Offset;
  UINT64                   Base;
  UINT64                   Size;
  MACH_SEGMENT_COMMAND_64  *Segment;

  ASSERT (Context != NULL);
  ASSERT (FileOffset != NULL);

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
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

  *FileOffset = (UINT32) (Base - Context->ContainerOffset + Offset);

  if (MaxSize != NULL) {
    *MaxSize = (UINT32) (Size - Offset);
  }

  return TRUE;
}

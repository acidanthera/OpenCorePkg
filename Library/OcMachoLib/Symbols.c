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

STATIC
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
  ASSERT ((Symbol > Context->SymbolTable)
       && (Symbol <= &Context->SymbolTable[Context->Symtab->NumSymbols - 1]));
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
MachoIsSymbolValueSane64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  MACH_SEGMENT_COMMAND_64 *Segment;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  if (!MachoSymbolIsLocalDefined (Context, Symbol)) {
    return TRUE;
  }

  Segment = NULL;
  while (MachoGetNextSegment64 (Context, &Segment) && (Segment != NULL)) {
    if ((Symbol->Value >= Segment->VirtualAddress)
      && (Symbol->Value < (Segment->VirtualAddress + Segment->Size))) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Returns whether Symbol describes a section.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol  Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsSection (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (MachoIsSymbolValueSane64 (Context, Symbol));

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

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsDefined (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  return ((Symbol->Type == MACH_RELOC_ABSOLUTE)
           || MachoSymbolIsSection (Context, Symbol));
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
  ASSERT (DySymtab != NULL);

  if (DySymtab->NumUndefinedSymbols == 0) {
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

  return MachoSymbolIsDefined (Context, Symbol);
}

/**
  Retrieves a symbol by its index.

  @param[in]  Context  Context of the Mach-O.
  @param[in]  Index    Index of the symbol to locate.
  @param[out] Symbol   Pointer to return the symbol into.
                       If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSymbolByIndex64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index,
  OUT    MACH_NLIST_64     **Symbol
  )
{
  MACH_NLIST_64 *SymbolTemp;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  if (!InternalRetrieveSymtabs64 (Context)) {
    return FALSE;
  }

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab->NumSymbols > 0);

  SymbolTemp = NULL;

  if (Index < Context->Symtab->NumSymbols) {
    SymbolTemp = &Context->SymbolTable[Index];
    if (!InternalSymbolIsSane (Context, SymbolTemp)) {
      return FALSE;
    }
  }

  *Symbol = SymbolTemp;
  return TRUE;
}

/**
  Retrieves Symbol's name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to retrieve the name of.

**/
CONST CHAR8 *
MachoGetSymbolName64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (MachoIsSymbolValueSane64 (Context, Symbol));

  ASSERT (Context->SymbolTable != NULL);
  ASSERT (Context->Symtab->StringsSize > Symbol->UnifiedName.StringIndex);

  ASSERT (((Symbol->Type & MACH_N_TYPE_STAB) != 0)
       || ((Symbol->Type & MACH_N_TYPE_TYPE) != MACH_N_TYPE_INDR));

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

  ASSERT (MachoIsSymbolValueSane64 (Context, Symbol));

  ASSERT (Context->SymbolTable != NULL);

  ASSERT (((Symbol->Type & MACH_N_TYPE_STAB) == 0)
       && ((Symbol->Type & MACH_N_TYPE_TYPE) == MACH_N_TYPE_INDR));

  if (Context->Symtab->StringsSize <= Symbol->Value) {
    return NULL;
  }

  return (Context->StringTable + Symbol->Value);
}

/**
  Retrieves the symbol referenced by the Relocation targeting Address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Address to search for.
  @param[out]    Symbol   Pointer the symbol is returned into.
                          If FALSE is returned, the output data is undefined.
                          If TRUE is returned and Symbol is NULL, no
                          Relocation exists that references Address.                     

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSymbolByExternRelocationOffset64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    MACH_NLIST_64     **Symbol
  )
{
  MACH_NLIST_64        *SymbolTemp;
  BOOLEAN              Result;
  MACH_RELOCATION_INFO *Relocation;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  Result = InternalGetExternalRelocationByOffset (
             Context,
             Address,
             &Relocation
             );
  if (!Result) {
    return FALSE;
  }

  SymbolTemp = NULL;

  if (Relocation != NULL) {
    Result = MachoGetSymbolByIndex64 (
               Context,
               Relocation->SymbolNumber,
               &SymbolTemp
               );
    if (!Result || (SymbolTemp == NULL)) {
      return FALSE;
    }
  }

  *Symbol = SymbolTemp;
  return TRUE;
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
  UINT32 Index;
  INTN   Result;

  ASSERT (SymbolTable != NULL);
  ASSERT (Name != NULL);

  for (Index = 0; Index < NumberOfSymbols; ++Index) {
    Result = AsciiStrCmp (
               Name,
               MachoGetSymbolName64 (Context, &SymbolTable[Index])
               );
    if (Result == 0) {
      return &SymbolTable[Index];
    }
  }

  return NULL;
}

/**
  Retrieves a locally defined symbol by its name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Name     Name of the symbol to locate.
  @param[out]    Symbol   Pointer to return the symbol into.
                          If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetLocalDefinedSymbolByName (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *Name,
  OUT    MACH_NLIST_64     **Symbol
  )
{
  MACH_NLIST_64               *SymbolTable;
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  MACH_NLIST_64               *SymbolTemp;

  ASSERT (Context != NULL);
  ASSERT (Name != NULL);
  ASSERT (Symbol != NULL);

  if (!InternalRetrieveSymtabs64 (Context)) {
    return FALSE;
  }

  SymbolTable  = Context->SymbolTable;
  DySymtab     = Context->DySymtab;
  ASSERT (SymbolTable != NULL);
  ASSERT (DySymtab != NULL);

  SymbolTemp = InternalGetSymbolByName (
                 Context,
                 &SymbolTable[DySymtab->LocalSymbolsIndex],
                 DySymtab->NumLocalSymbols,
                 Name
                 );
  if (SymbolTemp == NULL) {
    SymbolTemp = InternalGetSymbolByName (
                   Context,
                   &SymbolTable[DySymtab->ExternalSymbolsIndex],
                   DySymtab->NumExternalSymbols,
                   Name
                   );
  }

  if ((SymbolTemp != NULL) && !InternalSymbolIsSane (Context, SymbolTemp)) {
    return FALSE;
  }

  *Symbol = SymbolTemp;
  return TRUE;
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
  MACH_SECTION_64 *Section;
  UINT64          Value;
  BOOLEAN         Result;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  ASSERT (MachoIsSymbolValueSane64 (Context, Symbol));
  ASSERT ((Symbol->Type & MACH_N_TYPE_EXT) == 0);
  //
  // Symbols are relocated when they describe sections.
  //
  if (MachoSymbolIsSection (Context, Symbol)) {
    Result = MachoGetSectionByAddress64 (Context, Symbol->Value, &Section);
    if (!Result || (Section == NULL)) {
      return FALSE;
    }

    Value = ALIGN_VALUE (
              (Section->Address + LinkAddress),
              (1UL << Section->Alignment)
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
    if (!Result) {
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

  @retval 0  0 is returned on failure.

**/
BOOLEAN
MachoSymbolGetFileOffset64 (
  IN OUT OC_MACHO_CONTEXT      *Context,
  IN     CONST  MACH_NLIST_64  *Symbol,
  OUT    UINT32                *FileOffset
  )
{
  UINT64          Offset;
  MACH_SECTION_64 *Section;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  ASSERT (FileOffset != NULL);

  ASSERT (MachoIsSymbolValueSane64 (Context, Symbol));

  if (!MachoGetSectionByIndex64 (Context, Symbol->Section, &Section)
   || (Section == NULL) || (Symbol->Value < Section->Address)) {
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
  ASSERT (Offset == (UINT32)Offset);
  *FileOffset = (Section->Offset + (UINT32)Offset);
  return TRUE;
}

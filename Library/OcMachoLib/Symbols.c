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
#include <Library/OcMachoLib.h>

#include "OcMachoLibInternal.h"

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
  IN OUT VOID                 *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  )
{
  CONST OC_MACHO_CONTEXT      *MachoContext;
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  CONST MACH_NLIST_64         *UndefinedSymbols;
  CONST MACH_NLIST_64         *UndefinedSymbolsTop;
  CONST MACH_NLIST_64         *IndirectSymbols;
  CONST MACH_NLIST_64         *IndirectSymbolsTop;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);

  MachoContext = (CONST OC_MACHO_CONTEXT *)Context;
  ASSERT (MachoContext->SymbolTable != NULL);
  ASSERT (MachoContext->DySymtab != NULL);
  //
  // The symbol must have been declared locally prior to solving.  As there is
  // no information on whether the symbol has been solved explicitely, check
  // its storage location for Undefined or Indirect.
  //
  DySymtab            = MachoContext->DySymtab;
  UndefinedSymbols    = &MachoContext->SymbolTable[DySymtab->UndefinedSymbolsIndex];
  UndefinedSymbolsTop = &UndefinedSymbols[DySymtab->NumUndefinedSymbols];

  if ((Symbol >= UndefinedSymbols) && (Symbol < UndefinedSymbolsTop)) {
    return FALSE;
  }

  IndirectSymbols = MachoContext->IndirectSymbolTable;
  IndirectSymbolsTop = &IndirectSymbols[DySymtab->NumIndirectSymbols];

  if ((Symbol >= IndirectSymbols) && (Symbol < IndirectSymbolsTop)) {
    return FALSE;
  }

  return MachoSymbolIsDefined (Symbol);
}

/**
  Retrieves a symbol by its name.

  @param[in] SymbolTable      Symbol Table of the Mach-O.
  @param[in] StringTable      String Table pf the Mach-O.
  @param[in] NumberOfSymbols  Number of symbols in SymbolTable.
  @param[in] Name             Name of the symbol to locate.

  @retval NULL  NULL is returned on failure.

**/
STATIC
CONST MACH_NLIST_64 *
InternalGetSymbolByName (
  IN CONST MACH_NLIST_64  *SymbolTable,
  IN CONST CHAR8          *StringTable,
  IN UINTN                NumberOfSymbols,
  IN CONST CHAR8          *Name
  )
{
  UINTN Index;
  INTN  Result;

  ASSERT (SymbolTable != NULL);
  ASSERT (StringTable != NULL);
  ASSERT (Name != NULL);

  for (Index = 0; Index < NumberOfSymbols; ++Index) {
    Result = AsciiStrCmp (
               Name,
               (StringTable + SymbolTable[Index].UnifiedName.StringIndex)
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

**/
CONST MACH_NLIST_64 *
MachoGetLocalDefinedSymbolByName (
  IN OUT VOID         *Context,
  IN     CONST CHAR8  *Name
  )
{
  OC_MACHO_CONTEXT            *MachoContext;
  CONST MACH_NLIST_64         *SymbolTable;
  CONST CHAR8                 *StringTable;
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  CONST MACH_NLIST_64         *Symbol;

  ASSERT (Context != NULL);
  ASSERT (Name != NULL);

  MachoContext = (OC_MACHO_CONTEXT *)Context;

  if ((MachoContext->SymbolTable == NULL)
   && !InternalRetrieveSymtabs64 (MachoContext)) {
    return NULL;
  }

  SymbolTable  = MachoContext->SymbolTable;
  StringTable  = MachoContext->StringTable;
  DySymtab     = MachoContext->DySymtab;
  ASSERT (SymbolTable != NULL);
  ASSERT (StringTable != NULL);
  ASSERT (DySymtab != NULL);

  Symbol = InternalGetSymbolByName (
             &SymbolTable[DySymtab->LocalSymbolsIndex],
             StringTable,
             DySymtab->NumLocalSymbols,
             Name
             );
  if (Symbol == NULL) {
    Symbol = InternalGetSymbolByName (
               &SymbolTable[DySymtab->ExternalSymbolsIndex],
               StringTable,
               DySymtab->NumExternalSymbols,
               Name
               );
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
  IN OUT VOID           *Context,
  IN     UINT64         LinkAddress,
  IN OUT MACH_NLIST_64  *Symbol
  )
{
  CONST MACH_SECTION_64 *Section;

  ASSERT (Context != NULL);
  ASSERT (Symbol != NULL);
  ASSERT ((Symbol->Type & MACH_N_TYPE_EXT) == 0);
  //
  // Symbols are relocated when they describe sections.
  //
  if (MachoSymbolIsSection (Symbol)) {
    Section = MachoGetSectionByAddress64 (Context, Symbol->Value);
    if (Section == NULL) {
      return FALSE;
    }

    Symbol->Value += ALIGN_VALUE (
                       (Section->Address + LinkAddress),
                       Section->Alignment
                       );
    Symbol->Value -= Section->Address;
  }

  return TRUE;
}

/**
  Retrieves a symbol by the Relocation it is referenced by.

  @param[in,out] Context     Context of the Mach-O.
  @param[in]     Relocation  The Relocation to evaluate.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_NLIST_64 *
MachoGetCxxSymbolByRelocation64 (
  IN OUT VOID                        *Context,
  IN     CONST MACH_RELOCATION_INFO  *Relocation
  )
{
  OC_MACHO_CONTEXT       *MachoContext;
  CONST MACH_NLIST_64    *SymbolTable;
  CONST CHAR8            *StringTable;
  CONST MACH_SECTION_64  *Section;
  UINT64                 Value;
  UINTN                  Index;
  CONST MACH_NLIST_64    *Symbol;
  CONST CHAR8            *Name;

  ASSERT (Context != NULL);
  ASSERT (Relocation != NULL);

  MachoContext = (OC_MACHO_CONTEXT *)Context;

  if ((MachoContext->SymbolTable == NULL)
   && !InternalRetrieveSymtabs64 (MachoContext)) {
    return NULL;
  }

  SymbolTable  = MachoContext->SymbolTable;
  StringTable  = MachoContext->StringTable;
  ASSERT (SymbolTable != NULL);
  ASSERT (StringTable != NULL);
  //
  // Scattered Relocations are only supported by i386.
  //
  ASSERT (((UINT32)Relocation->Address & MACH_RELOC_SCATTERED) == 0);

  if (Relocation->Extern != 0) {
    return &SymbolTable[Relocation->SymbolNumber];
  }

  Section = MachoGetSectionByIndex64 (Context, Relocation->SymbolNumber);
  if (Section == NULL) {
    return NULL;
  }

  Value = *(CONST UINT64 *)(
             (UINTN)((CONST OC_MACHO_CONTEXT *)Context)->MachHeader
               + (Section->Address + Relocation->Address)
              );
  for (Index = 0; Index < MachoContext->NumSymbols; ++Index) {
    Symbol = &SymbolTable[Index];
    Name   = (StringTable + Symbol->UnifiedName.StringIndex);

    if ((Symbol->Value == Value) && MachoIsSymbolNameCxx (Name)) {
      return Symbol;
    }
  }

  return NULL;
}

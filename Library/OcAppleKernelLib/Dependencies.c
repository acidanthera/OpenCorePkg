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
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcAppleKernelLib.h>

#include "PrelinkedInternal.h"

/**
  Fills SymbolTable with the symbols provided in Symbols.  For performance
  reasons, the C++ symbols are continuously added to the top of the buffer.
  Their order is not preserved.  SymbolTable->SymbolTable is expected to be
  a valid buffer that can store at least NumSymbols symbols.

  @param[in]     MachoContext  Context of the Mach-O.
  @param[in]     NumSymbols    The number of symbols to copy.
  @param[in]     Symbols       The source symbol array.
  @param[in,out] SymbolTable   The desination Symbol List.  Must be able to
                               hold at least NumSymbols symbols.

**/
VOID
InternalFillSymbolTable64 (
  IN OUT OC_MACHO_CONTEXT     *MachoContext,
  IN     UINT32               NumSymbols,
  IN     CONST MACH_NLIST_64  *Symbols,
  IN OUT OC_SYMBOL_TABLE_64   *SymbolTable
  )
{
  OC_SYMBOL_64        *WalkerBottom;
  OC_SYMBOL_64        *WalkerTop;
  UINT32              NumCxxSymbols;
  UINT32              Index;
  CONST MACH_NLIST_64 *Symbol;
  CONST CHAR8         *Name;
  BOOLEAN             Result;

  ASSERT (MachoContext != NULL);
  ASSERT (Symbols != NULL);
  ASSERT (SymbolTable != NULL);

  WalkerBottom = &SymbolTable->Symbols[0];
  WalkerTop    = &SymbolTable->Symbols[SymbolTable->NumSymbols - 1];

  NumCxxSymbols = 0;

  for (Index = 0; Index < NumSymbols; ++Index) {
    Symbol = &Symbols[Index];
    Name   = MachoGetSymbolName64 (MachoContext, Symbol);
    Result = MachoSymbolNameIsCxx (Name);

    if (!Result) {
      WalkerBottom->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerBottom->Value       = Symbol->Value;
      ++WalkerBottom;
    } else {
      WalkerTop->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerTop->Value       = Symbol->Value;
      --WalkerTop;

      ++NumCxxSymbols;
    }
  }

  SymbolTable->NumSymbols    = NumSymbols;
  SymbolTable->NumCxxSymbols = NumCxxSymbols;
}

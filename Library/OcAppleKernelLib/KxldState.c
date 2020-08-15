/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Base.h>

#include <IndustryStandard/AppleKxldState.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcXmlLib.h>

#include "PrelinkedInternal.h"

STATIC
CONST KXLD_LINK_STATE_HEADER *
InternalGetKxldHeader (
  IN  CONST VOID     *KxldState,
  IN  UINT32         KxldStateSize,
  IN  MACH_CPU_TYPE  CpuType
  )
{
  CONST KXLD_LINK_STATE_HEADER  *Header;

  if (KxldStateSize < sizeof (KXLD_LINK_STATE_HEADER)
    || !OC_TYPE_ALIGNED (KXLD_LINK_STATE_HEADER, KxldState)) {
    return NULL;
  }

  Header = KxldState;

  if (Header->Signature != KXLD_LINK_STATE_SIGNATURE
    || Header->Version != KXLD_LINK_STATE_VERSION
    || Header->CpuType != CpuType) {
    return NULL;
  }

  return Header;
}

STATIC
CONST KXLD_VTABLE_HEADER *
InternalGetKxldVtables (
  IN  CONST VOID     *KxldState,
  IN  UINT32         KxldStateSize,
  IN  MACH_CPU_TYPE  CpuType,
  OUT UINT32         *NumVtables
  )
{
  CONST KXLD_LINK_STATE_HEADER  *Header;
  CONST KXLD_VTABLE_HEADER      *Vtables;
  UINT32                        SymbolSize;
  UINT32                        Index;
  UINT32                        End;

  Header = InternalGetKxldHeader (KxldState, KxldStateSize, CpuType);
  if (Header == NULL) {
    return NULL;
  }

  if (Header->NumVtables == 0) {
    return NULL;
  }

  if (CpuType == MachCpuTypeX86) {
    SymbolSize = sizeof (KXLD_SYM_ENTRY_32);
    if (!OC_TYPE_ALIGNED (UINT32, Header->VtableOffset)) {
      return NULL;
    }
  } else if (CpuType == MachCpuTypeX8664) {
    SymbolSize = sizeof (KXLD_SYM_ENTRY_64);
    if (!OC_TYPE_ALIGNED (UINT64, Header->VtableOffset)) {
      return NULL;
    }
  } else {
    return NULL;
  }

  if (OcOverflowMulAddU32 (Header->NumVtables, sizeof (KXLD_VTABLE_HEADER), Header->VtableOffset, &End)
    || End > KxldStateSize) {
    return NULL;
  }

  Vtables = (KXLD_VTABLE_HEADER *) ((UINT8 *) KxldState + Header->VtableOffset);

  for (Index = 0; Index < Header->NumVtables; ++Index) {
    if (OcOverflowMulAddU32 (Vtables[Index].NumEntries, SymbolSize, Vtables[Index].EntryOffset, &End)
      || End > KxldStateSize) {
      return NULL;
    }
  }

  *NumVtables = Header->NumVtables;
  return Vtables;
}

STATIC
CONST VOID *
InternalGetKxldSymbols (
  IN  CONST VOID     *KxldState,
  IN  UINT32         KxldStateSize,
  IN  MACH_CPU_TYPE  CpuType,
  OUT UINT32         *NumSymbols
  )
{
  CONST KXLD_LINK_STATE_HEADER  *Header;
  UINT32                        SymbolSize;
  UINT32                        End;

  Header = InternalGetKxldHeader (KxldState, KxldStateSize, CpuType);
  if (Header == NULL) {
    return NULL;
  }

  if (Header->NumSymbols == 0) {
    return NULL;
  }

  if (CpuType == MachCpuTypeX86) {
    SymbolSize = sizeof (KXLD_SYM_ENTRY_32);
    if (!OC_TYPE_ALIGNED (UINT32, Header->SymbolOffset)) {
      return NULL;
    }
  } else if (CpuType == MachCpuTypeX8664) {
    SymbolSize = sizeof (KXLD_SYM_ENTRY_64);
    if (!OC_TYPE_ALIGNED (UINT64, Header->SymbolOffset)) {
      return NULL;
    }
  } else {
    return NULL;
  }

  if (OcOverflowMulAddU32 (Header->NumSymbols, SymbolSize, Header->SymbolOffset, &End)
    || End > KxldStateSize) {
    return NULL;
  }

  *NumSymbols = Header->NumSymbols;
  return ((UINT8 *) KxldState + Header->SymbolOffset);
}

STATIC
CONST CHAR8 *
InternalGetKxldString (
  IN  CONST VOID     *KxldState,
  IN  UINT32         KxldStateSize,
  IN  UINT32         Offset
  )
{
  CONST CHAR8   *SymbolWalker;
  CONST CHAR8   *SymbolWalkerEnd;

  if (Offset >= KxldStateSize) {
    return NULL;
  }

  SymbolWalker    = (CONST CHAR8*) KxldState + Offset;
  SymbolWalkerEnd = (CONST CHAR8*) KxldState + KxldStateSize;

  while (SymbolWalker < SymbolWalkerEnd) {
    if (*SymbolWalker == '\0') {
      return (CONST CHAR8*) KxldState + Offset;
    }
    ++SymbolWalker;
  }

  return NULL;
}

EFI_STATUS
InternalKxldStateBuildLinkedSymbolTable (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  PRELINKED_KEXT_SYMBOL    *SymbolTable;
  PRELINKED_KEXT_SYMBOL    *WalkerBottom;
  PRELINKED_KEXT_SYMBOL    *WalkerTop;
  CONST CHAR8              *Name;
  CONST KXLD_SYM_ENTRY_64  *KxldSymbols;
  UINT32                   Index;
  UINT32                   NumSymbols;
  UINT32                   NumCxxSymbols;
  BOOLEAN                  Result;  

  if (Kext->KxldState == NULL || Kext->KxldStateSize == 0) {
    DEBUG ((DEBUG_INFO, "OCAK: %a has %u size KXLD, ignoring symbols\n", Kext->Identifier, Kext->KxldStateSize));
    return EFI_SUCCESS;
  }

  if (Kext->LinkedSymbolTable != NULL) {
    return EFI_SUCCESS;
  }

  KxldSymbols = InternalGetKxldSymbols (
    Kext->KxldState,
    Kext->KxldStateSize,
    MachCpuTypeX8664,
    &NumSymbols
    );

  if (KxldSymbols == NULL) {
    return EFI_UNSUPPORTED;
  }

  SymbolTable = AllocatePool (NumSymbols * sizeof (*SymbolTable));
  if (SymbolTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  WalkerBottom = &SymbolTable[0];
  WalkerTop    = &SymbolTable[NumSymbols - 1];

  NumCxxSymbols = 0;

  DEBUG ((
    DEBUG_INFO,
    "OCAK: Processing %a KXLD state with %u symbols\n",
    Kext->Identifier,
    NumSymbols
    ));

  for (Index = 0; Index < NumSymbols; ++Index) {
    Name = InternalGetKxldString (
      Kext->KxldState,
      Kext->KxldStateSize,
      KxldSymbols->NameOffset
      );
    if (Name == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Result = MachoSymbolNameIsCxx (Name);

    DEBUG ((
      DEBUG_VERBOSE,
      "OCAK: Adding symbol %a with %Lx value (flags %u)\n",
      Name,
      KxldSymbols->Address,
      KxldSymbols->Flags
      ));

    if (!Result) {
      WalkerBottom->Value  = KxldSymbols->Address;
      WalkerBottom->Name   = Name;
      WalkerBottom->Length = (UINT32)AsciiStrLen (WalkerBottom->Name);
      ++WalkerBottom;
    } else {
      WalkerTop->Value  = KxldSymbols->Address;
      WalkerTop->Name   = Name;
      WalkerTop->Length = (UINT32)AsciiStrLen (WalkerTop->Name);
      --WalkerTop;

      ++NumCxxSymbols;
    }

    ++KxldSymbols;
  }

  Kext->NumberOfSymbols    = NumSymbols;
  Kext->NumberOfCxxSymbols = NumCxxSymbols;
  Kext->LinkedSymbolTable  = SymbolTable;

  return EFI_SUCCESS;
}

EFI_STATUS
InternalKxldStateBuildLinkedVtables (
  IN OUT PRELINKED_KEXT     *Kext,
  IN     PRELINKED_CONTEXT  *Context
  )
{
  PRELINKED_VTABLE                 *LinkedVtables;
  PRELINKED_VTABLE                 *CurrentVtable;
  CONST KXLD_SYM_ENTRY_64          *KxldSymbols;
  CONST KXLD_VTABLE_HEADER         *KxldVtables;
  UINT32                           Index;
  UINT32                           Index2;
  UINT32                           NumVtables;
  UINT32                           NumEntries;
  UINT32                           ResultingSize;

  if (Kext->KxldState == NULL || Kext->KxldStateSize == 0) {
    DEBUG ((DEBUG_INFO, "OCAK: %a has %u size KXLD, ignoring symbols\n", Kext->Identifier, Kext->KxldStateSize));
    return EFI_SUCCESS;
  }

  if (Kext->LinkedVtables != NULL) {
    return EFI_SUCCESS;
  }

  KxldVtables = InternalGetKxldVtables (
    Kext->KxldState,
    Kext->KxldStateSize,
    MachCpuTypeX8664,
    &NumVtables
    );

  if (KxldVtables == NULL) {
    return EFI_UNSUPPORTED;
  }

  NumEntries = 0;

  for (Index = 0; Index < Kext->NumberOfVtables; ++Index) {
    if (OcOverflowAddU32 (NumEntries, KxldVtables[Index].NumEntries, &NumEntries)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if (OcOverflowMulU32 (NumVtables, sizeof (*LinkedVtables), &ResultingSize)
    || OcOverflowMulAddU32 (NumEntries, sizeof (*LinkedVtables->Entries), ResultingSize, &ResultingSize)) {
    return EFI_OUT_OF_RESOURCES;
  }

  LinkedVtables = AllocatePool (ResultingSize);
  if (LinkedVtables == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentVtable = LinkedVtables;
  for (Index = 0; Index < NumVtables; ++Index) {
    CurrentVtable->Name = InternalGetKxldString (
      Kext->KxldState,
      Kext->KxldStateSize,
      KxldVtables[Index].NameOffset
      );
    if (CurrentVtable->Name == NULL) {
      FreePool (LinkedVtables);
      return EFI_INVALID_PARAMETER;
    }

    KxldSymbols = (KXLD_SYM_ENTRY_64 *) ((UINT8 *) Kext->KxldState + KxldVtables[Index].EntryOffset);
    CurrentVtable->NumEntries = KxldVtables[Index].NumEntries;

    DEBUG ((
      DEBUG_VERBOSE,
      "OCAK: Adding vtable %a (%u/%u) for %a of %u entries\n",
      CurrentVtable->Name,
      Index + 1,
      NumVtables,
      Kext->Identifier,
      CurrentVtable->NumEntries
      ));

    for (Index2 = 0; Index2 < CurrentVtable->NumEntries; ++Index2) {
      CurrentVtable->Entries[Index2].Address = KxldSymbols->Address;
      CurrentVtable->Entries[Index2].Name    = InternalGetKxldString (
        Kext->KxldState,
        Kext->KxldStateSize,
        KxldSymbols->NameOffset
        );
      if (CurrentVtable->Entries[Index2].Name == NULL) {
        FreePool (LinkedVtables);
        return EFI_INVALID_PARAMETER;
      }

      ++KxldSymbols;
    }

    CurrentVtable = GET_NEXT_PRELINKED_VTABLE (CurrentVtable);
  }

  Kext->LinkedVtables   = LinkedVtables;
  Kext->NumberOfVtables = NumVtables;

  return EFI_SUCCESS;
}

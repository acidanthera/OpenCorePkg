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
  CONST KXLD_SYM_ENTRY_ANY *KxldSymbols;
  UINT64                   KxldAddress;
  UINT32                   Index;
  UINT32                   NumSymbols;
  UINT32                   NumCxxSymbols;
  BOOLEAN                  Result;  

  ASSERT (Kext->Context.KxldState != NULL);
  ASSERT (Kext->Context.KxldStateSize > 0);

  if (Kext->LinkedSymbolTable != NULL) {
    return EFI_SUCCESS;
  }

  KxldSymbols = InternalGetKxldSymbols (
    Kext->Context.KxldState,
    Kext->Context.KxldStateSize,
    Context->Is32Bit ? MachCpuTypeI386 : MachCpuTypeX8664,
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
    DEBUG_VERBOSE,
    "OCAK: Processing %a-bit %a KXLD state with %u symbols\n",
    Context->Is32Bit ? "32" : "64",
    Kext->Identifier,
    NumSymbols
    ));

  for (Index = 0; Index < NumSymbols; ++Index) {
    Name = InternalGetKxldString (
      Kext->Context.KxldState,
      Kext->Context.KxldStateSize,
      Context->Is32Bit ? KxldSymbols->Kxld32.NameOffset : KxldSymbols->Kxld64.NameOffset
      );
    if (Name == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Result = MachoSymbolNameIsCxx (Name);

    KxldAddress = Context->Is32Bit ? KxldSymbols->Kxld32.Address : KxldSymbols->Kxld64.Address;

    DEBUG ((
      DEBUG_VERBOSE,
      "OCAK: Adding %a-bit symbol %a with %Lx value (flags %u)\n",
      Context->Is32Bit ? "32" : "64",
      Name,
      KxldAddress,
      Context->Is32Bit ? KxldSymbols->Kxld32.Flags : KxldSymbols->Kxld64.Flags
      ));

    if (!Result) {
      WalkerBottom->Value  = KxldAddress;
      WalkerBottom->Name   = Name;
      WalkerBottom->Length = (UINT32) AsciiStrLen (WalkerBottom->Name);
      ++WalkerBottom;
    } else {
      WalkerTop->Value  = KxldAddress;
      WalkerTop->Name   = Name;
      WalkerTop->Length = (UINT32) AsciiStrLen (WalkerTop->Name);
      --WalkerTop;

      ++NumCxxSymbols;
    }

    KxldSymbols = KXLD_ANY_NEXT (Context->Is32Bit, KxldSymbols);
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
  CONST KXLD_SYM_ENTRY_ANY         *KxldSymbols;
  CONST KXLD_VTABLE_HEADER         *KxldVtables;
  UINT32                           Index;
  UINT32                           Index2;
  UINT32                           NumVtables;
  UINT32                           NumEntries;
  UINT32                           ResultingSize;

  ASSERT (Kext->Context.KxldState != NULL);
  ASSERT (Kext->Context.KxldStateSize > 0);

  if (Kext->LinkedVtables != NULL) {
    return EFI_SUCCESS;
  }

  KxldVtables = InternalGetKxldVtables (
    Kext->Context.KxldState,
    Kext->Context.KxldStateSize,
    Context->Is32Bit ? MachCpuTypeI386 : MachCpuTypeX8664,
    &NumVtables
    );

  //
  // Some KPIs may not have vtables (e.g. BSD).
  //
  if (KxldVtables == NULL) {
    Kext->LinkedVtables   = NULL;
    Kext->NumberOfVtables = 0;
    return EFI_SUCCESS;
  }

  NumEntries = 0;

  for (Index = 0; Index < NumVtables; ++Index) {
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
      Kext->Context.KxldState,
      Kext->Context.KxldStateSize,
      KxldVtables[Index].NameOffset
      );
    if (CurrentVtable->Name == NULL) {
      FreePool (LinkedVtables);
      return EFI_INVALID_PARAMETER;
    }

    KxldSymbols = (KXLD_SYM_ENTRY_ANY *) ((UINT8 *) Kext->Context.KxldState + KxldVtables[Index].EntryOffset);
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
      CurrentVtable->Entries[Index2].Address = Context->Is32Bit ? KxldSymbols->Kxld32.Address : KxldSymbols->Kxld64.Address;
      CurrentVtable->Entries[Index2].Name    = InternalGetKxldString (
        Kext->Context.KxldState,
        Kext->Context.KxldStateSize,
        Context->Is32Bit ? KxldSymbols->Kxld32.NameOffset : KxldSymbols->Kxld64.NameOffset
        );
      if (CurrentVtable->Entries[Index2].Name == NULL) {
        FreePool (LinkedVtables);
        return EFI_INVALID_PARAMETER;
      }

      KxldSymbols = KXLD_ANY_NEXT (Context->Is32Bit, KxldSymbols);
    }

    CurrentVtable = GET_NEXT_PRELINKED_VTABLE (CurrentVtable);
  }

  Kext->LinkedVtables   = LinkedVtables;
  Kext->NumberOfVtables = NumVtables;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalKxldStateRebasePlist (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     INT64              Delta
  )
{
  UINT32                   Index;
  UINT32                   KextCount;
  UINT32                   FieldIndex;
  UINT32                   FieldCount;
  XML_NODE                 *KextPlist;
  CONST CHAR8              *KextPlistKey;
  CHAR8                    *ScratchWalker;
  XML_NODE                 *KextPlistValue;
  UINT64                   KxldState;

  KextCount = XmlNodeChildren (Context->KextList);

  Context->KextScratchBuffer = ScratchWalker = AllocatePool (KextCount * KEXT_OFFSET_STR_LEN);
  if (Context->KextScratchBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < KextCount; ++Index) {
    KextPlist = PlistNodeCast (XmlNodeChild (Context->KextList, Index), PLIST_NODE_TYPE_DICT);

    if (KextPlist == NULL) {
      continue;
    }

    FieldCount = PlistDictChildren (KextPlist);
    for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
      KextPlistKey = PlistKeyValue (PlistDictChild (KextPlist, FieldIndex, &KextPlistValue));
      if (KextPlistKey == NULL) {
        continue;
      }

      if (AsciiStrCmp (KextPlistKey, PRELINK_INFO_LINK_STATE_ADDR_KEY) == 0) {
        if (!PlistIntegerValue (KextPlistValue, &KxldState, sizeof (KxldState), TRUE)) {
          return EFI_INVALID_PARAMETER;
        }

        DEBUG ((DEBUG_VERBOSE, "OCAK: Shifting 0x%Lx to 0x%Lx\n", KxldState, KxldState + Delta));

        KxldState += Delta;

        if (!AsciiUint64ToLowerHex (ScratchWalker, KEXT_OFFSET_STR_LEN, KxldState)) {
          return EFI_INVALID_PARAMETER;
        }
        XmlNodeChangeContent (KextPlistValue, ScratchWalker);
        ScratchWalker += AsciiStrSize (ScratchWalker);
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalKxldStateRebuild (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  UINT32      AlignedSize;
  UINT32      NewSize;

  //
  // This is a requirement from 10.6.8, should be guaranteed?
  //
  ASSERT (OC_POT_ALIGNED (MACHO_PAGE_SIZE, Context->PrelinkedLastAddress));

  //
  // Append prelink state for 10.6.8
  //
  AlignedSize = MACHO_ALIGN (Context->PrelinkedStateKernelSize);
  if (OcOverflowAddU32 (Context->PrelinkedSize, AlignedSize, &NewSize)
    || NewSize > Context->PrelinkedAllocSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Context->Is32Bit) {
    Context->PrelinkedStateSegment->Segment32.VirtualAddress = (UINT32) Context->PrelinkedLastAddress;
    Context->PrelinkedStateSegment->Segment32.Size           = AlignedSize;
    Context->PrelinkedStateSegment->Segment32.FileOffset     = Context->PrelinkedSize;
    Context->PrelinkedStateSegment->Segment32.FileSize       = AlignedSize;
    Context->PrelinkedStateSectionKernel->Section32.Address  = (UINT32) Context->PrelinkedLastAddress;
    Context->PrelinkedStateSectionKernel->Section32.Offset   = Context->PrelinkedSize;
    Context->PrelinkedStateSectionKernel->Section32.Size     = Context->PrelinkedStateKernelSize;
  } else {
    Context->PrelinkedStateSegment->Segment64.VirtualAddress = Context->PrelinkedLastAddress;
    Context->PrelinkedStateSegment->Segment64.Size           = AlignedSize;
    Context->PrelinkedStateSegment->Segment64.FileOffset     = Context->PrelinkedSize;
    Context->PrelinkedStateSegment->Segment64.FileSize       = AlignedSize;
    Context->PrelinkedStateSectionKernel->Section64.Address  = Context->PrelinkedLastAddress;
    Context->PrelinkedStateSectionKernel->Section64.Offset   = Context->PrelinkedSize;
    Context->PrelinkedStateSectionKernel->Section64.Size     = Context->PrelinkedStateKernelSize;
  }

  CopyMem (
    &Context->Prelinked[Context->PrelinkedSize],
    Context->PrelinkedStateKernel,
    Context->PrelinkedStateKernelSize
    );
  ZeroMem (
    &Context->Prelinked[Context->PrelinkedSize + Context->PrelinkedStateKernelSize],
    AlignedSize - Context->PrelinkedStateKernelSize
    );

  Context->PrelinkedLastAddress += AlignedSize;
  Context->PrelinkedSize        += AlignedSize;

  AlignedSize = MACHO_ALIGN (Context->PrelinkedStateKextsSize);
  if (OcOverflowAddU32 (Context->PrelinkedSize, AlignedSize, &NewSize)
    || NewSize > Context->PrelinkedAllocSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Context->Is32Bit) {
    Context->PrelinkedStateSegment->Segment32.Size         += AlignedSize;
    Context->PrelinkedStateSegment->Segment32.FileSize     += AlignedSize;
    Context->PrelinkedStateSectionKexts->Section32.Address  = (UINT32) Context->PrelinkedLastAddress;
    Context->PrelinkedStateSectionKexts->Section32.Offset   = Context->PrelinkedSize;
    Context->PrelinkedStateSectionKexts->Section32.Size     = Context->PrelinkedStateKextsSize;
  } else {
    Context->PrelinkedStateSegment->Segment64.Size         += AlignedSize;
    Context->PrelinkedStateSegment->Segment64.FileSize     += AlignedSize;
    Context->PrelinkedStateSectionKexts->Section64.Address  = Context->PrelinkedLastAddress;
    Context->PrelinkedStateSectionKexts->Section64.Offset   = Context->PrelinkedSize;
    Context->PrelinkedStateSectionKexts->Section64.Size     = Context->PrelinkedStateKextsSize;
  }

  CopyMem (
    &Context->Prelinked[Context->PrelinkedSize],
    Context->PrelinkedStateKexts,
    Context->PrelinkedStateKextsSize
    );
  ZeroMem (
    &Context->Prelinked[Context->PrelinkedSize + Context->PrelinkedStateKextsSize],
    AlignedSize - Context->PrelinkedStateKextsSize
    );

  Context->PrelinkedLastAddress += AlignedSize;
  Context->PrelinkedSize        += AlignedSize;

  if ((Context->Is32Bit ?
    Context->PrelinkedStateSectionKexts->Section32.Address : Context->PrelinkedStateSectionKexts->Section64.Address)
    != Context->PrelinkedStateKextsAddress) {
    Status = InternalKxldStateRebasePlist (
      Context,
      (INT64) ((Context->Is32Bit ?
        Context->PrelinkedStateSectionKexts->Section32.Address : Context->PrelinkedStateSectionKexts->Section64.Address)
        - Context->PrelinkedStateKextsAddress)
      );
    if (!EFI_ERROR (Status)) {
      Context->PrelinkedStateKextsAddress = Context->Is32Bit ?
        Context->PrelinkedStateSectionKexts->Section32.Address : Context->PrelinkedStateSectionKexts->Section64.Address;
    }
    DEBUG ((
      DEBUG_INFO,
      "OCAK: Rebasing %a-bit KXLD state from %Lx to %Lx - %r\n",
      Context->Is32Bit ? "32" : "64",
      Context->PrelinkedStateKextsAddress,
      Context->Is32Bit ?
        Context->PrelinkedStateSectionKexts->Section32.Address : Context->PrelinkedStateSectionKexts->Section64.Address,
      Status
      ));
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

UINT64
InternalKxldSolveSymbol (
  IN BOOLEAN       Is32Bit,
  IN CONST VOID    *KxldState,
  IN UINT32        KxldStateSize,
  IN CONST CHAR8   *Name
  )
{
  CONST CHAR8              *LocalName;
  CONST KXLD_SYM_ENTRY_ANY *KxldSymbols;
  UINT64                   KxldAddress;
  UINT32                   Index;
  UINT32                   NumSymbols;

  ASSERT (KxldState != NULL);
  ASSERT (KxldStateSize > 0);
  ASSERT (Name != NULL);

  KxldSymbols = InternalGetKxldSymbols (
    KxldState,
    KxldStateSize,
    Is32Bit ? MachCpuTypeI386 : MachCpuTypeX8664,
    &NumSymbols
    );

  if (KxldSymbols == NULL) {
    return 0;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "OCAK: Processing %a-bit KXLD state for %a with %u symbols\n",
    Is32Bit ? "32" : "64",
    Name,
    NumSymbols
    ));

  for (Index = 0; Index < NumSymbols; ++Index) {
    LocalName = InternalGetKxldString (
      KxldState,
      KxldStateSize,
      Is32Bit ? KxldSymbols->Kxld32.NameOffset : KxldSymbols->Kxld64.NameOffset
      );
    if (LocalName == NULL) {
      return 0;
    }

    KxldAddress = Is32Bit ? KxldSymbols->Kxld32.Address : KxldSymbols->Kxld64.Address;

    DEBUG ((
      DEBUG_VERBOSE,
      "OCAK: Checking %a-bit symbol %a with %Lx value (flags %u)\n",
      Is32Bit ? "32" : "64",
      LocalName,
      KxldAddress,
      Is32Bit ? KxldSymbols->Kxld32.Flags : KxldSymbols->Kxld64.Flags
      ));

    if (AsciiStrCmp (LocalName, Name) == 0) {
      return KxldAddress;
    }

    KxldSymbols = KXLD_ANY_NEXT (Is32Bit, KxldSymbols);
  }

  return 0;
}

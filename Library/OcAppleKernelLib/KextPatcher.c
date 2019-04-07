/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Base.h>

#include <IndustryStandard/AppleKmodInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcXmlLib.h>

#include "PrelinkedInternal.h"

RETURN_STATUS
PatcherInitContextFromPrelinked (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Name
  )
{
  PRELINKED_KEXT  *Kext;

  Kext = InternalCachedPrelinkedKext (Prelinked, Name);
  if (Kext == NULL) {
    return RETURN_NOT_FOUND;
  }

  CopyMem (Context, &Kext->Context, sizeof (*Context));
  return RETURN_SUCCESS;
}

RETURN_STATUS
PatcherInitContextFromBuffer (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT UINT8              *Buffer,
  IN     UINT32             BufferSize
  )
{
  MACH_SEGMENT_COMMAND_64  *Segment;

  ASSERT (Context != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);

  //
  // This interface is still used for the kernel due to the need to patch
  // standalone kernel outside of prelinkedkernel in e.g. 10.9.
  // Once 10.9 support is dropped one could call PatcherInitContextFromPrelinked
  // and request PRELINK_KERNEL_IDENTIFIER.
  //

  if (!MachoInitializeContext (&Context->MachContext, Buffer, BufferSize)) {
    return RETURN_INVALID_PARAMETER;
  }

  Segment = MachoGetSegmentByName64 (
    &Context->MachContext,
    "__TEXT"
    );
  if (Segment == NULL || Segment->VirtualAddress < Segment->FileOffset) {
    return RETURN_NOT_FOUND;
  }

  Context->VirtualBase = Segment->VirtualAddress - Segment->FileOffset;
  Context->VirtualKmod = 0;

  return RETURN_SUCCESS;
}

RETURN_STATUS
PatcherGetSymbolAddress (
  IN OUT PATCHER_CONTEXT    *Context,
  IN     CONST CHAR8        *Name,
  IN OUT UINT8              **Address
  )
{
  MACH_NLIST_64  *Symbol;
  CONST CHAR8    *SymbolName;
  UINT32         Offset;
  UINT32         Index;

  Index = 0;
  while (TRUE) {
    Symbol = MachoGetSymbolByIndex64 (&Context->MachContext, Index);
    if (Symbol == NULL) {
      return RETURN_NOT_FOUND;
    }

    SymbolName = MachoGetSymbolName64 (&Context->MachContext, Symbol);

    if (SymbolName && AsciiStrCmp (Name, SymbolName) == 0) {
      break;
    }

    Index++;
  }

  if (!MachoSymbolGetFileOffset64 (&Context->MachContext, Symbol, &Offset, NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  *Address = (UINT8 *)MachoGetMachHeader64 (&Context->MachContext) + Offset;
  return RETURN_SUCCESS;
}

RETURN_STATUS
PatcherApplyGenericPatch (
  IN OUT PATCHER_CONTEXT        *Context,
  IN     PATCHER_GENERIC_PATCH  *Patch
  )
{
  RETURN_STATUS  Status;
  UINT8       *Base;
  UINT32      Size;
  UINT32      ReplaceCount;

  Base = (UINT8 *)MachoGetMachHeader64 (&Context->MachContext);
  Size = MachoGetFileSize (&Context->MachContext);
  if (Patch->Base != NULL) {
    Status = PatcherGetSymbolAddress (Context, Patch->Base, &Base);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    Size -= (UINT32)(Base - (UINT8 *)MachoGetMachHeader64 (&Context->MachContext));
  }

  if (Patch->Find == NULL) {
    if (Size < Patch->Size) {
      return RETURN_NOT_FOUND;
    }
    CopyMem (Base, Patch->Replace, Patch->Size);
    return RETURN_SUCCESS;
  }

  if (Patch->Limit > 0 && Patch->Limit < Size) {
    Size = Patch->Limit;
  }

  ReplaceCount = ApplyPatch (
    Patch->Find,
    Patch->Mask,
    Patch->Size,
    Patch->Replace,
    Patch->ReplaceMask,
    Base,
    Size,
    Patch->Count,
    Patch->Skip
    );

  if (ReplaceCount > 0 && Patch->Count > 0 && ReplaceCount != Patch->Count) {
    DEBUG ((
      DEBUG_INFO,
      "Performed only %u replacements out of %u\n",
      ReplaceCount,
      Patch->Count
      ));
  }

  if (ReplaceCount > 0) {
    return RETURN_SUCCESS;
  }

  return RETURN_NOT_FOUND;
}

RETURN_STATUS
PatcherBlockKext (
  IN OUT PATCHER_CONTEXT        *Context
  )
{
  UINT64           KmodOffset;
  UINT64           KmodStartAddr;
  UINT64           TmpOffset;
  KMOD_INFO_64_V1  *KmodInfo;
  UINT8            *PatchAddr;

  //
  // Kernel has 0 kmod.
  //
  if (Context->VirtualKmod == 0 || Context->VirtualBase > Context->VirtualKmod) {
    return RETURN_UNSUPPORTED;
  }

  KmodOffset = Context->VirtualKmod - Context->VirtualBase;
  KmodInfo   = (KMOD_INFO_64_V1 *)((UINT8 *) MachoGetMachHeader64 (&Context->MachContext) + KmodOffset);
  if (OcOverflowAddU64 (KmodOffset, sizeof (KMOD_INFO_64_V1), &TmpOffset)
    || KmodOffset > MachoGetFileSize (&Context->MachContext)) {
    return RETURN_INVALID_PARAMETER;
  }

  CopyMem (&KmodStartAddr, &KmodInfo->StartAddr, sizeof (KmodStartAddr));
  if (KmodStartAddr == 0 || Context->VirtualBase > KmodInfo->StartAddr) {
    return RETURN_BAD_BUFFER_SIZE;
  }

  TmpOffset = KmodStartAddr - Context->VirtualBase;
  if (TmpOffset > MachoGetFileSize (&Context->MachContext) - 6) {
    return RETURN_BUFFER_TOO_SMALL;
  }

  PatchAddr = (UINT8 *)MachoGetMachHeader64 (&Context->MachContext) + TmpOffset;

  //
  // mov eax, KMOD_RETURN_FAILURE
  // ret
  //
  PatchAddr[0] = 0xB8;
  PatchAddr[1] = KMOD_RETURN_FAILURE;
  PatchAddr[2] = 0x00;
  PatchAddr[3] = 0x00;
  PatchAddr[4] = 0x00;
  PatchAddr[5] = 0xC3;

  return RETURN_SUCCESS;
}

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

#include "MkextInternal.h"
#include "PrelinkedInternal.h"

STATIC
BOOLEAN
GetTextBaseOffset (
  IN  OC_MACHO_CONTEXT  *ExecutableContext,
  OUT UINT64            *Address,
  OUT UINT64            *Offset
  )
{
  MACH_SEGMENT_COMMAND      *Segment32;
  MACH_SECTION              *Section32;
  MACH_SEGMENT_COMMAND_64   *Segment64;

  UINT64                    VirtualAddress;
  UINT64                    FileOffset;

  //
  // 32-bit can be of type MH_OBJECT, which has all sections in a single unnamed segment.
  // We'll fallback to that if there is no __TEXT segment.
  //
  if (ExecutableContext->Is32Bit) {
    Segment32 = MachoGetNextSegment32 (ExecutableContext, NULL);
    if (Segment32 == NULL) {
      return FALSE;
    }

    if (AsciiStrCmp (Segment32->SegmentName, "") == 0) {
      Section32 = MachoGetSectionByName32 (
        ExecutableContext,
        Segment32,
        "__text"
        );
      if (Section32 == NULL) {
        return FALSE;
      }

      VirtualAddress = Section32->Address;
      FileOffset     = Section32->Offset;

    } else {
      Segment32 = MachoGetSegmentByName32 (
        ExecutableContext,
        "__TEXT"
        );
      if (Segment32 == NULL || Segment32->VirtualAddress < Segment32->FileOffset) {
        return FALSE;
      }

      VirtualAddress = Segment32->VirtualAddress;
      FileOffset     = Segment32->FileOffset;
    }

  } else {
    Segment64 = MachoGetSegmentByName64 (
      ExecutableContext,
      "__TEXT"
      );
    if (Segment64 == NULL || Segment64->VirtualAddress < Segment64->FileOffset) {
      return FALSE;
    }

    VirtualAddress = Segment64->VirtualAddress;
    FileOffset     = Segment64->FileOffset;
  }

  *Address  = VirtualAddress;
  *Offset   = FileOffset;

  return TRUE;
}

EFI_STATUS
PatcherInitContextFromPrelinked (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Name
  )
{
  PRELINKED_KEXT  *Kext;

  Kext = InternalCachedPrelinkedKext (Prelinked, Name);
  if (Kext == NULL) {
    return EFI_NOT_FOUND;
  }

  CopyMem (Context, &Kext->Context, sizeof (*Context));
  return EFI_SUCCESS;
}

EFI_STATUS
PatcherInitContextFromMkext(
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT MKEXT_CONTEXT      *Mkext,
  IN     CONST CHAR8        *Name
  )
{
  MKEXT_KEXT  *Kext;

  Kext = InternalCachedMkextKext (Mkext, Name);
  if (Kext == NULL) {
    return EFI_NOT_FOUND;
  }

  return PatcherInitContextFromBuffer (Context, &Mkext->Mkext[Kext->BinaryOffset], Kext->BinarySize, Mkext->Is32Bit);
}

EFI_STATUS
PatcherInitContextFromBuffer (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT UINT8              *Buffer,
  IN     UINT32             BufferSize,
  IN     BOOLEAN            Is32Bit
  )
{
  EFI_STATUS                Status;
  OC_MACHO_CONTEXT          InnerContext;

  UINT64                    VirtualAddress;
  UINT64                    FileOffset;

  ASSERT (Context != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);

  //
  // This interface is still used for the kernel due to the need to patch
  // standalone kernel outside of prelinkedkernel in e.g. 10.9.
  // Once 10.9 support is dropped one could call PatcherInitContextFromPrelinked
  // and request PRELINK_KERNEL_IDENTIFIER.
  //

  if (!MachoInitializeContext (&Context->MachContext, Buffer, BufferSize, 0, Is32Bit)) {
    DEBUG ((
      DEBUG_INFO,
      "OCAK: %a-bit patcher init from buffer %p %u has unsupported mach-o\n",
      Is32Bit ? "32" : "64",
      Buffer, 
      BufferSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!GetTextBaseOffset (&Context->MachContext, &VirtualAddress, &FileOffset)) {
    return EFI_NOT_FOUND;
  }

  Context->Is32Bit            = Is32Bit;
  Context->VirtualBase        = VirtualAddress;
  Context->FileOffset         = FileOffset;
  Context->VirtualKmod        = 0;
  Context->KxldState          = NULL;
  Context->KxldStateSize      = 0;
  Context->IsKernelCollection = FALSE;

  KextFindKmodAddress (
    &Context->MachContext,
    0,
    BufferSize,
    &Context->VirtualKmod
    );

  if (!Context->Is32Bit) {
    Status = InternalConnectExternalSymtab (
      &Context->MachContext,
      &InnerContext,
      Buffer,
      BufferSize,
      NULL
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "OCAK: %a-bit patcher base 0x%llX kmod 0x%llX file 0x%llX\n",
    Is32Bit ? "32" : "64",
    Context->VirtualBase,
    Context->VirtualKmod,
    Context->FileOffset
    ));

  return EFI_SUCCESS;
}

EFI_STATUS
PatcherGetSymbolAddress (
  IN OUT PATCHER_CONTEXT    *Context,
  IN     CONST CHAR8        *Name,
  IN OUT UINT8              **Address
  )
{
  MACH_NLIST_ANY  *Symbol;
  CONST CHAR8     *SymbolName;
  UINT64          SymbolAddress;
  UINT32          Offset;
  UINT32          Index;

  Index  = 0;
  Offset = 0;
  while (TRUE) {
    //
    // Try the usual way first via SYMTAB.
    //
    Symbol = MachoGetSymbolByIndex (&Context->MachContext, Index);
    if (Symbol == NULL) {
      //
      // If we have KxldState, use it.
      //
      if (Index == 0 && Context->KxldState != NULL) {
        SymbolAddress = InternalKxldSolveSymbol (
          Context->Is32Bit,
          Context->KxldState,
          Context->KxldStateSize,
          Name
          );
        //
        // If we have a symbol, get its ondisk offset.
        //
        if (SymbolAddress != 0 && MachoSymbolGetDirectFileOffset (&Context->MachContext, SymbolAddress, &Offset, NULL)) {
          //
          // Proceed to success.
          //
          break;
        }
      }

      return EFI_NOT_FOUND;
    }

    SymbolName = MachoGetSymbolName (&Context->MachContext, Symbol);
    if (SymbolName != NULL && AsciiStrCmp (Name, SymbolName) == 0) {
      //
      // Once we have a symbol, get its ondisk offset.
      //
      if (MachoSymbolGetFileOffset (&Context->MachContext, Symbol, &Offset, NULL)) {
        //
        // Proceed to success.
        //
        break;
      }

      return EFI_INVALID_PARAMETER;
    }

    Index++;
  }

  *Address = (UINT8 *) MachoGetMachHeader (&Context->MachContext) + Offset;
  return EFI_SUCCESS;
}

EFI_STATUS
PatcherApplyGenericPatch (
  IN OUT PATCHER_CONTEXT        *Context,
  IN     PATCHER_GENERIC_PATCH  *Patch
  )
{
  EFI_STATUS     Status;
  UINT8          *Base;
  UINT32         Size;
  UINT32         ReplaceCount;

  Base = (UINT8 *) MachoGetMachHeader (&Context->MachContext);
  Size = MachoGetFileSize (&Context->MachContext);
  if (Patch->Base != NULL) {
    Status = PatcherGetSymbolAddress (Context, Patch->Base, &Base);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: %a-bit %a base lookup failure %r\n",
        Context->Is32Bit ? "32" : "64",
        Patch->Comment != NULL ? Patch->Comment : "Patch",
        Status
        ));
      return Status;
    }

    Size -= (UINT32)(Base - (UINT8 *) MachoGetMachHeader (&Context->MachContext));
  }

  if (Patch->Find == NULL) {
    if (Size < Patch->Size) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: %a-bit %a is borked, not found\n",
        Context->Is32Bit ? "32" : "64",
        Patch->Comment != NULL ? Patch->Comment : "Patch"
        ));
      return EFI_NOT_FOUND;
    }
    CopyMem (Base, Patch->Replace, Patch->Size);
    return EFI_SUCCESS;
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

  DEBUG ((
    DEBUG_INFO,
    "OCAK: %a-bit %a replace count - %u\n",
    Context->Is32Bit ? "32" : "64",
    Patch->Comment != NULL ? Patch->Comment : "Patch",
    ReplaceCount
    ));

  if (ReplaceCount > 0 && Patch->Count > 0 && ReplaceCount != Patch->Count) {
    DEBUG ((
      DEBUG_INFO,
      "OCAK: %a-bit %a performed only %u replacements out of %u\n",
      Context->Is32Bit ? "32" : "64",
      Patch->Comment != NULL ? Patch->Comment : "Patch",
      ReplaceCount,
      Patch->Count
      ));
  }

  if (ReplaceCount > 0) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
PatcherBlockKext (
  IN OUT PATCHER_CONTEXT        *Context
  )
{
  UINT8             *MachBase;
  UINT32            MachSize;
  UINT64            KmodOffset;
  UINT64            StartAddr;
  UINT64            TmpOffset;
  UINT8             *KmodInfo;
  UINT8             *PatchAddr;

  //
  // Kernel has 0 kmod.
  //
  if (Context->VirtualKmod == 0 || Context->VirtualBase > Context->VirtualKmod) {
    return EFI_UNSUPPORTED;
  }

  MachBase = (UINT8 *) MachoGetMachHeader (&Context->MachContext);
  MachSize = MachoGetFileSize (&Context->MachContext);

  //
  // Determine offset of kmod within file.
  //
  KmodOffset = Context->VirtualKmod - Context->VirtualBase;
  if (OcOverflowAddU64 (KmodOffset, Context->FileOffset, &KmodOffset)
    || OcOverflowAddU64 (KmodOffset, Context->Is32Bit ? sizeof (KMOD_INFO_32_V1) : sizeof (KMOD_INFO_64_V1), &TmpOffset)
    || TmpOffset > MachSize) {
    return EFI_INVALID_PARAMETER;
  }

  KmodInfo = MachBase + KmodOffset;
  if (Context->Is32Bit) {
    StartAddr = ((KMOD_INFO_32_V1 *) KmodInfo)->StartAddr;
  } else {
    StartAddr = ((KMOD_INFO_64_V1 *) KmodInfo)->StartAddr;
    if (Context->IsKernelCollection) {
      StartAddr = KcFixupValue (StartAddr, NULL);
    }
  }

  if (StartAddr == 0 || Context->VirtualBase > StartAddr) {
    return EFI_INVALID_PARAMETER;
  }

  TmpOffset = StartAddr - Context->VirtualBase;
  if (OcOverflowAddU64 (TmpOffset, Context->FileOffset, &TmpOffset)
    || TmpOffset > MachSize - 6) {
    return EFI_BUFFER_TOO_SMALL;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "OCAK: %a-bit blocker start @ 0x%llX\n",
    Context->Is32Bit ? "32" : "64",
    TmpOffset
    ));

  PatchAddr = MachBase + TmpOffset;

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

  return EFI_SUCCESS;
}

BOOLEAN
KextFindKmodAddress (
  IN  OC_MACHO_CONTEXT  *ExecutableContext,
  IN  UINT64            LoadAddress,
  IN  UINT32            Size,
  OUT UINT64            *Kmod
  )
{
  BOOLEAN                   Is32Bit;
  MACH_NLIST_ANY            *Symbol;
  CONST CHAR8               *SymbolName;
  UINT64                    Address;
  UINT64                    FileOffset;
  UINT32                    Index;

  Is32Bit = ExecutableContext->Is32Bit;
  Index   = 0;

  while (TRUE) {
    Symbol = MachoGetSymbolByIndex (ExecutableContext, Index);
    if (Symbol == NULL) {
      *Kmod = 0;
      return TRUE;
    }

    if (((Is32Bit ? Symbol->Symbol32.Type : Symbol->Symbol64.Type) & MACH_N_TYPE_STAB) == 0) {
      SymbolName = MachoGetSymbolName (ExecutableContext, Symbol);
      if (SymbolName && AsciiStrCmp (SymbolName, "_kmod_info") == 0) {
        if (!MachoIsSymbolValueInRange (ExecutableContext, Symbol)) {
          return FALSE;
        }
        break;
      }
    }

    Index++;
  }

  if (!GetTextBaseOffset (ExecutableContext, &Address, &FileOffset)) {
    return FALSE;
  }

  if (OcOverflowTriAddU64 (Address, LoadAddress, Is32Bit ? Symbol->Symbol32.Value : Symbol->Symbol64.Value, &Address)
    || Address > LoadAddress + Size - (Is32Bit ? sizeof (KMOD_INFO_32_V1) : sizeof (KMOD_INFO_64_V1))
    || (Is32Bit && Address > MAX_UINT32)) {
    return FALSE;
  }

  *Kmod = Address;
  return TRUE;
}

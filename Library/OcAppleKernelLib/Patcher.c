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

#include <IndustryStandard/AppleKmodInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcXmlLib.h>
#include <Library/PrintLib.h>

#include "Link.h"

STATIC
EFI_STATUS
InternalInitPatcherContext (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT UINT8              *Buffer,
  IN     UINT32             BufferSize,
  IN     UINT64             VirtualBase OPTIONAL,
  IN     UINT64             VirtualKmod OPTIONAL
  )
{
  MACH_SEGMENT_COMMAND_64  *Segment;

  if (!MachoInitializeContext (&Context->MachContext, Buffer, BufferSize)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VirtualBase == 0) {
    //
    // Get the segment containing kernel segment.
    //
    Segment = MachoGetSegmentByName64 (
      &Context->MachContext,
      "__TEXT"
      );
    if (Segment == NULL || Segment->VirtualAddress < Segment->FileOffset) {
      return EFI_NOT_FOUND;
    }

    VirtualBase = Segment->VirtualAddress - Segment->FileOffset;
  }

  Context->VirtualBase = VirtualBase;
  Context->VirtualKmod = VirtualKmod;

  return EFI_SUCCESS;
}

PRELINKED_KEXT *
InternalGetPrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Identifier
  )
{
  PRELINKED_KEXT  *NewKext;
  LIST_ENTRY      *Kext;
  EFI_STATUS      Status;
  UINT32          Index;
  UINT32          KextCount;
  UINT32          FieldIndex;
  UINT32          FieldCount;
  XML_NODE        *KextPlist;
  XML_NODE        *KextPlistKey;
  XML_NODE        *KextPlistValue;
  XML_NODE        *BundleLibraries;
  CONST CHAR8     *KextIdentifier;
  CONST CHAR8     *CompatibleVersion;
  UINT64          SourceBase;
  UINT64          SourceSize;
  UINT64          SourceEnd;
  UINT64          VirtualBase;
  UINT64          VirtualKmod;
  BOOLEAN         Found;

  //
  // Find cached entry if any.
  //
  Kext = GetFirstNode (&Prelinked->PrelinkedKexts);
  while (!IsNull (&Prelinked->PrelinkedKexts, Kext)) {
    if (AsciiStrCmp (Identifier, GET_PRELINKED_KEXT_FROM_LINK (Kext)->Identifier) == 0) {
      return GET_PRELINKED_KEXT_FROM_LINK (Kext);
    }

    Kext = GetNextNode (&Prelinked->PrelinkedKexts, Kext);
  }

  //
  // Try with real entry.
  //
  Found     = FALSE;
  KextCount = XmlNodeChildren (Prelinked->KextList);

  for (Index = 0; Index < KextCount && !Found; ++Index) {
    KextPlist = PlistNodeCast (XmlNodeChild (Prelinked->KextList, Index), PLIST_NODE_TYPE_DICT);

    if (KextPlist == NULL) {
      continue;
    }

    VirtualBase       = 0;
    VirtualKmod       = 0;
    SourceBase        = 0;
    SourceSize        = 0;
    BundleLibraries   = NULL;
    CompatibleVersion = NULL;

    FieldCount = PlistDictChildren (KextPlist);
    for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
      KextPlistKey = PlistDictChild (KextPlist, FieldIndex, &KextPlistValue);
      if (KextPlistKey == NULL) {
        continue;
      }

      if (!Found && AsciiStrCmp (PlistKeyValue (KextPlistKey), INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
        KextIdentifier = XmlNodeContent (KextPlistValue);
        if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_STRING) == NULL || KextIdentifier == NULL) {
          break;
        }
        if (AsciiStrCmp (KextIdentifier, Identifier) == 0) {
          Found = TRUE;
          continue;
        }
      } else if (VirtualBase == 0 && AsciiStrCmp (PlistKeyValue (KextPlistKey), PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY) == 0) {
        if (!PlistIntegerValue (KextPlistValue, &VirtualBase, sizeof (VirtualBase), TRUE)) {
          break;
        }
      } else if (VirtualKmod == 0 && AsciiStrCmp (PlistKeyValue (KextPlistKey), PRELINK_INFO_KMOD_INFO_KEY) == 0) {
        if (!PlistIntegerValue (KextPlistValue, &VirtualKmod, sizeof (VirtualKmod), TRUE)) {
          break;
        }
      } else if (SourceBase == 0 && AsciiStrCmp (PlistKeyValue (KextPlistKey), PRELINK_INFO_EXECUTABLE_SOURCE_ADDR_KEY) == 0) {
        if (!PlistIntegerValue (KextPlistValue, &SourceBase, sizeof (SourceBase), TRUE)) {
          break;
        }
      } else if (SourceSize == 0 && AsciiStrCmp (PlistKeyValue (KextPlistKey), PRELINK_INFO_EXECUTABLE_SIZE_KEY) == 0) {
        if (!PlistIntegerValue (KextPlistValue, &SourceSize, sizeof (SourceSize), TRUE)) {
          break;
        }
      } else if (BundleLibraries == NULL && AsciiStrCmp (PlistKeyValue (KextPlistKey), INFO_BUNDLE_LIBRARIES_KEY) == 0) {
        if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_DICT) == NULL) {
          break;
        }
        BundleLibraries = KextPlistValue;
      } else if (CompatibleVersion == NULL && AsciiStrCmp (PlistKeyValue (KextPlistKey), INFO_BUNDLE_COMPATIBLE_VERSION_KEY) == 0) {
        if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_STRING) == NULL) {
          break;
        }
        CompatibleVersion = XmlNodeContent (KextPlistValue);
        if (CompatibleVersion == NULL) {
          break;
        }
      }

      if (Found
        && VirtualBase != 0
        && VirtualKmod != 0
        && SourceBase != 0
        && SourceSize != 0
        && BundleLibraries != NULL
        && CompatibleVersion != NULL) {
        break;
      }
    }
  }

  //
  // CompatibleVersion is optional and thus not checked.
  //
  if (!Found || VirtualBase == 0 || VirtualKmod == 0 || SourceBase == 0 || SourceSize == 0 || BundleLibraries == NULL) {
    return NULL;
  }

  if (SourceBase < VirtualBase) {
    return NULL;
  }

  SourceBase -= Prelinked->PrelinkedTextSegment->VirtualAddress;
  if (OcOverflowAddU64 (SourceBase, Prelinked->PrelinkedTextSegment->FileOffset, &SourceBase) ||
    OcOverflowAddU64 (SourceBase, SourceSize, &SourceEnd) ||
    SourceEnd > Prelinked->PrelinkedSize) {
    return NULL;
  }

  NewKext = AllocatePool (sizeof (*NewKext));
  if (NewKext == NULL) {
    return NULL;
  }

  Status = InternalInitPatcherContext (
    &NewKext->Context,
    &Prelinked->Prelinked[SourceBase],
    (UINT32) SourceSize,
    VirtualBase,
    VirtualKmod
    );

  if (EFI_ERROR (Status)) {
    FreePool (NewKext);
    return NULL;
  }

  NewKext->Signature         = PRELINKED_KEXT_SIGNATURE;
  NewKext->Identifier        = KextIdentifier;
  NewKext->BundleLibraries   = BundleLibraries;
  NewKext->CompatibleVersion = CompatibleVersion;

  InsertTailList (&Prelinked->PrelinkedKexts, &NewKext->Link);

  return NewKext;
}

VOID
InternalFreePrelinkedKexts (
  LIST_ENTRY  *Kexts
  )
{
  LIST_ENTRY      *Link;
  PRELINKED_KEXT  *Kext;

  while (!IsListEmpty (Kexts)) {
    Link = GetFirstNode (Kexts);
    Kext = GET_PRELINKED_KEXT_FROM_LINK (Link);
    RemoveEntryList (Link);
    FreePool (Kext);
  }

  ZeroMem (Kexts, sizeof (*Kexts));
}

EFI_STATUS
PatcherInitContextFromPrelinked (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Name
  )
{
  PRELINKED_KEXT  *Kext;

  Kext = InternalGetPrelinkedKext (Prelinked, Name);
  if (Kext == NULL) {
    return EFI_NOT_FOUND;
  }

  CopyMem (Context, &Kext->Context, sizeof (*Context));
  return EFI_SUCCESS;
}

EFI_STATUS
PatcherInitContextFromBuffer (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT UINT8              *Buffer,
  IN     UINT32             BufferSize
  )
{
  return InternalInitPatcherContext (Context, Buffer, BufferSize, 0, 0);
}

EFI_STATUS
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
      return EFI_NOT_FOUND;
    }

    SymbolName = MachoGetSymbolName64 (&Context->MachContext, Symbol);

    if (SymbolName && AsciiStrCmp (Name, SymbolName) == 0) {
      break;
    }

    Index++;
  }

  if (!MachoSymbolGetFileOffset64 (&Context->MachContext, Symbol, &Offset)) {
    return EFI_INVALID_PARAMETER;
  }

  *Address = (UINT8 *) MachoGetMachHeader64 (&Context->MachContext) + Offset;
  return EFI_SUCCESS;
}

EFI_STATUS
PatcherApplyGenericPatch (
  IN OUT PATCHER_CONTEXT        *Context,
  IN     PATCHER_GENERIC_PATCH  *Patch
  )
{
  EFI_STATUS  Status;
  UINT8       *Base;
  UINT32      Size;
  UINT32      ReplaceCount;

  Base = (UINT8 *) MachoGetMachHeader64 (&Context->MachContext);
  Size = MachoGetFileSize (&Context->MachContext);
  if (Patch->Base != NULL) {
    Status = PatcherGetSymbolAddress (Context, Patch->Base, &Base);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Size -= (UINT32) (Base - (UINT8 *) MachoGetMachHeader64 (&Context->MachContext));
  }

  if (Patch->Find == NULL) {
    if (Size < Patch->Size) {
      return EFI_NOT_FOUND;
    }
    CopyMem (Base, Patch->Replace, Patch->Size);
    return EFI_SUCCESS;
  }

  ReplaceCount = ApplyPatch (
    Patch->Find,
    Patch->Mask,
    Patch->Size,
    Patch->Replace,
    Base,
    Size,
    Patch->Count,
    Patch->Skip
    );

  if ((ReplaceCount > 0 && Patch->Count == 0)
    || (ReplaceCount == Patch->Count && Patch->Count > 0)) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
PatcherBlockKext (
  IN OUT PATCHER_CONTEXT        *Context
  )
{
  UINT64           KmodOffset;
  UINT64           TmpOffset;
  KMOD_INFO_64_V1  *KmodInfo;
  UINT8            *PatchAddr;

  //
  // Kernel has 0 kmod.
  //
  if (Context->VirtualKmod == 0 || Context->VirtualBase > Context->VirtualKmod) {
    return EFI_UNSUPPORTED;
  }

  KmodOffset = Context->VirtualKmod - Context->VirtualBase;
  KmodInfo   = (KMOD_INFO_64_V1 *) ((UINT8 *) MachoGetMachHeader64 (&Context->MachContext) + KmodOffset);
  if (OcOverflowAddU64 (KmodOffset, sizeof (KMOD_INFO_64_V1), &TmpOffset)
    || KmodOffset > MachoGetFileSize (&Context->MachContext)
    || !OC_ALIGNED (KmodInfo)
    || KmodInfo->StartAddr == 0
    || Context->VirtualBase > KmodInfo->StartAddr) {
    return EFI_INVALID_PARAMETER;
  }

  TmpOffset = KmodInfo->StartAddr - Context->VirtualBase;
  if (TmpOffset > MachoGetFileSize (&Context->MachContext) - 6) {
    return EFI_INVALID_PARAMETER;
  }

  PatchAddr = (UINT8 *) MachoGetMachHeader64 (&Context->MachContext) + TmpOffset;

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

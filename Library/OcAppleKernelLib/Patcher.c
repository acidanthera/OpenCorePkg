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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcXmlLib.h>
#include <Library/PrintLib.h>

EFI_STATUS
PatcherInitContextFromPrelinked (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Name
  )
{
  UINT32    Index;
  UINT32    KextCount;
  UINT32    FieldIndex;
  UINT32    FieldCount;
  XML_NODE  *KextPlist;
  XML_NODE  *KextPlistKey;
  XML_NODE  *KextPlistValue;
  UINT64    SourceBase;
  UINT64    SourceSize;
  UINT64    SourceEnd;
  UINT64    VirtualBase;
  UINT64    VirtualKmod;
  BOOLEAN   Found;

  Found     = FALSE;
  KextCount = XmlNodeChildren (Prelinked->KextList);

  for (Index = 0; Index < KextCount && !Found; ++Index) {
    KextPlist = PlistNodeCast (XmlNodeChild (Prelinked->KextList, Index), PLIST_NODE_TYPE_DICT);

    if (KextPlist == NULL) {
      continue;
    }

    VirtualBase = 0;
    VirtualKmod = 0;
    SourceBase  = 0;
    SourceSize  = 0;
    BOOLEAN HasId = FALSE;

    FieldCount = PlistDictChildren (KextPlist);
    for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
      KextPlistKey = PlistDictChild (KextPlist, FieldIndex, &KextPlistValue);
      if (KextPlistKey == NULL) {
        continue;
      }

      if (!Found && AsciiStrCmp (PlistKeyValue (KextPlistKey), INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
        HasId = TRUE;
        if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_STRING) == NULL
          || XmlNodeContent (KextPlistValue) == NULL) {
          break;
        }
        if (AsciiStrCmp (XmlNodeContent (KextPlistValue), Name) == 0) {
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
      }

      if (Found && VirtualBase != 0 && VirtualKmod != 0 && SourceBase != 0 && SourceSize != 0) {
        break;
      }
    }
  }

  if (!Found || VirtualBase == 0 || VirtualKmod == 0 || SourceBase == 0 || SourceSize == 0) {
    return EFI_NOT_FOUND;
  }

  if (SourceBase < VirtualBase) {
    return EFI_INVALID_PARAMETER;
  }

  SourceBase -= Prelinked->PrelinkedTextSegment->VirtualAddress;
  if (OcOverflowAddU64 (SourceBase, Prelinked->PrelinkedTextSegment->FileOffset, &SourceBase) ||
    OcOverflowAddU64 (SourceBase, SourceSize, &SourceEnd) ||
    SourceEnd > Prelinked->PrelinkedSize) {
    return EFI_INVALID_PARAMETER;
  }

  return PatcherInitContextFromBuffer (
    Context,
    &Prelinked->Prelinked[SourceBase],
    (UINT32) SourceSize,
    VirtualBase,
    VirtualKmod
    );
}

EFI_STATUS
PatcherInitContextFromBuffer (
  IN OUT PATCHER_CONTEXT    *Context,
  IN OUT UINT8              *Buffer,
  IN     UINT32             BufferSize,
  IN     UINT64             VirtualBase OPTIONAL,
  IN     UINT64             VirtualKmod OPTIONAL
  )
{
  if (!MachoInitializeContext (&Context->MachContext, Buffer, BufferSize)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VirtualBase == 0) {
    //
    // TODO: iterate segments and get it.
    //
    return EFI_UNSUPPORTED;
  }

  Context->VirtualBase = VirtualBase;
  Context->VirtualKmod = VirtualKmod;

  return EFI_SUCCESS;
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

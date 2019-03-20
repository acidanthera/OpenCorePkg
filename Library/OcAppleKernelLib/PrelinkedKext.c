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

#include "PrelinkedInternal.h"

/**
  Creates new uncached PRELINKED_KEXT from pool.

  @param[in] Prelinked  Initialises PRELINKED_KEXT from prelinked buffer.
  @param[in] KextPlist  Plist root node with Kext Information.
  @param[in] Identifier Abort on CFBundleIdentifier mismatch.

  @return allocated PRELINKED_KEXT or NULL.
**/
STATIC
PRELINKED_KEXT *
InternalCreatePrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Prelinked OPTIONAL,
  IN XML_NODE               *KextPlist,
  IN CONST CHAR8            *Identifier OPTIONAL
  )
{
  PRELINKED_KEXT  *NewKext;
  UINT32          FieldIndex;
  UINT32          FieldCount;
  CONST CHAR8     *KextPlistKey;
  XML_NODE        *KextPlistValue;
  CONST CHAR8     *KextIdentifier;
  XML_NODE        *BundleLibraries;
  CONST CHAR8     *CompatibleVersion;
  UINT64          VirtualBase;
  UINT64          VirtualKmod;
  UINT64          SourceBase;
  UINT64          SourceSize;
  UINT64          SourceEnd;
  BOOLEAN         Found;

  KextIdentifier    = NULL;
  BundleLibraries   = NULL;
  CompatibleVersion = NULL;
  VirtualBase       = 0;
  VirtualKmod       = 0;
  SourceBase        = 0;
  SourceSize        = 0;

  Found       = Identifier == NULL;

  FieldCount = PlistDictChildren (KextPlist);
  for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
    KextPlistKey = PlistKeyValue (PlistDictChild (KextPlist, FieldIndex, &KextPlistValue));
    if (KextPlistKey == NULL) {
      continue;
    }

    if (KextIdentifier == NULL && AsciiStrCmp (KextPlistKey, INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
      KextIdentifier = XmlNodeContent (KextPlistValue);
      if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_STRING) == NULL || KextIdentifier == NULL) {
        break;
      }
      if (!Found && AsciiStrCmp (KextIdentifier, Identifier) == 0) {
        Found = TRUE;
      }
    } else if (BundleLibraries == NULL && AsciiStrCmp (KextPlistKey, INFO_BUNDLE_LIBRARIES_KEY) == 0) {
      if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_DICT) == NULL) {
        break;
      }
      BundleLibraries = KextPlistValue;
    } else if (CompatibleVersion == NULL && AsciiStrCmp (KextPlistKey, INFO_BUNDLE_COMPATIBLE_VERSION_KEY) == 0) {
      if (PlistNodeCast (KextPlistValue, PLIST_NODE_TYPE_STRING) == NULL) {
        break;
      }
      CompatibleVersion = XmlNodeContent (KextPlistValue);
      if (CompatibleVersion == NULL) {
        break;
      }
    } else if (Prelinked != NULL && VirtualBase == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &VirtualBase, sizeof (VirtualBase), TRUE)) {
        break;
      }
    } else if (Prelinked != NULL && VirtualKmod == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_KMOD_INFO_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &VirtualKmod, sizeof (VirtualKmod), TRUE)) {
        break;
      }
    } else if (Prelinked != NULL && SourceBase == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_SOURCE_ADDR_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &SourceBase, sizeof (SourceBase), TRUE)) {
        break;
      }
    } else if (Prelinked != NULL && SourceSize == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_SIZE_KEY) == 0) {
      if (!PlistIntegerValue (KextPlistValue, &SourceSize, sizeof (SourceSize), TRUE)) {
        break;
      }
    }

    if (KextIdentifier != NULL && BundleLibraries != NULL && CompatibleVersion != NULL
      && (Prelinked == NULL || (Prelinked != NULL && VirtualBase != 0 && VirtualKmod != 0 && SourceBase != 0 && SourceSize != 0))) {
      break;
    }
  }

  //
  // BundleLibraries, CompatibleVersion, and KmodInfo are optional and thus not checked.
  //
  if (!Found || KextIdentifier == NULL || SourceBase < VirtualBase
    || (Prelinked != NULL && (VirtualBase == 0 || SourceBase == 0 || SourceSize == 0))) {
    return NULL;
  }

  if (Prelinked != NULL) {
    SourceBase -= Prelinked->PrelinkedTextSegment->VirtualAddress;
    if (OcOverflowAddU64 (SourceBase, Prelinked->PrelinkedTextSegment->FileOffset, &SourceBase) ||
      OcOverflowAddU64 (SourceBase, SourceSize, &SourceEnd) ||
      SourceEnd > Prelinked->PrelinkedSize) {
      return NULL;
    }
  }

  //
  // Important to ZeroPool for dependency cleanup.
  //
  NewKext = AllocateZeroPool (sizeof (*NewKext));
  if (NewKext == NULL) {
    return NULL;
  }

  if (Prelinked != NULL
    && !MachoInitializeContext (&NewKext->Context.MachContext, &Prelinked->Prelinked[SourceBase], SourceSize)) {
    FreePool (NewKext);
    return NULL;
  }

  NewKext->Signature            = PRELINKED_KEXT_SIGNATURE;
  NewKext->Identifier           = KextIdentifier;
  NewKext->BundleLibraries      = BundleLibraries;
  NewKext->CompatibleVersion    = CompatibleVersion;
  NewKext->Context.VirtualBase  = VirtualBase;
  NewKext->Context.VirtualKmod  = VirtualKmod;

  return NewKext;
}

STATIC
EFI_STATUS
InternalScanCurrentPrelinkedKext (
  IN OUT PRELINKED_KEXT  *Kext
  )
{
  if (Kext->LinkEditSegment == NULL) {
    Kext->LinkEditSegment = MachoGetSegmentByName64 (
      &Kext->Context.MachContext,
      "__LINKEDIT"
      );

    if (Kext->LinkEditSegment == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  if (Kext->SymbolTable == NULL) {
    Kext->NumberOfSymbols = MachoGetSymbolTable (
                   &Kext->Context.MachContext,
                   &Kext->SymbolTable,
                   &Kext->StringTable,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL
                   );
    if (Kext->NumberOfSymbols == 0) {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalScanBuildLinkedSymbolTable (
  IN OUT PRELINKED_KEXT  *Kext
  )
{
  if (Kext->LinkedSymbolTable != NULL) {
    return EFI_ALREADY_STARTED;
  }

  //TODO: port InternalFillSymbolTable64.
  return EFI_UNSUPPORTED;
}

PRELINKED_KEXT *
InternalNewPrelinkedKext (
  IN OC_MACHO_CONTEXT       *Context,
  IN XML_NODE               *KextPlist
  )
{
  PRELINKED_KEXT  *NewKext;

  NewKext = InternalCreatePrelinkedKext (NULL, KextPlist, NULL);
  if (NewKext == NULL) {
    return NULL;
  }

  CopyMem (&NewKext->Context.MachContext, Context, sizeof (NewKext->Context.MachContext));
  return NewKext;
}

VOID
InternalFreePrelinkedKext (
  IN PRELINKED_KEXT  *Kext
  )
{
  if (Kext->LinkedSymbolTable != NULL) {
    FreePool (Kext->LinkedSymbolTable);
    Kext->LinkedSymbolTable = NULL;
  }

  FreePool (Kext);
}

PRELINKED_KEXT *
InternalCachedPrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Identifier
  )
{
  PRELINKED_KEXT  *NewKext;
  LIST_ENTRY      *Kext;
  UINT32          Index;
  UINT32          KextCount;
  XML_NODE        *KextPlist;

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
  NewKext   = NULL;
  KextCount = XmlNodeChildren (Prelinked->KextList);
  for (Index = 0; Index < KextCount; ++Index) {
    KextPlist = PlistNodeCast (XmlNodeChild (Prelinked->KextList, Index), PLIST_NODE_TYPE_DICT);

    if (KextPlist == NULL) {
      continue;
    }

    NewKext = InternalCreatePrelinkedKext (Prelinked, KextPlist, Identifier);
    if (NewKext != NULL) {
      break;
    }
  }

  if (NewKext == NULL) {
    return NULL;
  }

  InsertTailList (&Prelinked->PrelinkedKexts, &NewKext->Link);

  return NewKext;
}

EFI_STATUS
InternalScanPrelinkedKext (
  IN OUT PRELINKED_KEXT     *Kext,
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  EFI_STATUS      Status;
  UINT32          FieldCount;
  UINT32          FieldIndex;
  UINT32          DependencyIndex;
  CONST CHAR8     *DependencyId;
  PRELINKED_KEXT  *DependencyKext;

  if (Kext->Dependencies[0] != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (Kext->BundleLibraries == NULL) {
    return EFI_SUCCESS;
  }

  Status = InternalScanCurrentPrelinkedKext (Kext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DependencyIndex = 0;
  FieldCount = PlistDictChildren (Kext->BundleLibraries);
  for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
    DependencyId = PlistKeyValue (PlistDictChild (Kext->BundleLibraries, FieldIndex, NULL));
    if (DependencyId == NULL) {
      continue;
    }

    DependencyKext = InternalCachedPrelinkedKext (Context, DependencyId);
    if (DependencyKext == NULL) {
      return EFI_NOT_FOUND;
    }

    if (DependencyIndex >= ARRAY_SIZE (Kext->Dependencies)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = InternalScanPrelinkedKext (DependencyKext, Context);
    if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
      return Status;
    }

    Status = InternalScanBuildLinkedSymbolTable (DependencyKext);
    if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
      return Status;
    }

    Kext->Dependencies[DependencyIndex] = DependencyKext;
    ++DependencyIndex;
  }

  return EFI_SUCCESS;
}

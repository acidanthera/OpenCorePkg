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

#include <Uefi.h>

#include <IndustryStandard/AppleKmodInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcStringLib.h>

#include "PrelinkedInternal.h"
#include "ProcessorBind.h"

STATIC
UINT64
PrelinkedFindLastLoadAddress (
  IN XML_NODE  *KextList
  )
{
  UINT32       KextCount;
  UINT32       KextIndex;
  UINT32       FieldIndex;
  UINT32       FieldCount;
  XML_NODE     *LastKext;
  CONST CHAR8  *KextPlistKey;
  XML_NODE     *KextPlistValue;
  UINT64       LoadAddress;
  UINT64       LoadSize;

  KextCount = XmlNodeChildren (KextList);
  if (KextCount == 0) {
    return 0;
  }

  //
  // Here we make an assumption that last kext has the highest load address,
  // yet there might be an arbitrary amount of trailing executable-less kexts.
  //
  for (KextIndex = 1; KextIndex <= KextCount; ++KextIndex) {
    LastKext = PlistNodeCast (XmlNodeChild (KextList, KextCount - KextIndex), PLIST_NODE_TYPE_DICT);
    if (LastKext == NULL) {
      return 0;
    }

    LoadAddress = 0;
    LoadSize = 0;

    FieldCount = PlistDictChildren (LastKext);
    for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
      KextPlistKey = PlistKeyValue (PlistDictChild (LastKext, FieldIndex, &KextPlistValue));
      if (KextPlistKey == NULL) {
        continue;
      }

      if (LoadAddress == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY) == 0) {
        if (!PlistIntegerValue (KextPlistValue, &LoadAddress, sizeof (LoadAddress), TRUE)) {
          return 0;
        }
      } else if (LoadSize == 0 && AsciiStrCmp (KextPlistKey, PRELINK_INFO_EXECUTABLE_SIZE_KEY) == 0) {
        if (!PlistIntegerValue (KextPlistValue, &LoadSize, sizeof (LoadSize), TRUE)) {
          return 0;
        }
      }

      if (LoadSize != 0 && LoadAddress != 0) {
        break;
      }
    }

    if (OcOverflowAddU64 (LoadAddress, LoadSize, &LoadAddress)) {
      return 0;
    }

    if (LoadAddress != 0) {
      break;
    }
  }

  return LoadAddress;
}

STATIC
BOOLEAN
PrelinkedFindKmodAddress (
  IN  OC_MACHO_CONTEXT  *ExecutableContext,
  IN  UINT64            LoadAddress,
  IN  UINT32            Size,
  OUT UINT64            *Kmod
  )
{
  MACH_NLIST_64            *Symbol;
  CONST CHAR8              *SymbolName;
  MACH_SEGMENT_COMMAND_64  *TextSegment;
  UINT64                   Address;
  UINT32                   Index;

  Index = 0;
  while (TRUE) {
    Symbol = MachoGetSymbolByIndex64 (ExecutableContext, Index);
    if (Symbol == NULL) {
      *Kmod = 0;
      return TRUE;
    }

    if ((Symbol->Type & MACH_N_TYPE_STAB) == 0) {
      SymbolName = MachoGetSymbolName64 (ExecutableContext, Symbol);
      if (SymbolName && AsciiStrCmp (SymbolName, "_kmod_info") == 0) {
        if (!MachoIsSymbolValueInRange64 (ExecutableContext, Symbol)) {
          return FALSE;
        }
        break;
      }
    }

    Index++;
  }

  TextSegment = MachoGetSegmentByName64 (ExecutableContext, "__TEXT");
  if (TextSegment == NULL || TextSegment->FileOffset > TextSegment->VirtualAddress) {
    return FALSE;
  }

  Address = TextSegment->VirtualAddress - TextSegment->FileOffset;
  if (OcOverflowTriAddU64 (Address, LoadAddress, Symbol->Value, &Address)
    || Address > LoadAddress + Size - sizeof (KMOD_INFO_64_V1)) {
    return FALSE;
  }

  *Kmod = Address;
  return TRUE;
}

STATIC
EFI_STATUS
PrelinkedGetSegmentsFromMacho (
  IN   OC_MACHO_CONTEXT         *MachoContext,
  OUT  MACH_SEGMENT_COMMAND_64  **PrelinkedInfoSegment,
  OUT  MACH_SECTION_64          **PrelinkedInfoSection
  )
{
  *PrelinkedInfoSegment = MachoGetSegmentByName64 (
    MachoContext,
    PRELINK_INFO_SEGMENT
    );
  if (*PrelinkedInfoSegment == NULL) {
    return EFI_NOT_FOUND;
  }
  if ((*PrelinkedInfoSegment)->FileOffset > MAX_UINT32) {
    return EFI_UNSUPPORTED;
  }

  *PrelinkedInfoSection = MachoGetSectionByName64 (
    MachoContext,
    *PrelinkedInfoSegment,
    PRELINK_INFO_SECTION
    );
  if (*PrelinkedInfoSection == NULL) {
    return EFI_NOT_FOUND;
  }
  if ((*PrelinkedInfoSection)->Size > MAX_UINT32) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalConnectExternalSymtab (
  IN OUT OC_MACHO_CONTEXT  *Context,
     OUT OC_MACHO_CONTEXT  *InnerContext,
  IN     UINT8             *Buffer,
  IN     UINT32            BufferSize,
     OUT BOOLEAN           *KernelCollection  OPTIONAL
  )
{
  MACH_HEADER_64           *Header;
  MACH_SEGMENT_COMMAND_64  *Segment;
  BOOLEAN                  IsKernelCollection;

  //
  // Detect kernel type.
  //
  Header             = MachoGetMachHeader64 (Context);
  IsKernelCollection = Header->FileType == MachHeaderFileTypeFileSet;

  if (KernelCollection != NULL) {
    *KernelCollection = IsKernelCollection;
  }

  //
  // When dealing with the kernel collections the actual kernel is pointed by one of the segments.
  //
  if (IsKernelCollection) {
    Segment = MachoGetSegmentByName64 (
      Context,
      "__TEXT_EXEC"
      );
    if (Segment == NULL || Segment->VirtualAddress < Segment->FileOffset) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: KC symtab failed locating inner %Lx %Lx (%d)\n",
        Segment != NULL ? Segment->VirtualAddress : 0,
        Segment != NULL ? Segment->FileOffset : 0,
        Segment != NULL
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (!MachoInitializeContext (
      InnerContext,
      &Buffer[Segment->FileOffset],
      (UINT32) (BufferSize - Segment->FileOffset),
      (UINT32) Segment->FileOffset)) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: KC symtab failed initialising inner %Lx %x\n",
        Segment->FileOffset,
        BufferSize
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (!MachoInitialiseSymtabsExternal64 (Context, InnerContext)) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: KC symtab failed getting symtab from inner %Lx %x\n",
        Segment->FileOffset,
        BufferSize
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrelinkedContextInit (
  IN OUT  PRELINKED_CONTEXT  *Context,
  IN OUT  UINT8              *Prelinked,
  IN      UINT32             PrelinkedSize,
  IN      UINT32             PrelinkedAllocSize
  )
{
  EFI_STATUS               Status;
  XML_NODE                 *DocumentRoot;
  XML_NODE                 *PrelinkedInfoRoot;
  CONST CHAR8              *PrelinkedInfoRootKey;
  UINT64                   SegmentEndOffset;
  UINT32                   PrelinkedInfoRootIndex;
  UINT32                   PrelinkedInfoRootCount;
  PRELINKED_KEXT           *PrelinkedKext;

  ASSERT (Context != NULL);
  ASSERT (Prelinked != NULL);
  ASSERT (PrelinkedSize > 0);
  ASSERT (PrelinkedAllocSize >= PrelinkedSize);

  ZeroMem (Context, sizeof (*Context));

  Context->Prelinked          = Prelinked;
  Context->PrelinkedSize      = MACHO_ALIGN (PrelinkedSize);
  Context->PrelinkedAllocSize = PrelinkedAllocSize;

  //
  // Ensure that PrelinkedSize is always aligned.
  //
  if (Context->PrelinkedSize != PrelinkedSize) {
    if (Context->PrelinkedSize > PrelinkedAllocSize) {
      return EFI_BUFFER_TOO_SMALL;
    }

    ZeroMem (&Prelinked[PrelinkedSize], Context->PrelinkedSize - PrelinkedSize);
  }

  //
  // Initialise primary context.
  //
  if (!MachoInitializeContext (&Context->PrelinkedMachContext, Prelinked, PrelinkedSize, 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InternalConnectExternalSymtab (
    &Context->PrelinkedMachContext,
    &Context->InnerMachContext,
    Context->Prelinked,
    Context->PrelinkedSize,
    &Context->IsKernelCollection
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize kext list with kernel pseudo kext.
  //
  InitializeListHead (&Context->PrelinkedKexts);
  InitializeListHead (&Context->InjectedKexts);
  PrelinkedKext = InternalCachedPrelinkedKernel (Context);
  if (PrelinkedKext == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Context->VirtualBase = PrelinkedKext->Context.VirtualBase;

  Context->PrelinkedLastAddress = MACHO_ALIGN (MachoGetLastAddress64 (&Context->PrelinkedMachContext));
  if (Context->PrelinkedLastAddress == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Register normal segment entries.
  //
  Status = PrelinkedGetSegmentsFromMacho (
    &Context->PrelinkedMachContext,
    &Context->PrelinkedInfoSegment,
    &Context->PrelinkedInfoSection
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Context->PrelinkedTextSegment = MachoGetSegmentByName64 (
    &Context->PrelinkedMachContext,
    PRELINK_TEXT_SEGMENT
    );
  if (Context->PrelinkedTextSegment == NULL) {
    return EFI_NOT_FOUND;
  }

  Context->PrelinkedTextSection = MachoGetSectionByName64 (
    &Context->PrelinkedMachContext,
    Context->PrelinkedTextSegment,
    PRELINK_TEXT_SECTION
    );
  if (Context->PrelinkedTextSection == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Additionally process special entries for KC.
  //
  if (Context->IsKernelCollection) {
    Status = PrelinkedGetSegmentsFromMacho (
      &Context->InnerMachContext,
      &Context->InnerInfoSegment,
      &Context->InnerInfoSection
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Context->RegionSegment = MachoGetSegmentByName64 (
      &Context->PrelinkedMachContext,
      KC_REGION0_SEGMENT
      );
    if (Context->RegionSegment == NULL) {
      return EFI_NOT_FOUND;
    }

    Context->LinkEditSegment = MachoGetSegmentByName64 (
      &Context->PrelinkedMachContext,
      KC_LINKEDIT_SEGMENT
      );
    if (Context->LinkEditSegment == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  Context->PrelinkedInfo = AllocateCopyPool (
    (UINTN)Context->PrelinkedInfoSection->Size,
    &Context->Prelinked[Context->PrelinkedInfoSection->Offset]
    );
  if (Context->PrelinkedInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Context->PrelinkedInfoDocument = XmlDocumentParse (Context->PrelinkedInfo, (UINT32)Context->PrelinkedInfoSection->Size, TRUE);
  if (Context->PrelinkedInfoDocument == NULL) {
    PrelinkedContextFree (Context);
    return EFI_INVALID_PARAMETER;
  }

  //
  // For a kernel collection the this is a full plist, while for legacy prelinked format
  // it starts with a <dict> node.
  //
  if (Context->IsKernelCollection) {
    DocumentRoot = PlistDocumentRoot (Context->PrelinkedInfoDocument);
  } else {
    DocumentRoot = XmlDocumentRoot (Context->PrelinkedInfoDocument);
  }

  PrelinkedInfoRoot = PlistNodeCast (DocumentRoot, PLIST_NODE_TYPE_DICT);
  if (PrelinkedInfoRoot == NULL) {
    PrelinkedContextFree (Context);
    return EFI_INVALID_PARAMETER;
  }

  if (Context->IsKernelCollection) {
    //
    // In KC mode last load address is the __LINKEDIT address.
    //
    SegmentEndOffset = Context->LinkEditSegment->FileOffset + Context->LinkEditSegment->FileSize;

    if (MACHO_ALIGN (SegmentEndOffset) != Context->PrelinkedSize) {
      PrelinkedContextFree (Context);
      return EFI_INVALID_PARAMETER;
    }
    
    Context->PrelinkedLastLoadAddress = Context->LinkEditSegment->VirtualAddress + Context->LinkEditSegment->Size;
  }

  //
  // In legacy mode last load address is last kext address.
  //
  PrelinkedInfoRootCount = PlistDictChildren (PrelinkedInfoRoot);
  for (PrelinkedInfoRootIndex = 0; PrelinkedInfoRootIndex < PrelinkedInfoRootCount; ++PrelinkedInfoRootIndex) {
    PrelinkedInfoRootKey = PlistKeyValue (PlistDictChild (PrelinkedInfoRoot, PrelinkedInfoRootIndex, &Context->KextList));
    if (PrelinkedInfoRootKey == NULL) {
      continue;
    }

    if (AsciiStrCmp (PrelinkedInfoRootKey, PRELINK_INFO_DICTIONARY_KEY) == 0) {
      if (PlistNodeCast (Context->KextList, PLIST_NODE_TYPE_ARRAY) != NULL) {
        if (Context->PrelinkedLastLoadAddress == 0) {
          Context->PrelinkedLastLoadAddress = PrelinkedFindLastLoadAddress (Context->KextList);
        }
        if (Context->PrelinkedLastLoadAddress != 0) {
          return EFI_SUCCESS;
        }
      }
      break;
    }
  }

  PrelinkedContextFree (Context);
  return EFI_INVALID_PARAMETER;
}

VOID
PrelinkedContextFree (
  IN OUT  PRELINKED_CONTEXT  *Context
  )
{
  UINT32          Index;
  LIST_ENTRY      *Link;
  PRELINKED_KEXT  *Kext;

  if (Context->PrelinkedInfoDocument != NULL) {
    XmlDocumentFree (Context->PrelinkedInfoDocument);
    Context->PrelinkedInfoDocument = NULL;
  }

  if (Context->PrelinkedInfo != NULL) {
    FreePool (Context->PrelinkedInfo);
    Context->PrelinkedInfo = NULL;
  }

  if (Context->PooledBuffers != NULL) {
    for (Index = 0; Index < Context->PooledBuffersCount; ++Index) {
      FreePool (Context->PooledBuffers[Index]);
    }
    FreePool (Context->PooledBuffers);
    Context->PooledBuffers = NULL;
  }

  if (Context->LinkBuffer != NULL) {
    ZeroMem (Context->LinkBuffer, Context->LinkBufferSize);
    FreePool (Context->LinkBuffer);
    Context->LinkBuffer = NULL;
  }

  while (!IsListEmpty (&Context->PrelinkedKexts)) {
    Link = GetFirstNode (&Context->PrelinkedKexts);
    Kext = GET_PRELINKED_KEXT_FROM_LINK (Link);
    RemoveEntryList (Link);
    InternalFreePrelinkedKext (Kext);
  }

  ZeroMem (&Context->PrelinkedKexts, sizeof (Context->PrelinkedKexts));

  //
  // We do not need to iterate InjectedKexts here, as its memory was freed above.
  //
  ZeroMem (&Context->InjectedKexts, sizeof (Context->InjectedKexts));
}

EFI_STATUS
PrelinkedDependencyInsert (
  IN OUT  PRELINKED_CONTEXT  *Context,
  IN      VOID               *Buffer
  )
{
  VOID   **NewPooledBuffers;

  if (Context->PooledBuffersCount == Context->PooledBuffersAllocCount) {
    NewPooledBuffers = AllocatePool (
      2 * (Context->PooledBuffersAllocCount + 1) * sizeof (NewPooledBuffers[0])
      );
    if (NewPooledBuffers == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    if (Context->PooledBuffers != NULL) {
      CopyMem (
        &NewPooledBuffers[0],
        &Context->PooledBuffers[0],
        Context->PooledBuffersCount * sizeof (NewPooledBuffers[0])
        );
      FreePool (Context->PooledBuffers);
    }
    Context->PooledBuffers           = NewPooledBuffers;
    Context->PooledBuffersAllocCount = 2 * (Context->PooledBuffersAllocCount + 1);
  }

  Context->PooledBuffers[Context->PooledBuffersCount] = Buffer;
  Context->PooledBuffersCount++;

  return EFI_SUCCESS;
}

EFI_STATUS
PrelinkedInjectPrepare (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     UINT32             LinkedExpansion,
  IN     UINT32             ReservedExeSize
  )
{
  EFI_STATUS Status;
  UINT64     SegmentEndOffset;
  UINT32     AlignedExpansion;

  if (Context->IsKernelCollection) {
    //
    // For newer variant (KC mode) __LINKEDIT is last, and we need to expand it to enable
    // dyld fixup generation.
    //
    if (Context->PrelinkedAllocSize < LinkedExpansion
      || Context->PrelinkedAllocSize - LinkedExpansion < Context->PrelinkedSize) {
      return EFI_OUT_OF_RESOURCES;
    }

    ASSERT (Context->PrelinkedLastAddress == Context->PrelinkedLastLoadAddress);

    Context->KextsFixupChains = (VOID *) (Context->Prelinked +
      Context->LinkEditSegment->FileOffset + Context->LinkEditSegment->FileSize);

    AlignedExpansion = MACHO_ALIGN (LinkedExpansion);

    Context->PrelinkedSize              += AlignedExpansion;
    Context->PrelinkedLastAddress       += AlignedExpansion;
    Context->PrelinkedLastLoadAddress   += AlignedExpansion;
    Context->LinkEditSegment->Size      += AlignedExpansion;
    Context->LinkEditSegment->FileSize  += AlignedExpansion;
  } else {
    //
    // For older variant of the prelinkedkernel plist info is normally
    // the last segment, so we may potentially save some data by removing
    // it and then appending new kexts over. This is different for KC,
    // where plist info is in the middle of the file.
    //
    SegmentEndOffset = Context->PrelinkedInfoSegment->FileOffset + Context->PrelinkedInfoSegment->FileSize;

    if (MACHO_ALIGN (SegmentEndOffset) == Context->PrelinkedSize) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: Reducing prelink size from %X to %X via plist\n",
        Context->PrelinkedSize, 
        (UINT32) MACHO_ALIGN (Context->PrelinkedInfoSegment->FileOffset)
        ));
      Context->PrelinkedSize = (UINT32) MACHO_ALIGN (Context->PrelinkedInfoSegment->FileOffset);
    } else {
       DEBUG ((
        DEBUG_INFO,
        "OCAK:Leaving unchanged prelink size %X due to %LX plist\n",
        Context->PrelinkedSize, 
        SegmentEndOffset
        ));
    }

    Context->PrelinkedInfoSegment->VirtualAddress = 0;
    Context->PrelinkedInfoSegment->Size           = 0;
    Context->PrelinkedInfoSegment->FileOffset     = 0;
    Context->PrelinkedInfoSegment->FileSize       = 0;
    Context->PrelinkedInfoSection->Address        = 0;
    Context->PrelinkedInfoSection->Size           = 0;
    Context->PrelinkedInfoSection->Offset         = 0;

    Context->PrelinkedLastAddress = MACHO_ALIGN (MachoGetLastAddress64 (&Context->PrelinkedMachContext));
    if (Context->PrelinkedLastAddress == 0) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Prior to plist there usually is prelinked text. 
    //
    SegmentEndOffset = Context->PrelinkedTextSegment->FileOffset + Context->PrelinkedTextSegment->FileSize;

    if (MACHO_ALIGN (SegmentEndOffset) != Context->PrelinkedSize) {
      //
      // TODO: Implement prelinked text relocation when it is not preceding prelinked info
      // and is not in the end of prelinked info.
      //
      return EFI_UNSUPPORTED;
    }
  }
  //
  // Append the injected KEXTs to the current kernel end.
  //
  Context->KextsFileOffset = Context->PrelinkedSize;
  Context->KextsVmAddress  = Context->PrelinkedLastAddress;

  if (Context->IsKernelCollection) {
    Status = KcInitKextFixupChains (Context, LinkedExpansion, ReservedExeSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrelinkedInjectComplete (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  CHAR8       *ExportedInfo;
  UINT32      ExportedInfoSize;
  UINT32      NewSize;
  UINT32      KextsSize;
  UINT32      ChainSize;

  if (Context->IsKernelCollection) {
    //
    // Fix up the segment fixup chains structure to reflect the actually
    // injected segment size.
    //
    KextsSize = Context->PrelinkedSize - Context->KextsFileOffset;

    ChainSize = KcGetSegmentFixupChainsSize (KextsSize);
    ASSERT (ChainSize != 0);
    ASSERT (ChainSize <= Context->KextsFixupChains->Size);

    Context->KextsFixupChains->Size      = ChainSize;
    Context->KextsFixupChains->PageCount = (UINT16) (KextsSize / MACHO_PAGE_SIZE);

    Status = KcRebuildMachHeader (Context);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  ExportedInfo = XmlDocumentExport (Context->PrelinkedInfoDocument, &ExportedInfoSize, 0, FALSE);
  if (ExportedInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Include \0 terminator.
  //
  ExportedInfoSize++;

  if (OcOverflowAddU32 (Context->PrelinkedSize, MACHO_ALIGN (ExportedInfoSize), &NewSize)
    || NewSize > Context->PrelinkedAllocSize) {
    FreePool (ExportedInfo);
    return EFI_BUFFER_TOO_SMALL;
  }

#if 0
  //
  // This is a potential optimisation for smaller kexts allowing us to use less space.
  // This requires disable __KREMLIN relocation segment addition.
  //
  if (Context->IsKernelCollection && MACHO_ALIGN (ExportedInfoSize) <= Context->PrelinkedInfoSegment->Size) {
    CopyMem (
      &Context->Prelinked[Context->PrelinkedInfoSegment->FileOffset],
      ExportedInfo,
      ExportedInfoSize
      );

    ZeroMem (
      &Context->Prelinked[Context->PrelinkedInfoSegment->FileOffset + ExportedInfoSize],
      Context->PrelinkedInfoSegment->FileSize - ExportedInfoSize
      );

    FreePool (ExportedInfo);

    return EFI_SUCCESS;
  }
#endif

  Context->PrelinkedInfoSegment->VirtualAddress = Context->PrelinkedLastAddress;
  Context->PrelinkedInfoSegment->Size           = MACHO_ALIGN (ExportedInfoSize);
  Context->PrelinkedInfoSegment->FileOffset     = Context->PrelinkedSize;
  Context->PrelinkedInfoSegment->FileSize       = MACHO_ALIGN (ExportedInfoSize);
  Context->PrelinkedInfoSection->Address        = Context->PrelinkedLastAddress;
  Context->PrelinkedInfoSection->Size           = MACHO_ALIGN (ExportedInfoSize);
  Context->PrelinkedInfoSection->Offset         = Context->PrelinkedSize;

  if (Context->IsKernelCollection) {
    //
    // For newer variant of the prelinkedkernel plist we need to adapt it
    // in both inner and outer images.
    //
    Context->InnerInfoSegment->VirtualAddress = Context->PrelinkedLastAddress;
    Context->InnerInfoSegment->Size           = MACHO_ALIGN (ExportedInfoSize);
    Context->InnerInfoSegment->FileOffset     = Context->PrelinkedSize;
    Context->InnerInfoSegment->FileSize       = MACHO_ALIGN (ExportedInfoSize);
    Context->InnerInfoSection->Address        = Context->PrelinkedLastAddress;
    Context->InnerInfoSection->Size           = MACHO_ALIGN (ExportedInfoSize);
    Context->InnerInfoSection->Offset         = Context->PrelinkedSize;
  }

  CopyMem (
    &Context->Prelinked[Context->PrelinkedSize],
    ExportedInfo,
    ExportedInfoSize
    );

  ZeroMem (
    &Context->Prelinked[Context->PrelinkedSize + ExportedInfoSize],
    MACHO_ALIGN (ExportedInfoSize) - ExportedInfoSize
    );

  Context->PrelinkedLastAddress += MACHO_ALIGN (ExportedInfoSize);
  Context->PrelinkedSize        += MACHO_ALIGN (ExportedInfoSize);

  FreePool (ExportedInfo);

  return EFI_SUCCESS;
}

EFI_STATUS
PrelinkedReserveKextSize (
  IN OUT UINT32       *ReservedInfoSize,
  IN OUT UINT32       *ReservedExeSize,
  IN     UINT32       InfoPlistSize,
  IN     UINT8        *Executable,
  IN     UINT32       ExecutableSize OPTIONAL
  )
{
  OC_MACHO_CONTEXT  Context;

  //
  // For new fields.
  //
  if (OcOverflowAddU32 (InfoPlistSize, 512, &InfoPlistSize)) {
    return EFI_INVALID_PARAMETER;
  }

  InfoPlistSize  = MACHO_ALIGN (InfoPlistSize);

  if (Executable != NULL) {
    ASSERT (ExecutableSize > 0);
    if (!MachoInitializeContext (&Context, Executable, ExecutableSize, 0)) {
      return EFI_INVALID_PARAMETER;
    }

    ExecutableSize = MachoGetVmSize64 (&Context);
    if (ExecutableSize == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (OcOverflowAddU32 (*ReservedInfoSize, InfoPlistSize, &InfoPlistSize)
   || OcOverflowAddU32 (*ReservedExeSize, ExecutableSize, &ExecutableSize)) {
    return EFI_INVALID_PARAMETER;
  }

  *ReservedInfoSize = InfoPlistSize;
  *ReservedExeSize  = ExecutableSize;

  return EFI_SUCCESS;
}

EFI_STATUS
PrelinkedInjectKext (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     CONST CHAR8        *BundlePath,
  IN     CONST CHAR8        *InfoPlist,
  IN     UINT32             InfoPlistSize,
  IN     CONST CHAR8        *ExecutablePath OPTIONAL,
  IN     CONST UINT8        *Executable OPTIONAL,
  IN     UINT32             ExecutableSize OPTIONAL
  )
{
  EFI_STATUS        Status;
  BOOLEAN           Result;

  XML_DOCUMENT      *InfoPlistDocument;
  XML_NODE          *InfoPlistRoot;
  CHAR8             *TmpInfoPlist;
  CHAR8             *NewInfoPlist;
  OC_MACHO_CONTEXT  ExecutableContext;
  CONST CHAR8       *TmpKeyValue;
  UINT32            FieldCount;
  UINT32            FieldIndex;
  UINT32            NewInfoPlistSize;
  UINT32            NewPrelinkedSize;
  UINT32            AlignedExecutableSize;
  BOOLEAN           Failed;
  UINT64            KmodAddress;
  PRELINKED_KEXT    *PrelinkedKext;
  CHAR8             ExecutableSourceAddrStr[24];
  CHAR8             ExecutableSizeStr[24];
  CHAR8             ExecutableLoadAddrStr[24];
  CHAR8             KmodInfoStr[24];
  UINT32            KextOffset;

  PrelinkedKext = NULL;

  ASSERT (InfoPlistSize > 0);

  KmodAddress           = 0;
  AlignedExecutableSize = 0;
  KextOffset            = 0;

  //
  // Copy executable to prelinkedkernel.
  //
  if (Executable != NULL) {
    ASSERT (ExecutableSize > 0);
    if (!MachoInitializeContext (&ExecutableContext, (UINT8 *)Executable, ExecutableSize, 0)) {
      DEBUG ((DEBUG_INFO, "OCAK: Injected kext %a/%a is not a supported executable\n", BundlePath, ExecutablePath));
      return EFI_INVALID_PARAMETER;
    }
    //
    // Append the KEXT to the current prelinked end.
    //
    KextOffset = Context->PrelinkedSize;

    ExecutableSize = MachoExpandImage64 (
      &ExecutableContext,
      &Context->Prelinked[KextOffset],
      Context->PrelinkedAllocSize - KextOffset,
      TRUE
      );

    AlignedExecutableSize = MACHO_ALIGN (ExecutableSize);

    if (OcOverflowAddU32 (KextOffset, AlignedExecutableSize, &NewPrelinkedSize)
      || NewPrelinkedSize > Context->PrelinkedAllocSize
      || ExecutableSize == 0) {
      return EFI_BUFFER_TOO_SMALL;
    }

    ZeroMem (
      &Context->Prelinked[KextOffset + ExecutableSize],
      AlignedExecutableSize - ExecutableSize
      );

    if (!MachoInitializeContext (&ExecutableContext, &Context->Prelinked[KextOffset], ExecutableSize, 0)) {
      return EFI_INVALID_PARAMETER;
    }

    Result = PrelinkedFindKmodAddress (&ExecutableContext, Context->PrelinkedLastLoadAddress, ExecutableSize, &KmodAddress);
    if (!Result) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Allocate Info.plist copy for XML_DOCUMENT.
  //
  TmpInfoPlist = AllocateCopyPool (InfoPlistSize, InfoPlist);
  if (TmpInfoPlist == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InfoPlistDocument = XmlDocumentParse (TmpInfoPlist, InfoPlistSize, FALSE);
  if (InfoPlistDocument == NULL) {
    FreePool (TmpInfoPlist);
    return EFI_INVALID_PARAMETER;
  }

  InfoPlistRoot = PlistNodeCast (PlistDocumentRoot (InfoPlistDocument), PLIST_NODE_TYPE_DICT);
  if (InfoPlistRoot == NULL) {
    XmlDocumentFree (InfoPlistDocument);
    FreePool (TmpInfoPlist);
    return EFI_INVALID_PARAMETER;
  }

  //
  // We are not supposed to check for this, it is XNU responsibility, which reliably panics.
  // However, to avoid certain users making this kind of mistake, we still provide some
  // code in debug mode to diagnose it.
  //
  DEBUG_CODE_BEGIN ();
  if (Executable == NULL) {
    FieldCount = PlistDictChildren (InfoPlistRoot);
    for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
      TmpKeyValue = PlistKeyValue (PlistDictChild (InfoPlistRoot, FieldIndex, NULL));
      if (TmpKeyValue == NULL) {
        continue;
      }

      if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_EXECUTABLE_KEY) == 0) {
        DEBUG ((DEBUG_ERROR, "OCAK: Plist-only kext has %a key\n", INFO_BUNDLE_EXECUTABLE_KEY));
        ASSERT (FALSE);
        CpuDeadLoop ();
      }
    }
  }
  DEBUG_CODE_END ();

  Failed = FALSE;
  Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_BUNDLE_PATH_KEY) == NULL;
  Failed |= XmlNodeAppend (InfoPlistRoot, "string", NULL, BundlePath) == NULL;
  if (Executable != NULL) {
    Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_EXECUTABLE_RELATIVE_PATH_KEY) == NULL;
    Failed |= XmlNodeAppend (InfoPlistRoot, "string", NULL, ExecutablePath) == NULL;
    Failed |= !AsciiUint64ToLowerHex (ExecutableSourceAddrStr, sizeof (ExecutableSourceAddrStr), Context->PrelinkedLastAddress);
    Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_EXECUTABLE_SOURCE_ADDR_KEY) == NULL;
    Failed |= XmlNodeAppend (InfoPlistRoot, "integer", PRELINK_INFO_INTEGER_ATTRIBUTES, ExecutableSourceAddrStr) == NULL;
    Failed |= !AsciiUint64ToLowerHex (ExecutableLoadAddrStr, sizeof (ExecutableLoadAddrStr), Context->PrelinkedLastLoadAddress);
    Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_EXECUTABLE_LOAD_ADDR_KEY) == NULL;
    Failed |= XmlNodeAppend (InfoPlistRoot, "integer", PRELINK_INFO_INTEGER_ATTRIBUTES, ExecutableLoadAddrStr) == NULL;
    Failed |= !AsciiUint64ToLowerHex (ExecutableSizeStr, sizeof (ExecutableSizeStr), AlignedExecutableSize);
    Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_EXECUTABLE_SIZE_KEY) == NULL;
    Failed |= XmlNodeAppend (InfoPlistRoot, "integer", PRELINK_INFO_INTEGER_ATTRIBUTES, ExecutableSizeStr) == NULL;
    Failed |= !AsciiUint64ToLowerHex (KmodInfoStr, sizeof (KmodInfoStr), KmodAddress);
    Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_KMOD_INFO_KEY) == NULL;
    Failed |= XmlNodeAppend (InfoPlistRoot, "integer", PRELINK_INFO_INTEGER_ATTRIBUTES, KmodInfoStr) == NULL;  
  }

  if (Failed) {
    XmlDocumentFree (InfoPlistDocument);
    FreePool (TmpInfoPlist);
    return EFI_OUT_OF_RESOURCES;
  }

  if (Executable != NULL) {
    PrelinkedKext = InternalLinkPrelinkedKext (
      Context,
      &ExecutableContext,
      InfoPlistRoot,
      Context->PrelinkedLastLoadAddress,
      KmodAddress
      );

    if (PrelinkedKext == NULL) {
      XmlDocumentFree (InfoPlistDocument);
      FreePool (TmpInfoPlist);
      return EFI_INVALID_PARAMETER;
    }

    //
    // XNU assumes that load size and source size are same, so we should append
    // whatever is bigger to all sizes.
    //
    Context->PrelinkedSize                  += AlignedExecutableSize;
    Context->PrelinkedLastAddress           += AlignedExecutableSize;
    Context->PrelinkedLastLoadAddress       += AlignedExecutableSize;

    if (Context->IsKernelCollection) {
      //
      // For KC, our KEXTs have their own segment - do not mod __PRELINK_INFO.
      // Integrate the KEXT into KC by indexing its fixups and rebasing.
      // Note, we are no longer using ExecutableContext here, as the context
      // ownership was transferred by InternalLinkPrelinkedKext.
      //
      KcKextIndexFixups (Context, &PrelinkedKext->Context.MachContext);
      Status = KcKextApplyFileDelta (&PrelinkedKext->Context.MachContext, KextOffset);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_WARN,
          "Failed to rebase injected kext %a/%a by %u\n",
          BundlePath,
          ExecutablePath,
          KextOffset
          ));
        return Status;
      }
    } else {
      //
      // For legacy prelinkedkernel we append to __PRELINK_TEXT.
      //
      Context->PrelinkedTextSegment->Size     += AlignedExecutableSize;
      Context->PrelinkedTextSegment->FileSize += AlignedExecutableSize;
      Context->PrelinkedTextSection->Size     += AlignedExecutableSize;
    }
  }

  //
  // Strip outer plist & dict.
  //
  NewInfoPlist = XmlDocumentExport (InfoPlistDocument, &NewInfoPlistSize, 2, FALSE);

  XmlDocumentFree (InfoPlistDocument);
  FreePool (TmpInfoPlist);

  if (NewInfoPlist == NULL) {
    if (PrelinkedKext != NULL) {
      InternalFreePrelinkedKext (PrelinkedKext);
    }
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PrelinkedDependencyInsert (Context, NewInfoPlist);
  if (EFI_ERROR (Status)) {
    FreePool (NewInfoPlist);
    if (PrelinkedKext != NULL) {
      InternalFreePrelinkedKext (PrelinkedKext);
    }
    return Status;
  }

  if (XmlNodeAppend (Context->KextList, "dict", NULL, NewInfoPlist) == NULL) {
    if (PrelinkedKext != NULL) {
      InternalFreePrelinkedKext (PrelinkedKext);
    }
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Let other kexts depend on this one.
  //
  if (PrelinkedKext != NULL) {
    InsertTailList (&Context->PrelinkedKexts, &PrelinkedKext->Link);
    //
    // Additionally register this kext in the injected list, as this is required
    // for KernelCollection support.
    //
    InsertTailList (&Context->InjectedKexts, &PrelinkedKext->InjectedLink);
  }

  return EFI_SUCCESS;
}

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
EFI_STATUS
PrelinkedGetSegmentsFromMacho (
  IN   OC_MACHO_CONTEXT           *MachoContext,
  OUT  MACH_SEGMENT_COMMAND_ANY   **PrelinkedInfoSegment,
  OUT  MACH_SECTION_ANY           **PrelinkedInfoSection
  )
{
  *PrelinkedInfoSegment = MachoGetSegmentByName (
    MachoContext,
    PRELINK_INFO_SEGMENT
    );
  if (*PrelinkedInfoSegment == NULL) {
    return EFI_NOT_FOUND;
  }
  if ((MachoContext->Is32Bit ? (*PrelinkedInfoSegment)->Segment32.FileOffset : (*PrelinkedInfoSegment)->Segment64.FileOffset) > MAX_UINT32) {
    return EFI_UNSUPPORTED;
  }

  *PrelinkedInfoSection = MachoGetSectionByName (
    MachoContext,
    *PrelinkedInfoSegment,
    PRELINK_INFO_SECTION
    );
  if (*PrelinkedInfoSection == NULL) {
    return EFI_NOT_FOUND;
  }
  if ((MachoContext->Is32Bit ? (*PrelinkedInfoSection)->Section32.Size : (*PrelinkedInfoSection)->Section64.Size) > MAX_UINT32) {
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
  MACH_HEADER_ANY          *Header;
  MACH_SEGMENT_COMMAND_64  *Segment;
  BOOLEAN                  IsKernelCollection;

  //
  // Detect kernel type.
  //
  Header             = MachoGetMachHeader (Context);
  IsKernelCollection = (Context->Is32Bit ?
    Header->Header32.FileType : Header->Header64.FileType) == MachHeaderFileTypeFileSet;

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

    if (!MachoInitializeContext64 (
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

    if (!MachoInitialiseSymtabsExternal (Context, InnerContext)) {
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
  IN      UINT32             PrelinkedAllocSize,
  IN      BOOLEAN            Is32Bit
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
  Context->Is32Bit            = Is32Bit;

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
  if (!MachoInitializeContext (&Context->PrelinkedMachContext, Prelinked, PrelinkedSize, 0, Context->Is32Bit)) {
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

  Context->PrelinkedLastAddress = MACHO_ALIGN (MachoGetLastAddress (&Context->PrelinkedMachContext));
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

  Context->PrelinkedTextSegment = MachoGetSegmentByName (
    &Context->PrelinkedMachContext,
    PRELINK_TEXT_SEGMENT
    );
  if (Context->PrelinkedTextSegment == NULL) {
    return EFI_NOT_FOUND;
  }

  Context->PrelinkedTextSection = MachoGetSectionByName (
    &Context->PrelinkedMachContext,
    Context->PrelinkedTextSegment,
    PRELINK_TEXT_SECTION
    );
  if (Context->PrelinkedTextSection == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Context->IsKernelCollection) {
    //
    // Additionally process special entries for KC.
    //
    Status = PrelinkedGetSegmentsFromMacho (
      &Context->InnerMachContext,
      (MACH_SEGMENT_COMMAND_ANY **) &Context->InnerInfoSegment,
      (MACH_SECTION_ANY **) &Context->InnerInfoSection
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

    Context->LinkEditSegment = MachoGetSegmentByName (
      &Context->PrelinkedMachContext,
      KC_LINKEDIT_SEGMENT
      );
    if (Context->LinkEditSegment == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  Context->PrelinkedInfo = Context->Is32Bit ?
    AllocateCopyPool (Context->PrelinkedInfoSection->Section32.Size,
      &Context->Prelinked[Context->PrelinkedInfoSection->Section32.Offset]) :
    AllocateCopyPool ((UINTN) Context->PrelinkedInfoSection->Section64.Size,
      &Context->Prelinked[Context->PrelinkedInfoSection->Section64.Offset]);  
  if (Context->PrelinkedInfo == NULL) {
    PrelinkedContextFree (Context);
    return EFI_OUT_OF_RESOURCES;
  }

  Context->PrelinkedInfoDocument = XmlDocumentParse (
    Context->PrelinkedInfo,
    (UINT32) (Context->Is32Bit ?
      Context->PrelinkedInfoSection->Section32.Size : Context->PrelinkedInfoSection->Section64.Size),
    TRUE
    );
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
    SegmentEndOffset = Context->LinkEditSegment->Segment64.FileOffset + Context->LinkEditSegment->Segment64.FileSize;

    if (MACHO_ALIGN (SegmentEndOffset) != Context->PrelinkedSize) {
      PrelinkedContextFree (Context);
      return EFI_INVALID_PARAMETER;
    }
    
    Context->PrelinkedLastLoadAddress = Context->LinkEditSegment->Segment64.VirtualAddress + Context->LinkEditSegment->Segment64.Size;
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

  if (Context->KextScratchBuffer != NULL) {
    FreePool (Context->KextScratchBuffer);
    Context->KextScratchBuffer = NULL;
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

  if (Context->PrelinkedStateKernel != NULL) {
    FreePool (Context->PrelinkedStateKernel);
    Context->PrelinkedStateKernel = NULL;
  }

  if (Context->PrelinkedStateKexts != NULL) {
    FreePool (Context->PrelinkedStateKexts);
    Context->PrelinkedStateKexts = NULL;
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
    //
    // Ensured by PrelinkedContextInit().
    //
    ASSERT (Context->PrelinkedSize % MACHO_PAGE_SIZE == 0);
    STATIC_ASSERT (
      MACHO_PAGE_SIZE % OC_ALIGNOF (MACH_DYLD_CHAINED_STARTS_IN_SEGMENT) == 0,
      "KextsFixupChains may be unaligned"
      );

    Context->KextsFixupChains = (VOID *) (Context->Prelinked + Context->PrelinkedSize);

    AlignedExpansion = MACHO_ALIGN (LinkedExpansion);
    //
    // Zero the expansion to account for padding.
    //
    ZeroMem (
      Context->Prelinked + Context->PrelinkedSize,
      AlignedExpansion
      );

    Context->PrelinkedSize                        += AlignedExpansion;
    Context->PrelinkedLastAddress                 += AlignedExpansion;
    Context->PrelinkedLastLoadAddress             += AlignedExpansion;
    Context->LinkEditSegment->Segment64.Size      += AlignedExpansion;
    Context->LinkEditSegment->Segment64.FileSize  += AlignedExpansion;
  } else {
    //
    // For older variant of the prelinkedkernel plist info is normally
    // the last segment, so we may potentially save some data by removing
    // it and then appending new kexts over. This is different for KC,
    // where plist info is in the middle of the file. For 10.6.8 we will
    // also need to move __PRELINK_STATE, which is just some blob for kextd.
    //
    // 10.6.8
    //
    // ffffff8000200000 - ffffff8000600000 (at 0000000000000000 - 0000000000400000) - __TEXT
    // ffffff8000600000 - ffffff80006da000 (at 0000000000400000 - 000000000046f000) - __DATA
    // ffffff8000106000 - ffffff8000107000 (at 000000000046f000 - 0000000000470000) - __INITGDT
    // ffffff8000100000 - ffffff8000106000 (at 0000000000470000 - 0000000000476000) - __INITPT
    // ffffff80006da000 - ffffff80006db000 (at 0000000000476000 - 0000000000477000) - __DESC
    // ffffff80006db000 - ffffff80006dc000 (at 0000000000477000 - 0000000000478000) - __VECTORS
    // ffffff8000108000 - ffffff800010e000 (at 0000000000478000 - 000000000047d000) - __HIB
    // ffffff8000107000 - ffffff8000108000 (at 000000000047d000 - 000000000047e000) - __SLEEP
    // ffffff80006dc000 - ffffff80006de000 (at 000000000047e000 - 0000000000480000) - __KLD
    // ffffff80006de000 - ffffff80006de000 (at 0000000000480000 - 0000000000480000) - __LAST
    // ffffff8000772000 - ffffff800100a000 (at 0000000000514000 - 0000000000dac000) - __PRELINK_TEXT
    // ffffff800100a000 - ffffff800155c000 (at 0000000000dac000 - 00000000012fe000) - __PRELINK_STATE
    // ffffff800155c000 - ffffff8001612000 (at 00000000012fe000 - 00000000013b4000) - __PRELINK_INFO
    // 0000000000000000 - 0000000000000000 (at 0000000000513018 - 0000000000551269) - __CTF
    // ffffff80006de000 - ffffff8000771018 (at 0000000000480000 - 0000000000513018) - __LINKEDIT
    //
    // 10.15.6
    //
    // ffffff8000200000 - ffffff8000c00000 (at 0000000000000000 - 0000000000a00000) - __TEXT
    // ffffff8000c00000 - ffffff8000e70000 (at 0000000000a00000 - 0000000000c70000) - __DATA
    // ffffff8000e70000 - ffffff8000ea9000 (at 0000000000c70000 - 0000000000ca9000) - __DATA_CONST
    // ffffff8000100000 - ffffff800019e000 (at 0000000000ca9000 - 0000000000d47000) - __HIB
    // ffffff8000ea9000 - ffffff8000eaa000 (at 0000000000d47000 - 0000000000d48000) - __VECTORS
    // ffffff8000eaa000 - ffffff8000ec4000 (at 0000000000d48000 - 0000000000d62000) - __KLD
    // ffffff8000ec4000 - ffffff8000ec5000 (at 0000000000d62000 - 0000000000d63000) - __LAST
    // ffffff8001036000 - ffffff8002e24000 (at 0000000000f48000 - 0000000002d36000) - __PRELINK_TEXT
    // ffffff8002e24000 - ffffff80030d2000 (at 0000000002d36000 - 0000000002fe3389) - __PRELINK_INFO
    // ffffff8000ec5000 - ffffff8000ec5000 (at 0000000000d63000 - 0000000000dd7000) - __CTF
    // ffffff8000ec5000 - ffffff80010358a8 (at 0000000000dd7000 - 0000000000f478a8) - __LINKEDIT
    //
    SegmentEndOffset = Context->Is32Bit ?
      Context->PrelinkedInfoSegment->Segment32.FileOffset + Context->PrelinkedInfoSegment->Segment32.FileSize :
      Context->PrelinkedInfoSegment->Segment64.FileOffset + Context->PrelinkedInfoSegment->Segment64.FileSize;

    if (MACHO_ALIGN (SegmentEndOffset) == Context->PrelinkedSize) {
      DEBUG ((
        DEBUG_INFO,
        "OCAK: Reducing %a-bit prelink size from %X to %X via plist\n",
        Context->Is32Bit ? "32" : "64",
        Context->PrelinkedSize,
        (UINT32) MACHO_ALIGN (Context->Is32Bit ?
          Context->PrelinkedInfoSegment->Segment32.FileOffset : Context->PrelinkedInfoSegment->Segment64.FileOffset
          )
        ));
      Context->PrelinkedSize = (UINT32) MACHO_ALIGN (Context->Is32Bit ?
        Context->PrelinkedInfoSegment->Segment32.FileOffset : Context->PrelinkedInfoSegment->Segment64.FileOffset
        );
    } else {
       DEBUG ((
        DEBUG_INFO,
        "OCAK:Leaving unchanged %a-bit prelink size %X due to %LX plist\n",
        Context->Is32Bit ? "32" : "64",
        Context->PrelinkedSize,
        SegmentEndOffset
        ));
    }

    if (Context->PrelinkedStateSegment != NULL) {
      SegmentEndOffset = Context->Is32Bit ?
        Context->PrelinkedStateSegment->Segment32.FileOffset + Context->PrelinkedStateSegment->Segment32.FileSize :
        Context->PrelinkedStateSegment->Segment64.FileOffset + Context->PrelinkedStateSegment->Segment64.FileSize;

      if (MACHO_ALIGN (SegmentEndOffset) == Context->PrelinkedSize) {
        DEBUG ((
          DEBUG_INFO,
          "OCAK: Reducing %a-bit prelink size from %X to %X via state\n",
          Context->Is32Bit ? "32" : "64",
          Context->PrelinkedSize,
          (UINT32) MACHO_ALIGN (Context->Is32Bit ?
            Context->PrelinkedStateSegment->Segment32.FileOffset : Context->PrelinkedStateSegment->Segment64.FileOffset
            )
          ));
        Context->PrelinkedSize = (UINT32) MACHO_ALIGN (Context->Is32Bit ?
          Context->PrelinkedStateSegment->Segment32.FileOffset : Context->PrelinkedStateSegment->Segment64.FileOffset
          );

        //
        // Need to NULL this, as they are used in address calculations
        // in e.g. MachoGetLastAddress.
        //
        if (Context->Is32Bit) {
          Context->PrelinkedStateSegment->Segment32.VirtualAddress = 0;
          Context->PrelinkedStateSegment->Segment32.Size           = 0;
          Context->PrelinkedStateSegment->Segment32.FileOffset     = 0;
          Context->PrelinkedStateSegment->Segment32.FileSize       = 0;
          Context->PrelinkedStateSectionKernel->Section32.Address  = 0;
          Context->PrelinkedStateSectionKernel->Section32.Size     = 0;
          Context->PrelinkedStateSectionKernel->Section32.Offset   = 0;
          Context->PrelinkedStateSectionKexts->Section32.Address   = 0;
          Context->PrelinkedStateSectionKexts->Section32.Size      = 0;
          Context->PrelinkedStateSectionKexts->Section32.Offset    = 0;
        } else {
          Context->PrelinkedStateSegment->Segment64.VirtualAddress = 0;
          Context->PrelinkedStateSegment->Segment64.Size           = 0;
          Context->PrelinkedStateSegment->Segment64.FileOffset     = 0;
          Context->PrelinkedStateSegment->Segment64.FileSize       = 0;
          Context->PrelinkedStateSectionKernel->Section64.Address  = 0;
          Context->PrelinkedStateSectionKernel->Section64.Size     = 0;
          Context->PrelinkedStateSectionKernel->Section64.Offset   = 0;
          Context->PrelinkedStateSectionKexts->Section64.Address   = 0;
          Context->PrelinkedStateSectionKexts->Section64.Size      = 0;
          Context->PrelinkedStateSectionKexts->Section64.Offset    = 0;
        }
      } else {
         DEBUG ((
          DEBUG_INFO,
          "OCAK:Leaving unchanged %a-bit prelink size %X due to %LX state\n",
          Context->Is32Bit ? "32" : "64",
          Context->PrelinkedSize,
          SegmentEndOffset
          ));
      }
    }

    if (Context->Is32Bit) {
      Context->PrelinkedInfoSegment->Segment32.VirtualAddress = 0;
      Context->PrelinkedInfoSegment->Segment32.Size           = 0;
      Context->PrelinkedInfoSegment->Segment32.FileOffset     = 0;
      Context->PrelinkedInfoSegment->Segment32.FileSize       = 0;
      Context->PrelinkedInfoSection->Section32.Address        = 0;
      Context->PrelinkedInfoSection->Section32.Size           = 0;
      Context->PrelinkedInfoSection->Section32.Offset         = 0;
    } else {
      Context->PrelinkedInfoSegment->Segment64.VirtualAddress = 0;
      Context->PrelinkedInfoSegment->Segment64.Size           = 0;
      Context->PrelinkedInfoSegment->Segment64.FileOffset     = 0;
      Context->PrelinkedInfoSegment->Segment64.FileSize       = 0;
      Context->PrelinkedInfoSection->Section64.Address        = 0;
      Context->PrelinkedInfoSection->Section64.Size           = 0;
      Context->PrelinkedInfoSection->Section64.Offset         = 0;
    }

    Context->PrelinkedLastAddress = MACHO_ALIGN (MachoGetLastAddress (&Context->PrelinkedMachContext));
    if (Context->PrelinkedLastAddress == 0) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Prior to plist there usually is prelinked text. 
    //
    SegmentEndOffset = Context->Is32Bit ?
      Context->PrelinkedTextSegment->Segment32.FileOffset + Context->PrelinkedTextSegment->Segment32.FileSize :
      Context->PrelinkedTextSegment->Segment64.FileOffset + Context->PrelinkedTextSegment->Segment64.FileSize;

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
  } else if (Context->PrelinkedStateSegment != NULL && (Context->Is32Bit ?
    Context->PrelinkedStateSegment->Segment32.VirtualAddress : Context->PrelinkedStateSegment->Segment64.VirtualAddress)
    == 0) {
    Status = InternalKxldStateRebuild (Context);
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

  if (Context->Is32Bit) {
    Context->PrelinkedInfoSegment->Segment32.VirtualAddress = (UINT32) Context->PrelinkedLastAddress;
    Context->PrelinkedInfoSegment->Segment32.Size           = MACHO_ALIGN (ExportedInfoSize);
    Context->PrelinkedInfoSegment->Segment32.FileOffset     = Context->PrelinkedSize;
    Context->PrelinkedInfoSegment->Segment32.FileSize       = MACHO_ALIGN (ExportedInfoSize);
    Context->PrelinkedInfoSection->Section32.Address        = (UINT32) Context->PrelinkedLastAddress;
    Context->PrelinkedInfoSection->Section32.Size           = MACHO_ALIGN (ExportedInfoSize);
    Context->PrelinkedInfoSection->Section32.Offset         = Context->PrelinkedSize;
  } else {
    Context->PrelinkedInfoSegment->Segment64.VirtualAddress = Context->PrelinkedLastAddress;
    Context->PrelinkedInfoSegment->Segment64.Size           = MACHO_ALIGN (ExportedInfoSize);
    Context->PrelinkedInfoSegment->Segment64.FileOffset     = Context->PrelinkedSize;
    Context->PrelinkedInfoSegment->Segment64.FileSize       = MACHO_ALIGN (ExportedInfoSize);
    Context->PrelinkedInfoSection->Section64.Address        = Context->PrelinkedLastAddress;
    Context->PrelinkedInfoSection->Section64.Size           = MACHO_ALIGN (ExportedInfoSize);
    Context->PrelinkedInfoSection->Section64.Offset         = Context->PrelinkedSize;
  }

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

  Context->PrelinkedMachContext.FileSize = Context->PrelinkedSize;

  if (Context->IsKernelCollection) {
    //
    // After rebuilding the KC we may have moved the __LINKEDIT command.
    // This happens when __REGION kexts are squashed.
    // Obtain its address again.
    //
    Context->LinkEditSegment = MachoGetSegmentByName (
      &Context->PrelinkedMachContext,
      KC_LINKEDIT_SEGMENT
      );
  }

  FreePool (ExportedInfo);

  return EFI_SUCCESS;
}

EFI_STATUS
PrelinkedReserveKextSize (
  IN OUT UINT32       *ReservedInfoSize,
  IN OUT UINT32       *ReservedExeSize,
  IN     UINT32       InfoPlistSize,
  IN     UINT8        *Executable OPTIONAL,
  IN     UINT32       ExecutableSize OPTIONAL,
  IN     BOOLEAN      Is32Bit
  )
{
  OC_MACHO_CONTEXT  Context;

  //
  // For new fields.
  //
  if (OcOverflowAddU32 (InfoPlistSize, PLIST_EXPANSION_SIZE, &InfoPlistSize)) {
    return EFI_INVALID_PARAMETER;
  }

  InfoPlistSize  = MACHO_ALIGN (InfoPlistSize);

  if (Executable != NULL) {
    ASSERT (ExecutableSize > 0);
    if (!MachoInitializeContext (&Context, Executable, ExecutableSize, 0, Is32Bit)) {
      return EFI_INVALID_PARAMETER;
    }

    ExecutableSize = MachoGetExpandedImageSize (&Context);
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
  IN     CONST CHAR8        *Identifier OPTIONAL,
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
  UINT64            FileOffset;
  UINT64            LoadAddressOffset;

  PrelinkedKext = NULL;

  ASSERT (Context != NULL);
  ASSERT (BundlePath != NULL);
  ASSERT (InfoPlist != NULL);
  ASSERT (InfoPlistSize > 0);

  KmodAddress           = 0;
  AlignedExecutableSize = 0;
  KextOffset            = 0;

  //
  // If an identifier was passed, ensure it does not already exist.
  //
  if (Identifier != NULL) {
    if (InternalCachedPrelinkedKext (Context, Identifier) != NULL) {
      DEBUG ((DEBUG_INFO, "OCAK: Bundle %a is already present in prelinked\n", Identifier));
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Copy executable to prelinkedkernel.
  //
  if (Executable != NULL) {
    ASSERT (ExecutableSize > 0);
    if (!MachoInitializeContext (&ExecutableContext, (UINT8 *)Executable, ExecutableSize, 0, Context->Is32Bit)) {
      DEBUG ((DEBUG_INFO, "OCAK: Injected kext %a/%a is not a supported executable\n", BundlePath, ExecutablePath));
      return EFI_INVALID_PARAMETER;
    }
    //
    // Append the KEXT to the current prelinked end.
    //
    KextOffset = Context->PrelinkedSize;

    ExecutableSize = MachoExpandImage (
      &ExecutableContext,
      &Context->Prelinked[KextOffset],
      Context->PrelinkedAllocSize - KextOffset,
      TRUE,
      &FileOffset
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

    if (!MachoInitializeContext (&ExecutableContext, &Context->Prelinked[KextOffset], ExecutableSize, 0, Context->Is32Bit)
      || OcOverflowAddU64 (Context->PrelinkedLastLoadAddress, FileOffset, &LoadAddressOffset)) {
      return EFI_INVALID_PARAMETER;
    }

    Result = KextFindKmodAddress (&ExecutableContext, LoadAddressOffset, ExecutableSize, &KmodAddress);
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
      KmodAddress,
      FileOffset
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
      if (Context->Is32Bit) {
        Context->PrelinkedTextSegment->Segment32.Size     += AlignedExecutableSize;
        Context->PrelinkedTextSegment->Segment32.FileSize += AlignedExecutableSize;
        Context->PrelinkedTextSection->Section32.Size     += AlignedExecutableSize;
      } else {
        Context->PrelinkedTextSegment->Segment64.Size     += AlignedExecutableSize;
        Context->PrelinkedTextSegment->Segment64.FileSize += AlignedExecutableSize;
        Context->PrelinkedTextSection->Section64.Size     += AlignedExecutableSize;
      }
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

EFI_STATUS
PrelinkedContextApplyPatch (
  IN OUT PRELINKED_CONTEXT      *Context,
  IN     CONST CHAR8            *Identifier,
  IN     PATCHER_GENERIC_PATCH  *Patch
  )
{
  EFI_STATUS            Status;
  PATCHER_CONTEXT       Patcher;

  ASSERT (Context != NULL);
  ASSERT (Identifier != NULL);
  ASSERT (Patch != NULL);

  Status = PatcherInitContextFromPrelinked (&Patcher, Context, Identifier);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to pk find %a - %r\n", Identifier, Status));
    return Status;
  }

  return PatcherApplyGenericPatch (&Patcher, Patch);
}

EFI_STATUS
PrelinkedContextApplyQuirk (
  IN OUT PRELINKED_CONTEXT    *Context,
  IN     KERNEL_QUIRK_NAME    Quirk,
  IN     UINT32               KernelVersion
  )
{
  EFI_STATUS            Status;
  KERNEL_QUIRK          *KernelQuirk;
  PATCHER_CONTEXT       Patcher;

  ASSERT (Context != NULL);

  KernelQuirk = &gKernelQuirks[Quirk];
  ASSERT (KernelQuirk->Identifier != NULL);

  Status = PatcherInitContextFromPrelinked (&Patcher, Context, KernelQuirk->Identifier);
  if (!EFI_ERROR (Status)) {
    return KernelQuirk->PatchFunction (&Patcher, KernelVersion);
  }

  //
  // It is up to the function to decide whether this is critical or not.
  //
  DEBUG ((DEBUG_INFO, "OCAK: Failed to pk find %a - %r\n", KernelQuirk->Identifier, Status));
  return KernelQuirk->PatchFunction (NULL, KernelVersion);
}

EFI_STATUS
PrelinkedContextBlock (
  IN OUT PRELINKED_CONTEXT      *Context,
  IN     CONST CHAR8            *Identifier
  )
{
  EFI_STATUS            Status;
  PATCHER_CONTEXT       Patcher;

  ASSERT (Context != NULL);
  ASSERT (Identifier != NULL);

  Status = PatcherInitContextFromPrelinked (&Patcher, Context, Identifier);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to pk find %a - %r\n", Identifier, Status));
    return Status;
  }

  return PatcherBlockKext (&Patcher);
}

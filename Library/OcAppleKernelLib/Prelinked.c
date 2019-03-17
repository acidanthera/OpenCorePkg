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

EFI_STATUS
PrelinkedContextInit (
  IN OUT  PRELINKED_CONTEXT  *Context,
  IN OUT  UINT8              *Prelinked,
  IN      UINT32             PrelinkedSize,
  IN      UINT32             PrelinkedAllocSize
  )
{
  XML_NODE                 *PlistInfoRoot;
  XML_NODE                 *PlistInfoRootKey;
  UINT32                   PlistInfoRootIndex;
  UINT32                   PlistInfoRootCount;

  ZeroMem (Context, sizeof (*Context));

  Context->Prelinked          = Prelinked;
  Context->PrelinkedSize      = PRELINKED_ALIGN (PrelinkedSize);
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

  if (!MachoInitializeContext (&Context->PrelinkedMachContext, Prelinked, PrelinkedSize)) {
    return EFI_INVALID_PARAMETER;
  }

  Context->PrelinkedMachHeader = MachoGetMachHeader64 (&Context->PrelinkedMachContext);
  if (Context->PrelinkedMachHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Context->PlistInfoSegment = MachoGetSegmentByName64 (
    &Context->PrelinkedMachContext,
    PRELINK_INFO_SEGMENT
    );
  if (Context->PlistInfoSegment == NULL) {
    return EFI_NOT_FOUND;
  }

  Context->PlistInfoSection = MachoGetSectionByName64 (
    &Context->PrelinkedMachContext,
    Context->PlistInfoSegment,
    PRELINK_INFO_SECTION
    );
  if (Context->PlistInfoSection == NULL) {
    return EFI_NOT_FOUND;
  }

  Context->PlistTextSegment = MachoGetSegmentByName64 (
    &Context->PrelinkedMachContext,
    PRELINK_TEXT_SEGMENT
    );
  if (Context->PlistTextSegment == NULL) {
    return EFI_NOT_FOUND;
  }

  Context->PlistTextSection = MachoGetSectionByName64 (
    &Context->PrelinkedMachContext,
    Context->PlistTextSegment,
    PRELINK_TEXT_SECTION
    );
  if (Context->PlistTextSection == NULL) {
    return EFI_NOT_FOUND;
  }

  Context->PlistInfo = AllocateCopyPool (
    Context->PlistInfoSection->Size,
    &Context->Prelinked[Context->PlistInfoSection->Offset]
    );
  if (Context->PlistInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Context->PlistInfoDocument = XmlDocumentParse (Context->PlistInfo, Context->PlistInfoSection->Size);
  if (Context->PlistInfoDocument == NULL) {
    PrelinkedContextFree (Context);
    return EFI_INVALID_PARAMETER;
  }

  PlistInfoRoot = PlistNodeCast (XmlDocumentRoot (Context->PlistInfoDocument), PLIST_NODE_TYPE_DICT);
  if (PlistInfoRoot == NULL) {
    PrelinkedContextFree (Context);
    return EFI_INVALID_PARAMETER;
  }

  PlistInfoRootCount = PlistDictChildren (PlistInfoRoot);
  for (PlistInfoRootIndex = 0; PlistInfoRootIndex < PlistInfoRootCount; ++PlistInfoRootIndex) {
    PlistInfoRootKey = PlistDictChild (PlistInfoRoot, PlistInfoRootIndex, &Context->KextList);
    if (PlistInfoRootKey == NULL) {
      continue;
    }

    if (AsciiStrCmp (PlistKeyValue (PlistInfoRootKey), PRELINK_INFO_DICTIONARY_KEY) == 0) {
      if (PlistNodeCast (Context->KextList, PLIST_NODE_TYPE_ARRAY) != NULL) {
        return EFI_SUCCESS;
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
  UINT32  Index;

  if (Context->PlistInfoDocument != NULL) {
    XmlDocumentFree (Context->PlistInfoDocument);
    Context->PlistInfoDocument = NULL;
  }

  if (Context->PlistInfo != NULL) {
    FreePool (Context->PlistInfo);
    Context->PlistInfo = NULL;
  }

  if (Context->PooledBuffers != NULL) {
    for (Index = 0; Index < Context->PooledBuffersCount; ++Index) {
      FreePool (Context->PooledBuffers[Index]);
    }
    FreePool (Context->PooledBuffers);
    Context->PooledBuffers = NULL;
  }
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
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  UINT32  SegmentEndOffset;

  //
  // Plist info is normally the last segment, so we may potentially save
  // some data by removing it and then appending new kexts over.
  //

  SegmentEndOffset = Context->PlistInfoSegment->FileOffset + Context->PlistInfoSegment->FileSize;

  if (PRELINKED_ALIGN (SegmentEndOffset) == Context->PrelinkedSize) {
    Context->PrelinkedSize = PRELINKED_ALIGN (Context->PlistInfoSegment->FileOffset);
  }

  Context->PlistInfoSegment->VirtualAddress = 0;
  Context->PlistInfoSegment->Size           = 0;
  Context->PlistInfoSegment->FileOffset     = 0;
  Context->PlistInfoSegment->FileSize       = 0;
  Context->PlistInfoSection->Address        = 0;
  Context->PlistInfoSection->Size           = 0;
  Context->PlistInfoSection->Offset         = 0;

  //
  // Prior to plist there usually is prelinked text. 
  //

  SegmentEndOffset = Context->PlistTextSegment->FileOffset + Context->PlistTextSegment->FileSize;

  if (PRELINKED_ALIGN (SegmentEndOffset) != Context->PrelinkedSize) {
    //
    // TODO: Implement prelinked text relocation when it is not preceding prelinked info
    // and is not in the end of prelinked info.
    //
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrelinkedInjectComplete (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  UINT64      LastAddress;
  CHAR8       *ExportedInfo;
  UINT32      ExportedInfoSize;
  UINT32      NewSize;

  LastAddress = PRELINKED_ALIGN (MachoGetLastAddress64 (&Context->PrelinkedMachContext));
  if (LastAddress == 0) {
    return EFI_INVALID_PARAMETER;
  }

  ExportedInfo = XmlDocumentExport (Context->PlistInfoDocument, &ExportedInfoSize, 0);
  if (ExportedInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Include \0 terminator.
  //
  ExportedInfoSize++;

  if (OcOverflowAddU32 (Context->PrelinkedSize, PRELINKED_ALIGN (ExportedInfoSize), &NewSize)
    || NewSize > Context->PrelinkedAllocSize) {
    FreePool (ExportedInfo);
    return EFI_BUFFER_TOO_SMALL;
  }

  Context->PlistInfoSegment->VirtualAddress = LastAddress;
  Context->PlistInfoSegment->Size           = ExportedInfoSize;
  Context->PlistInfoSegment->FileOffset     = Context->PrelinkedSize;
  Context->PlistInfoSegment->FileSize       = ExportedInfoSize;
  Context->PlistInfoSection->Address        = LastAddress;
  Context->PlistInfoSection->Size           = ExportedInfoSize;
  Context->PlistInfoSection->Offset         = Context->PrelinkedSize;

  CopyMem (
    &Context->Prelinked[Context->PrelinkedSize],
    ExportedInfo,
    ExportedInfoSize
    );

  ZeroMem (
    &Context->Prelinked[Context->PrelinkedSize + ExportedInfoSize],
    PRELINKED_ALIGN (ExportedInfoSize) - ExportedInfoSize
    );

  Context->PrelinkedSize += PRELINKED_ALIGN (ExportedInfoSize);

  FreePool (ExportedInfo);

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
  EFI_STATUS    Status;
  XML_DOCUMENT  *InfoPlistDocument;
  XML_NODE      *InfoPlistRoot;
  CHAR8         *TmpInfoPlist;
  CHAR8         *NewInfoPlist;
  UINT32        NewInfoPlistSize;
  UINT32        NewPrelinkedSize;

  //
  // Copy executable to prelinkedkernel.
  //
  if (Executable != NULL) {
    if (OcOverflowAddU32 (Context->PrelinkedSize, PRELINKED_ALIGN (ExecutableSize), &NewPrelinkedSize)
      || NewPrelinkedSize > Context->PrelinkedAllocSize) {
      return EFI_BUFFER_TOO_SMALL;
    }

    CopyMem (
      &Context->Prelinked[Context->PrelinkedSize],
      Executable,
      ExecutableSize
      );

    //
    // TODO: Implement full kext injection.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate Info.plist copy for XML_DOCUMENT.
  //
  TmpInfoPlist = AllocateCopyPool (InfoPlistSize, InfoPlist);
  if (TmpInfoPlist == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InfoPlistDocument = XmlDocumentParse (TmpInfoPlist, InfoPlistSize);
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

  if (XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_BUNDLE_PATH_KEY) == NULL ||
    XmlNodeAppend (InfoPlistRoot, "string", NULL, BundlePath) == NULL) {
    XmlDocumentFree (InfoPlistDocument);
    FreePool (TmpInfoPlist);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Strip outer plist & dict.
  //
  NewInfoPlist = XmlDocumentExport (InfoPlistDocument, &NewInfoPlistSize, 2);

  XmlDocumentFree (InfoPlistDocument);
  FreePool (TmpInfoPlist);

  if (NewInfoPlist == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PrelinkedDependencyInsert (Context, NewInfoPlist);
  if (EFI_ERROR (Status)) {
    FreePool (NewInfoPlist);
    return Status;
  }

  if (XmlNodeAppend (Context->KextList, "dict", NULL, NewInfoPlist) == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

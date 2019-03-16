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
  if (Context->PlistInfoDocument != NULL) {
    XmlDocumentFree (Context->PlistInfoDocument);
    Context->PlistInfoDocument = NULL;
  }

  if (Context->PlistInfo) {
    FreePool (Context->PlistInfo);
    Context->PlistInfo = NULL;
  }
}

VOID
PrelinkedDropPlistInfo (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  UINT32  PlistEndOffset;

  //
  // Plist info is normally the last segment, so we may potentially save
  // some data by removing it and then appending new kexts over.
  //

  PlistEndOffset = Context->PlistInfoSegment->FileOffset + Context->PlistInfoSegment->FileSize;

  if (PRELINKED_ALIGN (PlistEndOffset) == PRELINKED_ALIGN (Context->PrelinkedSize)) {
    Context->PrelinkedSize = Context->PlistInfoSegment->FileOffset;
  }

  Context->PlistInfoSegment->VirtualAddress = 0;
  Context->PlistInfoSegment->Size           = 0;
  Context->PlistInfoSegment->FileOffset     = 0;
  Context->PlistInfoSegment->FileSize       = 0;
  Context->PlistInfoSection->Address        = 0;
  Context->PlistInfoSection->Size           = 0;
  Context->PlistInfoSection->Offset         = 0;
}

EFI_STATUS
PrelinkedInsertPlistInfo (
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
  IN OUT CHAR8              *InfoPlist,
  IN     UINT32             InfoPlistSize,
     OUT CHAR8              **NewInfoPlist,
  IN     CONST CHAR8        *ExecutablePath OPTIONAL,
  IN OUT UINT8              *Executable OPTIONAL,
  IN     UINT32             ExecutableSize OPTIONAL
  )
{
  XML_DOCUMENT  *InfoPlistDocument;
  XML_NODE      *InfoPlistRoot;
  UINT32        NewInfoPlistSize;

  if (Executable != NULL) {
    //
    // TODO: Implement full kext injection.
    //
    return EFI_UNSUPPORTED;
  }

  InfoPlistDocument = XmlDocumentParse (InfoPlist, InfoPlistSize);
  if (InfoPlistDocument == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InfoPlistRoot = PlistNodeCast (PlistDocumentRoot (InfoPlistDocument), PLIST_NODE_TYPE_DICT);
  if (InfoPlistRoot == NULL) {
    XmlDocumentFree (InfoPlistDocument);
    return EFI_INVALID_PARAMETER;
  }

  if (XmlNodeAppend (InfoPlistRoot, "key", NULL, PRELINK_INFO_BUNDLE_PATH_KEY) == NULL ||
    XmlNodeAppend (InfoPlistRoot, "string", NULL, BundlePath) == NULL) {
    XmlDocumentFree (InfoPlistDocument);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Strip outer plist & dict.
  //
  *NewInfoPlist = XmlDocumentExport (InfoPlistDocument, &NewInfoPlistSize, 2);

  XmlDocumentFree (InfoPlistDocument);

  if (*NewInfoPlist == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (XmlNodeAppend (Context->KextList, "dict", NULL, *NewInfoPlist) == NULL) {
    FreePool (*NewInfoPlist);
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

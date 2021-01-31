/** @file
  Implements APIs to load PE/COFF Debug information.

  Portions copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "PeCoffInternal.h"

VOID
PeCoffLoaderRetrieveCodeViewInfo (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context,
  IN     UINT32                 FileSize
  )
{
  BOOLEAN                               Result;

  CONST EFI_IMAGE_DATA_DIRECTORY        *DebugDir;
  CONST EFI_TE_IMAGE_HEADER             *TeHdr;
  CONST EFI_IMAGE_NT_HEADERS32          *Pe32Hdr;
  CONST EFI_IMAGE_NT_HEADERS64          *Pe32PlusHdr;

  CONST EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *DebugEntries;
  UINT32                                NumDebugEntries;
  UINT32                                DebugIndex;
  CONST EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *CodeViewEntry;

  UINT32                                DebugDirTop;
  UINT32                                DebugDirFileOffset;
  UINT32                                DebugDirSectionOffset;
  UINT32                                DebugDirSectionRawTop;
  UINT32                                DebugEntryTopOffset;
  CONST EFI_IMAGE_SECTION_HEADER        *Sections;
  UINT16                                SectIndex;

  UINT32                                DebugSizeOfImage;

  ASSERT (Context != NULL);
  ASSERT (Context->SizeOfImageDebugAdd == 0);
  ASSERT (Context->CodeViewRva == 0);

  Context->SizeOfImageDebugAdd = 0;
  Context->CodeViewRva         = 0;
  //
  // Retrieve the Debug Directory information of the Image.
  //
  switch (Context->ImageType) { /* LCOV_EXCL_BR_LINE */
    case ImageTypeTe:
      TeHdr = (CONST EFI_TE_IMAGE_HEADER *) (CONST VOID *) (
                (CONST CHAR8 *) Context->FileBuffer
                );

      DebugDir = &TeHdr->DataDirectory[1];
      break;

    case ImageTypePe32:
      Pe32Hdr = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
                  (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                  );

      if (Pe32Hdr->NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
        return;
      }

      DebugDir = &Pe32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG];
      break;

    case ImageTypePe32Plus:
      Pe32PlusHdr = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                      (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                      );

      if (Pe32PlusHdr->NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
        return;
      }

      DebugDir = &Pe32PlusHdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG];
      break;

  /* LCOV_EXCL_START */
    default:
      ASSERT (FALSE);
      UNREACHABLE ();
  }
  /* LCOV_EXCL_STOP */

  Result = BaseOverflowAddU32 (
             DebugDir->VirtualAddress,
             DebugDir->Size,
             &DebugDirTop
             );
  if (Result || DebugDirTop > Context->SizeOfImage) {
    return;
  }
  //
  // Determine the file offset of the debug directory...  This means we walk
  // the sections to find which section contains the RVA of the debug
  // directory
  //
  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->SectionsOffset
               );

  for (SectIndex = 0; SectIndex < Context->NumberOfSections; ++SectIndex) {
    if (DebugDir->VirtualAddress >= Sections[SectIndex].VirtualAddress
     && DebugDirTop <= Sections[SectIndex].VirtualAddress + Sections[SectIndex].VirtualSize) {
       break;
     }
  }

  if (SectIndex == Context->NumberOfSections) {
    return;
  }

  DebugDirSectionOffset = DebugDir->VirtualAddress - Sections[SectIndex].VirtualAddress;
  DebugDirSectionRawTop = DebugDirSectionOffset + DebugDir->Size;
  if (DebugDirSectionRawTop > Sections[SectIndex].SizeOfRawData) {
    return;
  }

  DebugDirFileOffset = (Sections[SectIndex].PointerToRawData - Context->TeStrippedOffset) + DebugDirSectionOffset;

  if (!IS_ALIGNED (DebugDirFileOffset, OC_ALIGNOF (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY))) {
    return;
  }

  DebugEntries = (CONST EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) (CONST VOID *) (
                  (CONST CHAR8 *) Context->FileBuffer + DebugDirFileOffset
                  );

  NumDebugEntries = DebugDir->Size / sizeof (*DebugEntries);

  if (DebugDir->Size % sizeof (*DebugEntries) != 0) {
    return;
  }

  for (DebugIndex = 0; DebugIndex < NumDebugEntries; ++DebugIndex) {
    if (DebugEntries[DebugIndex].Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
      break;
    }
  }

  if (DebugIndex == NumDebugEntries) {
    return;
  }

  CodeViewEntry        = &DebugEntries[DebugIndex];
  Context->CodeViewRva = Sections[SectIndex].VirtualAddress + DebugIndex * sizeof (*DebugEntries);
  ASSERT (Context->CodeViewRva >= Sections[SectIndex].VirtualAddress);
  //
  // If the Image does not load the Debug information into memory on its own,
  // request reserved space for it to force-load it.
  //
  if (CodeViewEntry->RVA == 0) {
    Result = BaseOverflowSubU32 (
              CodeViewEntry->FileOffset,
              Context->TeStrippedOffset,
              &DebugEntryTopOffset
              );
    if (Result) {
      return;
    }

    Result = BaseOverflowAddU32 (
              DebugEntryTopOffset,
              CodeViewEntry->SizeOfData,
              &DebugEntryTopOffset
              );
    if (Result || DebugEntryTopOffset > FileSize) {
      return;
    }

    Result = BaseOverflowAlignUpU32 (
                Context->SizeOfImage,
                OC_ALIGNOF (UINT32),
                &DebugSizeOfImage
                );
    if (Result) {
      return;
    }

    Result = BaseOverflowAddU32 (
                DebugSizeOfImage,
                CodeViewEntry->SizeOfData,
                &DebugSizeOfImage
                );
    if (Result) {
      return;
    }

    Result = BaseOverflowAlignUpU32 (
                DebugSizeOfImage,
                Context->SectionAlignment,
                &DebugSizeOfImage
                );
    if (Result) {
      return;
    }

    Context->SizeOfImageDebugAdd = DebugSizeOfImage - Context->SizeOfImage;
  }
}

VOID
PeCoffLoaderLoadCodeView (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  BOOLEAN                         Result;

  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *CodeViewEntry;
  UINT32                          DebugEntryRvaTop;

  ASSERT (Context != NULL);

  if (Context->CodeViewRva == 0) {
    return;
  }

  CodeViewEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) (VOID *) (
                    (CHAR8 *) Context->ImageBuffer + Context->CodeViewRva
                    );
  //
  // Load the Codeview information if present
  //
  if (CodeViewEntry->RVA != 0) {
    Result = BaseOverflowAddU32 (
               CodeViewEntry->RVA,
               CodeViewEntry->SizeOfData,
               &DebugEntryRvaTop
               );
    if (Result || DebugEntryRvaTop > Context->SizeOfImage
     || !IS_ALIGNED (CodeViewEntry->RVA, OC_ALIGNOF (UINT32))) {
      Context->CodeViewRva = 0;
      return;
    }
  } else {
    if (!PcdGetBool (PcdImageLoaderForceLoadDebug)
     || Context->SizeOfImageDebugAdd == 0) {
      Context->CodeViewRva = 0;
      return;
    }

    CodeViewEntry->RVA = ALIGN_VALUE (Context->SizeOfImage, OC_ALIGNOF (UINT32));

    ASSERT (Context->SizeOfImageDebugAdd >= (CodeViewEntry->RVA - Context->SizeOfImage) + CodeViewEntry->SizeOfData);

    CopyMem (
      (CHAR8 *) Context->ImageBuffer + CodeViewEntry->RVA,
      (CONST CHAR8 *) Context->FileBuffer + (CodeViewEntry->FileOffset - Context->TeStrippedOffset),
      CodeViewEntry->SizeOfData
      );
  }

  if (CodeViewEntry->SizeOfData < sizeof (UINT32)) {
    Context->CodeViewRva = 0;
  }
}

RETURN_STATUS
PeCoffGetPdbPath (
  IN  CONST PE_COFF_IMAGE_CONTEXT  *Context,
  OUT CHAR8                        **PdbPath,
  OUT UINT32                       *PdbPathSize
  )
{
  BOOLEAN                         Result;

  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *CodeViewEntry;
  CONST CHAR8                     *CodeView;
  UINT32                          PdbOffset;

  ASSERT (Context != NULL);
  ASSERT (PdbPath != NULL);
  ASSERT (PdbPathSize != NULL);

  if (Context->CodeViewRva == 0) {
    return RETURN_UNSUPPORTED;
  }

  CodeViewEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) (VOID *) (
                    (CHAR8 *) Context->ImageBuffer + Context->CodeViewRva
                    );

  CodeView = (CONST CHAR8 *) Context->ImageBuffer + CodeViewEntry->RVA;

  switch (READ_ALIGNED_32 (CodeView)) {
    case CODEVIEW_SIGNATURE_NB10:
      PdbOffset = sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);

      STATIC_ASSERT (
        OC_ALIGNOF (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY) <= OC_ALIGNOF (UINT32),
        "The structure may be misalignedd."
        );
      break;

    case CODEVIEW_SIGNATURE_RSDS:
      PdbOffset = sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);

      STATIC_ASSERT (
        OC_ALIGNOF (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY) <= OC_ALIGNOF (UINT32),
        "The structure may be misalignedd."
        );
      break;

    case CODEVIEW_SIGNATURE_MTOC:
      PdbOffset = sizeof (EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY);

      STATIC_ASSERT (
        OC_ALIGNOF (EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY) <= OC_ALIGNOF (UINT32),
        "The structure may be misalignedd."
        );
      break;

    default:
      return RETURN_UNSUPPORTED;
  }

  Result = BaseOverflowSubU32 (
             CodeViewEntry->SizeOfData,
             PdbOffset,
             PdbPathSize
             );
  if (Result) {
    return RETURN_UNSUPPORTED;
  }

  *PdbPath = (CHAR8 *) Context->ImageBuffer + CodeViewEntry->RVA + PdbOffset;
  return RETURN_SUCCESS;
}

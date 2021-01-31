/** @file
  Implements APIs to access SEC directory for PE/COFF Images.

  Portions copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "PeCoffInternal.h"

RETURN_STATUS
PeCoffGetDataDirectoryEntry (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINT32                          FileSize,
  IN  UINT32                          DirectoryEntryIndex,
  OUT CONST EFI_IMAGE_DATA_DIRECTORY  **DirectoryEntry,
  OUT UINT32                          *FileOffset
  )
{
  CONST EFI_IMAGE_SECTION_HEADER        *Sections;
  CONST EFI_IMAGE_NT_HEADERS32          *Pe32Hdr;
  CONST EFI_IMAGE_NT_HEADERS64          *Pe32PlusHdr;
  UINT32                                EntryTop;
  UINT32                                SectionOffset;
  UINT32                                SectionRawTop;
  UINT16                                SectIndex;
  BOOLEAN                               Result;

  switch (Context->ImageType) {
    case ImageTypePe32:
      Pe32Hdr = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
                  (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                  );

      if (Pe32Hdr->NumberOfRvaAndSizes <= DirectoryEntryIndex) {
        return RETURN_UNSUPPORTED;
      }

      *DirectoryEntry = &Pe32Hdr->DataDirectory[DirectoryEntryIndex];
      break;

    case ImageTypePe32Plus:
      Pe32PlusHdr = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                      (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                      );

      if (Pe32PlusHdr->NumberOfRvaAndSizes <= DirectoryEntryIndex) {
        return RETURN_UNSUPPORTED;
      }

      *DirectoryEntry = &Pe32PlusHdr->DataDirectory[DirectoryEntryIndex];
      break;

    default:
      return RETURN_INVALID_PARAMETER;
  }

  Result = BaseOverflowAddU32 (
             (*DirectoryEntry)->VirtualAddress,
             (*DirectoryEntry)->Size,
             &EntryTop
             );
  if (Result || EntryTop > Context->SizeOfImage) {
    return RETURN_INVALID_PARAMETER;
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
    if ((*DirectoryEntry)->VirtualAddress >= Sections[SectIndex].VirtualAddress
     && EntryTop <= Sections[SectIndex].VirtualAddress + Sections[SectIndex].VirtualSize) {
       break;
     }
  }

  if (SectIndex == Context->NumberOfSections
    || DirectoryEntryIndex == EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
    *FileOffset = (*DirectoryEntry)->VirtualAddress;
    return EFI_SUCCESS;
  }

  SectionOffset = (*DirectoryEntry)->VirtualAddress - Sections[SectIndex].VirtualAddress;
  SectionRawTop = SectionOffset + (*DirectoryEntry)->Size;
  if (SectionRawTop > Sections[SectIndex].SizeOfRawData) {
    return EFI_OUT_OF_RESOURCES;
  }

  *FileOffset = (Sections[SectIndex].PointerToRawData - Context->TeStrippedOffset) + SectionOffset;


  return RETURN_SUCCESS;
}

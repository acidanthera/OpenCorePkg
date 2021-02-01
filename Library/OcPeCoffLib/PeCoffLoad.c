/** @file
  Implements APIs to load PE/COFF Images.

  Portions copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "PeCoffInternal.h"

/**
  Loads the Image Sections into the memory space and initialises any padding
  with zeros.

  @param[in]  Context           The context describing the Image. Must have been
                                initialised by PeCoffInitializeContext().
  @param[in]  LoadedHeaderSize  The size, in bytes, of the loaded Image Headers.
  @param[out] Destination       The Image destination memory.
  @param[in]  DestinationSize   The size, in bytes, of Destination.
                                Must be at least
                                Context->SizeOfImage +
                                Context->SizeOfImageDebugAdd. If the Section
                                Alignment exceeds 4 KB, must be at least
                                Context->SizeOfImage +
                                Context->SizeOfImageDebugAdd
                                Context->SectionAlignment.
**/
STATIC
VOID
InternalLoadSections (
  IN  CONST PE_COFF_IMAGE_CONTEXT  *Context,
  IN  UINT32                       LoadedHeaderSize,
  OUT VOID                         *Destination,
  IN  UINT32                       DestinationSize
  )
{
  CONST EFI_IMAGE_SECTION_HEADER *Sections;
  UINT16                         Index;
  UINT32                         DataSize;
  UINT32                         PreviousTopRva;

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->SectionsOffset
               );
  //
  // As the loop zero's the data from the end of the previous section, start
  // with the size of the loaded Image Headers to zero their trailing data.
  //

  PreviousTopRva = LoadedHeaderSize;

  for (Index = 0; Index < Context->NumberOfSections; ++Index) {
    if (Sections[Index].VirtualSize < Sections[Index].SizeOfRawData) {
      DataSize = Sections[Index].VirtualSize;
    } else {
      DataSize = Sections[Index].SizeOfRawData;
    }

    //
    // Zero from the end of the previous Section to the start of this Section.
    //
    ZeroMem ((CHAR8 *) Destination + PreviousTopRva, Sections[Index].VirtualAddress - PreviousTopRva);

    //
    // Load the current Section into memory.
    //
    CopyMem (
      (CHAR8 *) Destination + Sections[Index].VirtualAddress,
      (CONST CHAR8 *) Context->FileBuffer + (Sections[Index].PointerToRawData - Context->TeStrippedOffset),
      DataSize
      );

    PreviousTopRva = Sections[Index].VirtualAddress + DataSize;
  }

  //
  // Zero the trailer after the last Image Section.
  //
  ZeroMem (
    (CHAR8 *) Destination + PreviousTopRva,
    DestinationSize - PreviousTopRva
    );
}

RETURN_STATUS
PeCoffLoadImage (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context,
  OUT    VOID                   *Destination,
  IN     UINT32                 DestinationSize
  )
{
  CHAR8                          *AlignedDest;
  UINT32                         AlignOffset;
  UINT32                         AlignedSize;
  UINT32                         LoadedHeaderSize;
  CONST EFI_IMAGE_SECTION_HEADER *Sections;
  UINTN                          Address;
  UINTN                          AlignedAddress;

  ASSERT (Context != NULL);
  ASSERT (Destination != NULL);
  ASSERT (DestinationSize >= Context->SectionAlignment);

  //
  // Correctly align the Image data in memory.
  //

  if (Context->SectionAlignment <= EFI_PAGE_SIZE) {
    //
    // The caller is required to allocate page memory, hence we have at least
    // 4 KB alignment guaranteed.
    //
    AlignedDest = Destination;
    AlignedSize = DestinationSize;
    AlignOffset = 0;
  } else {
    Address = PTR_TO_ADDR (Destination, DestinationSize);
    AlignedAddress = ALIGN_VALUE (Address, (UINTN) Context->SectionAlignment);
    AlignOffset = (UINT32) (AlignedAddress - Address);
    AlignedSize = DestinationSize - AlignOffset;
    ASSERT (Context->SizeOfImage <= AlignedSize);
    AlignedDest = (CHAR8 *) Destination + AlignOffset;
    ZeroMem (Destination, AlignOffset);
  }

  ASSERT (AlignedSize >= Context->SizeOfImage);

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->SectionsOffset
               );
  //
  // If configured, load the Image Headers into the memory space.
  //

  if (Sections[0].VirtualAddress != 0 && PcdGetBool (PcdImageLoaderLoadHeader)) {
    LoadedHeaderSize = (Context->SizeOfHeaders - Context->TeStrippedOffset);
    CopyMem (AlignedDest, Context->FileBuffer, LoadedHeaderSize);
  } else {
    LoadedHeaderSize = 0;
  }

  InternalLoadSections (
    Context,
    LoadedHeaderSize,
    AlignedDest,
    AlignedSize
    );

  Context->ImageBuffer = AlignedDest;

  //
  // If debugging is enabled, force-load its contents into the memory space.
  //

  if (PcdGetBool (PcdImageLoaderSupportDebug)) {
    PeCoffLoaderLoadCodeView (Context);
  }

  return RETURN_SUCCESS;
}

//
// TODO: Provide a Runtime version of this API as well.
//
VOID
PeCoffDiscardSections (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  CONST EFI_IMAGE_SECTION_HEADER *Sections;
  UINT32                         SectIndex;
  BOOLEAN                        Discardable;

  ASSERT (Context != NULL);

  //
  // By the PE/COFF specification, the .reloc section is supposed to be
  // discardable, so we must assume it is no longer valid.
  //

  Context->RelocDirRva = 0;
  Context->RelocDirSize = 0;

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->SectionsOffset
               );
  //
  // Discard all Image Sections that are flagged as optional.
  //
  for (SectIndex = 0; SectIndex < Context->NumberOfSections; ++SectIndex) {
    Discardable = (Sections[SectIndex].Characteristics & EFI_IMAGE_SCN_MEM_DISCARDABLE) != 0;

    if (Discardable) {
      ZeroMem (
        (CHAR8 *) Context->ImageBuffer + Sections[SectIndex].VirtualAddress,
        Sections[SectIndex].VirtualSize
        );
    }
  }
}

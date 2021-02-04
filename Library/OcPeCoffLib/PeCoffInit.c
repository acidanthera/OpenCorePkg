/** @file
  Implements APIs to verify PE/COFF Images for further processing.

  Portions copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

/** @file
  Base PE/COFF loader supports loading any PE32/PE32+ or TE image, but
  only supports relocating IA32, x64, IPF, and EBC images.

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  PeCoffInitializeContext() routine will do basic check for whole PE/COFF image.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeCoffInternal.h"

//
// TODO: Provide an API to destruct the context.
//


/**
  Verify the Image Section Headers.

  The first section must be the beginning of the virtual address space or be
  contiguous to the aligned Image Headers.
  Sections must be disjunct and, in strict mode, contiguous in virtual space.
  Section data must be in file bounds.

  @param[in]  Context       The context describing the Image. Must have been
                            initialised by PeCoffInitializeContext().
  @param[in]  FileSize      The size, in bytes, of Context->FileBuffer.
  @param[out] StartAddress  On output, the RVA of the first Image Section.
  @param[out] EndAddress    On output, the size of the virtual address space.

  @retval RETURN_SUCCESS  The Image Section Headers are correct.
  @retval other           The Image section Headers are malformed.
**/

STATIC
RETURN_STATUS
InternalVerifySections (
  IN  CONST PE_COFF_IMAGE_CONTEXT  *Context,
  IN  UINT32                       FileSize,
  OUT UINT32                       *StartAddress,
  OUT UINT32                       *EndAddress
  )
{
  BOOLEAN                        Result;
  UINT32                         NextSectRva;
  UINT32                         SectRawEnd;
  UINT16                         SectIndex;
  CONST EFI_IMAGE_SECTION_HEADER *Sections;

  ASSERT (Context != NULL);
  ASSERT (Context->SizeOfHeaders >= Context->TeStrippedOffset);
  ASSERT (IS_POW2 (Context->SectionAlignment));
  ASSERT (Context->NumberOfSections > 0);
  ASSERT (StartAddress != NULL);
  ASSERT (EndAddress != NULL);


  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->SectionsOffset
               );
  //
  // The first Image Section must begin the Image memory space, or it must be
  // adjacent to the Image Headers.
  //

  if (Sections[0].VirtualAddress == 0) {
    if (Context->ImageType == ImageTypeTe) {
      return RETURN_UNSUPPORTED;
    }

    NextSectRva = 0;
  } else {
    if (!PcdGetBool (PcdImageLoaderTolerantLoad)) {
      Result = BaseOverflowAlignUpU32 (
                Context->SizeOfHeaders,
                Context->SectionAlignment,
                &NextSectRva
                );
      if (Result) {
        return RETURN_UNSUPPORTED;
      }
    } else {
      NextSectRva = Context->SizeOfHeaders;
    }
  }

  *StartAddress = NextSectRva;
  //
  // Ensure all Image Sections are valid.
  //
  for (SectIndex = 0; SectIndex < Context->NumberOfSections; ++SectIndex) {
    //
    // Ensure the Image Section are disjunct (relaxed) or adjacent (strict).
    //

    if (!PcdGetBool (PcdImageLoaderTolerantLoad) && Sections[SectIndex].VirtualAddress != NextSectRva) {
      return RETURN_UNSUPPORTED;
    }

    if (PcdGetBool (PcdImageLoaderTolerantLoad) && Sections[SectIndex].VirtualAddress < NextSectRva) {
      return RETURN_UNSUPPORTED;
    }

    //
    // Ensure Image Sections with data are in bounds.
    //
    if (Sections[SectIndex].SizeOfRawData > 0) {
      if (Context->TeStrippedOffset > Sections[SectIndex].PointerToRawData) {
        return RETURN_UNSUPPORTED;
      }

      Result = BaseOverflowAddU32 (
                 Sections[SectIndex].PointerToRawData,
                 Sections[SectIndex].SizeOfRawData,
                 &SectRawEnd
                 );

      if (Result) {
        return RETURN_UNSUPPORTED;
      }


      if ((SectRawEnd - Context->TeStrippedOffset) > FileSize) {
        return RETURN_UNSUPPORTED;
      }
    }
    //
    // Determine the end of the current Image Section.
    //

    Result = BaseOverflowAddU32 (
              Sections[SectIndex].VirtualAddress,
              Sections[SectIndex].VirtualSize,
              &NextSectRva
              );
    if (Result) {
      return RETURN_UNSUPPORTED;
    }
    //
    // SectionSize does not need to be aligned, so align the result.
    //

    if (!PcdGetBool (PcdImageLoaderTolerantLoad)) {
      Result = BaseOverflowAlignUpU32 (
                NextSectRva,
                Context->SectionAlignment,
                &NextSectRva
                );
      if (Result) {
        return RETURN_UNSUPPORTED;
      }
    }
  }

  *EndAddress = NextSectRva;

  return RETURN_SUCCESS;
}

/**
  Verify the basic Image Relocation information.

  The preferred Image load address must be aligned by the Section Alignment.
  The Relocation Directory must be contained within the Image Section memory.
  The Relocation Directory must be correctly aligned in memory.

  @param[in]  Context       The context describing the Image. Must have been
                            initialised by PeCoffInitializeContext().
  @param[out] StartAddress  The RVA of the first Image Section.

  @retval RETURN_SUCCESS  The basic Image Relocation information is correct.
  @retval other           The basic Image Relocation information is malformed.
**/

STATIC
RETURN_STATUS
InternalValidateRelocInfo (
  IN CONST PE_COFF_IMAGE_CONTEXT  *Context,
  IN UINT32                       StartAddress
  )
{
  BOOLEAN Result;
  UINT32  SectRvaEnd;

  ASSERT (Context != NULL);
  //
  // If the Base Relocations have not been stripped, verify their Directory.
  //

  if (!Context->RelocsStripped) {
    //
    // Ensure the Relocation Directory is not empty.
    //

    if (sizeof (EFI_IMAGE_BASE_RELOCATION_BLOCK) > Context->RelocDirSize) {
      return RETURN_UNSUPPORTED;
    }

    Result = BaseOverflowAddU32 (
               Context->RelocDirRva,
               Context->RelocDirSize,
               &SectRvaEnd
               );

    if (Result) {
      return RETURN_UNSUPPORTED;
    }

    //
    // Ensure the Relocation Directory does not overlap with the Image Header.
    //

    if (StartAddress > Context->RelocDirRva) {
      return RETURN_UNSUPPORTED;
    }

    //
    // Ensure the Relocation Directory is contained in the Image memory space.
    //

    if (SectRvaEnd > Context->SizeOfImage) {
      return RETURN_UNSUPPORTED;
    }

    //
    // Ensure the Relocation Directory start is correctly aligned.
    //

    if (!IS_ALIGNED (Context->RelocDirRva, OC_ALIGNOF (EFI_IMAGE_BASE_RELOCATION_BLOCK))) {
      return RETURN_UNSUPPORTED;
    }
  }

  //
  // Ensure the preferred load address is correctly aligned.
  //

  if (!IS_ALIGNED (Context->ImageBase, (UINT64) Context->SectionAlignment)) {
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Verify the TE Image and initialise Context.

  Used offsets and ranges must be aligned and in the bounds of the raw file.
  Image Section Headers and basic Relocation information must be correct.

  @param[in,out] Context   The context describing the Image. Must have been
                           initialised by PeCoffInitializeContext().
  @param[in]     FileSize  The size, in bytes, of Context->FileBuffer.

  @retval RETURN_SUCCESS  The TE Image is correct.
  @retval other           The TE Image is malformed.
**/
STATIC
RETURN_STATUS
InternalInitializeTe (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context,
  IN     UINT32                 FileSize
  )
{
  RETURN_STATUS             Status;
  BOOLEAN                   Result;
  CONST EFI_TE_IMAGE_HEADER *TeHdr;
  UINT32                    StartAddress;
  UINT32                    SizeOfImage;

  ASSERT (Context != NULL);
  ASSERT (sizeof (EFI_TE_IMAGE_HEADER) <= FileSize);
  ASSERT (FileSize >= sizeof (EFI_TE_IMAGE_HEADER));

  Context->ImageType = ImageTypeTe;


  TeHdr = (CONST EFI_TE_IMAGE_HEADER *) (CONST VOID *) (
            (CONST CHAR8 *) Context->FileBuffer + 0
            );

  Result = BaseOverflowSubU16 (
             TeHdr->StrippedSize,
             sizeof (*TeHdr),
             &Context->TeStrippedOffset
             );

  if (Result) {
    return RETURN_UNSUPPORTED;
  }

  if (TeHdr->NumberOfSections == 0) {
    return RETURN_UNSUPPORTED;
  }

  STATIC_ASSERT (
    IS_ALIGNED (sizeof (*TeHdr), OC_ALIGNOF (EFI_IMAGE_SECTION_HEADER)),
    "The section alignment requirements are violated."
    );
  //
  // Assign SizeOfHeaders in a way that is equivalent to what the size would
  // be if this was the original (unstripped) PE32 binary. As the TE image
  // creation fixes no fields up, tests work the same way as for PE32.
  // when referencing raw data however, the offset must be subracted.
  //
  STATIC_ASSERT (
    MAX_UINT8 * sizeof (EFI_IMAGE_SECTION_HEADER) <= MAX_UINT32 - MAX_UINT16,
    "The following arithmetic may overflow."
    );

  Context->SizeOfHeaders = (UINT32) TeHdr->StrippedSize + (UINT32) TeHdr->NumberOfSections * sizeof (EFI_IMAGE_SECTION_HEADER);
  //
  // Ensure that all headers are in bounds of the file buffer.
  //

  if ((Context->SizeOfHeaders - Context->TeStrippedOffset) > FileSize) {
    return RETURN_UNSUPPORTED;
  }

  Context->SectionsOffset = sizeof (EFI_TE_IMAGE_HEADER);
  Context->SectionAlignment = BASE_4KB;
  Context->NumberOfSections = TeHdr->NumberOfSections;
  //
  // Validate the sections.
  // TE images do not have a field to explicitly describe the image size.
  // Set it to the top of the image's virtual space.
  //

  Status = InternalVerifySections (
             Context,
             FileSize,
             &StartAddress,
             &SizeOfImage
             );

  if (Status != RETURN_SUCCESS) {
    return Status;
  }


  if (TeHdr->AddressOfEntryPoint >= SizeOfImage) {
    return RETURN_UNSUPPORTED;
  }

  Context->SizeOfImage         = SizeOfImage;
  Context->Machine             = TeHdr->Machine;
  Context->Subsystem           = TeHdr->Subsystem;
  Context->ImageBase           = TeHdr->ImageBase;
  Context->RelocsStripped      = TeHdr->DataDirectory[0].Size > 0;
  Context->AddressOfEntryPoint = TeHdr->AddressOfEntryPoint;
  Context->RelocDirRva         = TeHdr->DataDirectory[0].VirtualAddress;
  Context->RelocDirSize        = TeHdr->DataDirectory[0].Size;

  Status = InternalValidateRelocInfo (Context, StartAddress);

  return Status;
}

/**
  Verify the PE32 or PE32+ Image and initialise Context.

  Used offsets and ranges must be aligned and in the bounds of the raw file.
  Image Section Headers and basic Relocation information must be correct.

  @param[in,out] Context   The context describing the Image. Must have been
                           initialised by PeCoffInitializeContext().
  @param[in]     FileSize  The size, in bytes, of Context->FileBuffer.

  @retval RETURN_SUCCESS  The PE Image is correct.
  @retval other           The PE Image is malformed.
**/
STATIC
RETURN_STATUS
InternalInitializePe (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context,
  IN     UINT32                 FileSize
  )
{
  BOOLEAN                               Result;
  CONST EFI_IMAGE_NT_HEADERS_COMMON_HDR *PeCommon;
  CONST EFI_IMAGE_NT_HEADERS32          *Pe32;
  CONST EFI_IMAGE_NT_HEADERS64          *Pe32Plus;
  CONST CHAR8                           *OptHdrPtr;
  UINT32                                HdrSizeWithoutDataDir;
  UINT32                                MinSizeOfOptionalHeader;
  UINT32                                MinSizeOfHeaders;
  CONST EFI_IMAGE_DATA_DIRECTORY        *RelocDir;
  UINT32                                NumberOfRvaAndSizes;
  RETURN_STATUS                          Status;
  UINT32                                StartAddress;
  UINT32                                MinSizeOfImage;

  ASSERT (Context != NULL);
  ASSERT (IS_ALIGNED (Context->ExeHdrOffset, OC_ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR)));

  OptHdrPtr = (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset;
  OptHdrPtr += sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);

  STATIC_ASSERT (
    IS_ALIGNED (OC_ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR), OC_ALIGNOF (UINT16))
   && IS_ALIGNED (sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR), OC_ALIGNOF (UINT16)),
    "The following operation might be an unaligned access."
  );
  //
  // Determine the type of and retrieve data from the PE Optional Header.
  //

  switch (READ_ALIGNED_16 (OptHdrPtr)) {
    case EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC:

      if (sizeof (*Pe32) > FileSize - Context->ExeHdrOffset) {
        return RETURN_UNSUPPORTED;
      }
      //
      // INTEGRATION: Condition superfluous, see STATIC_ASSERT below.
      // BUG: AV crash...?!
      //

      STATIC_ASSERT (
        OC_ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR) == OC_ALIGNOF (EFI_IMAGE_NT_HEADERS32),
        "The following operations may be unaligned."
        );

      Context->ImageType = ImageTypePe32;

      Pe32 = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
               );


      Context->Subsystem    = Pe32->Subsystem;
      RelocDir = Pe32->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC;
      Context->SizeOfImage         = Pe32->SizeOfImage;
      Context->SizeOfHeaders       = Pe32->SizeOfHeaders;
      Context->ImageBase           = Pe32->ImageBase;
      Context->AddressOfEntryPoint = Pe32->AddressOfEntryPoint;
      Context->SectionAlignment    = Pe32->SectionAlignment;
      PeCommon = &Pe32->CommonHeader;
      NumberOfRvaAndSizes   = Pe32->NumberOfRvaAndSizes;
      HdrSizeWithoutDataDir = sizeof (EFI_IMAGE_NT_HEADERS32) - sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);
      break;

    case EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC:

      if (sizeof (*Pe32Plus) > FileSize - Context->ExeHdrOffset) {
        return RETURN_UNSUPPORTED;
      }
      //
      // BUG: AV crash...?!
      //

      if (!IS_ALIGNED (Context->ExeHdrOffset, OC_ALIGNOF (EFI_IMAGE_NT_HEADERS64))) {
        return RETURN_UNSUPPORTED;
      }


      Context->ImageType = ImageTypePe32Plus;

      Pe32Plus = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                   (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                   );


      Context->Subsystem           = Pe32Plus->Subsystem;
      RelocDir = Pe32Plus->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC;
      Context->SizeOfImage         = Pe32Plus->SizeOfImage;
      Context->SizeOfHeaders       = Pe32Plus->SizeOfHeaders;
      Context->ImageBase           = Pe32Plus->ImageBase;
      Context->AddressOfEntryPoint = Pe32Plus->AddressOfEntryPoint;
      Context->SectionAlignment    = Pe32Plus->SectionAlignment;
      PeCommon = &Pe32Plus->CommonHeader;
      NumberOfRvaAndSizes   = Pe32Plus->NumberOfRvaAndSizes;
      HdrSizeWithoutDataDir = sizeof (EFI_IMAGE_NT_HEADERS64) - sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);
      break;

    default:
      // assert !valid_pe ((char *) Context->FileBuffer, Context->ExeHdrOffset, FileSize);
      return RETURN_UNSUPPORTED;
  }
  //
  // Do not load images with unknown directories.
  //

  if (NumberOfRvaAndSizes > EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES) {
    return RETURN_UNSUPPORTED;
  }

  if (PeCommon->FileHeader.NumberOfSections == 0) {
    return RETURN_UNSUPPORTED;
  }


  if (Context->AddressOfEntryPoint >= Context->SizeOfImage) {
    return RETURN_UNSUPPORTED;
  }


  if (!IS_POW2 (Context->SectionAlignment)) {
    return RETURN_UNSUPPORTED;
  }

  STATIC_ASSERT (
    sizeof (EFI_IMAGE_DATA_DIRECTORY) <= MAX_UINT32 / EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES,
    "The following arithmetic may overflow."
    );
  //
  // Context->ExeHdrOffset + sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) cannot overflow because
  //   * ExeFileSize > sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) and
  //   * Context->ExeHdrOffset + ExeFileSize = FileSize
  //

  Result = BaseOverflowAddU32 (
             Context->ExeHdrOffset + sizeof (*PeCommon),
             PeCommon->FileHeader.SizeOfOptionalHeader,
             &Context->SectionsOffset
             );
  if (Result) {
    return RETURN_UNSUPPORTED;
  }
  //
  // Ensure the section headers offset is properly aligned.
  //

  if (!IS_ALIGNED (Context->SectionsOffset, OC_ALIGNOF (EFI_IMAGE_SECTION_HEADER))) {
    return RETURN_UNSUPPORTED;
  }

  //
  // MinSizeOfOptionalHeader cannot overflow because NumberOfRvaAndSizes has
  // been verified and the other two components are validated constants.
  //

  MinSizeOfOptionalHeader = HdrSizeWithoutDataDir +
                              NumberOfRvaAndSizes * sizeof (EFI_IMAGE_DATA_DIRECTORY);

  ASSERT (MinSizeOfOptionalHeader >= HdrSizeWithoutDataDir);

  STATIC_ASSERT (
    sizeof (EFI_IMAGE_SECTION_HEADER) <= (MAX_UINT32 + 1ULL) / (MAX_UINT16 + 1ULL),
    "The following arithmetic may overflow."
    );

  Result = BaseOverflowAddU32 (
             Context->SectionsOffset,
             (UINT32) PeCommon->FileHeader.NumberOfSections * sizeof (EFI_IMAGE_SECTION_HEADER),
             &MinSizeOfHeaders
             );


  if (Result) {
    return RETURN_UNSUPPORTED;
  }
  //
  // Ensure the header sizes are sane. SizeOfHeaders contains all header
  // components (DOS, PE Common and Optional Header).
  //

  if (MinSizeOfOptionalHeader > PeCommon->FileHeader.SizeOfOptionalHeader) {
    return RETURN_UNSUPPORTED;
  }


  if (MinSizeOfHeaders > Context->SizeOfHeaders) {
    return RETURN_UNSUPPORTED;
  }
  //
  // Ensure that all headers are in bounds of the file buffer.
  //

  if (Context->SizeOfHeaders > FileSize) {
    return RETURN_UNSUPPORTED;
  }

  Context->NumberOfSections = PeCommon->FileHeader.NumberOfSections;
  //
  // If there's no relocations, then make sure it's not a runtime driver.
  //

  Context->RelocsStripped =
    (
      PeCommon->FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED
    ) != 0;

  Context->Machine = PeCommon->FileHeader.Machine;

  ASSERT (Context->TeStrippedOffset == 0);

  if (EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC < NumberOfRvaAndSizes) {
    Context->RelocDirRva  = RelocDir->VirtualAddress;
    Context->RelocDirSize = RelocDir->Size;
  } else {
    ASSERT (Context->RelocDirRva == 0 && Context->RelocDirSize == 0);
  }

  Status = InternalVerifySections (
             Context,
             FileSize,
             &StartAddress,
             &MinSizeOfImage
             );

  if (Status != RETURN_SUCCESS) {
    return Status;
  }


  //
  // Ensure SizeOfImage is equal to the top of the image's virtual space.
  //
  if (MinSizeOfImage > Context->SizeOfImage) {
    return RETURN_UNSUPPORTED;
  }

  Status = InternalValidateRelocInfo (Context, StartAddress);

  return Status;
}

RETURN_STATUS
PeCoffInitializeContext (
  OUT PE_COFF_IMAGE_CONTEXT  *Context,
  IN  CONST VOID             *FileBuffer,
  IN  UINT32                 FileSize
  )
{
  RETURN_STATUS               Status;
  CONST EFI_IMAGE_DOS_HEADER *DosHdr;

  ASSERT (Context != NULL);
  ASSERT (FileBuffer != NULL || FileSize == 0);

  ZeroMem (Context, sizeof (*Context));

  Context->FileBuffer = FileBuffer;

  //
  // Check whether the DOS Image Header is present.
  //
  if (FileSize >= sizeof (*DosHdr)
   && READ_ALIGNED_16 (FileBuffer) == EFI_IMAGE_DOS_SIGNATURE) {
    DosHdr = (CONST EFI_IMAGE_DOS_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) FileBuffer + 0
               );
    //
    // When the DOS Image Header is present, verify the offset and
    // retrieve the size of the executable image.
    //
    if (sizeof (EFI_IMAGE_DOS_HEADER) > DosHdr->e_lfanew
     || DosHdr->e_lfanew > FileSize) {
      return RETURN_UNSUPPORTED;
    }

    Context->ExeHdrOffset = DosHdr->e_lfanew;
  } else {
    //
    // If the DOS Image Header is not present, assume the Image starts with the
    // Executable Header.
    //
    if (FileSize >= sizeof (EFI_TE_IMAGE_HEADER)
     && READ_ALIGNED_16 (FileBuffer) == EFI_TE_IMAGE_HEADER_SIGNATURE) {
      Status = InternalInitializeTe (Context, FileSize);
      if (PcdGetBool (PcdImageLoaderSupportDebug) && Status == RETURN_SUCCESS) {
        PeCoffLoaderRetrieveCodeViewInfo (Context, FileSize);
      }

      return Status;
    }
  }
  //
  // Use Signature to determine and handle the image format (PE32(+) / TE).
  //

  if (FileSize - Context->ExeHdrOffset < sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) + sizeof (UINT16)) {
    return RETURN_UNSUPPORTED;
  }

  if (!IS_ALIGNED (Context->ExeHdrOffset, OC_ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR))) {
    return RETURN_UNSUPPORTED;
  }

  STATIC_ASSERT (
    OC_ALIGNOF (UINT32) <= OC_ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR),
    "The following access may be performed unaligned"
    );

  //
  // Ensure the Image Executable Header has a PE signature.
  //

  if (READ_ALIGNED_32 ((CONST CHAR8 *) FileBuffer + Context->ExeHdrOffset) != EFI_IMAGE_NT_SIGNATURE) {
    return RETURN_UNSUPPORTED;
  }

  Status = InternalInitializePe (Context, FileSize);
  //
  // If debugging is enabled, retrieve information on the debug data.
  //

  if (PcdGetBool (PcdImageLoaderSupportDebug) && Status == RETURN_SUCCESS) {
    PeCoffLoaderRetrieveCodeViewInfo (Context, FileSize);
  }

  return Status;
}

/** @file
  Base PE/COFF loader supports loading any PE32/PE32+ or TE image.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

#include <IndustryStandard/OcPeImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcPeCoffLib.h>
#include <Library/PcdLib.h>

#define IS_POW2(v)        ((v) != 0U && ((v) & ((v) - 1U)) == 0U)
#define IS_ALIGNED(v, a)  (((v) & ((a) - 1U)) == 0U)

#define IMAGE_RELOC_TYPE(Relocation)    ((Relocation) >> 12U)
#define IMAGE_RELOC_OFFSET(Relocation)  ((Relocation) & 0x0FFFU)

#define IMAGE_IS_EFI_SUBYSYSTEM(Subsystem) \
  ((Subsystem) >= EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION && \
   (Subsystem) <= EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER)

#define IMAGE_RELOC_TYPE_SUPPORTED(Type) \
  ((Type) == EFI_IMAGE_REL_BASED_ABSOLUTE) || \
  ((Type) == EFI_IMAGE_REL_BASED_HIGHLOW) || \
  ((Type) == EFI_IMAGE_REL_BASED_DIR64))

#define IMAGE_RELOC_SUPPORTED(Reloc) \
  IMAGE_RELOC_TYPE_SUPPORTED (IMAGE_RELOC_TYPE (Reloc))

STATIC
IMAGE_STATUS
InternalVerifySections (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  UINTN                               FileSize,
  IN  UINT32                              SectionsOffset,
  IN  UINT16                              NumberOfSections,
  OUT UINT32                              *BottomAddress,
  OUT UINT32                              *TopAddress
  )
{
  BOOLEAN                        Result;
  UINT32                         NextSectRva;
  UINT32                         SectRvaPrevEnd;
  UINT32                         SectRvaEnd;
  UINT32                         SectRawEnd;
  UINT16                         SectIndex;
  CONST EFI_IMAGE_SECTION_HEADER *Sections;

  ASSERT (Context != NULL);
  ASSERT (Context->SizeOfHeaders >= Context->TeStrippedOffset);
  ASSERT (IS_POW2 (Context->SectionAlignment));
  ASSERT (NumberOfSections > 0);
  ASSERT (BottomAddress != NULL);
  ASSERT (TopAddress != NULL);

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + SectionsOffset
               );

  if (Sections[0].VirtualAddress == 0) {
    if (Context->ImageType == ImageTypeTe) {
      return IMAGE_ERROR_UNSUPPORTED;
    }

    NextSectRva = 0;
  } else {
    // TODO: Disallow PCD

    Result = OcOverflowAlignUpU32 (
               Context->SizeOfHeaders,
               Context->SectionAlignment,
               &NextSectRva
               );
    if (Result) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
  }

  SectRvaPrevEnd = NextSectRva;

  *BottomAddress = NextSectRva;
  
  for (SectIndex = 0; SectIndex < NumberOfSections; ++SectIndex) {
    if (Sections[SectIndex].SizeOfRawData > 0) {
      if (Context->TeStrippedOffset > Sections[SectIndex].PointerToRawData) {
        return IMAGE_ERROR_UNSUPPORTED;
      }

      Result = OcOverflowAddU32 (
                 Sections[SectIndex].PointerToRawData,
                 Sections[SectIndex].SizeOfRawData,
                 &SectRawEnd
                 );
      if (Result) {
        return IMAGE_ERROR_UNSUPPORTED;
      }
      
      if ((SectRawEnd - Context->TeStrippedOffset) > FileSize) {
        return IMAGE_ERROR_UNSUPPORTED;
      }
    }

    Result = OcOverflowAddU32 (
               Sections[SectIndex].VirtualAddress,
               Sections[SectIndex].VirtualSize,
               &SectRvaEnd
               );
    if (Result) {
      return IMAGE_ERROR_UNSUPPORTED;
    }

    //
    // FIXME: Misaligned images should be handled with a PCD.
    //
    if (Sections[SectIndex].VirtualAddress < SectRvaPrevEnd) {
      return IMAGE_ERROR_UNSUPPORTED;
    }

    SectRvaPrevEnd = SectRvaEnd;

    //
    // Sections must have virtual addresses adjacent in ascending order.
    // SectionSize does not need to be aligned, so align the result.
    //
    Result = OcOverflowAlignUpU32 (
               SectRvaEnd,
               Context->SectionAlignment,
               &NextSectRva
               );

    if (Result) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
  }

  *TopAddress = NextSectRva;

  return IMAGE_ERROR_SUCCESS;
}

STATIC
IMAGE_STATUS
InternalValidateRelocInfo (
  IN CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN UINT32                              BottomAddress
  )
{
  BOOLEAN Result;
  UINT32  SectRvaEnd;

  ASSERT (Context != NULL);
  ASSERT (BottomAddress == 0 || BottomAddress == ALIGN_VALUE (Context->SizeOfHeaders, Context->SectionAlignment));
  //
  // If the relocations have not been stripped, sanitize their directory.
  //
  if (!Context->RelocsStripped) {
    if (sizeof (EFI_IMAGE_BASE_RELOCATION) > Context->RelocDirSize) {
      return IMAGE_ERROR_UNSUPPORTED;
    }

    Result = OcOverflowAddU32 (
               Context->RelocDirRva,
               Context->RelocDirSize,
               &SectRvaEnd
               );
    //
    // Ensure no overflow has occured.
    //
    if (Result) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
    //
    // Ensure the directory does not overlap with the image header.
    //
    if (BottomAddress > Context->RelocDirRva) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
    //
    // Ensure the directory is within the bounds of the image's virtual space.
    //
    if (SectRvaEnd > Context->SizeOfImage) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
    //
    // Ensure the directory's start is propery aligned.
    //
    if (!OC_TYPE_ALIGNED (EFI_IMAGE_BASE_RELOCATION, Context->RelocDirRva)) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
  } else {
    if (!IS_ALIGNED (Context->ImageBase, Context->SectionAlignment)) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
    //
    // Ensure DXE Runtime Driver can be relocated.
    //
    if (Context->Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
      return IMAGE_ERROR_UNSUPPORTED;
    }
  }

  return IMAGE_ERROR_SUCCESS;
}

STATIC
IMAGE_STATUS
InternalInitializeTe (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN     UINTN                         FileSize
  )
{
  IMAGE_STATUS              Status;
  BOOLEAN                   Result;
  CONST EFI_TE_IMAGE_HEADER *TeHdr;
  UINT32                    BottomAddress;
  UINT32                    SizeOfImage;
  UINT32                    SectionsOffset;

  ASSERT (Context != NULL);
  ASSERT (Context->ExeHdrOffset <= FileSize);
  ASSERT (FileSize - Context->ExeHdrOffset >= sizeof (EFI_TE_IMAGE_HEADER));
  ASSERT (OC_TYPE_ALIGNED (EFI_TE_IMAGE_HEADER, Context->ExeHdrOffset));

  Context->ImageType = ImageTypeTe;


  TeHdr = (CONST EFI_TE_IMAGE_HEADER *) (CONST VOID *) (
            (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
            );

  STATIC_ASSERT (
    OC_TYPE_ALIGNED (EFI_IMAGE_SECTION_HEADER, sizeof (*TeHdr)),
    "The section alignment requirements are violated."
    );

  Result = OcOverflowSubU32 (
             TeHdr->StrippedSize,
             sizeof (*TeHdr),
             &Context->TeStrippedOffset
             );
  if (Result) {
    return IMAGE_ERROR_UNSUPPORTED;
  }

  if (TeHdr->NumberOfSections == 0) {
    return IMAGE_ERROR_UNSUPPORTED;
  }

  STATIC_ASSERT (
    sizeof (EFI_IMAGE_SECTION_HEADER) <= (MAX_UINT32 - sizeof (*TeHdr)) / MAX_UINT8,
    "These arithmetics may overflow."
    );
  //
  // Assign SizeOfHeaders in a way that is equivalent to what the size would
  // be if this was the original (unstripped) PE32 binary. As the TE image
  // creation fixes no fields up, tests work the same way as for PE32.
  // when referencing raw data however, the offset must be subracted.
  //
  Result = OcOverflowAddU32 (
             TeHdr->StrippedSize,
             (UINT32) TeHdr->NumberOfSections * sizeof (EFI_IMAGE_SECTION_HEADER),
             &Context->SizeOfHeaders
             );
  if (Result) {
    return IMAGE_ERROR_UNSUPPORTED;
  }
  //
  // Ensure that all headers are in bounds of the file buffer.
  //
  if ((Context->SizeOfHeaders - Context->TeStrippedOffset) > FileSize) {
    return IMAGE_ERROR_UNSUPPORTED;
  }

  Context->SectionAlignment = BASE_4KB;
  SectionsOffset = Context->ExeHdrOffset + sizeof (EFI_TE_IMAGE_HEADER);
  //
  // Validate the sections.
  // TE images do not have a field to explicitly describe the image size.
  // Set it to the top of the image's virtual space.
  //
  Status = InternalVerifySections (
             Context,
             FileSize,
             SectionsOffset,
             TeHdr->NumberOfSections,
             &BottomAddress,
             &SizeOfImage
             );

  if (Status != IMAGE_ERROR_SUCCESS) {
    return Status;
  }

  Context->SizeOfImage         = SizeOfImage;
  Context->Machine             = TeHdr->Machine;
  Context->Subsystem           = TeHdr->Subsystem;
  Context->ImageBase           = TeHdr->ImageBase;
  Context->RelocsStripped      = TeHdr->DataDirectory[0].Size > 0;
  Context->AddressOfEntryPoint = TeHdr->AddressOfEntryPoint;
  Context->RelocDirRva         = TeHdr->DataDirectory[0].VirtualAddress;
  Context->RelocDirSize        = TeHdr->DataDirectory[0].Size;

  return InternalValidateRelocInfo (Context, BottomAddress);
}

STATIC
IMAGE_STATUS
InternalInitializePe (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN     UINTN                         FileSize
  )
{
  BOOLEAN                               Result;
  CONST EFI_IMAGE_NT_HEADERS_COMMON_HDR *PeCommon;
  CONST EFI_IMAGE_NT_HEADERS32          *Pe32;
  CONST EFI_IMAGE_NT_HEADERS64          *Pe32Plus;
  CONST VOID                            *OptHdrPtr;
  UINT32                                HdrSizeWithoutDataDir;
  UINT32                                SizeOfOptionalHdr;
  UINT32                                SizeOfHeaders;
  CONST EFI_IMAGE_DATA_DIRECTORY        *RelocDir;
  UINT32                                NumberOfRvaAndSizes;
  UINT32                                SectHdrOffset;
  IMAGE_STATUS                          Status;
  UINT32                                BottomAddress;
  UINT32                                SizeOfImage;

  ASSERT (Context != NULL);

  OptHdrPtr = (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset + sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);
  ASSERT (OC_TYPE_ALIGNED (EFI_IMAGE_NT_HEADERS_COMMON_HDR, Context->ExeHdrOffset));
  
  STATIC_ASSERT (
    OC_TYPE_ALIGNED (UINT16, OC_ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR))
   && OC_TYPE_ALIGNED (UINT16, sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR)),
    "The following operation might be an unaligned access."
  );

  //
  // Determine the type of and retrieve data from the PE Optional Header.
  //
  //
  // FIXME: OptHdrPtr could point to unaligned memory.
  //
  switch (*(CONST UINT16 *) OptHdrPtr) {
    case EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC:
      if (sizeof (*Pe32) > FileSize - Context->ExeHdrOffset) {
        DEBUG ((DEBUG_INFO, "OCPE: Invalid 32-bit OPT header\n"));
        return IMAGE_ERROR_UNSUPPORTED;
      }

      Context->ImageType = ImageTypePe32;

      Pe32 = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
               );

      HdrSizeWithoutDataDir = OFFSET_OF (EFI_IMAGE_NT_HEADERS32, DataDirectory) - sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);
      Context->Subsystem    = Pe32->Subsystem;
      NumberOfRvaAndSizes   = Pe32->NumberOfRvaAndSizes;

      RelocDir = Pe32->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC;

      Context->SizeOfImage         = Pe32->SizeOfImage;
      Context->SizeOfHeaders       = Pe32->SizeOfHeaders;
      Context->ImageBase           = Pe32->ImageBase;
      Context->AddressOfEntryPoint = Pe32->AddressOfEntryPoint;
      Context->SectionAlignment    = Pe32->SectionAlignment;

      PeCommon = &Pe32->CommonHeader;
      break;

    case EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC:
      if (sizeof (*Pe32Plus) > FileSize - Context->ExeHdrOffset) {
        DEBUG ((DEBUG_INFO, "OCPE: Invalid 64-bit OPT header\n"));
        return IMAGE_ERROR_UNSUPPORTED;
      }

      Context->ImageType = ImageTypePe32Plus;


      Pe32Plus = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                   (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                   );

      HdrSizeWithoutDataDir = OFFSET_OF (EFI_IMAGE_NT_HEADERS64, DataDirectory) - sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);
      Context->Subsystem    = Pe32Plus->Subsystem;
      NumberOfRvaAndSizes   = Pe32Plus->NumberOfRvaAndSizes;

      RelocDir = Pe32Plus->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC;

      Context->SizeOfImage         = Pe32Plus->SizeOfImage;
      Context->SizeOfHeaders       = Pe32Plus->SizeOfHeaders;
      Context->ImageBase           = Pe32Plus->ImageBase;
      Context->AddressOfEntryPoint = Pe32Plus->AddressOfEntryPoint;
      Context->SectionAlignment    = Pe32Plus->SectionAlignment;

      PeCommon = &Pe32Plus->CommonHeader;
      break;

    default:
      return IMAGE_ERROR_UNSUPPORTED;
  }

  if (PeCommon->FileHeader.NumberOfSections == 0) {
    DEBUG ((DEBUG_INFO, "OCPE: No sections in the image\n"));
    return IMAGE_ERROR_UNSUPPORTED;
  }
  //
  // Do not load images with unknown directories.
  //
  if (NumberOfRvaAndSizes > EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES) {
    DEBUG ((DEBUG_INFO, "OCPE: NumberOfRvaAndSizes is too high %u\n", NumberOfRvaAndSizes));
    return IMAGE_ERROR_UNSUPPORTED;
  }

  if (!IS_POW2 (Context->SectionAlignment)) {
    DEBUG ((DEBUG_INFO, "OCPE: Invalid section alignment %u\n", Context->SectionAlignment));
    return IMAGE_ERROR_UNSUPPORTED;
  }
  //
  // SizeOfOptionalHdr cannot overflow because NumberOfRvaAndSizes has
  // been sanitized and the other two components are validated constants.
  //
  STATIC_ASSERT (
    sizeof (EFI_IMAGE_DATA_DIRECTORY) <= MAX_UINT32 / EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES,
    "These arithmetics may overflow."
    );

  SizeOfOptionalHdr = HdrSizeWithoutDataDir +
                        NumberOfRvaAndSizes * sizeof (EFI_IMAGE_DATA_DIRECTORY);

  ASSERT (SizeOfOptionalHdr >= HdrSizeWithoutDataDir);
  //
  // Context->ExeHdrOffset + sizeof (*PeCommon) cannot overflow because
  //   * ExeFileSize > sizeof (*PeCommon) and
  //   * Context->ExeHdrOffset + ExeFileSize = FileSize
  //
  Result = OcOverflowAddU32 (
             Context->ExeHdrOffset + sizeof (*PeCommon),
             SizeOfOptionalHdr,
             &SectHdrOffset
             );

  if (Result) {
    DEBUG ((
      DEBUG_INFO,
      "OCPE: Sections offset overflow %u + %u + %u\n",
      Context->ExeHdrOffset,
      (UINT32) sizeof (*PeCommon),
      SizeOfOptionalHdr
      ));
    return IMAGE_ERROR_UNSUPPORTED;
  }
  //
  // Ensure the section headers offset is properly aligned.
  //
  if (!OC_TYPE_ALIGNED (EFI_IMAGE_SECTION_HEADER, SectHdrOffset)) {
    DEBUG ((DEBUG_INFO, "OCPE: Sections are unaligned %u\n", SectHdrOffset));
    return IMAGE_ERROR_UNSUPPORTED;
  }

  STATIC_ASSERT (
    sizeof (EFI_IMAGE_SECTION_HEADER) <= (MAX_UINT32 + 1ULL) / (MAX_UINT16 + 1ULL),
    "These arithmetics may overflow."
    );

  Result = OcOverflowAddU32 (
             SectHdrOffset,
             (UINT32) PeCommon->FileHeader.NumberOfSections * sizeof (EFI_IMAGE_SECTION_HEADER),
             &SizeOfHeaders
             );

  if (Result) {
    DEBUG ((DEBUG_INFO, "OCPE: Sections overflow %u %u\n", SectHdrOffset, PeCommon->FileHeader.NumberOfSections));
    return IMAGE_ERROR_UNSUPPORTED;
  }
  //
  // Ensure the header sizes are sane. SizeOfHeaders contains all header
  // components (DOS, PE Common and Optional Header).
  //
  if (PeCommon->FileHeader.SizeOfOptionalHeader < SizeOfOptionalHdr
   || Context->SizeOfHeaders < SizeOfHeaders) {
    DEBUG ((DEBUG_INFO, "OCPE: SizeOfOptionalHeader %u %u\n", PeCommon->FileHeader.SizeOfOptionalHeader, SizeOfOptionalHdr));
    DEBUG ((DEBUG_INFO, "OCPE: ImageSizeOfHeaders %u %u\n", Context->SizeOfHeaders, SizeOfHeaders));
    return IMAGE_ERROR_UNSUPPORTED;
  }
  //
  // Ensure that all headers are in bounds of the file buffer.
  //
  if (Context->SizeOfHeaders > FileSize) {
    DEBUG ((DEBUG_INFO, "OCPE: Context->SizeOfHeaders > FileSize %u %u\n", Context->SizeOfHeaders, FileSize));
    return IMAGE_ERROR_UNSUPPORTED;
  }

  Status = InternalVerifySections (
             Context,
             FileSize,
             SectHdrOffset,
             PeCommon->FileHeader.NumberOfSections,
             &BottomAddress,
             &SizeOfImage
             );
  if (Status != IMAGE_ERROR_SUCCESS) {
    DEBUG ((DEBUG_INFO, "OCPE: InternalVerifySections %d\n", Status));
    return Status;
  }  
  //
  // Ensure SizeOfImage is equal to the top of the image's virtual space.
  // FIXME: Misaligned images should load with a PCD
  //
  if (Context->SizeOfImage < SizeOfImage) {
    DEBUG ((DEBUG_INFO, "OCPE: Context->SizeOfImage < SizeOfImage %u %u\n", Context->SizeOfImage, SizeOfImage));
    return IMAGE_ERROR_UNSUPPORTED;
  }
  //
  // If there's no relocations, then make sure it's not a runtime driver.
  //
  Context->RelocsStripped =
    (
      PeCommon->FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED
    ) != 0;

  Context->Machine = PeCommon->FileHeader.Machine;

  if (EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC < NumberOfRvaAndSizes) {
    Context->RelocDirRva  = RelocDir->VirtualAddress;
    Context->RelocDirSize = RelocDir->Size;
  } else {
    Context->RelocDirRva  = 0;
    Context->RelocDirSize = 0;
  }

  return InternalValidateRelocInfo (Context, BottomAddress);
}

IMAGE_STATUS
OcPeCoffLoaderInitializeContext (
  OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  CONST VOID                    *FileBuffer,
  IN  UINTN                         FileSize
  )
{
  CONST VOID                 *ImageSig;
  CONST EFI_IMAGE_DOS_HEADER *DosHdr;

  ASSERT (Context != NULL);
  ASSERT (FileBuffer != NULL);
  ASSERT (FileSize > 0);

  ZeroMem (Context, sizeof (*Context));

  Context->FileBuffer = (VOID *) FileBuffer;

  ASSERT (Context != NULL);
  //
  // Check whether the DOS image header is present.
  //
  if (FileSize > sizeof (*DosHdr)
   && *(CONST UINT16 *) Context->FileBuffer == EFI_IMAGE_DOS_SIGNATURE) {
    DosHdr = (CONST EFI_IMAGE_DOS_HEADER *) Context->FileBuffer;
    //
    // When the DOS image header is present, sanitize the offset and
    // retrieve the size of the executable image.
    //
    if (sizeof (EFI_IMAGE_DOS_HEADER) > DosHdr->e_lfanew
     || DosHdr->e_lfanew >= FileSize) {
      return IMAGE_ERROR_UNSUPPORTED;
    }

    Context->ExeHdrOffset = DosHdr->e_lfanew;
  } else {
    //
    // When the DOS image header is not present, assume the image starts with
    // the executable header.
    //
    Context->ExeHdrOffset = 0;
  }
  //
  // Use Signature to determine and handle the image format (PE32(+) / TE).
  //
  ImageSig = (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset;

  if (FileSize - Context->ExeHdrOffset >= sizeof (EFI_TE_IMAGE_HEADER)
   && OC_TYPE_ALIGNED (EFI_TE_IMAGE_HEADER, Context->ExeHdrOffset)
   && *(CONST UINT16 *) ImageSig == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    return InternalInitializeTe (Context, FileSize);
  }
  
  if (FileSize - Context->ExeHdrOffset >= sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) + sizeof (UINT16)
   && OC_TYPE_ALIGNED (EFI_IMAGE_NT_HEADERS_COMMON_HDR, Context->ExeHdrOffset)
   && *(CONST UINT32 *) ImageSig == EFI_IMAGE_NT_SIGNATURE) {
    return InternalInitializePe (Context, FileSize);
  }
  
  return IMAGE_ERROR_INVALID_MACHINE_TYPE;
}

STATIC
BOOLEAN
InternalHashSections (
  IN     CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN     UINT32                              SectionsOffset,
  IN     UINT16                              NumberOfSections,
  IN     HASH_UPDATE                         HashUpdate,
  IN OUT VOID                                *HashContext
  )
{
  BOOLEAN                        Result;

  CONST EFI_IMAGE_SECTION_HEADER *Sections;
  CONST EFI_IMAGE_SECTION_HEADER **SortedSections;
  UINT16                         SectIndex;
  UINT16                         SectionPos;
  UINT32                         SectionTop;
  //
  // Build a temporary table of pointers to all section headers of the image
  // to sort them appropiately.
  //
  SortedSections = AllocatePool (
                     (UINT32) NumberOfSections * sizeof (*SortedSections)
                     );

  if (SortedSections == NULL) {
    return FALSE;
  }

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + SectionsOffset
               );
  //
  // Sort SortedSections by PointerToRawData in ascending order.
  //
  SortedSections[0] = &Sections[0];
  //
  // Insertion Sort.
  //
  for (SectIndex = 1; SectIndex < NumberOfSections; ++SectIndex) {
    for (SectionPos = SectIndex;
     0 < SectionPos
     && SortedSections[SectionPos - 1]->PointerToRawData > Sections[SectIndex].PointerToRawData;
     --SectionPos) {
      SortedSections[SectionPos] = SortedSections[SectionPos - 1];
    }

    SortedSections[SectionPos] = &Sections[SectIndex];
  }

  Result = TRUE;

  SectionTop = 0;
  //
  // Hash the image's sections' data in the ascending order of their offset.
  //
  for (SectIndex = 0; SectIndex < NumberOfSections; ++SectIndex) {
    if (PcdGetBool (PcdImageLoaderHashProhibitOverlap)) {
      if (SectionTop > SortedSections[SectIndex]->PointerToRawData) {
        Result = FALSE;
        break;
      }

      SectionTop = SortedSections[SectIndex]->PointerToRawData + SortedSections[SectIndex]->SizeOfRawData;
    }

    if (SortedSections[SectIndex]->SizeOfRawData > 0) {
      Result = HashUpdate (
                 HashContext,
                 (CONST CHAR8 *) Context->FileBuffer + SortedSections[SectIndex]->PointerToRawData,
                 SortedSections[SectIndex]->SizeOfRawData
                 );
      if (!Result) {
        break;
      }
    }
  }

  FreePool ((VOID *)SortedSections);
  return Result;
}

BOOLEAN
OcPeCoffLoaderHashImage (
  IN     CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN     HASH_UPDATE                         HashUpdate,
  IN OUT VOID                                *HashContext
  )
{
  BOOLEAN                      Result;
  UINT32                       NumberOfRvaAndSizes;
  UINT32                       ChecksumOffset;
  UINT32                       SecurityDirOffset;
  UINT32                       CurrentOffset;
  UINT32                       HashSize;
  CONST EFI_IMAGE_NT_HEADERS32 *Pe32;
  CONST EFI_IMAGE_NT_HEADERS64 *Pe32Plus;
  UINT32                       SectionsOffset;
  UINT16                       NumberOfSections;
  //
  // Hash the entire image excluding:
  //   * its checksum
  //   * its security directory
  //   * its signature
  //
  switch (Context->ImageType) {
    case ImageTypeTe:
      //
      // TE images are not to be signed, as they are supposed to only be part of
      // Firmware Volumes, which may be signed as a whole.
      //
      return FALSE;

    case ImageTypePe32:
      Pe32 = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
               );

      ChecksumOffset      = Context->ExeHdrOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS32, CheckSum);
      SecurityDirOffset   = Context->ExeHdrOffset + (UINT32) OFFSET_OF (EFI_IMAGE_NT_HEADERS32, DataDirectory) +(UINT32) (EFI_IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof (EFI_IMAGE_DATA_DIRECTORY));
      NumberOfRvaAndSizes = Pe32->NumberOfRvaAndSizes;
      SectionsOffset      = Context->ExeHdrOffset + sizeof (Pe32->CommonHeader) + Pe32->CommonHeader.FileHeader.SizeOfOptionalHeader;
      NumberOfSections    = Pe32->CommonHeader.FileHeader.NumberOfSections;

      break;

    case ImageTypePe32Plus:
      Pe32Plus = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                   (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                   );

      ChecksumOffset      = Context->ExeHdrOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS64, CheckSum);
      SecurityDirOffset   = Context->ExeHdrOffset + (UINT32) OFFSET_OF (EFI_IMAGE_NT_HEADERS64, DataDirectory) +(UINT32) (EFI_IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof (EFI_IMAGE_DATA_DIRECTORY));
      NumberOfRvaAndSizes = Pe32Plus->NumberOfRvaAndSizes;
      SectionsOffset      = Context->ExeHdrOffset + sizeof (Pe32Plus->CommonHeader) + Pe32Plus->CommonHeader.FileHeader.SizeOfOptionalHeader;
      NumberOfSections    = Pe32Plus->CommonHeader.FileHeader.NumberOfSections;

      break;

    default:
      ASSERT (FALSE);
      return FALSE;
  }
  //
  // Hash the image header till the image's checksum.
  //
  Result = HashUpdate (HashContext, Context->FileBuffer, ChecksumOffset);
  if (!Result) {
    return FALSE;
  }
  //
  // Skip over the image's checksum.
  //
  CurrentOffset = ChecksumOffset + sizeof (UINT32);

  if (EFI_IMAGE_DIRECTORY_ENTRY_SECURITY < NumberOfRvaAndSizes) {
    //
    // Hash everything after the checksum till the security directory.
    //
    HashSize = SecurityDirOffset - CurrentOffset;
    Result   = HashUpdate (
                 HashContext,
                 (CONST CHAR8 *) Context->FileBuffer + CurrentOffset,
                 HashSize
                 );
    if (!Result) {
      return FALSE;
    }
    //
    // Skip over the security directory. If no further directory exists, this
    // will point to the top of the directory.
    //
    CurrentOffset = SecurityDirOffset + sizeof (EFI_IMAGE_DATA_DIRECTORY);
  }
  //
  // Hash the remainders of the image header.
  //
  HashSize = Context->SizeOfHeaders - CurrentOffset;
  Result   = HashUpdate (
               HashContext,
               (CONST CHAR8 *) Context->FileBuffer + CurrentOffset,
               HashSize
               );

  if (!Result) {
    return FALSE;
  }
  
  return InternalHashSections (
           Context,
           SectionsOffset,
           NumberOfSections,
           HashUpdate,
           HashContext
           );
}

STATIC
IMAGE_STATUS
InternalApplyRelocation (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  UINT32                              RelocOffset,
  IN  UINT32                              RelocIndex,
  IN  UINT64                              Adjust
  )
{
  CONST EFI_IMAGE_BASE_RELOCATION *RelocWalker;
  UINT16                          RelocType;
  UINT16                          RelocOff;
  BOOLEAN                         Result;
  UINT32                          RelocTarget;
  UINT32                          RemRelocTargetSize;
  UINT32                          Fixup32;
  UINT64                          Fixup64;
  CHAR8                           *Fixup;

  RelocWalker = (CONST EFI_IMAGE_BASE_RELOCATION *) (
                  (CONST VOID *) ((CONST CHAR8 *) Context->FileBuffer + RelocOffset)
                  );

  RelocType = IMAGE_RELOC_TYPE (RelocWalker->Relocations[RelocIndex]);
  RelocOff  = IMAGE_RELOC_OFFSET (RelocWalker->Relocations[RelocIndex]);

  if (RelocType == EFI_IMAGE_REL_BASED_ABSOLUTE) {
    return IMAGE_ERROR_SUCCESS;
  }
  //
  // Determine the relocation's target address.
  //
  Result = OcOverflowAddU32 (
             RelocWalker->VirtualAddress,
             RelocOff,
             &RelocTarget
             );

  if (Result) {
    return IMAGE_ERROR_FAILED_RELOCATION;
  }

  Result = OcOverflowSubU32 (
             Context->SizeOfImage,
             RelocTarget,
             &RemRelocTargetSize
             );

  if (Result) {
    return IMAGE_ERROR_FAILED_RELOCATION;
  }

  Fixup = (CHAR8 *) Context->FileBuffer + RelocTarget;
  //
  // Apply the relocation fixup per type.
  // If RelocationData is not NULL, store the current value of the fixup
  // target to determine whether it has been changed during runtime
  // execution.
  //
  // It is not clear how EFI_IMAGE_REL_BASED_HIGH and
  // EFI_IMAGE_REL_BASED_LOW are supposed to be handled. While PE reference
  // suggests to just add the high or low part of the displacement, there
  // are concerns about how it's supposed to deal with wraparounds.
  // As neither LLD,
  //

  switch (RelocType) {
    case EFI_IMAGE_REL_BASED_HIGHLOW:
      if (sizeof (UINT32) > RemRelocTargetSize) {
        return IMAGE_ERROR_FAILED_RELOCATION;
      }

      if (RelocTarget + sizeof (UINT32) > Context->RelocDirRva
       && Context->RelocDirRva + Context->RelocDirSize > RelocTarget) {
        return IMAGE_ERROR_FAILED_RELOCATION;
      }

      Fixup32 = ReadUnaligned32 ((UINT32 *) (VOID *) Fixup) +(UINT32)Adjust;
      WriteUnaligned32 ((UINT32 *) (VOID *) Fixup, Fixup32);

      break;

    case EFI_IMAGE_REL_BASED_DIR64:
      if (sizeof (UINT64) > RemRelocTargetSize) {
        return IMAGE_ERROR_FAILED_RELOCATION;
      }

      if (RelocTarget + sizeof (UINT64) > Context->RelocDirRva
       && Context->RelocDirRva + Context->RelocDirSize > RelocTarget) {
        return IMAGE_ERROR_FAILED_RELOCATION;
      }

      Fixup64 = ReadUnaligned64 ((UINT64 *) (VOID *) Fixup) + Adjust;
      WriteUnaligned64 ((UINT64 *) (VOID *) Fixup, Fixup64);

      break;

    default:
      return IMAGE_ERROR_FAILED_RELOCATION;
  }

  return IMAGE_ERROR_SUCCESS;
}

IMAGE_STATUS
OcPeCoffLoaderRelocateImage (
  IN CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN UINTN                               BaseAddress
  )
{
  BOOLEAN                         Result;
  IMAGE_STATUS                    Status;

  UINT64                          Adjust;
  CONST EFI_IMAGE_BASE_RELOCATION *RelocWalker;

  UINT32                          SizeOfRelocs;
  UINT32                          NumRelocs;

  UINT32                          RelocDataIndex;

  UINT32                          RelocOffset;
  UINT32                          RelocMax;

  UINT32                          RelocIndex;

  ASSERT (Context != NULL);
  ASSERT (Context->RelocDirRva + Context->RelocDirSize >= Context->RelocDirRva);
  ASSERT (Context->RelocDirRva + Context->RelocDirSize <= Context->SizeOfImage);
  //
  // Calculate the image's displacement from its prefered location.
  //
  Adjust = (UINT64) BaseAddress -Context->ImageBase;
  //
  // Runtime drivers should unconditionally go through the full relocation
  // procedure early to eliminate the possibility of errors later at runtime.
  // Runtime drivers don't have their relocations stripped, this is verified
  // during context creation.
  // Skip explicit relocation when the image is already loaded at its
  // prefered location.
  //
  if (Context->Subsystem != EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
   && Adjust == 0) {
    return IMAGE_ERROR_SUCCESS;
  }
  //
  // Ensure relocations have not been stripped.
  //
  ASSERT (!Context->RelocsStripped);
  //
  // Apply relocation fixups to the image.
  //
  RelocOffset = Context->RelocDirRva;

  RelocMax = Context->RelocDirRva + Context->RelocDirSize - sizeof (EFI_IMAGE_BASE_RELOCATION);

  RelocDataIndex = 0;

  while (RelocOffset < RelocMax) {
    RelocWalker = (CONST EFI_IMAGE_BASE_RELOCATION *) (
                    (CONST VOID *) ((CONST CHAR8 *) Context->FileBuffer + RelocOffset)
                    );

    STATIC_ASSERT (
      (sizeof (UINT32) % OC_ALIGNOF (EFI_IMAGE_BASE_RELOCATION)) == 0,
      "The following accesses must be performed unaligned."
      );

    Result = OcOverflowSubU32 (
               RelocWalker->SizeOfBlock,
               sizeof (EFI_IMAGE_BASE_RELOCATION),
               &SizeOfRelocs
               );
    //
    // Ensure no overflow has occured and there is at least one entry.
    //
    if (Result || SizeOfRelocs == 0 || SizeOfRelocs > RelocMax - RelocOffset) {
      return IMAGE_ERROR_FAILED_RELOCATION;
    }
    //
    // Ensure the block's size is padded to ensure proper alignment.
    //
    if ((RelocWalker->SizeOfBlock % sizeof (UINT32)) != 0) {
      return IMAGE_ERROR_FAILED_RELOCATION;
    }
    //
    // This division is safe due to the guarantee made above.
    //
    NumRelocs = SizeOfRelocs / sizeof (*RelocWalker->Relocations);
    //
    // Apply all relocation fixups of the current block.
    //
    for (RelocIndex = 0; RelocIndex < NumRelocs; ++RelocIndex) {
      //
      // Apply the relocation fixup per type.
      // If RelocationData is not NULL, store the current value of the fixup
      // target to determine whether it has been changed during runtime
      // execution.
      //
      // It is not clear how EFI_IMAGE_REL_BASED_HIGH and
      // EFI_IMAGE_REL_BASED_LOW are supposed to be handled. While PE reference
      // suggests to just add the high or low part of the displacement, there
      // are concerns about how it's supposed to deal with wraparounds.
      // As neither LLD,
      //
      Status = InternalApplyRelocation (
                 Context,
                 RelocOffset,
                 RelocIndex,
                 Adjust
                 );
      if (Status != IMAGE_ERROR_SUCCESS) {
        return Status;
      }
    }

    RelocDataIndex += NumRelocs;
    RelocOffset    += RelocWalker->SizeOfBlock;
  }
  //
  // Ensure the relocation directory size matches the contained relocations.
  //
  if (RelocOffset != RelocMax + sizeof (EFI_IMAGE_BASE_RELOCATION)) {
    return IMAGE_ERROR_FAILED_RELOCATION;
  }

  return IMAGE_ERROR_SUCCESS;
}

STATIC
VOID
InternalLoadSections (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  UINT32                              LoadedSizeOfHeaders,
  IN  UINT32                              SectionsOffset,
  IN  UINT16                              NumberOfSections,
  OUT VOID                                *Destination,
  IN  UINT32                              DestinationSize
  )
{
  CONST EFI_IMAGE_SECTION_HEADER *Sections;
  UINT16                         Index;
  UINT32                         DataSize;
  UINT32                         PreviousTopRva;

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + SectionsOffset
               );

  PreviousTopRva = LoadedSizeOfHeaders;

  for (Index = 0; Index < NumberOfSections; ++Index) {
    if (Sections[Index].VirtualSize < Sections[Index].SizeOfRawData) {
      DataSize = Sections[Index].VirtualSize;
    } else {
      DataSize = Sections[Index].SizeOfRawData;
    }

    ZeroMem ((CHAR8 *) Destination + PreviousTopRva, Sections[Index].VirtualAddress - PreviousTopRva);
    CopyMem (
      (CHAR8 *) Destination + Sections[Index].VirtualAddress,
      (CONST CHAR8 *) Context->FileBuffer + (Sections[Index].PointerToRawData -Context->TeStrippedOffset),
      DataSize
      );

    PreviousTopRva = Sections[Index].VirtualAddress + DataSize;
  }

  ZeroMem (
    (CHAR8 *) Destination + PreviousTopRva,
    DestinationSize - PreviousTopRva
    );
}

IMAGE_STATUS
OcPeCoffLoaderLoadImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  OUT    VOID                          *Destination,
  IN     UINT32                        DestinationSize
  )
{
  CHAR8                          *AlignedDest;
  UINT32                         AlignOffset;
  UINT32                         AlignedSize;
  UINT32                         LoadedSizeOfHeaders;
  CONST EFI_IMAGE_NT_HEADERS32   *SrcPe32;
  CONST EFI_IMAGE_NT_HEADERS64   *SrcPe32Plus;
  CONST EFI_TE_IMAGE_HEADER      *SrcTe;
  CONST EFI_IMAGE_SECTION_HEADER *Sections;
  UINT32                         SectionsOffset;
  UINT16                         NumberOfSections;
  UINTN                          Address;
  UINTN                          AlignedAddress;

  ASSERT (Context != NULL);
  ASSERT (Destination != NULL);
  ASSERT (DestinationSize > 0);
  ASSERT (DestinationSize >= Context->SectionAlignment);

  Address = (UINTN)Destination;

  ASSERT (!Context->RelocsStripped || Context->ImageBase == Address);

  AlignedAddress = ALIGN_VALUE (Address, (UINTN) Context->SectionAlignment);
  AlignOffset    = (UINT32) (AlignedAddress - Address);
  AlignedSize    = DestinationSize - AlignOffset;

  ASSERT (Context->SizeOfImage <= AlignedSize);

  AlignedDest = (CHAR8 *) Destination + AlignOffset;

  ZeroMem (Destination, AlignOffset);

  switch (Context->ImageType) {
    case ImageTypeTe:
      SrcTe = (CONST EFI_TE_IMAGE_HEADER *) (CONST VOID *) (
                (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                );

      NumberOfSections = SrcTe->NumberOfSections;
      SectionsOffset   = Context->ExeHdrOffset + sizeof (*SrcTe);
      break;

    case ImageTypePe32:
      SrcPe32 = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
                  (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                  );

      NumberOfSections = SrcPe32->CommonHeader.FileHeader.NumberOfSections;
      SectionsOffset   = Context->ExeHdrOffset + sizeof (SrcPe32->CommonHeader) + SrcPe32->CommonHeader.FileHeader.SizeOfOptionalHeader;
      break;

    case ImageTypePe32Plus:
      SrcPe32Plus = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                      (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                      );

      NumberOfSections = SrcPe32Plus->CommonHeader.FileHeader.NumberOfSections;
      SectionsOffset   = Context->ExeHdrOffset + sizeof (SrcPe32Plus->CommonHeader) + SrcPe32Plus->CommonHeader.FileHeader.SizeOfOptionalHeader;
      break;

    default:
      ASSERT (FALSE);
      return IMAGE_ERROR_UNSUPPORTED;
  }

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + SectionsOffset
               );

  if (Sections[0].VirtualAddress != 0 && PcdGetBool (PcdImageLoaderLoadHeader)) {
    LoadedSizeOfHeaders = (Context->SizeOfHeaders - Context->TeStrippedOffset);

    CopyMem (AlignedDest, Context->FileBuffer, LoadedSizeOfHeaders);
  } else {
    LoadedSizeOfHeaders = 0;
  }

  InternalLoadSections (
    Context,
    LoadedSizeOfHeaders,
    SectionsOffset,
    NumberOfSections,
    AlignedDest,
    AlignedSize
    );
  //
  // Update the location-dependent fields to the loaded destination.
  //
  Context->FileBuffer = AlignedDest;

  return IMAGE_ERROR_SUCCESS;
}

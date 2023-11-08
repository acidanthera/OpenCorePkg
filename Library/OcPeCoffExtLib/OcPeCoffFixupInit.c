/** @file
  Implements APIs to fix certain issues in legacy EFI files in memory before loading.

  Very closely based on MdePkg/Library/BasePeCoffLib2/PeCoffInit.c, and intentionally
  kept more similar to that file than it would otherwise need to be, to easily allow
  diffing and importing future changes if required.

  Copyright (c) 2023, Mike Beaton, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020 - 2021, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  Portions copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>

#include <IndustryStandard/PeImage2.h>

#include <Guid/WinCertificate.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseOverflowLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffLib2.h>
#include <Library/OcStringLib.h>

#include "BasePeCoffLib2Internals.h"

//
// FIXME: Provide an API to destruct the context?
//

/**
  Verify the Image section Headers and initialise the Image memory space size.

  The first Image section must be the beginning of the memory space, or be
  contiguous to the aligned Image Headers.
  Sections must be disjoint and, depending on the policy, contiguous in the
  memory space space.
  The section data must be in bounds bounds of the file buffer.

  @param[in,out] Context        The context describing the Image. Must have been
                                initialised by PeCoffInitializeContext().
  @param[in]     FileSize       The size, in Bytes, of Context->FileBuffer.
  @param[out]    StartAddress   On output, the RVA of the first Image section.
  @param[in]     InMemoryFixup  If TRUE, fixes are made to image in memory.
                                If FALSE, Context is initialised as if fixes were
                                made, but no changes are made to loaded image.

  @retval RETURN_SUCCESS  The Image section Headers are well-formed.
  @retval other           The Image section Headers are malformed.
**/
STATIC
RETURN_STATUS
InternalVerifySections (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN     UINT32                        FileSize,
  OUT    UINT32                        *StartAddress,
  IN     BOOLEAN                       InMemoryFixup
  )
{
  BOOLEAN                   Overflow;
  UINT32                    NextSectRva;
  UINT32                    FixupOffset;
  UINT32                    PreFixupVirtualAddress;
  UINT32                    PostFixupVirtualAddress;
  UINT32                    PreFixupVirtualSize;
  UINT32                    PostFixupVirtualSize;
  CHAR8                     SectionName[EFI_IMAGE_SIZEOF_SHORT_NAME + 1];
  UINT32                    SectRawEnd;
  UINT16                    SectionIndex;
  EFI_IMAGE_SECTION_HEADER  *Sections;

  ASSERT (Context != NULL);
  ASSERT (IS_POW2 (Context->SectionAlignment));
  ASSERT (StartAddress != NULL);
  //
  // Images without Sections have no usable data, disallow them.
  //
  if (Context->NumberOfSections == 0) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  Sections = (EFI_IMAGE_SECTION_HEADER *)(VOID *)(
                                                  (CHAR8 *)Context->FileBuffer + Context->SectionsOffset
                                                  );
  //
  // The first Image section must begin the Image memory space, or it must be
  // adjacent to the Image Headers.
  //
  if (Sections[0].VirtualAddress == 0) {
    // FIXME: Add PCD to disallow.
    NextSectRva = 0;
  } else {
    //
    // Choose the raw or aligned Image Headers size depending on whether loading
    // unaligned Sections is allowed.
    //
    if ((PcdGet32 (PcdImageLoaderAlignmentPolicy) & PCD_ALIGNMENT_POLICY_CONTIGUOUS_SECTIONS) == 0) {
      Overflow = BaseOverflowAlignUpU32 (
                   Context->SizeOfHeaders,
                   Context->SectionAlignment,
                   &NextSectRva
                   );
      if (Overflow) {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }
    } else {
      NextSectRva = Context->SizeOfHeaders;
    }
  }

  SectionName[L_STR_LEN (SectionName)] = '\0';
  *StartAddress                        = NextSectRva;
  //
  // Verify all Image sections are valid.
  //
  for (SectionIndex = 0; SectionIndex < Context->NumberOfSections; ++SectionIndex) {
    AsciiStrnCpyS (SectionName, L_STR_SIZE (SectionName), (CHAR8 *)Sections[SectionIndex].Name, EFI_IMAGE_SIZEOF_SHORT_NAME);
    //
    // Fix up W^X errors in memory.
    //
    if ((Sections[SectionIndex].Characteristics & (EFI_IMAGE_SCN_MEM_EXECUTE | EFI_IMAGE_SCN_MEM_WRITE)) == (EFI_IMAGE_SCN_MEM_EXECUTE | EFI_IMAGE_SCN_MEM_WRITE)) {
      if (InMemoryFixup) {
        Sections[SectionIndex].Characteristics &= ~EFI_IMAGE_SCN_MEM_EXECUTE;
      }

      DEBUG ((DEBUG_INFO, "OCPE: %u fixup W^X for %a\n", InMemoryFixup, SectionName));
    }

    //
    // Verify the Image sections are disjoint (relaxed) or adjacent (strict)
    // depending on whether unaligned Image sections may be loaded or not.
    // Unaligned Image sections have been observed with iPXE Option ROMs and old
    // Apple Mac OS X bootloaders.
    //
    PreFixupVirtualAddress  = Sections[SectionIndex].VirtualAddress;
    PostFixupVirtualAddress = PreFixupVirtualAddress;
    PreFixupVirtualSize     = Sections[SectionIndex].VirtualSize;
    PostFixupVirtualSize    = PreFixupVirtualSize;
    if ((PcdGet32 (PcdImageLoaderAlignmentPolicy) & PCD_ALIGNMENT_POLICY_CONTIGUOUS_SECTIONS) == 0) {
      if (Sections[SectionIndex].VirtualAddress != NextSectRva) {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }
    } else {
      if (Sections[SectionIndex].VirtualAddress < NextSectRva) {
        //
        // Disallow overlap fixup unless we're ovelapping into an empty section.
        //
        if (Sections[SectionIndex].SizeOfRawData > 0) {
          DEBUG_RAISE ();
          return RETURN_VOLUME_CORRUPTED;
        }

        //
        // Fix up section overlap errors in memory.
        //
        FixupOffset = NextSectRva - Sections[SectionIndex].VirtualAddress;
        if (FixupOffset > Sections[SectionIndex].VirtualSize) {
          PostFixupVirtualSize = 0;
        } else {
          PostFixupVirtualSize -= FixupOffset;
        }

        PostFixupVirtualAddress = NextSectRva;
        if (InMemoryFixup) {
          Sections[SectionIndex].VirtualAddress = PostFixupVirtualAddress;
          Sections[SectionIndex].VirtualSize    = PostFixupVirtualSize;
        }

        DEBUG ((
          DEBUG_INFO,
          "OCPE: %u fixup section overlap for %a 0x%X(0x%X)->0x%X(0x%X)\n",
          InMemoryFixup,
          SectionName,
          PreFixupVirtualAddress,
          PreFixupVirtualSize,
          PostFixupVirtualAddress,
          PostFixupVirtualSize
          ));
      }

      //
      // If the Image section address is not aligned by the Image section
      // alignment, fall back to important architecture-specific page sizes if
      // possible, to ensure the Image can have memory protection applied.
      // Otherwise, report no alignment for the Image.
      //
      if (!IS_ALIGNED (PostFixupVirtualAddress, Context->SectionAlignment)) {
        STATIC_ASSERT (
          DEFAULT_PAGE_ALLOCATION_GRANULARITY <= RUNTIME_PAGE_ALLOCATION_GRANULARITY,
          "This code must be adapted to consider the reversed order."
          );

        if (IS_ALIGNED (PostFixupVirtualAddress, RUNTIME_PAGE_ALLOCATION_GRANULARITY)) {
          Context->SectionAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
        } else if (  (DEFAULT_PAGE_ALLOCATION_GRANULARITY < RUNTIME_PAGE_ALLOCATION_GRANULARITY)
                  && IS_ALIGNED (PostFixupVirtualAddress, DEFAULT_PAGE_ALLOCATION_GRANULARITY))
        {
          Context->SectionAlignment = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
        } else {
          Context->SectionAlignment = 1;
        }
      }
    }

    //
    // Verify the Image sections with data are in bounds of the file buffer.
    //
    if (Sections[SectionIndex].SizeOfRawData > 0) {
      Overflow = BaseOverflowAddU32 (
                   Sections[SectionIndex].PointerToRawData,
                   Sections[SectionIndex].SizeOfRawData,
                   &SectRawEnd
                   );
      if (Overflow) {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }

      if (SectRawEnd > FileSize) {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }
    }

    //
    // Determine the end of the current Image section.
    //
    Overflow = BaseOverflowAddU32 (
                 PostFixupVirtualAddress,
                 PostFixupVirtualSize,
                 &NextSectRva
                 );
    if (Overflow) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }

    //
    // VirtualSize does not need to be aligned, so align the result if needed.
    //
    if ((PcdGet32 (PcdImageLoaderAlignmentPolicy) & PCD_ALIGNMENT_POLICY_CONTIGUOUS_SECTIONS) == 0) {
      Overflow = BaseOverflowAlignUpU32 (
                   NextSectRva,
                   Context->SectionAlignment,
                   &NextSectRva
                   );
      if (Overflow) {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }
    }
  }

  //
  // Set SizeOfImage to the aligned end address of the last ImageSection.
  //
  if ((PcdGet32 (PcdImageLoaderAlignmentPolicy) & PCD_ALIGNMENT_POLICY_CONTIGUOUS_SECTIONS) == 0) {
    Context->SizeOfImage = NextSectRva;
  } else {
    //
    // Because VirtualAddress is aligned by SectionAlignment for all Image
    // sections, and they are disjoint and ordered by VirtualAddress,
    // VirtualAddress + VirtualSize must be safe to align by SectionAlignment for
    // all but the last Image section.
    // Determine the strictest common alignment that the last section's end is
    // safe to align to.
    //
    Overflow = BaseOverflowAlignUpU32 (
                 NextSectRva,
                 Context->SectionAlignment,
                 &Context->SizeOfImage
                 );
    if (Overflow) {
      Context->SectionAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      Overflow                  = BaseOverflowAlignUpU32 (
                                    NextSectRva,
                                    Context->SectionAlignment,
                                    &Context->SizeOfImage
                                    );
      if (  (DEFAULT_PAGE_ALLOCATION_GRANULARITY < RUNTIME_PAGE_ALLOCATION_GRANULARITY)
         && Overflow)
      {
        Context->SectionAlignment = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
        Overflow                  = BaseOverflowAlignUpU32 (
                                      NextSectRva,
                                      Context->SectionAlignment,
                                      &Context->SizeOfImage
                                      );
      }

      if (Overflow) {
        Context->SectionAlignment = 1;
      }
    }
  }

  return RETURN_SUCCESS;
}

/**
  Verify the basic Image Relocation information.

  The preferred Image load address must be aligned by the section alignment.
  The Relocation Directory must be contained within the Image section memory.
  The Relocation Directory must be sufficiently aligned in memory.

  @param[in] Context       The context describing the Image. Must have been
                           initialised by PeCoffInitializeContext().
  @param[in] StartAddress  The RVA of the first Image section.

  @retval RETURN_SUCCESS  The basic Image Relocation information is well-formed.
  @retval other           The basic Image Relocation information is malformed.
**/
STATIC
RETURN_STATUS
InternalValidateRelocInfo (
  IN CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN UINT32                              StartAddress
  )
{
  BOOLEAN  Overflow;
  UINT32   SectRvaEnd;

  ASSERT (Context != NULL);
  ASSERT (!Context->RelocsStripped || Context->RelocDirSize == 0);
  //
  // If the Base Relocations have not been stripped, verify their Directory.
  //
  if (Context->RelocDirSize != 0) {
    //
    // Verify the Relocation Directory is not empty.
    //
    if (sizeof (EFI_IMAGE_BASE_RELOCATION_BLOCK) > Context->RelocDirSize) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }

    //
    // Verify the Relocation Directory does not overlap with the Image Headers.
    //
    if (StartAddress > Context->RelocDirRva) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }

    //
    // Verify the Relocation Directory is contained in the Image memory space.
    //
    Overflow = BaseOverflowAddU32 (
                 Context->RelocDirRva,
                 Context->RelocDirSize,
                 &SectRvaEnd
                 );
    if (Overflow || (SectRvaEnd > Context->SizeOfImage)) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }

    //
    // Verify the Relocation Directory start is sufficiently aligned.
    //
    if (!IS_ALIGNED (Context->RelocDirRva, ALIGNOF (EFI_IMAGE_BASE_RELOCATION_BLOCK))) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }
  }

  //
  // Verify the preferred Image load address is sufficiently aligned.
  //
  // FIXME: Only with force-aligned sections? What to do with XIP?
  if (!IS_ALIGNED (Context->ImageBase, (UINT64)Context->SectionAlignment)) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  return RETURN_SUCCESS;
}

/**
  Verify the PE32 or PE32+ Image and initialise Context.

  Used offsets and ranges must be aligned and in the bounds of the raw file.
  Image section Headers and basic Relocation information must be Well-formed.

  @param[in,out] Context        The context describing the Image. Must have been
                                initialised by PeCoffInitializeContext().
  @param[in]     FileSize       The size, in Bytes, of Context->FileBuffer.
  @param[in]     InMemoryFixup  If TRUE, fixes are made to image in memory.
                                If FALSE, Context is initialised as if fixes were
                                made, but no changes are made to loaded image.

  @retval RETURN_SUCCESS  The PE Image is Well-formed.
  @retval other           The PE Image is malformed.
**/
STATIC
RETURN_STATUS
InternalInitializePe (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN     UINT32                        FileSize,
  IN     BOOLEAN                       InMemoryFixup
  )
{
  BOOLEAN                                Overflow;
  CONST EFI_IMAGE_NT_HEADERS_COMMON_HDR  *PeCommon;
  CONST EFI_IMAGE_NT_HEADERS32           *Pe32;
  CONST EFI_IMAGE_NT_HEADERS64           *Pe32Plus;
  CONST CHAR8                            *OptHdrPtr;
  UINT32                                 HdrSizeWithoutDataDir;
  UINT32                                 MinSizeOfOptionalHeader;
  UINT32                                 MinSizeOfHeaders;
  CONST EFI_IMAGE_DATA_DIRECTORY         *RelocDir;
  CONST EFI_IMAGE_DATA_DIRECTORY         *SecDir;
  UINT32                                 SecDirEnd;
  UINT32                                 NumberOfRvaAndSizes;
  RETURN_STATUS                          Status;
  UINT32                                 StartAddress;

  ASSERT (Context != NULL);
  ASSERT (sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) + sizeof (UINT16) <= FileSize - Context->ExeHdrOffset);
  if (!PcdGetBool (PcdImageLoaderAllowMisalignedOffset)) {
    ASSERT (IS_ALIGNED (Context->ExeHdrOffset, ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR)));
  }

  //
  // Locate the PE Optional Header.
  //
  OptHdrPtr  = (CONST CHAR8 *)Context->FileBuffer + Context->ExeHdrOffset;
  OptHdrPtr += sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);

  STATIC_ASSERT (
    IS_ALIGNED (ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR), ALIGNOF (UINT16))
                && IS_ALIGNED (sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR), ALIGNOF (UINT16)),
    "The following operation might be an unaligned access."
    );
  //
  // Determine the type of and retrieve data from the PE Optional Header.
  // Do not retrieve SizeOfImage as the value usually does not follow the
  // specification. Even if the value is large enough to hold the last Image
  // section, it may not be aligned, or it may be too large. No data can
  // possibly be loaded past the last Image section anyway.
  //
  switch (*(CONST UINT16 *)(CONST VOID *)OptHdrPtr) {
    case EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC:
      //
      // Verify the PE32 header is in bounds of the file buffer.
      //
      if (sizeof (*Pe32) > FileSize - Context->ExeHdrOffset) {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }

      //
      // The PE32 header offset is always sufficiently aligned.
      //
      STATIC_ASSERT (
        ALIGNOF (EFI_IMAGE_NT_HEADERS32) <= ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR),
        "The following operations may be unaligned."
        );
      //
      // Populate the common data with information from the Optional Header.
      //
      Pe32 = (CONST EFI_IMAGE_NT_HEADERS32 *)(CONST VOID *)(
                                                            (CONST CHAR8 *)Context->FileBuffer + Context->ExeHdrOffset
                                                            );

      Context->ImageType           = PeCoffLoaderTypePe32;
      Context->Subsystem           = Pe32->Subsystem;
      Context->SizeOfHeaders       = Pe32->SizeOfHeaders;
      Context->ImageBase           = Pe32->ImageBase;
      Context->AddressOfEntryPoint = Pe32->AddressOfEntryPoint;
      Context->SectionAlignment    = Pe32->SectionAlignment;

      RelocDir = Pe32->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC;
      SecDir   = Pe32->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_SECURITY;

      PeCommon              = &Pe32->CommonHeader;
      NumberOfRvaAndSizes   = Pe32->NumberOfRvaAndSizes;
      HdrSizeWithoutDataDir = sizeof (EFI_IMAGE_NT_HEADERS32) - sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);

      break;

    case EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC:
      //
      // Verify the PE32+ header is in bounds of the file buffer.
      //
      if (sizeof (*Pe32Plus) > FileSize - Context->ExeHdrOffset) {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }

      //
      // Verify the PE32+ header offset is sufficiently aligned.
      //
      if (  !PcdGetBool (PcdImageLoaderAllowMisalignedOffset)
         && !IS_ALIGNED (Context->ExeHdrOffset, ALIGNOF (EFI_IMAGE_NT_HEADERS64)))
      {
        DEBUG_RAISE ();
        return RETURN_VOLUME_CORRUPTED;
      }

      //
      // Populate the common data with information from the Optional Header.
      //
      Pe32Plus = (CONST EFI_IMAGE_NT_HEADERS64 *)(CONST VOID *)(
                                                                (CONST CHAR8 *)Context->FileBuffer + Context->ExeHdrOffset
                                                                );

      Context->ImageType           = PeCoffLoaderTypePe32Plus;
      Context->Subsystem           = Pe32Plus->Subsystem;
      Context->SizeOfHeaders       = Pe32Plus->SizeOfHeaders;
      Context->ImageBase           = Pe32Plus->ImageBase;
      Context->AddressOfEntryPoint = Pe32Plus->AddressOfEntryPoint;
      Context->SectionAlignment    = Pe32Plus->SectionAlignment;

      RelocDir = Pe32Plus->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC;
      SecDir   = Pe32Plus->DataDirectory + EFI_IMAGE_DIRECTORY_ENTRY_SECURITY;

      PeCommon              = &Pe32Plus->CommonHeader;
      NumberOfRvaAndSizes   = Pe32Plus->NumberOfRvaAndSizes;
      HdrSizeWithoutDataDir = sizeof (EFI_IMAGE_NT_HEADERS64) - sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR);

      break;

    default:
      //
      // Disallow Images with unknown PE Optional Header signatures.
      //
      DEBUG_RAISE ();
      return RETURN_UNSUPPORTED;
  }

  //
  // Disallow Images with unknown directories.
  //
  if (NumberOfRvaAndSizes > EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  //
  // Verify the Image alignment is a power of 2.
  //
  if (!IS_POW2 (Context->SectionAlignment)) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  STATIC_ASSERT (
    sizeof (EFI_IMAGE_DATA_DIRECTORY) <= MAX_UINT32 / EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES,
    "The following arithmetic may overflow."
    );
  //
  // Calculate the offset of the Image sections.
  //
  // Context->ExeHdrOffset + sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) cannot overflow because
  //   * ExeFileSize > sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) and
  //   * Context->ExeHdrOffset + ExeFileSize = FileSize
  //
  Overflow = BaseOverflowAddU32 (
               Context->ExeHdrOffset + sizeof (*PeCommon),
               PeCommon->FileHeader.SizeOfOptionalHeader,
               &Context->SectionsOffset
               );
  if (Overflow) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  //
  // Verify the Section Headers offset is sufficiently aligned.
  //
  if (  !PcdGetBool (PcdImageLoaderAllowMisalignedOffset)
     && !IS_ALIGNED (Context->SectionsOffset, ALIGNOF (EFI_IMAGE_SECTION_HEADER)))
  {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  //
  // This arithmetic cannot overflow because all values are sufficiently
  // bounded.
  //
  MinSizeOfOptionalHeader = HdrSizeWithoutDataDir +
                            NumberOfRvaAndSizes * sizeof (EFI_IMAGE_DATA_DIRECTORY);

  ASSERT (MinSizeOfOptionalHeader >= HdrSizeWithoutDataDir);

  STATIC_ASSERT (
    sizeof (EFI_IMAGE_SECTION_HEADER) <= (MAX_UINT32 + 1ULL) / (MAX_UINT16 + 1ULL),
    "The following arithmetic may overflow."
    );
  //
  // Calculate the minimum size of the Image Headers.
  //
  Overflow = BaseOverflowAddU32 (
               Context->SectionsOffset,
               (UINT32)PeCommon->FileHeader.NumberOfSections * sizeof (EFI_IMAGE_SECTION_HEADER),
               &MinSizeOfHeaders
               );
  if (Overflow) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  //
  // Verify the Image Header sizes are sane. SizeOfHeaders contains all header
  // components (DOS, PE Common and Optional Header).
  //
  if (MinSizeOfOptionalHeader > PeCommon->FileHeader.SizeOfOptionalHeader) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  if (MinSizeOfHeaders > Context->SizeOfHeaders) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  //
  // Verify the Image Headers are in bounds of the file buffer.
  //
  if (Context->SizeOfHeaders > FileSize) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  //
  // Populate the Image context with information from the Common Header.
  //
  Context->NumberOfSections = PeCommon->FileHeader.NumberOfSections;
  Context->Machine          = PeCommon->FileHeader.Machine;
  Context->RelocsStripped   =
    (
     PeCommon->FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED
    ) != 0;

  if (EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC < NumberOfRvaAndSizes) {
    Context->RelocDirRva  = RelocDir->VirtualAddress;
    Context->RelocDirSize = RelocDir->Size;

    if (Context->RelocsStripped && (Context->RelocDirSize != 0)) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }
  } else {
    ASSERT (Context->RelocDirRva == 0);
    ASSERT (Context->RelocDirSize == 0);
  }

  if (EFI_IMAGE_DIRECTORY_ENTRY_SECURITY < NumberOfRvaAndSizes) {
    Context->SecDirOffset = SecDir->VirtualAddress;
    Context->SecDirSize   = SecDir->Size;
    //
    // Verify the Security Directory is in bounds of the Image buffer.
    //
    Overflow = BaseOverflowAddU32 (
                 Context->SecDirOffset,
                 Context->SecDirSize,
                 &SecDirEnd
                 );
    if (Overflow || (SecDirEnd > FileSize)) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }

    //
    // Verify the Security Directory is sufficiently aligned.
    //
    if (!IS_ALIGNED (Context->SecDirOffset, IMAGE_CERTIFICATE_ALIGN)) {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }

    //
    // Verify the Security Directory size is sufficiently aligned, and that if
    // it is not empty, it can fit at least one certificate.
    //
    if (  (Context->SecDirSize != 0)
       && (  !IS_ALIGNED (Context->SecDirSize, IMAGE_CERTIFICATE_ALIGN)
          || (Context->SecDirSize < sizeof (WIN_CERTIFICATE))))
    {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }
  } else {
    //
    // The Image context is zero'd on allocation.
    //
    ASSERT (Context->SecDirOffset == 0);
    ASSERT (Context->SecDirSize == 0);
  }

  //
  // Verify the Image sections are Well-formed.
  //
  Status = InternalVerifySections (
             Context,
             FileSize,
             &StartAddress,
             InMemoryFixup
             );
  if (Status != RETURN_SUCCESS) {
    DEBUG_RAISE ();
    return Status;
  }

  //
  // Verify the entry point is in bounds of the Image buffer.
  //
  if (Context->AddressOfEntryPoint >= Context->SizeOfImage) {
    DEBUG_RAISE ();
    return RETURN_VOLUME_CORRUPTED;
  }

  //
  // Verify the basic Relocation information is well-formed.
  //
  Status = InternalValidateRelocInfo (Context, StartAddress);
  if (Status != RETURN_SUCCESS) {
    DEBUG_RAISE ();
  }

  return Status;
}

RETURN_STATUS
OcPeCoffFixupInitializeContext (
  OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  CONST VOID                    *FileBuffer,
  IN  UINT32                        FileSize,
  IN  BOOLEAN                       InMemoryFixup
  )
{
  RETURN_STATUS               Status;
  CONST EFI_IMAGE_DOS_HEADER  *DosHdr;

 #ifndef EFIUSER
  // The only expected calling path with InMemoryFixup == FALSE is from AppleEfiSignTool.
  ASSERT (InMemoryFixup);
 #endif

  //
  // Failure of these asserts can be fixed if needed by not using the Pcd
  // values above, we do not do this initially to make it simpler to compare
  // this file with BasePeCoffLib2/PeCoffInit.c.
  // STATIC_ASSERT not suitable here: 'not an integral constant expression'.
  //
  if ((PcdGet32 (PcdImageLoaderAlignmentPolicy) & PCD_ALIGNMENT_POLICY_CONTIGUOUS_SECTIONS) == 0) {
    ASSERT (FALSE);
  }

  if (PcdGetBool (PcdImageLoaderAllowMisalignedOffset)) {
    ASSERT (FALSE);
  }

  ASSERT (Context != NULL);
  ASSERT (FileBuffer != NULL || FileSize == 0);
  //
  // Initialise the Image context with 0-values.
  //
  ZeroMem (Context, sizeof (*Context));

  Context->FileBuffer = FileBuffer;
  Context->FileSize   = FileSize;
  //
  // Check whether the DOS Image Header is present.
  //
  if (  (sizeof (*DosHdr) <= FileSize)
     && (*(CONST UINT16 *)(CONST VOID *)FileBuffer == EFI_IMAGE_DOS_SIGNATURE))
  {
    DosHdr = (CONST EFI_IMAGE_DOS_HEADER *)(CONST VOID *)(
                                                          (CONST CHAR8 *)FileBuffer
                                                          );
    //
    // Verify the DOS Image Header and the Executable Header are in bounds of
    // the file buffer, and that they are disjoint.
    //
    if (  (sizeof (EFI_IMAGE_DOS_HEADER) > DosHdr->e_lfanew)
       || (DosHdr->e_lfanew > FileSize))
    {
      DEBUG_RAISE ();
      return RETURN_VOLUME_CORRUPTED;
    }

    Context->ExeHdrOffset = DosHdr->e_lfanew;
    //
    // Verify the Execution Header offset is sufficiently aligned.
    //
    if (  !PcdGetBool (PcdImageLoaderAllowMisalignedOffset)
       && !IS_ALIGNED (Context->ExeHdrOffset, ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR)))
    {
      return RETURN_UNSUPPORTED;
    }
  }

  //
  // Verify the file buffer can hold a PE Common Header.
  //
  if (FileSize - Context->ExeHdrOffset < sizeof (EFI_IMAGE_NT_HEADERS_COMMON_HDR) + sizeof (UINT16)) {
    return RETURN_UNSUPPORTED;
  }

  STATIC_ASSERT (
    ALIGNOF (UINT32) <= ALIGNOF (EFI_IMAGE_NT_HEADERS_COMMON_HDR),
    "The following access may be performed unaligned"
    );
  //
  // Verify the Image Executable Header has a PE signature.
  //
  if (*(CONST UINT32 *)(CONST VOID *)((CONST CHAR8 *)FileBuffer + Context->ExeHdrOffset) != EFI_IMAGE_NT_SIGNATURE) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Verify the PE Image Header is well-formed.
  //
  Status = InternalInitializePe (Context, FileSize, InMemoryFixup);
  if (Status != RETURN_SUCCESS) {
    return Status;
  }

  return RETURN_SUCCESS;
}

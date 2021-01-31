/** @file
  Implements APIs to verify the Authenticode Signature of PE/COFF Images.

  Portions copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Portions Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "PeCoffInternal.h"

//
// TODO: Import Authenticode fixes and improvements.
//

/**
  Hashes the Image Section data in ascending order of raw file apprearance.

  @param[in]     Context      The context describing the Image. Must have been
                              initialised by PeCoffInitializeContext().
  @param[in]     HashUpdate   The data hashing function.
  @param[in,out] HashContext  The context of the current hash.

  @returns  Whether hashing has been successful.
**/

STATIC
BOOLEAN
InternalHashSections (
  IN     CONST PE_COFF_IMAGE_CONTEXT  *Context,
  IN     PE_COFF_HASH_UPDATE          HashUpdate,
  IN OUT VOID                         *HashContext
  )
{
  BOOLEAN                        Result;

  CONST EFI_IMAGE_SECTION_HEADER *Sections;
  CONST EFI_IMAGE_SECTION_HEADER **SortedSections;
  UINT16                         SectIndex;
  UINT16                         SectionPos;
  UINT32                         SectionTop;
  //
  // 9. Build a temporary table of pointers to all of the section headers in the
  //   image. The NumberOfSections field of COFF File Header indicates how big
  //   the table should be. Do not include any section headers in the table
  //   whose SizeOfRawData field is zero.
  //

  SortedSections = AllocatePool (
                     (UINT32) Context->NumberOfSections * sizeof (*SortedSections)
                     );

  if (SortedSections == NULL) {
    return FALSE;
  }

  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->SectionsOffset
               );
  //
  // 10. Using the PointerToRawData field (offset 20) in the referenced
  //     SectionHeader structure as a key, arrange the table's elements in
  //     ascending order. In other words, sort the section headers in ascending
  //     order according to the disk-file offset of the sections.
  //

  SortedSections[0] = &Sections[0];
  //
  // Perform Insertion Sort.
  //
  for (SectIndex = 1; SectIndex < Context->NumberOfSections; ++SectIndex) {
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
  // 13. Repeat steps 11 and 12 for all of the sections in the sorted table.
  //
  for (SectIndex = 0; SectIndex < Context->NumberOfSections; ++SectIndex) {
    if (PcdGetBool (PcdImageLoaderHashProhibitOverlap)) {
      if (SectionTop > SortedSections[SectIndex]->PointerToRawData) {
        Result = FALSE;
        break;
      }

      SectionTop = SortedSections[SectIndex]->PointerToRawData + SortedSections[SectIndex]->SizeOfRawData;
    }
    //
    // 11. Walk through the sorted table, load the corresponding section into
    //     memory, and hash the entire section. Use the SizeOfRawData field in the
    //     SectionHeader structure to determine the amount of data to hash.
    //

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

  FreePool (SortedSections);
  return Result;
}

BOOLEAN
PeCoffHashImage (
  IN     CONST PE_COFF_IMAGE_CONTEXT  *Context,
  IN     PE_COFF_HASH_UPDATE          HashUpdate,
  IN OUT VOID                         *HashContext
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
  //
  // Preconditions:
  // 1. Load the image header into memory.
  // 2. Initialize a hash algorithm context.
  //

  //
  // This step can be moved here because steps 1 to 5 do not modify the Image.
  //
  // 6. Get the Attribute Certificate Table address and size from the
  //    Certificate Table entry. For details, see section 5.7 of the PE/COFF
  //    specification.
  //

  switch (Context->ImageType) { /* LCOV_EXCL_BR_LINE */
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
      ChecksumOffset = Context->ExeHdrOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS32, CheckSum);
      SecurityDirOffset = Context->ExeHdrOffset + (UINT32) OFFSET_OF (EFI_IMAGE_NT_HEADERS32, DataDirectory) + (UINT32) (EFI_IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof (EFI_IMAGE_DATA_DIRECTORY));
      NumberOfRvaAndSizes = Pe32->NumberOfRvaAndSizes;
      break;

    case ImageTypePe32Plus:
      Pe32Plus = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                   (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                   );
      ChecksumOffset = Context->ExeHdrOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS64, CheckSum);
      SecurityDirOffset = Context->ExeHdrOffset + (UINT32) OFFSET_OF (EFI_IMAGE_NT_HEADERS64, DataDirectory) + (UINT32) (EFI_IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof (EFI_IMAGE_DATA_DIRECTORY));
      NumberOfRvaAndSizes = Pe32Plus->NumberOfRvaAndSizes;
      break;

  /* LCOV_EXCL_START */
    default:
      ASSERT (FALSE);
      UNREACHABLE ();
  }
  /* LCOV_EXCL_STOP */
  //
  // 3. Hash the image header from its base to immediately before the start of
  //    the checksum address, as specified in Optional Header Windows-Specific
  //    Fields.
  //

  Result = HashUpdate (HashContext, Context->FileBuffer, ChecksumOffset);
  if (!Result) {
    return FALSE;
  }

  //
  // 4. Skip over the checksum, which is a 4-byte field.
  //
  CurrentOffset = ChecksumOffset + sizeof (UINT32);

  //
  // 5. Hash everything from the end of the checksum field to immediately before
  //    the start of the Certificate Table entry, as specified in Optional
  //    Header Data Directories.
  //
  if (EFI_IMAGE_DIRECTORY_ENTRY_SECURITY < NumberOfRvaAndSizes) {
    HashSize = SecurityDirOffset - CurrentOffset;

    Result = HashUpdate (
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
  // 7. Exclude the Certificate Table entry from the calculation and hash
  //    everything from the end of the Certificate Table entry to the end of
  //    image header, including Section Table (headers).The Certificate Table
  //    entry is 8 bytes long, as specified in Optional Header Data Directories.
  //
  HashSize = Context->SizeOfHeaders - CurrentOffset;
  Result = HashUpdate (
             HashContext,
             (CONST CHAR8 *) Context->FileBuffer + CurrentOffset,
             HashSize
             );

  if (!Result) {
    return FALSE;
  }

  return InternalHashSections (
           Context,
           HashUpdate,
           HashContext
           );

  //
  // Please note that this implementation currently lacks the hashing of trailing
  // data.
  //

  //
  // This step must be performed by the caller after this routine succeeded.
  // 15. Finalize the hash algorithm context.
  //
}

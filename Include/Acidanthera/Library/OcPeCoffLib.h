/** @file
  Provides services to load and relocate a PE/COFF image.

  The PE/COFF Loader Library abstracts the implementation of a PE/COFF loader for
  IA-32, x86, and EBC processor types. The library functions are memory-based
  and can be ported easily to any environment.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef OC_PE_COFF_LIB_H
#define OC_PE_COFF_LIB_H

#include <IndustryStandard/PeCoffImage.h>

///
/// Image type enumeration for Image format identification from the context.
///
typedef enum {
  ImageTypeTe,
  ImageTypePe32,
  ImageTypePe32Plus,
  ImageTypeMax
} IMAGE_LOADER_IMAGE_TYPE;

///
/// Image context structure used for abstraction and bookkeeping.
/// This structure is publicly exposed for memory allocation reasons and must
/// not be accessed directly outside of the library implementation.
///
typedef struct {
  ///
  /// The preferred load address of the Image.
  ///
  UINT64     ImageBase;
  ///
  /// A pointer to the Image raw file buffer.
  ///
  CONST VOID *FileBuffer;
  ///
  /// A pointer to the loaded Image destination.
  ///
  VOID       *ImageBuffer;
  ///
  /// The offset of the Section Headers from the beginning of the raw file.
  ///
  UINT32     SectionsOffset;
  ///
  /// The number of Sections in the Image.
  ///
  UINT16     NumberOfSections;
  ///
  /// The size, in bytes, required to load the Image.
  ///
  UINT32     SizeOfImage;
  ///
  /// The additional size, in bytes, required to force-load debug information.
  ///
  UINT32     SizeOfImageDebugAdd;
  ///
  /// The alignment, in bytes, of Image Sections virtual addresses.
  ///
  UINT32     SectionAlignment;
  ///
  /// The offset of the Image Header from the beginning of the raw file.
  ///
  UINT32     ExeHdrOffset;
  ///
  /// The combined size, in bytes, of all Image Headers.
  ///
  UINT32     SizeOfHeaders;
  ///
  /// The RVA of the Image entry point.
  ///
  UINT32     AddressOfEntryPoint;
  ///
  /// Indicates whether relocation information has been stripped from the Image.
  ///
  BOOLEAN    RelocsStripped;
  ///
  /// The file format of the Image raw file, refer to IMAGE_LOADER_IMAGE_TYPE.
  ///
  UINT8      ImageType;
  ///
  /// The Subsystem value from the Image Header.
  ///
  UINT16     Subsystem;
  ///
  /// The Machine value from the Image Header.
  ///
  UINT16     Machine;
  ///
  /// The size, in bytes, stripped from the beginning of the Image raw file
  /// during TE file generation. Always 0 for PE Images.
  ///
  UINT16     TeStrippedOffset;
  ///
  /// The RVA of the Relocation Directory.
  ///
  UINT32     RelocDirRva;
  ///
  /// The size, in bytes, of the Relocation Directory.
  ///
  UINT32     RelocDirSize;
  ///
  /// The RVA of the CodeView debug information.
  ///
  UINT32     CodeViewRva;
} PE_COFF_IMAGE_CONTEXT;

///
/// Runtime Image context used to relocate the Image into virtual addressing.
///
typedef struct {
  ///
  /// The RVA of the Relocation Directory.
  ///
  UINT32 RelocDirRva;
  ///
  /// The size, in bytes, of the Relocation Directory.
  ///
  UINT32 RelocDirSize;
  ///
  /// Information bookkept during the initial Image Relocation.
  ///
  UINT64 FixupData[];
} PE_COFF_RUNTIME_CONTEXT;

/**
  Adds the digest of Data to HashContext. This function can be called multiple
  times to compute the digest of discontinuous data.

  @param[in,out] HashContext  The context of the current hash.
  @param[in]     Data         Pointer to the data to be hashed.
  @param[in]     DataSize     The size, in bytes, of Data.

  @returns  Whether hashing has been successful.
**/
typedef
BOOLEAN
(EFIAPI *PE_COFF_HASH_UPDATE)(
  IN OUT VOID        *HashContext,
  IN     CONST VOID  *Data,
  IN     UINTN       DataSize
  );

/**
  Verify the TE, PE32, or PE32+ Image and initialise Context.

  Used offsets and ranges must be aligned and in the bounds of the raw file.
  Image Section Headers and basic Relocation information must be correct.

  @param[out] Context   The context describing the Image.
  @param[in]  FileSize  The size, in bytes, of Context->FileBuffer.

  @retval RETURN_SUCCESS  The file data is correct.
  @retval other           The file data is malformed.
**/
RETURN_STATUS
PeCoffInitializeContext (
  OUT PE_COFF_IMAGE_CONTEXT  *Context,
  IN  CONST VOID             *FileBuffer,
  IN  UINT32                 FileSize
  );

/**
  Load the Image into the destination memory space.

  @param[in]  Context          The context describing the Image. Must have been
                               initialised by PeCoffInitializeContext().
  @param[out] Destination      The Image destination memory. Must be allocated
                               from page memory.
  @param[in]  DestinationSize  The size, in bytes, of Destination.
                               Must be at least
                               Context->SizeOfImage +
                               Context->SizeOfImageDebugAdd. If the Section
                               Alignment exceeds 4 KB, must be at least
                               Context->SizeOfImage +
                               Context->SizeOfImageDebugAdd
                               Context->SectionAlignment.

  @retval RETURN_SUCCESS  The Image was loaded successfully.
  @retval other           The Image could not be loaded successfully.
**/
RETURN_STATUS
PeCoffLoadImage (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context,
  OUT    VOID                   *Destination,
  IN     UINT32                 DestinationSize
  );

/**
  Discards optional Image Sections to disguise sensitive data.

  @param[in] Context  The context describing the Image. Must have been loaded by
                      PeCoffLoadImage().
**/
VOID
PeCoffDiscardSections (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

/**
  Retrieves the size required to bookkeep Runtime Relocation information.

  @param[in]  Context  The context describing the Image. Must have been loaded
                       by PeCoffLoadImage().
  @param[out] Size     On output, the size, in bytes, of the bookkeeping buffer.

  @retval RETURN_SUCCESS  The Runtime context size for the Image was retrieved
                          successfully.
  @retval other           The Runtime context size for the Image could not be
                          retrieved successfully.
**/
RETURN_STATUS
PeCoffRelocationDataSize (
  IN  CONST PE_COFF_IMAGE_CONTEXT  *Context,
  OUT UINT32                       *Size
  );

/**
  Relocate Image for boot-time usage.

  @param[in]  Context             The context describing the Image. Must have
                                  been loaded by PeCoffLoadImage().
  @param[in]  BaseAddress         The address to relocate the Image to.
  @param[out] RelocationData      If not NULL, on output, a buffer bookkeeping
                                  data required for Runtime Relocation.
  @param[in]  RelocationDataSize  The size, in bytes, of RelocationData. Must be
                                  at least as big as PeCoffRelocationDataSize().

  @retval RETURN_SUCCESS  The Image has been relocated successfully.
  @retval other           The Image could not be relocated successfully.
**/
RETURN_STATUS
PeCoffRelocateImage (
  IN  CONST PE_COFF_IMAGE_CONTEXT  *Context,
  IN  UINT64                       BaseAddress,
  OUT PE_COFF_RUNTIME_CONTEXT      *RelocationData OPTIONAL,
  IN  UINT32                       RelocationDataSize
  );

/**
  Relocate Image for Runtime usage.

  @param[in]  Image           The Image destination memory. Must have been
                              relocated by PeCoffRelocateImage().
  @param[in]  ImageSize       The size, in bytes, of Image.
  @param[in]  BaseAddress     The address to relocate the Image to.
  @param[in]  RelocationData  The Relocation context obtained by
                              PeCoffRelocateImage().

  @retval RETURN_SUCCESS  The Image has been relocated successfully.
  @retval other           The Image could not be relocated successfully.
**/
RETURN_STATUS
PeCoffRelocateImageForRuntime (
  IN OUT VOID                           *Image,
  IN     UINT32                         ImageSize,
  IN     UINT64                         BaseAddress,
  IN     CONST PE_COFF_RUNTIME_CONTEXT  *RelocationData
  );

/**
  Retrieves information about the Image CodeView data.

  The Image context is updated accordingly.

  @param[in,out]  Context   The context describing the Image. Must have been
                            initialised by PeCoffInitializeContext().
  @param[in]      FileSize  The size, in bytes, of Context->FileBuffer.
**/
VOID
PeCoffLoaderRetrieveCodeViewInfo (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context,
  IN     UINT32                 FileSize
  );

/**
  Loads the Image CodeView data into memory.

  @param[in,out]  Context   The context describing the Image. Must have been
                            updated by PeCoffLoaderRetrieveCodeViewInfo().
**/
VOID
PeCoffLoaderLoadCodeView (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

/**
  Retrieves the Image PDB path.

  @param[in,out] Context      The context describing the Image. Must have been
                              initialised by PeCoffInitializeContext().
  @param[out]    PdbPath      On output, a pointer to the Image PDB path.
  @param[out]    PdbPathSize  On output, the size, in bytes, of *PdbPath.

  @retval RETURN_SUCCESS  The Image PDB path was retrieved successfully.
  @retval other           The Image PDB path could not be retrieved
                          successfully.
**/
RETURN_STATUS
PeCoffGetPdbPath (
  IN  CONST PE_COFF_IMAGE_CONTEXT  *Context,
  OUT CHAR8                        **PdbPath,
  OUT UINT32                       *PdbPathSize
  );

/**
  Hashes the Image using the Authenticode (PE/COFF Specification 8.1 Appendix A)
  algorithm.

  @param[in]     Context      The context describing the Image. Must have been
                              initialised by PeCoffInitializeContext().
  @param[in]     HashUpdate   The data hashing function.
  @param[in,out] HashContext  The context of the current hash.

  @returns  Whether hashing has been successful.
**/
BOOLEAN
PeCoffHashImage (
  IN     CONST PE_COFF_IMAGE_CONTEXT  *Context,
  IN     PE_COFF_HASH_UPDATE          HashUpdate,
  IN OUT VOID                         *HashContext
  );

#endif // OC_PE_COFF_LIB_H

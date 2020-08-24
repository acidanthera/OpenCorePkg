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

#include <IndustryStandard/OcPeImage.h>

// TODO: move?
/**
  Performs digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If HashContext is NULL, then ASSERT().

  @param[in,out]  HashContext  Pointer to the MD5 context.
  @param[in]      Data         Pointer to the buffer containing the data to be hashed.
  @param[in]      DataLength   Length of Data buffer in bytes.

  @retval TRUE     HASH data digest succeeded.
  @retval FALSE    Invalid HASH context. After HashFinal function has been called, the
                   HASH context cannot be reused.

**/
typedef
BOOLEAN
(EFIAPI *HASH_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );

//
// Return status codes from the PE/COFF Loader services
//
#define IMAGE_ERROR_SUCCESS                      0U
#define IMAGE_ERROR_IMAGE_READ                   1U
#define IMAGE_ERROR_INVALID_PE_HEADER_SIGNATURE  2U
#define IMAGE_ERROR_INVALID_MACHINE_TYPE         3U
#define IMAGE_ERROR_INVALID_SUBSYSTEM            4U
#define IMAGE_ERROR_INVALID_IMAGE_ADDRESS        5U
#define IMAGE_ERROR_INVALID_IMAGE_SIZE           6U
#define IMAGE_ERROR_INVALID_SECTION_ALIGNMENT    7U
#define IMAGE_ERROR_SECTION_NOT_LOADED           8U
#define IMAGE_ERROR_FAILED_RELOCATION            9U
#define IMAGE_ERROR_FAILED_ICACHE_FLUSH          10U
#define IMAGE_ERROR_UNSUPPORTED                  11U

typedef UINTN IMAGE_STATUS;

typedef enum {
  ImageTypeTe,
  ImageTypePe32,
  ImageTypePe32Plus,
  ImageTypeMax
} IMAGE_LOADER_IMAGE_TYPE;

///
/// The context structure used while PE/COFF image is being loaded and relocated.
///
typedef struct {
  ///
  /// Set by OcPeCoffLoaderInitializeContext() to the ImageBase in the PE/COFF header.
  ///
  UINT64  ImageBase;
  //
  // Before LoadImage returns, a pointer to the raw file image.
  // After LoadImage returns, a pointer to the loaded image.
  //
  VOID    *FileBuffer;
  ///
  /// Set by OcPeCoffLoaderInitializeContext() to the SizeOfImage in the PE/COFF header.
  /// Image size includes the size of Debug Entry if it is present.
  ///
  UINT32  SizeOfImage;
  UINT32  SectionAlignment;
  ///
  /// Set by OcPeCoffLoaderInitializeContext() to offset to the PE/COFF header.
  /// If the PE/COFF image does not start with a DOS header, this value is zero.
  /// Otherwise, it's the offset to the PE/COFF header.
  ///
  UINT32  ExeHdrOffset;
  ///
  /// Is set by OcPeCoffLoaderInitializeContext() to the Section Alignment in the PE/COFF header.
  ///
  UINT32  SizeOfHeaders;
  UINT32  AddressOfEntryPoint;
  ///
  /// Set by OcPeCoffLoaderInitializeContext() to TRUE if the PE/COFF image does not contain
  /// relocation information.
  ///
  BOOLEAN RelocsStripped;
  ///
  /// Set by OcPeCoffLoaderInitializeContext() to TRUE if the image is a TE image.
  /// For a definition of the TE Image format, see the Platform Initialization Pre-EFI
  /// Initialization Core Interface Specification.
  ///
  UINT8   ImageType;
  UINT16  Subsystem;
  UINT16  Machine;
  UINT32  TeStrippedOffset;

  UINT32  RelocDirRva;
  UINT32  RelocDirSize;
} PE_COFF_LOADER_IMAGE_CONTEXT;

/**
  Retrieves information about a PE/COFF image.

  Computes the ExeHdrOffset, IsTeImage, ImageType, ImageAddress, ImageSize,
  DestinationAddress, RelocsStripped, SectionAlignment, SizeOfHeaders, and
  DebugDirectoryEntryRva fields of the ImageContext structure.
  If ImageContext is NULL, then return RETURN_INVALID_PARAMETER.
  If the PE/COFF image accessed through the ImageRead service in the ImageContext
  structure is not a supported PE/COFF image type, then return RETURN_UNSUPPORTED.
  If any errors occur while computing the fields of ImageContext,
  then the error status is returned in the ImageError field of ImageContext.
  If the image is a TE image, then SectionAlignment is set to 0.
  The ImageRead and Handle fields of ImageContext structure must be valid prior
  to invoking this service.

  @param  ImageContext              The pointer to the image context structure that
                                    describes the PE/COFF image that needs to be
                                    examined by this function.

  @retval RETURN_SUCCESS            The information on the PE/COFF image was collected.
  @retval RETURN_INVALID_PARAMETER  ImageContext is NULL.
  @retval RETURN_UNSUPPORTED        The PE/COFF image is not supported.

**/
IMAGE_STATUS
OcPeCoffLoaderInitializeContext (
  OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  CONST VOID                    *FileBuffer,
  IN  UINTN                         FileSize
  );

/**
  Loads a PE/COFF image into memory.

  Loads the PE/COFF image accessed through the ImageRead service of ImageContext into the buffer
  specified by the ImageAddress and ImageSize fields of ImageContext.  The caller must allocate
  the load buffer and fill in the ImageAddress and ImageSize fields prior to calling this function.
  The EntryPoint, FixupDataSize, CodeView, PdbPointer and HiiResourceData fields of ImageContext are computed.
  The ImageRead, Handle, ExeHdrOffset, IsTeImage, Machine, ImageType, ImageAddress, ImageSize,
  DestinationAddress, RelocsStripped, SectionAlignment, SizeOfHeaders, and DebugDirectoryEntryRva
  fields of the ImageContext structure must be valid prior to invoking this service.

  If ImageContext is NULL, then ASSERT().

  Note that if the platform does not maintain coherency between the instruction cache(s) and the data
  cache(s) in hardware, then the caller is responsible for performing cache maintenance operations
  prior to transferring control to a PE/COFF image that is loaded using this library.

  @param  ImageContext              The pointer to the image context structure that describes the PE/COFF
                                    image that is being loaded.

  @retval RETURN_SUCCESS            The PE/COFF image was loaded into the buffer specified by
                                    the ImageAddress and ImageSize fields of ImageContext.
                                    Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_BUFFER_TOO_SMALL   The caller did not provide a large enough buffer.
                                    Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_LOAD_ERROR         The PE/COFF image is an EFI Runtime image with no relocations.
                                    Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_INVALID_PARAMETER  The image address is invalid.
                                    Extended status information is in the ImageError field of ImageContext.

**/
IMAGE_STATUS
OcPeCoffLoaderLoadImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  OUT    VOID                          *Destination,
  IN     UINT32                        DestinationSize
  );

IMAGE_STATUS
OcPeCoffLoaderRelocateImage (
  IN CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN UINTN                               BaseAddress
  );

BOOLEAN
OcPeCoffLoaderHashImage (
  IN     CONST PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN     HASH_UPDATE                         HashUpdate,
  IN OUT VOID                                *HashContext
  );

#endif // OC_PE_COFF_LIB_H

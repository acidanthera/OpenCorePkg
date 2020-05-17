/** @file
Copyright (C) 2012, tiamo.  All rights reserved.<BR>
Copyright (C) 2014, dmazar.  All rights reserved.<BR>
Copyright (C) 2018, savvas.  All rights reserved.<BR>
Copyright (C) 2018, Download-Fritz.  All rights reserved.<BR>
Copyright (C) 2019-2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_IMAGE_CONVERSION_H
#define APPLE_IMAGE_CONVERSION_H

#include <Protocol/UgaDraw.h>

#define APPLE_IMAGE_CONVERSION_PROTOCOL_GUID          \
  { 0x0DFCE9F6, 0xC4E3, 0x45EE,                       \
    {0xA0, 0x6A, 0xA8, 0x61, 0x3B, 0x98, 0xA5, 0x07 } }

//
// Protocol revision
// Starting with this version scaled interfaces wered added.
// Older versions had none.
//
#define APPLE_IMAGE_CONVERSION_PROTOCOL_REVISION 0x20000

//
// Generic protocol extension capable of opening any file,
// possibly by chainloading other files
//
#define APPLE_IMAGE_CONVERSION_PROTOCOL_ANY_EXTENSION 0

/**
  Recognise image passed through the buffer.

  @param[in]  ImageBuffer   Buffer containing image data.
  @param[in]  ImageSize     Size of the buffer.

  @retval EFI_INVALID_PARAMETER when ImageBuffer is NULL.
  @retval EFI_INVALID_PARAMETER when ImageSize is 0.
  @retval EFI_UNSUPPORTED       when image is not supported (e.g. too large or corrupted).
  @retval EFI_SUCCESS           when image can be decoded.
**/
typedef
EFI_STATUS
(EFIAPI* RECOGNIZE_IMAGE_DATA) (
  IN VOID                 *ImageBuffer,
  IN UINTN                ImageSize
  );

/**
  Get image dimensions.

  @param[in]  ImageBuffer   Buffer containing image data.
  @param[in]  ImageSize     Size of the buffer.
  @param[out] ImageWidth    Image width in pixels.
  @param[out] ImageHeight   Image height in pixels.

  @retval EFI_INVALID_PARAMETER when ImageBuffer is NULL.
  @retval EFI_INVALID_PARAMETER when ImageSize is 0.
  @retval EFI_INVALID_PARAMETER when ImageWidth is NULL.
  @retval EFI_INVALID_PARAMETER when ImageHeight is NULL.
  @retval EFI_UNSUPPORTED       when image is not supported (e.g. too large or corrupted).
  @retval EFI_SUCCESS           when image dimensions were read.
**/
typedef
EFI_STATUS
(EFIAPI* GET_IMAGE_DIMS) (
  IN  VOID                *ImageBuffer,
  IN  UINTN               ImageSize,
  OUT UINT32              *ImageWidth,
  OUT UINT32              *ImageHeight
  );

/**
  Decode image data in 32-bit format.

  @param[in]     ImageBuffer       Buffer containing image data.
  @param[in]     ImageSize         Size of the buffer.
  @param[in,out] RawImageData      Pointer to decoded buffer pointer.
                                   - When NULL it is allocated from pool.
                                   - When not NULL provided buffer is used.
  @param[in,out] RawImageDataSize  Decoded buffer size.
                                   - Set to allocated area size if allocated.
                                   - Set to truncated area size when provided buffer is used.
                                   - Set to required area size when provided buffer is too small.

  @retval EFI_INVALID_PARAMETER when ImageBuffer is NULL.
  @retval EFI_INVALID_PARAMETER when ImageSize is 0.
  @retval EFI_INVALID_PARAMETER when RawImageData is NULL.
  @retval EFI_INVALID_PARAMETER when RawImageDataSize is NULL.
  @retval EFI_UNSUPPORTED       when image is not supported (e.g. too large or corrupted).
  @retval EFI_OUT_OF_RESOURCES  when allocation error happened.
  @retval EFI_BUFFER_TOO_SMALL  when provided buffer is too small, RawImageDataSize is updated.
  @retval EFI_SUCCESS           when image was decoded successfully.
**/
typedef
EFI_STATUS
(EFIAPI* DECODE_IMAGE_DATA) (
  IN     VOID            *ImageBuffer,
  IN     UINTN           ImageSize,
  IN OUT EFI_UGA_PIXEL   **RawImageData,
  IN OUT UINTN           *RawImageDataSize
  );

/**
  Get image dimensions for scale.
  Protocol revision APPLE_IMAGE_CONVERSION_PROTOCOL_REVISION or higher is required.

  @param[in]  ImageBuffer   Buffer containing image data.
  @param[in]  ImageSize     Size of the buffer.
  @param[in]  Scale         Scaling factor (e.g. 1 or 2).
  @param[out] ImageWidth    Image width in pixels.
  @param[out] ImageHeight   Image height in pixels.

  @retval EFI_INVALID_PARAMETER when ImageBuffer is NULL.
  @retval EFI_INVALID_PARAMETER when ImageSize is 0.
  @retval EFI_INVALID_PARAMETER when Scale is 0.
  @retval EFI_INVALID_PARAMETER when ImageWidth is NULL.
  @retval EFI_INVALID_PARAMETER when ImageHeight is NULL.
  @retval EFI_UNSUPPORTED       when Scale is not supported.
  @retval EFI_UNSUPPORTED       when image is not supported (e.g. too large or corrupted).
  @retval EFI_SUCCESS           when image dimensions were read.
**/
typedef
EFI_STATUS
(EFIAPI* GET_IMAGE_DIMS_EX) (
  IN  VOID               *ImageBuffer,
  IN  UINTN              ImageSize,
  IN  UINTN              Scale,
  OUT UINT32             *ImageWidth,
  OUT UINT32             *ImageHeight
  );

/**
  Decode image data in 32-bit format.
  Protocol revision APPLE_IMAGE_CONVERSION_PROTOCOL_REVISION or higher is required.

  @param[in]     ImageBuffer       Buffer containing image data.
  @param[in]     ImageSize         Size of the buffer.
  @param[in]     Scale             Scaling factor (e.g. 1 or 2).
  @param[in,out] RawImageData      Pointer to decoded buffer pointer.
                                   - When NULL it is allocated from pool.
                                   - When not NULL provided buffer is used.
  @param[in,out] RawImageDataSize  Decoded buffer size.
                                   - Set to allocated area size if allocated.
                                   - Set to truncated area size when provided buffer is used.
                                   - Set to required area size when provided buffer is too small.

  @retval EFI_INVALID_PARAMETER when ImageBuffer is NULL.
  @retval EFI_INVALID_PARAMETER when ImageSize is 0.
  @retval EFI_INVALID_PARAMETER when RawImageData is NULL.
  @retval EFI_INVALID_PARAMETER when RawImageDataSize is NULL.
  @retval EFI_UNSUPPORTED       when Scale is not supported.
  @retval EFI_UNSUPPORTED       when image is not supported (e.g. too large or corrupted).
  @retval EFI_OUT_OF_RESOURCES  when allocation error happened.
  @retval EFI_BUFFER_TOO_SMALL  when provided buffer is too small, RawImageDataSize is updated.
  @retval EFI_SUCCESS           when image was decoded successfully.
**/
typedef
EFI_STATUS
(EFIAPI* DECODE_IMAGE_DATA_EX) (
  IN     VOID            *ImageBuffer,
  IN     UINTN           ImageSize,
  IN     UINTN           Scale,
  IN OUT EFI_UGA_PIXEL   **RawImageData,
  IN OUT UINTN           *RawImageDataSize
  );

/**
  Apple image conversion protocol definition.
**/
typedef struct APPLE_IMAGE_CONVERSION_PROTOCOL_ {
  UINT64                 Revision;
  UINTN                  FileExt;
  RECOGNIZE_IMAGE_DATA   RecognizeImageData;
  GET_IMAGE_DIMS         GetImageDims;
  DECODE_IMAGE_DATA      DecodeImageData;
  GET_IMAGE_DIMS_EX      GetImageDimsEx;
  DECODE_IMAGE_DATA_EX   DecodeImageDataEx;
} APPLE_IMAGE_CONVERSION_PROTOCOL;

extern EFI_GUID gAppleImageConversionProtocolGuid;

#endif //APPLE_IMAGE_CONVERSION_H

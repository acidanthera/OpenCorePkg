/** @file
AppleImageConversion protocol

Copyright (C) 2018 savvas. All rights reserved.<BR>
Portions copyright (C) 2016 slice. All rights reserved.<BR>
Portions copyright (C) 2018 vit9696. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleImageConversionLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcPngLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC
CONST UINT8
mPngHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

STATIC
EFI_STATUS
EFIAPI
RecognizeImageData (
  IN VOID   *ImageBuffer,
  IN UINTN  ImageSize
  )
{
  if (ImageBuffer == NULL || ImageSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (ImageSize < sizeof (mPngHeader)
    || CompareMem (ImageBuffer, mPngHeader, sizeof (mPngHeader)) != 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
GetImageDims (
  IN  VOID    *ImageBuffer,
  IN  UINTN   ImageSize,
  OUT UINT32  *ImageWidth,
  OUT UINT32  *ImageHeight
  )
{
  EFI_STATUS  Status;

  if (ImageBuffer == NULL
    || ImageSize == 0
    || ImageWidth == NULL
    || ImageHeight == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetPngDims (ImageBuffer, ImageSize, ImageWidth, ImageHeight);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCIC: Failed to obtain image dimensions for image\n"));
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
DecodeImageData (
  IN     VOID           *ImageBuffer,
  IN     UINTN          ImageSize,
  IN OUT EFI_UGA_PIXEL  **RawImageData,
  IN OUT UINTN          *RawImageDataSize
  )
{
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           PixelCount;
  UINTN           ByteCount;
  VOID            *RealImageData;
  EFI_UGA_PIXEL   *PixelWalker;
  UINT32          Width;
  UINT32          Height;
  UINT8           TmpChannel;

  STATIC_ASSERT (sizeof (EFI_UGA_PIXEL) == sizeof (UINT32), "Unsupported pixel size");
  STATIC_ASSERT (OFFSET_OF (EFI_UGA_PIXEL, Blue)     == 0,  "Unsupported pixel format");
  STATIC_ASSERT (OFFSET_OF (EFI_UGA_PIXEL, Green)    == 1,  "Unsupported pixel format");
  STATIC_ASSERT (OFFSET_OF (EFI_UGA_PIXEL, Red)      == 2,  "Unsupported pixel format");
  STATIC_ASSERT (OFFSET_OF (EFI_UGA_PIXEL, Reserved) == 3,  "Unsupported pixel format");

  if (ImageBuffer == NULL
    || ImageSize == 0
    || RawImageData == NULL
    || RawImageDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = DecodePng (
    ImageBuffer,
    ImageSize,
    (VOID **) &RealImageData,
    &Width,
    &Height,
    NULL
    );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  PixelCount  = (UINTN) Width * Height;
  ByteCount   = PixelCount * sizeof (*RawImageData);

  //
  // The buffer can be callee or caller allocated.
  // This is differentiated by passing non-null to *RawImageData.
  // For now we always allocate our own data, since boot.efi lets caller do it anyway.
  //
  if (*RawImageData != NULL) {
    if (*RawImageDataSize < ByteCount) {
      FreePool (RealImageData);
      *RawImageDataSize = ByteCount;
      return EFI_BUFFER_TOO_SMALL;
    }

    CopyMem (*RawImageData, RealImageData, ByteCount);
    FreePool (RealImageData);
    *RawImageDataSize = ByteCount;
  } else {
    *RawImageData = RealImageData;
  }

  PixelWalker = *RawImageData;

  for (Index = 0; Index < PixelCount; ++Index) {
    TmpChannel            = PixelWalker->Blue;
    PixelWalker->Blue     = PixelWalker->Red;
    PixelWalker->Red      = TmpChannel;
    PixelWalker->Reserved = 0xFF - PixelWalker->Reserved;
    ++PixelWalker;
  }
  
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
GetImageDimsEx (
  IN  VOID    *Buffer,
  IN  UINTN   BufferSize,
  IN  UINTN   Scale,
  OUT UINT32  *Width,
  OUT UINT32  *Height
  )
{
  if (Buffer == NULL
    || BufferSize == 0
    || Scale == 0
    || Width == NULL
    || Height == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Scale > 1) {
    return EFI_UNSUPPORTED;
  }

  return GetImageDims (Buffer, BufferSize, Width, Height);
}

STATIC
EFI_STATUS
EFIAPI
DecodeImageDataEx (
  IN     VOID           *Buffer,
  IN     UINTN          BufferSize,
  IN     UINTN          Scale,
  IN OUT EFI_UGA_PIXEL  **RawImageData,
  IN OUT UINTN          *RawImageDataSize
  )
{
  if (Buffer == NULL
    || BufferSize == 0
    || Scale == 0
    || RawImageData == NULL
    || RawImageDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Scale > 1) {
    return EFI_UNSUPPORTED;
  }

  return DecodeImageData (Buffer, BufferSize, RawImageData, RawImageDataSize);
}

//
// Image codec protocol instance.
//
STATIC APPLE_IMAGE_CONVERSION_PROTOCOL mAppleImageConversion = {
  APPLE_IMAGE_CONVERSION_PROTOCOL_REVISION,
  APPLE_IMAGE_CONVERSION_PROTOCOL_ANY_EXTENSION,
  RecognizeImageData,
  GetImageDims,
  DecodeImageData,
  GetImageDimsEx,
  DecodeImageDataEx
};

APPLE_IMAGE_CONVERSION_PROTOCOL *
OcAppleImageConversionInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                       Status;
  APPLE_IMAGE_CONVERSION_PROTOCOL  *AppleImageConversionInterface;
  EFI_HANDLE                       NewHandle;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleImageConversionProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCIC: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleImageConversionProtocolGuid,
      NULL,
      (VOID **) &AppleImageConversionInterface
      );
    if (!EFI_ERROR (Status)) {
      return AppleImageConversionInterface;
    }
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gAppleImageConversionProtocolGuid,
    &mAppleImageConversion,
    NULL
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleImageConversion;
}

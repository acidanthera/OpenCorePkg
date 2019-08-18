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

STATIC CONST UINT8 mPngHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

STATIC
EFI_STATUS
EFIAPI
RecognizeImageData (
  IN VOID   *ImageBuffer,
  IN UINTN  ImageSize
  )
{
  BOOLEAN     IsValidPngImage;
  EFI_STATUS  Status = EFI_INVALID_PARAMETER;

  if (ImageBuffer != NULL && ImageSize >= sizeof (mPngHeader)) {
    IsValidPngImage = CompareMem (ImageBuffer, mPngHeader, sizeof (mPngHeader)) == 0;
    if (IsValidPngImage) {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
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

  Status = GetPngDims (ImageBuffer, ImageSize, ImageWidth, ImageHeight);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to obtain image dimensions for image\n"));
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
DecodeImageData (
  IN  VOID           *ImageBuffer,
  IN  UINTN          ImageSize,
  OUT EFI_UGA_PIXEL  **RawImageData,
  OUT UINTN          *RawImageDataSize
  )
{
  UINT32          X;
  UINT32          Y;
  VOID            *Data;
  UINT32          Width;
  UINT32          Height;
  UINT8           *DataWalker;
  EFI_UGA_PIXEL   *Pixel;
  EFI_STATUS      Status;

  if (!RawImageData || !RawImageDataSize) {
    return EFI_INVALID_PARAMETER;
  }

  Status = DecodePng (
            ImageBuffer,
            ImageSize,
            &Data,
            &Width,
            &Height,
            NULL
            );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (OcOverflowTriMulUN (Width, Height, sizeof(EFI_UGA_PIXEL), RawImageDataSize)) {
    FreePng (Data);
    return EFI_UNSUPPORTED;
  }

  *RawImageData = (EFI_UGA_PIXEL *) AllocatePool (*RawImageDataSize);

  if (*RawImageData == NULL) {
    FreePng (Data);
    return EFI_OUT_OF_RESOURCES;
  }

  DataWalker = (UINT8 *) Data;
  Pixel      = *RawImageData;
  for (Y = 0; Y < Height; Y++) {
    for (X = 0; X < Width; X++) {
      Pixel->Red = *DataWalker++;
      Pixel->Green = *DataWalker++;
      Pixel->Blue = *DataWalker++;
      Pixel->Reserved = 0xFF - *DataWalker++;
      Pixel++;
    }
  }

  FreePng (Data);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
GetImageDimsVersion (
  IN VOID     *Buffer,
  IN  UINTN   BufferSize,
  IN  UINTN   Version,
  OUT UINT32  *Width,
  OUT UINT32  *Height
  )
{
  EFI_STATUS  Status = EFI_INVALID_PARAMETER;
  if (Buffer && BufferSize && Version && Height && Width) {
    Status = EFI_UNSUPPORTED;
    if (Version <= APPLE_IMAGE_CONVERSION_PROTOCOL_INTERFACE_V1) {
      Status = GetImageDims (Buffer, BufferSize, Width, Height);
    }
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
DecodeImageDataVersion (
  IN  VOID           *Buffer,
  IN  UINTN          BufferSize,
  IN  UINTN          Version,
  OUT EFI_UGA_PIXEL  **RawImageData,
  OUT UINTN          *RawImageDataSize
  )
{
  EFI_STATUS  Status = EFI_INVALID_PARAMETER;
  if (Buffer && BufferSize && Version && RawImageData && RawImageDataSize) {
    Status = EFI_UNSUPPORTED;
    if (Version <= APPLE_IMAGE_CONVERSION_PROTOCOL_INTERFACE_V1) {
      Status = DecodeImageData (Buffer, BufferSize, RawImageData, RawImageDataSize);
    }
  }

  return Status;
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
  GetImageDimsVersion,
  DecodeImageDataVersion
};

/**
  Install and initialise the Apple Image Conversion protocol.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed or located protocol.
  @retval NULL  There was an error locating or installing the protocol.
**/
APPLE_IMAGE_CONVERSION_PROTOCOL *
OcAppleImageConversionInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                       Status;
  APPLE_IMAGE_CONVERSION_PROTOCOL  *AppleImageConversionInterface = NULL;
  EFI_HANDLE                       NewHandle                      = NULL;

  if (Reinstall) {
    Status = UninstallAllProtocolInstances (&gAppleImageConversionProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCIC: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
                    &gAppleImageConversionProtocolGuid,
                    NULL,
                    (VOID **)&AppleImageConversionInterface
                    );
    if (!EFI_ERROR (Status)) {
      return AppleImageConversionInterface;
    }
  }

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

/** @file
Implementation for EFI_HII_IMAGE_EX_PROTOCOL.


Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HiiDatabase.h"

/**
  The prototype of this extension function is the same with EFI_HII_IMAGE_PROTOCOL.NewImage().
  This protocol invokes EFI_HII_IMAGE_PROTOCOL.NewImage() implicitly.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            Handle of the package list where this image will
                                 be added.
  @param  ImageId                On return, contains the new image id, which is
                                 unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was added successfully.
  @retval EFI_NOT_FOUND          The PackageList could not be found.
  @retval EFI_OUT_OF_RESOURCES   Could not add the image due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Image is NULL or ImageId is NULL.
**/
EFI_STATUS
EFIAPI
HiiNewImageEx (
  IN  CONST EFI_HII_IMAGE_EX_PROTOCOL  *This,
  IN  EFI_HII_HANDLE                   PackageList,
  OUT EFI_IMAGE_ID                     *ImageId,
  IN  CONST EFI_IMAGE_INPUT            *Image
  )
{
  HII_DATABASE_PRIVATE_DATA  *Private;

  Private = HII_IMAGE_EX_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  return HiiNewImage (&Private->HiiImage, PackageList, ImageId, Image);
}

/**
  Return the information about the image, associated with the package list.
  The prototype of this extension function is the same with EFI_HII_IMAGE_PROTOCOL.GetImage().

  This function is similar to EFI_HII_IMAGE_PROTOCOL.GetImage().The difference is that
  this function will locate all EFI_HII_IMAGE_DECODER_PROTOCOL instances installed in the
  system if the decoder of the certain image type is not supported by the
  EFI_HII_IMAGE_EX_PROTOCOL. The function will attempt to decode the image to the
  EFI_IMAGE_INPUT using the first EFI_HII_IMAGE_DECODER_PROTOCOL instance that
  supports the requested image type.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            The package list in the HII database to search for the
                                 specified image.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was returned successfully.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not available. The specified
                                 PackageList is not in the Database.
  @retval EFI_INVALID_PARAMETER  Image was NULL or ImageId was 0.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there
                                 was not enough memory.

**/
EFI_STATUS
EFIAPI
HiiGetImageEx (
  IN  CONST EFI_HII_IMAGE_EX_PROTOCOL  *This,
  IN  EFI_HII_HANDLE                   PackageList,
  IN  EFI_IMAGE_ID                     ImageId,
  OUT EFI_IMAGE_INPUT                  *Image
  )
{
  HII_DATABASE_PRIVATE_DATA  *Private;

  Private = HII_IMAGE_EX_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  return IGetImage (&Private->DatabaseList, PackageList, ImageId, Image, FALSE);
}

/**
  Change the information about the image.

  Same with EFI_HII_IMAGE_PROTOCOL.SetImage(),this protocol invokes
  EFI_HII_IMAGE_PROTOCOL.SetImage()implicitly.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            The package list containing the images.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was successfully updated.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the
                                 database. The specified PackageList is not in
                                 the database.
  @retval EFI_INVALID_PARAMETER  The Image was NULL, the ImageId was 0 or
                                 the Image->Bitmap was NULL.

**/
EFI_STATUS
EFIAPI
HiiSetImageEx (
  IN CONST EFI_HII_IMAGE_EX_PROTOCOL  *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_IMAGE_ID                     ImageId,
  IN CONST EFI_IMAGE_INPUT            *Image
  )
{
  HII_DATABASE_PRIVATE_DATA  *Private;

  Private = HII_IMAGE_EX_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  return HiiSetImage (&Private->HiiImage, PackageList, ImageId, Image);
}

/**
  Renders an image to a bitmap or to the display.

  The prototype of this extension function is the same with
  EFI_HII_IMAGE_PROTOCOL.DrawImage(). This protocol invokes
  EFI_HII_IMAGE_PROTOCOL.DrawImage() implicitly.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  Flags                  Describes how the image is to be drawn.
  @param  Image                  Points to the image to be displayed.
  @param  Blt                    If this points to a non-NULL on entry, this points
                                 to the image, which is Width pixels wide and
                                 Height pixels high.  The image will be drawn onto
                                 this image and  EFI_HII_DRAW_FLAG_CLIP is implied.
                                 If this points to a NULL on entry, then a buffer
                                 will be allocated to hold the generated image and
                                 the pointer updated on exit. It is the caller's
                                 responsibility to free this buffer.
  @param  BltX                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.
  @param  BltY                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.

  @retval EFI_SUCCESS            The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES   Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER  The Image or Blt was NULL.

**/
EFI_STATUS
EFIAPI
HiiDrawImageEx (
  IN CONST EFI_HII_IMAGE_EX_PROTOCOL  *This,
  IN EFI_HII_DRAW_FLAGS               Flags,
  IN CONST EFI_IMAGE_INPUT            *Image,
  IN OUT EFI_IMAGE_OUTPUT             **Blt,
  IN UINTN                            BltX,
  IN UINTN                            BltY
  )
{
  HII_DATABASE_PRIVATE_DATA  *Private;

  Private = HII_IMAGE_EX_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  return HiiDrawImage (&Private->HiiImage, Flags, Image, Blt, BltX, BltY);
}

/**
  Renders an image to a bitmap or the screen containing the contents of the specified
  image.

  This function is similar to EFI_HII_IMAGE_PROTOCOL.DrawImageId(). The difference is that
  this function will locate all EFI_HII_IMAGE_DECODER_PROTOCOL instances installed in the
  system if the decoder of the certain image type is not supported by the
  EFI_HII_IMAGE_EX_PROTOCOL. The function will attempt to decode the image to the
  EFI_IMAGE_INPUT using the first EFI_HII_IMAGE_DECODER_PROTOCOL instance that
  supports the requested image type.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  Flags                  Describes how the image is to be drawn.
  @param  PackageList            The package list in the HII database to search for
                                 the  specified image.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Blt                    If this points to a non-NULL on entry, this points
                                 to the image, which is Width pixels wide and
                                 Height pixels high. The image will be drawn onto
                                 this image and EFI_HII_DRAW_FLAG_CLIP is implied.
                                 If this points to a NULL on entry, then a buffer
                                 will be allocated to hold  the generated image
                                 and the pointer updated on exit. It is the caller's
                                 responsibility to free this buffer.
  @param  BltX                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.
  @param  BltY                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.

  @retval EFI_SUCCESS            The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES   Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER  The Blt was NULL or ImageId was 0.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the database.
                                 The specified PackageList is not in the database.

**/
EFI_STATUS
EFIAPI
HiiDrawImageIdEx (
  IN CONST EFI_HII_IMAGE_EX_PROTOCOL  *This,
  IN EFI_HII_DRAW_FLAGS               Flags,
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_IMAGE_ID                     ImageId,
  IN OUT EFI_IMAGE_OUTPUT             **Blt,
  IN UINTN                            BltX,
  IN UINTN                            BltY
  )
{
  EFI_STATUS       Status;
  EFI_IMAGE_INPUT  Image;

  //
  // Check input parameter.
  //
  if ((This == NULL) || (Blt == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the specified Image.
  //
  Status = HiiGetImageEx (This, PackageList, ImageId, &Image);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Draw this image.
  //
  Status = HiiDrawImageEx (This, Flags, &Image, Blt, BltX, BltY);
  if (Image.Bitmap != NULL) {
    FreePool (Image.Bitmap);
  }

  return Status;
}

/**
  Return the first HII image decoder instance which supports the DecoderName.

  @param BlockType  The image block type.

  @retval Pointer to the HII image decoder instance.
**/
EFI_HII_IMAGE_DECODER_PROTOCOL *
LocateHiiImageDecoder (
  UINT8  BlockType
  )
{
  EFI_STATUS                      Status;
  EFI_HII_IMAGE_DECODER_PROTOCOL  *Decoder;
  EFI_HANDLE                      *Handles;
  UINTN                           HandleNum;
  UINTN                           Index;
  EFI_GUID                        *DecoderNames;
  UINT16                          NumberOfDecoderName;
  UINT16                          DecoderNameIndex;
  EFI_GUID                        *DecoderName;

  switch (BlockType) {
    case EFI_HII_IIBT_IMAGE_JPEG:
      DecoderName = &gEfiHiiImageDecoderNameJpegGuid;
      break;

    case EFI_HII_IIBT_IMAGE_PNG:
      DecoderName = &gEfiHiiImageDecoderNamePngGuid;
      break;

    default:
      ASSERT (FALSE);
      return NULL;
  }

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiHiiImageDecoderProtocolGuid, NULL, &HandleNum, &Handles);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < HandleNum; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiHiiImageDecoderProtocolGuid, (VOID **)&Decoder);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = Decoder->GetImageDecoderName (Decoder, &DecoderNames, &NumberOfDecoderName);
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (DecoderNameIndex = 0; DecoderNameIndex < NumberOfDecoderName; DecoderNameIndex++) {
      if (CompareGuid (DecoderName, &DecoderNames[DecoderNameIndex])) {
        return Decoder;
      }
    }
  }

  return NULL;
}

/**
  This function returns the image information to EFI_IMAGE_OUTPUT. Only the width
  and height are returned to the EFI_IMAGE_OUTPUT instead of decoding the image
  to the buffer. This function is used to get the geometry of the image. This function
  will try to locate all of the EFI_HII_IMAGE_DECODER_PROTOCOL installed on the
  system if the decoder of image type is not supported by the EFI_HII_IMAGE_EX_PROTOCOL.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            Handle of the package list where this image will
                                 be searched.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was returned successfully.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the
                                 database. The specified PackageList is not in the database.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to
                                 hold the image.
  @retval EFI_INVALID_PARAMETER  The Image was NULL or the ImageId was 0.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there
                                 was not enough memory.

**/
EFI_STATUS
EFIAPI
HiiGetImageInfo (
  IN CONST  EFI_HII_IMAGE_EX_PROTOCOL  *This,
  IN        EFI_HII_HANDLE             PackageList,
  IN        EFI_IMAGE_ID               ImageId,
  OUT       EFI_IMAGE_OUTPUT           *Image
  )
{
  EFI_STATUS                               Status;
  HII_DATABASE_PRIVATE_DATA                *Private;
  HII_DATABASE_PACKAGE_LIST_INSTANCE       *PackageListNode;
  HII_IMAGE_PACKAGE_INSTANCE               *ImagePackage;
  EFI_HII_IMAGE_BLOCK                      *CurrentImageBlock;
  EFI_HII_IMAGE_DECODER_PROTOCOL           *Decoder;
  EFI_HII_IMAGE_DECODER_IMAGE_INFO_HEADER  *ImageInfo;

  if ((Image == NULL) || (ImageId == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Private         = HII_IMAGE_EX_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  PackageListNode = LocatePackageList (&Private->DatabaseList, PackageList);
  if (PackageListNode == NULL) {
    return EFI_NOT_FOUND;
  }

  ImagePackage = PackageListNode->ImagePkg;
  if (ImagePackage == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Find the image block specified by ImageId
  //
  CurrentImageBlock = GetImageIdOrAddress (ImagePackage->ImageBlock, &ImageId);
  if (CurrentImageBlock == NULL) {
    return EFI_NOT_FOUND;
  }

  switch (CurrentImageBlock->BlockType) {
    case EFI_HII_IIBT_IMAGE_JPEG:
    case EFI_HII_IIBT_IMAGE_PNG:
      Decoder = LocateHiiImageDecoder (CurrentImageBlock->BlockType);
      if (Decoder == NULL) {
        return EFI_UNSUPPORTED;
      }

      //
      // Use the common block code since the definition of two structures is the same.
      //
      ASSERT (OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Data) == OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Data));
      ASSERT (
        sizeof (((EFI_HII_IIBT_JPEG_BLOCK *)CurrentImageBlock)->Data) ==
        sizeof (((EFI_HII_IIBT_PNG_BLOCK *)CurrentImageBlock)->Data)
        );
      ASSERT (OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Size) == OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Size));
      ASSERT (
        sizeof (((EFI_HII_IIBT_JPEG_BLOCK *)CurrentImageBlock)->Size) ==
        sizeof (((EFI_HII_IIBT_PNG_BLOCK *)CurrentImageBlock)->Size)
        );
      Status = Decoder->GetImageInfo (
                          Decoder,
                          ((EFI_HII_IIBT_JPEG_BLOCK *)CurrentImageBlock)->Data,
                          ((EFI_HII_IIBT_JPEG_BLOCK *)CurrentImageBlock)->Size,
                          &ImageInfo
                          );

      //
      // Spec requires to use the first capable image decoder instance.
      // The first image decoder instance may fail to decode the image.
      //
      if (!EFI_ERROR (Status)) {
        Image->Height       = ImageInfo->ImageHeight;
        Image->Width        = ImageInfo->ImageWidth;
        Image->Image.Bitmap = NULL;
        FreePool (ImageInfo);
      }

      return Status;

    case EFI_HII_IIBT_IMAGE_1BIT_TRANS:
    case EFI_HII_IIBT_IMAGE_4BIT_TRANS:
    case EFI_HII_IIBT_IMAGE_8BIT_TRANS:
    case EFI_HII_IIBT_IMAGE_1BIT:
    case EFI_HII_IIBT_IMAGE_4BIT:
    case EFI_HII_IIBT_IMAGE_8BIT:
      //
      // Use the common block code since the definition of these structures is the same.
      //
      Image->Width        = ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *)CurrentImageBlock)->Bitmap.Width);
      Image->Height       = ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *)CurrentImageBlock)->Bitmap.Height);
      Image->Image.Bitmap = NULL;
      return EFI_SUCCESS;

    case EFI_HII_IIBT_IMAGE_24BIT_TRANS:
    case EFI_HII_IIBT_IMAGE_24BIT:
      Image->Width        = ReadUnaligned16 ((VOID *)&((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *)CurrentImageBlock)->Bitmap.Width);
      Image->Height       = ReadUnaligned16 ((VOID *)&((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *)CurrentImageBlock)->Bitmap.Height);
      Image->Image.Bitmap = NULL;
      return EFI_SUCCESS;

    default:
      return EFI_NOT_FOUND;
  }
}

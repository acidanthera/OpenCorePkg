/** @file
Implementation for EFI_HII_IMAGE_PROTOCOL.


Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "HiiDatabase.h"

#define MAX_UINT24    0xFFFFFF

/**
  Get the imageid of last image block: EFI_HII_IIBT_END_BLOCK when input
  ImageId is zero, otherwise return the address of the
  corresponding image block with identifier specified by ImageId.

  This is a internal function.

  @param ImageBlocks     Points to the beginning of a series of image blocks stored in order.
  @param ImageId         If input ImageId is 0, output the image id of the EFI_HII_IIBT_END_BLOCK;
                         else use this id to find its corresponding image block address.

  @return The image block address when input ImageId is not zero; otherwise return NULL.

**/
EFI_HII_IMAGE_BLOCK *
GetImageIdOrAddress (
  IN EFI_HII_IMAGE_BLOCK *ImageBlocks,
  IN OUT EFI_IMAGE_ID    *ImageId
  )
{
  EFI_IMAGE_ID                   ImageIdCurrent;
  EFI_HII_IMAGE_BLOCK            *CurrentImageBlock;
  UINTN                          Length;

  ASSERT (ImageBlocks != NULL && ImageId != NULL);
  CurrentImageBlock = ImageBlocks;
  ImageIdCurrent    = 1;

  while (CurrentImageBlock->BlockType != EFI_HII_IIBT_END) {
    if (*ImageId != 0) {
      if (*ImageId == ImageIdCurrent) {
        //
        // If the found image block is a duplicate block, update the ImageId to
        // find the previous defined image block.
        //
        if (CurrentImageBlock->BlockType == EFI_HII_IIBT_DUPLICATE) {
          *ImageId = ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_DUPLICATE_BLOCK *) CurrentImageBlock)->ImageId);
          ASSERT (*ImageId != ImageIdCurrent);
          ASSERT (*ImageId != 0);
          CurrentImageBlock = ImageBlocks;
          ImageIdCurrent = 1;
          continue;
        }

        return CurrentImageBlock;
      }
      if (*ImageId < ImageIdCurrent) {
        //
        // Can not find the specified image block in this image.
        //
        return NULL;
      }
    }
    switch (CurrentImageBlock->BlockType) {
    case EFI_HII_IIBT_EXT1:
      Length = ((EFI_HII_IIBT_EXT1_BLOCK *) CurrentImageBlock)->Length;
      break;
    case EFI_HII_IIBT_EXT2:
      Length = ReadUnaligned16 (&((EFI_HII_IIBT_EXT2_BLOCK *) CurrentImageBlock)->Length);
      break;
    case EFI_HII_IIBT_EXT4:
      Length = ReadUnaligned32 ((VOID *) &((EFI_HII_IIBT_EXT4_BLOCK *) CurrentImageBlock)->Length);
      break;

    case EFI_HII_IIBT_IMAGE_1BIT:
    case EFI_HII_IIBT_IMAGE_1BIT_TRANS:
      Length = sizeof (EFI_HII_IIBT_IMAGE_1BIT_BLOCK) - sizeof (UINT8) +
               BITMAP_LEN_1_BIT (
                 ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                 ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                 );
      ImageIdCurrent++;
      break;

    case EFI_HII_IIBT_IMAGE_4BIT:
    case EFI_HII_IIBT_IMAGE_4BIT_TRANS:
      Length = sizeof (EFI_HII_IIBT_IMAGE_4BIT_BLOCK) - sizeof (UINT8) +
               BITMAP_LEN_4_BIT (
                 ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                 ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                 );
      ImageIdCurrent++;
      break;

    case EFI_HII_IIBT_IMAGE_8BIT:
    case EFI_HII_IIBT_IMAGE_8BIT_TRANS:
      Length = sizeof (EFI_HII_IIBT_IMAGE_8BIT_BLOCK) - sizeof (UINT8) +
               BITMAP_LEN_8_BIT (
                 (UINT32) ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                 ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                 );
      ImageIdCurrent++;
      break;

    case EFI_HII_IIBT_IMAGE_24BIT:
    case EFI_HII_IIBT_IMAGE_24BIT_TRANS:
      Length = sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL) +
               BITMAP_LEN_24_BIT (
                 (UINT32) ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                 ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                 );
      ImageIdCurrent++;
      break;

    case EFI_HII_IIBT_DUPLICATE:
      Length = sizeof (EFI_HII_IIBT_DUPLICATE_BLOCK);
      ImageIdCurrent++;
      break;

    case EFI_HII_IIBT_IMAGE_JPEG:
      Length = OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Data) + ReadUnaligned32 ((VOID *) &((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Size);
      ImageIdCurrent++;
      break;

    case EFI_HII_IIBT_IMAGE_PNG:
      Length = OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Data) + ReadUnaligned32 ((VOID *) &((EFI_HII_IIBT_PNG_BLOCK *) CurrentImageBlock)->Size);
      ImageIdCurrent++;
      break;

    case EFI_HII_IIBT_SKIP1:
      Length = sizeof (EFI_HII_IIBT_SKIP1_BLOCK);
      ImageIdCurrent += ((EFI_HII_IIBT_SKIP1_BLOCK *) CurrentImageBlock)->SkipCount;
      break;

    case EFI_HII_IIBT_SKIP2:
      Length = sizeof (EFI_HII_IIBT_SKIP2_BLOCK);
      ImageIdCurrent += ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_SKIP2_BLOCK *) CurrentImageBlock)->SkipCount);
      break;

    default:
      //
      // Unknown image blocks can not be skipped, processing halts.
      //
      ASSERT (FALSE);
      Length = 0;
      break;
    }

    CurrentImageBlock = (EFI_HII_IMAGE_BLOCK *) ((UINT8 *) CurrentImageBlock + Length);

  }

  //
  // When ImageId is zero, return the imageid of last image block: EFI_HII_IIBT_END_BLOCK.
  //
  if (*ImageId == 0) {
    *ImageId = ImageIdCurrent;
    return CurrentImageBlock;
  }

  return NULL;
}



/**
  Convert pixels from EFI_GRAPHICS_OUTPUT_BLT_PIXEL to EFI_HII_RGB_PIXEL style.

  This is a internal function.


  @param  BitMapOut              Pixels in EFI_HII_RGB_PIXEL format.
  @param  BitMapIn               Pixels in EFI_GRAPHICS_OUTPUT_BLT_PIXEL format.
  @param  PixelNum               The number of pixels to be converted.


**/
VOID
CopyGopToRgbPixel (
  OUT EFI_HII_RGB_PIXEL              *BitMapOut,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BitMapIn,
  IN  UINTN                          PixelNum
  )
{
  UINTN Index;

  ASSERT (BitMapOut != NULL && BitMapIn != NULL);

  for (Index = 0; Index < PixelNum; Index++) {
    CopyMem (BitMapOut + Index, BitMapIn + Index, sizeof (EFI_HII_RGB_PIXEL));
  }
}


/**
  Convert pixels from EFI_HII_RGB_PIXEL to EFI_GRAPHICS_OUTPUT_BLT_PIXEL style.

  This is a internal function.


  @param  BitMapOut              Pixels in EFI_GRAPHICS_OUTPUT_BLT_PIXEL format.
  @param  BitMapIn               Pixels in EFI_HII_RGB_PIXEL format.
  @param  PixelNum               The number of pixels to be converted.


**/
VOID
CopyRgbToGopPixel (
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BitMapOut,
  IN  EFI_HII_RGB_PIXEL              *BitMapIn,
  IN  UINTN                          PixelNum
  )
{
  UINTN Index;

  ASSERT (BitMapOut != NULL && BitMapIn != NULL);

  for (Index = 0; Index < PixelNum; Index++) {
    CopyMem (BitMapOut + Index, BitMapIn + Index, sizeof (EFI_HII_RGB_PIXEL));
  }
}


/**
  Output pixels in "1 bit per pixel" format to an image.

  This is a internal function.


  @param  Image                  Points to the image which will store the pixels.
  @param  Data                   Stores the value of output pixels, 0 or 1.
  @param  PaletteInfo            PaletteInfo which stores the color of the output
                                 pixels. First entry corresponds to color 0 and
                                 second one to color 1.


**/
VOID
Output1bitPixel (
  IN OUT EFI_IMAGE_INPUT             *Image,
  IN UINT8                           *Data,
  IN EFI_HII_IMAGE_PALETTE_INFO      *PaletteInfo
  )
{
  UINT16                             Xpos;
  UINT16                             Ypos;
  UINTN                              OffsetY;
  UINT8                              Index;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BitMapPtr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      PaletteValue[2];
  EFI_HII_IMAGE_PALETTE_INFO         *Palette;
  UINTN                              PaletteSize;
  UINT8                              Byte;

  ASSERT (Image != NULL && Data != NULL && PaletteInfo != NULL);

  BitMapPtr = Image->Bitmap;

  //
  // First entry corresponds to color 0 and second entry corresponds to color 1.
  //
  PaletteSize = 0;
  CopyMem (&PaletteSize, PaletteInfo, sizeof (UINT16));
  PaletteSize += sizeof (UINT16);
  Palette = AllocateZeroPool (PaletteSize);
  ASSERT (Palette != NULL);
  if (Palette == NULL) {
    return;
  }
  CopyMem (Palette, PaletteInfo, PaletteSize);

  ZeroMem (PaletteValue, sizeof (PaletteValue));
  CopyRgbToGopPixel (&PaletteValue[0], &Palette->PaletteValue[0], 1);
  CopyRgbToGopPixel (&PaletteValue[1], &Palette->PaletteValue[1], 1);
  FreePool (Palette);

  //
  // Convert the pixel from one bit to corresponding color.
  //
  for (Ypos = 0; Ypos < Image->Height; Ypos++) {
    OffsetY = BITMAP_LEN_1_BIT (Image->Width, Ypos);
    //
    // All bits in these bytes are meaningful
    //
    for (Xpos = 0; Xpos < Image->Width / 8; Xpos++) {
      Byte = *(Data + OffsetY + Xpos);
      for (Index = 0; Index < 8; Index++) {
        if ((Byte & (1 << Index)) != 0) {
          BitMapPtr[Ypos * Image->Width + Xpos * 8 + (8 - Index - 1)] = PaletteValue[1];
        } else {
          BitMapPtr[Ypos * Image->Width + Xpos * 8 + (8 - Index - 1)] = PaletteValue[0];
        }
      }
    }

    if (Image->Width % 8 != 0) {
      //
      // Padding bits in this byte should be ignored.
      //
      Byte = *(Data + OffsetY + Xpos);
      for (Index = 0; Index < Image->Width % 8; Index++) {
        if ((Byte & (1 << (8 - Index - 1))) != 0) {
          BitMapPtr[Ypos * Image->Width + Xpos * 8 + Index] = PaletteValue[1];
        } else {
          BitMapPtr[Ypos * Image->Width + Xpos * 8 + Index] = PaletteValue[0];
        }
      }
    }
  }
}


/**
  Output pixels in "4 bit per pixel" format to an image.

  This is a internal function.


  @param  Image                  Points to the image which will store the pixels.
  @param  Data                   Stores the value of output pixels, 0 ~ 15.
  @param[in]  PaletteInfo            PaletteInfo which stores the color of the output
                                 pixels. Each entry corresponds to a color within
                                 [0, 15].


**/
VOID
Output4bitPixel (
  IN OUT EFI_IMAGE_INPUT             *Image,
  IN UINT8                           *Data,
  IN EFI_HII_IMAGE_PALETTE_INFO      *PaletteInfo
  )
{
  UINT16                             Xpos;
  UINT16                             Ypos;
  UINTN                              OffsetY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BitMapPtr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      PaletteValue[16];
  EFI_HII_IMAGE_PALETTE_INFO         *Palette;
  UINTN                              PaletteSize;
  UINT16                             PaletteNum;
  UINT8                              Byte;

  ASSERT (Image != NULL && Data != NULL && PaletteInfo != NULL);

  BitMapPtr = Image->Bitmap;

  //
  // The bitmap should allocate each color index starting from 0.
  //
  PaletteSize = 0;
  CopyMem (&PaletteSize, PaletteInfo, sizeof (UINT16));
  PaletteSize += sizeof (UINT16);
  Palette = AllocateZeroPool (PaletteSize);
  ASSERT (Palette != NULL);
  if (Palette == NULL) {
    return;
  }
  CopyMem (Palette, PaletteInfo, PaletteSize);
  PaletteNum = (UINT16)(Palette->PaletteSize / sizeof (EFI_HII_RGB_PIXEL));

  ZeroMem (PaletteValue, sizeof (PaletteValue));
  CopyRgbToGopPixel (PaletteValue, Palette->PaletteValue, MIN (PaletteNum, ARRAY_SIZE (PaletteValue)));
  FreePool (Palette);

  //
  // Convert the pixel from 4 bit to corresponding color.
  //
  for (Ypos = 0; Ypos < Image->Height; Ypos++) {
    OffsetY = BITMAP_LEN_4_BIT (Image->Width, Ypos);
    //
    // All bits in these bytes are meaningful
    //
    for (Xpos = 0; Xpos < Image->Width / 2; Xpos++) {
      Byte = *(Data + OffsetY + Xpos);
      BitMapPtr[Ypos * Image->Width + Xpos * 2]     = PaletteValue[Byte >> 4];
      BitMapPtr[Ypos * Image->Width + Xpos * 2 + 1] = PaletteValue[Byte & 0x0F];
    }

    if (Image->Width % 2 != 0) {
      //
      // Padding bits in this byte should be ignored.
      //
      Byte = *(Data + OffsetY + Xpos);
      BitMapPtr[Ypos * Image->Width + Xpos * 2]     = PaletteValue[Byte >> 4];
    }
  }
}


/**
  Output pixels in "8 bit per pixel" format to an image.

  This is a internal function.


  @param  Image                  Points to the image which will store the pixels.
  @param  Data                   Stores the value of output pixels, 0 ~ 255.
  @param[in]  PaletteInfo        PaletteInfo which stores the color of the output
                                 pixels. Each entry corresponds to a color within
                                 [0, 255].


**/
VOID
Output8bitPixel (
  IN OUT EFI_IMAGE_INPUT             *Image,
  IN UINT8                           *Data,
  IN EFI_HII_IMAGE_PALETTE_INFO      *PaletteInfo
  )
{
  UINT16                             Xpos;
  UINT16                             Ypos;
  UINTN                              OffsetY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BitMapPtr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      PaletteValue[256];
  EFI_HII_IMAGE_PALETTE_INFO         *Palette;
  UINTN                              PaletteSize;
  UINT16                             PaletteNum;
  UINT8                              Byte;

  ASSERT (Image != NULL && Data != NULL && PaletteInfo != NULL);

  BitMapPtr = Image->Bitmap;

  //
  // The bitmap should allocate each color index starting from 0.
  //
  PaletteSize = 0;
  CopyMem (&PaletteSize, PaletteInfo, sizeof (UINT16));
  PaletteSize += sizeof (UINT16);
  Palette = AllocateZeroPool (PaletteSize);
  ASSERT (Palette != NULL);
  if (Palette == NULL) {
    return;
  }
  CopyMem (Palette, PaletteInfo, PaletteSize);
  PaletteNum = (UINT16)(Palette->PaletteSize / sizeof (EFI_HII_RGB_PIXEL));
  ZeroMem (PaletteValue, sizeof (PaletteValue));
  CopyRgbToGopPixel (PaletteValue, Palette->PaletteValue, MIN (PaletteNum, ARRAY_SIZE (PaletteValue)));
  FreePool (Palette);

  //
  // Convert the pixel from 8 bits to corresponding color.
  //
  for (Ypos = 0; Ypos < Image->Height; Ypos++) {
    OffsetY = BITMAP_LEN_8_BIT ((UINT32) Image->Width, Ypos);
    //
    // All bits are meaningful since the bitmap is 8 bits per pixel.
    //
    for (Xpos = 0; Xpos < Image->Width; Xpos++) {
      Byte = *(Data + OffsetY + Xpos);
      BitMapPtr[OffsetY + Xpos] = PaletteValue[Byte];
    }
  }

}


/**
  Output pixels in "24 bit per pixel" format to an image.

  This is a internal function.


  @param  Image                  Points to the image which will store the pixels.
  @param  Data                   Stores the color of output pixels, allowing 16.8
                                 millions colors.


**/
VOID
Output24bitPixel (
  IN OUT EFI_IMAGE_INPUT             *Image,
  IN EFI_HII_RGB_PIXEL               *Data
  )
{
  UINT16                             Ypos;
  UINTN                              OffsetY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BitMapPtr;

  ASSERT (Image != NULL && Data != NULL);

  BitMapPtr = Image->Bitmap;

  for (Ypos = 0; Ypos < Image->Height; Ypos++) {
    OffsetY = BITMAP_LEN_8_BIT ((UINT32) Image->Width, Ypos);
    CopyRgbToGopPixel (&BitMapPtr[OffsetY], &Data[OffsetY], Image->Width);
  }

}


/**
  Convert the image from EFI_IMAGE_INPUT to EFI_IMAGE_OUTPUT format.

  This is a internal function.


  @param  BltBuffer              Buffer points to bitmap data of incoming image.
  @param  BltX                   Specifies the offset from the left and top edge of
                                  the output image of the first pixel in the image.
  @param  BltY                   Specifies the offset from the left and top edge of
                                  the output image of the first pixel in the image.
  @param  Width                  Width of the incoming image, in pixels.
  @param  Height                 Height of the incoming image, in pixels.
  @param  Transparent            If TRUE, all "off" pixels in the image will be
                                 drawn using the pixel value from blt and all other
                                 pixels will be copied.
  @param  Blt                    Buffer points to bitmap data of output image.

  @retval EFI_SUCCESS            The image was successfully converted.
  @retval EFI_INVALID_PARAMETER  Any incoming parameter is invalid.

**/
EFI_STATUS
ImageToBlt (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN UINTN                           BltX,
  IN UINTN                           BltY,
  IN UINTN                           Width,
  IN UINTN                           Height,
  IN BOOLEAN                         Transparent,
  IN OUT EFI_IMAGE_OUTPUT            **Blt
  )
{
  EFI_IMAGE_OUTPUT                   *ImageOut;
  UINTN                              Xpos;
  UINTN                              Ypos;
  UINTN                              OffsetY1; // src buffer
  UINTN                              OffsetY2; // dest buffer
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      SrcPixel;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      ZeroPixel;

  if (BltBuffer == NULL || Blt == NULL || *Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ImageOut = *Blt;

  if (Width + BltX > ImageOut->Width) {
    return EFI_INVALID_PARAMETER;
  }
  if (Height + BltY > ImageOut->Height) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ZeroPixel, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  for (Ypos = 0; Ypos < Height; Ypos++) {
    OffsetY1 = Width * Ypos;
    OffsetY2 = ImageOut->Width * (BltY + Ypos);
    for (Xpos = 0; Xpos < Width; Xpos++) {
      SrcPixel = BltBuffer[OffsetY1 + Xpos];
      if (Transparent) {
        if (CompareMem (&SrcPixel, &ZeroPixel, 3) != 0) {
          ImageOut->Image.Bitmap[OffsetY2 + BltX + Xpos] = SrcPixel;
        }
      } else {
        ImageOut->Image.Bitmap[OffsetY2 + BltX + Xpos] = SrcPixel;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Return the HII package list identified by PackageList HII handle.

  @param Database    Pointer to HII database list header.
  @param PackageList HII handle of the package list to locate.

  @retval The HII package list instance.
**/
HII_DATABASE_PACKAGE_LIST_INSTANCE *
LocatePackageList (
  IN  LIST_ENTRY                     *Database,
  IN  EFI_HII_HANDLE                 PackageList
  )
{
  LIST_ENTRY                         *Link;
  HII_DATABASE_RECORD                *Record;

  //
  // Get the specified package list and image package.
  //
  for (Link = GetFirstNode (Database);
       !IsNull (Database, Link);
       Link = GetNextNode (Database, Link)
      ) {
    Record = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if (Record->Handle == PackageList) {
      return Record->PackageList;
    }
  }
  return NULL;
}

/**
  This function adds the image Image to the group of images owned by PackageList, and returns
  a new image identifier (ImageId).

  @param  This                   A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  PackageList            Handle of the package list where this image will
                                 be added.
  @param  ImageId                On return, contains the new image id, which is
                                 unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was added successfully.
  @retval EFI_NOT_FOUND          The specified PackageList could not be found in
                                 database.
  @retval EFI_OUT_OF_RESOURCES   Could not add the image due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Image is NULL or ImageId is NULL.

**/
EFI_STATUS
EFIAPI
HiiNewImage (
  IN  CONST EFI_HII_IMAGE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                 PackageList,
  OUT EFI_IMAGE_ID                   *ImageId,
  IN  CONST EFI_IMAGE_INPUT          *Image
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IMAGE_PACKAGE_INSTANCE          *ImagePackage;
  EFI_HII_IMAGE_BLOCK                 *ImageBlocks;
  UINT32                              NewBlockSize;

  if (This == NULL || ImageId == NULL || Image == NULL || Image->Bitmap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  PackageListNode = LocatePackageList (&Private->DatabaseList, PackageList);
  if (PackageListNode == NULL) {
    return EFI_NOT_FOUND;
  }

  EfiAcquireLock (&mHiiDatabaseLock);

  //
  // Calcuate the size of new image.
  // Make sure the size doesn't overflow UINT32.
  // Note: 24Bit BMP occpuies 3 bytes per pixel.
  //
  NewBlockSize = (UINT32)Image->Width * Image->Height;
  if (NewBlockSize > (MAX_UINT32 - (sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL))) / 3) {
    EfiReleaseLock (&mHiiDatabaseLock);
    return EFI_OUT_OF_RESOURCES;
  }
  NewBlockSize = NewBlockSize * 3 + (sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL));

  //
  // Get the image package in the package list,
  // or create a new image package if image package does not exist.
  //
  if (PackageListNode->ImagePkg != NULL) {
    ImagePackage = PackageListNode->ImagePkg;

    //
    // Output the image id of the incoming image being inserted, which is the
    // image id of the EFI_HII_IIBT_END block of old image package.
    //
    *ImageId = 0;
    GetImageIdOrAddress (ImagePackage->ImageBlock, ImageId);

    //
    // Update the package's image block by appending the new block to the end.
    //

    //
    // Make sure the final package length doesn't overflow.
    // Length of the package header is represented using 24 bits. So MAX length is MAX_UINT24.
    //
    if (NewBlockSize > MAX_UINT24 - ImagePackage->ImagePkgHdr.Header.Length) {
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Because ImagePackage->ImageBlockSize < ImagePackage->ImagePkgHdr.Header.Length,
    // So (ImagePackage->ImageBlockSize + NewBlockSize) <= MAX_UINT24
    //
    ImageBlocks = AllocatePool (ImagePackage->ImageBlockSize + NewBlockSize);
    if (ImageBlocks == NULL) {
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Copy the original content.
    //
    CopyMem (
      ImageBlocks,
      ImagePackage->ImageBlock,
      ImagePackage->ImageBlockSize - sizeof (EFI_HII_IIBT_END_BLOCK)
      );
    FreePool (ImagePackage->ImageBlock);
    ImagePackage->ImageBlock = ImageBlocks;

    //
    // Point to the very last block.
    //
    ImageBlocks = (EFI_HII_IMAGE_BLOCK *) (
                    (UINT8 *) ImageBlocks + ImagePackage->ImageBlockSize - sizeof (EFI_HII_IIBT_END_BLOCK)
                    );
    //
    // Update the length record.
    //
    ImagePackage->ImageBlockSize                  += NewBlockSize;
    ImagePackage->ImagePkgHdr.Header.Length       += NewBlockSize;
    PackageListNode->PackageListHdr.PackageLength += NewBlockSize;

  } else {
    //
    // Make sure the final package length doesn't overflow.
    // Length of the package header is represented using 24 bits. So MAX length is MAX_UINT24.
    //
    if (NewBlockSize > MAX_UINT24 - (sizeof (EFI_HII_IMAGE_PACKAGE_HDR) + sizeof (EFI_HII_IIBT_END_BLOCK))) {
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // The specified package list does not contain image package.
    // Create one to add this image block.
    //
    ImagePackage = (HII_IMAGE_PACKAGE_INSTANCE *) AllocateZeroPool (sizeof (HII_IMAGE_PACKAGE_INSTANCE));
    if (ImagePackage == NULL) {
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Output the image id of the incoming image being inserted, which is the
    // first image block so that id is initially to one.
    //
    *ImageId = 1;
    //
    // Fill in image package header.
    //
    ImagePackage->ImagePkgHdr.Header.Length     = sizeof (EFI_HII_IMAGE_PACKAGE_HDR) + NewBlockSize + sizeof (EFI_HII_IIBT_END_BLOCK);
    ImagePackage->ImagePkgHdr.Header.Type       = EFI_HII_PACKAGE_IMAGES;
    ImagePackage->ImagePkgHdr.ImageInfoOffset   = sizeof (EFI_HII_IMAGE_PACKAGE_HDR);
    ImagePackage->ImagePkgHdr.PaletteInfoOffset = 0;

    //
    // Fill in palette info.
    //
    ImagePackage->PaletteBlock    = NULL;
    ImagePackage->PaletteInfoSize = 0;

    //
    // Fill in image blocks.
    //
    ImagePackage->ImageBlockSize = NewBlockSize + sizeof (EFI_HII_IIBT_END_BLOCK);
    ImagePackage->ImageBlock = AllocateZeroPool (NewBlockSize + sizeof (EFI_HII_IIBT_END_BLOCK));
    if (ImagePackage->ImageBlock == NULL) {
      FreePool (ImagePackage);
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    ImageBlocks = ImagePackage->ImageBlock;

    //
    // Insert this image package.
    //
    PackageListNode->ImagePkg = ImagePackage;
    PackageListNode->PackageListHdr.PackageLength += ImagePackage->ImagePkgHdr.Header.Length;
  }

  //
  // Append the new block here
  //
  if (Image->Flags == EFI_IMAGE_TRANSPARENT) {
    ImageBlocks->BlockType = EFI_HII_IIBT_IMAGE_24BIT_TRANS;
  } else {
    ImageBlocks->BlockType = EFI_HII_IIBT_IMAGE_24BIT;
  }
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) ImageBlocks)->Bitmap.Width, Image->Width);
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) ImageBlocks)->Bitmap.Height, Image->Height);
  CopyGopToRgbPixel (((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) ImageBlocks)->Bitmap.Bitmap, Image->Bitmap, (UINT32) Image->Width * Image->Height);

  //
  // Append the block end
  //
  ImageBlocks = (EFI_HII_IMAGE_BLOCK *) ((UINT8 *) ImageBlocks + NewBlockSize);
  ImageBlocks->BlockType = EFI_HII_IIBT_END;

  //
  // Check whether need to get the contents of HiiDataBase.
  // Only after ReadyToBoot to do the export.
  //
  if (gExportAfterReadyToBoot) {
    HiiGetDatabaseInfo(&Private->HiiDatabase);
  }

  EfiReleaseLock (&mHiiDatabaseLock);

  return EFI_SUCCESS;
}


/**
  This function retrieves the image specified by ImageId which is associated with
  the specified PackageList and copies it into the buffer specified by Image.

  @param  Database               A pointer to the database list header.
  @param  PackageList            Handle of the package list where this image will
                                 be searched.
  @param  ImageId                The image's id,, which is unique within
                                 PackageList.
  @param  Image                  Points to the image.
  @param  BitmapOnly             TRUE to only return the bitmap type image.
                                 FALSE to locate image decoder instance to decode image.

  @retval EFI_SUCCESS            The new image was returned successfully.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the
                                 database. The specified PackageList is not in the database.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to
                                 hold the image.
  @retval EFI_INVALID_PARAMETER  The Image or ImageSize was NULL.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there was not
                                 enough memory.
**/
EFI_STATUS
IGetImage (
  IN  LIST_ENTRY                     *Database,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_IMAGE_ID                   ImageId,
  OUT EFI_IMAGE_INPUT                *Image,
  IN  BOOLEAN                        BitmapOnly
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IMAGE_PACKAGE_INSTANCE          *ImagePackage;
  EFI_HII_IMAGE_BLOCK                 *CurrentImageBlock;
  EFI_HII_IIBT_IMAGE_1BIT_BLOCK       Iibt1bit;
  UINT16                              Width;
  UINT16                              Height;
  UINTN                               ImageLength;
  UINT8                               *PaletteInfo;
  UINT8                               PaletteIndex;
  UINT16                              PaletteSize;
  EFI_HII_IMAGE_DECODER_PROTOCOL      *Decoder;
  EFI_IMAGE_OUTPUT                    *ImageOut;

  if (Image == NULL || ImageId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PackageListNode = LocatePackageList (Database, PackageList);
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

  Image->Flags = 0;
  switch (CurrentImageBlock->BlockType) {
  case EFI_HII_IIBT_IMAGE_JPEG:
  case EFI_HII_IIBT_IMAGE_PNG:
    if (BitmapOnly) {
      return EFI_UNSUPPORTED;
    }

    ImageOut = NULL;
    Decoder = LocateHiiImageDecoder (CurrentImageBlock->BlockType);
    if (Decoder == NULL) {
      return EFI_UNSUPPORTED;
    }
    //
    // Use the common block code since the definition of two structures is the same.
    //
    ASSERT (OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Data) == OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Data));
    ASSERT (sizeof (((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Data) ==
            sizeof (((EFI_HII_IIBT_PNG_BLOCK *) CurrentImageBlock)->Data));
    ASSERT (OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Size) == OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Size));
    ASSERT (sizeof (((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Size) ==
            sizeof (((EFI_HII_IIBT_PNG_BLOCK *) CurrentImageBlock)->Size));
    Status = Decoder->DecodeImage (
      Decoder,
      ((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Data,
      ((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Size,
      &ImageOut,
      FALSE
    );

    //
    // Spec requires to use the first capable image decoder instance.
    // The first image decoder instance may fail to decode the image.
    //
    if (!EFI_ERROR (Status)) {
      Image->Bitmap = ImageOut->Image.Bitmap;
      Image->Height = ImageOut->Height;
      Image->Width = ImageOut->Width;
      FreePool (ImageOut);
    }
    return Status;

  case EFI_HII_IIBT_IMAGE_1BIT_TRANS:
  case EFI_HII_IIBT_IMAGE_4BIT_TRANS:
  case EFI_HII_IIBT_IMAGE_8BIT_TRANS:
    Image->Flags = EFI_IMAGE_TRANSPARENT;
    //
    // fall through
    //
  case EFI_HII_IIBT_IMAGE_1BIT:
  case EFI_HII_IIBT_IMAGE_4BIT:
  case EFI_HII_IIBT_IMAGE_8BIT:
    //
    // Use the common block code since the definition of these structures is the same.
    //
    CopyMem (&Iibt1bit, CurrentImageBlock, sizeof (EFI_HII_IIBT_IMAGE_1BIT_BLOCK));
    ImageLength = (UINTN) Iibt1bit.Bitmap.Width * Iibt1bit.Bitmap.Height;
    if (ImageLength > MAX_UINTN / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) {
      return EFI_OUT_OF_RESOURCES;
    }
    ImageLength  *= sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    Image->Bitmap = AllocateZeroPool (ImageLength);
    if (Image->Bitmap == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Image->Width  = Iibt1bit.Bitmap.Width;
    Image->Height = Iibt1bit.Bitmap.Height;

    PaletteInfo = ImagePackage->PaletteBlock + sizeof (EFI_HII_IMAGE_PALETTE_INFO_HEADER);
    for (PaletteIndex = 1; PaletteIndex < Iibt1bit.PaletteIndex; PaletteIndex++) {
      CopyMem (&PaletteSize, PaletteInfo, sizeof (UINT16));
      PaletteInfo += PaletteSize + sizeof (UINT16);
    }
    ASSERT (PaletteIndex == Iibt1bit.PaletteIndex);

    //
    // Output bitmap data
    //
    if (CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_1BIT ||
        CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_1BIT_TRANS) {
      Output1bitPixel (
        Image,
        ((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Data,
        (EFI_HII_IMAGE_PALETTE_INFO *) PaletteInfo
        );
    } else if (CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_4BIT ||
               CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_4BIT_TRANS) {
      Output4bitPixel (
        Image,
        ((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Data,
        (EFI_HII_IMAGE_PALETTE_INFO *) PaletteInfo
        );
    } else {
      Output8bitPixel (
        Image,
        ((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Data,
        (EFI_HII_IMAGE_PALETTE_INFO *) PaletteInfo
        );
    }

    return EFI_SUCCESS;

  case EFI_HII_IIBT_IMAGE_24BIT_TRANS:
    Image->Flags = EFI_IMAGE_TRANSPARENT;
    //
    // fall through
    //
  case EFI_HII_IIBT_IMAGE_24BIT:
    Width  = ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width);
    Height = ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height);
    ImageLength = (UINTN)Width * Height;
    if (ImageLength > MAX_UINTN / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) {
      return EFI_OUT_OF_RESOURCES;
    }
    ImageLength  *= sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    Image->Bitmap = AllocateZeroPool (ImageLength);
    if (Image->Bitmap == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Image->Width  = Width;
    Image->Height = Height;

    //
    // Output the bitmap data directly.
    //
    Output24bitPixel (
      Image,
      ((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Bitmap
      );
    return EFI_SUCCESS;

  default:
    return EFI_NOT_FOUND;
  }
}

/**
  This function retrieves the image specified by ImageId which is associated with
  the specified PackageList and copies it into the buffer specified by Image.

  @param  This                   A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  PackageList            Handle of the package list where this image will
                                 be searched.
  @param  ImageId                The image's id,, which is unique within
                                 PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was returned successfully.
  @retval EFI_NOT_FOUND           The image specified by ImageId is not in the
                                                database. The specified PackageList is not in the database.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to
                                 hold the image.
  @retval EFI_INVALID_PARAMETER  The Image or ImageSize was NULL.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there was not
                                 enough memory.

**/
EFI_STATUS
EFIAPI
HiiGetImage (
  IN  CONST EFI_HII_IMAGE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_IMAGE_ID                   ImageId,
  OUT EFI_IMAGE_INPUT                *Image
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  Private = HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  return IGetImage (&Private->DatabaseList, PackageList, ImageId, Image, TRUE);
}


/**
  This function updates the image specified by ImageId in the specified PackageListHandle to
  the image specified by Image.

  @param  This                   A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  PackageList            The package list containing the images.
  @param  ImageId                The image's id,, which is unique within
                                 PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was updated successfully.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the
                                                database. The specified PackageList is not in the database.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.

**/
EFI_STATUS
EFIAPI
HiiSetImage (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN CONST EFI_IMAGE_INPUT           *Image
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IMAGE_PACKAGE_INSTANCE          *ImagePackage;
  EFI_HII_IMAGE_BLOCK                 *CurrentImageBlock;
  EFI_HII_IMAGE_BLOCK                 *ImageBlocks;
  EFI_HII_IMAGE_BLOCK                 *NewImageBlock;
  UINT32                              NewBlockSize;
  UINT32                              OldBlockSize;
  UINT32                               Part1Size;
  UINT32                               Part2Size;

  if (This == NULL || Image == NULL || ImageId == 0 || Image->Bitmap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS (This);
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

  EfiAcquireLock (&mHiiDatabaseLock);

  //
  // Get the size of original image block. Use some common block code here
  // since the definition of some structures is the same.
  //
  switch (CurrentImageBlock->BlockType) {
  case EFI_HII_IIBT_IMAGE_JPEG:
    OldBlockSize = OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Data) + ReadUnaligned32 ((VOID *) &((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Size);
    break;
  case EFI_HII_IIBT_IMAGE_PNG:
    OldBlockSize = OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Data) + ReadUnaligned32 ((VOID *) &((EFI_HII_IIBT_PNG_BLOCK *) CurrentImageBlock)->Size);
    break;
  case EFI_HII_IIBT_IMAGE_1BIT:
  case EFI_HII_IIBT_IMAGE_1BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_1BIT_BLOCK) - sizeof (UINT8) +
                   BITMAP_LEN_1_BIT (
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  case EFI_HII_IIBT_IMAGE_4BIT:
  case EFI_HII_IIBT_IMAGE_4BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_4BIT_BLOCK) - sizeof (UINT8) +
                   BITMAP_LEN_4_BIT (
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  case EFI_HII_IIBT_IMAGE_8BIT:
  case EFI_HII_IIBT_IMAGE_8BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_8BIT_BLOCK) - sizeof (UINT8) +
                   BITMAP_LEN_8_BIT (
                     (UINT32) ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  case EFI_HII_IIBT_IMAGE_24BIT:
  case EFI_HII_IIBT_IMAGE_24BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL) +
                   BITMAP_LEN_24_BIT (
                     (UINT32) ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  default:
    EfiReleaseLock (&mHiiDatabaseLock);
    return EFI_NOT_FOUND;
  }

  //
  // Create the new image block according to input image.
  //

  //
  // Make sure the final package length doesn't overflow.
  // Length of the package header is represented using 24 bits. So MAX length is MAX_UINT24.
  // 24Bit BMP occpuies 3 bytes per pixel.
  //
  NewBlockSize = (UINT32)Image->Width * Image->Height;
  if (NewBlockSize > (MAX_UINT32 - (sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL))) / 3) {
    EfiReleaseLock (&mHiiDatabaseLock);
    return EFI_OUT_OF_RESOURCES;
  }
  NewBlockSize = NewBlockSize * 3 + (sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL));
  if ((NewBlockSize > OldBlockSize) &&
      (NewBlockSize - OldBlockSize > MAX_UINT24 - ImagePackage->ImagePkgHdr.Header.Length)
      ) {
    EfiReleaseLock (&mHiiDatabaseLock);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Adjust the image package to remove the original block firstly then add the new block.
  //
  ImageBlocks = AllocateZeroPool (ImagePackage->ImageBlockSize + NewBlockSize - OldBlockSize);
  if (ImageBlocks == NULL) {
    EfiReleaseLock (&mHiiDatabaseLock);
    return EFI_OUT_OF_RESOURCES;
  }

  Part1Size = (UINT32) ((UINTN) CurrentImageBlock - (UINTN) ImagePackage->ImageBlock);
  Part2Size = ImagePackage->ImageBlockSize - Part1Size - OldBlockSize;
  CopyMem (ImageBlocks, ImagePackage->ImageBlock, Part1Size);

  //
  // Set the new image block
  //
  NewImageBlock = (EFI_HII_IMAGE_BLOCK *) ((UINT8 *) ImageBlocks + Part1Size);
  if ((Image->Flags & EFI_IMAGE_TRANSPARENT) == EFI_IMAGE_TRANSPARENT) {
    NewImageBlock->BlockType= EFI_HII_IIBT_IMAGE_24BIT_TRANS;
  } else {
    NewImageBlock->BlockType = EFI_HII_IIBT_IMAGE_24BIT;
  }
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) NewImageBlock)->Bitmap.Width, Image->Width);
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) NewImageBlock)->Bitmap.Height, Image->Height);
  CopyGopToRgbPixel (((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) NewImageBlock)->Bitmap.Bitmap,
                       Image->Bitmap, (UINT32) Image->Width * Image->Height);

  CopyMem ((UINT8 *) NewImageBlock + NewBlockSize, (UINT8 *) CurrentImageBlock + OldBlockSize, Part2Size);

  FreePool (ImagePackage->ImageBlock);
  ImagePackage->ImageBlock                       = ImageBlocks;
  ImagePackage->ImageBlockSize                  += NewBlockSize - OldBlockSize;
  ImagePackage->ImagePkgHdr.Header.Length       += NewBlockSize - OldBlockSize;
  PackageListNode->PackageListHdr.PackageLength += NewBlockSize - OldBlockSize;

  //
  // Check whether need to get the contents of HiiDataBase.
  // Only after ReadyToBoot to do the export.
  //
  if (gExportAfterReadyToBoot) {
    HiiGetDatabaseInfo(&Private->HiiDatabase);
  }

  EfiReleaseLock (&mHiiDatabaseLock);
  return EFI_SUCCESS;

}


/**
  This function renders an image to a bitmap or the screen using the specified
  color and options. It draws the image on an existing bitmap, allocates a new
  bitmap or uses the screen. The images can be clipped.

  @param  This                   A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  Flags                  Describes how the image is to be drawn.
  @param  Image                  Points to the image to be displayed.
  @param  Blt                    If this points to a non-NULL on entry, this points
                                 to the image, which is Width pixels wide and
                                 Height pixels high.  The image will be drawn onto
                                 this image and  EFI_HII_DRAW_FLAG_CLIP is implied.
                                 If this points to a  NULL on entry, then a buffer
                                 will be allocated to hold  the generated image and
                                 the pointer updated on exit. It is the caller's
                                 responsibility to free this buffer.
  @param  BltX                   Specifies the offset from the left and top edge of
                                 the  output image of the first pixel in the image.
  @param  BltY                   Specifies the offset from the left and top edge of
                                 the  output image of the first pixel in the image.

  @retval EFI_SUCCESS            The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES   Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER  The Image or Blt was NULL.
  @retval EFI_INVALID_PARAMETER  Any combination of Flags is invalid.

**/
EFI_STATUS
EFIAPI
HiiDrawImage (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN CONST EFI_IMAGE_INPUT           *Image,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PRIVATE_DATA           *Private;
  BOOLEAN                             Transparent;
  EFI_IMAGE_OUTPUT                    *ImageOut;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BltBuffer;
  UINTN                               BufferLen;
  UINT16                              Width;
  UINT16                              Height;
  UINTN                               Xpos;
  UINTN                               Ypos;
  UINTN                               OffsetY1;
  UINTN                               OffsetY2;
  EFI_FONT_DISPLAY_INFO               *FontInfo;
  UINTN                               Index;

  if (This == NULL || Image == NULL || Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Flags & EFI_HII_DRAW_FLAG_CLIP) == EFI_HII_DRAW_FLAG_CLIP && *Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Flags & EFI_HII_DRAW_FLAG_TRANSPARENT) == EFI_HII_DRAW_FLAG_TRANSPARENT) {
    return EFI_INVALID_PARAMETER;
  }

  FontInfo = NULL;

  //
  // Check whether the image will be drawn transparently or opaquely.
  //
  Transparent = FALSE;
  if ((Flags & EFI_HII_DRAW_FLAG_TRANSPARENT) == EFI_HII_DRAW_FLAG_FORCE_TRANS) {
    Transparent = TRUE;
  } else if ((Flags & EFI_HII_DRAW_FLAG_TRANSPARENT) == EFI_HII_DRAW_FLAG_FORCE_OPAQUE){
    Transparent = FALSE;
  } else {
    //
    // Now EFI_HII_DRAW_FLAG_DEFAULT is set, whether image will be drawn depending
    // on the image's transparency setting.
    //
    if ((Image->Flags & EFI_IMAGE_TRANSPARENT) == EFI_IMAGE_TRANSPARENT) {
      Transparent = TRUE;
    }
  }

  //
  // Image cannot be drawn transparently if Blt points to NULL on entry.
  // Currently output to Screen transparently is not supported, either.
  //
  if (Transparent) {
    if (*Blt == NULL) {
      return EFI_INVALID_PARAMETER;
    } else if ((Flags & EFI_HII_DIRECT_TO_SCREEN) == EFI_HII_DIRECT_TO_SCREEN) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Private = HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  //
  // When Blt points to a non-NULL on entry, this image will be drawn onto
  // this bitmap or screen pointed by "*Blt" and EFI_HII_DRAW_FLAG_CLIP is implied.
  // Otherwise a new bitmap will be allocated to hold this image.
  //
  if (*Blt != NULL) {
    //
    // Make sure the BltX and BltY is inside the Blt area.
    //
    if ((BltX >= (*Blt)->Width) || (BltY >= (*Blt)->Height)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Clip the image by (Width, Height)
    //

    Width  = Image->Width;
    Height = Image->Height;

    if (Width > (*Blt)->Width - (UINT16)BltX) {
      Width = (*Blt)->Width - (UINT16)BltX;
    }
    if (Height > (*Blt)->Height - (UINT16)BltY) {
      Height = (*Blt)->Height - (UINT16)BltY;
    }

    //
    // Prepare the buffer for the temporary image.
    // Make sure the buffer size doesn't overflow UINTN.
    //
    BufferLen = Width * Height;
    if (BufferLen > MAX_UINTN / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) {
      return EFI_OUT_OF_RESOURCES;
    }
    BufferLen *= sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    BltBuffer  = AllocateZeroPool (BufferLen);
    if (BltBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (Width == Image->Width && Height == Image->Height) {
      CopyMem (BltBuffer, Image->Bitmap, BufferLen);
    } else {
      for (Ypos = 0; Ypos < Height; Ypos++) {
        OffsetY1 = Image->Width * Ypos;
        OffsetY2 = Width * Ypos;
        for (Xpos = 0; Xpos < Width; Xpos++) {
          BltBuffer[OffsetY2 + Xpos] = Image->Bitmap[OffsetY1 + Xpos];
        }
      }
    }

    //
    // Draw the image to existing bitmap or screen depending on flag.
    //
    if ((Flags & EFI_HII_DIRECT_TO_SCREEN) == EFI_HII_DIRECT_TO_SCREEN) {
      //
      // Caller should make sure the current UGA console is grarphic mode.
      //

      //
      // Write the image directly to the output device specified by Screen.
      //
      Status = (*Blt)->Image.Screen->Blt (
                                       (*Blt)->Image.Screen,
                                       BltBuffer,
                                       EfiBltBufferToVideo,
                                       0,
                                       0,
                                       BltX,
                                       BltY,
                                       Width,
                                       Height,
                                       0
                                       );
    } else {
      //
      // Draw the image onto the existing bitmap specified by Bitmap.
      //
      Status = ImageToBlt (
                 BltBuffer,
                 BltX,
                 BltY,
                 Width,
                 Height,
                 Transparent,
                 Blt
                 );

    }

    FreePool (BltBuffer);
    return Status;

  } else {
    //
    // Allocate a new bitmap to hold the incoming image.
    //

    //
    // Make sure the final width and height doesn't overflow UINT16.
    //
    if ((BltX > (UINTN)MAX_UINT16 - Image->Width) || (BltY > (UINTN)MAX_UINT16 - Image->Height)) {
      return EFI_INVALID_PARAMETER;
    }

    Width  = Image->Width  + (UINT16)BltX;
    Height = Image->Height + (UINT16)BltY;

    //
    // Make sure the output image size doesn't overflow UINTN.
    //
    BufferLen = Width * Height;
    if (BufferLen > MAX_UINTN / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) {
      return EFI_OUT_OF_RESOURCES;
    }
    BufferLen *= sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    BltBuffer  = AllocateZeroPool (BufferLen);
    if (BltBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ImageOut = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
    if (ImageOut == NULL) {
      FreePool (BltBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
    ImageOut->Width        = Width;
    ImageOut->Height       = Height;
    ImageOut->Image.Bitmap = BltBuffer;

    //
    // BUGBUG: Now all the "blank" pixels are filled with system default background
    // color. Not sure if it need to be updated or not.
    //
    Status = GetSystemFont (Private, &FontInfo, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (BltBuffer);
      FreePool (ImageOut);
      return Status;
    }
    ASSERT (FontInfo != NULL);
    for (Index = 0; Index < (UINTN)Width * Height; Index++) {
      BltBuffer[Index] = FontInfo->BackgroundColor;
    }
    FreePool (FontInfo);

    //
    // Draw the incoming image to the new created image.
    //
    *Blt = ImageOut;
    return ImageToBlt (
             Image->Bitmap,
             BltX,
             BltY,
             Image->Width,
             Image->Height,
             Transparent,
             Blt
             );

  }
}


/**
  This function renders an image to a bitmap or the screen using the specified
  color and options. It draws the image on an existing bitmap, allocates a new
  bitmap or uses the screen. The images can be clipped.

  @param  This                   A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  Flags                  Describes how the image is to be drawn.
  @param  PackageList            The package list in the HII database to search for
                                 the  specified image.
  @param  ImageId                The image's id, which is unique within
                                 PackageList.
  @param  Blt                    If this points to a non-NULL on entry, this points
                                 to the image, which is Width pixels wide and
                                 Height pixels high. The image will be drawn onto
                                 this image and
                                 EFI_HII_DRAW_FLAG_CLIP is implied. If this points
                                 to a  NULL on entry, then a buffer will be
                                 allocated to hold  the generated image and the
                                 pointer updated on exit. It is the caller's
                                 responsibility to free this buffer.
  @param  BltX                   Specifies the offset from the left and top edge of
                                 the  output image of the first pixel in the image.
  @param  BltY                   Specifies the offset from the left and top edge of
                                 the  output image of the first pixel in the image.

  @retval EFI_SUCCESS            The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES   Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER  The Blt was NULL.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the database.
                           The specified PackageList is not in the database.

**/
EFI_STATUS
EFIAPI
HiiDrawImageId (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  )
{
  EFI_STATUS                          Status;
  EFI_IMAGE_INPUT                     Image;

  //
  // Check input parameter.
  //
  if (This == NULL || Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the specified Image.
  //
  Status = HiiGetImage (This, PackageList, ImageId, &Image);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Draw this image.
  //
  Status = HiiDrawImage (This, Flags, &Image, Blt, BltX, BltY);
  if (Image.Bitmap != NULL) {
    FreePool (Image.Bitmap);
  }
  return Status;
}


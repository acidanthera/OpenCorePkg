/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <IndustryStandard/AppleIcon.h>
#include <IndustryStandard/AppleDiskLabel.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/OcPngLib.h>

#include "OpenCanopy.h"

//
// Disk label palette.
//
STATIC
CONST UINT8
gAppleDiskLabelImagePalette[256] = {
  [0x00] = 255,
  [0xf6] = 238,
  [0xf7] = 221,
  [0x2a] = 204,
  [0xf8] = 187,
  [0xf9] = 170,
  [0x55] = 153,
  [0xfa] = 136,
  [0xfb] = 119,
  [0x80] = 102,
  [0xfc] = 85,
  [0xfd] = 68,
  [0xab] = 51,
  [0xfe] = 34,
  [0xff] = 17,
  [0xd6] = 0
};

EFI_STATUS
GuiIcnsToImageIcon (
  OUT GUI_IMAGE  *Image,
  IN  VOID       *IcnsImage,
  IN  UINT32     IcnsImageSize,
  IN  UINT8      Scale,
  IN  UINT32     MatchWidth,
  IN  UINT32     MatchHeight,
  IN  BOOLEAN    AllowLess
  )
{
  EFI_STATUS         Status;
  UINT32             Offset;
  UINT32             RecordLength;
  UINT32             ImageSize;
  UINT32             DecodedBytes;
  APPLE_ICNS_RECORD  *Record;
  APPLE_ICNS_RECORD  *RecordIT32;
  APPLE_ICNS_RECORD  *RecordT8MK;

  ASSERT (Scale == 1 || Scale == 2);

  //
  // We do not need to support 'it32' 128x128 icon format,
  // because Finder automatically converts the icons to PNG-based
  // when assigning volume icon.
  //

  if (IcnsImageSize < sizeof (APPLE_ICNS_RECORD)*2) {
    return EFI_INVALID_PARAMETER;
  }

  Record = IcnsImage;
  if (Record->Type != APPLE_ICNS_MAGIC || SwapBytes32 (Record->Size) != IcnsImageSize) {
    return EFI_SECURITY_VIOLATION;
  }

  RecordIT32 = NULL;
  RecordT8MK = NULL;

  Offset  = sizeof (APPLE_ICNS_RECORD);
  while (Offset < IcnsImageSize - sizeof (APPLE_ICNS_RECORD)) {
    Record       = (APPLE_ICNS_RECORD *) ((UINT8 *) IcnsImage + Offset);
    RecordLength = SwapBytes32 (Record->Size);

    //
    // 1. Record smaller than its header and 1 32-bit word is invalid.
    //    32-bit is required by some entries like IT32 (see below).
    // 2. Record overflowing UINT32 is invalid.
    // 3. Record larger than file size is invalid.
    //
    if (RecordLength < sizeof (APPLE_ICNS_RECORD) + sizeof (UINT32)
      || OcOverflowAddU32 (Offset, RecordLength, &Offset)
      || Offset > IcnsImageSize) {
      return EFI_SECURITY_VIOLATION;
    }

    if ((Scale == 1 && Record->Type == APPLE_ICNS_IC07)
      || (Scale == 2 && Record->Type == APPLE_ICNS_IC13)) {
      Status = GuiPngToImage (
        Image,
        Record->Data,
        RecordLength - sizeof (APPLE_ICNS_RECORD),
        TRUE
        );

      if (!EFI_ERROR (Status) && MatchWidth > 0 && MatchHeight > 0) {
        if (AllowLess
          ? (Image->Width >  MatchWidth * Scale || Image->Height >  MatchWidth * Scale
          || Image->Width == 0 || Image->Height == 0)
          : (Image->Width != MatchWidth * Scale || Image->Height != MatchHeight * Scale)) {
          FreePool (Image->Buffer);
          DEBUG ((
            DEBUG_INFO,
            "OCUI: Expected %dx%d, actual %dx%d, allow less: %d\n",
             MatchWidth * Scale,
             MatchHeight * Scale,
             Image->Width,
             Image->Height,
             AllowLess
            ));
          Status = EFI_UNSUPPORTED;
        }
      }

      return Status;
    }

    if (Scale == 1) {
      if (Record->Type == APPLE_ICNS_IT32) {
        RecordIT32 = Record;
      } else if (Record->Type == APPLE_ICNS_T8MK) {
        RecordT8MK = Record;
      }

      if (RecordT8MK != NULL && RecordIT32 != NULL) {
        Image->Width  = MatchWidth;
        Image->Height = MatchHeight;
        ImageSize     = (MatchWidth * MatchHeight) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
        Image->Buffer = AllocateZeroPool (ImageSize);

        if (Image->Buffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // We have to add an additional UINT32 for IT32, since it has a reserved field.
        //
        DecodedBytes = DecompressMaskedRLE24 (
          (UINT8 *) Image->Buffer,
          ImageSize,
          RecordIT32->Data + sizeof (UINT32),
          SwapBytes32 (RecordIT32->Size) - sizeof (APPLE_ICNS_RECORD) - sizeof (UINT32),
          RecordT8MK->Data,
          SwapBytes32 (RecordT8MK->Size) - sizeof (APPLE_ICNS_RECORD),
          TRUE
          );

        if (DecodedBytes != ImageSize) {
          FreePool (Image->Buffer);
          return EFI_UNSUPPORTED;
        }

        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
GuiLabelToImage (
  OUT GUI_IMAGE *Image,
  IN  VOID      *RawData,
  IN  UINT32    DataLength,
  IN  UINT8     Scale,
  IN  BOOLEAN   Inverted
  )
{
  APPLE_DISK_LABEL  *Label;
  UINT32            PixelIdx;

  ASSERT (RawData != NULL);
  ASSERT (Scale == 1 || Scale == 2);

  if (DataLength < sizeof (APPLE_DISK_LABEL)) {
    return EFI_INVALID_PARAMETER;
  }

  Label = RawData;
  Image->Width  = SwapBytes16 (Label->Width);
  Image->Height = SwapBytes16 (Label->Height);

  if (Image->Width > APPLE_DISK_LABEL_MAX_WIDTH * Scale
    || Image->Height > APPLE_DISK_LABEL_MAX_HEIGHT * Scale
    || DataLength != sizeof (APPLE_DISK_LABEL) + Image->Width * Image->Height) {
    DEBUG ((DEBUG_INFO, "OCUI: Invalid label has %dx%d dims at %u size\n", Image->Width, Image->Height, DataLength));
    return EFI_SECURITY_VIOLATION;
  }

  Image->Buffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) AllocatePool (
    Image->Width * Image->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
    );

  if (Image->Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Inverted) {
    for (PixelIdx = 0; PixelIdx < Image->Width * Image->Height; PixelIdx++) {
      Image->Buffer[PixelIdx].Blue     = 0;
      Image->Buffer[PixelIdx].Green    = 0;
      Image->Buffer[PixelIdx].Red      = 0;
      Image->Buffer[PixelIdx].Reserved = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
    }
  } else {
    for (PixelIdx = 0; PixelIdx < Image->Width * Image->Height; PixelIdx++) {
      Image->Buffer[PixelIdx].Blue     = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
      Image->Buffer[PixelIdx].Green    = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
      Image->Buffer[PixelIdx].Red      = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
      Image->Buffer[PixelIdx].Reserved = 255 -  gAppleDiskLabelImagePalette[Label->Data[PixelIdx]];
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GuiPngToImage (
  OUT GUI_IMAGE  *Image,
  IN  VOID       *ImageData,
  IN  UINTN      ImageDataSize,
  IN  BOOLEAN    PremultiplyAlpha
  )
{
  EFI_STATUS                       Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BufferWalker;
  UINTN                            Index;
  UINT8                            TmpChannel;

  Status = OcDecodePng (
    ImageData,
    ImageDataSize,
    (VOID **) &Image->Buffer,
    &Image->Width,
    &Image->Height,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCUI: DecodePNG - %r\n", Status));
    return Status;
  }

  if (PremultiplyAlpha) {
    BufferWalker = Image->Buffer;
    for (Index = 0; Index < (UINTN) Image->Width * Image->Height; ++Index) {
      TmpChannel             = (UINT8) ((BufferWalker->Blue * BufferWalker->Reserved) / 0xFF);
      BufferWalker->Blue     = (UINT8) ((BufferWalker->Red * BufferWalker->Reserved) / 0xFF);
      BufferWalker->Green    = (UINT8) ((BufferWalker->Green * BufferWalker->Reserved) / 0xFF);
      BufferWalker->Red      = TmpChannel;
      ++BufferWalker;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GuiCreateHighlightedImage (
  OUT GUI_IMAGE                            *SelectedImage,
  IN  CONST GUI_IMAGE                      *SourceImage,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL PremulPixel;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
  UINT32                        ColumnOffset;
  BOOLEAN                       OneSet;
  UINT32                        FirstUnsetX;
  UINT32                        IndexY;
  UINT32                        RowOffset;

  ASSERT (SelectedImage != NULL);
  ASSERT (SourceImage != NULL);
  ASSERT (SourceImage->Buffer != NULL);
  ASSERT (HighlightPixel != NULL);
  //
  // The multiplication cannot wrap around because the original allocation sane.
  //
  Buffer = AllocateCopyPool (
             SourceImage->Width * SourceImage->Height * sizeof (*SourceImage->Buffer),
             SourceImage->Buffer
             );
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PremulPixel.Blue     = (UINT8)((HighlightPixel->Blue  * HighlightPixel->Reserved) / 0xFF);
  PremulPixel.Green    = (UINT8)((HighlightPixel->Green * HighlightPixel->Reserved) / 0xFF);
  PremulPixel.Red      = (UINT8)((HighlightPixel->Red   * HighlightPixel->Reserved) / 0xFF);
  PremulPixel.Reserved = HighlightPixel->Reserved;

  for (
    IndexY = 0, RowOffset = 0;
    IndexY < SourceImage->Height;
    ++IndexY, RowOffset += SourceImage->Width
    ) {
    FirstUnsetX = 0;
    OneSet      = FALSE;

    for (ColumnOffset = 0; ColumnOffset < SourceImage->Width; ++ColumnOffset) {
      if (SourceImage->Buffer[RowOffset + ColumnOffset].Reserved != 0) {
        OneSet = TRUE;
        GuiBlendPixelSolid (&Buffer[RowOffset + ColumnOffset], &PremulPixel);
        if (FirstUnsetX != 0) {
          //
          // Set all fully transparent pixels between two not fully transparent
          // pixels to the highlighter pixel.
          //
          while (FirstUnsetX < ColumnOffset) {
            CopyMem (
              &Buffer[RowOffset + FirstUnsetX],
              &PremulPixel,
              sizeof (*Buffer)
              );
            ++FirstUnsetX;
          }

          FirstUnsetX = 0;
        }
      } else if (FirstUnsetX == 0 && OneSet) {
        FirstUnsetX = ColumnOffset;
      }
    }
  }

  SelectedImage->Width  = SourceImage->Width;
  SelectedImage->Height = SourceImage->Height;
  SelectedImage->Buffer = Buffer;
  return EFI_SUCCESS;
}

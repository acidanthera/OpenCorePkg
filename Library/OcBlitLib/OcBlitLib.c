/** @file
  OcBlitLib - Library to perform blt operations on a frame buffer.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Protocol/GraphicsOutput.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcBlitLib.h>

#include "BlitInternal.h"

STATIC CONST EFI_PIXEL_BITMASK mRgbPixelMasks = {
  0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
};

STATIC CONST EFI_PIXEL_BITMASK mBgrPixelMasks = {
  0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000
};

/**
  Initialize the bit mask in frame buffer configure.

  @param BitMask       The bit mask of pixel.
  @param BytesPerPixel Size in bytes of pixel.
  @param PixelShl      Left shift array.
  @param PixelShr      Right shift array.
**/
STATIC
VOID
BlitLibConfigurePixelFormat (
  IN  CONST EFI_PIXEL_BITMASK   *BitMask,
  OUT UINT32                    *BytesPerPixel,
  OUT INT8                      *PixelShl,
  OUT INT8                      *PixelShr
  )
{
  UINT8   Index;
  UINT32  *Masks;
  UINT32  MergedMasks;

  ASSERT (BytesPerPixel != NULL);

  MergedMasks = 0;
  Masks = (UINT32*) BitMask;
  for (Index = 0; Index < 3; Index++) {
    ASSERT ((MergedMasks & Masks[Index]) == 0);

    PixelShl[Index] = (INT8) HighBitSet32 (Masks[Index]) - 23 + (Index * 8);
    if (PixelShl[Index] < 0) {
      PixelShr[Index] = -PixelShl[Index];
      PixelShl[Index] = 0;
    } else {
      PixelShr[Index] = 0;
    }
    DEBUG ((
      DEBUG_INFO,
      "OCBLT: %d: shl:%d shr:%d mask:%x\n",
      Index,
      PixelShl[Index],
      PixelShr[Index],
      Masks[Index]
      ));

    MergedMasks = (UINT32) (MergedMasks | Masks[Index]);
  }
  MergedMasks = (UINT32) (MergedMasks | Masks[3]);

  ASSERT (MergedMasks != 0);
  *BytesPerPixel = (UINT32) ((HighBitSet32 (MergedMasks) + 7) / 8);
  DEBUG ((DEBUG_INFO, "OCBLT: Bytes per pixel: %d\n", *BytesPerPixel));
}

/**
  Performs a UEFI Graphics Output Protocol Blt Video Fill.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[in]  Color         Color to fill the region with.
  @param[in]  DestinationX  X location to start fill operation.
  @param[in]  DestinationY  Y location to start fill operation.
  @param[in]  Width         Width (in pixels) to fill.
  @param[in]  Height        Height to fill.

  @retval  RETURN_INVALID_PARAMETER Invalid parameter was passed in.
  @retval  RETURN_SUCCESS           The video was filled successfully.

**/
STATIC
EFI_STATUS
BlitLibVideoFill (
  IN  OC_BLIT_CONFIGURE             *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Color,
  IN  UINTN                         DestinationX,
  IN  UINTN                         DestinationY,
  IN  UINTN                         Width,
  IN  UINTN                         Height
  )
{
  UINTN                             IndexX;
  UINTN                             IndexY;
  UINTN                             Tmp;
  UINT8                             *Destination;
  UINT32                            Uint32;
  UINT64                            WideFill;
  BOOLEAN                           LineBufferReady;
  UINTN                             Offset;
  UINTN                             WidthInBytes;
  UINTN                             SizeInBytes;

  //
  // BltBuffer to Video: Source is BltBuffer, destination is Video
  //
  if (DestinationY + Height > Configure->RotatedHeight) {
    DEBUG ((DEBUG_VERBOSE, "OCBLT: Past screen (Y)\n"));
    return RETURN_INVALID_PARAMETER;
  }

  if (DestinationX + Width > Configure->RotatedWidth) {
    DEBUG ((DEBUG_VERBOSE, "OCBLT: Past screen (X)\n"));
    return RETURN_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    DEBUG ((DEBUG_VERBOSE, "OCBLT: Width or Height is 0\n"));
    return RETURN_INVALID_PARAMETER;
  }

  if (Configure->Rotation == 90) {
    //
    // Perform -90 rotation.
    //
    Tmp          = Width;
    Width        = Height;
    Height       = Tmp;
    Tmp          = DestinationX;
    DestinationX = Configure->Width - DestinationY - Width;
    DestinationY = Tmp;
  } else if (Configure->Rotation == 180) {
    //
    // Perform -180 rotation.
    //
    DestinationX = Configure->Width  - DestinationX - Width;
    DestinationY = Configure->Height - DestinationY - Height;
  } else if (Configure->Rotation == 270) {
    //
    // Perform +90 rotation.
    //
    Tmp          = Width;
    Width        = Height;
    Height       = Tmp;

    Tmp          = DestinationX;
    DestinationX = DestinationY;
    DestinationY = Configure->Height - Tmp - Height;
  }

  WidthInBytes = Width * BYTES_PER_PIXEL;

  Uint32 = *(UINT32*) Color;
  WideFill =
    (UINT32) (
    (((Uint32 << Configure->PixelShl[0]) >> Configure->PixelShr[0]) &
     Configure->PixelMasks.RedMask) |
     (((Uint32 << Configure->PixelShl[1]) >> Configure->PixelShr[1]) &
      Configure->PixelMasks.GreenMask) |
      (((Uint32 << Configure->PixelShl[2]) >> Configure->PixelShr[2]) &
       Configure->PixelMasks.BlueMask)
      );
  DEBUG ((DEBUG_VERBOSE, "OCBLT: color=0x%x, wide-fill=0x%x\n", Uint32, WideFill));

  //
  // If the size of the pixel data evenly divides the sizeof
  // WideFill, then a wide fill operation can be used
  //
  for (IndexX = BYTES_PER_PIXEL; IndexX < sizeof (WideFill); IndexX++) {
    ((UINT8*) &WideFill)[IndexX] = ((UINT8*) &WideFill)[IndexX % BYTES_PER_PIXEL];
  }

  if (DestinationX == 0 && Width == Configure->PixelsPerScanLine) {
    DEBUG ((DEBUG_VERBOSE, "OCBLT: VideoFill (wide, one-shot)\n"));
    Offset = DestinationY * Configure->PixelsPerScanLine;
    Offset = BYTES_PER_PIXEL * Offset;
    Destination = Configure->FrameBuffer + Offset;
    SizeInBytes = WidthInBytes * Height;
    if (SizeInBytes >= 8) {
      SetMem32 (Destination, SizeInBytes & ~3, (UINT32) WideFill);
      Destination += SizeInBytes & ~3;
      SizeInBytes &= 3;
    }
    if (SizeInBytes > 0) {
      SetMem (Destination, SizeInBytes, (UINT8) (UINTN) WideFill);
    }
  } else {
    LineBufferReady = FALSE;
    for (IndexY = DestinationY; IndexY < (Height + DestinationY); IndexY++) {
      Offset = (IndexY * Configure->PixelsPerScanLine) + DestinationX;
      Offset = BYTES_PER_PIXEL * Offset;
      Destination = Configure->FrameBuffer + Offset;

      if (((UINTN) Destination & 7) == 0) {
        DEBUG ((DEBUG_VERBOSE, "OCBLT: VideoFill (wide)\n"));
        SizeInBytes = WidthInBytes;
        if (SizeInBytes >= 8) {
          SetMem64 (Destination, SizeInBytes & ~7, WideFill);
          Destination += SizeInBytes & ~7;
          SizeInBytes &= 7;
        }
        if (SizeInBytes > 0) {
          CopyMem (Destination, &WideFill, SizeInBytes);
        }
      } else {
        DEBUG ((DEBUG_VERBOSE, "OCBLT: VideoFill (not wide)\n"));
        if (!LineBufferReady) {
          CopyMem (Configure->LineBuffer, &WideFill, BYTES_PER_PIXEL);
          for (IndexX = 1; IndexX < Width; ) {
            CopyMem (
              (Configure->LineBuffer + (IndexX * BYTES_PER_PIXEL)),
              Configure->LineBuffer,
              MIN (IndexX, Width - IndexX) * BYTES_PER_PIXEL
            );
            IndexX += MIN (IndexX, Width - IndexX);
          }
          LineBufferReady = TRUE;
        }
        CopyMem (Destination, Configure->LineBuffer, WidthInBytes);
      }
    }
  }

  return RETURN_SUCCESS;
}

/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation
  with extended parameters.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[out] BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within video.
  @param[in]  SourceY       Y location within video.
  @param[in]  DestinationX  X location within BltBuffer.
  @param[in]  DestinationY  Y location within BltBuffer.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  Delta         Number of bytes in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
STATIC
RETURN_STATUS
BlitLibVideoToBuffer (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           Delta
  )
{
  //
  // Video to BltBuffer: Source is Video, destination is BltBuffer
  //
  if (SourceY + Height > Configure->RotatedHeight) {
    return RETURN_INVALID_PARAMETER;
  }

  if (SourceX + Width > Configure->RotatedWidth) {
    return RETURN_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta is
  // the number of bytes in each row of BltBuffer. Since BltBuffer is Width
  // pixels size, the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width;
  } else {
    Delta /= BYTES_PER_PIXEL;
  }

  switch (Configure->Rotation) {
    case 0:
      return BlitLibVideoToBuffer0 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    case 90:
      return BlitLibVideoToBuffer90 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    case 180:
      return BlitLibVideoToBuffer180 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    case 270:
      return BlitLibVideoToBuffer270 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    default:
      ASSERT (FALSE);
      break;
  }

  return RETURN_SUCCESS;
}

/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation
  with extended parameters.

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[in]  BltBuffer     Output buffer for pixel color data.
  @param[in]  SourceX       X location within BltBuffer.
  @param[in]  SourceY       Y location within BltBuffer.
  @param[in]  DestinationX  X location within video.
  @param[in]  DestinationY  Y location within video.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.
  @param[in]  Delta         Number of bytes in a row of BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
STATIC
RETURN_STATUS
BlitLibBufferToVideo (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  )
{
  //
  // BltBuffer to Video: Source is BltBuffer, destination is Video
  //
  if (DestinationY + Height > Configure->RotatedHeight) {
    return RETURN_INVALID_PARAMETER;
  }

  if (DestinationX + Width > Configure->RotatedWidth) {
    return RETURN_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta is
  // the number of bytes in each row of BltBuffer. Since BltBuffer is Width
  // pixels size, the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width;
  } else {
    Delta /= BYTES_PER_PIXEL;
  }

  switch (Configure->Rotation) {
    case 0:
      return BlitLibBufferToVideo0 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    case 90:
      return BlitLibBufferToVideo90 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    case 180:
      return BlitLibBufferToVideo180 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    case 270:
      return BlitLibBufferToVideo270 (
        Configure,
        BltBuffer,
        SourceX,
        SourceY,
        DestinationX,
        DestinationY,
        Width,
        Height,
        Delta
        );
    default:
      ASSERT (FALSE);
      break;
  }

  return RETURN_SUCCESS;
}

/**
  Performs a UEFI Graphics Output Protocol Blt Video to Video operation

  @param[in]  Configure     Pointer to a configuration which was successfully
                            created by FrameBufferBltConfigure ().
  @param[in]  SourceX       X location within video.
  @param[in]  SourceY       Y location within video.
  @param[in]  DestinationX  X location within video.
  @param[in]  DestinationY  Y location within video.
  @param[in]  Width         Width (in pixels).
  @param[in]  Height        Height.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
STATIC
RETURN_STATUS
BlitLibVideoToVideo (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  UINT8                                     *Source;
  UINT8                                     *Destination;
  UINTN                                     Offset;
  UINTN                                     WidthInBytes;
  UINTN                                     Tmp;
  INTN                                      LineStride;

  //
  // Video to Video: Source is Video, destination is Video
  //
  if (SourceY + Height > Configure->RotatedHeight) {
    return RETURN_INVALID_PARAMETER;
  }

  if (SourceX + Width > Configure->RotatedWidth) {
    return RETURN_INVALID_PARAMETER;
  }

  if (DestinationY + Height > Configure->RotatedHeight) {
    return RETURN_INVALID_PARAMETER;
  }

  if (DestinationX + Width > Configure->RotatedWidth) {
    return RETURN_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  if (Configure->Rotation == 90) {
    //
    // Perform -90 rotation.
    //
    Tmp          = Width;
    Width        = Height;
    Height       = Tmp;
    Tmp          = DestinationX;
    DestinationX = Configure->Width - DestinationY - Width;
    DestinationY = Tmp;
    Tmp          = SourceX;
    SourceX      = Configure->Width - SourceY - Width;
    SourceY      = Tmp;
  } else if (Configure->Rotation == 180) {
    //
    // Perform -180 rotation.
    //
    DestinationX = Configure->Width  - DestinationX - Width;
    DestinationY = Configure->Height - DestinationY - Height;
    SourceX      = Configure->Width  - SourceX - Width;
    SourceY      = Configure->Height - SourceY - Height;
  } else if (Configure->Rotation == 270) {
    //
    // Perform +90 rotation.
    //
    Tmp          = Width;
    Width        = Height;
    Height       = Tmp;
    Tmp          = DestinationX;
    DestinationX = DestinationY;
    DestinationY = Configure->Height - Tmp - Height;
    Tmp          = SourceX;
    SourceX      = SourceY;
    SourceY      = Configure->Height - Tmp - Height;
  }

  WidthInBytes = Width * BYTES_PER_PIXEL;

  Offset = (SourceY * Configure->PixelsPerScanLine) + SourceX;
  Offset = BYTES_PER_PIXEL * Offset;
  Source = Configure->FrameBuffer + Offset;

  Offset = (DestinationY * Configure->PixelsPerScanLine) + DestinationX;
  Offset = BYTES_PER_PIXEL * Offset;
  Destination = Configure->FrameBuffer + Offset;

  LineStride = BYTES_PER_PIXEL * Configure->PixelsPerScanLine;
  if (Destination > Source) {
    //
    // Copy from last line to avoid source is corrupted by copying
    //
    Source += Height * LineStride;
    Destination += Height * LineStride;
    LineStride = -LineStride;
  }

  while (Height-- > 0) {
    CopyMem (Destination, Source, WidthInBytes);

    Source += LineStride;
    Destination += LineStride;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
OcBlitConfigure (
  IN     VOID                                  *FrameBuffer,
  IN     EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *FrameBufferInfo,
  IN     UINT32                                Rotation,
  IN OUT OC_BLIT_CONFIGURE                     *Configure,
  IN OUT UINTN                                 *ConfigureSize
  )
{
  CONST EFI_PIXEL_BITMASK                      *BitMask;
  UINT32                                       BytesPerPixel;
  INT8                                         PixelShl[4];
  INT8                                         PixelShr[4];

  if (ConfigureSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  switch (FrameBufferInfo->PixelFormat) {
  case PixelRedGreenBlueReserved8BitPerColor:
    BitMask = &mRgbPixelMasks;
    break;

  case PixelBlueGreenRedReserved8BitPerColor:
    BitMask = &mBgrPixelMasks;
    break;

  case PixelBitMask:
    BitMask = &FrameBufferInfo->PixelInformation;
    break;

  case PixelBltOnly:
    ASSERT (FrameBufferInfo->PixelFormat != PixelBltOnly);
    return RETURN_UNSUPPORTED;

  default:
    ASSERT (FALSE);
    return RETURN_INVALID_PARAMETER;
  }

  if (FrameBufferInfo->PixelsPerScanLine < FrameBufferInfo->HorizontalResolution) {
    return RETURN_UNSUPPORTED;
  }

  BlitLibConfigurePixelFormat (BitMask, &BytesPerPixel, PixelShl, PixelShr);

  if (BytesPerPixel != sizeof (UINT32)) {
    return RETURN_UNSUPPORTED;
  }

  if (*ConfigureSize < sizeof (OC_BLIT_CONFIGURE)
                     + FrameBufferInfo->HorizontalResolution * sizeof (UINT32)) {
    *ConfigureSize = sizeof (OC_BLIT_CONFIGURE)
                   + FrameBufferInfo->HorizontalResolution * sizeof (UINT32);
    return RETURN_BUFFER_TOO_SMALL;
  }

  if (Configure == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  CopyMem (&Configure->PixelMasks, BitMask,  sizeof (*BitMask));
  CopyMem (Configure->PixelShl,    PixelShl, sizeof (PixelShl));
  CopyMem (Configure->PixelShr,    PixelShr, sizeof (PixelShr));
  Configure->PixelFormat       = FrameBufferInfo->PixelFormat;
  Configure->FrameBuffer       = (UINT8*) FrameBuffer;
  Configure->Width             = FrameBufferInfo->HorizontalResolution;
  Configure->Height            = FrameBufferInfo->VerticalResolution;
  Configure->PixelsPerScanLine = FrameBufferInfo->PixelsPerScanLine;
  Configure->Rotation          = Rotation;

  if (Rotation == 90 || Rotation == 270) {
    Configure->RotatedWidth  = FrameBufferInfo->VerticalResolution;
    Configure->RotatedHeight = FrameBufferInfo->HorizontalResolution;
  } else {
    Configure->RotatedWidth  = FrameBufferInfo->HorizontalResolution;
    Configure->RotatedHeight = FrameBufferInfo->VerticalResolution;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
OcBlitRender (
  IN     OC_BLIT_CONFIGURE                     *Configure,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer OPTIONAL,
  IN     EFI_GRAPHICS_OUTPUT_BLT_OPERATION     BltOperation,
  IN     UINTN                                 SourceX,
  IN     UINTN                                 SourceY,
  IN     UINTN                                 DestinationX,
  IN     UINTN                                 DestinationY,
  IN     UINTN                                 Width,
  IN     UINTN                                 Height,
  IN     UINTN                                 Delta
  )
{
  ASSERT (Configure != NULL);

  switch (BltOperation) {
  case EfiBltVideoToBltBuffer:
    return BlitLibVideoToBuffer (
             Configure,
             BltBuffer,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta
             );

  case EfiBltVideoToVideo:
    return BlitLibVideoToVideo (
             Configure,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height
             );

  case EfiBltVideoFill:
    return BlitLibVideoFill (
             Configure,
             BltBuffer,
             DestinationX,
             DestinationY,
             Width,
             Height
             );

  case EfiBltBufferToVideo:
    return BlitLibBufferToVideo (
             Configure,
             BltBuffer,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta
             );

  default:
    return RETURN_INVALID_PARAMETER;
  }
}

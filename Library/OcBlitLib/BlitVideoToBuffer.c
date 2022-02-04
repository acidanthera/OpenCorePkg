/** @file
  OcBlitLib - Library to perform blt operations on a frame buffer.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "BlitInternal.h"

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

RETURN_STATUS
BlitLibVideoToBuffer0 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  )
{
  UINT32                                   *Source;
  UINT32                                   *Destination;
  UINT32                                   *SourceWalker;
  UINT32                                   *DestinationWalker;
  UINTN                                    IndexX;
  UINTN                                    WidthInBytes;
  UINTN                                    PixelsPerScanLine;
  UINT32                                   Uint32;

  WidthInBytes      = Width * BYTES_PER_PIXEL;
  PixelsPerScanLine = Configure->PixelsPerScanLine;

  Destination = (UINT32 *) BltBuffer
    + DestinationY * DeltaPixels + DestinationX;
  Source = (UINT32 *) Configure->FrameBuffer
    + SourceY * PixelsPerScanLine + SourceX;
  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      CopyMem (Destination, Source, WidthInBytes);
      Source      += PixelsPerScanLine;
      Destination += DeltaPixels;
      Height--;
    }
  } else {
    while (Height > 0) {
      DestinationWalker = (UINT32 *) Configure->LineBuffer;
      SourceWalker      = (UINT32 *) Source;
      for (IndexX = 0; IndexX < Width; IndexX++) {
        Uint32 = *SourceWalker++;
        *DestinationWalker++ =
          (UINT32) (
            (((Uint32 << Configure->PixelShl[0]) >> Configure->PixelShr[0]) &
             Configure->PixelMasks.RedMask) |
            (((Uint32 << Configure->PixelShl[1]) >> Configure->PixelShr[1]) &
             Configure->PixelMasks.GreenMask) |
            (((Uint32 << Configure->PixelShl[2]) >> Configure->PixelShr[2]) &
             Configure->PixelMasks.BlueMask)
            );
      }

      CopyMem (Destination, Configure->LineBuffer, WidthInBytes);
      Source      += PixelsPerScanLine;
      Destination += DeltaPixels;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

RETURN_STATUS
BlitLibVideoToBuffer90 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  )
{
  UINT32                                   *Source;
  UINT32                                   *Destination;
  UINT32                                   *SourceWalker;
  UINT32                                   *DestinationWalker;
  UINTN                                    IndexX;
  UINTN                                    PixelsPerScanLine;
  UINT32                                   Uint32;

  PixelsPerScanLine = Configure->PixelsPerScanLine;

  Destination = (UINT32 *) BltBuffer
    + DestinationY * DeltaPixels + DestinationX;
  Source = (UINT32 *) Configure->FrameBuffer
    + (Configure->Width - SourceY - 1);

  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      for (IndexX = SourceX; IndexX < SourceX + Width; IndexX++) {
        *DestinationWalker++ = SourceWalker[IndexX * PixelsPerScanLine];
      }
      Source--;
      Destination += DeltaPixels;
      Height--;
    }
  } else {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      for (IndexX = SourceX; IndexX < SourceX + Width; IndexX++) {
        Uint32 = SourceWalker[IndexX * PixelsPerScanLine];
        *DestinationWalker++ =
          (UINT32) (
            (((Uint32 << Configure->PixelShl[0]) >> Configure->PixelShr[0]) &
             Configure->PixelMasks.RedMask) |
            (((Uint32 << Configure->PixelShl[1]) >> Configure->PixelShr[1]) &
             Configure->PixelMasks.GreenMask) |
            (((Uint32 << Configure->PixelShl[2]) >> Configure->PixelShr[2]) &
             Configure->PixelMasks.BlueMask)
            );
      }
      Source--;
      Destination += DeltaPixels;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

RETURN_STATUS
BlitLibVideoToBuffer180 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  )
{
  UINT32                                   *Source;
  UINT32                                   *Destination;
  UINT32                                   *SourceWalker;
  UINT32                                   *DestinationWalker;
  UINTN                                    IndexX;
  UINTN                                    PixelsPerScanLine;
  UINT32                                   Uint32;

  PixelsPerScanLine = Configure->PixelsPerScanLine;

  SourceX = Configure->Width  - SourceX - Width;
  SourceY = Configure->Height - SourceY - Height;

  Destination = (UINT32 *) BltBuffer
    + DestinationY * DeltaPixels + DestinationX;
  Source = (UINT32 *) Configure->FrameBuffer
    + (SourceY + (Height - 1)) * PixelsPerScanLine + SourceX;

  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source + (Width - 1);
      for (IndexX = 0; IndexX < Width; IndexX++) {
        *DestinationWalker++ = *SourceWalker--;
      }
      Source      -= PixelsPerScanLine;
      Destination += DeltaPixels;
      Height--;
    }
  } else {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source + (Width - 1);
      for (IndexX = 0; IndexX < Width; IndexX++) {
        Uint32 = *SourceWalker--;
        *DestinationWalker++ =
          (UINT32) (
            (((Uint32 << Configure->PixelShl[0]) >> Configure->PixelShr[0]) &
             Configure->PixelMasks.RedMask) |
            (((Uint32 << Configure->PixelShl[1]) >> Configure->PixelShr[1]) &
             Configure->PixelMasks.GreenMask) |
            (((Uint32 << Configure->PixelShl[2]) >> Configure->PixelShr[2]) &
             Configure->PixelMasks.BlueMask)
            );
      }
      Source      -= PixelsPerScanLine;
      Destination += DeltaPixels;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

RETURN_STATUS
BlitLibVideoToBuffer270 (
  IN     OC_BLIT_CONFIGURE               *Configure,
     OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,
  IN     UINTN                           SourceX,
  IN     UINTN                           SourceY,
  IN     UINTN                           DestinationX,
  IN     UINTN                           DestinationY,
  IN     UINTN                           Width,
  IN     UINTN                           Height,
  IN     UINTN                           DeltaPixels
  )
{
  UINT32                                   *Source;
  UINT32                                   *Destination;
  UINT32                                   *SourceWalker;
  UINT32                                   *DestinationWalker;
  UINTN                                    IndexX;
  UINTN                                    LastX;
  UINTN                                    PixelsPerScanLine;
  UINT32                                   Uint32;

  PixelsPerScanLine = Configure->PixelsPerScanLine;

  Destination = (UINT32 *) BltBuffer
    + DestinationY * DeltaPixels + DestinationX;
  Source = (UINT32 *) Configure->FrameBuffer
    + SourceY;

  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      LastX = Configure->Height - SourceX - 1;
      for (IndexX = 0; IndexX < Width; IndexX++) {
        *DestinationWalker++ = SourceWalker[(LastX - IndexX) * PixelsPerScanLine];
      }
      Source++;
      Destination += DeltaPixels;
      Height--;
    }
  } else {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      LastX = Configure->Height - SourceX - 1;
      for (IndexX = 0; IndexX < Width; IndexX++) {
        Uint32 = SourceWalker[(LastX - IndexX) * PixelsPerScanLine];
        *DestinationWalker++ =
          (UINT32) (
            (((Uint32 << Configure->PixelShl[0]) >> Configure->PixelShr[0]) &
             Configure->PixelMasks.RedMask) |
            (((Uint32 << Configure->PixelShl[1]) >> Configure->PixelShr[1]) &
             Configure->PixelMasks.GreenMask) |
            (((Uint32 << Configure->PixelShl[2]) >> Configure->PixelShr[2]) &
             Configure->PixelMasks.BlueMask)
            );
      }
      Source++;
      Destination += DeltaPixels;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

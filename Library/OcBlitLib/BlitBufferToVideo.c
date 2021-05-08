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
BlitLibBufferToVideo0 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
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

  Destination = (UINT32 *) Configure->FrameBuffer
    + DestinationY * PixelsPerScanLine + DestinationX;
  Source = (UINT32 *) BltBuffer
    + SourceY * DeltaPixels + SourceX;
  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      CopyMem (Destination, Source, WidthInBytes);
      Source      += DeltaPixels;
      Destination += PixelsPerScanLine;
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
      Source      += DeltaPixels;
      Destination += PixelsPerScanLine;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

RETURN_STATUS
BlitLibBufferToVideo90 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
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

  Destination = (UINT32 *) Configure->FrameBuffer
    + (Configure->Width - DestinationY - 1);
  Source = (UINT32 *) BltBuffer
    + SourceY * DeltaPixels + SourceX;

  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      for (IndexX = DestinationX; IndexX < DestinationX + Width; IndexX++) {
        DestinationWalker[IndexX * PixelsPerScanLine] = *SourceWalker++;
      }
      Source += DeltaPixels;
      Destination--;
      Height--;
    }
  } else {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      for (IndexX = DestinationX; IndexX < DestinationX + Width; IndexX++) {
        Uint32 = *SourceWalker++;
        DestinationWalker[IndexX * PixelsPerScanLine] =
          (UINT32) (
            (((Uint32 << Configure->PixelShl[0]) >> Configure->PixelShr[0]) &
             Configure->PixelMasks.RedMask) |
            (((Uint32 << Configure->PixelShl[1]) >> Configure->PixelShr[1]) &
             Configure->PixelMasks.GreenMask) |
            (((Uint32 << Configure->PixelShl[2]) >> Configure->PixelShr[2]) &
             Configure->PixelMasks.BlueMask)
            );
      }
      Source += DeltaPixels;
      Destination--;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

RETURN_STATUS
BlitLibBufferToVideo180 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
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

  DestinationX = Configure->Width  - DestinationX - Width;
  DestinationY = Configure->Height - DestinationY - Height;

  Destination = (UINT32 *) Configure->FrameBuffer
    + (DestinationY + (Height - 1)) * PixelsPerScanLine + DestinationX;
  Source = (UINT32 *) BltBuffer
    + SourceY * DeltaPixels + SourceX;

  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source + (Width - 1);
      for (IndexX = 0; IndexX < Width; IndexX++) {
        *DestinationWalker++ = *SourceWalker--;
      }
      Source      += DeltaPixels;
      Destination -= PixelsPerScanLine;
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
      Source      += DeltaPixels;
      Destination -= PixelsPerScanLine;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

RETURN_STATUS
BlitLibBufferToVideo270 (
  IN  OC_BLIT_CONFIGURE                     *Configure,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 DeltaPixels
  )
{
  UINT32                                   *Source;
  UINT32                                   *Destination;
  UINT32                                   *SourceWalker;
  UINT32                                   *DestinationWalker;
  UINTN                                    LastX;
  UINTN                                    IndexX;
  UINTN                                    PixelsPerScanLine;
  UINT32                                   Uint32;

  PixelsPerScanLine = Configure->PixelsPerScanLine;

  Destination = (UINT32 *) Configure->FrameBuffer
    + DestinationY;
  Source = (UINT32 *) BltBuffer
    + SourceY * DeltaPixels + SourceX;

  if (Configure->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      LastX = Configure->Height - DestinationX - 1;
      for (IndexX = LastX; IndexX > LastX - Width; IndexX--) {
        DestinationWalker[IndexX * PixelsPerScanLine] = *SourceWalker++;
      }
      Source += DeltaPixels;
      Destination++;
      Height--;
    }
  } else {
    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      LastX = Configure->Height - DestinationX - 1;
      for (IndexX = LastX; IndexX > LastX - Width; IndexX--) {
        Uint32 = *SourceWalker++;
        DestinationWalker[IndexX * PixelsPerScanLine] =
          (UINT32) (
            (((Uint32 << Configure->PixelShl[0]) >> Configure->PixelShr[0]) &
             Configure->PixelMasks.RedMask) |
            (((Uint32 << Configure->PixelShl[1]) >> Configure->PixelShr[1]) &
             Configure->PixelMasks.GreenMask) |
            (((Uint32 << Configure->PixelShl[2]) >> Configure->PixelShr[2]) &
             Configure->PixelMasks.BlueMask)
            );
      }
      Source += DeltaPixels;
      Destination--;
      Height--;
    }
  }

  return EFI_SUCCESS;
}

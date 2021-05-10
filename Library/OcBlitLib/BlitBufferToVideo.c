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
    Uint32 = ((UINT32) SourceX | (UINT32) SourceY
      | (UINT32) DestinationX  | (UINT32) DestinationY
      | (UINT32) Width         | (UINT32) Height
      | (UINT32) DeltaPixels   | (UINT32) PixelsPerScanLine) & (16 - 1);

    switch (Uint32) {
      case 0:
        {
          CONST UINTN BlockWidth = 16;
          Destination += DestinationX * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination + Index2 * PixelsPerScanLine - Index;
              //
              // Unrolling this one is ineffecient due to loop size(?). Gets down from 160 to 90 FPS.
              //
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex2 * PixelsPerScanLine - BlockIndex] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      case 8:
        {
          CONST UINTN BlockWidth = 8;
          Destination += DestinationX * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination + Index2 * PixelsPerScanLine - Index;
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                #if 0 /* Old compilers cannot unroll, disable for now */
                #pragma clang loop unroll(full)
                #endif
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex2 * PixelsPerScanLine - BlockIndex] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      case 4:
        {
          CONST UINTN BlockWidth = 4;
          Destination += DestinationX * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination + Index2 * PixelsPerScanLine - Index;
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                #if 0 /* Old compilers cannot unroll, disable for now */
                #pragma clang loop unroll(full)
                #endif
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex2 * PixelsPerScanLine - BlockIndex] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      case 2:
        {
          CONST UINTN BlockWidth = 2;
          Destination += DestinationX * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination + Index2 * PixelsPerScanLine - Index;
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                #if 0 /* Old compilers cannot unroll, disable for now */
                #pragma clang loop unroll(full)
                #endif
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex2 * PixelsPerScanLine - BlockIndex] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      default:
        break;
    }

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
    Uint32 = ((UINT32) SourceX | (UINT32) SourceY
      | (UINT32) DestinationX  | (UINT32) DestinationY
      | (UINT32) Width         | (UINT32) Height
      | (UINT32) DeltaPixels   | (UINT32) PixelsPerScanLine) & (16 - 1);

    switch (Uint32) {
      case 0:
        {
          CONST UINTN BlockWidth = 16;
          Destination += (Configure->Height - DestinationX - 1) * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination - Index2 * PixelsPerScanLine + Index;
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                #if 0 /* Old compilers cannot unroll, disable for now */
                #pragma clang loop unroll(full)
                #endif
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex - BlockIndex2 * PixelsPerScanLine] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      case 8:
        {
          CONST UINTN BlockWidth = 8;
          Destination += (Configure->Height - DestinationX - 1) * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination - Index2 * PixelsPerScanLine + Index;
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                #if 0 /* Old compilers cannot unroll, disable for now */
                #pragma clang loop unroll(full)
                #endif
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex - BlockIndex2 * PixelsPerScanLine] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      case 4:
        {
          CONST UINTN BlockWidth = 4;
          Destination += (Configure->Height - DestinationX - 1) * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination - Index2 * PixelsPerScanLine + Index;
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                #if 0 /* Old compilers cannot unroll, disable for now */
                #pragma clang loop unroll(full)
                #endif
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex - BlockIndex2 * PixelsPerScanLine] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      case 2:
        {
          CONST UINTN BlockWidth = 2;
          Destination += (Configure->Height - DestinationX - 1) * PixelsPerScanLine;
          for (UINTN Index = 0; Index < Height; Index += BlockWidth) {
            for (UINTN Index2 = 0; Index2 < Width; Index2 += BlockWidth) {
              UINT32 *A = Source + Index * DeltaPixels + Index2;
              UINT32 *B = Destination - Index2 * PixelsPerScanLine + Index;
              for (UINTN BlockIndex = 0; BlockIndex < BlockWidth; BlockIndex++) {
                #if 0 /* Old compilers cannot unroll, disable for now */
                #pragma clang loop unroll(full)
                #endif
                for (UINTN BlockIndex2 = 0; BlockIndex2 < BlockWidth; BlockIndex2++) {
                  B[BlockIndex - BlockIndex2 * PixelsPerScanLine] = A[BlockIndex * DeltaPixels + BlockIndex2];
                }
              }
            }
          }
        }
        return EFI_SUCCESS;
      default:
        break;
    }

    while (Height > 0) {
      DestinationWalker = Destination;
      SourceWalker      = Source;
      LastX = Configure->Height - DestinationX - 1;
      for (IndexX = 0; IndexX < Width; IndexX++) {
        DestinationWalker[(LastX - IndexX) * PixelsPerScanLine] = *SourceWalker++;
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
      for (IndexX = 0; IndexX < Width; IndexX++) {
        Uint32 = *SourceWalker++;
        DestinationWalker[(LastX - IndexX) * PixelsPerScanLine] =
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

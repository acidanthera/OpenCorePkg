/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/DebugLib.h>

#include "OpenCanopy.h"
#include "Blending.h"

#define PIXEL_TO_UINT32(Pixel)  \
  ((UINT32) SIGNATURE_32 ((Pixel)->Blue, (Pixel)->Green, (Pixel)->Red, (Pixel)->Reserved))

#define RGB_ALPHA_BLEND(Back, Front, InvFrontOpacity)  \
  ((Front) + RGB_APPLY_OPACITY (InvFrontOpacity, Back))

STATIC
VOID
InternalBlendPixel (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel
  )
{
  UINT8 InvFrontOpacity;

  InvFrontOpacity = (0xFF - FrontPixel->Reserved);

  BackPixel->Blue = RGB_ALPHA_BLEND (
                      BackPixel->Blue,
                      FrontPixel->Blue,
                      InvFrontOpacity
                      );
  BackPixel->Green = RGB_ALPHA_BLEND (
                       BackPixel->Green,
                       FrontPixel->Green,
                       InvFrontOpacity
                       );
  BackPixel->Red = RGB_ALPHA_BLEND (
                     BackPixel->Red,
                     FrontPixel->Red,
                     InvFrontOpacity
                     );

  if (BackPixel->Reserved != 0xFF) {
    BackPixel->Reserved = RGB_ALPHA_BLEND (
                            BackPixel->Reserved,
                            FrontPixel->Reserved,
                            InvFrontOpacity
                            );
  }
}

VOID
GuiBlendPixelOpaque (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel,
  IN     UINT8                                Opacity
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL OpacFrontPixel;
  //
  // FIXME: Optimise with SIMD or such.
  // qt_blend_argb32_on_argb32 in QT
  //
  ASSERT (BackPixel != NULL);
  ASSERT (FrontPixel != NULL);
  ASSERT (Opacity > 0);
  ASSERT (Opacity < 0xFF);

  if (FrontPixel->Reserved == 0) {
    return;
  }

  if (FrontPixel->Reserved == 0xFF) {
    OpacFrontPixel.Reserved = Opacity;
  } else {
    OpacFrontPixel.Reserved = RGB_APPLY_OPACITY (FrontPixel->Reserved, Opacity);
    if (OpacFrontPixel.Reserved == 0) {
      return;
    }
  }

  OpacFrontPixel.Blue     = RGB_APPLY_OPACITY (FrontPixel->Blue,  Opacity);
  OpacFrontPixel.Green    = RGB_APPLY_OPACITY (FrontPixel->Green, Opacity);
  OpacFrontPixel.Red      = RGB_APPLY_OPACITY (FrontPixel->Red,   Opacity);

  InternalBlendPixel (BackPixel, &OpacFrontPixel);
}

VOID
GuiBlendPixelSolid (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel
  )
{
  //
  // FIXME: Optimise with SIMD or such.
  // qt_blend_argb32_on_argb32 in QT
  //
  ASSERT (BackPixel != NULL);
  ASSERT (FrontPixel != NULL);

  if (FrontPixel->Reserved == 0) {
    return;
  }

  if (FrontPixel->Reserved == 0xFF) {
    BackPixel->Blue     = FrontPixel->Blue;
    BackPixel->Green    = FrontPixel->Green;
    BackPixel->Red      = FrontPixel->Red;
    BackPixel->Reserved = FrontPixel->Reserved;
    return;
  }

  InternalBlendPixel (BackPixel, FrontPixel);
}

VOID
GuiBlendPixel (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel,
  IN     UINT8                                Opacity
  )
{
  if (Opacity == 0xFF) {
    GuiBlendPixelSolid (BackPixel, FrontPixel);
  } else {
    GuiBlendPixelOpaque (BackPixel, FrontPixel, Opacity);
  }
}

/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserFile.h>

#include <Base.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BmpSupportLib.h>

#include "BmfLib.h"

EFI_STATUS
GuiBmpToImage (
  IN OUT GUI_IMAGE  *Image,
  IN     VOID       *BmpImage,
  IN     UINTN      BmpImageSize
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
  UINTN                         BufferSize;
  UINTN                         BmpHeight;
  UINTN                         BmpWidth;

  ASSERT (Image != NULL);
  ASSERT (BmpImage != NULL);
  ASSERT (BmpImageSize > 0);

  Buffer = NULL;
  Status = TranslateBmpToGopBlt (
             BmpImage,
             BmpImageSize,
             &Buffer,
             &BufferSize,
             &BmpHeight,
             &BmpWidth
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  // TODO: Update the lib?
  ASSERT ((UINT32)BmpHeight == BmpHeight);
  ASSERT ((UINT32)BmpWidth  == BmpWidth);

  Image->Height = (UINT32)BmpHeight;
  Image->Width  = (UINT32)BmpWidth;
  Image->Buffer = Buffer;
  return EFI_SUCCESS;
}

int main (int argc, char** argv)
{
  BOOLEAN Result;
  GUI_FONT_CONTEXT Context;
  uint8_t          *FontImage;
  uint32_t         FontImageSize;
  uint8_t          *FontMetrics;
  uint32_t         FontMetricsSize;
  GUI_IMAGE        Label;
  EFI_STATUS       Status;
  VOID             *BmpImage;
  UINT32           BmpImageSize;
  FILE *write_ptr;

  FontImage   = UserReadFile (argv[1], &FontImageSize);
  FontMetrics = UserReadFile (argv[2], &FontMetricsSize);
  Result      = GuiFontConstruct (&Context, FontImage, FontImageSize, FontMetrics, FontMetricsSize, 1);
  if (!Result) {
    DEBUG ((DEBUG_WARN, "BMF: Helvetica failed\n"));
    return -1;
  }
  
  Result = GuiGetLabel (&Label, &Context, L"Time Machine HD", sizeof ("Time Machine HD") - 1, FALSE);
  if (!Result) {
    DEBUG ((DEBUG_WARN, "BMF: label failed\n"));
    return -1;
  }

  DEBUG ((DEBUG_WARN, "Result: %u %u\n", Label.Height, Label.Width));

  BmpImage     = NULL;
  BmpImageSize = 0;
  Status = TranslateGopBltToBmp (
             Label.Buffer,
             Label.Height,
             Label.Width,
             &BmpImage,
             &BmpImageSize
             );
  if (EFI_ERROR (Status)) {
    return -1;
  }

  write_ptr = fopen ("Label.bmp", "wb");
  if (write_ptr != NULL) {
    fwrite (BmpImage, BmpImageSize, 1, write_ptr);
  }
  fclose (write_ptr);

  FreePool (BmpImage);

  FreePool (Context.FontImage.Buffer);
  FreePool (Label.Buffer);

  return 0;
}
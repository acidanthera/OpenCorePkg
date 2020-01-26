/*
clang -g -fshort-wchar -fsanitize=undefined,address -I$WORKSPACE/Public/Vendor/Acidanthera/OcSupportPkg/Include -I$WORKSPACE/Public/Vendor/Acidanthera/OcSupportPkg/TestsUser/Include -I$WORKSPACE/Public/edk2/MdePkg/Include/ -I$WORKSPACE/Public/edk2/MdeModulePkg/Include/ -include $WORKSPACE/Public/Vendor/Acidanthera/OcSupportPkg/TestsUser/Include/Base.h BitmapFontUser.c BitmapFont.c Images/Helvetica_bmp.c Images/Helvetica_fnt.c $WORKSPACE/Public/Vendor/Acidanthera/OcSupportPkg/Library/BaseBmpSupportLib/BmpSupportLib.c -o Bmf
*/

#include <stdio.h>

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BmpSupportLib.h>

#include "GUI.h"
#include "BmfLib.h"

RETURN_STATUS
GuiBmpToImage (
  IN OUT GUI_IMAGE  *Image,
  IN     VOID       *BmpImage,
  IN     UINTN      BmpImageSize
  )
{
  RETURN_STATUS                 Status;
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
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  // TODO: Update the lib?
  ASSERT ((UINT32)BmpHeight == BmpHeight);
  ASSERT ((UINT32)BmpWidth  == BmpWidth);

  Image->Height = (UINT32)BmpHeight;
  Image->Width  = (UINT32)BmpWidth;
  Image->Buffer = Buffer;
  return RETURN_SUCCESS;
}

int main (void)
{
  BOOLEAN Result;
  GUI_FONT_CONTEXT HelveticaContext;
  GUI_IMAGE        Label;
  RETURN_STATUS Status;
  VOID            *BmpImage;
  UINT32           BmpImageSize;
  FILE *write_ptr;

  Result = GuiInitializeFontHelvetica (&HelveticaContext);
  if (!Result) {
    DEBUG ((DEBUG_WARN, "BMF: Helvetica failed\n"));
    return -1;
  }
  
  Result = GuiGetLabel (&Label, &HelveticaContext, L"Time Machine HD", sizeof ("Time Machine HD") - 1);
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
  if (RETURN_ERROR (Status)) {
    return -1;
  }

  write_ptr = fopen ("Label.bmp", "wb");
  if (write_ptr != NULL) {
    fwrite (BmpImage, BmpImageSize, 1, write_ptr);
  }
  fclose (write_ptr);

  FreePool (BmpImage);

  FreePool (HelveticaContext.FontImage.Buffer);
  FreePool (Label.Buffer);

  return 0;
}
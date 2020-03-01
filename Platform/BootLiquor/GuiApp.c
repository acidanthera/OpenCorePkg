/** @file
  This file is part of BootLiquor, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/OcInterface.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcStorageLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "BootLiquor.h"
#include "BmfLib.h"
#include "GuiApp.h"

GLOBAL_REMOVE_IF_UNREFERENCED BOOT_PICKER_GUI_CONTEXT mGuiContext = { { { 0 } } };

STATIC
VOID
InternalSafeFreePool (
  IN CONST VOID  *Memory
  )
{
  if (Memory != NULL) {
    FreePool ((VOID *)Memory);
  }
}

STATIC
VOID
InternalContextDestruct (
  IN OUT BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  InternalSafeFreePool (Context->Cursor.Buffer);
  InternalSafeFreePool (Context->EntryBackSelected.Buffer);
  InternalSafeFreePool (Context->EntrySelector.BaseImage.Buffer);
  InternalSafeFreePool (Context->EntrySelector.HoldImage.Buffer);
  InternalSafeFreePool (Context->EntryIconInternal.Buffer);
  InternalSafeFreePool (Context->EntryIconInternal.Buffer);
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->FontContext.FontImage.Buffer);
}


RETURN_STATUS
LoadImageFromStorage (
  IN  OC_STORAGE_CONTEXT                   *Storage,
  IN  CONST CHAR16                         *ImageFilePath,
  OUT VOID                                 *Image,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel  OPTIONAL
  )
{
  VOID          *ImageData;
  UINT32        ImageSize;
  RETURN_STATUS Status;

  ImageData = OcStorageReadFileUnicode (Storage, ImageFilePath, &ImageSize);
  if (ImageData == NULL) {
    return EFI_NOT_FOUND;
  }

  if (HighlightPixel != NULL) {
    Status  = GuiPngToClickImage (Image, ImageData, ImageSize, HighlightPixel);
  } else {
    Status  = GuiPngToImage (Image, ImageData, ImageSize);
  }

  FreePool (ImageData);

  return Status;
}


RETURN_STATUS
LoadClickImageFromStorage (
  IN  OC_STORAGE_CONTEXT                  *Storage,
  IN  CONST CHAR16                        *ImageFilePath,
  OUT GUI_CLICK_IMAGE                     *Image,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *HighlightPixel
  )
{
  VOID          *ImageData;
  UINT32        ImageSize;
  RETURN_STATUS Status;
  ImageData = OcStorageReadFileUnicode (Storage, ImageFilePath, &ImageSize);
  Status  = GuiPngToClickImage (Image, ImageData, ImageSize, HighlightPixel);
  FreePool(ImageData);
  return Status;
}


RETURN_STATUS
InternalContextConstruct (
  OUT BOOT_PICKER_GUI_CONTEXT  *Context,
  IN  OC_STORAGE_CONTEXT       *Storage
  )
{
  STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL HighlightPixel = {
    0xAF, 0xAF, 0xAF, 0x32
  };

  RETURN_STATUS Status;
  BOOLEAN       Result;
  VOID          *FontImage;
  VOID          *FontData;
  UINT32        FontImageSize;
  UINT32        FontDataSize;

  ASSERT (Context != NULL);

  Context->BootEntry = NULL;

  Status  = LoadImageFromStorage (Storage, L"Resources\\Image\\Cursor.png", &Context->Cursor, NULL);
  Status |= LoadImageFromStorage (Storage, L"Resources\\Image\\Selected.png", &Context->EntryBackSelected, NULL);
  Status |= LoadImageFromStorage (Storage, L"Resources\\Image\\Selector.png", &Context->EntrySelector, &HighlightPixel);
  Status |= LoadImageFromStorage (Storage, L"Resources\\Image\\InternalHardDrive.png", &Context->EntryIconInternal, NULL);
  Status |= LoadImageFromStorage (Storage, L"Resources\\Image\\ExternalHardDrive.png", &Context->EntryIconExternal, NULL);

  if (RETURN_ERROR (Status)) {
    InternalContextDestruct (Context);
    return RETURN_UNSUPPORTED;
  }

  FontImage = OcStorageReadFileUnicode (Storage, L"Resources\\Font\\Font.png", &FontImageSize);
  FontData  = OcStorageReadFileUnicode (Storage, L"Resources\\Font\\Font.bin", &FontDataSize);

  if (FontImage != NULL && FontData != NULL) {
    Result = GuiFontConstruct (
      &Context->FontContext,
      FontImage,
      FontImageSize,
      FontData,
      FontDataSize
      );
  } else {
    Result = FALSE;
  }

  if (!Result) {
    DEBUG ((DEBUG_WARN, "BMF: Font failed\n"));
    InternalContextDestruct (Context);
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

CONST GUI_IMAGE *
InternalGetCursorImage (
  IN OUT GUI_SCREEN_CURSOR  *This,
  IN     VOID               *Context
  )
{
  CONST BOOT_PICKER_GUI_CONTEXT *GuiContext;

  ASSERT (This != NULL);
  ASSERT (Context != NULL);

  GuiContext = (CONST BOOT_PICKER_GUI_CONTEXT *)Context;
  return &GuiContext->Cursor;
}

EFI_STATUS
EFIAPI
UefiUnload (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  GuiLibDestruct ();
  return EFI_SUCCESS;
}

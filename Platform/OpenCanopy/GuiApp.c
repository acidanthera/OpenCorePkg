/** @file
  This file is part of OpenCanopy, OpenCore GUI.

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
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>

#include <Guid/AppleVariable.h>

#include "OpenCanopy.h"
#include "BmfLib.h"
#include "GuiApp.h"

GLOBAL_REMOVE_IF_UNREFERENCED BOOT_PICKER_GUI_CONTEXT mGuiContext = { { { 0 } } };

// FIXME: Move out.
EFI_STATUS
DecodeAppleDiskLabelImage (
  OUT GUI_IMAGE *Image,
  IN  UINT8     *RawData,
  IN  UINT32    DataLength
  );

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
  // TODO: maybe refactor it to an array?
  InternalSafeFreePool (Context->Cursor.Buffer);
  InternalSafeFreePool (Context->EntryBackSelected.Buffer);
  InternalSafeFreePool (Context->EntrySelector.BaseImage.Buffer);
  InternalSafeFreePool (Context->EntrySelector.HoldImage.Buffer);
  InternalSafeFreePool (Context->EntryIconInternal.Buffer);
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->EntryIconTool.Buffer);
  InternalSafeFreePool (Context->FontContext.FontImage.Buffer);
  InternalSafeFreePool (Context->EntryLabelEFIBoot.Buffer);
  InternalSafeFreePool (Context->EntryLabelWindows.Buffer);
  InternalSafeFreePool (Context->EntryLabelRecovery.Buffer);
  InternalSafeFreePool (Context->EntryLabelMacOS.Buffer);
  InternalSafeFreePool (Context->EntryLabelTool.Buffer);
  InternalSafeFreePool (Context->EntryLabelResetNVRAM.Buffer);
  /*
  InternalSafeFreePool (Context->Poof[0].Buffer);
  InternalSafeFreePool (Context->Poof[1].Buffer);
  InternalSafeFreePool (Context->Poof[2].Buffer);
  InternalSafeFreePool (Context->Poof[3].Buffer);
  InternalSafeFreePool (Context->Poof[4].Buffer);
  */
}

STATIC
RETURN_STATUS
LoadImageFileFromStorageForScale (
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  CONST CHAR8              *ImageFilePath,
  IN  CONST CHAR8              *ImageFileExt,
  IN  UINT8                    Scale,
  OUT VOID                     **FileData,
  OUT UINT32                   *FileSize
  )
{
  EFI_STATUS    Status;
  CHAR16        Path[OC_STORAGE_SAFE_PATH_MAX];

  Status = OcUnicodeSafeSPrint (
    Path,
    sizeof (Path),
    OPEN_CORE_IMAGE_PATH L"%a%a.%a",
    ImageFilePath,
    Scale == 2 ? "@2x" : "",
    ImageFileExt
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCCP: Cannot fit %a\n", ImageFilePath));
    return EFI_OUT_OF_RESOURCES;
  }

  *FileData = OcStorageReadFileUnicode (Storage, Path, FileSize);

  if (*FileData == NULL) {
    DEBUG ((DEBUG_WARN, "OCCP: Failed to load %s\n", Path));
    return RETURN_NOT_FOUND;
  }

  if (*FileSize == 0) {
    FreePool (*FileData);
    DEBUG ((DEBUG_WARN, "OCCP: Empty %s\n", Path));
    return RETURN_NOT_FOUND; 
  }

  return RETURN_SUCCESS;
}


STATIC
RETURN_STATUS
LoadLabelFileFromStorageForScale (
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  CONST CHAR8              *LabelFilePath,
  IN  UINT8                    Scale,
  OUT VOID                     **FileData,
  OUT UINT32                   *FileSize
  )
{
  EFI_STATUS    Status;
  CHAR16        Path[OC_STORAGE_SAFE_PATH_MAX];

  Status = OcUnicodeSafeSPrint (
    Path,
    sizeof (Path),
    OPEN_CORE_LABEL_PATH L"%a.%a",
    LabelFilePath,
    Scale == 2 ? "l2x" : "lbl"
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCCP: Cannot fit %a\n", LabelFilePath));
    return EFI_OUT_OF_RESOURCES;
  }

  *FileData = OcStorageReadFileUnicode (Storage, Path, FileSize);

  if (*FileData == NULL) {
    DEBUG ((DEBUG_WARN, "OCCP: Failed to load %s\n", Path));
    return RETURN_NOT_FOUND;
  }

  if (*FileSize == 0) {
    FreePool (*FileData);
    DEBUG ((DEBUG_WARN, "OCCP: Empty %s\n", Path));
    return RETURN_NOT_FOUND; 
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
LoadImageFromStorage (
  IN  OC_STORAGE_CONTEXT                   *Storage,
  IN  CONST CHAR8                          *ImageFilePath,
  IN  CONST CHAR8                          *ImageFileExt,
  IN  UINT8                                Scale,
  OUT VOID                                 *Image,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel  OPTIONAL
  )
{
  VOID          *ImageData;
  UINT32        ImageSize;
  RETURN_STATUS Status;

  Status = LoadImageFileFromStorageForScale (
    Storage,
    ImageFilePath,
    ImageFileExt,
    Scale,
    &ImageData,
    &ImageSize
    );
  if (ImageData == NULL) {
    return EFI_NOT_FOUND;
  }

  if (HighlightPixel != NULL) {
    Status  = GuiPngToClickImage (Image, ImageData, ImageSize, HighlightPixel);
  } else {
    Status  = GuiPngToImage (Image, ImageData, ImageSize);
  }

  FreePool (ImageData);

  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to decode image %a\n", ImageFilePath));
  }

  return Status;
}

RETURN_STATUS
LoadLabelFromStorage (
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  CONST CHAR8              *ImageFilePath,
  IN  UINT8                    Scale,
  OUT GUI_IMAGE                *Image
  )
{
  VOID          *ImageData;
  UINT32        ImageSize;
  RETURN_STATUS Status;

  Status = LoadLabelFileFromStorageForScale (Storage, ImageFilePath, Scale, &ImageData, &ImageSize);
  if (RETURN_ERROR(Status)) {
    return Status;
  }

  Status = DecodeAppleDiskLabelImage (Image, ImageData, ImageSize);

  FreePool (ImageData);

  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to decode image %s\n", ImageFilePath));
  }

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
  UINTN         UiScaleSize;

  ASSERT (Context != NULL);

  Context->BootEntry = NULL;

  Status  = LoadImageFromStorage(Storage, "Cursor",            "png", Context->Scale, &Context->Cursor, NULL);
  Status |= LoadImageFromStorage(Storage, "Selected",          "png", Context->Scale, &Context->EntryBackSelected, NULL);
  Status |= LoadImageFromStorage(Storage, "InternalHardDrive", "png", Context->Scale, &Context->EntryIconInternal, NULL);
  Status |= LoadImageFromStorage(Storage, "ExternalHardDrive", "png", Context->Scale, &Context->EntryIconExternal, NULL);
  Status |= LoadImageFromStorage(Storage, "Tool",              "png", Context->Scale, &Context->EntryIconTool, NULL);
  Status |= LoadImageFromStorage(Storage, "Selector",          "png", Context->Scale, &Context->EntrySelector, &HighlightPixel);

  // TODO: don't load prerendered labels if the user requested to always use font rendering. But we don't have picker context here
  Status |= LoadLabelFromStorage(Storage, "EFIBoot",     Context->Scale, &Context->EntryLabelEFIBoot);
  Status |= LoadLabelFromStorage(Storage, "Windows",     Context->Scale, &Context->EntryLabelWindows);
  Status |= LoadLabelFromStorage(Storage, "Recovery",    Context->Scale, &Context->EntryLabelRecovery);
  Status |= LoadLabelFromStorage(Storage, "ResetNVRAM",  Context->Scale, &Context->EntryLabelResetNVRAM);
  Status |= LoadLabelFromStorage(Storage, "Tool",        Context->Scale, &Context->EntryLabelTool);
  Status |= LoadLabelFromStorage(Storage, "macOS",       Context->Scale, &Context->EntryLabelMacOS);

  /*
  Status |= LoadImageFromStorage(Storage, "ToolbarPoof1128x128", "png", Context->Scale, &Context->Poof[0], NULL);
  Status |= LoadImageFromStorage(Storage, "ToolbarPoof2128x128", "png", Context->Scale, &Context->Poof[1], NULL);
  Status |= LoadImageFromStorage(Storage, "ToolbarPoof3128x128", "png", Context->Scale, &Context->Poof[2], NULL);
  Status |= LoadImageFromStorage(Storage, "ToolbarPoof4128x128", "png", Context->Scale, &Context->Poof[3], NULL);
  Status |= LoadImageFromStorage(Storage, "ToolbarPoof5128x128", "png", Context->Scale, &Context->Poof[4], NULL);
  */
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to load image\n"));
    InternalContextDestruct (Context);
    return RETURN_UNSUPPORTED;
  }

  FontImage = OcStorageReadFileUnicode (Storage, OPEN_CORE_FONT_PATH L"Font.png", &FontImageSize);
  FontData  = OcStorageReadFileUnicode (Storage, OPEN_CORE_FONT_PATH L"Font.bin", &FontDataSize);

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
  
  Context->Scale = 1;
  UiScaleSize = sizeof (Context->Scale);

  Status = gRT->GetVariable (
    APPLE_UI_SCALE_VARIABLE_NAME,
    &gAppleVendorVariableGuid,
    NULL,
    &UiScaleSize,
    (VOID *) &Context->Scale
    );

  if (EFI_ERROR (Status) || Context->Scale != 2) {
    Context->Scale = 1;
  }

  // FIXME: Add support for 2x scaling.
  Context->Scale = 1;

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

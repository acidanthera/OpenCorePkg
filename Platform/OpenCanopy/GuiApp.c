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
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->FontContext.FontImage.Buffer);
  // TODO: free all labels and icons
}

STATIC
RETURN_STATUS
LoadImageFileFromStorageForScale (
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  CONST CHAR16             *ImageFilePath,
  IN  UINT32                   Scale,
  OUT VOID                     **FileData,
  OUT UINT32                   *FileSize
  )
{
  UINTN         BufferSize;
  CHAR16        *Path;

  BufferSize = sizeof(CHAR16) * (StrLen (ImageFilePath) + 2);
  Path = AllocatePool (BufferSize);
  if (Path == NULL) {
    DEBUG ((DEBUG_ERROR, "Out of memory"));
    return RETURN_OUT_OF_RESOURCES;
  }

  // FIXME: what if format string is of wrong kind?
  UnicodeSPrint (Path, BufferSize, ImageFilePath, Scale == 2 ? L"@2x" : L"");

  *FileData = OcStorageReadFileUnicode (Storage, Path, FileSize);

  if (*FileData == NULL || *FileSize == 0) {
    DEBUG ((DEBUG_WARN, "Failed to load %s\n", Path));
    return RETURN_NOT_FOUND;
  }
  FreePool (Path);
  return RETURN_SUCCESS;
}


RETURN_STATUS
LoadImageFromStorage (
  IN  OC_STORAGE_CONTEXT                   *Storage,
  IN  CONST CHAR16                         *ImageFilePath,
  IN  UINT32                               Scale,
  OUT VOID                                 *Image,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel  OPTIONAL
  )
{
  VOID          *ImageData;
  UINT32        ImageSize;
  RETURN_STATUS Status;

  Status = LoadImageFileFromStorageForScale(Storage, ImageFilePath, Scale, &ImageData, &ImageSize);
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
    DEBUG ((DEBUG_WARN, "Failed to decode image %s\n", ImageFilePath));
  }

  return Status;
}


EFI_STATUS
DecodeAppleDiskLabelImage (
  OUT GUI_IMAGE *Image,
  IN  UINT8     *RawData,
  IN  UINT32    DataLength
  );


RETURN_STATUS
LoadLabelFromStorage (
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  CONST CHAR16             *ImageFilePath,
  IN  UINT32                   Scale,
  OUT GUI_IMAGE                *Image
  )
{
  VOID          *ImageData;
  UINT32        ImageSize;
  RETURN_STATUS Status;

  Status = LoadImageFileFromStorageForScale (Storage, ImageFilePath, Scale, &ImageData, &ImageSize);
  if (RETURN_ERROR(Status)) {
    return Status;
  }

  Status = DecodeAppleDiskLabelImage (Image, ImageData, ImageSize);
  FreePool (ImageData);
  if (RETURN_ERROR(Status)) {
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

  Status  = LoadImageFromStorage(Storage, L"Resources\\Image\\Cursor%s.png",            Context->Scale, &Context->Cursor, NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\Selected%s.png",          Context->Scale, &Context->EntryBackSelected, NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\InternalHardDrive%s.png", Context->Scale, &Context->EntryIconInternal, NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\ExternalHardDrive%s.png", Context->Scale, &Context->EntryIconExternal, NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\Tool%s.png",              Context->Scale, &Context->EntryIconTool, NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\Selector%s.png",     Context->Scale, &Context->EntrySelector, &HighlightPixel);

  Status |= LoadLabelFromStorage(Storage, L"Resources\\Label\\EFIBoot%s.disklabel",     Context->Scale, &Context->EntryLabelEFIBoot);
  Status |= LoadLabelFromStorage(Storage, L"Resources\\Label\\Windows%s.disklabel",     Context->Scale, &Context->EntryLabelWindows);
  Status |= LoadLabelFromStorage(Storage, L"Resources\\Label\\Recovery%s.disklabel",    Context->Scale, &Context->EntryLabelRecovery);
  Status |= LoadLabelFromStorage(Storage, L"Resources\\Label\\Tool%s.disklabel",        Context->Scale, &Context->EntryLabelTool);
  Status |= LoadLabelFromStorage(Storage, L"Resources\\Label\\ResetNVRAM%s.disklabel",  Context->Scale, &Context->EntryLabelResetNVRAM);
  Status |= LoadLabelFromStorage(Storage, L"Resources\\Label\\macOS%s.disklabel",       Context->Scale, &Context->EntryLabelMacOS);

  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\ToolbarPoof1128x128%s.png", Context->Scale, &Context->Poof[0], NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\ToolbarPoof2128x128%s.png", Context->Scale, &Context->Poof[1], NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\ToolbarPoof3128x128%s.png", Context->Scale, &Context->Poof[2], NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\ToolbarPoof4128x128%s.png", Context->Scale, &Context->Poof[3], NULL);
  Status |= LoadImageFromStorage(Storage, L"Resources\\Image\\ToolbarPoof5128x128%s.png", Context->Scale, &Context->Poof[4], NULL);
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to load image\n"));
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

/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <IndustryStandard/AppleIcon.h>
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
#include <Protocol/UserInterfaceTheme.h>

#include "OpenCanopy.h"
#include "BmfLib.h"
#include "GuiApp.h"

GLOBAL_REMOVE_IF_UNREFERENCED BOOT_PICKER_GUI_CONTEXT mGuiContext;

//
// FIXME: Should not be global here.
//
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mBackgroundPixel;
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mHighlightPixel = {0xAF, 0xAF, 0xAF, 0x32};
CONST GUI_IMAGE mBackgroundImage = { 1, 1, &mBackgroundPixel };

STATIC
CONST CHAR8 *
mLabelNames[LABEL_NUM_TOTAL] = {
  [LABEL_GENERIC_HDD]          = "EFIBoot",
  [LABEL_APPLE]                = "Apple",
  [LABEL_APPLE_RECOVERY]       = "AppleRecv",
  [LABEL_APPLE_TIME_MACHINE]   = "AppleTM",
  [LABEL_WINDOWS]              = "Windows",
  [LABEL_OTHER]                = "Other",
  [LABEL_TOOL]                 = "Tool",
  [LABEL_RESET_NVRAM]          = "ResetNVRAM",
  [LABEL_SHELL]                = "Shell"
};

STATIC
CONST CHAR8 *
mIconNames[ICON_NUM_TOTAL] = {
  [ICON_CURSOR]             = "Cursor",
  [ICON_SELECTED]           = "Selected",
  [ICON_SELECTOR]           = "Selector",
  [ICON_GENERIC_HDD]        = "HardDrive",
  [ICON_APPLE]              = "Apple",
  [ICON_APPLE_RECOVERY]     = "AppleRecv",
  [ICON_APPLE_TIME_MACHINE] = "AppleTM",
  [ICON_WINDOWS]            = "Windows",
  [ICON_OTHER]              = "Other",
  [ICON_TOOL]               = "Tool",
  [ICON_RESET_NVRAM]        = "ResetNVRAM",
  [ICON_SHELL]              = "Shell"
};

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
  UINT32  Index;
  UINT32  Index2;

  for (Index = 0; Index < ICON_NUM_TOTAL; ++Index) {
    for (Index2 = 0; Index2 < ICON_TYPE_COUNT; ++Index2) {
      InternalSafeFreePool (Context->Icons[Index][Index2].Buffer);
    }
  }

  for (Index = 0; Index < LABEL_NUM_TOTAL; ++Index) {
    InternalSafeFreePool (Context->Labels[Index].Buffer);
  }

  InternalSafeFreePool (Context->FontContext.FontImage.Buffer);
  /*
  InternalSafeFreePool (Context->Poof[0].Buffer);
  InternalSafeFreePool (Context->Poof[1].Buffer);
  InternalSafeFreePool (Context->Poof[2].Buffer);
  InternalSafeFreePool (Context->Poof[3].Buffer);
  InternalSafeFreePool (Context->Poof[4].Buffer);
  */
}

STATIC
EFI_STATUS
LoadImageFileFromStorage (
  OUT GUI_IMAGE                *Images,
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  CONST CHAR8              *ImageFilePath,
  IN  UINT8                    Scale,
  IN  UINT32                   MatchWidth,
  IN  UINT32                   MatchHeight,
  IN  BOOLEAN                  Icon,
  IN  BOOLEAN                  Old,
  IN  BOOLEAN                  AllowLessSize
  )
{
  EFI_STATUS    Status;
  CHAR16        Path[OC_STORAGE_SAFE_PATH_MAX];
  UINT8         *FileData;
  UINT32        FileSize;
  UINT32        ImageCount;
  UINT32        Index;

  ASSERT (ImageFilePath != NULL);
  ASSERT (Scale == 1 || Scale == 2);

  ImageCount = Icon ? ICON_TYPE_COUNT : 1; ///< Icons can be external.

  for (Index = 0; Index < ImageCount; ++Index) {
    Status = OcUnicodeSafeSPrint (
      Path,
      sizeof (Path),
      OPEN_CORE_IMAGE_PATH L"%a%a%a.icns",
      Old ? "Old" : "",
      Index > 0 ? "Ext" : "",
      ImageFilePath
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCUI: Cannot fit %a\n", ImageFilePath));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = EFI_NOT_FOUND;
    if (OcStorageExistsFileUnicode (Storage, Path)) {
      FileData = OcStorageReadFileUnicode (Storage, Path, &FileSize);
      if (FileData != NULL && FileSize > 0) {
        Status = GuiIcnsToImageIcon (
          &Images[Index],
          FileData,
          FileSize,
          Scale,
          MatchWidth,
          MatchHeight,
          AllowLessSize
          );
      }

      if (FileData != NULL) {
        FreePool (FileData);
      }
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCUI: Failed to load image (%u/%u) %s old:%d icon:%d - %r\n",
        Index+1,
        ImageCount,
        Path,
        Old,
        Icon,
        Status
        ));
      return Index == ICON_TYPE_BASE ? EFI_NOT_FOUND : EFI_SUCCESS;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
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

  ASSERT (Scale == 1 || Scale == 2);

  Status = OcUnicodeSafeSPrint (
    Path,
    sizeof (Path),
    OPEN_CORE_LABEL_PATH L"%a.%a",
    LabelFilePath,
    Scale == 2 ? "l2x" : "lbl"
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCUI: Cannot fit %a\n", LabelFilePath));
    return EFI_OUT_OF_RESOURCES;
  }

  *FileData = OcStorageReadFileUnicode (Storage, Path, FileSize);

  if (*FileData == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to load %s\n", Path));
    return EFI_NOT_FOUND;
  }

  if (*FileSize == 0) {
    FreePool (*FileData);
    DEBUG ((DEBUG_WARN, "OCUI: Empty %s\n", Path));
    return EFI_NOT_FOUND; 
  }

  return EFI_SUCCESS;
}

EFI_STATUS
LoadLabelFromStorage (
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  CONST CHAR8              *ImageFilePath,
  IN  UINT8                    Scale,
  IN  BOOLEAN                  Inverted,
  OUT GUI_IMAGE                *Image
  )
{
  VOID           *ImageData;
  UINT32         ImageSize;
  EFI_STATUS     Status;

  ASSERT (Scale == 1 || Scale == 2);

  Status = LoadLabelFileFromStorageForScale (Storage, ImageFilePath, Scale, &ImageData, &ImageSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GuiLabelToImage (Image, ImageData, ImageSize, Scale, Inverted);

  FreePool (ImageData);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to decode label %a - %r\n", ImageFilePath, Status));
  }

  return Status;
}

EFI_STATUS
InternalContextConstruct (
  OUT BOOT_PICKER_GUI_CONTEXT  *Context,
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  OC_PICKER_CONTEXT        *Picker
  )
{
  EFI_STATUS                         Status;
  EFI_USER_INTERFACE_THEME_PROTOCOL  *UiTheme;
  VOID                               *FontImage;
  VOID                               *FontData;
  UINT32                             FontImageSize;
  UINT32                             FontDataSize;
  UINTN                              UiScaleSize;
  UINT32                             Index;
  UINT32                             ImageDimension;
  BOOLEAN                            Old;
  BOOLEAN                            Result;

  ASSERT (Context != NULL);

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

  Status = gBS->LocateProtocol (
    &gEfiUserInterfaceThemeProtocolGuid,
    NULL,
    (VOID **) &UiTheme
    );
  if (!EFI_ERROR (Status)) {
    Status = UiTheme->GetBackgroundColor (&Context->BackgroundColor.Raw);
  }

  if (EFI_ERROR (Status)) {
    Context->BackgroundColor.Raw = APPLE_COLOR_SYRAH_BLACK;
  }

  //
  // Set background colour with full opacity.
  //
  mBackgroundPixel.Red      = Context->BackgroundColor.Pixel.Red;
  mBackgroundPixel.Green    = Context->BackgroundColor.Pixel.Green;
  mBackgroundPixel.Blue     = Context->BackgroundColor.Pixel.Blue;
  mBackgroundPixel.Reserved = 0xFF;

  Old = Context->BackgroundColor.Raw == APPLE_COLOR_LIGHT_GRAY;
  if ((Picker->PickerAttributes & OC_ATTR_USE_ALTERNATE_ICONS) != 0) {
    Old = !Old;
  }

  if (Context->BackgroundColor.Raw == APPLE_COLOR_SYRAH_BLACK) {
    Context->LightBackground = FALSE;
  } else if (Context->BackgroundColor.Raw == APPLE_COLOR_LIGHT_GRAY) {
    Context->LightBackground = TRUE;
  } else {
    //
    // FIXME: Support proper W3C formula.
    //
    Context->LightBackground = (Context->BackgroundColor.Pixel.Red * 299U
      + Context->BackgroundColor.Pixel.Green * 587U
      + Context->BackgroundColor.Pixel.Blue * 114U) >= 186000;
  }

  Context->BootEntry = NULL;

  Status = EFI_SUCCESS;

  for (Index = 0; Index < ICON_NUM_TOTAL; ++Index) {
    if (Index == ICON_CURSOR) {
      ImageDimension = MAX_CURSOR_DIMENSION;
    } else if (Index == ICON_SELECTED) {
      ImageDimension = BOOT_SELECTOR_BACKGROUND_DIMENSION;
    } else if (Index == ICON_SELECTOR) {
      ImageDimension = BOOT_SELECTOR_BUTTON_DIMENSION;
    } else {
      ImageDimension = BOOT_ENTRY_ICON_DIMENSION;
    }

    Status = LoadImageFileFromStorage (
      Context->Icons[Index],
      Storage,
      mIconNames[Index],
      Context->Scale,
      ImageDimension,
      ImageDimension,
      Index >= ICON_NUM_SYS,
      Old,
      Index == ICON_CURSOR
      );

    if (!EFI_ERROR (Status) && Index == ICON_SELECTOR) {
      Status = GuiCreateHighlightedImage (
        &Context->Icons[Index][ICON_TYPE_HELD],
        &Context->Icons[Index][ICON_TYPE_BASE],
        &mHighlightPixel
        );
    }

    //
    // For generic disk icon being able to distinguish internal and external
    // disk icons is a security requirement. These icons are used whenever
    // 'typed' external icons are not available.
    //
    if (!EFI_ERROR (Status)
      && Index == ICON_GENERIC_HDD
      && Context->Icons[Index][ICON_TYPE_EXTERNAL].Buffer == NULL) {
      Status = EFI_NOT_FOUND;
      DEBUG ((DEBUG_WARN, "OCUI: Missing external disk icon\n"));
      break;
    }

    if (EFI_ERROR (Status) && Index < ICON_NUM_MANDATORY) {
      break;
    }

    Status = EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < LABEL_NUM_TOTAL; ++Index) {
      Status |= LoadLabelFromStorage (
        Storage,
        mLabelNames[Index],
        Context->Scale,
        Context->LightBackground,
        &Context->Labels[Index]
        );
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to load images\n"));
    InternalContextDestruct (Context);
    return EFI_UNSUPPORTED;
  }

  if (Context->Scale == 2) {
    FontImage = OcStorageReadFileUnicode (Storage, OPEN_CORE_FONT_PATH L"Font_2x.png", &FontImageSize);
    FontData  = OcStorageReadFileUnicode (Storage, OPEN_CORE_FONT_PATH L"Font_2x.bin", &FontDataSize);
  } else {
    FontImage = OcStorageReadFileUnicode (Storage, OPEN_CORE_FONT_PATH L"Font_1x.png", &FontImageSize);
    FontData  = OcStorageReadFileUnicode (Storage, OPEN_CORE_FONT_PATH L"Font_1x.bin", &FontDataSize);
  }

  if (FontImage != NULL && FontData != NULL) {
    Result = GuiFontConstruct (
      &Context->FontContext,
      FontImage,
      FontImageSize,
      FontData,
      FontDataSize
      );
    if (Context->FontContext.BmfContext.Height != BOOT_ENTRY_LABEL_HEIGHT * Context->Scale) {
        DEBUG((
          DEBUG_WARN,
          "OCUI: Font has height %d instead of %d\n",
          Context->FontContext.BmfContext.Height,
          BOOT_ENTRY_LABEL_HEIGHT * Context->Scale
          ));
      Result = FALSE;
    }
  } else {
    Result = FALSE;
  }

  if (!Result) {
    DEBUG ((DEBUG_WARN, "OCUI: Font init failed\n"));
    InternalContextDestruct (Context);
    return EFI_UNSUPPORTED;
  }
  
  return EFI_SUCCESS;
}

CONST GUI_IMAGE *
InternalGetCursorImage (
  IN OUT GUI_SCREEN_CURSOR       *This,
  IN     BOOT_PICKER_GUI_CONTEXT *Context
  )
{
  ASSERT (This != NULL);
  ASSERT (Context != NULL);

  return &Context->Icons[ICON_CURSOR][ICON_TYPE_BASE];
}

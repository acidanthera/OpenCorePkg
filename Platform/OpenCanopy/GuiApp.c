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
STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mHighlightPixel = {0xAF, 0xAF, 0xAF, 0x32};

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
  [LABEL_SHELL]                = "Shell",
  [LABEL_SIP_IS_ENABLED]       = "SIPEnabled",
  [LABEL_SIP_IS_DISABLED]      = "SIPDisabled"
};

STATIC
CONST CHAR8 *
mIconNames[ICON_NUM_TOTAL] = {
  [ICON_CURSOR]             = "Cursor",
  [ICON_SELECTED]           = "Selected",
  [ICON_SELECTOR]           = "Selector",
  [ICON_SET_DEFAULT]        = "SetDefault",
  [ICON_LEFT]               = "Left",
  [ICON_RIGHT]              = "Right",
  [ICON_SHUT_DOWN]          = "ShutDown",
  [ICON_RESTART]            = "Restart",
  [ICON_BUTTON_FOCUS]       = "BtnFocus",
  [ICON_PASSWORD]           = "Password",
  [ICON_DOT]                = "Dot",
  [ICON_ENTER]              = "Enter",
  [ICON_LOCK]               = "Lock",
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

  InternalSafeFreePool (Context->Background.Buffer);
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
  IN  CONST CHAR8              *Prefix,
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
      OPEN_CORE_IMAGE_PATH L"%a\\%a%a.icns",
      Prefix,
      Index > 0 ? "Ext" : "",
      ImageFilePath
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCUI: Cannot fit %a\n", ImageFilePath));
      return EFI_OUT_OF_RESOURCES;
    }

    UnicodeUefiSlashes (Path);
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
        "OCUI: Failed to load image (%u/%u) %s prefix:%a icon:%d - %r\n",
        Index+1,
        ImageCount,
        Path,
        Prefix,
        Icon,
        Status
        ));
      if (Index == ICON_TYPE_BASE) {
        STATIC_ASSERT (
          ICON_TYPE_BASE == 0,
          "Memory may be leaked due to previously loaded images."
          );
        return EFI_NOT_FOUND;
      }

      Images[Index].Width  = 0;
      Images[Index].Height = 0;
      Images[Index].Buffer = NULL;
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
InternalGetFlavourIcon (
  IN  BOOT_PICKER_GUI_CONTEXT       *GuiContext,
  IN  OC_STORAGE_CONTEXT            *Storage,
  IN  CHAR8                         *FlavourName,
  IN  UINTN                         FlavourNameLen,
  IN  UINT32                        IconTypeIndex,
  IN  BOOLEAN                       UseFlavourIcon,
  OUT GUI_IMAGE                     *EntryIcon,
  OUT BOOLEAN                       *CustomIcon
  )
{
  EFI_STATUS              Status;
  CHAR16                  Path[OC_STORAGE_SAFE_PATH_MAX];
  CHAR8                   ImageName[OC_MAX_CONTENT_FLAVOUR_SIZE];
  UINT8                   *FileData;
  UINT32                  FileSize;
  UINTN                   Index;

  ASSERT (EntryIcon != NULL);
  ASSERT (CustomIcon != NULL);

  if (FlavourNameLen == 0 ||
    OcAsciiStrniCmp (FlavourName, OC_FLAVOUR_AUTO, FlavourNameLen) == 0
    ) {
    return EFI_UNSUPPORTED;
  }

  //
  // Look in preloaded icons
  //
  for (Index = ICON_NUM_SYS; Index < ICON_NUM_TOTAL; ++Index) {
    if (OcAsciiStrniCmp (FlavourName, mIconNames[Index], FlavourNameLen) == 0) {
      if (GuiContext->Icons[Index][IconTypeIndex].Buffer != NULL) {
        CopyMem (EntryIcon, &GuiContext->Icons[Index][IconTypeIndex], sizeof (*EntryIcon));
        *CustomIcon = FALSE;
        return EFI_SUCCESS;
      }
      break;
    }
  }

  //
  // Look for custom icon
  //
  if (!UseFlavourIcon) {
    return EFI_NOT_FOUND;
  }

  AsciiStrnCpyS (ImageName, OC_MAX_CONTENT_FLAVOUR_SIZE, FlavourName, FlavourNameLen);
  Status = OcUnicodeSafeSPrint (
    Path,
    sizeof (Path),
    OPEN_CORE_IMAGE_PATH L"%a\\%a%a.icns",
    GuiContext->Prefix,
    IconTypeIndex > 0 ? "Ext" : "",
    ImageName
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCUI: Cannot fit %a\n", ImageName));
    return Status;
  }

  UnicodeUefiSlashes (Path);
  DEBUG ((DEBUG_INFO, "OCUI: Trying flavour icon %s\n", Path));

  Status = EFI_NOT_FOUND;
  if (OcStorageExistsFileUnicode (Storage, Path)) {
    FileData = OcStorageReadFileUnicode (Storage, Path, &FileSize);
    if (FileData != NULL && FileSize > 0) {
      Status = GuiIcnsToImageIcon (
        EntryIcon,
        FileData,
        FileSize,
        GuiContext->Scale,
        BOOT_ENTRY_ICON_DIMENSION,
        BOOT_ENTRY_ICON_DIMENSION,
        FALSE
        );
    }

    if (FileData != NULL) {
      FreePool (FileData);
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCUI: Invalid icon file\n"));
    }
  }

  if (!EFI_ERROR (Status)) {
    ASSERT (EntryIcon->Buffer != NULL);
    *CustomIcon = TRUE;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
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
  UINT32                             ImageWidth;
  UINT32                             ImageHeight;
  BOOLEAN                            Result;
  BOOLEAN                            AllowLessSize;

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

  if (AsciiStrCmp (Picker->PickerVariant, "Auto") == 0) {
    if (Context->BackgroundColor.Raw == APPLE_COLOR_LIGHT_GRAY) {
      Context->Prefix = "Acidanthera\\Chardonnay";
    } else {
      Context->Prefix = "Acidanthera\\GoldenGate";
    }
  } else if (AsciiStrCmp (Picker->PickerVariant, "Default") == 0) {
    Context->Prefix = "Acidanthera\\GoldenGate";
  } else {
    Context->Prefix = Picker->PickerVariant;
  }

  LoadImageFileFromStorage (
    &Context->Background,
    Storage,
    "Background",
    Context->Scale,
    0,
    0,
    FALSE,
    Context->Prefix,
    FALSE
    );

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
  //
  // Set background colour with full opacity.
  //
  Context->BackgroundColor.Pixel.Reserved = 0xFF;

  for (Index = 0; Index < ICON_NUM_TOTAL; ++Index) {
    AllowLessSize = FALSE;
    if (Index == ICON_CURSOR) {
      ImageWidth  = MAX_CURSOR_DIMENSION;
      ImageHeight = MAX_CURSOR_DIMENSION;
      AllowLessSize = TRUE;
    } else if (Index == ICON_SELECTED) {
      ImageWidth  = BOOT_SELECTOR_BACKGROUND_DIMENSION;
      ImageHeight = BOOT_SELECTOR_BACKGROUND_DIMENSION;
    } else if (Index == ICON_SELECTOR || Index == ICON_SET_DEFAULT) {
      ImageWidth  = BOOT_SELECTOR_BUTTON_WIDTH;
      ImageHeight = BOOT_SELECTOR_BUTTON_HEIGHT;
      AllowLessSize = TRUE;
    } else if (Index == ICON_LEFT || Index == ICON_RIGHT) {
      ImageWidth  = BOOT_SCROLL_BUTTON_DIMENSION;
      ImageHeight = BOOT_SCROLL_BUTTON_DIMENSION;
    } else if (Index == ICON_SHUT_DOWN || Index == ICON_RESTART) {
      ImageWidth  = BOOT_ACTION_BUTTON_DIMENSION;
      ImageHeight = BOOT_ACTION_BUTTON_DIMENSION;
      AllowLessSize = TRUE;
    } else if (Index == ICON_BUTTON_FOCUS) {
      ImageWidth  = BOOT_ACTION_BUTTON_FOCUS_DIMENSION;
      ImageHeight = BOOT_ACTION_BUTTON_FOCUS_DIMENSION;
      AllowLessSize = TRUE;
    } else if (Index == ICON_PASSWORD) {
      ImageWidth  = PASSWORD_BOX_WIDTH;
      ImageHeight = PASSWORD_BOX_HEIGHT;
      AllowLessSize = TRUE;
    } else if (Index == ICON_LOCK) {
      ImageWidth  = PASSWORD_LOCK_DIMENSION;
      ImageHeight = PASSWORD_LOCK_DIMENSION;
      AllowLessSize = TRUE;
    } else if (Index == ICON_ENTER) {
      ImageWidth  = PASSWORD_ENTER_WIDTH;
      ImageHeight = PASSWORD_ENTER_HEIGHT;
      AllowLessSize = TRUE;
    } else if (Index == ICON_DOT) {
      ImageWidth  = PASSWORD_DOT_DIMENSION;
      ImageHeight = PASSWORD_DOT_DIMENSION;
      AllowLessSize = TRUE;
    } else {
      ImageWidth  = BOOT_ENTRY_ICON_DIMENSION;
      ImageHeight = BOOT_ENTRY_ICON_DIMENSION;
    }

    Status = LoadImageFileFromStorage (
      Context->Icons[Index],
      Storage,
      mIconNames[Index],
      Context->Scale,
      ImageWidth,
      ImageHeight,
      Index >= ICON_NUM_SYS,
      Context->Prefix,
      AllowLessSize
      );
    if (!EFI_ERROR (Status)) {
      if (Index == ICON_SELECTOR || Index == ICON_SET_DEFAULT || Index == ICON_LEFT || Index == ICON_RIGHT || Index == ICON_SHUT_DOWN || Index == ICON_RESTART || Index == ICON_ENTER) {
        Status = GuiCreateHighlightedImage (
          &Context->Icons[Index][ICON_TYPE_HELD],
          &Context->Icons[Index][ICON_TYPE_BASE],
          &mHighlightPixel
          );
        if (Index == ICON_SET_DEFAULT) {
          if (Context->Icons[Index]->Width != Context->Icons[ICON_SELECTOR]->Width) {
            Status = EFI_UNSUPPORTED;
            DEBUG ((
              DEBUG_WARN,
              "OCUI: %a width %upx != %a width %upx\n",
              mIconNames[Index],
              Context->Icons[Index]->Width,
              mIconNames[ICON_SELECTOR],
              Context->Icons[ICON_SELECTOR]->Width
              ));
            STATIC_ASSERT (
              ICON_SELECTOR < ICON_SET_DEFAULT,
              "Selector must be loaded before SetDefault."
              );
          }
        }
      } else if (Index == ICON_GENERIC_HDD
              && Context->Icons[Index][ICON_TYPE_EXTERNAL].Buffer == NULL) {
        //
        // For generic disk icon being able to distinguish internal and external
        // disk icons is a security requirement. These icons are used whenever
        // 'typed' external icons are not available.
        //
        Status = EFI_NOT_FOUND;
        DEBUG ((DEBUG_WARN, "OCUI: Missing external disk icon\n"));
        STATIC_ASSERT (
          ICON_GENERIC_HDD < ICON_NUM_MANDATORY,
          "The base icon should must be cleaned up explicitly."
          );
      } else if (Index == ICON_CURSOR) {
        if (Context->Icons[ICON_CURSOR][ICON_TYPE_BASE].Width < MIN_CURSOR_DIMENSION * Context->Scale
         || Context->Icons[ICON_CURSOR][ICON_TYPE_BASE].Height < MIN_CURSOR_DIMENSION * Context->Scale) {
          DEBUG ((
            DEBUG_INFO,
            "OCUI: Expected at least %dx%d for cursor, actual %dx%d\n",
             MIN_CURSOR_DIMENSION * Context->Scale,
             MIN_CURSOR_DIMENSION * Context->Scale,
             Context->Icons[ICON_CURSOR][ICON_TYPE_BASE].Width,
             Context->Icons[ICON_CURSOR][ICON_TYPE_BASE].Height
            ));
          Status = EFI_UNSUPPORTED;
        }
      }
    } else {
      ZeroMem (&Context->Icons[Index], sizeof (Context->Icons[Index]));
    }

    if (EFI_ERROR (Status) && Index < ICON_NUM_MANDATORY) {
      DEBUG ((DEBUG_WARN, "OCUI: Failed to load images for %a\n", Context->Prefix));
      InternalContextDestruct (Context);
      return EFI_UNSUPPORTED;
    }
  }

  for (Index = 0; Index < LABEL_NUM_TOTAL; ++Index) {
    Status = LoadLabelFromStorage (
      Storage,
      mLabelNames[Index],
      Context->Scale,
      Context->LightBackground,
      &Context->Labels[Index]
      );
    if (EFI_ERROR (Status)) {
      Context->Labels[Index].Buffer = NULL;
      DEBUG ((DEBUG_WARN, "OCUI: Failed to load images\n"));
      InternalContextDestruct (Context);
      return EFI_UNSUPPORTED;
    }
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
      FontDataSize,
      Context->Scale 
      );
    if (Context->FontContext.BmfContext.Height != (BOOT_ENTRY_LABEL_HEIGHT - BOOT_ENTRY_LABEL_TEXT_OFFSET) * Context->Scale) {
        DEBUG((
          DEBUG_WARN,
          "OCUI: Font has height %d instead of %d\n",
          Context->FontContext.BmfContext.Height,
          (BOOT_ENTRY_LABEL_HEIGHT - BOOT_ENTRY_LABEL_TEXT_OFFSET) * Context->Scale
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
  IN BOOT_PICKER_GUI_CONTEXT *Context
  )
{
  ASSERT (Context != NULL);
  //
  // ATTENTION: All images must have the same dimensions.
  //
  return &Context->Icons[ICON_CURSOR][ICON_TYPE_BASE];
}

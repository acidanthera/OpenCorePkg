/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef GUI_APP_H
#define GUI_APP_H

#include "OpenCanopy.h"
#include "BmfLib.h"

#include <Library/OcBootManagementLib.h>

#define MAX_CURSOR_DIMENSION  144U

#define BOOT_ENTRY_DIMENSION       144U
#define BOOT_ENTRY_ICON_DIMENSION  APPLE_DISK_ICON_DIMENSION
#define BOOT_ENTRY_LABEL_SPACE     4U
#define BOOT_ENTRY_LABEL_HEIGHT    12U

#define BOOT_ENTRY_SPACE  8U

#define BOOT_SELECTOR_WIDTH                 144U
#define BOOT_SELECTOR_BACKGROUND_DIMENSION  BOOT_SELECTOR_WIDTH
#define BOOT_SELECTOR_BUTTON_DIMENSION      40U
#define BOOT_SELECTOR_BUTTON_SPACE          (BOOT_ENTRY_LABEL_SPACE + BOOT_ENTRY_LABEL_HEIGHT + 3)
#define BOOT_SELECTOR_HEIGHT                (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE + BOOT_SELECTOR_BUTTON_DIMENSION)

#define BOOT_ENTRY_WIDTH   (BOOT_ENTRY_DIMENSION)
#define BOOT_ENTRY_HEIGHT  (BOOT_ENTRY_DIMENSION + BOOT_ENTRY_LABEL_SPACE + BOOT_ENTRY_LABEL_HEIGHT)

typedef enum {
  LABEL_GENERIC_HDD,
  LABEL_APPLE,
  LABEL_APPLE_RECOVERY,
  LABEL_APPLE_TIME_MACHINE,
  LABEL_WINDOWS,
  LABEL_OTHER,
  LABEL_TOOL,
  LABEL_RESET_NVRAM,
  LABEL_SHELL,
  LABEL_NUM_TOTAL
} LABEL_TARGET;

typedef enum {
  ICON_CURSOR,
  ICON_SELECTED,
  ICON_SELECTOR,
  ICON_NUM_SYS,
  ICON_GENERIC_HDD       = ICON_NUM_SYS,
  ICON_NUM_MANDATORY,
  ICON_APPLE             = ICON_NUM_MANDATORY,
  ICON_APPLE_RECOVERY,
  ICON_APPLE_TIME_MACHINE,
  ICON_WINDOWS,
  ICON_OTHER,
  ICON_TOOL,
  ICON_RESET_NVRAM,
  ICON_SHELL,
  ICON_NUM_TOTAL
} ICON_TARGET;

typedef enum {
  ICON_TYPE_BASE     = 0,
  ICON_TYPE_EXTERNAL = 1,
  ICON_TYPE_HELD     = 1,
  ICON_TYPE_COUNT    = 2,
} ICON_TYPE;

typedef struct _BOOT_PICKER_GUI_CONTEXT {
  GUI_IMAGE                            Icons[ICON_NUM_TOTAL][ICON_TYPE_COUNT];
  GUI_IMAGE                            Labels[LABEL_NUM_TOTAL];
  // GUI_IMAGE                         Poof[5];
  GUI_FONT_CONTEXT                     FontContext;
  VOID                                 *BootEntry;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  BackgroundColor;
  BOOLEAN                              HideAuxiliary;
  BOOLEAN                              Refresh;
  BOOLEAN                              LightBackground;
  BOOLEAN                              DoneIntroAnimation;
  UINT8                                Scale;
  UINT32                               CursorDefaultX;
  UINT32                               CursorDefaultY;
} BOOT_PICKER_GUI_CONTEXT;

EFI_STATUS
BootPickerViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  GUI_CURSOR_GET_IMAGE     GetCursorImage
  );

EFI_STATUS
BootPickerEntriesAdd (
  IN OC_PICKER_CONTEXT              *Context,
  IN CONST BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN OC_BOOT_ENTRY                  *Entry,
  IN BOOLEAN                        Default
  );

VOID
BootPickerViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN OUT BOOT_PICKER_GUI_CONTEXT  *GuiContext
  );

CONST GUI_IMAGE *
InternalGetCursorImage (
  IN OUT GUI_SCREEN_CURSOR        *This,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context
  );

#endif // GUI_APP_H

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

#define BOOT_CURSOR_OFFSET  4U

#define MAX_CURSOR_DIMENSION  144U
#define MIN_CURSOR_DIMENSION  BOOT_CURSOR_OFFSET

#define BOOT_ENTRY_DIMENSION          144U
#define BOOT_ENTRY_ICON_DIMENSION     APPLE_DISK_ICON_DIMENSION
#define BOOT_ENTRY_ICON_SPACE         ((BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) / 2)
#define BOOT_ENTRY_LABEL_SPACE        4U
#define BOOT_ENTRY_LABEL_HEIGHT       12U
#define BOOT_ENTRY_LABEL_TEXT_OFFSET  2U

#define BOOT_ENTRY_SPACE  8U

#define BOOT_SELECTOR_WIDTH                 144U
#define BOOT_SELECTOR_BACKGROUND_DIMENSION  BOOT_SELECTOR_WIDTH
#define BOOT_SELECTOR_BUTTON_WIDTH          BOOT_SELECTOR_WIDTH
#define BOOT_SELECTOR_BUTTON_HEIGHT         40U
#define BOOT_SELECTOR_BUTTON_SPACE          (BOOT_ENTRY_LABEL_SPACE + BOOT_ENTRY_LABEL_HEIGHT + 3)
#define BOOT_SELECTOR_HEIGHT                (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE + BOOT_SELECTOR_BUTTON_HEIGHT)

#define BOOT_ENTRY_WIDTH   (BOOT_ENTRY_DIMENSION)
#define BOOT_ENTRY_HEIGHT  (BOOT_ENTRY_DIMENSION + BOOT_ENTRY_LABEL_SPACE + BOOT_ENTRY_LABEL_HEIGHT)

#define BOOT_SCROLL_BUTTON_DIMENSION  40U
#define BOOT_SCROLL_BUTTON_SPACE      40U

#define BOOT_ACTION_BUTTON_DIMENSION        128U
#define BOOT_ACTION_BUTTON_FOCUS_DIMENSION  144U
#define BOOT_ACTION_BUTTON_SPACE            36U

#define PASSWORD_LOCK_DIMENSION 144U

#define PASSWORD_ENTER_WIDTH  75U
#define PASSWORD_ENTER_HEIGHT 30U

#define PASSWORD_BOX_WIDTH   288U
#define PASSWORD_BOX_HEIGHT  30U

#define PASSWORD_DOT_DIMENSION  10U

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
  LABEL_SIP_IS_ENABLED,
  LABEL_SIP_IS_DISABLED,
  LABEL_NUM_TOTAL
} LABEL_TARGET;

typedef enum {
  ICON_CURSOR,
  ICON_SELECTED,
  ICON_SELECTOR,
  ICON_SET_DEFAULT,
  ICON_LEFT,
  ICON_RIGHT,
  ICON_SHUT_DOWN,
  ICON_RESTART,
  ICON_BUTTON_FOCUS,
  ICON_PASSWORD,
  ICON_DOT,
  ICON_ENTER,
  ICON_LOCK,
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

enum {
  CanopyVoSelectedEntry,
  CanopyVoFocusPassword,
  CanopyVoFocusShutDown,
  CanopyVoFocusRestart
};

typedef struct _BOOT_PICKER_GUI_CONTEXT {
  GUI_IMAGE                            Background;
  GUI_IMAGE                            Icons[ICON_NUM_TOTAL][ICON_TYPE_COUNT];
  GUI_IMAGE                            Labels[LABEL_NUM_TOTAL];
  // GUI_IMAGE                         Poof[5];
  GUI_FONT_CONTEXT                     FontContext;
  CONST CHAR8                          *Prefix;
  VOID                                 *BootEntry;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  BackgroundColor;
  BOOLEAN                              HideAuxiliary;
  BOOLEAN                              Refresh;
  BOOLEAN                              LightBackground;
  BOOLEAN                              DoneIntroAnimation;
  BOOLEAN                              ReadyToBoot;
  UINT8                                Scale;
  UINT8                                VoAction;
  INT32                                CursorOffsetX;
  INT32                                CursorOffsetY;
  INT32                                AudioPlaybackTimeout;
  OC_PICKER_CONTEXT                    *PickerContext;
} BOOT_PICKER_GUI_CONTEXT;

EFI_STATUS
PasswordViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext
  );

VOID
PasswordViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN OUT BOOT_PICKER_GUI_CONTEXT  *GuiContext
  );

EFI_STATUS
BootPickerViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  GUI_CURSOR_GET_IMAGE     GetCursorImage,
  IN  UINT8                    NumBootEntries
  );

VOID
BootPickerViewLateInitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN     UINT8                    DefaultIndex
  );

EFI_STATUS
BootPickerEntriesSet (
  IN OC_PICKER_CONTEXT              *Context,
  IN BOOT_PICKER_GUI_CONTEXT        *GuiContext,
  IN OC_BOOT_ENTRY                  *Entry,
  IN UINT8                          EntryIndex
  );

VOID
BootPickerViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN OUT BOOT_PICKER_GUI_CONTEXT  *GuiContext
  );

CONST GUI_IMAGE *
InternalGetCursorImage (
  IN BOOT_PICKER_GUI_CONTEXT  *Context
  );

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
  );

#endif // GUI_APP_H

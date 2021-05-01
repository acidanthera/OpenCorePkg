/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef GUI_IO_H
#define GUI_IO_H

#include <Library/OcBootManagementLib.h>
#include <Protocol/GraphicsOutput.h>
#include "OpenCanopy.h"

typedef struct GUI_OUTPUT_CONTEXT_  GUI_OUTPUT_CONTEXT;
typedef struct GUI_POINTER_CONTEXT_ GUI_POINTER_CONTEXT;
typedef struct GUI_KEY_CONTEXT_     GUI_KEY_CONTEXT;

GUI_OUTPUT_CONTEXT *
GuiOutputConstruct (
  IN UINT32  Scale
  );

EFI_STATUS
EFIAPI
GuiOutputBlt (
  IN GUI_OUTPUT_CONTEXT                 *Context,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN UINTN                              SourceX,
  IN UINTN                              SourceY,
  IN UINTN                              DestinationX,
  IN UINTN                              DestinationY,
  IN UINTN                              Width,
  IN UINTN                              Height,
  IN UINTN                              Delta OPTIONAL
  );

CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *
GuiOutputGetInfo (
  IN GUI_OUTPUT_CONTEXT  *Context
  );

VOID
GuiOutputDestruct (
  IN GUI_OUTPUT_CONTEXT  *Context
  );

BOOLEAN
GuiPointerGetEvent (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_PTR_EVENT        *Event
  );

VOID
GuiPointerGetPosition (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_PTR_POSITION     *Position
  );

VOID
GuiPointerSetPosition (
  IN OUT GUI_POINTER_CONTEXT     *Context,
  IN     CONST GUI_PTR_POSITION  *Position
  );

VOID
GuiPointerReset (
  IN OUT GUI_POINTER_CONTEXT  *Context
  );

GUI_POINTER_CONTEXT *
GuiPointerConstruct (
  IN UINT32  DefaultX,
  IN UINT32  DefaultY,
  IN UINT32  Width,
  IN UINT32  Height,
  IN UINT8   UiScale
  );

VOID
GuiPointerDestruct (
  IN GUI_POINTER_CONTEXT  *Context
  );

GUI_KEY_CONTEXT *
GuiKeyConstruct (
  IN OC_PICKER_CONTEXT  *PickerContext
  );

VOID
EFIAPI
GuiKeyReset (
  IN OUT GUI_KEY_CONTEXT  *Context
  );

BOOLEAN
GuiKeyGetEvent (
  IN OUT GUI_KEY_CONTEXT  *Context,
  OUT    GUI_KEY_EVENT    *Event
  );

VOID
GuiKeyDestruct (
  IN GUI_KEY_CONTEXT  *Context
  );

#endif
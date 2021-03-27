/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OPEN_CANOPY_H
#define OPEN_CANOPY_H

#include <Library/OcBootManagementLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextIn.h>

typedef struct GUI_OBJ_             GUI_OBJ;
typedef struct GUI_DRAWING_CONTEXT_ GUI_DRAWING_CONTEXT;

struct _BOOT_PICKER_GUI_CONTEXT;
typedef struct _BOOT_PICKER_GUI_CONTEXT BOOT_PICKER_GUI_CONTEXT;

enum {
  GuiPointerPrimaryDown,
  GuiPointerPrimaryUp,
  GuiPointerPrimaryDoubleClick
};

typedef union {
  struct {
    UINT32 X;
    UINT32 Y;
  }      Pos;
  UINT64 Uint64;
} GUI_PTR_POSITION;


typedef struct {
  UINT8            Type;
  GUI_PTR_POSITION Pos;
} GUI_PTR_EVENT;

typedef OC_PICKER_KEY_INFO GUI_KEY_EVENT;

struct GUI_KEY_CONTEXT_ {
  OC_PICKER_CONTEXT *Context;
  OC_PICKER_KEY_MAP KeyFilter;
  OC_MODIFIER_MAP   OcModifiers;
};

typedef
VOID
(*GUI_OBJ_DRAW)(
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     UINT32                  OffsetX,
  IN     UINT32                  OffsetY,
  IN     UINT32                  Width,
  IN     UINT32                  Height,
  IN     UINT8                   Opacity
  );

typedef
GUI_OBJ *
(*GUI_OBJ_PTR_EVENT)(
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  );

typedef
VOID
(*GUI_OBJ_KEY_EVENT)(
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  );

typedef
VOID
(*GUI_OBJ_FOCUS)(
  IN     CONST GUI_OBJ        *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     BOOLEAN              Focus
  );

typedef
BOOLEAN
(*GUI_ANIMATE)(
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  );

typedef struct {
  LIST_ENTRY  Link;
  VOID        *Context;
  GUI_ANIMATE Animate;
} GUI_ANIMATION;

typedef struct GUI_OBJ_CHILD_ GUI_OBJ_CHILD;

struct GUI_OBJ_ {
  INT64             OffsetX;
  INT64             OffsetY;
  UINT32            Width;
  UINT32            Height;
  UINT8             Opacity;
  GUI_OBJ_DRAW      Draw;
  GUI_OBJ_KEY_EVENT KeyEvent;
  GUI_OBJ_PTR_EVENT PtrEvent;
  GUI_OBJ_FOCUS     Focus;
  UINT32            NumChildren;
  GUI_OBJ_CHILD     **Children;
};

struct GUI_OBJ_CHILD_ {
  GUI_OBJ    Obj;
  GUI_OBJ    *Parent;
};

typedef struct {
  UINT32                        Width;
  UINT32                        Height;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
} GUI_IMAGE;

typedef struct GUI_SCREEN_CURSOR_ GUI_SCREEN_CURSOR;

typedef
CONST GUI_IMAGE *
(*GUI_CURSOR_GET_IMAGE)(
  IN BOOT_PICKER_GUI_CONTEXT *Context
  );

typedef
BOOLEAN
(*GUI_EXIT_LOOP)(
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context
  );

struct GUI_SCREEN_CURSOR_ {
  UINT32 X;
  UINT32 Y;
};

typedef struct {
  GUI_OBJ_DRAW         Draw;
  GUI_OBJ_PTR_EVENT    PtrEvent;
  UINT32               NumChildren;
  GUI_OBJ_CHILD        **Children;
  GUI_OBJ_KEY_EVENT    KeyEvent;
  GUI_CURSOR_GET_IMAGE GetCursorImage;
  GUI_EXIT_LOOP        ExitLoop;
  GUI_OBJ              **FocusList;
  UINT8                NumFocusList;
} GUI_VIEW_CONTEXT;

struct GUI_DRAWING_CONTEXT_ {
  //
  // Scene objects
  //
  GUI_OBJ                  Screen;
  GUI_CURSOR_GET_IMAGE     GetCursorImage;
  GUI_EXIT_LOOP            ExitLoop;
  LIST_ENTRY               Animations;
  BOOT_PICKER_GUI_CONTEXT  *GuiContext;
  UINT8                    Scale;
  UINT8                    CursorOpacity;
  UINT32                   TimeOutSeconds;
  UINT64                   FrameTime;
};

EFI_STATUS
GuiPngToImage (
  OUT GUI_IMAGE  *Image,
  IN  VOID       *ImageData,
  IN  UINTN      ImageDataSize,
  IN  BOOLEAN    PremultiplyAlpha
  );
  
EFI_STATUS
GuiIcnsToImageIcon (
  OUT GUI_IMAGE  *Image,
  IN  VOID       *IcnsImage,
  IN  UINT32     IcnsImageSize,
  IN  UINT8      Scale,
  IN  UINT32     MatchWidth,
  IN  UINT32     MatchHeight,
  IN  BOOLEAN    AllowLess
  );

EFI_STATUS
GuiLabelToImage (
  OUT GUI_IMAGE *Image,
  IN  VOID      *RawData,
  IN  UINT32    DataLength,
  IN  UINT8     Scale,
  IN  BOOLEAN   Inverted
  );

VOID
GuiObjDrawDelegate (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     UINT32                  OffsetX,
  IN     UINT32                  OffsetY,
  IN     UINT32                  Width,
  IN     UINT32                  Height,
  IN     UINT8                   Opacity
  );

GUI_OBJ *
GuiObjDelegatePtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  );

BOOLEAN
GuiClipChildBounds (
  IN     INT64   ChildOffset,
  IN     UINT32  ChildLength,
  IN OUT UINT32  *ReqOffset,
  IN OUT UINT32  *ReqLength
  );

VOID
GuiDrawToBuffer (
  IN     CONST GUI_IMAGE      *Image,
  IN     UINT8                Opacity,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height
  );

VOID
GuiDrawToBufferFill (
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Colour,
  IN OUT GUI_DRAWING_CONTEXT                  *DrawContext,
  IN     UINT32                               PosX,
  IN     UINT32                               PosY,
  IN     UINT32                               Width,
  IN     UINT32                               Height
  );

VOID
GuiRequestDraw (
  IN UINT32  PosX,
  IN UINT32  PosY,
  IN UINT32  Width,
  IN UINT32  Height
  );

VOID
GuiRequestDrawCrop (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                X,
  IN     INT64                Y,
  IN     UINT32               Width,
  IN     UINT32               Height
  );

VOID
GuiViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  CONST GUI_VIEW_CONTEXT   *ViewContext
  );

VOID
GuiViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT   *DrawContext,
  OUT    BOOT_PICKER_GUI_CONTEXT *GuiContext
  );

VOID
GuiRedrawAndFlushScreen (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  );

VOID
GuiDrawLoop (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  );

VOID
GuiClearScreen (
  IN OUT GUI_DRAWING_CONTEXT           *DrawContext,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Pixel
  );

EFI_STATUS
GuiLibConstruct (
  IN BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN INT32                    CursorOffsetX,
  IN INT32                    CursorOffsetY
  );

VOID
GuiLibDestruct (
  VOID
  );

VOID
GuiBlendPixel (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel,
  IN     UINT8                                Opacity
  );

VOID
GuiBlendPixelSolid (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel
  );

VOID
GuiBlendPixelOpaque (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel,
  IN     UINT8                                Opacity
  );

EFI_STATUS
GuiCreateHighlightedImage (
  OUT GUI_IMAGE                            *SelectedImage,
  IN  CONST GUI_IMAGE                      *SourceImage,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel
  );

typedef enum {
  GuiInterpolTypeLinear,
  GuiInterpolTypeSmooth
} GUI_INTERPOL_TYPE;

typedef struct {
  GUI_INTERPOL_TYPE Type;
  UINT64            StartTime;
  UINT64            Duration;
  UINT32            StartValue;
  UINT32            EndValue;
  // FIXME: This probably belongs into some animation context.
  UINT32            HoldTime;
} GUI_INTERPOLATION;

UINT32
GuiGetInterpolatedValue (
  IN CONST GUI_INTERPOLATION  *Interpol,
  IN       UINT64             CurrentTime
  );

VOID
GuiGetBaseCoords (
  IN  GUI_OBJ              *This,
  IN  GUI_DRAWING_CONTEXT  *DrawContext,
  OUT INT64                *BaseX,
  OUT INT64                *BaseY
  );

#endif // OPEN_CANOPY_H

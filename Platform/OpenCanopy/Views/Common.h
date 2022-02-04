#ifndef VIEWS_COMMON_H_
#define VIEWS_COMMON_H_

#include "../GuiApp.h"

typedef struct {
  GUI_OBJ_CHILD Hdr;
  UINT8         ImageId;
  UINT8         ImageState;
} GUI_OBJ_CLICKABLE;

VOID
GuiDrawChildImage (
  IN     CONST GUI_IMAGE      *Image,
  IN     UINT8                Opacity,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                ParentBaseX,
  IN     INT64                ParentBaseY,
  IN     INT64                ChildBaseX,
  IN     INT64                ChildBaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height
  );

BOOLEAN
GuiClickableIsHit (
  IN CONST GUI_IMAGE  *Image,
  IN INT64            OffsetX,
  IN INT64            OffsetY
  );

GUI_OBJ *
InternalFocusKeyHandler (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context,
  IN     CONST GUI_KEY_EVENT      *KeyEvent
  );

VOID
CommonViewInitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN     CONST GUI_VIEW_CONTEXT   *ViewContext
  );

VOID
InternalCommonViewDraw (
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
InternalCommonViewPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  );

VOID
InternalCommonSimpleButtonDraw (
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

typedef enum {
  CommonPtrNotHit = 0,
  CommonPtrAction = 1,
  CommonPtrHit    = 2
} COMMON_PTR_EVENT_RESULT;

UINT8
InternalCommonSimpleButtonPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  );

extern GUI_OBJ_CHILD     mCommonActionButtonsContainer;
extern GUI_OBJ_CLICKABLE mCommonRestart;
extern GUI_OBJ_CLICKABLE mCommonShutDown;

extern GUI_INTERPOLATION mCommonIntroOpacityInterpol;

#endif // VIEWS_COMMON_H_

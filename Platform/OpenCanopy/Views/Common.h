#ifndef VIEWS_COMMON_H_
#define VIEWS_COMMON_H_

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

VOID
InternalCommonIntroOpacoityInterpolInit (
  VOID
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
  IN     UINT32                  Height
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
  IN     UINT32                  Height
  );

extern GUI_OBJ_CHILD mCommonActionButtonsContainer;

extern UINT8 mCommonActionButtonsOpacity;

extern GUI_INTERPOLATION mCommonIntroOpacityInterpol;

#endif // VIEWS_COMMON_H_

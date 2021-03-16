#include <Base.h>

#include <Library/DebugLib.h>

#include "../OpenCanopy.h"
#include "../GuiApp.h"
#include "BootPicker.h"
#include "ProcessorBind.h"

#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ResetSystemLib.h>

#include "Common.h"

//
// FIXME: Create BootPickerView struct with background colour/image info.
//
GLOBAL_REMOVE_IF_UNREFERENCED INT64 mBackgroundImageOffsetX;
GLOBAL_REMOVE_IF_UNREFERENCED INT64 mBackgroundImageOffsetY;

GLOBAL_REMOVE_IF_UNREFERENCED GUI_INTERPOLATION mCommonIntroOpacityInterpol;

GLOBAL_REMOVE_IF_UNREFERENCED UINT8 mCommonActionButtonsOpacity = 0;

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
  )
{
  BOOLEAN Result;

  ASSERT (Image != NULL);
  ASSERT (DrawContext != NULL);

  Result = GuiClipChildBounds (
             ChildBaseX,
             Image->Width,
             &OffsetX,
             &Width
             );
  if (Result) {
    Result = GuiClipChildBounds (
               ChildBaseY,
               Image->Height,
               &OffsetY,
               &Height
               );
    if (Result) {
      ASSERT (Image->Width  > OffsetX);
      ASSERT (Image->Height > OffsetY);
      ASSERT (Image->Buffer != NULL);

      GuiDrawToBuffer (
        Image,
        Opacity,
        DrawContext,
        ParentBaseX + ChildBaseX,
        ParentBaseY + ChildBaseY,
        OffsetX,
        OffsetY,
        Width,
        Height
        );
    }
  }
}

BOOLEAN
GuiClickableIsHit (
  IN CONST GUI_IMAGE  *Image,
  IN INT64            OffsetX,
  IN INT64            OffsetY
  )
{
  ASSERT (Image != NULL);
  ASSERT (Image->Buffer != NULL);

  if (OffsetX < 0 || OffsetX >= Image->Width
   || OffsetY < 0 || OffsetY >= Image->Height) {
    return FALSE;
  }

  return Image->Buffer[(UINT32) OffsetY * Image->Width + (UINT32) OffsetX].Reserved > 0;
}

VOID
InternalCommonIntroOpacoityInterpolInit (
  VOID
  )
{
  mCommonIntroOpacityInterpol.Type       = GuiInterpolTypeSmooth;
  mCommonIntroOpacityInterpol.StartTime  = 0;
  mCommonIntroOpacityInterpol.Duration   = 25;
  mCommonIntroOpacityInterpol.StartValue = 0;
  mCommonIntroOpacityInterpol.EndValue   = 0xFF;
}

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
  )
{
  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  ASSERT (BaseX == 0);
  ASSERT (BaseY == 0);

  GuiDrawToBufferFill (
    &Context->BackgroundColor.Pixel,
    DrawContext,
    OffsetX,
    OffsetY,
    Width,
    Height
    );

  if (DrawContext->GuiContext->Background.Buffer != NULL) {
    GuiDrawChildImage (
      &DrawContext->GuiContext->Background,
      0xFF,
      DrawContext,
      0,
      0,
      mBackgroundImageOffsetX,
      mBackgroundImageOffsetY,
      OffsetX,
      OffsetY,
      Width,
      Height
      );
  }

  GuiObjDrawDelegate (
    This,
    DrawContext,
    Context,
    0,
    0,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
}

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
  )
{
  CONST GUI_OBJ_CLICKABLE       *Clickable;
  CONST GUI_IMAGE               *ButtonImage;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  Clickable   = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage = Clickable->CurrentImage;
  ASSERT (ButtonImage != NULL);

  ASSERT (ButtonImage->Width == This->Width);
  ASSERT (ButtonImage->Height == This->Height);
  ASSERT (ButtonImage->Buffer != NULL);

  GuiDrawToBuffer (
    ButtonImage,
    mCommonActionButtonsOpacity,
    DrawContext,
    BaseX,
    BaseY,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);
}

GUI_OBJ *
InternalBootPickerShutDownPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  GUI_OBJ_CLICKABLE *Clickable;
  CONST GUI_IMAGE   *ButtonImage;
  BOOLEAN           IsHit;

  Clickable   = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage = &Context->Icons[ICON_SHUT_DOWN][ICON_TYPE_BASE];

  ASSERT (Event->Type == GuiPointerPrimaryDown
       || Event->Type == GuiPointerPrimaryUp
       || Event->Type == GuiPointerPrimaryDoubleClick);

  IsHit = GuiClickableIsHit (
    ButtonImage,
    Event->Pos.Pos.X - BaseX,
    Event->Pos.Pos.Y - BaseY
    );
  if (IsHit) {
    if (Event->Type == GuiPointerPrimaryDown) {
      ButtonImage = &Context->Icons[ICON_SHUT_DOWN][ICON_TYPE_HELD];
    } else {
      ResetShutdown ();
    }
  }

  if (Clickable->CurrentImage != ButtonImage) {
    Clickable->CurrentImage = ButtonImage;
    //
    // The view is constructed such that the action buttons are always fully
    // visible.
    //
    ASSERT (BaseX >= 0);
    ASSERT (BaseY >= 0);
    ASSERT (BaseX + This->Width <= DrawContext->Screen.Width);
    ASSERT (BaseY + This->Height <= DrawContext->Screen.Height);
    GuiRequestDraw ((UINT32) BaseX, (UINT32) BaseY, This->Width, This->Height);
  }

  return This;
}

GUI_OBJ *
InternalBootPickerRestartPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  GUI_OBJ_CLICKABLE *Clickable;
  CONST GUI_IMAGE   *ButtonImage;
  BOOLEAN           IsHit;

  Clickable   = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage = &Context->Icons[ICON_RESTART][ICON_TYPE_BASE];

  ASSERT (Event->Type == GuiPointerPrimaryDown
       || Event->Type == GuiPointerPrimaryUp
       || Event->Type == GuiPointerPrimaryDoubleClick);

  IsHit = GuiClickableIsHit (
    ButtonImage,
    Event->Pos.Pos.X - BaseX,
    Event->Pos.Pos.Y - BaseY
    );
  if (IsHit) {
    if (Event->Type == GuiPointerPrimaryDown) {
      ButtonImage = &Context->Icons[ICON_RESTART][ICON_TYPE_HELD];
    } else {
      ResetWarm ();
    }
  }

  if (Clickable->CurrentImage != ButtonImage) {
    Clickable->CurrentImage = ButtonImage;
    //
    // The view is constructed such that the action buttons are always fully
    // visible.
    //
    ASSERT (BaseX >= 0);
    ASSERT (BaseY >= 0);
    ASSERT (BaseX + This->Width <= DrawContext->Screen.Width);
    ASSERT (BaseY + This->Height <= DrawContext->Screen.Height);
    GuiRequestDraw ((UINT32) BaseX, (UINT32) BaseY, This->Width, This->Height);
  }

  return This;
}

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mCommonRestart = {
  {
    {
      0, 0, 0, 0,
      InternalCommonSimpleButtonDraw,
      InternalBootPickerRestartPtrEvent,
      0,
      NULL
    },
    &mCommonActionButtonsContainer.Obj
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mCommonShutDown = {
  {
    {
      0, 0, 0, 0,
      InternalCommonSimpleButtonDraw,
      InternalBootPickerShutDownPtrEvent,
      0,
      NULL
    },
    &mCommonActionButtonsContainer.Obj
  },
  NULL
};

STATIC GUI_OBJ_CHILD *mCommonActionButtonsContainerChilds[] = {
  &mCommonRestart.Hdr,
  &mCommonShutDown.Hdr
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mCommonActionButtonsContainer = {
  {
    0, 0, 0, 0,
    GuiObjDrawDelegate,
    GuiObjDelegatePtrEvent,
    ARRAY_SIZE (mCommonActionButtonsContainerChilds),
    mCommonActionButtonsContainerChilds
  },
  NULL
};

VOID
CommonViewInitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN     CONST GUI_VIEW_CONTEXT   *ViewContext
  )
{
  ASSERT (GuiContext != NULL);

  GuiViewInitialize (DrawContext, GuiContext, ViewContext);

  mBackgroundImageOffsetX = DivS64x64Remainder (
    (INT64) DrawContext->Screen.Width - GuiContext->Background.Width,
    2,
    NULL
    );
  mBackgroundImageOffsetY = DivS64x64Remainder (
    (INT64) DrawContext->Screen.Height - GuiContext->Background.Height,
    2,
    NULL
    );

  mCommonRestart.CurrentImage    = &GuiContext->Icons[ICON_RESTART][ICON_TYPE_BASE];
  mCommonRestart.Hdr.Obj.Width   = mCommonRestart.CurrentImage->Width;
  mCommonRestart.Hdr.Obj.Height  = mCommonRestart.CurrentImage->Height;
  mCommonRestart.Hdr.Obj.OffsetX = 0;
  mCommonRestart.Hdr.Obj.OffsetY = 0;

  mCommonShutDown.CurrentImage    = &GuiContext->Icons[ICON_SHUT_DOWN][ICON_TYPE_BASE];
  mCommonShutDown.Hdr.Obj.Width   = mCommonShutDown.CurrentImage->Width;
  mCommonShutDown.Hdr.Obj.Height  = mCommonShutDown.CurrentImage->Height;
  mCommonShutDown.Hdr.Obj.OffsetX = mCommonRestart.Hdr.Obj.Width + BOOT_ACTION_BUTTON_SPACE * GuiContext->Scale;
  mCommonShutDown.Hdr.Obj.OffsetY = 0;

  mCommonActionButtonsContainer.Obj.Width   = mCommonRestart.Hdr.Obj.Width + mCommonShutDown.Hdr.Obj.Width + BOOT_ACTION_BUTTON_SPACE * GuiContext->Scale;
  mCommonActionButtonsContainer.Obj.Height  = MAX (mCommonRestart.CurrentImage->Height, mCommonShutDown.CurrentImage->Height);
  mCommonActionButtonsContainer.Obj.OffsetX = (DrawContext->Screen.Width - mCommonActionButtonsContainer.Obj.Width) / 2;
  mCommonActionButtonsContainer.Obj.OffsetY = DrawContext->Screen.Height - mCommonActionButtonsContainer.Obj.Height - BOOT_ACTION_BUTTON_SPACE * GuiContext->Scale;
}

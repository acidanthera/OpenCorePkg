#include <Base.h>

#include <Protocol/OcAudio.h>

#include <Library/DebugLib.h>

#include "../OpenCanopy.h"
#include "../GuiApp.h"

#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ResetSystemLib.h>

#include "Common.h"

//
// FIXME: Create BootPickerView struct with background colour/image info.
//
GLOBAL_REMOVE_IF_UNREFERENCED INT64 mBackgroundImageOffsetX;
GLOBAL_REMOVE_IF_UNREFERENCED INT64 mBackgroundImageOffsetY;

GLOBAL_REMOVE_IF_UNREFERENCED GUI_INTERPOLATION mCommonIntroOpacityInterpol = {
  GuiInterpolTypeSmooth,
  0,
  25,
  0,
  0xFF,
  0
};

STATIC UINT8 mCommonFocusState = 0;

STATIC GUI_OBJ **mCommonFocusList  = NULL;
STATIC UINT8   mNumCommonFocusList = 0;

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

GUI_OBJ *
InternalFocusKeyHandler (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context,
  IN     CONST GUI_KEY_EVENT      *KeyEvent
  )
{
  UINT8   CommonFocusState;
  GUI_OBJ *FocusChangedObj;

  if (KeyEvent->OcKeyCode == OC_INPUT_SWITCH_FOCUS) {
    if (mNumCommonFocusList > 1) {
      mCommonFocusList[mCommonFocusState]->Focus (
        mCommonFocusList[mCommonFocusState],
        DrawContext,
        FALSE
        );

      if ((KeyEvent->OcModifiers & OC_MODIFIERS_REVERSE_SWITCH_FOCUS) == 0) {
        CommonFocusState = mCommonFocusState + 1;
        if (CommonFocusState == mNumCommonFocusList) {
          CommonFocusState = 0;
        }
      } else {
        CommonFocusState = mCommonFocusState - 1;
        if (CommonFocusState == MAX_UINT8) {
          CommonFocusState = mNumCommonFocusList - 1;
        }
      }

      mCommonFocusState = CommonFocusState;

      mCommonFocusList[CommonFocusState]->Focus (
        mCommonFocusList[CommonFocusState],
        DrawContext,
        TRUE
        );

      FocusChangedObj = mCommonFocusList[CommonFocusState];
    } else {
      FocusChangedObj = NULL;
    }
  } else {
    mCommonFocusList[mCommonFocusState]->KeyEvent (
      mCommonFocusList[mCommonFocusState],
      DrawContext,
      Context,
      KeyEvent
      );
    
    FocusChangedObj = NULL;
  }

  return FocusChangedObj;
}

VOID
InternalResetFocus (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  if (mCommonFocusState == 0) {
    return;
  }

  mCommonFocusList[mCommonFocusState]->Focus (
    mCommonFocusList[mCommonFocusState],
    DrawContext,
    FALSE
    );

  mCommonFocusState = 0;

  mCommonFocusList[mCommonFocusState]->Focus (
    mCommonFocusList[mCommonFocusState],
    DrawContext,
    TRUE
    );
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
  IN     UINT32                  Height,
  IN     UINT8                   Opacity
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
    Height,
    Opacity
    );
}

GUI_OBJ *
InternalCommonViewPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  InternalResetFocus (DrawContext);
  return GuiObjDelegatePtrEvent (This, DrawContext, Context, BaseX, BaseY, Event);
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
  IN     UINT32                  Height,
  IN     UINT8                   Opacity
  )
{
  CONST GUI_OBJ_CLICKABLE       *Clickable;
  CONST GUI_IMAGE               *ButtonImage;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);

  Clickable   = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage = &Context->Icons[Clickable->ImageId][Clickable->ImageState];
  ASSERT (ButtonImage != NULL);

  ASSERT (ButtonImage->Width == This->Width);
  ASSERT (ButtonImage->Height == This->Height);
  ASSERT (ButtonImage->Buffer != NULL);

  GuiDrawToBuffer (
    ButtonImage,
    Opacity,
    DrawContext,
    BaseX,
    BaseY,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
}

UINT8
InternalCommonSimpleButtonPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  UINT8             Return;

  GUI_OBJ_CLICKABLE *Clickable;
  CONST GUI_IMAGE   *ButtonImage;
  UINT8             ImageState;

  BOOLEAN           IsHit;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);
  //
  // Ignore double-clicks.
  //
  if (Event->Type == GuiPointerPrimaryDoubleClick) {
    return 1;
  }

  Clickable   = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ImageState  = ICON_TYPE_BASE;
  ButtonImage = &Context->Icons[Clickable->ImageId][ImageState];

  Return = CommonPtrNotHit;

  IsHit = GuiClickableIsHit (
    ButtonImage,
    Event->Pos.Pos.X - BaseX,
    Event->Pos.Pos.Y - BaseY
    );
  if (IsHit) {
    if (Event->Type == GuiPointerPrimaryUp) {
      Return = CommonPtrAction;
    } else {
      ImageState = ICON_TYPE_HELD;
      Return = CommonPtrHit;
    }
  }

  if (Clickable->ImageState != ImageState) {
    Clickable->ImageState = ImageState;
    //
    // The view is constructed such that the selector is always fully visible.
    //
    ASSERT (BaseX >= 0);
    ASSERT (BaseY >= 0);
    ASSERT (BaseX + This->Width <= DrawContext->Screen.Width);
    ASSERT (BaseY + This->Height <= DrawContext->Screen.Height);
    GuiRequestDraw ((UINT32) BaseX, (UINT32) BaseY, This->Width, This->Height);
  }

  return Return;
}

VOID
InternalCommonSimpleButtonFocusDraw (
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
  )
{
  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  GuiDrawChildImage (
    &Context->Icons[ICON_BUTTON_FOCUS][ICON_TYPE_BASE],
    Opacity,
    DrawContext,
    BaseX,
    BaseY,
    ((INT32) This->Width - (INT32) Context->Icons[ICON_BUTTON_FOCUS][ICON_TYPE_BASE].Width) / 2,
    ((INT32) This->Height - (INT32) Context->Icons[ICON_BUTTON_FOCUS][ICON_TYPE_BASE].Height) / 2,
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

VOID
InternalCommonShutDownKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  if (KeyEvent->OcKeyCode == OC_INPUT_CONTINUE
   || KeyEvent->OcKeyCode == OC_INPUT_TYPING_CONFIRM) {
    if (Context->PickerContext->PickerAudioAssist) {
      Context->PickerContext->PlayAudioFile (
        Context->PickerContext,
        AppleVoiceOverAudioFileBeep,
        TRUE
        );
    }

    ResetShutdown ();
  }
}

GUI_OBJ *
InternalCommonShutDownPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  UINT8 Result;

  Result = InternalCommonSimpleButtonPtrEvent (
    This,
    DrawContext,
    Context,
    BaseX,
    BaseY,
    Event
    );
  switch (Result) {
    case CommonPtrNotHit:
    {
      return NULL;
    }
    
    case CommonPtrAction:
    {
      ResetShutdown ();
      //
      // Falthrough to 'hit' case.
      //
    }

    case CommonPtrHit:
    {
      break;
    }
  }

  return This;
}

VOID
InternalCommonRestartKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  if (KeyEvent->OcKeyCode == OC_INPUT_CONTINUE
   || KeyEvent->OcKeyCode == OC_INPUT_TYPING_CONFIRM) {
    if (Context->PickerContext->PickerAudioAssist) {
      Context->PickerContext->PlayAudioFile (
        Context->PickerContext,
        AppleVoiceOverAudioFileBeep,
        TRUE
        );
    }
    
    ResetWarm ();
  }
}

GUI_OBJ *
InternalCommonRestartPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  UINT8 Result;

  Result = InternalCommonSimpleButtonPtrEvent (
    This,
    DrawContext,
    Context,
    BaseX,
    BaseY,
    Event
    );
  switch (Result) {
    case CommonPtrNotHit:
    {
      return NULL;
    }
    
    case CommonPtrAction:
    {
      ResetWarm ();
      //
      // Falthrough to 'hit' case.
      //
    }

    case CommonPtrHit:
    {
      break;
    }
  }

  return This;
}

extern GUI_OBJ_CHILD mCommonFocus;

STATIC GUI_OBJ_CHILD *mCommonActionButtonsContainerChilds[] = {
  &mCommonRestart.Hdr,
  &mCommonShutDown.Hdr,
  &mCommonFocus
};

VOID
InternalCommonActionButtonFocus (
  IN     CONST GUI_OBJ        *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     BOOLEAN              Focus
  )
{
  if (!Focus) {
    mCommonActionButtonsContainer.Obj.NumChildren = ARRAY_SIZE (mCommonActionButtonsContainerChilds) - 1;
  } else {
    mCommonActionButtonsContainer.Obj.NumChildren = ARRAY_SIZE (mCommonActionButtonsContainerChilds);

    GuiRequestDrawCrop (
      DrawContext,
      mCommonActionButtonsContainer.Obj.OffsetX + mCommonFocus.Obj.OffsetX,
      mCommonActionButtonsContainer.Obj.OffsetY + mCommonFocus.Obj.OffsetY,
      mCommonFocus.Obj.Width,
      mCommonFocus.Obj.Height
      );

    mCommonFocus.Obj.OffsetX = This->OffsetX + ((INT32) This->Width - (INT32) mCommonFocus.Obj.Width) / 2;
    mCommonFocus.Obj.OffsetY = This->OffsetY + ((INT32) This->Height - (INT32) mCommonFocus.Obj.Height) / 2;

    DrawContext->GuiContext->AudioPlaybackTimeout = 0;

    if (This == &mCommonShutDown.Hdr.Obj) {
      DrawContext->GuiContext->VoAction = CanopyVoFocusShutDown;
    } else {
      ASSERT (This == &mCommonRestart.Hdr.Obj);
      DrawContext->GuiContext->VoAction = CanopyVoFocusRestart;
    }
  }

  GuiRequestDrawCrop (
    DrawContext,
    mCommonActionButtonsContainer.Obj.OffsetX + mCommonFocus.Obj.OffsetX,
    mCommonActionButtonsContainer.Obj.OffsetY + mCommonFocus.Obj.OffsetY,
    mCommonFocus.Obj.Width,
    mCommonFocus.Obj.Height
    );
}

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mCommonFocus = {
  {
    0, 0, 0, 0, 0xFF,
    InternalCommonSimpleButtonFocusDraw,
    NULL,
    GuiObjDelegatePtrEvent,
    NULL,
    0,
    NULL
  },
  &mCommonActionButtonsContainer.Obj
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mCommonRestart = {
  {
    {
      0, 0, 0, 0, 0xFF,
      InternalCommonSimpleButtonDraw,
      InternalCommonRestartKeyEvent,
      InternalCommonRestartPtrEvent,
      InternalCommonActionButtonFocus,
      0,
      NULL
    },
    &mCommonActionButtonsContainer.Obj
  },
  0,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mCommonShutDown = {
  {
    {
      0, 0, 0, 0, 0xFF,
      InternalCommonSimpleButtonDraw,
      InternalCommonShutDownKeyEvent,
      InternalCommonShutDownPtrEvent,
      InternalCommonActionButtonFocus,
      0,
      NULL
    },
    &mCommonActionButtonsContainer.Obj
  },
  0,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mCommonActionButtonsContainer = {
  {
    0, 0, 0, 0, 0,
    GuiObjDrawDelegate,
    NULL,
    GuiObjDelegatePtrEvent,
    NULL,
    ARRAY_SIZE (mCommonActionButtonsContainerChilds) - 1,
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
  CONST GUI_IMAGE *FocusImage;
  CONST GUI_IMAGE *RestartImage;
  CONST GUI_IMAGE *ShutDownImage;
  UINT32          RestartWidth;
  UINT32          RestartHeight;
  UINT32          ShutDownWidth;
  UINT32          ShutDownHeight;

  ASSERT (GuiContext != NULL);

  mCommonFocusState   = 0;
  mCommonFocusList    = ViewContext->FocusList;
  mNumCommonFocusList = ViewContext->NumFocusList;

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
  
  FocusImage    = &GuiContext->Icons[ICON_BUTTON_FOCUS][ICON_TYPE_BASE];
  RestartImage  = &GuiContext->Icons[ICON_RESTART][ICON_TYPE_BASE];
  ShutDownImage = &GuiContext->Icons[ICON_SHUT_DOWN][ICON_TYPE_BASE];
  
  mCommonFocus.Obj.OffsetX = 0;
  mCommonFocus.Obj.OffsetY = 0;
  mCommonFocus.Obj.Width   = FocusImage->Width;
  mCommonFocus.Obj.Height  = FocusImage->Height;

  RestartWidth   = MAX (RestartImage->Width, FocusImage->Width);
  RestartHeight  = MAX (RestartImage->Height, FocusImage->Height);
  ShutDownWidth  = MAX (ShutDownImage->Width, FocusImage->Width);
  ShutDownHeight = MAX (ShutDownImage->Height, FocusImage->Height);

  mCommonRestart.Hdr.Obj.OffsetX = (RestartWidth - RestartImage->Width) / 2;
  mCommonRestart.Hdr.Obj.OffsetY = (RestartHeight - RestartImage->Height) / 2;
  mCommonRestart.Hdr.Obj.Width   = RestartImage->Width;
  mCommonRestart.Hdr.Obj.Height  = RestartImage->Height;
  mCommonRestart.ImageId         = ICON_RESTART;
  mCommonRestart.ImageState      = ICON_TYPE_BASE;

  mCommonShutDown.Hdr.Obj.OffsetX = RestartWidth + BOOT_ACTION_BUTTON_SPACE * GuiContext->Scale + (ShutDownWidth - ShutDownImage->Width) / 2;
  mCommonShutDown.Hdr.Obj.OffsetY = (ShutDownHeight - ShutDownImage->Height) / 2;
  mCommonShutDown.Hdr.Obj.Width   = ShutDownImage->Width;
  mCommonShutDown.Hdr.Obj.Height  = ShutDownImage->Height;
  mCommonShutDown.ImageId         = ICON_SHUT_DOWN;
  mCommonShutDown.ImageState      = ICON_TYPE_BASE;

  mCommonActionButtonsContainer.Obj.Width   = RestartWidth + ShutDownWidth + BOOT_ACTION_BUTTON_SPACE * GuiContext->Scale;
  mCommonActionButtonsContainer.Obj.Height  = MAX (RestartHeight, ShutDownHeight);
  mCommonActionButtonsContainer.Obj.OffsetX = (DrawContext->Screen.Width - mCommonActionButtonsContainer.Obj.Width) / 2;
  mCommonActionButtonsContainer.Obj.OffsetY = DrawContext->Screen.Height - mCommonActionButtonsContainer.Obj.Height - BOOT_ACTION_BUTTON_SPACE * GuiContext->Scale;
}

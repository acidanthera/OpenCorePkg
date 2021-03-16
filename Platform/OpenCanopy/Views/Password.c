#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/SerialPortLib.h>

#include "../OpenCanopy.h"
#include "../GuiApp.h"
#include "BootPicker.h"

#include "Common.h"

#include "../GuiIo.h"

typedef struct {
  CHAR8 Password[OC_PASSWORD_MAX_LEN];
  UINT8 PasswordLen;
} GUI_PASSWORD_INFO;

typedef struct {
  GUI_OBJ_CHILD     Hdr;
  GUI_PASSWORD_INFO PasswordInfo;
  BOOLEAN           RequestConfirm;
  UINT8             MaxPasswordLen;
} GUI_PASSWORD_BOX;

extern GUI_PASSWORD_BOX  mPasswordBox;
extern GUI_OBJ_CLICKABLE mPasswordEnter;
extern GUI_OBJ_CHILD     mPasswordLock;

// TODO: FIX
extern GUI_POINTER_CONTEXT *mPointerContext;

STATIC UINT8 mPasswordBoxOpacity = 0;

STATIC GUI_INTERPOLATION mPasswordIncorrectInterpol;

STATIC
VOID
InternalInitPasswordIncorrectInterpol (
  IN UINT64  StartTime,
  IN UINT8   Duration,
  IN UINT32  Delta
  )
{
  mPasswordIncorrectInterpol.Type       = GuiInterpolTypeSmooth;
  mPasswordIncorrectInterpol.StartTime  = StartTime;
  mPasswordIncorrectInterpol.Duration   = Duration;
  mPasswordIncorrectInterpol.StartValue = 0;
  mPasswordIncorrectInterpol.EndValue   = Delta;
}

BOOLEAN
InternalPasswordAnimateIncorrect (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  STATIC UINT32  LastOff = 0;
  STATIC BOOLEAN Left = FALSE;
  STATIC UINT8   Iteration = 0;

  BOOLEAN Return;
  UINT32  DeltaOrig;
  UINT32  DeltaAdd;

  
  DeltaOrig = GuiGetInterpolatedValue (
    &mPasswordIncorrectInterpol,
    CurrentTime
    );
  DeltaAdd = DeltaOrig - LastOff;
  LastOff  = DeltaOrig;

  if (Left) {
    mPasswordBox.Hdr.Obj.OffsetX   -= DeltaAdd;
    mPasswordEnter.Hdr.Obj.OffsetX -= DeltaAdd;
    GuiRequestDraw (
      (UINT32) mPasswordBox.Hdr.Obj.OffsetX,
      (UINT32) mPasswordBox.Hdr.Obj.OffsetY,
      mPasswordBox.Hdr.Obj.Width + DeltaAdd,
      mPasswordBox.Hdr.Obj.Height
      );
    GuiRequestDraw (
      (UINT32) mPasswordEnter.Hdr.Obj.OffsetX,
      (UINT32) mPasswordEnter.Hdr.Obj.OffsetY,
      mPasswordEnter.Hdr.Obj.Width + DeltaAdd,
      mPasswordEnter.Hdr.Obj.Height
      );
  } else {
    GuiRequestDraw (
      (UINT32) mPasswordBox.Hdr.Obj.OffsetX,
      (UINT32) mPasswordBox.Hdr.Obj.OffsetY,
      mPasswordBox.Hdr.Obj.Width + DeltaAdd,
      mPasswordBox.Hdr.Obj.Height
      );
    GuiRequestDraw (
      (UINT32) mPasswordEnter.Hdr.Obj.OffsetX,
      (UINT32) mPasswordEnter.Hdr.Obj.OffsetY,
      mPasswordEnter.Hdr.Obj.Width + DeltaAdd,
      mPasswordEnter.Hdr.Obj.Height
      );
    mPasswordBox.Hdr.Obj.OffsetX   += DeltaAdd;
    mPasswordEnter.Hdr.Obj.OffsetX += DeltaAdd;
  }

  Return = FALSE;

  if (CurrentTime - mPasswordIncorrectInterpol.StartTime >= mPasswordIncorrectInterpol.Duration) {
    switch (Iteration) {
      case 0:
      case 1:
        InternalInitPasswordIncorrectInterpol (CurrentTime + 1, 6, 40);
        break;

      case 2:
        InternalInitPasswordIncorrectInterpol (CurrentTime + 1, 3, 20);
        break;

      case 3:
        Iteration = 0xFF;
        Return = TRUE;
        mPasswordBoxOpacity = 0xFF;
        GuiRequestDraw (
          (UINT32) mPasswordBox.Hdr.Obj.OffsetX,
          (UINT32) mPasswordBox.Hdr.Obj.OffsetY,
          mPasswordBox.Hdr.Obj.Width + DeltaAdd,
          mPasswordBox.Hdr.Obj.Height
          );
        GuiRequestDraw (
          (UINT32) mPasswordEnter.Hdr.Obj.OffsetX,
          (UINT32) mPasswordEnter.Hdr.Obj.OffsetY,
          mPasswordEnter.Hdr.Obj.Width + DeltaAdd,
          mPasswordEnter.Hdr.Obj.Height
          );
        break;
    }

    ++Iteration;
    Left = !Left;
    LastOff = 0;
  }
  
  return Return;
}

STATIC
VOID
InternalQueueIncorrectPassword (
  OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  STATIC GUI_ANIMATION PwAnim = {
    INITIALIZE_LIST_HEAD_VARIABLE (PwAnim.Link),
    NULL,
    InternalPasswordAnimateIncorrect
  };
  if (!IsNodeInList (&PwAnim.Link, &DrawContext->Animations)) {
    InternalInitPasswordIncorrectInterpol (DrawContext->FrameTime, 3, 20);
    InsertHeadList (&DrawContext->Animations, &PwAnim.Link);
  }
}

BOOLEAN
InternalConfirmPassword (
  IN OUT GUI_DRAWING_CONTEXT            *DrawContext,
  IN     CONST BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  BOOLEAN              Result;
  OC_PRIVILEGE_CONTEXT *Privilege;
  GUI_PTR_POSITION     PtrPosition;

  GuiPointerGetPosition (mPointerContext, &PtrPosition);

  Privilege = Context->PickerContext->PrivilegeContext;

  Result = OcVerifyPasswordSha512 (
    (CONST UINT8 *) mPasswordBox.PasswordInfo.Password,
    mPasswordBox.PasswordInfo.PasswordLen,
    Privilege->Salt,
    Privilege->SaltSize,
    Privilege->Hash
    );
  
  GuiPointerReset (mPointerContext);
  GuiPointerSetPosition (mPointerContext, &PtrPosition);
  DrawContext->CursorOpacity = 0xFF;

  SecureZeroMem (mPasswordBox.PasswordInfo.Password, mPasswordBox.PasswordInfo.PasswordLen);

  if (Result) {
    Privilege->CurrentLevel = OcPrivilegeAuthorized;
    if (Context->PickerContext->PickerAudioAssist
     && Context->AudioPlaybackTimeout >= 0) {
      Context->PickerContext->PlayAudioFile (
        Context->PickerContext,
        OcVoiceOverAudioFilePasswordAccepted,
        TRUE
        );
    }

    return TRUE;
  }
  
  if (Context->PickerContext->PickerAudioAssist
   && Context->AudioPlaybackTimeout >= 0) {
    Context->PickerContext->PlayAudioFile (
      Context->PickerContext,
      OcVoiceOverAudioFilePasswordIncorrect,
      TRUE
      );
  }

  return FALSE;
}

STATIC
VOID
InternalRedrawPaswordBox (
  VOID
  )
{
  GuiRequestDraw (
    (UINT32) mPasswordBox.Hdr.Obj.OffsetX,
    (UINT32) mPasswordBox.Hdr.Obj.OffsetY,
    mPasswordBox.Hdr.Obj.Width,
    mPasswordBox.Hdr.Obj.Height
    );
  GuiRequestDraw (
    (UINT32) mPasswordEnter.Hdr.Obj.OffsetX,
    (UINT32) mPasswordEnter.Hdr.Obj.OffsetY,
    mPasswordEnter.Hdr.Obj.Width,
    mPasswordEnter.Hdr.Obj.Height
    );
}

STATIC
VOID
InternalRequestPasswordConfirmation (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  DrawContext->CursorOpacity = 0;
  mPasswordBoxOpacity = 0x100 / 2;
  mPasswordBox.RequestConfirm = TRUE;
  InternalRedrawPaswordBox ();
}

VOID
InternalPasswordViewKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);

  ASSERT (BaseX == 0);
  ASSERT (BaseY == 0);

  if (KeyEvent->Key.UnicodeChar == CHAR_NULL || KeyEvent->Key.UnicodeChar > 0xFF) {
    return;
  }

  if (KeyEvent->Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    InternalRequestPasswordConfirmation (DrawContext);
    return;
  }

  if (KeyEvent->Key.UnicodeChar != CHAR_BACKSPACE) {
    mPasswordBox.PasswordInfo.Password[mPasswordBox.PasswordInfo.PasswordLen] = (CHAR8) KeyEvent->Key.UnicodeChar;
    ++mPasswordBox.PasswordInfo.PasswordLen;
  } else if (mPasswordBox.PasswordInfo.PasswordLen > 0) {
    mPasswordBox.PasswordInfo.Password[mPasswordBox.PasswordInfo.PasswordLen] = 0;
    --mPasswordBox.PasswordInfo.PasswordLen;
  }

  ASSERT (mPasswordBox.Hdr.Parent == NULL);
  ASSERT (mPasswordBox.Hdr.Obj.OffsetX >= 0);
  ASSERT (mPasswordBox.Hdr.Obj.OffsetY >= 0);
  ASSERT (mPasswordBox.Hdr.Obj.OffsetX + mPasswordBox.Hdr.Obj.Width <= This->Width);
  ASSERT (mPasswordBox.Hdr.Obj.OffsetX + mPasswordBox.Hdr.Obj.Height <= This->Height);
  GuiRequestDraw (
    (UINT32) mPasswordBox.Hdr.Obj.OffsetX,
    (UINT32) mPasswordBox.Hdr.Obj.OffsetY,
    mPasswordBox.Hdr.Obj.Width,
    mPasswordBox.Hdr.Obj.Height
    );
}

VOID
InternalPasswordLockDraw (
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
  GuiDrawToBuffer (
    &Context->Icons[ICON_LOCK][ICON_TYPE_BASE],
    mCommonActionButtonsOpacity,
    DrawContext,
    BaseX,
    BaseY,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
}

VOID
InternalPasswordBoxDraw (
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
  CONST GUI_IMAGE *BoxImage;
  CONST GUI_IMAGE *DotImage;

  BoxImage = &Context->Icons[ICON_PASSWORD][ICON_TYPE_BASE];
  DotImage = &Context->Icons[ICON_DOT][ICON_TYPE_BASE];

  GuiDrawToBuffer (
    BoxImage,
    mPasswordBoxOpacity,
    DrawContext,
    BaseX,
    BaseY,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
  
  // TODO: Draw only requested width
  for (UINT8 Index = 0; Index < MIN (mPasswordBox.PasswordInfo.PasswordLen, mPasswordBox.MaxPasswordLen); ++Index) {
    GuiDrawChildImage (
      DotImage,
      mPasswordBoxOpacity,
      DrawContext,
      BaseX,
      BaseY,
      Index * (DotImage->Width + 5) + 8,
      (BoxImage->Height - DotImage->Height) / 2,
      OffsetX,
      OffsetY,
      Width,
      Height
      );
  }
}

GUI_OBJ *
InternalPasswordEnterPtrEvent (
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
  ButtonImage = &Context->Icons[ICON_ENTER][ICON_TYPE_BASE];

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
      ButtonImage = &Context->Icons[ICON_ENTER][ICON_TYPE_HELD];
    } else {
      InternalRequestPasswordConfirmation (DrawContext);
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

VOID
InternalPasswordEnterDraw (
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
    mPasswordBoxOpacity,
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

BOOLEAN
InternalPasswordExitLoop (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  BOOLEAN Confirmed;

  ASSERT (Context != NULL);

  if (mPasswordBox.RequestConfirm) {
    mPasswordBox.RequestConfirm = FALSE;
    InternalQueueIncorrectPassword (DrawContext);

    Confirmed = InternalConfirmPassword (DrawContext, Context);
    SecureZeroMem (&mPasswordBox.PasswordInfo, sizeof (mPasswordBox.PasswordInfo));
    return Confirmed;
  }

  return FALSE;
}

STATIC GUI_OBJ_CHILD *mPasswordViewChilds[] = {
  &mPasswordLock,
  &mPasswordBox.Hdr,
  &mPasswordEnter.Hdr,
  &mCommonActionButtonsContainer
};

STATIC GUI_VIEW_CONTEXT mPasswordViewContext = {
  InternalCommonViewDraw,
  GuiObjDelegatePtrEvent,
  ARRAY_SIZE (mPasswordViewChilds),
  mPasswordViewChilds,
  InternalPasswordViewKeyEvent,
  InternalGetCursorImage,
  InternalPasswordExitLoop
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mPasswordLock = {
  {
    0, 0, 0, 0,
    InternalPasswordLockDraw,
    GuiObjDelegatePtrEvent,
    0,
    NULL
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_PASSWORD_BOX mPasswordBox = {
  {
    {
      0, 0, 0, 0,
      InternalPasswordBoxDraw,
      GuiObjDelegatePtrEvent,
      0,
      NULL
    },
    NULL
  },
  { { '\0' }, 0 },
  FALSE,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mPasswordEnter = {
  {
    {
      0, 0, 0, 0,
      InternalPasswordEnterDraw,
      InternalPasswordEnterPtrEvent,
      0,
      NULL
    },
    NULL
  },
  NULL
};

BOOLEAN
InternalPasswordAnimateIntro (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  mCommonActionButtonsOpacity = (UINT8) GuiGetInterpolatedValue (
    &mCommonIntroOpacityInterpol,
    CurrentTime
    );
  mPasswordBoxOpacity = mCommonActionButtonsOpacity;
  GuiRequestDraw (
    (UINT32) mPasswordLock.Obj.OffsetX,
    (UINT32) mPasswordLock.Obj.OffsetY,
    mPasswordLock.Obj.Width,
    mPasswordLock.Obj.Height
    );
  GuiRequestDraw (
    (UINT32) mPasswordBox.Hdr.Obj.OffsetX,
    (UINT32) mPasswordBox.Hdr.Obj.OffsetY,
    mPasswordBox.Hdr.Obj.Width,
    mPasswordBox.Hdr.Obj.Height
    );
  GuiRequestDraw (
    (UINT32) mPasswordEnter.Hdr.Obj.OffsetX,
    (UINT32) mPasswordEnter.Hdr.Obj.OffsetY,
    mPasswordEnter.Hdr.Obj.Width,
    mPasswordEnter.Hdr.Obj.Height
    );
  GuiRequestDraw (
    (UINT32) mCommonActionButtonsContainer.Obj.OffsetX,
    (UINT32) mCommonActionButtonsContainer.Obj.OffsetY,
    mCommonActionButtonsContainer.Obj.Width,
    mCommonActionButtonsContainer.Obj.Height
    );
  
  return CurrentTime - mCommonIntroOpacityInterpol.StartTime >= mCommonIntroOpacityInterpol.Duration;
}

EFI_STATUS
PasswordViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext
  )
{
  CONST GUI_IMAGE *LockImage;
  CONST GUI_IMAGE *BoxImage;
  CONST GUI_IMAGE *DotImage;

  CommonViewInitialize (DrawContext, GuiContext, &mPasswordViewContext);

  LockImage = &GuiContext->Icons[ICON_LOCK][ICON_TYPE_BASE];
  BoxImage  = &GuiContext->Icons[ICON_PASSWORD][ICON_TYPE_BASE];
  DotImage  = &GuiContext->Icons[ICON_DOT][ICON_TYPE_BASE];

  mPasswordLock.Obj.Width   = LockImage->Width;
  mPasswordLock.Obj.Height  = LockImage->Height;
  mPasswordLock.Obj.OffsetX = (DrawContext->Screen.Width - mPasswordLock.Obj.Width) / 2;
  mPasswordLock.Obj.OffsetY = (DrawContext->Screen.Height - mPasswordLock.Obj.Height) / 2;

  mPasswordBox.Hdr.Obj.Width   = BoxImage->Width;
  mPasswordBox.Hdr.Obj.Height  = BoxImage->Height;
  mPasswordBox.Hdr.Obj.OffsetX = (DrawContext->Screen.Width - mPasswordBox.Hdr.Obj.Width) / 2;
  mPasswordBox.Hdr.Obj.OffsetY = mPasswordLock.Obj.OffsetY + mPasswordLock.Obj.Height + 30;
  mPasswordBox.MaxPasswordLen  = (UINT8) (mPasswordBox.Hdr.Obj.Width / DotImage->Width);

  mPasswordEnter.CurrentImage   = &GuiContext->Icons[ICON_ENTER][ICON_TYPE_BASE];
  mPasswordEnter.Hdr.Obj.Width  = mPasswordEnter.CurrentImage->Width;
  mPasswordEnter.Hdr.Obj.Height = mPasswordEnter.CurrentImage->Height;

  mPasswordEnter.Hdr.Obj.OffsetX = mPasswordBox.Hdr.Obj.OffsetX + mPasswordBox.Hdr.Obj.Width - 25 * DrawContext->Scale;
  mPasswordEnter.Hdr.Obj.OffsetY = mPasswordBox.Hdr.Obj.OffsetY + ((INT32) mPasswordBox.Hdr.Obj.Height - (INT32) mPasswordEnter.CurrentImage->Height) / 2;

  if (!GuiContext->PickerContext->PickerAudioAssist) {
    InternalCommonIntroOpacoityInterpolInit ();
    STATIC GUI_ANIMATION PickerAnim;
    PickerAnim.Context = NULL;
    PickerAnim.Animate = InternalPasswordAnimateIntro;
    InsertHeadList (&DrawContext->Animations, &PickerAnim.Link);  
  }

  return EFI_SUCCESS;
}

VOID
PasswordViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN OUT BOOT_PICKER_GUI_CONTEXT  *GuiContext
  )
{
  GuiViewDeinitialize (DrawContext, GuiContext);
}

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcMiscLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../OpenCanopy.h"
#include "../GuiApp.h"
#include "../GuiIo.h"
#include "BootPicker.h"

#include "Common.h"

#define PASSWORD_FIRST_DOT_SPACE  11U
//
// Due to the embedded ENTER button of the 'Modern' theme, the last dot needs
// less padding.
//
#define PASSWORD_LAST_DOT_SPACE   9U
#define PASSWORD_INTER_DOT_SPACE  5U
//
// Offset the ENTER button is offset into the right end of the password prompt.
//
#define PASSWORD_ENTER_INTERNAL_OFFSET  25U
//
// Space between the password lock and the password box.
//
#define PASSWORD_BOX_SPACE  30U

typedef struct {
  CHAR8 Password[OC_PASSWORD_MAX_LEN];
  UINT8 PasswordLen;
} GUI_PASSWORD_INFO;

typedef struct {
  GUI_OBJ_CHILD     Hdr;
  GUI_PASSWORD_INFO PasswordInfo;
  BOOLEAN           RequestConfirm;
  UINT8             MaxPasswordDots;
} GUI_PASSWORD_BOX;

extern GUI_KEY_CONTEXT   *mKeyContext;

extern GUI_PASSWORD_BOX  mPasswordBox;
extern GUI_OBJ_CLICKABLE mPasswordEnter;
extern GUI_OBJ_CHILD     mPasswordLock;
extern GUI_OBJ_CHILD     mPasswordBoxContainer;

// FIXME: Create one context for drawing, pointers, and keys.
extern GUI_POINTER_CONTEXT *mPointerContext;

STATIC GUI_INTERPOLATION mPasswordIncorrectInterpol = {
  GuiInterpolTypeLinear,
  0,
  3,
  0,
  0,
  0
};

STATIC GUI_OBJ *mPasswordFocusList[] = {
  &mPasswordBoxContainer.Obj,
  &mCommonRestart.Hdr.Obj,
  &mCommonShutDown.Hdr.Obj
};

STATIC GUI_OBJ *mPasswordFocusListMinimal[] = {
  &mPasswordBoxContainer.Obj
};

STATIC UINT8 mPasswordNumTries = 0;

STATIC
VOID
InternalInitPasswordIncorrectInterpol (
  IN CONST GUI_DRAWING_CONTEXT  *DrawContext,
  IN UINT64                     StartTime
  )
{
  mPasswordIncorrectInterpol.StartTime = StartTime;
  mPasswordIncorrectInterpol.EndValue  = 20 * DrawContext->Scale;
}

BOOLEAN
InternalPasswordAnimateIncorrect (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  STATIC UINT32  LastOff   = 0;
  STATIC BOOLEAN Left      = FALSE;
  STATIC UINT8   Iteration = 0;

  UINT32 DeltaOrig;
  UINT32 DeltaAdd;
  
  DeltaOrig = GuiGetInterpolatedValue (
    &mPasswordIncorrectInterpol,
    CurrentTime
    );
  DeltaAdd = DeltaOrig - LastOff;
  LastOff  = DeltaOrig;

  if (Left) {
    mPasswordBoxContainer.Obj.OffsetX -= DeltaAdd;
    GuiRequestDraw (
      (UINT32) mPasswordBoxContainer.Obj.OffsetX,
      (UINT32) mPasswordBoxContainer.Obj.OffsetY,
      mPasswordBoxContainer.Obj.Width + DeltaAdd,
      mPasswordBoxContainer.Obj.Height
      );
  } else {
    GuiRequestDraw (
      (UINT32) mPasswordBoxContainer.Obj.OffsetX,
      (UINT32) mPasswordBoxContainer.Obj.OffsetY,
      mPasswordBoxContainer.Obj.Width + DeltaAdd,
      mPasswordBoxContainer.Obj.Height
      );
    mPasswordBoxContainer.Obj.OffsetX += DeltaAdd;
  }

  if (CurrentTime - mPasswordIncorrectInterpol.StartTime >= mPasswordIncorrectInterpol.Duration) {
    LastOff = 0;
    
    if (Iteration == 5) {
      Iteration = 0;
      mPasswordEnter.Hdr.Obj.Opacity = 0xFF;
      return TRUE;
    }

    mPasswordIncorrectInterpol.StartTime = CurrentTime + 1;
    ++Iteration;
    //
    // Left/Right start toggles between tries.
    //
    Left = !Left;
  }
  
  return FALSE;
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
    InternalInitPasswordIncorrectInterpol (DrawContext, DrawContext->FrameTime);
    InsertHeadList (&DrawContext->Animations, &PwAnim.Link);
  }
}

STATIC
VOID
InternalRedrawPaswordBox (
  VOID
  )
{
  GuiRequestDraw (
    (UINT32) mPasswordBoxContainer.Obj.OffsetX,
    (UINT32) mPasswordBoxContainer.Obj.OffsetY,
    mPasswordBoxContainer.Obj.Width,
    mPasswordBoxContainer.Obj.Height
    );
}

BOOLEAN
InternalConfirmPassword (
  IN OUT GUI_DRAWING_CONTEXT            *DrawContext,
  IN     CONST BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  BOOLEAN                    Result;
  CONST OC_PRIVILEGE_CONTEXT *Privilege;
  GUI_PTR_POSITION           PtrPosition;

  GuiPointerGetPosition (mPointerContext, &PtrPosition);

  Privilege = Context->PickerContext->PrivilegeContext;

  Result = Context->PickerContext->VerifyPassword (
    (CONST UINT8 *) mPasswordBox.PasswordInfo.Password,
    mPasswordBox.PasswordInfo.PasswordLen,
    Privilege
    );
  
  GuiKeyReset (mKeyContext);
  GuiPointerReset (mPointerContext);
  GuiPointerSetPosition (mPointerContext, &PtrPosition);
  DrawContext->CursorOpacity = 0xFF;

  SecureZeroMem (mPasswordBox.PasswordInfo.Password, mPasswordBox.PasswordInfo.PasswordLen);

  if (Result) {
    if (Context->PickerContext->PickerAudioAssist) {
      Context->PickerContext->PlayAudioFile (
        Context->PickerContext,
        OcVoiceOverAudioFilePasswordAccepted,
        TRUE
        );
    }

    mPasswordNumTries = 0;
    return TRUE;
  }
  
  if (Context->PickerContext->PickerAudioAssist) {
    Context->PickerContext->PlayAudioFile (
      Context->PickerContext,
      OcVoiceOverAudioFilePasswordIncorrect,
      TRUE
      );
  }

  ++mPasswordNumTries;
  if (mPasswordNumTries == OC_PASSWORD_MAX_RETRIES) {
    if (Context->PickerContext->PickerAudioAssist) {
      Context->PickerContext->PlayAudioFile (
        Context->PickerContext,
        OcVoiceOverAudioFilePasswordRetryLimit,
        TRUE
        );
      DEBUG ((DEBUG_WARN, "OCB: User failed to verify password %d times running\n", OC_PASSWORD_MAX_RETRIES));
    }
    //
    // Hide all UI except the password lock when retries have been exceeded.
    //
    mPasswordBoxContainer.Obj.Opacity = 0;
    InternalRedrawPaswordBox ();

    mCommonActionButtonsContainer.Obj.Opacity = 0;
    GuiRequestDraw (
      (UINT32) mCommonActionButtonsContainer.Obj.OffsetX,
      (UINT32) mCommonActionButtonsContainer.Obj.OffsetY,
      mCommonActionButtonsContainer.Obj.Width,
      mCommonActionButtonsContainer.Obj.Height
      );

    DrawContext->CursorOpacity = 0;
  }

  return FALSE;
}

STATIC
VOID
InternalRequestPasswordConfirmation (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  DrawContext->CursorOpacity = 0;
  mPasswordEnter.Hdr.Obj.Opacity = 0x100 / 2;
  mPasswordBox.RequestConfirm = TRUE;
  InternalRedrawPaswordBox ();
    GuiRequestDraw (
    (UINT32) (mPasswordBoxContainer.Obj.OffsetX + mPasswordEnter.Hdr.Obj.OffsetX),
    (UINT32) (mPasswordBoxContainer.Obj.OffsetY + mPasswordEnter.Hdr.Obj.OffsetY),
    mPasswordEnter.Hdr.Obj.Width,
    mPasswordEnter.Hdr.Obj.Height
    );
}

VOID
InternalPasswordBoxKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  if (KeyEvent->UnicodeChar == CHAR_CARRIAGE_RETURN) {
    //
    // RETURN finalizes the input.
    //
    InternalRequestPasswordConfirmation (DrawContext);
    return;
  }

  if (KeyEvent->UnicodeChar == CHAR_BACKSPACE) {
    //
    // Delete the last entered character, if such exists.
    //
    if (mPasswordBox.PasswordInfo.PasswordLen > 0) {
      --mPasswordBox.PasswordInfo.PasswordLen;
      mPasswordBox.PasswordInfo.Password[mPasswordBox.PasswordInfo.PasswordLen] = 0;
    }

    if (Context->PickerContext->PickerAudioAssist) {
      Context->PickerContext->PlayAudioFile (
        Context->PickerContext,
        AppleVoiceOverAudioFileBeep,
        TRUE
        );
    }
  } else if (KeyEvent->UnicodeChar != CHAR_NULL
   && KeyEvent->UnicodeChar == (CHAR8) KeyEvent->UnicodeChar
   && mPasswordBox.PasswordInfo.PasswordLen < ARRAY_SIZE (mPasswordBox.PasswordInfo.Password)) {
    mPasswordBox.PasswordInfo.Password[mPasswordBox.PasswordInfo.PasswordLen] = (CHAR8) KeyEvent->UnicodeChar;
    ++mPasswordBox.PasswordInfo.PasswordLen;
  } else {
    //
    // Only ASCII characters are supported.
    //
    if (Context->PickerContext->PickerAudioAssist) {
      Context->PickerContext->PlayAudioBeep (
        Context->PickerContext,
        OC_VOICE_OVER_SIGNALS_ERROR,
        OC_VOICE_OVER_SIGNAL_ERROR_MS,
        OC_VOICE_OVER_SILENCE_ERROR_MS
        );
    }

    return;
  }

  ASSERT (mPasswordBoxContainer.Parent == NULL);
  ASSERT (mPasswordBoxContainer.Obj.OffsetX >= 0);
  ASSERT (mPasswordBoxContainer.Obj.OffsetY >= 0);
  ASSERT (mPasswordBoxContainer.Obj.OffsetX + mPasswordBoxContainer.Obj.Width <= DrawContext->Screen.Width);
  ASSERT (mPasswordBoxContainer.Obj.OffsetX + mPasswordBoxContainer.Obj.Height <= DrawContext->Screen.Height);
  GuiRequestDraw (
    (UINT32) mPasswordBoxContainer.Obj.OffsetX,
    (UINT32) mPasswordBoxContainer.Obj.OffsetY,
    mPasswordBoxContainer.Obj.Width,
    mPasswordBoxContainer.Obj.Height
    );
}

VOID
InternalPasswordBoxFocus (
  IN     CONST GUI_OBJ        *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     BOOLEAN              Focus
  )
{
  if (Focus) {
    mPasswordBoxContainer.Obj.Opacity = 0xFF;

    DrawContext->GuiContext->AudioPlaybackTimeout = 0;
    DrawContext->GuiContext->VoAction = CanopyVoFocusPassword;
  } else {
    mPasswordBoxContainer.Obj.Opacity = 0x100 / 2;
  }

  InternalRedrawPaswordBox ();
}

VOID
InternalPasswordViewKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  //
  // Disable input when maximum password retries have been reached.
  //
  if (mPasswordNumTries >= OC_PASSWORD_MAX_RETRIES) {
    return;
  }

  if (KeyEvent->OcKeyCode == OC_INPUT_VOICE_OVER) {
    DrawContext->GuiContext->PickerContext->ToggleVoiceOver (
      DrawContext->GuiContext->PickerContext,
      OcVoiceOverAudioFileEnterPassword
      );
    return;
  }

  InternalFocusKeyHandler (
    DrawContext,
    Context,
    KeyEvent
    );
}

GUI_OBJ *
InternalPasswordViewPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  //
  // Disable input when maximum password retries have been reached.
  //
  if (mPasswordNumTries >= OC_PASSWORD_MAX_RETRIES) {
    return NULL;
  }

  return InternalCommonViewPtrEvent (
    This,
    DrawContext,
    Context,
    BaseX,
    BaseY,
    Event
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
  IN     UINT32                  Height,
  IN     UINT8                   Opacity
  )
{
  GuiDrawToBuffer (
    &Context->Icons[ICON_LOCK][ICON_TYPE_BASE],
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
  IN     UINT32                  Height,
  IN     UINT8                   Opacity
  )
{
  CONST GUI_IMAGE *BoxImage;
  CONST GUI_IMAGE *DotImage;
  UINT8           DotIndex;

  BoxImage = &Context->Icons[ICON_PASSWORD][ICON_TYPE_BASE];
  DotImage = &Context->Icons[ICON_DOT][ICON_TYPE_BASE];

  GuiDrawToBuffer (
    BoxImage,
    Opacity,
    DrawContext,
    BaseX,
    BaseY,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
  //
  // Selective drawing is unlikely worth it.
  //
  for (
    DotIndex = 0;
    DotIndex < MIN (mPasswordBox.PasswordInfo.PasswordLen, mPasswordBox.MaxPasswordDots);
    ++DotIndex
    ) {
    GuiDrawChildImage (
      DotImage,
      Opacity,
      DrawContext,
      BaseX,
      BaseY,
      DotIndex * (DotImage->Width + PASSWORD_INTER_DOT_SPACE * DrawContext->Scale) + PASSWORD_FIRST_DOT_SPACE * DrawContext->Scale,
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
      InternalRequestPasswordConfirmation (DrawContext);
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

BOOLEAN
InternalPasswordExitLoop (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  BOOLEAN Confirmed;

  ASSERT (Context != NULL);
  //
  // This is done here to have one more draw loop iteration to flush hiding
  // the UI elements.
  //
  if (mPasswordNumTries >= OC_PASSWORD_MAX_RETRIES) {
    gBS->Stall (SECONDS_TO_MICROSECONDS (5));
    ResetWarm ();
  }

  if (mPasswordBox.RequestConfirm) {
    mPasswordBox.RequestConfirm = FALSE;

    if (!Context->PickerContext->PickerAudioAssist) {
      InternalQueueIncorrectPassword (DrawContext);
    }

    Confirmed = InternalConfirmPassword (DrawContext, Context);
    SecureZeroMem (&mPasswordBox.PasswordInfo, sizeof (mPasswordBox.PasswordInfo));
    return Confirmed;
  }

  return FALSE;
}

STATIC GUI_OBJ_CHILD *mPasswordViewChilds[] = {
  &mPasswordLock,
  &mPasswordBoxContainer,
  &mCommonActionButtonsContainer
};

STATIC GUI_OBJ_CHILD *mPasswordViewChildsMinimal[] = {
  &mPasswordLock,
  &mPasswordBoxContainer
};

STATIC GUI_VIEW_CONTEXT mPasswordViewContext = {
  InternalCommonViewDraw,
  InternalPasswordViewPtrEvent,
  ARRAY_SIZE (mPasswordViewChilds),
  mPasswordViewChilds,
  InternalPasswordViewKeyEvent,
  InternalGetCursorImage,
  InternalPasswordExitLoop,
  mPasswordFocusList,
  ARRAY_SIZE (mPasswordFocusList)
};

STATIC GUI_VIEW_CONTEXT mPasswordViewContextMinimal = {
  InternalCommonViewDraw,
  InternalPasswordViewPtrEvent,
  ARRAY_SIZE (mPasswordViewChildsMinimal),
  mPasswordViewChildsMinimal,
  InternalPasswordViewKeyEvent,
  InternalGetCursorImage,
  InternalPasswordExitLoop,
  mPasswordFocusListMinimal,
  ARRAY_SIZE (mPasswordFocusListMinimal)
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mPasswordLock = {
  {
    0, 0, 0, 0, 0xFF,
    InternalPasswordLockDraw,
    NULL,
    GuiObjDelegatePtrEvent,
    NULL,
    0,
    NULL
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_PASSWORD_BOX mPasswordBox = {
  {
    {
      0, 0, 0, 0, 0xFF,
      InternalPasswordBoxDraw,
      NULL,
      GuiObjDelegatePtrEvent,
      NULL,
      0,
      NULL
    },
    &mPasswordBoxContainer.Obj
  },
  { { '\0' }, 0 },
  FALSE,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mPasswordEnter = {
  {
    {
      0, 0, 0, 0, 0xFF,
      InternalCommonSimpleButtonDraw,
      NULL,
      InternalPasswordEnterPtrEvent,
      NULL,
      0,
      NULL
    },
    &mPasswordBoxContainer.Obj
  },
  0,
  0
};

STATIC GUI_OBJ_CHILD *mPasswordBoxContainerChilds[] = {
  &mPasswordBox.Hdr,
  &mPasswordEnter.Hdr
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mPasswordBoxContainer = {
  {
    0, 0, 0, 0, 0xFF,
    GuiObjDrawDelegate,
    InternalPasswordBoxKeyEvent,
    GuiObjDelegatePtrEvent,
    InternalPasswordBoxFocus,
    ARRAY_SIZE (mPasswordBoxContainerChilds),
    mPasswordBoxContainerChilds
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
  UINT8 Opacity;

  Opacity = (UINT8) GuiGetInterpolatedValue (
    &mCommonIntroOpacityInterpol,
    CurrentTime
    );
  DrawContext->Screen.Opacity = Opacity;

  GuiRequestDraw (
    (UINT32) mPasswordLock.Obj.OffsetX,
    (UINT32) mPasswordLock.Obj.OffsetY,
    mPasswordLock.Obj.Width,
    mPasswordLock.Obj.Height
    );
  GuiRequestDraw (
    (UINT32) mPasswordBoxContainer.Obj.OffsetX,
    (UINT32) mPasswordBoxContainer.Obj.OffsetY,
    mPasswordBoxContainer.Obj.Width,
    mPasswordBoxContainer.Obj.Height
    );
  GuiRequestDraw (
    (UINT32) mCommonActionButtonsContainer.Obj.OffsetX,
    (UINT32) mCommonActionButtonsContainer.Obj.OffsetY,
    mCommonActionButtonsContainer.Obj.Width,
    mCommonActionButtonsContainer.Obj.Height
    );
  
  return CurrentTime - mCommonIntroOpacityInterpol.StartTime >= mCommonIntroOpacityInterpol.Duration;
}

STATIC GUI_ANIMATION mPasswordIntroAnimation = {
  INITIALIZE_LIST_HEAD_VARIABLE (mPasswordIntroAnimation.Link),
  NULL,
  InternalPasswordAnimateIntro
};

EFI_STATUS
PasswordViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext
  )
{
  CONST GUI_IMAGE *LockImage;
  CONST GUI_IMAGE *BoxImage;
  CONST GUI_IMAGE *DotImage;
  CONST GUI_IMAGE *EnterImage;
  UINT32          EnterOffset;
  UINT32          BoxOffset;
  UINT32          ImageMidOffset;
  UINT32          FirstOffset;
  UINT32          EnterInternalOffset;

  mKeyContext->KeyFilter = OC_PICKER_KEYS_FOR_TYPING;

  CommonViewInitialize (
    DrawContext,
    GuiContext,
    (GuiContext->PickerContext->PickerAttributes & OC_ATTR_USE_MINIMAL_UI) == 0
      ? &mPasswordViewContext
      : &mPasswordViewContextMinimal
    );

  LockImage  = &GuiContext->Icons[ICON_LOCK][ICON_TYPE_BASE];
  BoxImage   = &GuiContext->Icons[ICON_PASSWORD][ICON_TYPE_BASE];
  DotImage   = &GuiContext->Icons[ICON_DOT][ICON_TYPE_BASE];
  EnterImage = &GuiContext->Icons[ICON_ENTER][ICON_TYPE_BASE];

  if (EnterImage->Height > BoxImage->Height) {
    BoxOffset   = (EnterImage->Height - BoxImage->Height) / 2;
    EnterOffset = 0;
  } else {
    BoxOffset   = 0;
    EnterOffset = (BoxImage->Height - EnterImage->Height) / 2;
  }

  mPasswordLock.Obj.Width   = LockImage->Width;
  mPasswordLock.Obj.Height  = LockImage->Height;
  mPasswordLock.Obj.OffsetX = (DrawContext->Screen.Width - mPasswordLock.Obj.Width) / 2;
  mPasswordLock.Obj.OffsetY = (DrawContext->Screen.Height - mPasswordLock.Obj.Height) / 2;

  ImageMidOffset = (EnterImage->Height / 2) * EnterImage->Width;
  for (
    FirstOffset = ImageMidOffset;
    FirstOffset < ImageMidOffset + EnterImage->Width;
    ++FirstOffset
  ) {
    if (EnterImage->Buffer[FirstOffset].Reserved > 0) {
      break;
    }
  }

  if (PASSWORD_ENTER_INTERNAL_OFFSET * DrawContext->Scale > FirstOffset - ImageMidOffset) {
    EnterInternalOffset = PASSWORD_ENTER_INTERNAL_OFFSET * DrawContext->Scale - (FirstOffset - ImageMidOffset);
  } else {
    EnterInternalOffset = 0;
  }

  mPasswordBox.Hdr.Obj.Width   = BoxImage->Width;
  mPasswordBox.Hdr.Obj.Height  = BoxImage->Height;
  mPasswordBox.Hdr.Obj.OffsetX = 0;
  mPasswordBox.Hdr.Obj.OffsetY = BoxOffset;
  mPasswordBox.MaxPasswordDots  = (UINT8) (
    (mPasswordBox.Hdr.Obj.Width - EnterInternalOffset - (PASSWORD_FIRST_DOT_SPACE + PASSWORD_LAST_DOT_SPACE - PASSWORD_INTER_DOT_SPACE) * DrawContext->Scale) / (DotImage->Width + PASSWORD_INTER_DOT_SPACE * DrawContext->Scale)
    );

  mPasswordEnter.Hdr.Obj.Width   = EnterImage->Width;
  mPasswordEnter.Hdr.Obj.Height  = EnterImage->Height;
  mPasswordEnter.Hdr.Obj.OffsetX = mPasswordBox.Hdr.Obj.Width - PASSWORD_ENTER_INTERNAL_OFFSET * DrawContext->Scale;
  mPasswordEnter.Hdr.Obj.OffsetY = EnterOffset;
  mPasswordEnter.ImageId         = ICON_ENTER;
  mPasswordEnter.ImageState      = ICON_TYPE_BASE;

  mPasswordBoxContainer.Obj.Width   = BoxImage->Width + EnterImage->Width - PASSWORD_ENTER_INTERNAL_OFFSET * DrawContext->Scale;
  mPasswordBoxContainer.Obj.Height  = MAX (BoxImage->Height, EnterImage->Height);
  mPasswordBoxContainer.Obj.OffsetX = (DrawContext->Screen.Width - mPasswordBox.Hdr.Obj.Width) / 2;
  mPasswordBoxContainer.Obj.OffsetY = mPasswordLock.Obj.OffsetY + mPasswordLock.Obj.Height + PASSWORD_BOX_SPACE * DrawContext->Scale - BoxOffset;

  if (!GuiContext->PickerContext->PickerAudioAssist) {
    //
    // Fade-in the entire screen.
    //
    DrawContext->Screen.Opacity = 0;

    InsertHeadList (&DrawContext->Animations, &mPasswordIntroAnimation.Link);  
  }
  //
  // Action Buttons fade-in is handled by screen opacity.
  //
  mCommonActionButtonsContainer.Obj.Opacity = 0xFF;

  if (GuiContext->PickerContext->PickerAudioAssist) {
    GuiContext->PickerContext->PlayAudioFile (
      GuiContext->PickerContext,
      OcVoiceOverAudioFileEnterPassword,
      TRUE
      );
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

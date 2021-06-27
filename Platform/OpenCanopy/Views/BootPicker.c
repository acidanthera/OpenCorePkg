/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <IndustryStandard/AppleIcon.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcPngLib.h>
#include <Library/OcStorageLib.h>
#include <Library/OcMiscLib.h>

#include "../OpenCanopy.h"
#include "../BmfLib.h"
#include "../GuiApp.h"
#include "../GuiIo.h"
#include "BootPicker.h"

#include "Common.h"
#include "../Blending.h"

#define BOOT_LABEL_WRAPAROUND_PADDING   30U
#define BOOT_LABEL_SCROLLING_HOLD_TIME  180U

#define BOOT_LABEL_SHADOW_WIDTH   8U
#define BOOT_LABEL_SHADOW_HEIGHT  BOOT_ENTRY_LABEL_HEIGHT

extern GUI_KEY_CONTEXT   *mKeyContext;

extern GUI_VOLUME_PICKER mBootPicker;
extern GUI_OBJ_CHILD     mBootPickerContainer;
extern GUI_OBJ_CHILD     mBootPickerSelectorContainer;
extern GUI_OBJ_CHILD     mBootPickerSelectorBackground;
extern GUI_OBJ_CLICKABLE mBootPickerSelectorButton;
extern GUI_OBJ_CLICKABLE mBootPickerRightScroll;
extern GUI_OBJ_CLICKABLE mBootPickerLeftScroll;
extern CONST GUI_IMAGE   mBackgroundImage;

// STATIC UINT8 mBootPickerImageIndex = 0;

extern INT64 mBackgroundImageOffsetX;
extern INT64 mBackgroundImageOffsetY;

STATIC UINT32 mBootPickerLabelScrollHoldTime = 0;

STATIC GUI_OBJ *mBootPickerFocusList[] = {
  &mBootPicker.Hdr.Obj,
  &mCommonRestart.Hdr.Obj,
  &mCommonShutDown.Hdr.Obj
};

STATIC GUI_OBJ *mBootPickerFocusListMinimal[] = {
  &mBootPicker.Hdr.Obj
};

STATIC
GUI_VOLUME_ENTRY *
InternalGetVolumeEntry (
  IN UINT32  Index
  )
{
  ASSERT (Index < mBootPicker.Hdr.Obj.NumChildren);
  return (GUI_VOLUME_ENTRY *) (
    mBootPicker.Hdr.Obj.Children[Index]
    );
}

STATIC
VOID
InternalRedrawVolumeLabel (
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     CONST GUI_VOLUME_ENTRY  *Entry
  )
{
  GuiRequestDrawCrop (
    DrawContext,
    mBootPickerContainer.Obj.OffsetX + mBootPicker.Hdr.Obj.OffsetX + Entry->Hdr.Obj.OffsetX,
    (mBootPickerContainer.Obj.OffsetY + mBootPicker.Hdr.Obj.OffsetY + Entry->Hdr.Obj.OffsetY + Entry->Hdr.Obj.Height - Entry->Label.Height),
    Entry->Hdr.Obj.Width,
    Entry->Label.Height
    );
}

BOOLEAN
InternalBootPickerAnimateLabel (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  GUI_VOLUME_ENTRY *Entry;

  ASSERT (DrawContext != NULL);

  if (mBootPickerLabelScrollHoldTime < BOOT_LABEL_SCROLLING_HOLD_TIME) {
    ++mBootPickerLabelScrollHoldTime;
    return FALSE;
  }

  Entry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
  if (Entry->Label.Width <= Entry->Hdr.Obj.Width) {
    return FALSE;
  }

  Entry->ShowLeftShadow = TRUE;
  Entry->LabelOffset -= DrawContext->Scale;

  if (Entry->LabelOffset <= -(INT16) Entry->Label.Width) {
    //
    // Hide the left shadow once the first label is hidden.
    //
    Entry->ShowLeftShadow = FALSE;
    //
    // If the second drawn label reaches the front, switch back to the first.
    //
    if (Entry->LabelOffset <= -(INT16) (Entry->Label.Width + BOOT_LABEL_WRAPAROUND_PADDING * DrawContext->Scale)) {
      Entry->LabelOffset = 0;
      mBootPickerLabelScrollHoldTime = 0;
    }
  }

  InternalRedrawVolumeLabel (DrawContext, Entry);

  return FALSE;
}

STATIC GUI_ANIMATION mBootPickerLabelAnimation = {
  INITIALIZE_LIST_HEAD_VARIABLE (mBootPickerLabelAnimation.Link),
  NULL,
  InternalBootPickerAnimateLabel
};

VOID
InternalStartAnimateLabel (
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     CONST GUI_VOLUME_ENTRY  *Entry
  )
{
  ASSERT (!IsNodeInList (&DrawContext->Animations, &mBootPickerLabelAnimation.Link));
  //
  // Reset the boot entry label scrolling timer.
  //
  mBootPickerLabelScrollHoldTime = 0;

  if (Entry->Label.Width > Entry->Hdr.Obj.Width) {
    //
    // Add the animation if the next entry needs scrolling.
    //
    InsertHeadList (&DrawContext->Animations, &mBootPickerLabelAnimation.Link);
  }
}

VOID
InternalStopAnimateLabel (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN OUT GUI_VOLUME_ENTRY     *Entry
  )
{
  if (Entry->Label.Width > Entry->Hdr.Obj.Width) {
    //
    // Reset the label of the old entry back to its default position.
    //
    if (Entry->LabelOffset != 0) {
      Entry->ShowLeftShadow = FALSE;
      Entry->LabelOffset    = 0;
      InternalRedrawVolumeLabel (DrawContext, Entry);
    }

    RemoveEntryList (&mBootPickerLabelAnimation.Link);
    InitializeListHead (&mBootPickerLabelAnimation.Link);
  }
}

VOID
InternalBootPickerSelectEntry (
  IN OUT GUI_VOLUME_PICKER   *This,
  IN OUT GUI_DRAWING_CONTEXT *DrawContext,
  IN     UINT32              NewIndex
  )
{
  GUI_VOLUME_ENTRY       *OldEntry;
  CONST GUI_VOLUME_ENTRY *NewEntry;

  ASSERT (This != NULL);
  ASSERT (NewIndex < mBootPicker.Hdr.Obj.NumChildren);

  OldEntry = InternalGetVolumeEntry (This->SelectedIndex);
  This->SelectedIndex = NewIndex;
  NewEntry = InternalGetVolumeEntry (NewIndex);

  ASSERT (NewEntry->Hdr.Obj.Width <= mBootPickerSelectorContainer.Obj.Width);
  ASSERT_EQUALS (This->Hdr.Obj.Height, mBootPickerSelectorContainer.Obj.OffsetY + mBootPickerSelectorContainer.Obj.Height);

  mBootPickerSelectorContainer.Obj.OffsetX = mBootPicker.Hdr.Obj.OffsetX + NewEntry->Hdr.Obj.OffsetX - (mBootPickerSelectorContainer.Obj.Width - NewEntry->Hdr.Obj.Width) / 2;

  if (DrawContext != NULL) {
    if (OldEntry->Label.Width > OldEntry->Hdr.Obj.Width) {
      ASSERT (IsNodeInList (&DrawContext->Animations, &mBootPickerLabelAnimation.Link));
    }

    InternalStopAnimateLabel (DrawContext, OldEntry);
    InternalStartAnimateLabel (DrawContext, NewEntry);
    //
    // Set voice timeout to N frames from now.
    //
    DrawContext->GuiContext->AudioPlaybackTimeout = OC_VOICE_OVER_IDLE_TIMEOUT_MS;
    DrawContext->GuiContext->VoAction = CanopyVoSelectedEntry;
    DrawContext->GuiContext->BootEntry = NewEntry->Context;
  }
}

INT64
InternelBootPickerScrollSelected (
  VOID
  )
{
  CONST GUI_VOLUME_ENTRY *SelectedEntry;
  INT64                  EntryOffsetX;
  UINT32                 EntryWidth;

  if (mBootPicker.Hdr.Obj.NumChildren == 1) {
    return 0;
  }
  //
  // If the selected entry is outside of the view, scroll it accordingly.
  //
  SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
  EntryOffsetX  = mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX - (mBootPickerSelectorContainer.Obj.Width - SelectedEntry->Hdr.Obj.Width) / 2;
  EntryWidth    = SelectedEntry->Hdr.Obj.Width + (mBootPickerSelectorContainer.Obj.Width - SelectedEntry->Hdr.Obj.Width);

  if (EntryOffsetX < 0) {
    return -EntryOffsetX;
  }
  
  if (EntryOffsetX + EntryWidth > mBootPickerContainer.Obj.Width) {
    return -((EntryOffsetX + EntryWidth) - mBootPickerContainer.Obj.Width);
  }

  return 0;
}

VOID
InternalUpdateScrollButtons (
  VOID
  )
{
  if (mBootPicker.Hdr.Obj.OffsetX < 0) {
    mBootPickerLeftScroll.Hdr.Obj.Opacity = 0xFF;
  } else {
    mBootPickerLeftScroll.Hdr.Obj.Opacity = 0;
  }

  if (mBootPicker.Hdr.Obj.OffsetX + mBootPicker.Hdr.Obj.Width > mBootPickerContainer.Obj.Width) {
    mBootPickerRightScroll.Hdr.Obj.Opacity = 0xFF;
  } else {
    mBootPickerRightScroll.Hdr.Obj.Opacity = 0;
  }
}

VOID
InternalBootPickerScroll (
  IN OUT GUI_VOLUME_PICKER    *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     INT64                ScrollOffset
  )
{
  INT64 ScrollX;
  INT64 ScrollY;

  mBootPicker.Hdr.Obj.OffsetX += ScrollOffset;
  mBootPickerSelectorContainer.Obj.OffsetX += ScrollOffset;
  InternalUpdateScrollButtons ();
  //
  // The entry list has been scrolled, the entire horizontal space to also cover
  // the scroll buttons.
  //
  DEBUG_CODE_BEGIN ();
  GuiGetBaseCoords (
    &mBootPickerLeftScroll.Hdr.Obj,
    DrawContext,
    &ScrollX,
    &ScrollY
    );
  ASSERT (ScrollY >= BaseY);
  ASSERT (ScrollY + mBootPickerLeftScroll.Hdr.Obj.Height <= BaseY + This->Hdr.Obj.Height);

  GuiGetBaseCoords (
    &mBootPickerRightScroll.Hdr.Obj,
    DrawContext,
    &ScrollX,
    &ScrollY
    );
  ASSERT (ScrollY >= BaseY);
  ASSERT (ScrollY + mBootPickerRightScroll.Hdr.Obj.Height <= BaseY + This->Hdr.Obj.Height);
  DEBUG_CODE_END ();
  //
  // The container is constructed such that it is always fully visible.
  //
  ASSERT (This->Hdr.Obj.Height <= mBootPickerContainer.Obj.Height);
  ASSERT (BaseY + This->Hdr.Obj.Height <= DrawContext->Screen.Height);
  
  GuiRequestDraw (
    0,
    (UINT32) BaseY,
    DrawContext->Screen.Width,
    This->Hdr.Obj.Height
    );
}

VOID
InternalBootPickerChangeEntry (
  IN OUT GUI_VOLUME_PICKER    *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               NewIndex
  )
{
  INT64 ScrollOffset;
  INT64 OldSelectorOffsetX;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (NewIndex < This->Hdr.Obj.NumChildren);
  //
  // The caller must guarantee the entry is actually new for performance
  // reasons.
  //
  ASSERT (This->SelectedIndex != NewIndex);

  OldSelectorOffsetX = mBootPickerSelectorContainer.Obj.OffsetX;
  InternalBootPickerSelectEntry (This, DrawContext, NewIndex);

  ScrollOffset = InternelBootPickerScrollSelected ();
  if (ScrollOffset == 0) {
    GuiRequestDrawCrop (
      DrawContext,
      mBootPickerContainer.Obj.OffsetX + OldSelectorOffsetX,
      mBootPickerContainer.Obj.OffsetY + mBootPickerSelectorContainer.Obj.OffsetY,
      mBootPickerSelectorContainer.Obj.Width,
      mBootPickerSelectorContainer.Obj.Height
      );

    GuiRequestDrawCrop (
      DrawContext,
      mBootPickerContainer.Obj.OffsetX + mBootPickerSelectorContainer.Obj.OffsetX,
      mBootPickerContainer.Obj.OffsetY + mBootPickerSelectorContainer.Obj.OffsetY,
      mBootPickerSelectorContainer.Obj.Width,
      mBootPickerSelectorContainer.Obj.Height
      );
  } else {
    InternalBootPickerScroll (
      This,
      DrawContext,
      BaseX,
      BaseY,
      ScrollOffset
      );
  }
}

VOID
InternalBootPickerKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *GuiContext,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  GUI_VOLUME_PICKER       *Picker;
  INT64                   BaseX;
  INT64                   BaseY;
  CONST GUI_VOLUME_ENTRY  *SelectedEntry;
  UINT8                   ImageId;

  ASSERT (This != NULL);
  ASSERT (GuiContext != NULL);
  ASSERT (DrawContext != NULL);

  if ((KeyEvent->OcModifiers & OC_MODIFIERS_SET_DEFAULT) == 0) {
    ImageId = ICON_SELECTOR;
  } else {
    ImageId = ICON_SET_DEFAULT;
  }

  if (mBootPickerSelectorButton.ImageId != ImageId) {
    mBootPickerSelectorButton.ImageId = ImageId;
    GuiRequestDraw (
      (UINT32) (mBootPickerContainer.Obj.OffsetX + mBootPickerSelectorContainer.Obj.OffsetX + mBootPickerSelectorButton.Hdr.Obj.OffsetX),
      (UINT32) (mBootPickerContainer.Obj.OffsetY + mBootPickerSelectorContainer.Obj.OffsetY + mBootPickerSelectorButton.Hdr.Obj.OffsetY),
      mBootPickerSelectorButton.Hdr.Obj.Width,
      mBootPickerSelectorButton.Hdr.Obj.Height
      );
  }

  Picker = BASE_CR (This, GUI_VOLUME_PICKER, Hdr.Obj);

  BaseX = mBootPickerContainer.Obj.OffsetX + mBootPicker.Hdr.Obj.OffsetX;
  BaseY = mBootPickerContainer.Obj.OffsetY + mBootPicker.Hdr.Obj.OffsetY;

  if (KeyEvent->OcKeyCode == OC_INPUT_RIGHT) {
    if (mBootPicker.Hdr.Obj.NumChildren > 1) {
      InternalBootPickerChangeEntry (
        Picker,
        DrawContext,
        BaseX,
        BaseY,
        mBootPicker.SelectedIndex + 1 < mBootPicker.Hdr.Obj.NumChildren
          ? mBootPicker.SelectedIndex + 1
          : 0
        );
    }
  } else if (KeyEvent->OcKeyCode == OC_INPUT_LEFT) {
    ASSERT (mBootPicker.Hdr.Obj.NumChildren > 0);
    if (mBootPicker.Hdr.Obj.NumChildren > 1) {
      InternalBootPickerChangeEntry (
        Picker,
        DrawContext,
        BaseX,
        BaseY,
        mBootPicker.SelectedIndex > 0
          ? mBootPicker.SelectedIndex - 1
          : mBootPicker.Hdr.Obj.NumChildren - 1
        );
    }
  } else if (KeyEvent->OcKeyCode == OC_INPUT_CONTINUE) {
    if (mBootPicker.Hdr.Obj.NumChildren > 0) {
      SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
      SelectedEntry->Context->SetDefault = (KeyEvent->OcModifiers & OC_MODIFIERS_SET_DEFAULT) != 0;
      GuiContext->ReadyToBoot = TRUE;
      ASSERT (GuiContext->BootEntry == SelectedEntry->Context);
    }
  } else if (mBootPickerContainer.Obj.Opacity != 0xFF) {
    //
    // FIXME: Other keys are not allowed when boot picker is partially transparent.
    //
    return;
  }

  if (KeyEvent->OcKeyCode == OC_INPUT_MORE) {
    //
    // Match Builtin picker logic here: only refresh if the keypress makes a change
    //
    if (GuiContext->HideAuxiliary) {
      GuiContext->HideAuxiliary = FALSE;
      GuiContext->Refresh = TRUE;
      DrawContext->GuiContext->PickerContext->PlayAudioFile (
        DrawContext->GuiContext->PickerContext,
        OcVoiceOverAudioFileShowAuxiliary,
        FALSE
        );
    }
  } else if (KeyEvent->OcKeyCode == OC_INPUT_ABORTED) {
    GuiContext->Refresh = TRUE;
    DrawContext->GuiContext->PickerContext->PlayAudioFile (
      DrawContext->GuiContext->PickerContext,
      OcVoiceOverAudioFileReloading,
      FALSE
      );
  }
}

STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mBootPickerLabelLeftShadowBuffer[2 * BOOT_LABEL_SHADOW_WIDTH * 2 * BOOT_LABEL_SHADOW_HEIGHT];
STATIC GUI_IMAGE mBootPickerLabelLeftShadowImage;

STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mBootPickerLabelRightShadowBuffer[2 * BOOT_LABEL_SHADOW_WIDTH * 2 * BOOT_LABEL_SHADOW_HEIGHT];
STATIC GUI_IMAGE mBootPickerLabelRightShadowImage;

VOID
InternalInitialiseLabelShadows (
  IN CONST GUI_DRAWING_CONTEXT  *DrawContext,
  IN BOOT_PICKER_GUI_CONTEXT    *GuiContext
  )
{
  UINT32 RowOffset;
  UINT32 ColumnOffset;
  UINT32 MaxOffset;
  UINT8  Opacity;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL LeftPixel;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL RightPixel;

  ASSERT (DrawContext->Scale <= 2);

  mBootPickerLabelLeftShadowImage.Width  = BOOT_LABEL_SHADOW_WIDTH * DrawContext->Scale;
  mBootPickerLabelLeftShadowImage.Height = BOOT_LABEL_SHADOW_HEIGHT * DrawContext->Scale;
  mBootPickerLabelLeftShadowImage.Buffer = mBootPickerLabelLeftShadowBuffer;

  mBootPickerLabelRightShadowImage.Width  = BOOT_LABEL_SHADOW_WIDTH * DrawContext->Scale;
  mBootPickerLabelRightShadowImage.Height = BOOT_LABEL_SHADOW_HEIGHT * DrawContext->Scale;
  mBootPickerLabelRightShadowImage.Buffer = mBootPickerLabelRightShadowBuffer;

  MaxOffset = mBootPickerLabelLeftShadowImage.Width * mBootPickerLabelLeftShadowImage.Height;

  for (
    ColumnOffset = 0;
    ColumnOffset < mBootPickerLabelLeftShadowImage.Width;
    ++ColumnOffset
    ) {
    Opacity = (UINT8) (((ColumnOffset + 1) * 0xFF) / (mBootPickerLabelLeftShadowImage.Width + 1));

    LeftPixel.Blue = RGB_APPLY_OPACITY (
      GuiContext->BackgroundColor.Pixel.Blue,
      0xFF - Opacity
      );
    LeftPixel.Green = RGB_APPLY_OPACITY (
      GuiContext->BackgroundColor.Pixel.Green,
      0xFF - Opacity
      );
    LeftPixel.Red = RGB_APPLY_OPACITY (
      GuiContext->BackgroundColor.Pixel.Red,
      0xFF - Opacity
      );
    LeftPixel.Reserved = 0xFF - Opacity;

    RightPixel.Blue =
      RGB_APPLY_OPACITY (
        GuiContext->BackgroundColor.Pixel.Blue,
        Opacity
        );
    RightPixel.Green =
      RGB_APPLY_OPACITY (
        GuiContext->BackgroundColor.Pixel.Green,
        Opacity
        );
    RightPixel.Red =
      RGB_APPLY_OPACITY (
        GuiContext->BackgroundColor.Pixel.Red,
        Opacity
        );
    RightPixel.Reserved = Opacity;

    for (
      RowOffset = 0;
      RowOffset < MaxOffset;
      RowOffset += mBootPickerLabelLeftShadowImage.Width
      ) {
      mBootPickerLabelLeftShadowBuffer[RowOffset + ColumnOffset].Blue     = LeftPixel.Blue;
      mBootPickerLabelLeftShadowBuffer[RowOffset + ColumnOffset].Green    = LeftPixel.Green;
      mBootPickerLabelLeftShadowBuffer[RowOffset + ColumnOffset].Red      = LeftPixel.Red;
      mBootPickerLabelLeftShadowBuffer[RowOffset + ColumnOffset].Reserved = LeftPixel.Reserved;

      mBootPickerLabelRightShadowBuffer[RowOffset + ColumnOffset].Blue     = RightPixel.Blue;
      mBootPickerLabelRightShadowBuffer[RowOffset + ColumnOffset].Green    = RightPixel.Green;
      mBootPickerLabelRightShadowBuffer[RowOffset + ColumnOffset].Red      = RightPixel.Red;
      mBootPickerLabelRightShadowBuffer[RowOffset + ColumnOffset].Reserved = Opacity;
    }
  }
}

STATIC
VOID
InternalBootPickerEntryDraw (
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
  CONST GUI_VOLUME_ENTRY *Entry;
  CONST GUI_IMAGE        *EntryIcon;
  CONST GUI_IMAGE        *Label;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  Entry       = BASE_CR (This, GUI_VOLUME_ENTRY, Hdr.Obj);
  /*if (mBootPickerImageIndex < 5) {
    EntryIcon = &((BOOT_PICKER_GUI_CONTEXT *) DrawContext->GuiContext)->Poof[mBootPickerImageIndex];
  } else */{
    EntryIcon   = &Entry->EntryIcon;
  }
  Label       = &Entry->Label;
  //
  // Draw the icon horizontally centered.
  //
  ASSERT (EntryIcon != NULL);
  ASSERT_EQUALS (EntryIcon->Width,  This->Width);

  if (OffsetY < EntryIcon->Height) {
    GuiDrawToBuffer (
      EntryIcon,
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
  //
  // Draw the label horizontally centered.
  //

  //
  // FIXME: Apple allows the label to be up to 340px wide,
  // but OpenCanopy can't display it now (it would overlap adjacent entries)
  //
  //ASSERT (Label->Width  <= BOOT_ENTRY_DIMENSION * DrawContext->Scale);
  ASSERT (Label->Height <= BOOT_ENTRY_LABEL_HEIGHT * DrawContext->Scale);

  GuiDrawChildImage (
    Label,
    Opacity,
    DrawContext,
    BaseX,
    BaseY,
    Entry->LabelOffset,
    This->Height - Label->Height,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
  //
  // If the label needs scrolling, draw a second one to get a wraparound effect.
  //
  if (Entry->Label.Width > This->Width) {
    GuiDrawChildImage (
      Label,
      Opacity,
      DrawContext,
      BaseX,
      BaseY,
      (INT64) Entry->LabelOffset + Entry->Label.Width + BOOT_LABEL_WRAPAROUND_PADDING * DrawContext->Scale,
      This->Height - Label->Height,
      OffsetX,
      OffsetY,
      Width,
      Height
      );

    if (Entry->ShowLeftShadow) {
      GuiDrawChildImage (
        &mBootPickerLabelLeftShadowImage,
        Opacity,
        DrawContext,
        BaseX,
        BaseY,
        0,
        This->Height - mBootPickerLabelLeftShadowImage.Height,
        OffsetX,
        OffsetY,
        Width,
        Height
        );
    }

    GuiDrawChildImage (
      &mBootPickerLabelRightShadowImage,
      Opacity,
      DrawContext,
      BaseX,
      BaseY,
      This->Width - mBootPickerLabelRightShadowImage.Width,
      This->Height - mBootPickerLabelRightShadowImage.Height,
      OffsetX,
      OffsetY,
      Width,
      Height
      );
  }
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);
}

STATIC
GUI_OBJ *
InternalBootPickerEntryPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  GUI_VOLUME_ENTRY        *Entry;
  BOOLEAN                 IsHit;
  UINT32                  OffsetX;
  UINT32                  OffsetY;

  OffsetX = (UINT32) (Event->Pos.Pos.X - BaseX);
  OffsetY = (UINT32) (Event->Pos.Pos.Y - BaseY);

  ASSERT (Event->Type == GuiPointerPrimaryDown
       || Event->Type == GuiPointerPrimaryUp
       || Event->Type == GuiPointerPrimaryDoubleClick);

  Entry = BASE_CR (This, GUI_VOLUME_ENTRY, Hdr.Obj);

  IsHit = GuiClickableIsHit (
            &Entry->EntryIcon,
            OffsetX,
            OffsetY
            );
  if (!IsHit) {
    return This;
  }

  if (Event->Type == GuiPointerPrimaryDown) {
    if (mBootPicker.SelectedIndex != Entry->Index) {
      ASSERT (Entry->Hdr.Parent == &mBootPicker.Hdr.Obj);
      InternalBootPickerChangeEntry (
        &mBootPicker,
        DrawContext,
        BaseX - This->OffsetX,
        BaseY - This->OffsetY,
        Entry->Index
        );
    }
  } else if (Event->Type == GuiPointerPrimaryDoubleClick) {
    //
    // This must be ensured because the UI directs Move/Up events to the object
    // Down had been sent to.
    //
    ASSERT (mBootPicker.SelectedIndex == Entry->Index);
    Context->ReadyToBoot = TRUE;
  }
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);
  return This;
}

VOID
InternalBootPickerSelectorBackgroundDraw (
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
  CONST GUI_IMAGE *BackgroundImage;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  BackgroundImage = &Context->Icons[ICON_SELECTED][ICON_TYPE_BASE];

  ASSERT_EQUALS (This->Width,  BackgroundImage->Width);
  ASSERT_EQUALS (This->Height, BackgroundImage->Height);

  GuiDrawToBuffer (
    BackgroundImage,
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
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);
}

GUI_OBJ *
InternalBootPickerSelectorPtrEvent (
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
      Context->ReadyToBoot = TRUE;
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

GUI_OBJ *
InternalBootPickerLeftScrollPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  UINT8                  Result;

  INT64                  BootPickerX;
  INT64                  BootPickerY;
  CONST GUI_VOLUME_ENTRY *SelectedEntry;

  if (This->Opacity == 0) {
    return NULL;
  }

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
      //
      // The view can only be scrolled when there are off-screen entries.
      //
      GuiGetBaseCoords (
        &mBootPicker.Hdr.Obj,
        DrawContext,
        &BootPickerX,
        &BootPickerY
        );
      //
      // Scroll the boot entry view by one spot.
      //
      InternalBootPickerScroll (
        &mBootPicker,
        DrawContext,
        BootPickerX,
        BootPickerY,
        (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * Context->Scale
        );
      //
      // If the selected entry is pushed off-screen by scrolling, select the
      // appropriate neighbour entry.
      //
      SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
      if (mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.Width > mBootPickerContainer.Obj.Width) {
        //
        // The internal design ensures a selected entry cannot be off-screen,
        // scrolling offsets it by at most one spot.
        //
        InternalBootPickerSelectEntry (
          &mBootPicker,
          DrawContext,
          mBootPicker.SelectedIndex > 0
            ? mBootPicker.SelectedIndex - 1
            : mBootPicker.Hdr.Obj.NumChildren - 1
          );

          SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
          ASSERT (!(mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.Width > mBootPickerContainer.Obj.Width));
      }
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

GUI_OBJ *
InternalBootPickerRightScrollPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_PTR_EVENT     *Event
  )
{
  UINT8            Result;

  INT64            BootPickerX;
  INT64            BootPickerY;
  GUI_VOLUME_ENTRY *SelectedEntry;

  if (This->Opacity == 0) {
    return NULL;
  }

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
      //
      // The view can only be scrolled when there are off-screen entries.
      //
      GuiGetBaseCoords (
        &mBootPicker.Hdr.Obj,
        DrawContext,
        &BootPickerX,
        &BootPickerY
        );
      //
      // Scroll the boot entry view by one spot.
      //
      InternalBootPickerScroll (
        &mBootPicker,
        DrawContext,
        BootPickerX,
        BootPickerY,
        -(INT64) (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * Context->Scale
        );
      //
      // If the selected entry is pushed off-screen by scrolling, select the
      // appropriate neighbour entry.
      //
      SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
      if (mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX < 0) {
        //
        // The internal design ensures a selected entry cannot be off-screen,
        // scrolling offsets it by at most one spot.
        //
        InternalBootPickerSelectEntry (
          &mBootPicker,
          DrawContext,
          mBootPicker.SelectedIndex + 1 < mBootPicker.Hdr.Obj.NumChildren
            ? mBootPicker.SelectedIndex + 1
            : 0
          );

        SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
        ASSERT (!(mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX < 0));
      }
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

STATIC GUI_IMAGE mVersionLabelImage;

VOID
InternalVersionLabelDraw (
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
  if (mVersionLabelImage.Buffer != NULL) {
    GuiDrawToBuffer (
      &mVersionLabelImage,
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
}

VOID
InternalBootPickerViewKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  GUI_OBJ          *FocusChangedObj;
  GUI_VOLUME_ENTRY *Entry;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);

  if (KeyEvent->OcKeyCode == OC_INPUT_VOICE_OVER) {
    DrawContext->GuiContext->PickerContext->ToggleVoiceOver (
      DrawContext->GuiContext->PickerContext,
      0
      );
    return;
  }

  Entry = InternalGetVolumeEntry(mBootPicker.SelectedIndex);

  FocusChangedObj = InternalFocusKeyHandler (
    DrawContext,
    Context,
    KeyEvent
    );
  if (FocusChangedObj == &mBootPicker.Hdr.Obj) {
    InternalStartAnimateLabel (DrawContext, Entry);
  } else if (FocusChangedObj != NULL) {
    if (!IsListEmpty (&mBootPickerLabelAnimation.Link)) {
      InternalStopAnimateLabel (DrawContext, Entry);
    }
  }
}

VOID
InternalBootPickerFocus (
  IN     CONST GUI_OBJ        *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     BOOLEAN              Focus
  )
{
  if (!Focus) {
    mBootPickerSelectorContainer.Obj.Opacity = 0;
  } else {
    mBootPickerSelectorContainer.Obj.Opacity = 0xFF;

    DrawContext->GuiContext->AudioPlaybackTimeout = 0;
    DrawContext->GuiContext->VoAction = CanopyVoSelectedEntry;
  }

  GuiRequestDraw (
    (UINT32) (mBootPickerContainer.Obj.OffsetX + mBootPickerSelectorContainer.Obj.OffsetX),
    (UINT32) (mBootPickerContainer.Obj.OffsetY + mBootPickerSelectorContainer.Obj.OffsetY),
    mBootPickerSelectorContainer.Obj.Width,
    mBootPickerSelectorContainer.Obj.Height
    );
}

BOOLEAN
InternalBootPickerExitLoop (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  if (Context->ReadyToBoot) {
    ASSERT (Context->BootEntry == InternalGetVolumeEntry (mBootPicker.SelectedIndex)->Context);
  }

  return Context->ReadyToBoot || Context->Refresh;
}

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mBootPickerSelectorBackground = {
  {
    0, 0, 0, 0, 0xFF,
    InternalBootPickerSelectorBackgroundDraw,
    NULL,
    GuiObjDelegatePtrEvent,
    NULL,
    0,
    NULL
  },
  &mBootPickerSelectorContainer.Obj
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mBootPickerSelectorButton = {
  {
    {
      0, 0, 0, 0, 0xFF,
      InternalCommonSimpleButtonDraw,
      NULL,
      InternalBootPickerSelectorPtrEvent,
      NULL,
      0,
      NULL
    },
    &mBootPickerSelectorContainer.Obj
  },
  0,
  0
};

STATIC GUI_OBJ_CHILD *mBootPickerSelectorContainerChilds[] = {
  &mBootPickerSelectorBackground,
  &mBootPickerSelectorButton.Hdr
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mBootPickerSelectorContainer = {
  {
    0, 0, 0, 0, 0xFF,
    GuiObjDrawDelegate,
    NULL,
    GuiObjDelegatePtrEvent,
    NULL,
    ARRAY_SIZE (mBootPickerSelectorContainerChilds),
    mBootPickerSelectorContainerChilds
  },
  &mBootPickerContainer.Obj
};

STATIC GUI_OBJ_CHILD *mBootPickerContainerChilds[] = {
  &mBootPickerSelectorContainer,
  &mBootPicker.Hdr
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mBootPickerContainer = {
  {
    0, 0, 0, 0, 0xFF,
    GuiObjDrawDelegate,
    NULL,
    GuiObjDelegatePtrEvent,
    NULL,
    ARRAY_SIZE (mBootPickerContainerChilds),
    mBootPickerContainerChilds
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_VOLUME_PICKER mBootPicker = {
  {
    {
      0, 0, 0, 0, 0xFF,
      GuiObjDrawDelegate,
      InternalBootPickerKeyEvent,
      GuiObjDelegatePtrEvent,
      InternalBootPickerFocus,
      0,
      NULL
    },
    &mBootPickerContainer.Obj
  },
  1
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mBootPickerLeftScroll = {
  {
    {
      0, 0, 0, 0, 0xFF,
      InternalCommonSimpleButtonDraw,
      NULL,
      InternalBootPickerLeftScrollPtrEvent,
      NULL,
      0,
      NULL
    },
    NULL
  },
  0,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mBootPickerRightScroll = {
  {
    {
      0, 0, 0, 0, 0xFF,
      InternalCommonSimpleButtonDraw,
      NULL,
      InternalBootPickerRightScrollPtrEvent,
      NULL,
      0,
      NULL
    },
    NULL
  },
  0,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mBootPickerVersionLabel = {
  {
    0, 0, 0, 0, 0xFF,
    InternalVersionLabelDraw,
    NULL,
    GuiObjDelegatePtrEvent,
    NULL,
    0,
    NULL
  },
  NULL
};

STATIC GUI_OBJ_CHILD *mBootPickerViewChilds[] = {
  &mBootPickerContainer,
  &mCommonActionButtonsContainer,
  &mBootPickerLeftScroll.Hdr,
  &mBootPickerRightScroll.Hdr,
  &mBootPickerVersionLabel
};

STATIC GUI_OBJ_CHILD *mBootPickerViewChildsMinimal[] = {
  &mBootPickerContainer,
  &mBootPickerLeftScroll.Hdr,
  &mBootPickerRightScroll.Hdr,
  &mBootPickerVersionLabel
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_VIEW_CONTEXT mBootPickerViewContext = {
  InternalCommonViewDraw,
  InternalCommonViewPtrEvent,
  ARRAY_SIZE (mBootPickerViewChilds),
  mBootPickerViewChilds,
  InternalBootPickerViewKeyEvent,
  InternalGetCursorImage,
  InternalBootPickerExitLoop,
  mBootPickerFocusList,
  ARRAY_SIZE (mBootPickerFocusList)
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_VIEW_CONTEXT mBootPickerViewContextMinimal = {
  InternalCommonViewDraw,
  InternalCommonViewPtrEvent,
  ARRAY_SIZE (mBootPickerViewChildsMinimal),
  mBootPickerViewChildsMinimal,
  InternalBootPickerViewKeyEvent,
  InternalGetCursorImage,
  InternalBootPickerExitLoop,
  mBootPickerFocusListMinimal,
  ARRAY_SIZE (mBootPickerFocusListMinimal)
};

STATIC
EFI_STATUS
CopyLabel (
  OUT GUI_IMAGE       *Destination,
  IN  CONST GUI_IMAGE *Source
  )
{
  Destination->Width = Source->Width;
  Destination->Height = Source->Height;
  Destination->Buffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) AllocateCopyPool (
    sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Source->Width * Source->Height,
    Source->Buffer
    );

  if (Destination->Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BootPickerEntriesSet (
  IN OC_PICKER_CONTEXT              *Context,
  IN BOOT_PICKER_GUI_CONTEXT        *GuiContext,
  IN OC_BOOT_ENTRY                  *Entry,
  IN UINT8                          EntryIndex
  )
{
  EFI_STATUS                  Status;
  GUI_VOLUME_ENTRY            *VolumeEntry;
  CONST GUI_IMAGE             *SuggestedIcon;
  CONST GUI_VOLUME_ENTRY      *PrevEntry;
  UINT32                      IconFileSize;
  UINT32                      IconTypeIndex;
  VOID                        *IconFileData;
  BOOLEAN                     UseVolumeIcon;
  BOOLEAN                     UseFlavourIcon;
  BOOLEAN                     UseDiskLabel;
  BOOLEAN                     UseGenericLabel;
  BOOLEAN                     Result;
  CHAR16                      *EntryName;
  UINTN                       EntryNameLength;
  CHAR8                       *FlavourNameStart;
  CHAR8                       *FlavourNameEnd;

  ASSERT (GuiContext != NULL);
  ASSERT (Entry != NULL);
  ASSERT (EntryIndex < mBootPicker.Hdr.Obj.NumChildren);

  DEBUG ((DEBUG_INFO, "OCUI: Console attributes: %d\n", Context->ConsoleAttributes));

  UseVolumeIcon   = (Context->PickerAttributes & OC_ATTR_USE_VOLUME_ICON) != 0;
  UseFlavourIcon  = (Context->PickerAttributes & OC_ATTR_USE_FLAVOUR_ICON) != 0;
  UseDiskLabel    = (Context->PickerAttributes & OC_ATTR_USE_DISK_LABEL_FILE) != 0;
  UseGenericLabel = (Context->PickerAttributes & OC_ATTR_USE_GENERIC_LABEL_IMAGE) != 0;

  DEBUG ((DEBUG_INFO, "OCUI: UseDiskLabel: %d, UseGenericLabel: %d\n", UseDiskLabel, UseGenericLabel));

  VolumeEntry = AllocateZeroPool (sizeof (*VolumeEntry));
  if (VolumeEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (UseDiskLabel) {
    Status = Context->GetEntryLabelImage (
      Context,
      Entry,
      GuiContext->Scale,
      &IconFileData,
      &IconFileSize
      );
    if (!EFI_ERROR (Status)) {
      Status = GuiLabelToImage (
        &VolumeEntry->Label,
        IconFileData,
        IconFileSize,
        GuiContext->Scale,
        GuiContext->LightBackground
        );
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status) && UseGenericLabel) {
    switch (Entry->Type) {
      case OC_BOOT_APPLE_OS:
        Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_APPLE]);
        break;
      case OC_BOOT_APPLE_FW_UPDATE:
      case OC_BOOT_APPLE_RECOVERY:
        Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_APPLE_RECOVERY]);
        break;
      case OC_BOOT_APPLE_TIME_MACHINE:
        Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_APPLE_TIME_MACHINE]);
        break;
      case OC_BOOT_WINDOWS:
        Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_WINDOWS]);
        break;
      case OC_BOOT_EXTERNAL_OS:
        Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_OTHER]);
        break;
      case OC_BOOT_RESET_NVRAM:
        Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_RESET_NVRAM]);
        break;
      case OC_BOOT_TOGGLE_SIP:
        ASSERT (
          StrCmp (Entry->Name, OC_MENU_SIP_IS_DISABLED) == 0 ||
          StrCmp (Entry->Name, OC_MENU_SIP_IS_ENABLED) == 0
          );
        if (StrCmp (Entry->Name, OC_MENU_SIP_IS_DISABLED) == 0) {
          Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_SIP_IS_DISABLED]);
        } else {
          Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_SIP_IS_ENABLED]);
        }
        break;
      case OC_BOOT_EXTERNAL_TOOL:
        if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_RESET_NVRAM) != NULL) {
          Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_RESET_NVRAM]);
        } else if (OcAsciiStriStr (Entry->Flavour, OC_FLAVOUR_ID_UEFI_SHELL) != NULL) {
          Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_SHELL]);
        } else {
          Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_TOOL]);
        }
        break;
      case OC_BOOT_UNKNOWN:
        Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_GENERIC_HDD]);
        break;
      default:
        DEBUG ((DEBUG_WARN, "OCUI: Entry kind %d unsupported for label\n", Entry->Type));
        return EFI_UNSUPPORTED;
    }
  }

  if (EFI_ERROR (Status)) {
    EntryName       = Entry->Name;
    EntryNameLength = StrLen (Entry->Name);
    if (Entry->IsFolder) {
      EntryName = AllocatePool (EntryNameLength * sizeof (CHAR16) + L_STR_SIZE (L" (dmg)"));
      if (EntryName != NULL) {
        CopyMem (EntryName, Entry->Name, EntryNameLength * sizeof (CHAR16));
        CopyMem (&EntryName[EntryNameLength], L" (dmg)", L_STR_SIZE (L" (dmg)"));
        EntryNameLength += L_STR_LEN (L" (dmg)");
      } else {
        EntryName = Entry->Name;
      }
    }

    ASSERT (StrLen (EntryName) == EntryNameLength);

    Result = GuiGetLabel (
      &VolumeEntry->Label,
      &GuiContext->FontContext,
      EntryName,
      EntryNameLength,
      GuiContext->LightBackground
      );
    
    if (EntryName != Entry->Name) {
      FreePool (EntryName);
    }

    if (!Result) {
      DEBUG ((DEBUG_WARN, "OCUI: label failed\n"));
      return EFI_UNSUPPORTED;
    }
  }

  VolumeEntry->Context = Entry;

  //
  // Load volume icons when allowed.
  // Do not load volume icons for Time Machine entries unless explicitly enabled.
  // This works around Time Machine icon style incompatibilities.
  //
  if (UseVolumeIcon
    && (Entry->Type != OC_BOOT_APPLE_TIME_MACHINE
      || (Context->PickerAttributes & OC_ATTR_HIDE_THEMED_ICONS) == 0)) {
    Status = Context->GetEntryIcon (Context, Entry, &IconFileData, &IconFileSize);

    if (!EFI_ERROR (Status)) {
      Status = GuiIcnsToImageIcon (
        &VolumeEntry->EntryIcon,
        IconFileData,
        IconFileSize,
        GuiContext->Scale,
        BOOT_ENTRY_ICON_DIMENSION,
        BOOT_ENTRY_ICON_DIMENSION,
        FALSE
        );
      FreePool (IconFileData);
      if (!EFI_ERROR (Status)) {
        VolumeEntry->CustomIcon = TRUE;
      } else {
        DEBUG ((DEBUG_INFO, "OCUI: Failed to convert icon - %r\n", Status));
      }
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  //
  // Flavour system is used internally for icon priorities even when
  // user-specified flavours from .contentFlavour are not being read
  //
  if (EFI_ERROR (Status)) {
    ASSERT (Entry->Flavour != NULL);

    IconTypeIndex = Entry->IsExternal ? ICON_TYPE_EXTERNAL : ICON_TYPE_BASE;

    FlavourNameEnd = Entry->Flavour - 1;
    do
    {
      for (FlavourNameStart = ++FlavourNameEnd; *FlavourNameEnd != '\0' && *FlavourNameEnd != ':'; ++FlavourNameEnd);

      Status = InternalGetFlavourIcon (
        GuiContext,
        Context->StorageContext,
        FlavourNameStart,
        FlavourNameEnd - FlavourNameStart,
        IconTypeIndex,
        UseFlavourIcon,
        &VolumeEntry->EntryIcon,
        &VolumeEntry->CustomIcon
        );
    } while (EFI_ERROR (Status) && *FlavourNameEnd != '\0');

    if (EFI_ERROR (Status))
    {
      SuggestedIcon = NULL;

      if (Entry->Type == OC_BOOT_EXTERNAL_OS) {
          SuggestedIcon = &GuiContext->Icons[ICON_OTHER][IconTypeIndex];
      } else if (Entry->Type == OC_BOOT_EXTERNAL_TOOL || (Entry->Type & OC_BOOT_SYSTEM) != 0) {
          SuggestedIcon = &GuiContext->Icons[ICON_TOOL][IconTypeIndex];
      }

      if (SuggestedIcon == NULL || SuggestedIcon->Buffer == NULL) {
          SuggestedIcon = &GuiContext->Icons[ICON_GENERIC_HDD][IconTypeIndex];
      }

      CopyMem (&VolumeEntry->EntryIcon, SuggestedIcon, sizeof (VolumeEntry->EntryIcon));
    } else {
      DEBUG ((DEBUG_INFO, "OCUI: Using flavour icon, custom: %u\n", VolumeEntry->CustomIcon));
    }
  }

  VolumeEntry->Hdr.Parent       = &mBootPicker.Hdr.Obj;
  VolumeEntry->Hdr.Obj.Width    = BOOT_ENTRY_ICON_DIMENSION * GuiContext->Scale;
  VolumeEntry->Hdr.Obj.Height   = (BOOT_ENTRY_HEIGHT - BOOT_ENTRY_ICON_SPACE) * GuiContext->Scale;
  VolumeEntry->Hdr.Obj.Opacity  = 0xFF;
  VolumeEntry->Hdr.Obj.Draw     = InternalBootPickerEntryDraw;
  VolumeEntry->Hdr.Obj.PtrEvent = InternalBootPickerEntryPtrEvent;
  VolumeEntry->Hdr.Obj.NumChildren = 0;
  VolumeEntry->Hdr.Obj.Children    = NULL;
  if (VolumeEntry->Hdr.Obj.Width > VolumeEntry->Label.Width) {
    VolumeEntry->LabelOffset = (INT16) ((VolumeEntry->Hdr.Obj.Width - VolumeEntry->Label.Width) / 2);
  }

  if (EntryIndex > 0) {
    PrevEntry = InternalGetVolumeEntry (EntryIndex - 1);
    VolumeEntry->Hdr.Obj.OffsetX = PrevEntry->Hdr.Obj.OffsetX + (BOOT_ENTRY_DIMENSION + BOOT_ENTRY_SPACE) * GuiContext->Scale;
  } else {
    //
    // Account for the selector background.
    //
    VolumeEntry->Hdr.Obj.OffsetX = BOOT_ENTRY_ICON_SPACE * GuiContext->Scale;
  }

  VolumeEntry->Hdr.Obj.OffsetY = BOOT_ENTRY_ICON_SPACE * GuiContext->Scale;

  mBootPicker.Hdr.Obj.Children[EntryIndex] = &VolumeEntry->Hdr;
  VolumeEntry->Index = EntryIndex;
  mBootPicker.Hdr.Obj.Width   += (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * GuiContext->Scale;
  mBootPicker.Hdr.Obj.OffsetX -= (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * GuiContext->Scale / 2;

  return EFI_SUCCESS;
}

VOID
InternalBootPickerEntryDestruct (
  IN GUI_VOLUME_ENTRY  *Entry
  )
{
  ASSERT (Entry != NULL);
  ASSERT (Entry->Label.Buffer != NULL);

  if (Entry->CustomIcon) {
    FreePool (Entry->EntryIcon.Buffer);
  }

  FreePool (Entry->Label.Buffer);
  FreePool (Entry);
}

STATIC GUI_INTERPOLATION mBpAnimInfoImageList;

VOID
InitBpAnimImageList (
  IN GUI_INTERPOL_TYPE  Type,
  IN UINT64             StartTime,
  IN UINT64             Duration
  )
{
  mBpAnimInfoImageList.Type       = Type;
  mBpAnimInfoImageList.StartTime  = StartTime;
  mBpAnimInfoImageList.Duration   = Duration;
  mBpAnimInfoImageList.StartValue = 0;
  mBpAnimInfoImageList.EndValue   = 5;
}


BOOLEAN
InternalBootPickerAnimateImageList (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
#if 0
  GUI_VOLUME_ENTRY *Entry;
  CONST GUI_IMAGE  *EntryIcon;

  Entry       = BASE_CR (&mBootPicker.Hdr.Obj, GUI_VOLUME_ENTRY, Hdr.Obj);
  EntryIcon   = &Entry->EntryIcon;

  mBootPickerImageIndex++;
  mBootPickerImageIndex = (UINT8)GuiGetInterpolatedValue (&mBpAnimInfoImageList, CurrentTime);
  Entry->EntryIcon = &((GUI_IMAGE*)Context)[mBootPickerImageIndex];
  GuiRedrawObject (
    &mBootPicker.Hdr.Obj,
    DrawContext,
    mBootPicker.Hdr.Obj.OffsetX,
    mBootPicker.Hdr.Obj.OffsetY
    );

  if (mBootPickerImageIndex == mBpAnimInfoImageList.EndValue) {
    return TRUE;
  }
#endif
  return FALSE;
}

STATIC GUI_INTERPOLATION mBpAnimInfoSinMove = {
  GuiInterpolTypeSmooth,
  0,
  25,
  0,
  0,
  0
};

VOID
InitBpAnimIntro (
  IN CONST GUI_DRAWING_CONTEXT  *DrawContext
  )
{
  mBpAnimInfoSinMove.EndValue = 35 * DrawContext->Scale;
  //
  // FIXME: This assumes that only relative changes of X are performed on
  //        mBootPickerContainer between animation initialisation and start.
  //
  mBootPickerContainer.Obj.OffsetX += 35 * DrawContext->Scale;
}

BOOLEAN
InternalBootPickerAnimateIntro (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  STATIC UINT32 PrevSine = 0;

  UINT8  Opacity;
  UINT32 InterpolVal;
  UINT32 DeltaSine;

  ASSERT (DrawContext != NULL);

  Opacity = (UINT8) GuiGetInterpolatedValue (
    &mCommonIntroOpacityInterpol,
    CurrentTime
    );
  mBootPickerContainer.Obj.Opacity = Opacity;
  //
  // Animate the scroll buttons based on their active state.
  //
  if (mBootPicker.Hdr.Obj.OffsetX < 0) {
    mBootPickerLeftScroll.Hdr.Obj.Opacity = Opacity;
  }
  if (mBootPicker.Hdr.Obj.OffsetX + mBootPicker.Hdr.Obj.Width > mBootPickerContainer.Obj.Width) {
    mBootPickerRightScroll.Hdr.Obj.Opacity = Opacity;
  }
  //
  // If PasswordView already faded-in the action buttons, skip them.
  //
  if (mCommonActionButtonsContainer.Obj.Opacity != 0xFF) {
    mCommonActionButtonsContainer.Obj.Opacity = Opacity;
    //
    // The view is constructed such that the action buttons are always fully
    // visible.
    //
    ASSERT (mCommonActionButtonsContainer.Obj.OffsetX >= 0);
    ASSERT (mCommonActionButtonsContainer.Obj.OffsetY >= 0);
    ASSERT (mCommonActionButtonsContainer.Obj.OffsetX + mCommonActionButtonsContainer.Obj.Width <= DrawContext->Screen.Width);
    ASSERT (mCommonActionButtonsContainer.Obj.OffsetY + mCommonActionButtonsContainer.Obj.Height <= DrawContext->Screen.Height);
    GuiRequestDraw (
      (UINT32) mCommonActionButtonsContainer.Obj.OffsetX,
      (UINT32) mCommonActionButtonsContainer.Obj.OffsetY,
      mCommonActionButtonsContainer.Obj.Width,
      mCommonActionButtonsContainer.Obj.Height
      );
  }

  InterpolVal = GuiGetInterpolatedValue (&mBpAnimInfoSinMove, CurrentTime);
  DeltaSine = InterpolVal - PrevSine;
  mBootPickerContainer.Obj.OffsetX -= DeltaSine;
  PrevSine = InterpolVal;
  //
  // Draw the full dimension of the inner container to implicitly cover the
  // scroll buttons with the off-screen entries.
  //
  GuiRequestDrawCrop (
    DrawContext,
    mBootPickerContainer.Obj.OffsetX + mBootPicker.Hdr.Obj.OffsetX,
    mBootPickerContainer.Obj.OffsetY + mBootPicker.Hdr.Obj.OffsetY,
    mBootPicker.Hdr.Obj.Width + DeltaSine,
    mBootPicker.Hdr.Obj.Height
    );
  
  ASSERT (mBpAnimInfoSinMove.Duration == mCommonIntroOpacityInterpol.Duration);
  return CurrentTime - mBpAnimInfoSinMove.StartTime >= mBpAnimInfoSinMove.Duration;
}

STATIC GUI_INTERPOLATION mBootPickerTimeoutOpacityInterpolDown = {
  GuiInterpolTypeLinear,
  0,
  125,
  0xFF,
  0x50,
  0
};

STATIC GUI_INTERPOLATION mBootPickerTimeoutOpacityInterpolUp = {
  GuiInterpolTypeLinear,
  0,
  125,
  0x50,
  0xFF,
  20
};

BOOLEAN
InternalBootPickerAnimateTimeout (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  STATIC GUI_INTERPOLATION *CurInterpol  = &mBootPickerTimeoutOpacityInterpolDown;
  STATIC GUI_INTERPOLATION *NextInterpol = &mBootPickerTimeoutOpacityInterpolUp;

  GUI_INTERPOLATION *Temp;

  if (DrawContext->TimeOutSeconds > 0) {
    mBootPickerSelectorButton.Hdr.Obj.Opacity = (UINT8) GuiGetInterpolatedValue (
      CurInterpol,
      CurrentTime
      );
  } else {
    mBootPickerSelectorButton.Hdr.Obj.Opacity = 0xFF;
  }
  //
  // The view is constructed such that the action buttons are always fully
  // visible.
  //
  GuiRequestDraw (
    (UINT32) (mBootPickerContainer.Obj.OffsetX + mBootPickerSelectorContainer.Obj.OffsetX + mBootPickerSelectorButton.Hdr.Obj.OffsetX),
    (UINT32) (mBootPickerContainer.Obj.OffsetY + mBootPickerSelectorContainer.Obj.OffsetY + mBootPickerSelectorButton.Hdr.Obj.OffsetY),
    mBootPickerSelectorButton.Hdr.Obj.Width,
    mBootPickerSelectorButton.Hdr.Obj.Height
    );
  
  if (DrawContext->TimeOutSeconds == 0) {
    return TRUE;
  }
  
  if (CurrentTime - CurInterpol->StartTime >= CurInterpol->Duration + CurInterpol->HoldTime) {
    Temp         = CurInterpol;
    CurInterpol  = NextInterpol;
    NextInterpol = Temp;
    CurInterpol->StartTime = CurrentTime + 1;
  }

  return FALSE;
}

STATIC GUI_ANIMATION mBootPickerIntroAnimation = {
  INITIALIZE_LIST_HEAD_VARIABLE (mBootPickerIntroAnimation.Link),
  NULL,
  InternalBootPickerAnimateIntro
};

EFI_STATUS
BootPickerViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  GUI_CURSOR_GET_IMAGE     GetCursorImage,
  IN  UINT8                    NumBootEntries
  )
{
  CONST GUI_IMAGE *SelectorBackgroundImage;
  CONST GUI_IMAGE *SelectorButtonImage;
  UINT32          ContainerMaxWidth;
  UINT32          ContainerWidthDelta;
  UINTN           DestLen;
  CHAR16          *UString;
  BOOLEAN         Result;

  ASSERT (DrawContext != NULL);
  ASSERT (GuiContext != NULL);
  ASSERT (GetCursorImage != NULL);

  mKeyContext->KeyFilter = OC_PICKER_KEYS_FOR_PICKER;

  CommonViewInitialize (
    DrawContext,
    GuiContext,
    (GuiContext->PickerContext->PickerAttributes & OC_ATTR_USE_MINIMAL_UI) == 0
      ? &mBootPickerViewContext
      : &mBootPickerViewContextMinimal
    );

  mBackgroundImageOffsetX = DivS64x64Remainder (
    (INT64) DrawContext->Screen.Width - DrawContext->GuiContext->Background.Width,
    2,
    NULL
    );
  mBackgroundImageOffsetY = DivS64x64Remainder (
    (INT64) DrawContext->Screen.Height - DrawContext->GuiContext->Background.Height,
    2,
    NULL
    );
  
  SelectorBackgroundImage = &GuiContext->Icons[ICON_SELECTED][ICON_TYPE_BASE];
  SelectorButtonImage     = &GuiContext->Icons[ICON_SELECTOR][ICON_TYPE_BASE];

  mBootPickerSelectorBackground.Obj.OffsetX = 0;
  mBootPickerSelectorBackground.Obj.OffsetY = 0;
  mBootPickerSelectorBackground.Obj.Width   = SelectorBackgroundImage->Width;
  mBootPickerSelectorBackground.Obj.Height  = SelectorBackgroundImage->Height;

  ASSERT (SelectorBackgroundImage->Width >= SelectorButtonImage->Width);
  mBootPickerSelectorButton.Hdr.Obj.OffsetX = (SelectorBackgroundImage->Width - SelectorButtonImage->Width) / 2;
  mBootPickerSelectorButton.Hdr.Obj.OffsetY = SelectorBackgroundImage->Height + BOOT_SELECTOR_BUTTON_SPACE * DrawContext->Scale;
  mBootPickerSelectorButton.Hdr.Obj.Width   = SelectorButtonImage->Width;
  mBootPickerSelectorButton.Hdr.Obj.Height  = SelectorButtonImage->Height;
  mBootPickerSelectorButton.ImageId         = ICON_SELECTOR;
  mBootPickerSelectorButton.ImageState      = ICON_TYPE_BASE;

  mBootPickerSelectorContainer.Obj.OffsetX = 0;
  mBootPickerSelectorContainer.Obj.OffsetY = 0;
  mBootPickerSelectorContainer.Obj.Width   = SelectorBackgroundImage->Width;
  mBootPickerSelectorContainer.Obj.Height  = (UINT32) (mBootPickerSelectorButton.Hdr.Obj.OffsetY + mBootPickerSelectorButton.Hdr.Obj.Height);

  mBootPickerLeftScroll.Hdr.Obj.Height = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerLeftScroll.Hdr.Obj.Width  = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerLeftScroll.Hdr.Obj.OffsetX = BOOT_SCROLL_BUTTON_SPACE;
  mBootPickerLeftScroll.Hdr.Obj.OffsetY = (DrawContext->Screen.Height - mBootPickerLeftScroll.Hdr.Obj.Height) / 2;
  mBootPickerLeftScroll.ImageId         = ICON_LEFT;
  mBootPickerLeftScroll.ImageState      = ICON_TYPE_BASE;

  mBootPickerRightScroll.Hdr.Obj.Height = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerRightScroll.Hdr.Obj.Width  = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerRightScroll.Hdr.Obj.OffsetX = DrawContext->Screen.Width - mBootPickerRightScroll.Hdr.Obj.Width - BOOT_SCROLL_BUTTON_SPACE;
  mBootPickerRightScroll.Hdr.Obj.OffsetY = (DrawContext->Screen.Height - mBootPickerRightScroll.Hdr.Obj.Height) / 2;
  mBootPickerRightScroll.ImageId         = ICON_RIGHT;
  mBootPickerRightScroll.ImageState      = ICON_TYPE_BASE;
  //
  // The boot entry container must precisely show a set of boot entries, i.e.
  // there may not be partial entries or extra padding.
  //
  ContainerMaxWidth   = DrawContext->Screen.Width - mBootPickerLeftScroll.Hdr.Obj.Width - 2 * BOOT_SCROLL_BUTTON_SPACE - mBootPickerRightScroll.Hdr.Obj.Width - 2 * BOOT_SCROLL_BUTTON_SPACE;
  ContainerWidthDelta = (ContainerMaxWidth + BOOT_ENTRY_SPACE * GuiContext->Scale) % ((BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * GuiContext->Scale);

  mBootPickerContainer.Obj.Height  = BOOT_SELECTOR_HEIGHT * GuiContext->Scale;
  mBootPickerContainer.Obj.Width   = ContainerMaxWidth - ContainerWidthDelta;
  mBootPickerContainer.Obj.OffsetX = (DrawContext->Screen.Width - mBootPickerContainer.Obj.Width) / 2;
  //
  // Center the icons and labels excluding the selector images vertically.
  //
  ASSERT ((DrawContext->Screen.Height - (BOOT_ENTRY_HEIGHT - BOOT_ENTRY_ICON_SPACE) * GuiContext->Scale) / 2 - (BOOT_ENTRY_ICON_SPACE * GuiContext->Scale) == (DrawContext->Screen.Height - (BOOT_ENTRY_HEIGHT + BOOT_ENTRY_ICON_SPACE) * GuiContext->Scale) / 2);
  mBootPickerContainer.Obj.OffsetY = (DrawContext->Screen.Height - (BOOT_ENTRY_HEIGHT + BOOT_ENTRY_ICON_SPACE) * GuiContext->Scale) / 2;

  mBootPicker.Hdr.Obj.Height  = BOOT_SELECTOR_HEIGHT * GuiContext->Scale;
  //
  // Adding an entry will wrap around Width such that the first entry has no
  // padding.
  //
  mBootPicker.Hdr.Obj.Width   = 0U - (UINT32) (BOOT_ENTRY_SPACE * GuiContext->Scale);
  //
  // Adding an entry will also shift OffsetX considering the added boot entry
  // space. This is not needed for the first, so initialise accordingly.
  //
  mBootPicker.Hdr.Obj.OffsetX = mBootPickerContainer.Obj.Width / 2 + (UINT32) (BOOT_ENTRY_SPACE * GuiContext->Scale) / 2;
  mBootPicker.Hdr.Obj.OffsetY = 0;

  mBootPicker.SelectedIndex = 0;

  mBootPicker.Hdr.Obj.Children = AllocateZeroPool (NumBootEntries * sizeof (*mBootPicker.Hdr.Obj.Children));
  if (mBootPicker.Hdr.Obj.Children == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  mBootPicker.Hdr.Obj.NumChildren = NumBootEntries;

  if (GuiContext->PickerContext->TitleSuffix == NULL) {
    mVersionLabelImage.Buffer = NULL;

    mBootPickerVersionLabel.Obj.Width   = 0;
    mBootPickerVersionLabel.Obj.Height  = 0;
    mBootPickerVersionLabel.Obj.OffsetX = 0;
    mBootPickerVersionLabel.Obj.OffsetY = 0;
  } else {
    DestLen = AsciiStrLen (GuiContext->PickerContext->TitleSuffix);
    UString = AsciiStrCopyToUnicode (GuiContext->PickerContext->TitleSuffix, DestLen);

    Result = GuiGetLabel (
      &mVersionLabelImage,
      &GuiContext->FontContext,
      UString,
      DestLen,
      GuiContext->LightBackground
      );

    FreePool (UString);
    
    if (!Result) {
      DEBUG ((DEBUG_WARN, "OCUI: version label failed\n"));
      return Result;
    }

    mBootPickerVersionLabel.Obj.Width   = mVersionLabelImage.Width;
    mBootPickerVersionLabel.Obj.Height  = mVersionLabelImage.Height;
    mBootPickerVersionLabel.Obj.OffsetX = DrawContext->Screen.Width  - ((3 * mBootPickerVersionLabel.Obj.Width ) / 2);
    mBootPickerVersionLabel.Obj.OffsetY = DrawContext->Screen.Height - ((5 * mBootPickerVersionLabel.Obj.Height) / 2);
  }

  // TODO: animations should be tied to UI objects, not global
  // Each object has its own list of animations.
  // How to animate addition of one or more boot entries?
  // 1. MOVE:
  //    - Calculate the delta by which each entry moves to the left or to the right.
  //      ∆i = (N(added after) - N(added before))
  // Conditions for delta function:
  //

  if (!GuiContext->DoneIntroAnimation) {
    InitBpAnimIntro (DrawContext);
    InsertHeadList (&DrawContext->Animations, &mBootPickerIntroAnimation.Link);
    //
    // Fade-in picker, scroll buttons, and action buttons.
    //
    mBootPickerContainer.Obj.Opacity       = 0;
    mBootPickerLeftScroll.Hdr.Obj.Opacity  = 0;
    mBootPickerRightScroll.Hdr.Obj.Opacity = 0;

    GuiContext->DoneIntroAnimation = TRUE;
  } else {
    //
    // The late code assumes the scroll buttons are visible by default.
    //
    mBootPickerLeftScroll.Hdr.Obj.Opacity  = 0xFF;
    mBootPickerRightScroll.Hdr.Obj.Opacity = 0xFF;
    //
    // Unhide action buttons immediately if they are not animated.
    //
    mCommonActionButtonsContainer.Obj.Opacity = 0xFF;
  }

  if (DrawContext->TimeOutSeconds > 0) {
    STATIC GUI_ANIMATION PickerAnim2;
    PickerAnim2.Context = NULL;
    PickerAnim2.Animate = InternalBootPickerAnimateTimeout;
    InsertHeadList (&DrawContext->Animations, &PickerAnim2.Link);
  }

  InternalInitialiseLabelShadows (DrawContext, GuiContext);

  /*
  InitBpAnimImageList(GuiInterpolTypeLinear, 25, 25);
  STATIC GUI_ANIMATION PoofAnim;
  PoofAnim.Context = GuiContext->Poof;
  PoofAnim.Animate = InternalBootPickerAnimateImageList;
  InsertHeadList(&DrawContext->Animations, &PoofAnim.Link);
  */

  return EFI_SUCCESS;
}

VOID
BootPickerViewLateInitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN     UINT8                    DefaultIndex
  )
{
  UINT32                 Index;
  INT64                  ScrollOffset;
  CONST GUI_VOLUME_ENTRY *BootEntry;

  ASSERT (DefaultIndex < mBootPicker.Hdr.Obj.NumChildren);

  ScrollOffset = InternelBootPickerScrollSelected ();
  //
  // If ScrollOffset is non-0, the selected entry will be aligned left- or
  // right-most. The view holds a discrete amount of entries, so cut-offs are
  // impossible.
  //
  if (ScrollOffset == 0) {
    //
    // Find the first entry that is fully visible.
    //
    for (Index = 0; Index < mBootPicker.Hdr.Obj.NumChildren; ++Index) {
      //
      // Move the first partially visible boot entry to the very left to prevent
      // cut-off entries. This only applies when entries overflow.
      //
      BootEntry = InternalGetVolumeEntry (Index);
      if (mBootPicker.Hdr.Obj.OffsetX + BootEntry->Hdr.Obj.OffsetX >= 0) {
        //
        // Move the cut-off entry on-screen.
        //
        ScrollOffset = -ScrollOffset;
        break;
      }

      ScrollOffset = mBootPicker.Hdr.Obj.OffsetX + BootEntry->Hdr.Obj.OffsetX;
    }
  }

  mBootPicker.Hdr.Obj.OffsetX += ScrollOffset;
  //
  // If the scroll buttons are hidden, the intro animation will update them
  // implicitly.
  //
  if (mBootPickerLeftScroll.Hdr.Obj.Opacity == 0xFF) {
    InternalUpdateScrollButtons ();
  }
  InternalBootPickerSelectEntry (&mBootPicker, NULL, DefaultIndex);
  BootEntry = InternalGetVolumeEntry (DefaultIndex);
  InternalStartAnimateLabel (DrawContext, BootEntry);
  GuiContext->BootEntry = BootEntry->Context;
}

VOID
BootPickerViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN OUT BOOT_PICKER_GUI_CONTEXT  *GuiContext
  )
{
  UINT32 Index;

  if (mVersionLabelImage.Buffer != NULL) {
    FreePool (mVersionLabelImage.Buffer);
    mVersionLabelImage.Buffer = NULL;
  }

  for (Index = 0; Index < mBootPicker.Hdr.Obj.NumChildren; ++Index) {
    InternalBootPickerEntryDestruct (InternalGetVolumeEntry (Index));
  }

  if (mBootPicker.Hdr.Obj.Children != NULL) {
    FreePool (mBootPicker.Hdr.Obj.Children);
  }
  mBootPicker.Hdr.Obj.NumChildren = 0;

  GuiViewDeinitialize (DrawContext, GuiContext);
}

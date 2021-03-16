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
#include "BootPicker.h"

#include "Common.h"

extern GUI_VOLUME_PICKER mBootPicker;
extern GUI_OBJ_CHILD     mBootPickerContainer;
extern GUI_OBJ_CLICKABLE mBootPickerSelector;
extern GUI_OBJ_CLICKABLE mBootPickerRightScroll;
extern GUI_OBJ_CLICKABLE mBootPickerLeftScroll;
extern CONST GUI_IMAGE   mBackgroundImage;

// STATIC UINT8 mBootPickerImageIndex = 0;

extern INT64 mBackgroundImageOffsetX;
extern INT64 mBackgroundImageOffsetY;

STATIC UINT8 mBootPickerOpacity = 0;

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

VOID
InternalBootPickerSelectEntry (
  IN OUT GUI_VOLUME_PICKER   *This,
  IN OUT GUI_DRAWING_CONTEXT *DrawContext,
  IN     UINT32              NewIndex
  )
{
  CONST GUI_VOLUME_ENTRY *NewEntry;

  ASSERT (This != NULL);
  ASSERT (NewIndex < mBootPicker.Hdr.Obj.NumChildren);

  This->SelectedIndex = NewIndex;
  NewEntry = (CONST GUI_VOLUME_ENTRY *) mBootPicker.Hdr.Obj.Children[NewIndex];

  ASSERT (mBootPickerSelector.Hdr.Obj.Width <= NewEntry->Hdr.Obj.Width);
  ASSERT_EQUALS (This->Hdr.Obj.Height, mBootPickerSelector.Hdr.Obj.OffsetY + mBootPickerSelector.Hdr.Obj.Height);

  mBootPickerSelector.Hdr.Obj.OffsetX  = mBootPicker.Hdr.Obj.OffsetX + NewEntry->Hdr.Obj.OffsetX;
  mBootPickerSelector.Hdr.Obj.OffsetX += (NewEntry->Hdr.Obj.Width - mBootPickerSelector.Hdr.Obj.Width) / 2;

  if (DrawContext != NULL) {
    //
    // Set voice timeout to N frames from now.
    //
    DrawContext->GuiContext->AudioPlaybackTimeout = OC_VOICE_OVER_IDLE_TIMEOUT_MS;
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

  if (mBootPicker.Hdr.Obj.NumChildren == 1) {
    return 0;
  }
  //
  // If the selected entry is outside of the view, scroll it accordingly.
  //
  SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
  EntryOffsetX  = mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX;

  if (EntryOffsetX < 0) {
    return -EntryOffsetX;
  }
  
  if (EntryOffsetX + SelectedEntry->Hdr.Obj.Width > mBootPickerContainer.Obj.Width) {
    return -((EntryOffsetX + SelectedEntry->Hdr.Obj.Width) - mBootPickerContainer.Obj.Width);
  }

  return 0;
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
  CONST GUI_VOLUME_ENTRY *NewEntry;
  CONST GUI_VOLUME_ENTRY *PrevEntry;
  INT64                  ScrollOffset;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (NewIndex < This->Hdr.Obj.NumChildren);
  //
  // The caller must guarantee the entry is actually new for performance
  // reasons.
  //
  ASSERT (This->SelectedIndex != NewIndex);
  //
  // Redraw the two now (un-)selected entries.
  //
  NewEntry  = InternalGetVolumeEntry (NewIndex);
  PrevEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
  InternalBootPickerSelectEntry (This, DrawContext, NewIndex);

  ScrollOffset = InternelBootPickerScrollSelected ();
  if (ScrollOffset == 0) {
    //
    // To redraw the entry *and* the selector, draw the entire height of the
    // Picker object. For this, the height just reach from the top of the entries
    // to the bottom of the selector.
    //
    GuiRequestDrawCrop (
      DrawContext,
      BaseX + NewEntry->Hdr.Obj.OffsetX,
      BaseY + NewEntry->Hdr.Obj.OffsetY,
      NewEntry->Hdr.Obj.Width,
      This->Hdr.Obj.Height
      );

    GuiRequestDrawCrop (
      DrawContext,
      BaseX + PrevEntry->Hdr.Obj.OffsetX,
      BaseY + PrevEntry->Hdr.Obj.OffsetY,
      PrevEntry->Hdr.Obj.Width,
      This->Hdr.Obj.Height
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
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     CONST GUI_KEY_EVENT     *KeyEvent
  )
{
  GUI_VOLUME_PICKER       *Picker;
  CONST GUI_VOLUME_ENTRY  *SelectedEntry;

  ASSERT (This != NULL);
  ASSERT (GuiContext != NULL);
  ASSERT (DrawContext != NULL);

  Picker = BASE_CR (This, GUI_VOLUME_PICKER, Hdr.Obj);

  if (KeyEvent->Key.ScanCode == SCAN_RIGHT) {
    //
    // Edge-case: The last child is the selector button.
    //
    if (mBootPicker.SelectedIndex + 1 < mBootPicker.Hdr.Obj.NumChildren) {
      //
      // Redraw the two now (un-)selected entries.
      //
      InternalBootPickerChangeEntry (
        Picker,
        DrawContext,
        BaseX,
        BaseY,
        mBootPicker.SelectedIndex + 1
        );
    }
  } else if (KeyEvent->Key.ScanCode == SCAN_LEFT) {
    if (mBootPicker.SelectedIndex > 0) {
      //
      // Redraw the two now (un-)selected entries.
      //
      InternalBootPickerChangeEntry (
        Picker,
        DrawContext,
        BaseX,
        BaseY,
        mBootPicker.SelectedIndex - 1
        );
    }
  } else if (KeyEvent->Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    if (mBootPicker.Hdr.Obj.NumChildren > 0) {
      SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
      SelectedEntry->Context->SetDefault = (KeyEvent->Modifiers & USB_HID_KB_KP_MODIFIERS_CONTROL) != 0;
      GuiContext->ReadyToBoot = TRUE;
      ASSERT (GuiContext->BootEntry == SelectedEntry->Context);
    }
  } else if (mBootPickerOpacity != 0xFF) {
    //
    // FIXME: Other keys are not allowed when boot picker is partially transparent.
    //
    return;
  }

  if (KeyEvent->Key.UnicodeChar == L' ') {
    GuiContext->HideAuxiliary = FALSE;
    GuiContext->Refresh = TRUE;
    DrawContext->GuiContext->PickerContext->PlayAudioFile (
      DrawContext->GuiContext->PickerContext,
      OcVoiceOverAudioFileShowAuxiliary,
      FALSE
      );
  } else if (KeyEvent->Key.ScanCode == SCAN_ESC || KeyEvent->Key.UnicodeChar == '0') {
    GuiContext->Refresh = TRUE;
    DrawContext->GuiContext->PickerContext->PlayAudioFile (
      DrawContext->GuiContext->PickerContext,
      OcVoiceOverAudioFileReloading,
      FALSE
      );
  } else if (KeyEvent->Key.ScanCode == SCAN_F5 && (KeyEvent->Modifiers & USB_HID_KB_KP_MODIFIERS_GUI) != 0) {
    DrawContext->GuiContext->PickerContext->ToggleVoiceOver (
      DrawContext->GuiContext->PickerContext,
      0
      );
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
  IN     UINT32                  Height
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

  ASSERT_EQUALS (This->Width,  BOOT_ENTRY_DIMENSION * DrawContext->Scale);
  ASSERT_EQUALS (This->Height, BOOT_ENTRY_HEIGHT    * DrawContext->Scale);
  //
  // Draw the icon horizontally centered.
  //
  ASSERT (EntryIcon != NULL);
  ASSERT_EQUALS (EntryIcon->Width,  BOOT_ENTRY_ICON_DIMENSION * DrawContext->Scale);
  ASSERT_EQUALS (EntryIcon->Height, BOOT_ENTRY_ICON_DIMENSION * DrawContext->Scale);

  GuiDrawChildImage (
    EntryIcon,
    mBootPickerOpacity,
    DrawContext,
    BaseX,
    BaseY,
    BOOT_ENTRY_ICON_SPACE * DrawContext->Scale,
    BOOT_ENTRY_ICON_SPACE * DrawContext->Scale,
    OffsetX,
    OffsetY,
    Width,
    Height
    );
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
    mBootPickerOpacity,
    DrawContext,
    BaseX,
    BaseY,
    (BOOT_ENTRY_DIMENSION * DrawContext->Scale - Label->Width) / 2,
    (BOOT_ENTRY_DIMENSION + BOOT_ENTRY_LABEL_SPACE + BOOT_ENTRY_LABEL_HEIGHT) * DrawContext->Scale - Label->Height,
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

  if (OffsetX < BOOT_ENTRY_ICON_SPACE * DrawContext->Scale
   || OffsetY < BOOT_ENTRY_ICON_SPACE * DrawContext->Scale) {
    return This;
  }

  Entry = BASE_CR (This, GUI_VOLUME_ENTRY, Hdr.Obj);

  IsHit = GuiClickableIsHit (
            &Entry->EntryIcon,
            OffsetX - BOOT_ENTRY_ICON_SPACE * DrawContext->Scale,
            OffsetY - BOOT_ENTRY_ICON_SPACE * DrawContext->Scale
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
    ASSERT (Context->BootEntry == Entry->Context);
  }
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);
  return This;
}

VOID
InternalBootPickerSelectorDraw (
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
  CONST GUI_IMAGE               *BackgroundImage;
  CONST GUI_IMAGE               *ButtonImage;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  Clickable  = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);

  ASSERT_EQUALS (This->Width,  BOOT_SELECTOR_WIDTH  * DrawContext->Scale);
  ASSERT_EQUALS (This->Height, BOOT_SELECTOR_HEIGHT * DrawContext->Scale);

  BackgroundImage = &Context->Icons[ICON_SELECTED][ICON_TYPE_BASE];

  ASSERT_EQUALS (BackgroundImage->Width,  BOOT_SELECTOR_BACKGROUND_DIMENSION * DrawContext->Scale);
  ASSERT_EQUALS (BackgroundImage->Height, BOOT_SELECTOR_BACKGROUND_DIMENSION * DrawContext->Scale);
  ASSERT (BackgroundImage->Buffer != NULL);
  //
  // Background starts at (0,0) and is as wide as This.
  //
  if (OffsetY < BOOT_SELECTOR_BACKGROUND_DIMENSION * DrawContext->Scale) {
    GuiDrawToBuffer (
      BackgroundImage,
      mBootPickerOpacity,
      DrawContext,
      BaseX,
      BaseY,
      OffsetX,
      OffsetY,
      Width,
      Height
      );
  }

  ButtonImage = Clickable->CurrentImage;
  ASSERT (ButtonImage != NULL);

  STATIC_ASSERT (
    BOOT_SELECTOR_BUTTON_WIDTH <= BOOT_SELECTOR_BACKGROUND_DIMENSION,
    "The selector width must not exceed the selector background width."
    );
  ASSERT (ButtonImage->Width <= BOOT_SELECTOR_BUTTON_WIDTH * DrawContext->Scale);
  ASSERT (ButtonImage->Height <= BOOT_SELECTOR_BUTTON_HEIGHT * DrawContext->Scale);
  ASSERT (ButtonImage->Buffer != NULL);

  GuiDrawChildImage (
    ButtonImage,
    mBootPickerOpacity,
    DrawContext,
    BaseX,
    BaseY,
    (BOOT_SELECTOR_BACKGROUND_DIMENSION * DrawContext->Scale - ButtonImage->Width) / 2,
    (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE) * DrawContext->Scale,
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
InternalBootPickerLeftScrollDraw (
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

  if (mBootPicker.Hdr.Obj.OffsetX >= 0) {
    return ;
  }

  Clickable  = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);

  ButtonImage = Clickable->CurrentImage;
  ASSERT (ButtonImage != NULL);

  ASSERT_EQUALS (ButtonImage->Width, This->Width);
  ASSERT_EQUALS (ButtonImage->Height, This->Height);
  ASSERT (ButtonImage->Buffer != NULL);

  GuiDrawToBuffer (
    ButtonImage,
    mBootPickerOpacity,
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

VOID
InternalBootPickerRightScrollDraw (
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

  if (mBootPicker.Hdr.Obj.OffsetX + mBootPicker.Hdr.Obj.Width <= mBootPickerContainer.Obj.Width) {
    return;
  }

  Clickable  = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);

  ButtonImage = Clickable->CurrentImage;
  ASSERT (ButtonImage != NULL);

  ASSERT_EQUALS (ButtonImage->Width , This->Width);
  ASSERT_EQUALS (ButtonImage->Height, This->Height);
  ASSERT (ButtonImage->Buffer != NULL);

  GuiDrawToBuffer (
    ButtonImage,
    mBootPickerOpacity,
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
  GUI_OBJ_CLICKABLE             *Clickable;
  CONST GUI_IMAGE               *ButtonImage;

  BOOLEAN                       IsHit;
  UINT32                        OffsetX;
  UINT32                        OffsetY;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);

  OffsetX = (UINT32) (Event->Pos.Pos.X - BaseX);
  OffsetY = (UINT32) (Event->Pos.Pos.Y - BaseY);

  Clickable     = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage   = &Context->Icons[ICON_SELECTOR][ICON_TYPE_BASE];

  ASSERT (Event->Type == GuiPointerPrimaryDown
       || Event->Type == GuiPointerPrimaryUp
       || Event->Type == GuiPointerPrimaryDoubleClick);
  if (OffsetX >= (BOOT_SELECTOR_BACKGROUND_DIMENSION * DrawContext->Scale - ButtonImage->Width) / 2
   && OffsetY >= (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE) * DrawContext->Scale) {
    IsHit = GuiClickableIsHit (
              ButtonImage,
              OffsetX - (BOOT_SELECTOR_BACKGROUND_DIMENSION * DrawContext->Scale - ButtonImage->Width) / 2,
              OffsetY - (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE) * DrawContext->Scale
              );
    if (IsHit) {
      if (Event->Type == GuiPointerPrimaryUp) {
        ASSERT (Context->BootEntry == InternalGetVolumeEntry (mBootPicker.SelectedIndex)->Context);
        Context->ReadyToBoot = TRUE;
      } else  {
        ButtonImage = &Context->Icons[ICON_SELECTOR][ICON_TYPE_HELD];
      }
    }
  }

  if (Clickable->CurrentImage != ButtonImage) {
    Clickable->CurrentImage = ButtonImage;
    //
    // The view is constructed such that the selector is always fully visible.
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
InternalBootPickerLeftScrollPtrEvent (
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
  INT64             BootPickerX;
  INT64             BootPickerY;
  BOOLEAN           IsHit;
  CONST GUI_VOLUME_ENTRY *SelectedEntry;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);

  Clickable   = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage = &Context->Icons[ICON_LEFT][ICON_TYPE_BASE];

  ASSERT (Event->Type == GuiPointerPrimaryDown
       || Event->Type == GuiPointerPrimaryUp
       || Event->Type == GuiPointerPrimaryDoubleClick);
  ASSERT (ButtonImage->Width == This->Width);
  ASSERT (ButtonImage->Height == This->Height);

  IsHit = GuiClickableIsHit (
    ButtonImage,
    Event->Pos.Pos.X - BaseX,
    Event->Pos.Pos.Y - BaseY
    );
  if (IsHit) {
    if (Event->Type == GuiPointerPrimaryDown) {
      ButtonImage = &Context->Icons[ICON_LEFT][ICON_TYPE_HELD];
    } else if (mBootPicker.Hdr.Obj.OffsetX < 0) {
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
      // If the selected entry is pushed off-screen by scrolling, select the
      // appropriate neighbour entry.
      //
      SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
      if (mBootPicker.Hdr.Obj.OffsetX + (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * Context->Scale + SelectedEntry->Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.Width > mBootPickerContainer.Obj.Width) {
        //
        // The internal design ensures a selected entry cannot be off-screen,
        // scrolling offsets it by at most one spot.
        //
        if (mBootPicker.SelectedIndex > 0) {
          InternalBootPickerSelectEntry (
            &mBootPicker,
            DrawContext,
            mBootPicker.SelectedIndex - 1
            );

          SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
          ASSERT (!(mBootPicker.Hdr.Obj.OffsetX + (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * Context->Scale + SelectedEntry->Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.Width > mBootPickerContainer.Obj.Width));
        }
      }
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
    }
  }

  if (Clickable->CurrentImage != ButtonImage) {
    Clickable->CurrentImage = ButtonImage;
    //
    // The view is constructed such that the scroll buttons are always fully
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
InternalBootPickerRightScrollPtrEvent (
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
  INT64             BootPickerX;
  INT64             BootPickerY;
  BOOLEAN           IsHit;
  GUI_VOLUME_ENTRY  *SelectedEntry;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);
  //
  // There should be no children.
  //
  ASSERT (This->NumChildren == 0);

  Clickable   = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage = &Context->Icons[ICON_RIGHT][ICON_TYPE_BASE];

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
      ButtonImage = &Context->Icons[ICON_RIGHT][ICON_TYPE_HELD];
    } else if (mBootPicker.Hdr.Obj.OffsetX + mBootPicker.Hdr.Obj.Width > mBootPickerContainer.Obj.Width) {
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
      // If the selected entry is pushed off-screen by scrolling, select the
      // appropriate neighbour entry.
      //
      SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
      if (mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX < (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * Context->Scale) {
        //
        // The internal design ensures a selected entry cannot be off-screen,
        // scrolling offsets it by at most one spot.
        //
        if (mBootPicker.SelectedIndex + 1 < mBootPicker.Hdr.Obj.NumChildren) {
          InternalBootPickerSelectEntry (
            &mBootPicker,
            DrawContext,
            mBootPicker.SelectedIndex + 1
            );
        }

        SelectedEntry = InternalGetVolumeEntry (mBootPicker.SelectedIndex);
        ASSERT (!(mBootPicker.Hdr.Obj.OffsetX + SelectedEntry->Hdr.Obj.OffsetX < (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * Context->Scale));
      }
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
    }
  }

  if (Clickable->CurrentImage != ButtonImage) {
    Clickable->CurrentImage = ButtonImage;
    //
    // The view is constructed such that the scroll buttons are always fully
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
InternalBootPickerViewKeyEvent (
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
  //
  // Consider moving between multiple panes with UP/DOWN and store the current
  // view within the object - for now, hardcoding this is enough.
  //
  InternalBootPickerKeyEvent (
    &mBootPicker.Hdr.Obj,
    DrawContext,
    Context,
    mBootPickerContainer.Obj.OffsetX + mBootPicker.Hdr.Obj.OffsetX,
    mBootPickerContainer.Obj.OffsetY + mBootPicker.Hdr.Obj.OffsetY,
    KeyEvent
    );
}

BOOLEAN
InternalBootPickerExitLoop (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->ReadyToBoot || Context->Refresh;
}

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mBootPickerSelector = {
  {
    {
      0, 0, 0, 0,
      InternalBootPickerSelectorDraw,
      InternalBootPickerSelectorPtrEvent,
      0,
      NULL
    },
    &mBootPickerContainer.Obj
  },
  NULL
};

STATIC GUI_OBJ_CHILD *mBootPickerContainerChilds[] = {
  &mBootPickerSelector.Hdr,
  &mBootPicker.Hdr
  };

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CHILD mBootPickerContainer = {
  {
    0, 0, 0, 0,
    GuiObjDrawDelegate,
    GuiObjDelegatePtrEvent,
    ARRAY_SIZE (mBootPickerContainerChilds),
    mBootPickerContainerChilds
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_VOLUME_PICKER mBootPicker = {
  {
    {
      0, 0, 0, 0,
      GuiObjDrawDelegate,
      GuiObjDelegatePtrEvent,
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
      0, 0, 0, 0,
      InternalBootPickerLeftScrollDraw,
      InternalBootPickerLeftScrollPtrEvent,
      0,
      NULL
    },
    NULL
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mBootPickerRightScroll = {
  {
    {
      0, 0, 0, 0,
      InternalBootPickerRightScrollDraw,
      InternalBootPickerRightScrollPtrEvent,
      0,
      NULL
    },
    NULL
  },
  NULL
};

STATIC GUI_OBJ_CHILD *mBootPickerViewChilds[] = {
  &mBootPickerContainer,
  &mCommonActionButtonsContainer,
  &mBootPickerLeftScroll.Hdr,
  &mBootPickerRightScroll.Hdr
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_VIEW_CONTEXT mBootPickerViewContext = {
  InternalCommonViewDraw,
  GuiObjDelegatePtrEvent,
  ARRAY_SIZE (mBootPickerViewChilds),
  mBootPickerViewChilds,
  InternalBootPickerViewKeyEvent,
  InternalGetCursorImage,
  InternalBootPickerExitLoop
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
  BOOLEAN                     UseDiskLabel;
  BOOLEAN                     UseGenericLabel;
  BOOLEAN                     Result;

  ASSERT (GuiContext != NULL);
  ASSERT (Entry != NULL);
  ASSERT (EntryIndex < mBootPicker.Hdr.Obj.NumChildren);

  DEBUG ((DEBUG_INFO, "OCUI: Console attributes: %d\n", Context->ConsoleAttributes));

  UseVolumeIcon   = (Context->PickerAttributes & OC_ATTR_USE_VOLUME_ICON) != 0;
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
      case OC_BOOT_EXTERNAL_TOOL:
        if (StrStr (Entry->Name, OC_MENU_RESET_NVRAM_ENTRY) != NULL) {
          Status = CopyLabel (&VolumeEntry->Label, &GuiContext->Labels[LABEL_RESET_NVRAM]);
        } else if (StrStr (Entry->Name, OC_MENU_UEFI_SHELL_ENTRY) != NULL) {
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
    Result = GuiGetLabel (
      &VolumeEntry->Label,
      &GuiContext->FontContext,
      Entry->Name,
      StrLen (Entry->Name),
      GuiContext->LightBackground
      );
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

  if (EFI_ERROR (Status)) {
    SuggestedIcon = NULL;
    IconTypeIndex = Entry->IsExternal ? ICON_TYPE_EXTERNAL : ICON_TYPE_BASE;
    switch (Entry->Type) {
      case OC_BOOT_APPLE_OS:
        SuggestedIcon = &GuiContext->Icons[ICON_APPLE][IconTypeIndex];
        break;
      case OC_BOOT_APPLE_FW_UPDATE:
      case OC_BOOT_APPLE_RECOVERY:
        SuggestedIcon = &GuiContext->Icons[ICON_APPLE_RECOVERY][IconTypeIndex];
        if (SuggestedIcon->Buffer == NULL) {
          SuggestedIcon = &GuiContext->Icons[ICON_APPLE][IconTypeIndex];
        }
        break;
      case OC_BOOT_APPLE_TIME_MACHINE:
        SuggestedIcon = &GuiContext->Icons[ICON_APPLE_TIME_MACHINE][IconTypeIndex];
        if (SuggestedIcon->Buffer == NULL) {
          SuggestedIcon = &GuiContext->Icons[ICON_APPLE][IconTypeIndex];
        }
        break;
      case OC_BOOT_WINDOWS:
        SuggestedIcon = &GuiContext->Icons[ICON_WINDOWS][IconTypeIndex];
        break;
      case OC_BOOT_EXTERNAL_OS:
        SuggestedIcon = &GuiContext->Icons[ICON_OTHER][IconTypeIndex];
        break;
      case OC_BOOT_RESET_NVRAM:
        SuggestedIcon = &GuiContext->Icons[ICON_RESET_NVRAM][IconTypeIndex];
        if (SuggestedIcon->Buffer == NULL) {
          SuggestedIcon = &GuiContext->Icons[ICON_TOOL][IconTypeIndex];
        }
        break;
      case OC_BOOT_EXTERNAL_TOOL:
        if (StrStr (Entry->Name, OC_MENU_RESET_NVRAM_ENTRY) != NULL) {
          SuggestedIcon = &GuiContext->Icons[ICON_RESET_NVRAM][IconTypeIndex];
        } else if (StrStr (Entry->Name, OC_MENU_UEFI_SHELL_ENTRY) != NULL) {
          SuggestedIcon = &GuiContext->Icons[ICON_SHELL][IconTypeIndex];
        }

        if (SuggestedIcon == NULL || SuggestedIcon->Buffer == NULL) {
          SuggestedIcon = &GuiContext->Icons[ICON_TOOL][IconTypeIndex];
        }
        break;
      case OC_BOOT_UNKNOWN:
        SuggestedIcon = &GuiContext->Icons[ICON_GENERIC_HDD][IconTypeIndex];
        break;
      default:
        DEBUG ((DEBUG_WARN, "OCUI: Entry kind %d unsupported for icon\n", Entry->Type));
        return EFI_UNSUPPORTED;
    }

    ASSERT (SuggestedIcon != NULL);

    if (SuggestedIcon->Buffer == NULL) {
      SuggestedIcon = &GuiContext->Icons[ICON_GENERIC_HDD][IconTypeIndex];
    }

    CopyMem (&VolumeEntry->EntryIcon, SuggestedIcon, sizeof (VolumeEntry->EntryIcon));
  }

  VolumeEntry->Hdr.Parent       = &mBootPicker.Hdr.Obj;
  VolumeEntry->Hdr.Obj.Width    = BOOT_ENTRY_WIDTH  * GuiContext->Scale;
  VolumeEntry->Hdr.Obj.Height   = BOOT_ENTRY_HEIGHT * GuiContext->Scale;
  VolumeEntry->Hdr.Obj.Draw     = InternalBootPickerEntryDraw;
  VolumeEntry->Hdr.Obj.PtrEvent = InternalBootPickerEntryPtrEvent;
  VolumeEntry->Hdr.Obj.NumChildren = 0;
  VolumeEntry->Hdr.Obj.Children    = NULL;

  if (EntryIndex > 0) {
    PrevEntry = InternalGetVolumeEntry (EntryIndex - 1);
    VolumeEntry->Hdr.Obj.OffsetX = PrevEntry->Hdr.Obj.OffsetX + (BOOT_ENTRY_DIMENSION + BOOT_ENTRY_SPACE) * GuiContext->Scale;
  }

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

  mBootPickerOpacity = 0;
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

STATIC GUI_INTERPOLATION mBpAnimInfoSinMove;

VOID
InitBpAnimIntro (
  VOID
  )
{
  InternalCommonIntroOpacoityInterpolInit ();

  mBpAnimInfoSinMove.Type       = GuiInterpolTypeSmooth;
  mBpAnimInfoSinMove.StartTime  = 0;
  mBpAnimInfoSinMove.Duration   = 25;
  mBpAnimInfoSinMove.StartValue = 0;
  mBpAnimInfoSinMove.EndValue   = 35;
  //
  // FIXME: This assumes that only relative changes of X are performed on
  //        mBootPickerContainer between animation initialisation and start.
  //
  mBootPickerContainer.Obj.OffsetX += 35;
}

BOOLEAN
InternalBootPickerAnimateIntro (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  STATIC UINT32 PrevSine = 0;

  UINT32 InterpolVal;
  UINT32 DeltaSine;

  ASSERT (DrawContext != NULL);

  mBootPickerOpacity = (UINT8)GuiGetInterpolatedValue (&mCommonIntroOpacityInterpol, CurrentTime);
  if (mBootPickerOpacity > mCommonActionButtonsOpacity) {
    mCommonActionButtonsOpacity = mBootPickerOpacity;
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
  
  ASSERT (mBpAnimInfoSinMove.Duration == mCommonIntroOpacityInterpol.Duration);
  return CurrentTime - mBpAnimInfoSinMove.StartTime >= mBpAnimInfoSinMove.Duration;
}

EFI_STATUS
BootPickerViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  GUI_CURSOR_GET_IMAGE     GetCursorImage,
  IN  UINT8                    NumBootEntries
  )
{
  UINT32 ContainerMaxWidth;
  UINT32 ContainerWidthDelta;

  ASSERT (DrawContext != NULL);
  ASSERT (GuiContext != NULL);
  ASSERT (GetCursorImage != NULL);

  CommonViewInitialize (
    DrawContext,
    GuiContext,
    &mBootPickerViewContext
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

  mBootPickerSelector.CurrentImage = &GuiContext->Icons[ICON_SELECTOR][ICON_TYPE_BASE];
  mBootPickerSelector.Hdr.Obj.OffsetX = 0;
  mBootPickerSelector.Hdr.Obj.OffsetY = 0;
  mBootPickerSelector.Hdr.Obj.Height = BOOT_SELECTOR_HEIGHT * GuiContext->Scale;
  mBootPickerSelector.Hdr.Obj.Width  = BOOT_SELECTOR_WIDTH  * GuiContext->Scale;

  mBootPickerLeftScroll.CurrentImage = &GuiContext->Icons[ICON_LEFT][ICON_TYPE_BASE];
  mBootPickerLeftScroll.Hdr.Obj.Height = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerLeftScroll.Hdr.Obj.Width  = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerLeftScroll.Hdr.Obj.OffsetX = BOOT_SCROLL_BUTTON_SPACE;
  mBootPickerLeftScroll.Hdr.Obj.OffsetY = (DrawContext->Screen.Height - mBootPickerLeftScroll.Hdr.Obj.Height) / 2;

  mBootPickerRightScroll.CurrentImage = &GuiContext->Icons[ICON_RIGHT][ICON_TYPE_BASE];
  mBootPickerRightScroll.Hdr.Obj.Height = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerRightScroll.Hdr.Obj.Width  = BOOT_SCROLL_BUTTON_DIMENSION * GuiContext->Scale;
  mBootPickerRightScroll.Hdr.Obj.OffsetX = DrawContext->Screen.Width - mBootPickerRightScroll.Hdr.Obj.Width - BOOT_SCROLL_BUTTON_SPACE;
  mBootPickerRightScroll.Hdr.Obj.OffsetY = (DrawContext->Screen.Height - mBootPickerRightScroll.Hdr.Obj.Height) / 2;
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

  // TODO: animations should be tied to UI objects, not global
  // Each object has its own list of animations.
  // How to animate addition of one or more boot entries?
  // 1. MOVE:
  //    - Calculate the delta by which each entry moves to the left or to the right.
  //      ∆i = (N(added after) - N(added before))
  // Conditions for delta function:
  //

  if (!GuiContext->DoneIntroAnimation) {
    InitBpAnimIntro ();
    STATIC GUI_ANIMATION PickerAnim;
    PickerAnim.Context = NULL;
    PickerAnim.Animate = InternalBootPickerAnimateIntro;
    InsertHeadList (&DrawContext->Animations, &PickerAnim.Link);

    GuiContext->DoneIntroAnimation = TRUE;
  }

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
  IN BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN UINT8                    DefaultIndex
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
  InternalBootPickerSelectEntry (&mBootPicker, NULL, DefaultIndex);
  GuiContext->BootEntry = InternalGetVolumeEntry (DefaultIndex)->Context;
}

VOID
BootPickerViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN OUT BOOT_PICKER_GUI_CONTEXT  *GuiContext
  )
{
  UINT32 Index;

  for (Index = 0; Index < mBootPicker.Hdr.Obj.NumChildren; ++Index) {
    InternalBootPickerEntryDestruct (InternalGetVolumeEntry (Index));
  }

  if (mBootPicker.Hdr.Obj.Children != NULL) {
    FreePool (mBootPicker.Hdr.Obj.Children);
  }
  mBootPicker.Hdr.Obj.NumChildren = 0;

  GuiViewDeinitialize (DrawContext, GuiContext);
}

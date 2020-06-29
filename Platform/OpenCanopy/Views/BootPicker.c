/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <IndustryStandard/AppleIcon.h>

#include <Library/UefiBootServicesTableLib.h>

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


extern GUI_OBJ           mBootPickerView;
extern GUI_VOLUME_PICKER mBootPicker;
extern GUI_OBJ_CLICKABLE mBootPickerSelector;
extern CONST GUI_IMAGE   mBackgroundImage;

STATIC UINT8 mBootPickerOpacity = 0xFF;
// STATIC UINT8 mBootPickerImageIndex = 0;

BOOLEAN
GuiClickableIsHit (
  IN CONST GUI_IMAGE  *Image,
  IN INT64            OffsetX,
  IN INT64            OffsetY
  )
{
  UINT32 RowOffset;
  UINT32 IndexX;

  ASSERT (Image != NULL);
  ASSERT (Image->Buffer != NULL);

  if (OffsetX < 0 || OffsetX >= Image->Width
   || OffsetY < 0 || OffsetY >= Image->Height) {
    return FALSE;
  }

  RowOffset = (UINT32)OffsetY * Image->Width;

  if (Image->Buffer[RowOffset + OffsetX].Reserved != 0) {
    return TRUE;
  }

  for (IndexX = 0; IndexX < OffsetX; ++IndexX) {
    if (Image->Buffer[RowOffset + IndexX].Reserved != 0) {
      break;
    }
  }

  if (IndexX == OffsetX) {
    return FALSE;
  }

  for (IndexX = Image->Width - 1; IndexX > OffsetX; --IndexX) {
    if (Image->Buffer[RowOffset + IndexX].Reserved != 0) {
      break;
    }
  }

  if (IndexX == OffsetX) {
    return FALSE;
  }

  return TRUE;
}

VOID
InternalBootPickerViewDraw (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     UINT32                  OffsetX,
  IN     UINT32                  OffsetY,
  IN     UINT32                  Width,
  IN     UINT32                  Height,
  IN     BOOLEAN                 RequestDraw
  )
{
  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  GuiDrawToBuffer (
    &mBackgroundImage,
    0xFF,
    TRUE,
    DrawContext,
    BaseX,
    BaseY,
    OffsetX,
    OffsetY,
    Width,
    Height,
    TRUE
    );

  GuiObjDrawDelegate (
    This,
    DrawContext,
    Context,
    BaseX,
    BaseY,
    OffsetX,
    OffsetY,
    Width,
    Height,
    FALSE
    );
}

VOID
InternalBootPickerViewKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     INTN                    Key,
  IN     BOOLEAN                 Modifier
  )
{
  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);

  //
  // Consider moving between multiple panes with UP/DOWN and store the current
  // view within the object - for now, hardcoding this is enough.
  //
  ASSERT (mBootPicker.Hdr.Obj.KeyEvent != NULL);
  mBootPicker.Hdr.Obj.KeyEvent (
                        &mBootPicker.Hdr.Obj,
                        DrawContext,
                        Context,
                        BaseX + mBootPicker.Hdr.Obj.OffsetX,
                        BaseY + mBootPicker.Hdr.Obj.OffsetY,
                        Key,
                        Modifier
                        );
}

VOID
InternalBootPickerSelectEntry (
  IN OUT GUI_VOLUME_PICKER  *This,
  IN     GUI_VOLUME_ENTRY   *NewEntry
  )
{
  CONST GUI_OBJ *VolumeEntryObj;
  LIST_ENTRY    *SelectorNode;
  GUI_OBJ_CHILD *Selector;

  ASSERT (This != NULL);
  ASSERT (NewEntry != NULL);
  ASSERT (IsNodeInList (&This->Hdr.Obj.Children, &NewEntry->Hdr.Link));

  This->SelectedEntry = NewEntry;
  VolumeEntryObj      = &NewEntry->Hdr.Obj;

  SelectorNode = This->Hdr.Obj.Children.BackLink;
  ASSERT (SelectorNode != NULL);

  Selector = BASE_CR (SelectorNode, GUI_OBJ_CHILD, Link);
  ASSERT (Selector->Obj.Width <= VolumeEntryObj->Width);
  ASSERT_EQUALS (This->Hdr.Obj.Height, Selector->Obj.OffsetY + Selector->Obj.Height);

  Selector->Obj.OffsetX  = VolumeEntryObj->OffsetX;
  Selector->Obj.OffsetX += (VolumeEntryObj->Width - Selector->Obj.Width) / 2;
}

VOID
InternalBootPickerChangeEntry (
  IN OUT GUI_VOLUME_PICKER    *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     GUI_VOLUME_ENTRY     *NewEntry
  )
{
  GUI_VOLUME_ENTRY *PrevEntry;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (NewEntry != NULL);
  ASSERT (IsNodeInList (&This->Hdr.Obj.Children, &NewEntry->Hdr.Link));
  //
  // The caller must guarantee the entry is actually new for performance
  // reasons.
  //
  ASSERT (This->SelectedEntry != NewEntry);
  //
  // Redraw the two now (un-)selected entries.
  //
  PrevEntry = This->SelectedEntry;
  InternalBootPickerSelectEntry (This, NewEntry);
  //
  // To redraw the entry *and* the selector, draw the entire height of the
  // Picker object. For this, the height just reach from the top of the entries
  // to the bottom of the selector.
  //
  GuiDrawScreen (
    DrawContext,
    BaseX + NewEntry->Hdr.Obj.OffsetX,
    BaseY + NewEntry->Hdr.Obj.OffsetY,
    NewEntry->Hdr.Obj.Width,
    This->Hdr.Obj.Height,
    TRUE
    );

  GuiDrawScreen (
    DrawContext,
    BaseX + PrevEntry->Hdr.Obj.OffsetX,
    BaseY + PrevEntry->Hdr.Obj.OffsetY,
    PrevEntry->Hdr.Obj.Width,
    This->Hdr.Obj.Height,
    TRUE
    );
}

VOID
InternalBootPickerKeyEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *GuiContext,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     INTN                    Key,
  IN     BOOLEAN                 Modifier
  )
{
  GUI_VOLUME_PICKER       *Picker;
  GUI_VOLUME_ENTRY        *PrevEntry;
  LIST_ENTRY              *NextLink;
  GUI_VOLUME_ENTRY        *NextEntry;

  ASSERT (This != NULL);
  ASSERT (GuiContext != NULL);
  ASSERT (DrawContext != NULL);

  Picker    = BASE_CR (This, GUI_VOLUME_PICKER, Hdr.Obj);
  PrevEntry = Picker->SelectedEntry;
  ASSERT (PrevEntry != NULL);

  if (Key == OC_INPUT_RIGHT) {
    NextLink = GetNextNode (
                 &Picker->Hdr.Obj.Children,
                 &PrevEntry->Hdr.Link
                 );
    //
    // Edge-case: The last child is the selector button.
    //
    if (This->Children.BackLink != NextLink) {
      //
      // Redraw the two now (un-)selected entries.
      //
      NextEntry = BASE_CR (NextLink, GUI_VOLUME_ENTRY, Hdr.Link);
      InternalBootPickerChangeEntry (Picker, DrawContext, BaseX, BaseY, NextEntry);
    }
  } else if (Key == OC_INPUT_LEFT) {
    NextLink = GetPreviousNode (
                 &Picker->Hdr.Obj.Children,
                 &PrevEntry->Hdr.Link
                 );
    if (!IsNull (&This->Children, NextLink)) {
      //
      // Redraw the two now (un-)selected entries.
      //
      NextEntry = BASE_CR (NextLink, GUI_VOLUME_ENTRY, Hdr.Link);
      InternalBootPickerChangeEntry (Picker, DrawContext, BaseX, BaseY, NextEntry);
    }
  } else if (Key == OC_INPUT_CONTINUE) {
    ASSERT (Picker->SelectedEntry != NULL);
    Picker->SelectedEntry->Context->SetDefault = Modifier;
    GuiContext->BootEntry = Picker->SelectedEntry->Context;
  } else if (mBootPickerOpacity != 0xFF) {
    //
    // FIXME: Other keys are not allowed when boot picker is partially transparent.
    //
    return;
  }

  if (Key == OC_INPUT_MORE) {
    GuiContext->HideAuxiliary = FALSE;
    GuiContext->Refresh = TRUE;
  } else if (Key == OC_INPUT_ABORTED) {
    GuiContext->Refresh = TRUE;
  }
}

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
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
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
        FALSE,
        DrawContext,
        ParentBaseX + ChildBaseX,
        ParentBaseY + ChildBaseY,
        OffsetX,
        OffsetY,
        Width,
        Height,
        RequestDraw
        );
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
  IN     BOOLEAN                 RequestDraw
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
    (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) * DrawContext->Scale / 2,
    (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) * DrawContext->Scale / 2,
    OffsetX,
    OffsetY,
    Width,
    Height,
    RequestDraw
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
    Height,
    RequestDraw
    );
  //
  // There should be no children.
  //
  ASSERT (IsListEmpty (&This->Children));
}

STATIC
GUI_OBJ *
InternalBootPickerEntryPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     GUI_PTR_EVENT           Event,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     INT64                   OffsetX,
  IN     INT64                   OffsetY
  )
{
  STATIC BOOLEAN SameIter = FALSE;

  GUI_VOLUME_ENTRY        *Entry;
  BOOLEAN                 IsHit;

  ASSERT (Event == GuiPointerPrimaryDown
       || Event == GuiPointerPrimaryHold
       || Event == GuiPointerPrimaryUp);
  if (Event == GuiPointerPrimaryHold) {
    return This;
  }

  if (OffsetX < (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) * DrawContext->Scale / 2
   || OffsetY < (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) * DrawContext->Scale / 2) {
    return This;
  }

  Entry = BASE_CR (This, GUI_VOLUME_ENTRY, Hdr.Obj);

  IsHit = GuiClickableIsHit (
            &Entry->EntryIcon,
            OffsetX - (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) * DrawContext->Scale / 2,
            OffsetY - (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) * DrawContext->Scale / 2
            );
  if (!IsHit) {
    return This;
  }

  if (Event == GuiPointerPrimaryDown) {
    if (mBootPicker.SelectedEntry != Entry) {
      ASSERT (Entry->Hdr.Parent == &mBootPicker.Hdr.Obj);
      InternalBootPickerChangeEntry (
        &mBootPicker,
        DrawContext,
        BaseX - This->OffsetX,
        BaseY - This->OffsetY,
        Entry
        );
      SameIter = TRUE;
    }
  } else {
    //
    // This must be ensured because the UI directs Move/Up events to the object
    // Down had been sent to.
    //
    ASSERT (mBootPicker.SelectedEntry == Entry);

    if (SameIter) {
      SameIter = FALSE;
    } else {
      Context->BootEntry = Entry->Context;
    }
  }
  //
  // There should be no children.
  //
  ASSERT (IsListEmpty (&This->Children));
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
  IN     UINT32                  Height,
  IN     BOOLEAN                 RequestDraw
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
      FALSE,
      DrawContext,
      BaseX,
      BaseY,
      OffsetX,
      OffsetY,
      Width,
      Height,
      RequestDraw
      );
  }

  ButtonImage = Clickable->CurrentImage;
  ASSERT (ButtonImage != NULL);

  ASSERT_EQUALS (ButtonImage->Width , BOOT_SELECTOR_BUTTON_DIMENSION * DrawContext->Scale);
  ASSERT_EQUALS (ButtonImage->Height, BOOT_SELECTOR_BUTTON_DIMENSION * DrawContext->Scale);
  ASSERT (ButtonImage->Buffer != NULL);

  GuiDrawChildImage (
    ButtonImage,
    mBootPickerOpacity,
    DrawContext,
    BaseX,
    BaseY,
    (BOOT_SELECTOR_BACKGROUND_DIMENSION - BOOT_SELECTOR_BUTTON_DIMENSION) * DrawContext->Scale / 2,
    (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE)     * DrawContext->Scale,
    OffsetX,
    OffsetY,
    Width,
    Height,
    RequestDraw
    );
  //
  // There should be no children.
  //
  ASSERT (IsListEmpty (&This->Children));
}

GUI_OBJ *
InternalBootPickerSelectorPtrEvent (
  IN OUT GUI_OBJ                 *This,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN     GUI_PTR_EVENT           Event,
  IN     INT64                   BaseX,
  IN     INT64                   BaseY,
  IN     INT64                   OffsetX,
  IN     INT64                   OffsetY
  )
{
  GUI_OBJ_CLICKABLE             *Clickable;
  CONST GUI_IMAGE               *ButtonImage;

  BOOLEAN                       IsHit;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);
  //
  // There should be no children.
  //
  ASSERT (IsListEmpty (&This->Children));

  Clickable     = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage   = &Context->Icons[ICON_SELECTOR][ICON_TYPE_BASE];

  ASSERT (Event == GuiPointerPrimaryDown
       || Event == GuiPointerPrimaryHold
       || Event == GuiPointerPrimaryUp);
  if (OffsetX >= (BOOT_SELECTOR_BACKGROUND_DIMENSION - BOOT_SELECTOR_BUTTON_DIMENSION) * DrawContext->Scale / 2
   && OffsetY >= (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE)     * DrawContext->Scale) {
    IsHit = GuiClickableIsHit (
              ButtonImage,
              OffsetX - (BOOT_SELECTOR_BACKGROUND_DIMENSION - BOOT_SELECTOR_BUTTON_DIMENSION) * DrawContext->Scale / 2,
              OffsetY - (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE)     * DrawContext->Scale
              );
    if (IsHit) {
      if (Event == GuiPointerPrimaryUp) {
        ASSERT (mBootPicker.SelectedEntry != NULL);
        Context->BootEntry = mBootPicker.SelectedEntry->Context;
      } else  {
        ButtonImage = &Context->Icons[ICON_SELECTOR][ICON_TYPE_HELD];
      }
    }
  }

  if (Clickable->CurrentImage != ButtonImage) {
    Clickable->CurrentImage = ButtonImage;
    GuiRedrawObject (This, DrawContext, BaseX, BaseY, TRUE);
  }

  return This;
}

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ_CLICKABLE mBootPickerSelector = {
  {
    INITIALIZE_LIST_HEAD_VARIABLE (mBootPicker.Hdr.Obj.Children),
    &mBootPicker.Hdr.Obj,
    {
      0, 0, BOOT_SELECTOR_WIDTH, BOOT_SELECTOR_HEIGHT,
      InternalBootPickerSelectorDraw,
      InternalBootPickerSelectorPtrEvent,
      NULL,
      INITIALIZE_LIST_HEAD_VARIABLE (mBootPickerSelector.Hdr.Obj.Children)
    }
  },
  NULL,
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_VOLUME_PICKER mBootPicker = {
  {
    INITIALIZE_LIST_HEAD_VARIABLE (mBootPickerView.Children),
    &mBootPickerView,
    {
      0, 0, 0, BOOT_SELECTOR_HEIGHT,
      GuiObjDrawDelegate,
      GuiObjDelegatePtrEvent,
      InternalBootPickerKeyEvent,
      INITIALIZE_LIST_HEAD_VARIABLE (mBootPickerSelector.Hdr.Link)
    }
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED GUI_OBJ mBootPickerView = {
  0, 0, 0, 0,
  InternalBootPickerViewDraw,
  GuiObjDelegatePtrEvent,
  InternalBootPickerViewKeyEvent,
  INITIALIZE_LIST_HEAD_VARIABLE (mBootPicker.Hdr.Link)
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
BootPickerEntriesAdd (
  IN OC_PICKER_CONTEXT              *Context,
  IN CONST BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN OC_BOOT_ENTRY                  *Entry,
  IN BOOLEAN                        Default
  )
{
  EFI_STATUS                  Status;
  GUI_VOLUME_ENTRY            *VolumeEntry;
  CONST GUI_IMAGE             *SuggestedIcon;
  LIST_ENTRY                  *ListEntry;
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
    Status = OcGetBootEntryLabelImage (
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

  if (UseVolumeIcon) {
    Status = OcGetBootEntryIcon (Context, Entry, &IconFileData, &IconFileSize);

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
  InitializeListHead (&VolumeEntry->Hdr.Obj.Children);
  //
  // The last entry is always the selector.
  //
  ListEntry = mBootPicker.Hdr.Obj.Children.BackLink;
  ASSERT (ListEntry == &mBootPickerSelector.Hdr.Link);

  ListEntry = ListEntry->BackLink;
  ASSERT (ListEntry != NULL);

  if (!IsNull (&mBootPicker.Hdr.Obj.Children, ListEntry)) {
    PrevEntry = BASE_CR (ListEntry, GUI_VOLUME_ENTRY, Hdr.Link);
    VolumeEntry->Hdr.Obj.OffsetX = PrevEntry->Hdr.Obj.OffsetX + (BOOT_ENTRY_DIMENSION + BOOT_ENTRY_SPACE) * GuiContext->Scale;
  }

  InsertHeadList (ListEntry, &VolumeEntry->Hdr.Link);
  mBootPicker.Hdr.Obj.Width   += (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * GuiContext->Scale;
  mBootPicker.Hdr.Obj.OffsetX -= (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) * GuiContext->Scale / 2;

  if (Default) {
    InternalBootPickerSelectEntry (&mBootPicker, VolumeEntry);
  }

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

BOOLEAN
InternalBootPickerExitLoop (
  IN BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->BootEntry != NULL || Context->Refresh;
}

STATIC GUI_INTERPOLATION mBpAnimInfoOpacity;

VOID
InitBpAnimOpacity (
  IN GUI_INTERPOL_TYPE  Type,
  IN UINT64             StartTime,
  IN UINT64             Duration
  )
{
  mBpAnimInfoOpacity.Type       = Type;
  mBpAnimInfoOpacity.StartTime  = StartTime;
  mBpAnimInfoOpacity.Duration   = Duration;
  mBpAnimInfoOpacity.StartValue = 0;
  mBpAnimInfoOpacity.EndValue   = 0xFF;

  mBootPickerOpacity = 0;
}

BOOLEAN
InternalBootPickerAnimateOpacity (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  ASSERT (DrawContext != NULL);

  mBootPickerOpacity = (UINT8)GuiGetInterpolatedValue (&mBpAnimInfoOpacity, CurrentTime);
  GuiRedrawObject (
    &mBootPicker.Hdr.Obj,
    DrawContext,
    mBootPicker.Hdr.Obj.OffsetX,
    mBootPicker.Hdr.Obj.OffsetY,
    TRUE
    );

  if (mBootPickerOpacity == mBpAnimInfoOpacity.EndValue) {
    return TRUE;
    /*UINT32 OrigVal = mBpAnimInfoOpacity.EndValue;
    mBpAnimInfoOpacity.EndValue   = mBpAnimInfoOpacity.StartValue;
    mBpAnimInfoOpacity.StartValue = OrigVal;
    mBpAnimInfoOpacity.StartTime  = CurrentTime;*/
  }

  return FALSE;
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
    mBootPicker.Hdr.Obj.OffsetY,
    TRUE
    );

  if (mBootPickerImageIndex == mBpAnimInfoImageList.EndValue) {
    return TRUE;
  }
#endif
  return FALSE;
}

STATIC GUI_INTERPOLATION mBpAnimInfoSinMove;

VOID
InitBpAnimSinMov (
  IN GUI_INTERPOL_TYPE  Type,
  IN UINT64             StartTime,
  IN UINT64             Duration
  )
{
  mBpAnimInfoSinMove.Type       = Type;
  mBpAnimInfoSinMove.StartTime  = StartTime;
  mBpAnimInfoSinMove.Duration   = Duration;
  mBpAnimInfoSinMove.StartValue = 0;
  mBpAnimInfoSinMove.EndValue   = 35;
}

BOOLEAN
InternalBootPickerAnimateSinMov (
  IN     BOOT_PICKER_GUI_CONTEXT *Context,
  IN OUT GUI_DRAWING_CONTEXT     *DrawContext,
  IN     UINT64                  CurrentTime
  )
{
  STATIC BOOLEAN First = TRUE;
  STATIC BOOLEAN Minus = TRUE;
  STATIC INT64 InitOffsetX = 0;

  INT64  OldOffsetX;
  UINT32 InterpolVal;

  ASSERT (DrawContext != NULL);

  OldOffsetX = mBootPicker.Hdr.Obj.OffsetX;
  if (First) {
    First       = FALSE;
    InitOffsetX = OldOffsetX + 35;
  }

  InterpolVal = GuiGetInterpolatedValue (&mBpAnimInfoSinMove, CurrentTime);
  if (Minus) {
    mBootPicker.Hdr.Obj.OffsetX = InitOffsetX - InterpolVal;
  } else {
    mBootPicker.Hdr.Obj.OffsetX = InitOffsetX + InterpolVal;
  }

  GuiDrawScreen (
    DrawContext,
    MIN (OldOffsetX, mBootPicker.Hdr.Obj.OffsetX),
    mBootPicker.Hdr.Obj.OffsetY,
    (UINT32)(mBootPicker.Hdr.Obj.Width + ABS (OldOffsetX - mBootPicker.Hdr.Obj.OffsetX)),
    mBootPicker.Hdr.Obj.Height,
    TRUE
    );

  if (InterpolVal == mBpAnimInfoSinMove.EndValue) {
    return TRUE;
    /*Minus = !Minus;
    InitOffsetX = mBootPicker.Hdr.Obj.OffsetX;
    mBpAnimInfoSinMove.StartTime = CurrentTime;*/
  }

  return FALSE;
}

EFI_STATUS
BootPickerViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  GUI_CURSOR_GET_IMAGE     GetCursorImage
  )
{
  ASSERT (DrawContext != NULL);
  ASSERT (GuiContext != NULL);
  ASSERT (GetCursorImage != NULL);

  GuiViewInitialize (
    DrawContext,
    &mBootPickerView,
    GetCursorImage,
    InternalBootPickerExitLoop,
    GuiContext
    );
  DrawContext->Scale = GuiContext->Scale;

  mBootPickerSelector.ClickImage   = &GuiContext->Icons[ICON_SELECTOR][ICON_TYPE_BASE];
  mBootPickerSelector.CurrentImage = &GuiContext->Icons[ICON_SELECTOR][ICON_TYPE_BASE];
  mBootPickerSelector.Hdr.Obj.OffsetX = 0;
  mBootPickerSelector.Hdr.Obj.OffsetY = 0;
  mBootPickerSelector.Hdr.Obj.Height = BOOT_SELECTOR_HEIGHT * GuiContext->Scale;
  mBootPickerSelector.Hdr.Obj.Width  = BOOT_SELECTOR_WIDTH  * GuiContext->Scale;

  mBootPicker.Hdr.Obj.Height  = BOOT_SELECTOR_HEIGHT * GuiContext->Scale;
  mBootPicker.Hdr.Obj.Width   = 0;
  mBootPicker.Hdr.Obj.OffsetX = mBootPickerView.Width / 2;
  mBootPicker.Hdr.Obj.OffsetY = (mBootPickerView.Height - mBootPicker.Hdr.Obj.Height) / 2;

  // TODO: animations should be tied to UI objects, not global
  // Each object has its own list of animations.
  // How to animate addition of one or more boot entries?
  // 1. MOVE:
  //    - Calculate the delta by which each entry moves to the left or to the right.
  //      ∆i = (N(added after) - N(added before))
  // Conditions for delta function:
  //

  if (!GuiContext->DoneIntroAnimation) {
    InitBpAnimSinMov (GuiInterpolTypeSmooth, 0, 25);
    STATIC GUI_ANIMATION PickerAnim;
    PickerAnim.Context = NULL;
    PickerAnim.Animate = InternalBootPickerAnimateSinMov;
    InsertHeadList (&DrawContext->Animations, &PickerAnim.Link);

    InitBpAnimOpacity (GuiInterpolTypeSmooth, 0, 25);
    STATIC GUI_ANIMATION PickerAnim2;
    PickerAnim2.Context = NULL;
    PickerAnim2.Animate = InternalBootPickerAnimateOpacity;
    InsertHeadList (&DrawContext->Animations, &PickerAnim2.Link);

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
BootPickerViewDeinitialize (
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN OUT BOOT_PICKER_GUI_CONTEXT  *GuiContext
  )
{
  LIST_ENTRY               *ListEntry;
  LIST_ENTRY               *NextEntry;
  GUI_VOLUME_ENTRY         *BootEntry;
  CONST GUI_SCREEN_CURSOR  *ScreenCursor;

  ListEntry = mBootPicker.Hdr.Obj.Children.BackLink;
  ASSERT (ListEntry == &mBootPickerSelector.Hdr.Link);

  //
  // Last entry is always the selector, which is special and cannot be freed.
  //
  ListEntry = ListEntry->BackLink;

  while (!IsNull (&mBootPicker.Hdr.Obj.Children, ListEntry)) {
    NextEntry = ListEntry->BackLink;
    RemoveEntryList (ListEntry);

    BootEntry = BASE_CR (ListEntry, GUI_VOLUME_ENTRY, Hdr.Link);
    InternalBootPickerEntryDestruct (BootEntry);

    ListEntry = NextEntry;
  }

  ScreenCursor = GuiViewCurrentCursor (DrawContext);
  GuiContext->CursorDefaultX = ScreenCursor->X;
  GuiContext->CursorDefaultY = ScreenCursor->Y;

  GuiViewDeinitialize (DrawContext);
}

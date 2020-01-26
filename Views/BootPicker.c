#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "../GUI.h"
#include "../BmfLib.h"
#include "../GuiApp.h"

#define CURSOR_DIMENSION  24

#define BOOT_ENTRY_DIMENSION       144
#define BOOT_ENTRY_ICON_DIMENSION  128
#define BOOT_ENTRY_LABEL_SPACE     4
#define BOOT_ENTRY_LABEL_HEIGHT    13

#define BOOT_ENTRY_SPACE  8

#define BOOT_SELECTOR_WIDTH                 144
#define BOOT_SELECTOR_BACKGROUND_DIMENSION  BOOT_SELECTOR_WIDTH
#define BOOT_SELECTOR_BUTTON_DIMENSION      40
#define BOOT_SELECTOR_BUTTON_SPACE          BOOT_ENTRY_LABEL_SPACE + BOOT_ENTRY_LABEL_HEIGHT + 3
#define BOOT_SELECTOR_HEIGHT                BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE + BOOT_SELECTOR_BUTTON_DIMENSION

#define BOOT_ENTRY_WIDTH   BOOT_ENTRY_DIMENSION
#define BOOT_ENTRY_HEIGHT  BOOT_ENTRY_DIMENSION + BOOT_ENTRY_LABEL_SPACE + BOOT_ENTRY_LABEL_HEIGHT

typedef struct {
  GUI_OBJ_CHILD         Hdr;
  CONST GUI_CLICK_IMAGE *ClickImage;
  CONST GUI_IMAGE       *CurrentImage;
} GUI_OBJ_CLICKABLE;

typedef struct {
  GUI_OBJ_CHILD   Hdr;
  CONST GUI_IMAGE *EntryIcon;
  GUI_IMAGE       Label;
  VOID            *Context;
} GUI_VOLUME_ENTRY;

typedef struct {
  GUI_OBJ_CHILD    Hdr;
  GUI_VOLUME_ENTRY *SelectedEntry;
} GUI_VOLUME_PICKER;

extern GUI_OBJ           mBootPickerView;
extern GUI_VOLUME_PICKER mBootPicker;
extern GUI_OBJ_CLICKABLE mBootPickerSelector;

STATIC UINT8 mBootPickerOpacity = 0xFF;

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
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  )
{
  //
  // Set the background to black with 100 % opacity.
  //
  STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL BlackPixel = {
    0x00, 0x00, 0x00, 0xFF
  };

  STATIC CONST GUI_IMAGE BlackImage = { 1, 1, &BlackPixel };

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  GuiDrawToBuffer (
    &BlackImage,
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
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     CONST EFI_INPUT_KEY  *Key
  )
{
  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Key != NULL);

  ASSERT (mBootPicker.Hdr.Obj.OffsetX >= 0);
  ASSERT (mBootPicker.Hdr.Obj.OffsetY >= 0);
  //
  // Consider moving between multiple panes with UP/DOWN and store the current
  // view within the object - for now, hardcoding this is enough.
  //
  ASSERT (mBootPicker.Hdr.Obj.KeyEvent != NULL);
  mBootPicker.Hdr.Obj.KeyEvent (
                        &mBootPicker.Hdr.Obj,
                        DrawContext,
                        Context,
                        BaseX + (UINT32)mBootPicker.Hdr.Obj.OffsetX,
                        BaseY + (UINT32)mBootPicker.Hdr.Obj.OffsetY,
                        Key
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
  ASSERT (This->Hdr.Obj.Height == Selector->Obj.OffsetY + Selector->Obj.Height);

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
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     CONST EFI_INPUT_KEY  *Key
  )
{
  GUI_VOLUME_PICKER       *Picker;
  GUI_VOLUME_ENTRY        *PrevEntry;
  LIST_ENTRY              *NextLink;
  GUI_VOLUME_ENTRY        *NextEntry;
  BOOT_PICKER_GUI_CONTEXT *GuiContext;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Key != NULL);

  Picker    = BASE_CR (This, GUI_VOLUME_PICKER, Hdr.Obj);
  PrevEntry = Picker->SelectedEntry;
  ASSERT (PrevEntry != NULL);

  if (Key->ScanCode == SCAN_RIGHT) {
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
  } else if (Key->ScanCode == SCAN_LEFT) {
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
  } else if (Key->UnicodeChar == CHAR_CARRIAGE_RETURN) {
    ASSERT (Context != NULL);
    ASSERT (Picker->SelectedEntry != NULL);
    GuiContext = (BOOT_PICKER_GUI_CONTEXT *)Context;
    GuiContext->BootEntry = Picker->SelectedEntry->Context;
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
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  )
{
  CONST GUI_VOLUME_ENTRY *Entry;
  CONST GUI_IMAGE        *EntryIcon;
  CONST GUI_IMAGE        *Label;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  Entry       = BASE_CR (This, GUI_VOLUME_ENTRY, Hdr.Obj);
  EntryIcon   = Entry->EntryIcon;
  Label       = &Entry->Label;

  ASSERT (This->Width  == BOOT_ENTRY_DIMENSION);
  ASSERT (This->Height == BOOT_ENTRY_HEIGHT);
  //
  // Draw the icon horizontally centered.
  //
  ASSERT (EntryIcon->Width  == BOOT_ENTRY_ICON_DIMENSION);
  ASSERT (EntryIcon->Height == BOOT_ENTRY_ICON_DIMENSION);

  GuiDrawChildImage (
    EntryIcon,
    mBootPickerOpacity,
    DrawContext,
    BaseX,
    BaseY,
    (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) / 2,
    (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) / 2,
    OffsetX,
    OffsetY,
    Width,
    Height,
    RequestDraw
    );
  //
  // Draw the label horizontally centered.
  //
  ASSERT (Label->Width  <= BOOT_ENTRY_DIMENSION);
  ASSERT (Label->Height == BOOT_ENTRY_LABEL_HEIGHT);

  GuiDrawChildImage (
    Label,
    mBootPickerOpacity,
    DrawContext,
    BaseX,
    BaseY,
    (BOOT_ENTRY_DIMENSION - Label->Width) / 2,
    BOOT_ENTRY_DIMENSION + BOOT_ENTRY_LABEL_SPACE,
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
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     GUI_PTR_EVENT        Event,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     INT64                OffsetX,
  IN     INT64                OffsetY
  )
{
  STATIC BOOLEAN SameIter = FALSE;

  GUI_VOLUME_ENTRY        *Entry;
  BOOT_PICKER_GUI_CONTEXT *GuiContext;
  BOOLEAN                 IsHit;

  ASSERT (Event == GuiPointerPrimaryDown
       || Event == GuiPointerPrimaryHold
       || Event == GuiPointerPrimaryUp);
  if (Event == GuiPointerPrimaryHold) {
    return This;
  }

  if (OffsetX < (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) / 2
   || OffsetY < (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) / 2) {
    return This;
  }

  Entry = BASE_CR (This, GUI_VOLUME_ENTRY, Hdr.Obj);

  IsHit = GuiClickableIsHit (
            Entry->EntryIcon,
            OffsetX - (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) / 2,
            OffsetY - (BOOT_ENTRY_DIMENSION - BOOT_ENTRY_ICON_DIMENSION) / 2
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
      GuiContext = (BOOT_PICKER_GUI_CONTEXT *)Context;
      GuiContext->BootEntry = Entry->Context;
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
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  )
{
  CONST GUI_OBJ_CLICKABLE       *Clickable;
  CONST BOOT_PICKER_GUI_CONTEXT *GuiContext;
  CONST GUI_IMAGE               *BackgroundImage;
  CONST GUI_IMAGE               *ButtonImage;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);

  Clickable  = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  GuiContext = (BOOT_PICKER_GUI_CONTEXT *)Context;

  ASSERT (This->Width  == BOOT_SELECTOR_WIDTH);
  ASSERT (This->Height == BOOT_SELECTOR_HEIGHT);

  BackgroundImage = &GuiContext->EntryBackSelected;

  ASSERT (BackgroundImage->Width  == BOOT_SELECTOR_BACKGROUND_DIMENSION);
  ASSERT (BackgroundImage->Height == BOOT_SELECTOR_BACKGROUND_DIMENSION);
  ASSERT (BackgroundImage->Buffer != NULL);
  //
  // Background starts at (0,0) and is as wide as This.
  //
  if (OffsetY < BOOT_SELECTOR_BACKGROUND_DIMENSION) {
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

  ASSERT (ButtonImage->Width  == BOOT_SELECTOR_BUTTON_DIMENSION);
  ASSERT (ButtonImage->Height == BOOT_SELECTOR_BUTTON_DIMENSION);
  ASSERT (ButtonImage->Buffer != NULL);

  GuiDrawChildImage (
    ButtonImage,
    mBootPickerOpacity,
    DrawContext,
    BaseX,
    BaseY,
    (BOOT_SELECTOR_BACKGROUND_DIMENSION - BOOT_SELECTOR_BUTTON_DIMENSION) / 2,
    BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE,
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
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     GUI_PTR_EVENT        Event,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     INT64                OffsetX,
  IN     INT64                OffsetY
  )
{
  BOOT_PICKER_GUI_CONTEXT       *GuiContext;
  GUI_OBJ_CLICKABLE             *Clickable;
  CONST BOOT_PICKER_GUI_CONTEXT *PickerContext;
  CONST GUI_IMAGE               *ButtonImage;

  BOOLEAN                       IsHit;

  ASSERT (This != NULL);
  ASSERT (DrawContext != NULL);
  ASSERT (Context != NULL);
  //
  // There should be no children.
  //
  ASSERT (IsListEmpty (&This->Children));

  PickerContext = (BOOT_PICKER_GUI_CONTEXT *)Context;
  Clickable     = BASE_CR (This, GUI_OBJ_CLICKABLE, Hdr.Obj);
  ButtonImage   = &PickerContext->EntrySelector.BaseImage;

  ASSERT (Event == GuiPointerPrimaryDown
       || Event == GuiPointerPrimaryHold
       || Event == GuiPointerPrimaryUp);
  if (OffsetX >= (BOOT_SELECTOR_BACKGROUND_DIMENSION - BOOT_SELECTOR_BUTTON_DIMENSION) / 2
   && OffsetY >= BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE) {
    IsHit = GuiClickableIsHit (
              ButtonImage,
              OffsetX - (BOOT_SELECTOR_BACKGROUND_DIMENSION - BOOT_SELECTOR_BUTTON_DIMENSION) / 2,
              OffsetY - (BOOT_SELECTOR_BACKGROUND_DIMENSION + BOOT_SELECTOR_BUTTON_SPACE)
              );
    if (IsHit) {
      if (Event == GuiPointerPrimaryUp) {
        ASSERT (mBootPicker.SelectedEntry != NULL);
        GuiContext = (BOOT_PICKER_GUI_CONTEXT *)Context;
        GuiContext->BootEntry = mBootPicker.SelectedEntry->Context;
      } else  {
        ButtonImage = &PickerContext->EntrySelector.HoldImage;
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

RETURN_STATUS
BootPickerEntriesAdd (
  IN CONST BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN CONST CHAR16                   *Name,
  IN VOID                           *EntryContext,
  IN BOOLEAN                        IsExternal,
  IN BOOLEAN                        Default
  )
{
  BOOLEAN                Result;
  GUI_VOLUME_ENTRY       *VolumeEntry;
  LIST_ENTRY             *ListEntry;
  CONST GUI_VOLUME_ENTRY *PrevEntry;

  ASSERT (GuiContext != NULL);
  ASSERT (Name != NULL);

  VolumeEntry = AllocateZeroPool (sizeof (*VolumeEntry));
  if (VolumeEntry == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Result = GuiGetLabel (
             &VolumeEntry->Label,
             &GuiContext->FontContext,
             Name,
             StrLen (Name)
             );
  if (!Result) {
    DEBUG ((DEBUG_WARN, "BMF: label failed\n"));
    return RETURN_UNSUPPORTED;
  }

  VolumeEntry->Context = EntryContext;

  if (!IsExternal) {
    VolumeEntry->EntryIcon = &GuiContext->EntryIconInternal;
  } else {
    VolumeEntry->EntryIcon = &GuiContext->EntryIconExternal;
  }

  VolumeEntry->Hdr.Parent       = &mBootPicker.Hdr.Obj;
  VolumeEntry->Hdr.Obj.Width    = BOOT_ENTRY_WIDTH;
  VolumeEntry->Hdr.Obj.Height   = BOOT_ENTRY_HEIGHT;
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
    VolumeEntry->Hdr.Obj.OffsetX = PrevEntry->Hdr.Obj.OffsetX + BOOT_ENTRY_DIMENSION + BOOT_ENTRY_SPACE;
  }

  InsertHeadList (ListEntry, &VolumeEntry->Hdr.Link);
  mBootPicker.Hdr.Obj.Width   += BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE;
  mBootPicker.Hdr.Obj.OffsetX -= (BOOT_ENTRY_WIDTH + BOOT_ENTRY_SPACE) / 2;

  if (Default) {
    InternalBootPickerSelectEntry (&mBootPicker, VolumeEntry);
  }

  return RETURN_SUCCESS;
}

VOID
InternalBootPickerEntryDestruct (
  IN GUI_VOLUME_ENTRY  *Entry
  )
{
  ASSERT (Entry != NULL);
  ASSERT (Entry->Label.Buffer != NULL);

  FreePool (Entry->Label.Buffer);
  FreePool (Entry);
}

VOID
BootPickerEntriesEmpty (
  VOID
  )
{
  LIST_ENTRY       *ListEntry;
  LIST_ENTRY       *NextEntry;
  GUI_VOLUME_ENTRY *BootEntry;

  ListEntry = mBootPicker.Hdr.Obj.Children.BackLink;
  ASSERT (ListEntry == &mBootPickerSelector.Hdr.Link);

  ListEntry = ListEntry->BackLink;

  while (!IsNull (&mBootPicker.Hdr.Obj.Children, ListEntry)) {
    NextEntry = ListEntry->BackLink;
    RemoveEntryList (ListEntry);
    ListEntry = NextEntry;

    BootEntry = BASE_CR (ListEntry, GUI_VOLUME_ENTRY, Hdr.Link);
    InternalBootPickerEntryDestruct (BootEntry);
  }
}

BOOLEAN
InternalBootPickerExitLoop (
  IN VOID  *Context
  )
{
  CONST BOOT_PICKER_GUI_CONTEXT *GuiContext;

  ASSERT (Context != NULL);

  GuiContext = (CONST BOOT_PICKER_GUI_CONTEXT *)Context;
  return GuiContext->BootEntry != NULL;
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
  IN     VOID                 *Context OPTIONAL,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     UINT64               CurrentTime
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
  IN     VOID                 *Context OPTIONAL,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     UINT64               CurrentTime
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

RETURN_STATUS
BootPickerViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  GUI_CURSOR_GET_IMAGE     GetCursorImage
  )
{
  ASSERT (DrawContext != NULL);
  ASSERT (GuiContext != NULL);
  ASSERT (GetCursorImage != NULL);

  if (GuiContext->Cursor.Height                  != CURSOR_DIMENSION
   || GuiContext->Cursor.Width                   != CURSOR_DIMENSION
   || GuiContext->EntryBackSelected.Height       != BOOT_SELECTOR_BACKGROUND_DIMENSION
   || GuiContext->EntryBackSelected.Width        != BOOT_SELECTOR_BACKGROUND_DIMENSION
   || GuiContext->EntrySelector.BaseImage.Height != BOOT_SELECTOR_BUTTON_DIMENSION
   || GuiContext->EntrySelector.BaseImage.Width  != BOOT_SELECTOR_BUTTON_DIMENSION
   || GuiContext->EntryIconInternal.Height       != BOOT_ENTRY_ICON_DIMENSION
   || GuiContext->EntryIconInternal.Width        != BOOT_ENTRY_ICON_DIMENSION
   || GuiContext->EntryIconExternal.Height       != BOOT_ENTRY_ICON_DIMENSION
   || GuiContext->EntryIconExternal.Width        != BOOT_ENTRY_ICON_DIMENSION
   || GuiContext->FontContext.BmfContext.Height  != BOOT_ENTRY_LABEL_HEIGHT) {
    return RETURN_UNSUPPORTED;
  }

  GuiViewInitialize (
    DrawContext,
    &mBootPickerView,
    GetCursorImage,
    InternalBootPickerExitLoop,
    GuiContext
    );

  mBootPickerSelector.ClickImage   = &GuiContext->EntrySelector;
  mBootPickerSelector.CurrentImage = &GuiContext->EntrySelector.BaseImage;

  mBootPicker.Hdr.Obj.OffsetX = mBootPickerView.Width / 2;
  mBootPicker.Hdr.Obj.OffsetY = (mBootPickerView.Height - mBootPicker.Hdr.Obj.Height) / 2;

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

  return RETURN_SUCCESS;
}

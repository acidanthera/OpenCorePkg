#ifndef GUI_APP_H
#define GUI_APP_H

typedef struct {
  GUI_CLICK_IMAGE  EntrySelector;
  GUI_IMAGE        EntryIconInternal;
  GUI_IMAGE        EntryIconExternal;
  GUI_IMAGE        EntryBackSelected;
  GUI_IMAGE        Cursor;
  GUI_FONT_CONTEXT FontContext;
  VOID             *BootEntry;
  BOOLEAN          HideAuxiliary;
  BOOLEAN          Refresh;
} BOOT_PICKER_GUI_CONTEXT;

RETURN_STATUS
BootPickerViewInitialize (
  OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN  BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN  GUI_CURSOR_GET_IMAGE     GetCursorImage
  );

RETURN_STATUS
BootPickerEntriesAdd (
  IN CONST BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN CONST CHAR16                   *Name,
  IN VOID                           *EntryContext,
  IN BOOLEAN                        IsExternal,
  IN BOOLEAN                        Default
  );

VOID
BootPickerEntriesEmpty (
  VOID
  );

CONST GUI_IMAGE *
InternalGetCursorImage (
  IN OUT GUI_SCREEN_CURSOR  *This,
  IN     VOID               *Context
  );

#endif // GUI_APP_H

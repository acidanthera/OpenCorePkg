#include <Uefi.h>

#include <Protocol/OcInterface.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcStorageLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "GUI.h"
#include "BmfLib.h"
#include "GuiApp.h"

extern unsigned char Cursor_png[];
extern unsigned int  Cursor_png_len;

extern unsigned char IconHd_png[];
extern unsigned int  IconHd_png_len;

extern unsigned char IconHdExt_png[];
extern unsigned int  IconHdExt_png_len;

extern unsigned char IconSelected_png[];
extern unsigned int  IconSelected_png_len;

extern unsigned char Selector_png[];
extern unsigned int  Selector_png_len;

GLOBAL_REMOVE_IF_UNREFERENCED BOOT_PICKER_GUI_CONTEXT mGuiContext = { { { 0 } } };

STATIC
VOID
InternalSafeFreePool (
  IN CONST VOID  *Memory
  )
{
  if (Memory != NULL) {
    FreePool ((VOID *)Memory);
  }
}

STATIC
VOID
InternalContextDestruct (
  IN OUT BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  InternalSafeFreePool (Context->Cursor.Buffer);
  InternalSafeFreePool (Context->EntryBackSelected.Buffer);
  InternalSafeFreePool (Context->EntrySelector.BaseImage.Buffer);
  InternalSafeFreePool (Context->EntrySelector.HoldImage.Buffer);
  InternalSafeFreePool (Context->EntryIconInternal.Buffer);
  InternalSafeFreePool (Context->EntryIconInternal.Buffer);
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->FontContext.FontImage.Buffer);
}

RETURN_STATUS
InternalContextConstruct (
  OUT BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL HighlightPixel = {
    0xAF, 0xAF, 0xAF, 0x32
  };

  RETURN_STATUS Status;
  BOOLEAN       Result;

  ASSERT (Context != NULL);

  Context->BootEntry = NULL;

  Status  = GuiBmpToImage (&Context->Cursor,            Cursor_png,       Cursor_png_len);
  Status |= GuiBmpToImage (&Context->EntryBackSelected, IconSelected_png, IconSelected_png_len);
  Status |= GuiBmpToClickImage (&Context->EntrySelector,     Selector_png,     Selector_png_len,  &HighlightPixel);
  Status |= GuiBmpToImage (&Context->EntryIconInternal, IconHd_png,       IconHd_png_len);
  Status |= GuiBmpToImage (&Context->EntryIconExternal, IconHdExt_png,    IconHdExt_png_len);
  if (RETURN_ERROR (Status)) {
    InternalContextDestruct (Context);
    return RETURN_UNSUPPORTED;
  }

  Result = GuiInitializeFontHelvetica (&Context->FontContext);
  if (!Result) {
    DEBUG ((DEBUG_WARN, "BMF: Helvetica failed\n"));
    InternalContextDestruct (Context);
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

CONST GUI_IMAGE *
InternalGetCursorImage (
  IN OUT GUI_SCREEN_CURSOR  *This,
  IN     VOID               *Context
  )
{
  CONST BOOT_PICKER_GUI_CONTEXT *GuiContext;

  ASSERT (This != NULL);
  ASSERT (Context != NULL);

  GuiContext = (CONST BOOT_PICKER_GUI_CONTEXT *)Context;
  return &GuiContext->Cursor;
}

EFI_STATUS
EFIAPI
UefiUnload (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  GuiLibDestruct ();
  return EFI_SUCCESS;
}

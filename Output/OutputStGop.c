#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "GuiIo.h"

struct GUI_OUTPUT_CONTEXT_ {
  EFI_GRAPHICS_OUTPUT_PROTOCOL Gop;
};

GUI_OUTPUT_CONTEXT *
GuiOutputConstruct (
  VOID
  )
{
  return GuiOutputConstructStGop ();
}

EFI_STATUS
EFIAPI
GuiOutputBlt (
  IN GUI_OUTPUT_CONTEXT                 *Context,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN UINTN                              SourceX,
  IN UINTN                              SourceY,
  IN UINTN                              DestinationX,
  IN UINTN                              DestinationY,
  IN UINTN                              Width,
  IN UINTN                              Height,
  IN UINTN                              Delta OPTIONAL
  )
{
  return GuiOutputBltStGop (
           Context,
           BltBuffer,
           BltOperation,
           SourceX,
           SourceY,
           DestinationX,
           DestinationY,
           Width,
           Height,
           Delta
           );
}

CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *
GuiOutputGetInfo (
  IN GUI_OUTPUT_CONTEXT  *Context
  )
{
  return GuiOutputGetInfo (Context);
}

VOID
GuiOutputDestruct (
  IN GUI_OUTPUT_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  GuiOutputDestructStGop (Context);
}

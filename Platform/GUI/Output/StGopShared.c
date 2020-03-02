#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "GuiIo.h"

struct GUI_OUTPUT_CONTEXT_GOP_ST_ {
  EFI_GRAPHICS_OUTPUT_PROTOCOL Gop;
};

GUI_OUTPUT_CONTEXT_GOP_ST *
GuiOutputConstructStGop (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (
                    &gEfiGraphicsOutputProtocolGuid,
                    NULL,
                    (VOID **)&Gop
                    );
    if (EFI_ERROR (Status)) {
      return NULL;
    }
  }

  return (GUI_OUTPUT_CONTEXT_GOP_ST *)Gop;
}

EFI_STATUS
EFIAPI
GuiOutputBltStGop (
  IN GUI_OUTPUT_CONTEXT_GOP_ST          *Context,
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
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

  Gop = &Context->Gop;
  return Gop->Blt (
                Gop,
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
GuiOutputGetInfoStGop (
  IN GUI_OUTPUT_CONTEXT_GOP_ST  *Context
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

  ASSERT (Context != NULL);

  Gop = &Context->Gop;
  return Gop->Mode->Info;
}

VOID
GuiOutputDestructStGop (
  IN GUI_OUTPUT_CONTEXT_GOP_ST  *Context
  )
{
  ASSERT (Context != NULL);
}

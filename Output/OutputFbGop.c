#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/DebugLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MtrrLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "GuiIo.h"
#include "HwOps.h"

struct GUI_OUTPUT_CONTEXT_ {
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  FRAME_BUFFER_CONFIGURE               *Config;
  GUI_OUTPUT_CONTEXT_GOP_ST            *GopStContext;
};

BOOLEAN
InternalOutputConstruct (
  OUT GUI_OUTPUT_CONTEXT  *Context
  )
{
  EFI_STATUS                   Status;
  UINTN                        NumHandles;
  EFI_HANDLE                   *Handles;
  UINTN                        Index;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  FRAME_BUFFER_CONFIGURE       *FbConfig;
  UINTN                        FbConfigSize;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  for (Index = 0; Index < NumHandles; ++Index) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID **)&Gop
                    );
    if (!EFI_ERROR (Status) && Gop->Mode->Info->PixelFormat != PixelBltOnly) {
      gBS->FreePool (Handles);

      FbConfigSize = 0;
      Status = FrameBufferBltConfigure (
                 (VOID *)(UINTN)Gop->Mode->FrameBufferBase,
                 Gop->Mode->Info,
                 NULL,
                 &FbConfigSize
                 );
      if (Status != RETURN_BUFFER_TOO_SMALL) {
        return FALSE;
      }

      FbConfig = AllocatePool (FbConfigSize);
      if (FbConfig == NULL) {
        return FALSE;
      }

      Status = FrameBufferBltConfigure (
                 (VOID *)(UINTN)Gop->Mode->FrameBufferBase,
                 Gop->Mode->Info,
                 FbConfig,
                 &FbConfigSize
                 );
      if (RETURN_ERROR (Status)) {
        return FALSE;
      }

      GuiMtrrSetMemoryAttribute (
        Gop->Mode->FrameBufferBase,
        Gop->Mode->FrameBufferSize,
        CacheWriteCombining
        );

      Context->Info         = Gop->Mode->Info;
      Context->Config       = FbConfig;
      Context->GopStContext = NULL;
      return TRUE;
    }
  }

  gBS->FreePool (Handles);
  return FALSE;
}

GUI_OUTPUT_CONTEXT *
GuiOutputConstruct (
  VOID
  )
{
  // TODO: alloc on call?
  STATIC GUI_OUTPUT_CONTEXT Context;

  BOOLEAN Result;

  Result = InternalOutputConstruct (&Context);
  if (!Result) {
    Context.GopStContext = GuiOutputConstructStGop ();
    if (Context.GopStContext == NULL) {
      return NULL;
    }

    Context.Config = NULL;
    Context.Info   = NULL;
  }

  return &Context;
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
  if (Context->GopStContext == NULL) {
    return FrameBufferBlt (
             Context->Config,
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
  } else {
    return GuiOutputBltStGop (
             Context->GopStContext,
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
}

CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *
GuiOutputGetInfo (
  IN GUI_OUTPUT_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  if (Context->GopStContext == NULL) {
    return Context->Info;
  } else {
    return GuiOutputGetInfoStGop (Context->GopStContext);
  }
}

VOID
GuiOutputDestruct (
  IN GUI_OUTPUT_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  if (Context->GopStContext == NULL) {
    if (Context->Config != NULL) {
      FreePool (Context->Config);
    }
  } else {
    GuiOutputDestructStGop (Context->GopStContext);
  }
}

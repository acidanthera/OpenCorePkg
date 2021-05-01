/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../GuiIo.h"

#define MIN_RESOLUTION_HORIZONTAL  640U
#define MIN_RESOLUTION_VERTICAL    480U

struct GUI_OUTPUT_CONTEXT_ {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
};

STATIC
EFI_GRAPHICS_OUTPUT_PROTOCOL *
InternalGuiOutputLocateGop (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &Gop
    );
  if (EFI_ERROR (Status)) {
    //
    // Do not fall back to other GOP instances to match AppleEvent.
    //
    return NULL;
  }

  return Gop;
}

GUI_OUTPUT_CONTEXT *
GuiOutputConstruct (
  IN UINT32  Scale
  )
{
  // TODO: alloc on the fly?
  STATIC GUI_OUTPUT_CONTEXT Context;

  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;

  Gop = InternalGuiOutputLocateGop();
  if (Gop == NULL) {
    return NULL;
  }

  if (Gop->Mode->Info->HorizontalResolution < MIN_RESOLUTION_HORIZONTAL * Scale
   || Gop->Mode->Info->VerticalResolution < MIN_RESOLUTION_VERTICAL * Scale) {
    DEBUG ((
      DEBUG_INFO,
      "OCUI: Expected at least %dx%d for resolution, actual %dx%d\n",
      MIN_RESOLUTION_HORIZONTAL * Scale,
      MIN_RESOLUTION_VERTICAL * Scale,
      Context.Gop->Mode->Info->HorizontalResolution,
      Context.Gop->Mode->Info->VerticalResolution
      ));
    return NULL;
  }

  Context.Gop = Gop;
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
  return Context->Gop->Blt (
    Context->Gop,
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
  return Context->Gop->Mode->Info;
}

VOID
GuiOutputDestruct (
  IN GUI_OUTPUT_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ZeroMem (Context, sizeof (*Context));
}

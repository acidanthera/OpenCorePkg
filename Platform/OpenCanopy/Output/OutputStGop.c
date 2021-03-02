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
  VOID
  )
{
  GUI_OUTPUT_CONTEXT            *Context;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;

  Gop = InternalGuiOutputLocateGop();
  if (Gop == NULL) {
    return NULL;
  }

  Context = AllocatePool (sizeof (*Context));
  if (Context == NULL) {
    return NULL;
  }

  Context->Gop = Gop;
  return Context;
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
  FreePool (Context);
}

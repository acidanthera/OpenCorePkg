/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/AppleEvent.h>
#include <Protocol/AbsolutePointer.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../OpenCanopy.h"
#include "../GuiIo.h"

struct GUI_POINTER_CONTEXT_ {
  APPLE_EVENT_PROTOCOL          *AppleEvent;
  EFI_ABSOLUTE_POINTER_PROTOCOL *AbsPointer;
  APPLE_EVENT_HANDLE            AppleEventHandle;
  UINT32                        MaxX;
  UINT32                        MaxY;
  INT32                         RawX;
  INT32                         RawY;
  GUI_POINTER_STATE             CurState;
  UINT8                         LockedBy;
};

enum {
  PointerUnlocked,
  PointerLockedSimple,
  PointerLockedAbsolute
};

STATIC
VOID
EFIAPI 
InternalAppleEventNotification (
  IN APPLE_EVENT_INFORMATION  *Information,
  IN VOID                     *NotifyContext
  )
{
  APPLE_POINTER_EVENT_TYPE  EventType;
  GUI_POINTER_CONTEXT       *Context;
  INT32                     NewX;
  INT32                     NewY;
  INT64                     NewCoord;

  Context = NotifyContext;

  ASSERT ((Information->EventType & APPLE_ALL_MOUSE_EVENTS) != 0);
  //
  // Discard simple pointer updates when absolute pointer locked.
  //
  if (Context->LockedBy == PointerLockedAbsolute) {
    return;
  }

  if ((Context->CurState.PrimaryDown | Context->CurState.SecondaryDown) == 0) {
    ASSERT (Context->LockedBy == PointerUnlocked);
  } else {
    ASSERT (Context->LockedBy == PointerLockedSimple);
  }

  EventType = Information->EventData.PointerEventType;

  if ((EventType & APPLE_EVENT_TYPE_MOUSE_MOVED) != 0) {
    //
    // Use a factor of 2 for pointer acceleration.
    //

    NewX = Information->PointerPosition.Horizontal;
    if (NewX == 0 || (UINT32) NewX == Context->MaxX) {
      Context->CurState.X = (UINT32) NewX;
    } else {
      NewCoord = (INT64) Context->CurState.X + 2 * ((INT64) NewX - Context->RawX);
      if (NewCoord < 0) {
        NewCoord = 0;
      } else if (NewCoord > Context->MaxX) {
        NewCoord = Context->MaxX;
      }

      Context->CurState.X = (UINT32) NewCoord;
    }

    Context->RawX = NewX;

    NewY = Information->PointerPosition.Vertical;
    if (NewY == 0 || (UINT32) NewY == Context->MaxY) {
      Context->CurState.Y = (UINT32) NewY;
    } else {
      NewCoord = (INT64) Context->CurState.Y + 2 * ((INT64) NewY - Context->RawY);
      if (NewCoord < 0) {
        NewCoord = 0;
      } else if (NewCoord > Context->MaxY) {
        NewCoord = Context->MaxY;
      }

      Context->CurState.Y = (UINT32) NewCoord;
    }

    Context->RawY = NewY;
  }

  if ((EventType & APPLE_EVENT_TYPE_MOUSE_DOWN) != 0) {
    if ((EventType & APPLE_EVENT_TYPE_LEFT_BUTTON) != 0) {
      Context->CurState.PrimaryDown = TRUE;
    } else if ((EventType & APPLE_EVENT_TYPE_RIGHT_BUTTON) != 0) {
      Context->CurState.SecondaryDown = TRUE;
    }
  } else if ((EventType & APPLE_EVENT_TYPE_MOUSE_UP) != 0) {
    if ((EventType & APPLE_EVENT_TYPE_LEFT_BUTTON) != 0) {
      Context->CurState.PrimaryDown = FALSE;
    } else if ((EventType & APPLE_EVENT_TYPE_RIGHT_BUTTON) != 0) {
      Context->CurState.SecondaryDown = FALSE;
    }
  }

  if ((Context->CurState.PrimaryDown | Context->CurState.SecondaryDown) == 0) {
    Context->LockedBy = PointerUnlocked;
  } else {
    Context->LockedBy = PointerLockedSimple;
  }
}

STATIC
VOID
InternalUpdateContextAbsolute (
  IN OUT GUI_POINTER_CONTEXT  *Context
  )
{
  EFI_STATUS                 Status;
  EFI_ABSOLUTE_POINTER_STATE PointerState;
  UINT64                     NewX;
  UINT64                     NewY;

  ASSERT (Context != NULL);
  //
  // Discard absolute pointer updates when simple pointer locked.
  //
  if (Context->LockedBy == PointerLockedSimple) {
    return;
  }

  if ((Context->CurState.PrimaryDown | Context->CurState.SecondaryDown) == 0) {
    ASSERT (Context->LockedBy == PointerUnlocked);
  } else {
    ASSERT (Context->LockedBy == PointerLockedAbsolute);
  }

  if (Context->AbsPointer == NULL) {
    return;
  }

  Status = Context->AbsPointer->GetState (Context->AbsPointer, &PointerState);
  if (EFI_ERROR (Status)) {
    return;
  }

  NewX  = PointerState.CurrentX - Context->AbsPointer->Mode->AbsoluteMinX;
  NewX *= Context->MaxX + 1;
  Context->CurState.X = (UINT32) DivU64x32 (
    NewX,
    (UINT32) (Context->AbsPointer->Mode->AbsoluteMaxX - Context->AbsPointer->Mode->AbsoluteMinX)
    );
  Context->RawX = (INT32) Context->CurState.X;

  NewY  = PointerState.CurrentY - Context->AbsPointer->Mode->AbsoluteMinY;
  NewY *= Context->MaxY + 1;
  Context->CurState.Y = (UINT32) DivU64x32 (
    NewY,
    (UINT32) (Context->AbsPointer->Mode->AbsoluteMaxY - Context->AbsPointer->Mode->AbsoluteMinY)
    );
  Context->RawY = (INT32) Context->CurState.Y;

  Context->CurState.PrimaryDown   = (PointerState.ActiveButtons & EFI_ABSP_TouchActive) != 0;
  Context->CurState.SecondaryDown = (PointerState.ActiveButtons & EFI_ABS_AltActive) != 0;

  if ((Context->CurState.PrimaryDown | Context->CurState.SecondaryDown) == 0) {
    Context->LockedBy = PointerUnlocked;
  } else {
    Context->LockedBy = PointerLockedAbsolute;
  }
}

VOID
GuiPointerReset (
  IN OUT GUI_POINTER_CONTEXT  *Context
  )
{
  EFI_ABSOLUTE_POINTER_STATE AbsoluteState;

  ASSERT (Context != NULL);

  if (Context->AbsPointer != NULL) {
    Context->AbsPointer->GetState (Context->AbsPointer, &AbsoluteState);
  }

  Context->LockedBy = PointerUnlocked;
}

VOID
GuiPointerGetState (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  )
{
  EFI_TPL OldTpl;

  ASSERT (Context != NULL);
  ASSERT (State != NULL);
  //
  // Prevent simple pointer updates during state retrieval.
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  //
  // The simple pointer updates are done in InternalAppleEventNotification().
  //
  InternalUpdateContextAbsolute (Context);
  //
  // Return the current pointer state.
  //
  State->X             = Context->CurState.X;
  State->Y             = Context->CurState.Y;
  State->PrimaryDown   = Context->CurState.PrimaryDown;
  State->SecondaryDown = Context->CurState.SecondaryDown;

  gBS->RestoreTPL (OldTpl);
}

GUI_POINTER_CONTEXT *
GuiPointerConstruct (
  IN UINT32  DefaultX,
  IN UINT32  DefaultY,
  IN UINT32  Width,
  IN UINT32  Height
  )
{
  // TODO: alloc on the fly?
  STATIC GUI_POINTER_CONTEXT Context;

  EFI_STATUS Status;
  EFI_STATUS Status2;
  EFI_TPL    OldTpl;
  DIMENSION  Dimension;

  ASSERT (DefaultX < Width);
  ASSERT (DefaultY < Height);
  ASSERT (Width    <= MAX_INT32);
  ASSERT (Height   <= MAX_INT32);

  Context.MaxX       = Width - 1;
  Context.MaxY       = Height - 1;
  Context.CurState.X = DefaultX;
  Context.CurState.Y = DefaultY;
  Context.RawX       = (INT32) DefaultX;
  Context.RawY       = (INT32) DefaultY;

  Status = OcHandleProtocolFallback (
    gST->ConsoleInHandle,
    &gAppleEventProtocolGuid,
    (VOID **)&Context.AppleEvent
    );
  if (!EFI_ERROR (Status)) {
    if (Context.AppleEvent->Revision >= APPLE_EVENT_PROTOCOL_REVISION) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

      Status = Context.AppleEvent->RegisterHandler (
        APPLE_ALL_MOUSE_EVENTS,
        InternalAppleEventNotification,
        &Context.AppleEventHandle,
        &Context
        );

      Dimension.Horizontal = (INT32) DefaultX;
      Dimension.Vertical   = (INT32) DefaultY;
      Context.AppleEvent->SetCursorPosition (
        &Dimension
        );

      gBS->RestoreTPL (OldTpl);
    } else {
      Status = EFI_UNSUPPORTED;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "OCUI: AppleEvent %u is unsupported - %r\n",
        Context.AppleEvent->Revision,
        Status
        ));
      return NULL;
    }
  }

  Status2 = OcHandleProtocolFallback (
    gST->ConsoleInHandle,
    &gEfiAbsolutePointerProtocolGuid,
    (VOID **)&Context.AbsPointer
    );

  if (EFI_ERROR (Status) && EFI_ERROR (Status2)) {
    return NULL;
  }

  return &Context;
}

VOID
GuiPointerDestruct (
  IN GUI_POINTER_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->AppleEvent != NULL);

  Context->AppleEvent->UnregisterHandler (Context->AppleEventHandle);
  ZeroMem (Context, sizeof (*Context));
}

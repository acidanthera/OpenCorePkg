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
#include <Library/AppleEventLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "../OpenCanopy.h"
#include "../GuiIo.h"

#define ABS_DOUBLE_CLICK_RADIUS  25U
#define IS_POWER_2(x)  (((x) & ((x) - 1)) == 0 && (x) != 0)

#define POINTER_SCALE  1U

struct GUI_POINTER_CONTEXT_ {
  APPLE_EVENT_PROTOCOL          *AppleEvent;
  EFI_ABSOLUTE_POINTER_PROTOCOL *AbsPointer;
  APPLE_EVENT_HANDLE            AppleEventHandle;
  EFI_EVENT                     AbsPollEvent;
  UINT32                        MaxXPlus1;
  UINT32                        MaxYPlus1;
  GUI_PTR_POSITION              CurPos;
  GUI_PTR_POSITION              AbsLastDownPos;
  UINT32                        OldEventExScale;
  BOOLEAN                       AbsPrimaryDown;
  BOOLEAN                       AbsDoubleClick;
  UINT8                         UiScale;
  UINT8                         LockedBy;
  UINT8                         EventQueueHead;
  UINT8                         EventQueueTail;
  GUI_PTR_EVENT                 EventQueue[16];
};

enum {
  PointerUnlocked,
  PointerLockedSimple,
  PointerLockedAbsolute
};

STATIC
VOID
InternalQueuePointerEvent (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  IN     UINT8                Type,
  IN     UINT32               X,
  IN     UINT32               Y
  )
{
  UINT32 Tail;
  //
  // Due to the modulus, wraparounds do not matter. The size of the queue must
  // be a power of two for this to hold.
  //
  STATIC_ASSERT (
    IS_POWER_2 (ARRAY_SIZE (Context->EventQueue)),
    "The pointer event queue must have a power of two length."
    );
    
  Tail = Context->EventQueueTail % ARRAY_SIZE (Context->EventQueue);
  Context->EventQueue[Tail].Type      = Type;
  Context->EventQueue[Tail].Pos.Pos.X = X;
  Context->EventQueue[Tail].Pos.Pos.Y = Y;
  ++Context->EventQueueTail;
}

BOOLEAN
GuiPointerGetEvent (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_PTR_EVENT        *Event
  )
{
  UINT32 Head;

  if (Context->EventQueueHead == Context->EventQueueTail) {
    return FALSE;
  }
  //
  // Due to the modulus, wraparounds do not matter. The size of the queue must
  // be a power of two for this to hold.
  //
  STATIC_ASSERT (
    IS_POWER_2 (ARRAY_SIZE (Context->EventQueue)),
    "The pointer event queue must have a power of two length."
    );

  Head = Context->EventQueueHead % ARRAY_SIZE (Context->EventQueue);
  Event->Type      = Context->EventQueue[Head].Type;
  Event->Pos.Pos.X = Context->EventQueue[Head].Pos.Pos.X;
  Event->Pos.Pos.Y = Context->EventQueue[Head].Pos.Pos.Y;
  ++Context->EventQueueHead;

  return TRUE;
}

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

  Context = NotifyContext;

  ASSERT ((Information->EventType & APPLE_ALL_MOUSE_EVENTS) != 0);
  //
  // Discard simple pointer updates when absolute pointer locked.
  //
  if (Context->LockedBy == PointerLockedAbsolute) {
    return;
  }

  EventType = Information->EventData.PointerEventType;

  if ((EventType & APPLE_EVENT_TYPE_MOUSE_MOVED) != 0) {
    Context->CurPos.Pos.X = (UINT32) Information->PointerPosition.Horizontal;
    Context->CurPos.Pos.Y = (UINT32) Information->PointerPosition.Vertical;
  }

  if ((EventType & APPLE_EVENT_TYPE_LEFT_BUTTON) != 0) {
    if ((EventType & APPLE_EVENT_TYPE_MOUSE_DOWN) != 0) {
      ASSERT (Context->LockedBy == PointerUnlocked);
      InternalQueuePointerEvent (
        Context,
        GuiPointerPrimaryDown,
        (UINT32) Information->PointerPosition.Horizontal,
        (UINT32) Information->PointerPosition.Vertical
        );
      Context->LockedBy = PointerLockedSimple;
    } else if ((EventType & APPLE_EVENT_TYPE_MOUSE_UP) != 0) {
      ASSERT (Context->LockedBy == PointerLockedSimple);
      InternalQueuePointerEvent (
        Context,
        GuiPointerPrimaryUp,
        (UINT32) Information->PointerPosition.Horizontal,
        (UINT32) Information->PointerPosition.Vertical
        );
      Context->LockedBy = PointerUnlocked;
    } else if ((EventType & APPLE_EVENT_TYPE_MOUSE_DOUBLE_CLICK) != 0) {
      InternalQueuePointerEvent (
        Context,
        GuiPointerPrimaryDoubleClick,
        (UINT32) Information->PointerPosition.Horizontal,
        (UINT32) Information->PointerPosition.Vertical
        );
    }
  }
}

STATIC
VOID
EFIAPI
InternalUpdateContextAbsolute (
  IN     EFI_EVENT  Event,
  IN OUT VOID       *NotifyContext
  )
{
  GUI_POINTER_CONTEXT        *Context;
  EFI_STATUS                 Status;
  EFI_ABSOLUTE_POINTER_STATE PointerState;
  UINT64                     NewX;
  UINT64                     NewY;
  GUI_PTR_POSITION           NewPos;

  ASSERT (NotifyContext != NULL);

  Context = NotifyContext;
  ASSERT (Context->AbsPointer != NULL);
  //
  // Discard absolute pointer updates when simple pointer locked.
  //
  if (Context->LockedBy == PointerLockedSimple) {
    return;
  }

  Status = Context->AbsPointer->GetState (Context->AbsPointer, &PointerState);
  if (EFI_ERROR (Status)) {
    return;
  }

  NewX  = PointerState.CurrentX - Context->AbsPointer->Mode->AbsoluteMinX;
  NewX *= Context->MaxXPlus1;
  NewPos.Pos.X = (UINT32) DivU64x32 (
    NewX,
    (UINT32) (Context->AbsPointer->Mode->AbsoluteMaxX - Context->AbsPointer->Mode->AbsoluteMinX)
    );

  NewY  = PointerState.CurrentY - Context->AbsPointer->Mode->AbsoluteMinY;
  NewY *= Context->MaxYPlus1;
  NewPos.Pos.Y = (UINT32) DivU64x32 (
    NewY,
    (UINT32) (Context->AbsPointer->Mode->AbsoluteMaxY - Context->AbsPointer->Mode->AbsoluteMinY)
    );

  Context->CurPos.Uint64 = NewPos.Uint64;
  //
  // Cancel double click when the finger is moved too far away.
  //
  ASSERT (Context->UiScale > 0);
  
  if (Context->AbsDoubleClick) {
    if (ABS ((INT64) NewPos.Pos.X - Context->AbsLastDownPos.Pos.X) > ABS_DOUBLE_CLICK_RADIUS * Context->UiScale
     || ABS ((INT64) NewPos.Pos.Y - Context->AbsLastDownPos.Pos.Y) > ABS_DOUBLE_CLICK_RADIUS * Context->UiScale) {
      Context->AbsDoubleClick = FALSE;
    }
  }

  if (Context->AbsPrimaryDown != ((PointerState.ActiveButtons & EFI_ABSP_TouchActive) != 0)) {
    Context->AbsPrimaryDown = ((PointerState.ActiveButtons & EFI_ABSP_TouchActive) != 0);
    if (Context->AbsPrimaryDown) {
      ASSERT (Context->LockedBy == PointerUnlocked);
      InternalQueuePointerEvent (
        Context,
        GuiPointerPrimaryDown,
        NewPos.Pos.X,
        NewPos.Pos.Y
        );
      Context->LockedBy = PointerLockedAbsolute;

      Context->AbsLastDownPos.Uint64 = NewPos.Uint64;
      Context->AbsDoubleClick = TRUE;
    } else {
      ASSERT (Context->LockedBy == PointerLockedAbsolute);
      InternalQueuePointerEvent (
        Context,
        GuiPointerPrimaryUp,
        NewPos.Pos.X,
        NewPos.Pos.Y
        );
      Context->LockedBy = PointerUnlocked;

      if (Context->AbsDoubleClick) {
        InternalQueuePointerEvent (
          Context,
          GuiPointerPrimaryDoubleClick,
          NewPos.Pos.X,
          NewPos.Pos.Y
          );
        Context->AbsDoubleClick = FALSE;
      }
    }
  }
}

VOID
GuiPointerReset (
  IN OUT GUI_POINTER_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  Context->EventQueueHead = 0;
  Context->EventQueueTail = 0;

  Context->LockedBy = PointerUnlocked;
}

VOID
GuiPointerGetPosition (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_PTR_POSITION     *Position
  )
{
  EFI_TPL OldTpl;

  ASSERT (Context != NULL);
  ASSERT (Position != NULL);
  //
  // Prevent pointer updates during state retrieval.
  // On 64+-bit systems, the operation is atomic.
  //
  if (sizeof (UINTN) < sizeof (UINT64)) {
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  }
  //
  // Return the current pointer position.
  //
  Position->Uint64 = Context->CurPos.Uint64;

  if (sizeof (UINTN) < sizeof (UINT64)) {
    gBS->RestoreTPL (OldTpl);
  }
}

VOID
GuiPointerSetPosition (
  IN OUT GUI_POINTER_CONTEXT     *Context,
  IN     CONST GUI_PTR_POSITION  *Position
  )
{
  EFI_TPL   OldTpl;
  DIMENSION ApplePosition;

  ASSERT (Context != NULL);
  ASSERT (Position != NULL);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  ApplePosition.Horizontal = (INT32) Position->Pos.X;
  ApplePosition.Vertical   = (INT32) Position->Pos.Y;
  Context->AppleEvent->SetCursorPosition (
    &ApplePosition
    );

  //
  // Return the current pointer position.
  //
  Context->CurPos.Uint64 = Position->Uint64;

  gBS->RestoreTPL (OldTpl);
}

GUI_POINTER_CONTEXT *
GuiPointerConstruct (
  IN UINT32  DefaultX,
  IN UINT32  DefaultY,
  IN UINT32  Width,
  IN UINT32  Height,
  IN UINT8   UiScale
  )
{
  // TODO: alloc on the fly?
  STATIC GUI_POINTER_CONTEXT Context;

  EFI_STATUS Status;
  EFI_STATUS Status2;
  DIMENSION  Dimension;

  ASSERT (DefaultX <= MAX_INT32);
  ASSERT (DefaultY <= MAX_INT32);

  Context.MaxXPlus1    = Width;
  Context.MaxYPlus1    = Height;
  Context.CurPos.Pos.X = DefaultX;
  Context.CurPos.Pos.Y = DefaultY;
  Context.UiScale      = UiScale;

  Context.AppleEvent = OcGetProtocol (
    &gAppleEventProtocolGuid,
    DEBUG_WARN,
    "GuiPointerConstruct",
    "AppleEvent"
    );

  Status = EFI_UNSUPPORTED;
  if (Context.AppleEvent != NULL) {
    if (Context.AppleEvent->Revision >= APPLE_EVENT_PROTOCOL_REVISION) {
      Dimension.Horizontal = (INT32) DefaultX;
      Dimension.Vertical   = (INT32) DefaultY;
      Context.AppleEvent->SetCursorPosition (
        &Dimension
        );

      //
      // Do not register 'Click' as its behaviour does not follow OS behaviour.
      //
      Status = Context.AppleEvent->RegisterHandler (
        APPLE_ALL_MOUSE_EVENTS & ~APPLE_EVENT_TYPE_MOUSE_CLICK,
        InternalAppleEventNotification,
        &Context.AppleEventHandle,
        &Context
        );
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
  if (!EFI_ERROR (Status2)) {
    Context.AbsPollEvent = EventLibCreateNotifyTimerEvent (
      InternalUpdateContextAbsolute,
      &Context,
      EFI_TIMER_PERIOD_MILLISECONDS (10),
      TRUE
      );
    if (Context.AbsPollEvent == NULL) {
      Status2 = EFI_UNSUPPORTED;
    }
  }

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

  if (Context->AbsPollEvent != NULL) {
    EventLibCancelEvent (Context->AbsPollEvent);
  }

  ZeroMem (Context, sizeof (*Context));
}

/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/AppleEvent.h>
#include <Protocol/AbsolutePointer.h>
#include <Protocol/SimplePointer.h>

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
  EFI_SIMPLE_POINTER_PROTOCOL   *Pointer;
  EFI_ABSOLUTE_POINTER_PROTOCOL *AbsPointer;
  APPLE_EVENT_HANDLE            AppleEventHandle;
  UINT32                        MaxX;
  UINT32                        MaxY;
  UINT32                        PrevX;
  UINT32                        PrevY;
  UINT32                        PrevRealX;
  UINT32                        PrevRealY;
  UINT32                        X;
  UINT32                        Y;
  UINT8                         LockedBy;
  BOOLEAN                       PrimaryDown;
  BOOLEAN                       SecondaryDown;
};

enum {
  PointerUnlocked,
  PointerLockedSimple,
  PointerLockedAbsolute
};

STATIC
UINT32
InternalClipPointerSimple (
  IN UINT32  OldCoord,
  IN INT64   DeltaCoord,
  IN UINT32  MaxCoord
  )
{
  BOOLEAN Result;
  INT64   NewCoord;

  Result = OcOverflowAddS64 (OldCoord, DeltaCoord, &NewCoord);
  if (!Result) {
    if (NewCoord <= 0) {
      return 0;
    }

    if (NewCoord > MaxCoord) {
      return MaxCoord;
    }

    return (UINT32) NewCoord;
  }

  if (DeltaCoord < 0) {
    return 0;
  }

  return MaxCoord;
}

STATIC
INT64
InternalGetInterpolatedValue (
  IN INT32  Value
  )
{
  INTN    Bit;

  //
  // For now this produces most natural speed.
  //
  STATIC CONST INT8 AccelerationNumbers[] = {2};

  if (Value != 0) {
    Bit = HighBitSet32 (ABS (Value));
    return (INT64) Value * AccelerationNumbers[
      MIN (Bit, (INTN) ARRAY_SIZE (AccelerationNumbers) - 1)
      ];
  }

  return 0;
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

  //
  // Should not happen but just in case.
  //
  if ((Information->EventType & APPLE_ALL_MOUSE_EVENTS) == 0) {
    return;
  }

  EventType = Information->EventData.PointerEventType;

  if ((EventType & APPLE_EVENT_TYPE_MOUSE_MOVED) != 0) {
    Context->X = MIN (
      (UINT32) Information->PointerPosition.Horizontal,
      Context->MaxX
      );
    Context->Y = MIN (
      (UINT32) Information->PointerPosition.Vertical,
      Context->MaxY
      );
  }

  if ((EventType & APPLE_EVENT_TYPE_MOUSE_DOWN) != 0) {
    if ((EventType & APPLE_EVENT_TYPE_LEFT_BUTTON) != 0) {
      Context->PrimaryDown = TRUE;
    } else if ((Information->EventType & APPLE_EVENT_TYPE_RIGHT_BUTTON) != 0) {
      Context->SecondaryDown = TRUE;
    }
  } else if ((EventType & APPLE_EVENT_TYPE_MOUSE_UP) != 0) {
    if ((EventType & APPLE_EVENT_TYPE_LEFT_BUTTON) != 0) {
      Context->PrimaryDown = FALSE;
    } else if ((EventType & APPLE_EVENT_TYPE_RIGHT_BUTTON) != 0) {
      Context->SecondaryDown = FALSE;
    }
  }
}

STATIC
EFI_STATUS
InternalUpdateStateSimpleAppleEvent (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  )
{
  INT64    Difference;
  INT32    PrevX;
  INT32    PrevY;
  EFI_TPL  OldTpl;

  ASSERT (Context->AppleEvent != NULL);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  PrevX = (INT32) Context->PrevX;
  PrevY = (INT32) Context->PrevY;  

  Context->PrevX = Context->X;
  Context->PrevY = Context->Y;

  if (Context->X == 0 || Context->X == Context->MaxX) {
    State->X = Context->X;
  } else {
    Difference = InternalGetInterpolatedValue ((INT32) Context->X - PrevX);
    State->X   = InternalClipPointerSimple (Context->PrevRealX, Difference, Context->MaxX);
  }

  if (Context->Y == 0 || Context->Y == Context->MaxY) {
    State->Y = Context->Y;    
  } else {
    Difference = InternalGetInterpolatedValue ((INT32) Context->Y - PrevY);
    State->Y   = InternalClipPointerSimple (Context->PrevRealY, Difference, Context->MaxY);
  }

  Context->PrevRealX = State->X;
  Context->PrevRealY = State->Y;

  State->PrimaryDown   = Context->PrimaryDown;
  State->SecondaryDown = Context->SecondaryDown;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalUpdateStateSimplePointer (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  )
{
  EFI_STATUS               Status;
  EFI_SIMPLE_POINTER_STATE PointerState;
  INT64                    InterpolatedX;
  INT64                    InterpolatedY;

  ASSERT (Context->Pointer != NULL);

  Status = Context->Pointer->GetState (Context->Pointer, &PointerState);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  InterpolatedX = DivS64x64Remainder (
    InternalGetInterpolatedValue (PointerState.RelativeMovementX),
    Context->Pointer->Mode->ResolutionX,
    NULL
    );
  InterpolatedY = DivS64x64Remainder (
    InternalGetInterpolatedValue (PointerState.RelativeMovementY),
    Context->Pointer->Mode->ResolutionY,
    NULL
    );

  if (InterpolatedX == 0) {
    if (PointerState.RelativeMovementX > 0) {
      InterpolatedX = 1;
    } else if (PointerState.RelativeMovementX < 0) {
      InterpolatedX = -1;
    }
  }

  if (InterpolatedY == 0) {
    if (PointerState.RelativeMovementY > 0) {
      InterpolatedY = 1;
    } else if (PointerState.RelativeMovementY < 0) {
      InterpolatedY = -1;
    }
  }

  Context->X = InternalClipPointerSimple (
    Context->X,
    InterpolatedX,
    Context->MaxX
    );
  State->X = Context->X;

  Context->Y = InternalClipPointerSimple (
    Context->Y,
    InterpolatedY,
    Context->MaxY
    );
  State->Y = Context->Y;

  State->PrimaryDown   = PointerState.LeftButton;
  State->SecondaryDown = PointerState.RightButton;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalUpdateStateSimple (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  )
{
  ASSERT (Context != NULL);
  ASSERT (State != NULL);

  if (Context->AppleEvent != NULL) {
    return InternalUpdateStateSimpleAppleEvent (
      Context,
      State
      );
  }

  if (Context->Pointer != NULL) {
    return InternalUpdateStateSimplePointer (
      Context,
      State
      );
  }

  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
InternalUpdateStateAbsolute (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  )
{
  EFI_STATUS                 Status;
  EFI_ABSOLUTE_POINTER_STATE PointerState;
  UINT64                     NewX;
  UINT64                     NewY;

  ASSERT (Context != NULL);
  ASSERT (State != NULL);

  if (Context->AbsPointer == NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = Context->AbsPointer->GetState (Context->AbsPointer, &PointerState);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NewX  = PointerState.CurrentX - Context->AbsPointer->Mode->AbsoluteMinX;
  NewX *= Context->MaxX + 1;
  NewX  = DivU64x32 (NewX, (UINT32) (Context->AbsPointer->Mode->AbsoluteMaxX - Context->AbsPointer->Mode->AbsoluteMinX));

  NewY  = PointerState.CurrentY - Context->AbsPointer->Mode->AbsoluteMinY;
  NewY *= Context->MaxY + 1;
  NewY  = DivU64x32 (NewY, (UINT32) (Context->AbsPointer->Mode->AbsoluteMaxY - Context->AbsPointer->Mode->AbsoluteMinY));

  State->X = (UINT32)NewX;
  State->Y = (UINT32)NewY;

  State->PrimaryDown   = (PointerState.ActiveButtons & EFI_ABSP_TouchActive) != 0;
  State->SecondaryDown = (PointerState.ActiveButtons & EFI_ABS_AltActive) != 0;
  //
  // Adapt X/Y based on touch position so the cursor will not jump when
  // changing from touch to mouse.
  //
  Context->X = (UINT32)NewX;
  Context->Y = (UINT32)NewY;

  return EFI_SUCCESS;
}

VOID
GuiPointerReset (
  IN OUT GUI_POINTER_CONTEXT  *Context
  )
{
  EFI_SIMPLE_POINTER_STATE   SimpleState;
  EFI_ABSOLUTE_POINTER_STATE AbsoluteState;

  ASSERT (Context != NULL);

  if (Context->Pointer != NULL) {
    Context->Pointer->GetState (Context->Pointer, &SimpleState);
  }

  if (Context->AbsPointer != NULL) {
    Context->AbsPointer->GetState (Context->AbsPointer, &AbsoluteState);
  }

  Context->LockedBy = PointerUnlocked;
}

EFI_STATUS
GuiPointerGetState (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  )
{
  EFI_STATUS Status;
  EFI_STATUS Status2;

  ASSERT (Context != NULL);
  ASSERT (State != NULL);

  Status = EFI_NOT_READY;

  switch (Context->LockedBy) {
    case PointerUnlocked:
    {
      Status = InternalUpdateStateSimple (Context, State);
      if (!EFI_ERROR (Status)
       && (State->PrimaryDown || State->SecondaryDown)) {
        Context->LockedBy = PointerLockedSimple;
        break;
      }

      Status2 = InternalUpdateStateAbsolute (Context, State);
      if (!EFI_ERROR (Status2)) {
        if (State->PrimaryDown || State->SecondaryDown) {
          Context->LockedBy = PointerLockedAbsolute;
        }

        Status = Status2;
        break;
      }

      break;
    }

    case PointerLockedSimple:
    {
      Status = InternalUpdateStateSimple (Context, State);
      if (!EFI_ERROR (Status)
       && !State->PrimaryDown && !State->SecondaryDown) {
        Context->LockedBy = PointerUnlocked;
      }

      break;
    }

    case PointerLockedAbsolute:
    {
      Status = InternalUpdateStateAbsolute (Context, State);
      if (!EFI_ERROR (Status)
       && !State->PrimaryDown && !State->SecondaryDown) {
        Context->LockedBy = PointerUnlocked;
      }

      break;
    }

    default:
    {
      ASSERT (FALSE);
      break;
    }
  }

  return Status;
}

GUI_POINTER_CONTEXT *
GuiPointerConstruct (
  IN OC_PICKER_CONTEXT  *PickerContext,
  IN UINT32             DefaultX,
  IN UINT32             DefaultY,
  IN UINT32             Width,
  IN UINT32             Height
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

  Context.MaxX          = Width - 1;
  Context.MaxY          = Height - 1;
  Context.PrevX         = DefaultX;
  Context.PrevY         = DefaultY;
  Context.PrevRealX     = DefaultX;
  Context.PrevRealY     = DefaultY;
  Context.X             = DefaultX;
  Context.Y             = DefaultY;

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
      Context.AppleEvent = NULL;
    }
  }

  if (EFI_ERROR (Status)) {
    Status = OcHandleProtocolFallback (
      gST->ConsoleInHandle,
      &gEfiSimplePointerProtocolGuid,
      (VOID **)&Context.Pointer
      );
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

  if (Context->AppleEvent != NULL) {
    Context->AppleEvent->UnregisterHandler (
      Context->AppleEventHandle
      );
  }

  ZeroMem (Context, sizeof (*Context));
}

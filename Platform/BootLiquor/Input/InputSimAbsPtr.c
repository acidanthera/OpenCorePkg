/** @file
  This file is part of BootLiquor, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/AbsolutePointer.h>
#include <Protocol/SimplePointer.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../BootLiquor.h"
#include "../GuiIo.h"

struct GUI_POINTER_CONTEXT_ {
  EFI_SIMPLE_POINTER_PROTOCOL   *Pointer;
  EFI_ABSOLUTE_POINTER_PROTOCOL *AbsPointer;
  UINT32                        Width;
  UINT32                        Height;
  UINT64                        SimpleMaxX;
  UINT64                        SimpleMaxY;
  UINT64                        SimpleX;
  UINT64                        SimpleY;
  UINT8                         LockedBy;
};

enum {
  PointerUnlocked,
  PointerLockedSimple,
  PointerLockedAbsolute
};

STATIC
UINT64
InternalClipPointerSimple (
  IN UINT64  OldCoord,
  IN INT32   DeltaCoord,
  IN UINT64  MaxCoord
  )
{
  BOOLEAN Result;
  INT64   NewCoord;

  Result = OcOverflowAddS64 (OldCoord, DeltaCoord, &NewCoord);
  if (!Result) {
    if (NewCoord <= 0) {
      NewCoord = 0;
    } else if ((UINT64)NewCoord > MaxCoord) {
      NewCoord = MaxCoord;
    }
  } else {
    if (DeltaCoord < 0) {
      NewCoord = 0;
    } else {
      NewCoord = MaxCoord;
    }
  }

  return NewCoord;
}

STATIC
INT64
InternalGetInterpolatedValue (
  IN INT64  Value
  )
{
  INTN    Bit;

  STATIC CONST INT8 AccelerationNumbers[] = {1, 2, 3, 4, 5, 6};

  if (Value != 0) {
    Bit = HighBitSet64 (ABS (Value));
    return Value * AccelerationNumbers[
      Bit >= ARRAY_SIZE (AccelerationNumbers) - 1
      ? ARRAY_SIZE (AccelerationNumbers) - 1
      : Bit
      ];
  }

  return 0;
}

STATIC
EFI_STATUS
InternalUpdateStateSimple (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  )
{
  EFI_STATUS               Status;
  EFI_SIMPLE_POINTER_STATE PointerState;
  INT64                    InterpolatedX;
  INT64                    InterpolatedY;

  ASSERT (Context != NULL);
  ASSERT (State != NULL);

  if (Context->Pointer == NULL) {
    return EFI_UNSUPPORTED;
  }

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

  Context->SimpleX = InternalClipPointerSimple (
    Context->SimpleX,
    (INT32) InterpolatedX,
    Context->SimpleMaxX
    );
  State->X = (UINT32) Context->SimpleX;

  Context->SimpleY = InternalClipPointerSimple (
    Context->SimpleY,
    (INT32) InterpolatedY,
    Context->SimpleMaxY
    );
  State->Y = (UINT32) Context->SimpleY;

  State->PrimaryDown   = PointerState.LeftButton;
  State->SecondaryDown = PointerState.RightButton;

  return EFI_SUCCESS;
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
  NewX *= Context->Width;
  NewX  = DivU64x32 (NewX, Context->AbsPointer->Mode->AbsoluteMaxX - Context->AbsPointer->Mode->AbsoluteMinX);

  NewY  = PointerState.CurrentY - Context->AbsPointer->Mode->AbsoluteMinY;
  NewY *= Context->Height;
  NewY  = DivU64x32 (NewX, Context->AbsPointer->Mode->AbsoluteMaxY - Context->AbsPointer->Mode->AbsoluteMinY);

  State->X = (UINT32)NewX;
  State->Y = (UINT32)NewY;

  State->PrimaryDown   = (PointerState.ActiveButtons & EFI_ABSP_TouchActive) != 0;
  State->SecondaryDown = (PointerState.ActiveButtons & EFI_ABS_AltActive) != 0;
  //
  // WORKAROUND: Adapt SimpleX/Y based on touch position so the cursor
  //             will not jump when changing from touch to mouse.
  //
  if (Context->Pointer != NULL) {
    Context->SimpleX = State->X * Context->Pointer->Mode->ResolutionX;
    Context->SimpleY = State->Y * Context->Pointer->Mode->ResolutionY;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalHandleProtocolFallback (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  EFI_STATUS Status;

  Status = gBS->HandleProtocol (
                  Handle,
                  Protocol,
                  Interface
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (
                    Protocol,
                    NULL,
                    Interface
                    );
  }

  return Status;
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

  ASSERT (DefaultX < Width);
  ASSERT (DefaultY < Height);

  Status = InternalHandleProtocolFallback (
             &gST->ConsoleInHandle,
             &gEfiSimplePointerProtocolGuid,
             (VOID **)&Context.Pointer
             );
  Status2 = InternalHandleProtocolFallback (
              &gST->ConsoleInHandle,
              &gEfiAbsolutePointerProtocolGuid,
              (VOID **)&Context.AbsPointer
              );
  if (EFI_ERROR (Status) && EFI_ERROR (Status2)) {
    return NULL;
  }

  if (Context.Pointer != NULL) {
    Context.SimpleX    = DefaultX;
    Context.SimpleY    = DefaultY;
    Context.SimpleMaxX = Width - 1;
    Context.SimpleMaxY = Height - 1;
  }

  Context.Width  = Width;
  Context.Height = Height;

  return &Context;
}

VOID
GuiPointerDestruct (
  IN GUI_POINTER_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
}

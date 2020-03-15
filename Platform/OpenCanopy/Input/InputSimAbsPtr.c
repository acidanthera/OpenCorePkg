/** @file
  This file is part of OpenCanopy, OpenCore GUI.

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

#include "../OpenCanopy.h"
#include "../GuiIo.h"

struct GUI_POINTER_CONTEXT_ {
  EFI_SIMPLE_POINTER_PROTOCOL   *Pointer;
  EFI_ABSOLUTE_POINTER_PROTOCOL *AbsPointer;
  UINT32                        Width;
  UINT32                        Height;
  UINT32                        MaxX;
  UINT32                        MaxY;
  UINT32                        X;
  UINT32                        Y;
  UINT8                         LockedBy;
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
    } else if (NewCoord > MaxCoord) {
      return MaxCoord;
    } else {
      return (UINT32) NewCoord;
    }
  } else {
    if (DeltaCoord < 0) {
      return 0;
    } else {
      return MaxCoord;
    }
  }
}

STATIC
INT64
InternalGetInterpolatedValue (
  IN INT32  Value
  )
{
  INTN    Bit;

  STATIC CONST INT8 AccelerationNumbers[] = {1, 2, 3, 4, 5, 6};

  if (Value != 0) {
    Bit = HighBitSet32 (ABS (Value));
    return (INT64) Value * AccelerationNumbers[
      MIN (Bit, ARRAY_SIZE (AccelerationNumbers) - 1)
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
  NewX  = DivU64x32 (NewX, (UINT32) (Context->AbsPointer->Mode->AbsoluteMaxX - Context->AbsPointer->Mode->AbsoluteMinX));

  NewY  = PointerState.CurrentY - Context->AbsPointer->Mode->AbsoluteMinY;
  NewY *= Context->Height;
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
    Context.X    = DefaultX;
    Context.Y    = DefaultY;
    Context.MaxX = Width - 1;
    Context.MaxY = Height - 1;
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

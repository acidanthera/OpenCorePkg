/** @file

AppleEventDxe

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <AppleMacEfi.h>

#include <Guid/AppleVariable.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/SimplePointer.h>

#include <Library/AppleEventLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/TimerLib.h>

#include "AppleEventInternal.h"

#include <Library/BaseLib.h>

// POINTER_POLL_FREQUENCY
#define POINTER_POLL_FREQUENCY  EFI_TIMER_PERIOD_MILLISECONDS (2)
#define MAX_POINTER_POLL_FREQUENCY  EFI_TIMER_PERIOD_MILLISECONDS (80)

// MAXIMUM_DOUBLE_CLICK_SPEED
/// (EFI_TIMER_PERIOD_MILLISECONDS (748) / POINTER_POLL_FREQUENCY)
#define MAXIMUM_DOUBLE_CLICK_SPEED  374

// MAXIMUM_CLICK_DURATION
/// (EFI_TIMER_PERIOD_MILLISECONDS (148) / POINTER_POLL_FREQUENCY)
#define MAXIMUM_CLICK_DURATION  74

// MINIMAL_MOVEMENT
#define MINIMAL_MOVEMENT  5

// POINTER_BUTTON_INFORMATION
typedef struct {
  APPLE_EVENT_TYPE  EventType;          ///<
  UINTN             NumberOfStrokes;    ///<
  UINTN             Polls;              ///<
  UINTN             PreviousEventType;  ///<
  BOOLEAN           PreviousButton;     ///<
  BOOLEAN           CurrentButton;      ///<
  DIMENSION         PreviousPosition;   ///<
  DIMENSION         Position;           ///<
} POINTER_BUTTON_INFORMATION;

// SIMPLE_POINTER_INSTANCE
typedef struct {
  EFI_HANDLE                  Handle;      ///<
  EFI_SIMPLE_POINTER_PROTOCOL *Interface;  ///<
  BOOLEAN                     Installed;   ///<
} SIMPLE_POINTER_INSTANCE;

// mSimplePointerInstallNotifyEvent
STATIC EFI_EVENT mSimplePointerInstallNotifyEvent = NULL;

// mSimplePointerInstallNotifyRegistration
STATIC VOID *mSimplePointerInstallNotifyRegistration = NULL;

// mSimplePointerInstances
STATIC SIMPLE_POINTER_INSTANCE *mPointerProtocols = NULL;

// mNoSimplePointerInstances
STATIC UINTN mNumberOfPointerProtocols = 0;

// mSimplePointerPollEvent
STATIC EFI_EVENT mSimplePointerPollEvent = NULL;

// mSimplePointerPollTime
STATIC UINT64 mSimplePointerPollTime;

// mUiScale
STATIC UINT8 mUiScale = 1;

// mLeftButtonInfo
STATIC POINTER_BUTTON_INFORMATION mLeftButtonInfo = {
  APPLE_EVENT_TYPE_LEFT_BUTTON,
  0,
  0,
  0,
  FALSE,
  FALSE,
  { 0, 0 },
  { 0, 0 }
};

// mRightButtonInfo
STATIC POINTER_BUTTON_INFORMATION mRightButtonInfo = {
  APPLE_EVENT_TYPE_RIGHT_BUTTON,
  0,
  0,
  0,
  FALSE,
  FALSE,
  { 0, 0 },
  { 0, 0 }
};

// mCursorPosition
STATIC DIMENSION mCursorPosition;

// mMouseMoved
STATIC BOOLEAN mMouseMoved;

// mScreenResolutionSet
STATIC BOOLEAN mScreenResolutionSet;

// mScreenResolution
STATIC DIMENSION mResolution;

// InternalRegisterSimplePointerInterface
STATIC
VOID
InternalRegisterSimplePointerInterface (
  IN EFI_HANDLE                   Handle,
  IN EFI_SIMPLE_POINTER_PROTOCOL  *SimplePointer
  )
{
  SIMPLE_POINTER_INSTANCE *Instance;
  UINTN                   Index;

  DEBUG ((DEBUG_VERBOSE, "InternalRegisterSimplePointerInterface\n"));

  Instance = AllocateZeroPool (
               (mNumberOfPointerProtocols + 1) * sizeof (*Instance)
               );

  if (Instance != NULL) {
    //
    // Apple did not check against NULL here and below as of boot.efi
    //
    if (mPointerProtocols != NULL) {
      CopyMem (
        Instance,
        mPointerProtocols,
        (mNumberOfPointerProtocols * sizeof (*Instance))
        );
    }

    Index = mNumberOfPointerProtocols;

    ++mNumberOfPointerProtocols;

    Instance[Index].Handle    = Handle;
    Instance[Index].Interface = SimplePointer;
    Instance[Index].Installed = TRUE;

    if (mPointerProtocols != NULL) {
      FreePool ((VOID *)mPointerProtocols);
    }

    mPointerProtocols = Instance;
  }
}

// EventSimplePointerDesctructor
VOID
EventSimplePointerDesctructor (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "EventSimplePointerDesctructor\n"));

  if (mPointerProtocols != NULL) {
    FreePool ((VOID *)mPointerProtocols);
    //
    // Apple did not null the pointer here (not an error)
    //
    mPointerProtocols = NULL;
  }
}

// InternalRemoveUninstalledInstances
STATIC
VOID
InternalRemoveUninstalledInstances (
  IN OUT SIMPLE_POINTER_INSTANCE  **InstancesPtr,
  IN     UINTN                    *NumberOfInstances,
  IN     EFI_GUID                 *Protocol
  )
{
  EFI_STATUS              Status;
  UINTN                   NumberHandles;
  EFI_HANDLE              *Buffer;
  SIMPLE_POINTER_INSTANCE *Instance;
  UINTN                   Index;
  UINTN                   Index2;
  SIMPLE_POINTER_INSTANCE *OrgInstances;
  SIMPLE_POINTER_INSTANCE *NewInstances;
  UINTN                   NumberOfMatches;

  DEBUG ((DEBUG_VERBOSE, "InternalRemoveUninstalledInstances\n"));

  OrgInstances    = *InstancesPtr;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  Protocol,
                  NULL,
                  &NumberHandles,
                  &Buffer
                  );

  if (!EFI_ERROR (Status)) {
    if (*NumberOfInstances > 0) {
      NumberOfMatches = 0;

      for (Index = 0; Index < *NumberOfInstances; ++Index) {
        Instance = &OrgInstances[Index];

        if (Instance->Installed) {
          for (Index2 = 0; Index2 < NumberHandles; ++Index2) {
            if (Instance->Handle == Buffer[Index2]) {
              ++NumberOfMatches;
              break;
            }
          }

          if (NumberHandles == Index2) {
            Instance->Installed = FALSE;
          }
        }
      }

      if (NumberOfMatches != *NumberOfInstances) {
        //
        // Apple did not check for NumberOfMatches being > 0.
        //
        NewInstances = NULL;
        if (NumberOfMatches > 0) {
          NewInstances = AllocateZeroPool (
                   NumberOfMatches * sizeof (*NewInstances)
                   );
        }

        if (NewInstances != NULL) {
          Index2 = 0;
          for (Index = 0; Index < *NumberOfInstances; ++Index) {
            Instance = &OrgInstances[Index];
            if (Instance->Installed) {
              CopyMem (
                (VOID *)&NewInstances[Index2],
                (VOID *)Instance, sizeof (*Instance)
                );

              ++Index2;
            }
          }

          FreePool (OrgInstances);

          *InstancesPtr      = NewInstances;
          *NumberOfInstances = NumberOfMatches;
        }
      }
    }

    FreePool (Buffer);
  } else {
    if (OrgInstances != NULL) {
      FreePool (OrgInstances);
    }

    *InstancesPtr      = NULL;
    *NumberOfInstances = 0;
  }
}

// InternalSimplePointerInstallNotifyFunction
STATIC
VOID
EFIAPI
InternalSimplePointerInstallNotifyFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                  Status;
  UINTN                       NumberHandles;
  EFI_HANDLE                  *Buffer;
  UINTN                       Index;
  EFI_SIMPLE_POINTER_PROTOCOL *SimplePointer;

  DEBUG ((DEBUG_VERBOSE, "InternalSimplePointerInstallNotifyFunction\n"));

  if (Event != NULL) {
    Status = gBS->LocateHandleBuffer (
                    ByRegisterNotify,
                    NULL,
                    mSimplePointerInstallNotifyRegistration,
                    &NumberHandles,
                    &Buffer
                    );
  } else {
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiSimplePointerProtocolGuid,
                    NULL,
                    &NumberHandles,
                    &Buffer
                    );
  }

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NumberHandles; ++Index) {
      Status = gBS->HandleProtocol (
                      Buffer[Index],
                      &gEfiSimplePointerProtocolGuid,
                      (VOID **)&SimplePointer
                      );

      if (!EFI_ERROR (Status)) {
        InternalRegisterSimplePointerInterface (Buffer[Index], SimplePointer);
      }
    }

    FreePool ((VOID *)Buffer);
  }
}

// EventCreateSimplePointerInstallNotifyEvent
EFI_STATUS
EventCreateSimplePointerInstallNotifyEvent (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "EventCreateSimplePointerInstallNotifyEvent\n"));

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InternalSimplePointerInstallNotifyFunction,
                  NULL,
                  &mSimplePointerInstallNotifyEvent
                  );

  if (!EFI_ERROR (Status)) {
    Status = gBS->RegisterProtocolNotify (
                    &gEfiSimplePointerProtocolGuid,
                    mSimplePointerInstallNotifyEvent,
                    &mSimplePointerInstallNotifyRegistration
                    );

    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (mSimplePointerInstallNotifyEvent);

      mSimplePointerInstallNotifyEvent = NULL;
    } else {
      InternalSimplePointerInstallNotifyFunction (NULL, NULL);
    }
  }

  return Status;
}

// EventCreateSimplePointerInstallNotifyEvent
VOID
EventCloseSimplePointerInstallNotifyEvent (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "EventCloseSimplePointerInstallNotifyEvent\n"));

  if (mSimplePointerInstallNotifyEvent != NULL) {
    gBS->CloseEvent (mSimplePointerInstallNotifyEvent);
  }
}

// InternalGetScreenResolution
STATIC
EFI_STATUS
InternalGetScreenResolution (
  VOID
  )
{
  EFI_STATUS                           Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL         *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL                *UgaDraw;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  UINT32                               HorizontalResolution;
  UINT32                               VerticalResolution;
  UINT32                               ColorDepth;
  UINT32                               RefreshRate;

  DEBUG ((DEBUG_VERBOSE, "InternalGetScreenResolution\n"));

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput
                  );

  if (Status == EFI_UNSUPPORTED) {
    //
    // Fallback to default resolution.
    //
    mResolution.Horizontal = 800;
    mResolution.Vertical   = 600;

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **)&UgaDraw
                    );

    if (!EFI_ERROR (Status)) {
      Status = UgaDraw->GetMode (
                          UgaDraw,
                          &HorizontalResolution,
                          &VerticalResolution,
                          &ColorDepth,
                          &RefreshRate
                          );
    }

    //
    // Apple does not check for Horizontal/Vertical resolution being > 0 here
    //
    if (!EFI_ERROR (Status) && HorizontalResolution > 0 && VerticalResolution > 0) {
      mResolution.Horizontal = HorizontalResolution;
      mResolution.Vertical   = VerticalResolution;
      //
      // Apple does not set mScreenResolutionSet to true here
      //
      mScreenResolutionSet   = TRUE;
    }

    Status = EFI_SUCCESS;
  } else if (!EFI_ERROR (Status)) {
    Info                   = GraphicsOutput->Mode->Info;
    mResolution.Horizontal = Info->HorizontalResolution;
    mResolution.Vertical   = Info->VerticalResolution;

    //
    // Apple does not check Info->HorizontalResolution being > 0 here
    //
    if (Info->HorizontalResolution > 0 && Info->VerticalResolution > 0) {
      mScreenResolutionSet = TRUE;
    } else {
      Status = EFI_NOT_READY;
    }
  }

  return Status;
}

// InternalGetUiScaleData
STATIC
INT64
InternalGetUiScaleData (
  IN INT64  Movement
  )
{
  INT64 AbsoluteValue;
  INTN  Value;
  INT64 Factor;

  DEBUG ((DEBUG_VERBOSE, "InternalGetUiScaleData\n"));

  AbsoluteValue = ABS(Movement);
  Value         = HighBitSet64 ((UINT64) (AbsoluteValue));
  Factor        = 5;

  if (Value <= 3) {
    Factor = (HighBitSet64 ((UINT64) (AbsoluteValue) + 1));
  }

  return (INT64)(MultS64x64 (
                   (INT64)(INT32)mUiScale,
                   MultS64x64 (Movement, Factor)
                   ));
}

// InternalCreatePointerEventQueueInformation
STATIC
APPLE_EVENT_INFORMATION *
InternalCreatePointerEventQueueInformation (
  IN APPLE_EVENT_TYPE    EventType,
  IN APPLE_MODIFIER_MAP  Modifiers
  )
{
  UINT32           FinalEventType;
  APPLE_EVENT_DATA EventData;

  DEBUG ((DEBUG_VERBOSE, "InternalCreatePointerEventQueueInformation\n"));

  FinalEventType = APPLE_EVENT_TYPE_MOUSE_MOVED;

  if ((EventType & APPLE_EVENT_TYPE_MOUSE_MOVED) == 0) {
    FinalEventType = APPLE_ALL_MOUSE_EVENTS;

    if ((EventType & APPLE_CLICK_MOUSE_EVENTS) != 0) {
      FinalEventType = APPLE_CLICK_MOUSE_EVENTS;
    }
  }

  EventData.PointerEventType = EventType;

  return EventCreateAppleEventQueueInfo (
           EventData,
           FinalEventType,
           &mCursorPosition,
           Modifiers
           );
}

// InternalHandleButtonInteraction
STATIC
VOID
InternalHandleButtonInteraction (
  IN     EFI_STATUS                  PointerStatus,
  IN OUT POINTER_BUTTON_INFORMATION  *Pointer,
  IN     APPLE_MODIFIER_MAP          Modifiers
  )
{
  APPLE_EVENT_INFORMATION *Information;
  INT32                   HorizontalMovement;
  INT32                   VerticalMovement;
  APPLE_EVENT_TYPE        EventType;

  DEBUG ((DEBUG_VERBOSE, "InternalHandleButtonInteraction\n"));

  if (!EFI_ERROR (PointerStatus)) {
    if (!Pointer->PreviousButton) {
      if (Pointer->CurrentButton) {
        Pointer->NumberOfStrokes  = 0;
        Pointer->PreviousPosition = mCursorPosition;

        Information = InternalCreatePointerEventQueueInformation (
                        (Pointer->EventType | APPLE_EVENT_TYPE_MOUSE_DOWN),
                        Modifiers
                        );

        if (Information != NULL) {
          EventAddEventToQueue (Information);
        }
      }
    } else if (!Pointer->CurrentButton) {
      Information = InternalCreatePointerEventQueueInformation (
                      (Pointer->EventType | APPLE_EVENT_TYPE_MOUSE_UP),
                      Modifiers
                      );

      if (Information != NULL) {
        EventAddEventToQueue (Information);
      }

      if (Pointer->NumberOfStrokes <= MAXIMUM_CLICK_DURATION) {
        HorizontalMovement = ABS(Pointer->PreviousPosition.Horizontal - mCursorPosition.Horizontal);
        VerticalMovement   = ABS(Pointer->PreviousPosition.Vertical - mCursorPosition.Vertical);

        if ((HorizontalMovement <= MINIMAL_MOVEMENT)
         && (VerticalMovement <= MINIMAL_MOVEMENT)) {
          EventType = APPLE_EVENT_TYPE_MOUSE_CLICK;

          if ((Pointer->PreviousEventType == APPLE_EVENT_TYPE_MOUSE_CLICK)
           && (Pointer->Polls <= MAXIMUM_DOUBLE_CLICK_SPEED)) {
            HorizontalMovement = ABS(Pointer->Position.Horizontal - mCursorPosition.Horizontal);
            VerticalMovement   = ABS(Pointer->Position.Vertical - mCursorPosition.Vertical);

            if ((HorizontalMovement <= MINIMAL_MOVEMENT)
             && (VerticalMovement <= MINIMAL_MOVEMENT)) {
              EventType = APPLE_EVENT_TYPE_MOUSE_DOUBLE_CLICK;
            }
          }

          Information = InternalCreatePointerEventQueueInformation (
                          (Pointer->EventType | EventType),
                          Modifiers
                          );

          if (Information != NULL) {
            EventAddEventToQueue (Information);
          }

          if (Pointer->PreviousEventType == APPLE_EVENT_TYPE_MOUSE_DOUBLE_CLICK) {
            EventType = ((Pointer->Polls <= MAXIMUM_DOUBLE_CLICK_SPEED)
                            ? APPLE_EVENT_TYPE_MOUSE_DOUBLE_CLICK
                            : APPLE_EVENT_TYPE_MOUSE_CLICK);
          }

          Pointer->PreviousEventType = EventType;
          Pointer->Position          = mCursorPosition;
          Pointer->Polls             = 0;
        }
      }
    }

    Pointer->PreviousButton = Pointer->CurrentButton;
  }

  if (Pointer->PreviousButton && Pointer->CurrentButton) {
    ++Pointer->NumberOfStrokes;
  }

  ++Pointer->Polls;
}

// InternalSimplePointerPollNotifyFunction
STATIC
VOID
EFIAPI
InternalSimplePointerPollNotifyFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  APPLE_MODIFIER_MAP          Modifiers;
  UINTN                       Index;
  SIMPLE_POINTER_INSTANCE     *Instance;
  EFI_SIMPLE_POINTER_PROTOCOL *SimplePointer;
  EFI_STATUS                  Status;
  EFI_STATUS                  CommonStatus;
  EFI_SIMPLE_POINTER_STATE    State;
  INT64                       UiScaleX;
  INT64                       UiScaleY;
  INT64                       MovementY;
  INT64                       MovementX;
  DIMENSION                   NewPosition;
  APPLE_EVENT_INFORMATION     *Information;
  APPLE_EVENT_DATA            EventData;
  UINT64                      StartTime;
  UINT64                      EndTime;

  StartTime = GetPerformanceCounter ();

  DEBUG ((DEBUG_VERBOSE, "InternalSimplePointerPollNotifyFunction\n"));

  Modifiers = InternalGetModifierStrokes ();

  InternalRemoveUninstalledInstances (
    &mPointerProtocols,
    &mNumberOfPointerProtocols,
    &gEfiSimplePointerProtocolGuid
    );

  CommonStatus = EFI_UNSUPPORTED;

  if (mNumberOfPointerProtocols > 0) {
    CommonStatus = EFI_NOT_READY;

    for (Index = 0; Index < mNumberOfPointerProtocols; ++Index) {
      Instance      = &mPointerProtocols[Index];
      SimplePointer = Instance->Interface;
      Status        = SimplePointer->GetState (SimplePointer, &State);

      if (!EFI_ERROR (Status)) {
        UiScaleX = InternalGetUiScaleData ((INT64)State.RelativeMovementX);
        UiScaleY = InternalGetUiScaleData ((INT64)State.RelativeMovementY);

        //
        // Apple did not check for zero resolution here
        //

        if (SimplePointer->Mode->ResolutionX > 0
          && SimplePointer->Mode->ResolutionY > 0) {
          MovementX = DivS64x64Remainder (
                        UiScaleX,
                        (INT64)SimplePointer->Mode->ResolutionX,
                        NULL
                        );

          MovementY = DivS64x64Remainder (
                        UiScaleY,
                        (INT64)SimplePointer->Mode->ResolutionY,
                        NULL
                        );
        } else {
          MovementX = 0;
          MovementY = 0;
        }

        if (MovementX == 0) {
          if (State.RelativeMovementX > 0) {
            MovementX = 1;
          } else if (State.RelativeMovementX < 0) {
            MovementX = -1;
          }
        }

        if (MovementY == 0) {
          if (State.RelativeMovementY > 0) {
            MovementY = 1;
          } else if (State.RelativeMovementY < 0) {
            MovementY = -1;
          }
        }

        NewPosition.Horizontal = (INT32)(
                                  mCursorPosition.Horizontal + MovementX
                                  );

        NewPosition.Vertical   = (INT32)(
                                  mCursorPosition.Vertical + MovementY
                                  );

        if (NewPosition.Horizontal > mResolution.Horizontal) {
          NewPosition.Horizontal = mResolution.Horizontal;
        } else if (NewPosition.Horizontal < 0) {
          NewPosition.Horizontal = 0;
        }

        if (NewPosition.Vertical > mResolution.Vertical) {
          NewPosition.Vertical = mResolution.Vertical;
        } else if (NewPosition.Vertical < 0) {
          NewPosition.Vertical = 0;
        }

        if ((mCursorPosition.Horizontal != NewPosition.Horizontal)
         || (mCursorPosition.Vertical != NewPosition.Vertical)) {
          CopyMem (
            &mCursorPosition,
            &NewPosition,
            sizeof (mCursorPosition)
            );

          mMouseMoved = TRUE;
        }

        mLeftButtonInfo.PreviousButton  = mLeftButtonInfo.CurrentButton;
        mLeftButtonInfo.CurrentButton   = State.LeftButton;
        mRightButtonInfo.PreviousButton = mLeftButtonInfo.CurrentButton;
        mRightButtonInfo.CurrentButton  = State.RightButton;
        CommonStatus                    = Status;
      }
    }

    InternalHandleButtonInteraction (CommonStatus, &mLeftButtonInfo, Modifiers);
    InternalHandleButtonInteraction (CommonStatus, &mRightButtonInfo, Modifiers);
  }

  if (EFI_ERROR (CommonStatus)) {
    if (CommonStatus == EFI_UNSUPPORTED) {
      EventLibCancelEvent (mSimplePointerPollEvent);

      mSimplePointerPollEvent = NULL;
    }
  } else if (mMouseMoved != FALSE) {
    mMouseMoved = FALSE;

    EventData.PointerEventType = APPLE_EVENT_TYPE_MOUSE_MOVED;
    Information                = EventCreateAppleEventQueueInfo (
                                   EventData,
                                   APPLE_EVENT_TYPE_MOUSE_MOVED,
                                   &mCursorPosition,
                                   Modifiers
                                   );

    if (Information != NULL) {
      EventAddEventToQueue (Information);
    }
  }

  //
  // This code is here to workaround very slow mouse polling performance on some computers,
  // like most of Dell laptops (one of the worst examples is Dell Latitude 3330 with ~50 ms).
  // Even if we try all the hacks we could make this code approximately only twice faster,
  // which is still far from enough. The event system on these laptops is pretty broken,
  // and even adding gBS->CheckEvent prior to GetState almost does not reduce the time spent.
  //
  if (mSimplePointerPollEvent != NULL && mSimplePointerPollTime < MAX_POINTER_POLL_FREQUENCY) {
    EndTime = GetPerformanceCounter ();
    if (StartTime > EndTime) {
      EndTime = StartTime;
    }
    EndTime = GetTimeInNanoSecond (EndTime - StartTime);
    // Maximum time allowed in this function is half the interval plus some margin (0.55 * 100ns)
    if (EndTime > mSimplePointerPollTime * 55ULL) {
      mSimplePointerPollTime = DivU64x32 (EndTime, 50);
      if (mSimplePointerPollTime > MAX_POINTER_POLL_FREQUENCY) {
        mSimplePointerPollTime = MAX_POINTER_POLL_FREQUENCY;
      }
      gBS->SetTimer (mSimplePointerPollEvent, TimerPeriodic, mSimplePointerPollTime);
    }
  }
}

// EventCreateSimplePointerPollEvent
EFI_STATUS
EventCreateSimplePointerPollEvent (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  UINTN       Index;

  DEBUG ((DEBUG_VERBOSE, "EventCreateSimplePointerPollEvent\n"));

  DataSize = sizeof (mUiScale);

  Status = gRT->GetVariable (
    APPLE_UI_SCALE_VARIABLE_NAME,
    &gAppleVendorVariableGuid,
    NULL,
    &DataSize,
    (VOID *) &mUiScale
    );

  if (EFI_ERROR (Status) || mUiScale != 2) {
    mUiScale = 1;
  }

  InternalRemoveUninstalledInstances (
    &mPointerProtocols,
    &mNumberOfPointerProtocols,
    &gEfiSimplePointerProtocolGuid
    );

  for (Index = 0; Index < mNumberOfPointerProtocols; ++Index) {
    mPointerProtocols[Index].Interface->Reset (
      mPointerProtocols[Index].Interface, FALSE
      );
  }

  InternalGetScreenResolution ();
  ZeroMem (&mCursorPosition, sizeof (mCursorPosition));

  mSimplePointerPollTime = POINTER_POLL_FREQUENCY;
  mSimplePointerPollEvent = EventLibCreateNotifyTimerEvent (
                              InternalSimplePointerPollNotifyFunction,
                              NULL,
                              mSimplePointerPollTime,
                              TRUE
                              );

  Status = EFI_OUT_OF_RESOURCES;

  if (mSimplePointerPollEvent != NULL) {
    mRightButtonInfo.CurrentButton  = FALSE;
    mLeftButtonInfo.CurrentButton   = FALSE;
    mRightButtonInfo.PreviousButton = FALSE;
    mLeftButtonInfo.PreviousButton  = FALSE;

    Status = EFI_SUCCESS;
  }

  return Status;
}

// EventCancelSimplePointerPollEvent
VOID
EventCancelSimplePointerPollEvent (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "EventCancelSimplePointerPollEvent\n"));

  EventLibCancelEvent (mSimplePointerPollEvent);

  mSimplePointerPollEvent = NULL;
}

// EventSetCursorPositionImpl
EFI_STATUS
EventSetCursorPositionImpl (
  IN DIMENSION  *Position
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "EventSetCursorPositionImpl\n"));

  if (!mScreenResolutionSet) {
    Status = InternalGetScreenResolution ();
  }

  if (mScreenResolutionSet) {
    Status = EFI_INVALID_PARAMETER;

    //
    // Apple did not check for negatives here.
    //

    if (Position->Horizontal >= 0 && Position->Vertical >= 0
     && (Position->Horizontal < mResolution.Horizontal)
     && (Position->Vertical < mResolution.Vertical)) {
      mCursorPosition.Horizontal = Position->Horizontal;
      mCursorPosition.Vertical   = Position->Vertical;
      Status = EFI_SUCCESS;
    }
  } else if (EFI_ERROR (Status)) {
    Status = EFI_NOT_READY;
  }

  return Status;
}

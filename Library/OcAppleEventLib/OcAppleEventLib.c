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

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "AppleEventInternal.h"

// APPLE_EVENT_HANDLE_PRIVATE_SIGNATURE
#define APPLE_EVENT_HANDLE_PRIVATE_SIGNATURE  \
  SIGNATURE_32 ('A', 'L', 's', 't')

// APPLE_EVENT_HANDLE_PRIVATE_FROM_LIST_ENTRY
#define APPLE_EVENT_HANDLE_PRIVATE_FROM_LIST_ENTRY(Handle)  \
  CR (                                                      \
    (Handle),                                               \
    APPLE_EVENT_HANDLE_PRIVATE,                             \
    Link,                                                   \
    APPLE_EVENT_HANDLE_PRIVATE_SIGNATURE                    \
    )

// APPLE_EVENT_HANDLE_PRIVATE
typedef struct {
  UINT32                      Signature;       ///<
  LIST_ENTRY                  Link;            ///<
  BOOLEAN                     Ready;           ///<
  BOOLEAN                     Registered;      ///<
  APPLE_EVENT_TYPE            EventType;       ///<
  APPLE_EVENT_NOTIFY_FUNCTION NotifyFunction;  ///<
  VOID                        *NotifyContext;  ///<
  CHAR8                       *Name;           ///<
} APPLE_EVENT_HANDLE_PRIVATE;

// mEventHandles
STATIC
LIST_ENTRY mEventHandles = INITIALIZE_LIST_HEAD_VARIABLE (mEventHandles);

// mNumberOfEventHandles
STATIC UINTN mNumberOfEventHandles = 0;

// EventLibCreateTimerEvent
EFI_EVENT
EventLibCreateTimerEvent (
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN VOID              *NotifyContext,
  IN UINT64            TriggerTime,
  IN BOOLEAN           SignalPeriodic,
  IN EFI_TPL           NotifyTpl
  )
{
  EFI_EVENT  Event;
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "EventLibCreateTimerEvent\n"));

  Event = NULL;

  if (NotifyTpl >= TPL_CALLBACK) {
    Status = gBS->CreateEvent (
                    ((NotifyFunction != NULL)
                      ? (EVT_TIMER | EVT_NOTIFY_SIGNAL)
                      : EVT_TIMER),
                    NotifyTpl,
                    NotifyFunction,
                    NotifyContext,
                    &Event
                    );

    if (!EFI_ERROR (Status)) {
      Status = gBS->SetTimer (
                      Event,
                      (SignalPeriodic ? TimerPeriodic : TimerRelative),
                      TriggerTime
                      );

      if (EFI_ERROR (Status)) {
        gBS->CloseEvent (Event);

        Event = NULL;
      }
    }
  }

  return Event;
}

// EventLibCreateNotifyTimerEvent
EFI_EVENT
EventLibCreateNotifyTimerEvent (
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN VOID              *NotifyContext,
  IN UINT64            TriggerTime,
  IN BOOLEAN           SignalPeriodic
  )
{
  DEBUG ((DEBUG_VERBOSE, "EventLibCreateNotifyTimerEvent\n"));

  return EventLibCreateTimerEvent (
           NotifyFunction,
           NotifyContext,
           TriggerTime,
           SignalPeriodic,
           TPL_CALLBACK
           );
}

// EventLibCancelEvent
VOID
EventLibCancelEvent (
  IN EFI_EVENT  Event
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "EventLibCancelEvent\n"));

  Status = gBS->SetTimer (Event, TimerCancel, 0);

  if (!EFI_ERROR (Status)) {
    gBS->CloseEvent (Event);
  }
}

// EventSignalEvents
VOID
EventSignalEvents (
  IN APPLE_EVENT_INFORMATION  *EventInformation
  )
{
  LIST_ENTRY                 *EventHandleEntry;
  APPLE_EVENT_HANDLE_PRIVATE *EventHandle;

  DEBUG ((DEBUG_VERBOSE, "EventSignalEvents\n"));

  EventHandleEntry = GetFirstNode (&mEventHandles);

  while (!IsNull (&mEventHandles, EventHandleEntry)) {
    EventHandle = APPLE_EVENT_HANDLE_PRIVATE_FROM_LIST_ENTRY (
                    EventHandleEntry
                    );

    if (EventHandle->Registered
      && EventHandle->Ready
      && ((EventInformation->EventType & EventHandle->EventType) != 0)
      && (EventHandle->NotifyFunction != NULL)) {
      EventHandle->NotifyFunction (
                      EventInformation,
                      EventHandle->NotifyContext
                      );
    }

    EventHandleEntry = GetNextNode (&mEventHandles, EventHandleEntry);
  }
}

// InternalFlagAllEventsReady
VOID
InternalFlagAllEventsReady (
  VOID
  )
{
  LIST_ENTRY                 *EventHandleEntry;
  APPLE_EVENT_HANDLE_PRIVATE *EventHandle;

  DEBUG ((DEBUG_VERBOSE, "InternalFlagAllEventsReady\n"));

  EventHandleEntry = GetFirstNode (&mEventHandles);

  if (!IsListEmpty (&mEventHandles)) {
    do {
      EventHandle = APPLE_EVENT_HANDLE_PRIVATE_FROM_LIST_ENTRY (
                      EventHandleEntry
                      );

      EventHandle->Ready = TRUE;

      EventHandleEntry = GetNextNode (&mEventHandles, EventHandleEntry);
    } while (!IsNull (&mEventHandles, EventHandleEntry));
  }
}

// InternalSignalEvents
VOID
InternalSignalEvents (
  IN APPLE_EVENT_INFORMATION  *Information
  )
{
  LIST_ENTRY                 *EventHandleEntry;
  APPLE_EVENT_HANDLE_PRIVATE *EventHandle;

  DEBUG ((DEBUG_VERBOSE, "InternalSignalEvents\n"));

  EventHandleEntry = GetFirstNode (&mEventHandles);

  if (!IsListEmpty (&mEventHandles)) {
    do {
      EventHandle = APPLE_EVENT_HANDLE_PRIVATE_FROM_LIST_ENTRY (
                      EventHandleEntry
                      );

      if (EventHandle->Registered && EventHandle->Ready
        && (EventHandle->EventType & Information->EventType)
        && EventHandle->NotifyFunction != NULL) {
        EventHandle->NotifyFunction (
          Information,
          EventHandle->NotifyContext
          );
      }

      EventHandleEntry = GetNextNode (&mEventHandles, EventHandleEntry);
    } while (!IsNull (&mEventHandles, EventHandleEntry));
  }
}

// InternalRemoveUnregisteredEvents
VOID
InternalRemoveUnregisteredEvents (
  VOID
  )
{
  LIST_ENTRY                 *EventHandleEntry;
  LIST_ENTRY                 *NextEventHandleEntry;
  APPLE_EVENT_HANDLE_PRIVATE *EventHandle;

  DEBUG ((DEBUG_VERBOSE, "InternalRemoveUnregisteredEvents\n"));

  EventHandleEntry = GetFirstNode (&mEventHandles);

  if (!IsListEmpty (&mEventHandles)) {
    do {
      NextEventHandleEntry = GetNextNode (&mEventHandles, EventHandleEntry);

      EventHandle = APPLE_EVENT_HANDLE_PRIVATE_FROM_LIST_ENTRY (
                      EventHandleEntry
                      );

      if (!EventHandle->Registered) {
        if (EventHandle->Name != NULL) {
          FreePool ((VOID *)EventHandle->Name);
        }

        RemoveEntryList (&EventHandle->Link);
        FreePool ((VOID *)EventHandle);
      }

      EventHandleEntry = NextEventHandleEntry;
    } while (!IsNull (&mEventHandles, NextEventHandleEntry));
  }
}

// InternalCreatePollEvents
STATIC
EFI_STATUS
InternalCreatePollEvents (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "InternalCreatePollEvents\n"));

  Status = EventCreateSimplePointerPollEvent ();

  if (!EFI_ERROR (Status)) {
    Status = EventCreateKeyStrokePollEvent ();

    if (EFI_ERROR (Status)) {
      EventCancelSimplePointerPollEvent ();
    }
  }

  return Status;
}

// InternalCancelPollEvents
VOID
InternalCancelPollEvents (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "InternalCancelPollEvents\n"));

  EventCancelSimplePointerPollEvent ();
  EventCancelKeyStrokePollEvent ();
}

// EventRegisterHandler
EFI_STATUS
EFIAPI
EventRegisterHandler (
  IN  APPLE_EVENT_TYPE             Type,
  IN  APPLE_EVENT_NOTIFY_FUNCTION  NotifyFunction,
  OUT APPLE_EVENT_HANDLE           *Handle,
  IN  VOID                         *NotifyContext
  )
{
  EFI_STATUS                 Status;
  APPLE_EVENT_HANDLE_PRIVATE *EventHandle;

  DEBUG ((DEBUG_VERBOSE, "EventRegisterHandler\n"));

  Status = EFI_INVALID_PARAMETER;

  if ((Handle != NULL)
   && (NotifyFunction != NULL)
   && (Type != APPLE_EVENT_TYPE_NONE)) {
    *Handle = NULL;

    InternalRemoveUnregisteredEvents ();

    Status = EFI_SUCCESS;

    if (mNumberOfEventHandles == 0) {
      Status = InternalCreatePollEvents ();

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }

    EventHandle = AllocatePool (sizeof (*EventHandle));

    Status = EFI_OUT_OF_RESOURCES;

    if (EventHandle != NULL) {
      EventHandle->Signature      = APPLE_EVENT_HANDLE_PRIVATE_SIGNATURE;
      EventHandle->Ready          = FALSE;
      EventHandle->Registered     = TRUE;
      EventHandle->EventType      = Type;
      EventHandle->NotifyFunction = NotifyFunction;
      EventHandle->NotifyContext  = NotifyContext;
      EventHandle->Name           = NULL;

      ++mNumberOfEventHandles;

      InsertTailList (&mEventHandles, &EventHandle->Link);

      *Handle = EventHandle;

      Status = EFI_SUCCESS;
    }
  }

Done:
  return Status;
}

// EventUnregisterHandler
EFI_STATUS
EFIAPI
EventUnregisterHandler (
  IN APPLE_EVENT_HANDLE  Handle
  )
{
  EFI_STATUS                 Status;
  LIST_ENTRY                 *EventHandleEntry;
  APPLE_EVENT_HANDLE_PRIVATE *EventHandle;

  DEBUG ((DEBUG_VERBOSE, "EventUnregisterHandler\n"));

  Status = EFI_INVALID_PARAMETER;

  EventHandleEntry = GetFirstNode (&mEventHandles);

  while (!IsNull (&mEventHandles, EventHandleEntry)) {
    EventHandle = APPLE_EVENT_HANDLE_PRIVATE_FROM_LIST_ENTRY (
                    EventHandleEntry
                    );

    if (((UINTN)EventHandle == (UINTN)Handle)
      || ((UINTN)Handle == (UINTN)-1)) {
      EventHandle->Registered = FALSE;
      --mNumberOfEventHandles;

      Status = EFI_SUCCESS;

      if ((UINTN)Handle != (UINTN)-1) {
        break;
      }
    }

    EventHandleEntry = GetNextNode (&mEventHandles, EventHandleEntry);
  }

  if (mNumberOfEventHandles == 0) {
    InternalCancelPollEvents ();
  }

  return Status;
}

// EventSetCursorPosition
/** This function is used to change the position of the cursor on the screen.

  @param[in] Position  The position where to position the cursor.

  @retval EFI_INVALID_PARAMETER  Position is invalid.
**/
EFI_STATUS
EFIAPI
EventSetCursorPosition (
  IN DIMENSION  *Position
  )
{
  DEBUG ((DEBUG_VERBOSE, "EventSetCursorPosition\n"));

  return EventSetCursorPositionImpl (Position);
}

// EventSetEventName
/** This function is used to assign a name to an event.

  @param[in,out]  Handle
  @param[in]      Name

  @retval EFI_SUCCESS            The event name was assigned successfully.
  @retval EFI_INVALID_PARAMETER  EventHandle or EventName is NULL.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to allocate the
                                 event name.
**/
EFI_STATUS
EFIAPI
EventSetEventName (
  IN OUT APPLE_EVENT_HANDLE  Handle,
  IN     CHAR8               *Name
  )
{
  EFI_STATUS Status;
  UINTN      AllocationSize;
  CHAR8      *EventName;

  DEBUG ((DEBUG_VERBOSE, "EventSetEventName\n"));

  Status = EFI_INVALID_PARAMETER;

  if ((Handle != NULL) && (Name != NULL)) {
    AllocationSize = AsciiStrSize (Name);
    EventName      = AllocateZeroPool (AllocationSize);

    ((APPLE_EVENT_HANDLE_PRIVATE *)Handle)->Name = EventName;

    Status = EFI_OUT_OF_RESOURCES;

    if (EventName != NULL) {
      AsciiStrCpyS (EventName, AllocationSize, Name);
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

// EventIsCapsLockOnImpl
/** Retrieves the state of the CapsLock key.

  @param[in,out]  CLockOn  This parameter indicates the state of the CapsLock
                           key.

  @retval EFI_SUCCESS            The CapsLock state was successfully returned
                                 in CLockOn.
  @retval EFI_INVALID_PARAMETER  CLockOn is NULL.
**/
EFI_STATUS
EFIAPI
EventIsCapsLockOn (
  IN OUT BOOLEAN  *CLockOn
  )
{
  DEBUG ((DEBUG_VERBOSE, "EventIsCapsLockOn\n"));

  return EventIsCapsLockOnImpl (CLockOn);
}

// InternalUnregisterHandlers
STATIC
VOID
InternalUnregisterHandlers (
  VOID
  )
{
  DEBUG ((DEBUG_VERBOSE, "InternalUnregisterHandlers\n"));

  EventSimplePointerDesctructor ();

  InternalRemoveUnregisteredEvents ();

  if (!IsListEmpty (&mEventHandles)) {
    EventUnregisterHandler ((APPLE_EVENT_HANDLE_PRIVATE *)((UINTN)-1));
  }
}

// mAppleEventProtocol
STATIC APPLE_EVENT_PROTOCOL mAppleEventProtocol = {
  APPLE_EVENT_PROTOCOL_REVISION,
  EventRegisterHandler,
  EventUnregisterHandler,
  EventSetCursorPosition,
  EventSetEventName,
  EventIsCapsLockOn
};

// AppleEventUnload
EFI_STATUS
EFIAPI
AppleEventUnload (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "AppleEventUnload\n"));

  EventCloseSimplePointerInstallNotifyEvent ();

  InternalSignalAndCloseQueueEvent ();
  InternalUnregisterHandlers ();
  InternalCancelPollEvents ();

  Status = gBS->UninstallProtocolInterface (
                  gImageHandle,
                  &gAppleEventProtocolGuid,
                  (VOID *)&mAppleEventProtocol
                  );

  return Status;
}

/**
  Install and initialise Apple Event protocol.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_EVENT_PROTOCOL *
OcAppleEventInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS           Status;
  APPLE_EVENT_PROTOCOL *Protocol;

  DEBUG ((DEBUG_VERBOSE, "OcAppleEventInstallProtocol\n"));

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleEventProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCAE: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleEventProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  //
  // Apple code supports unloading, ours does not.
  //
  Status      = gBS->InstallProtocolInterface (
                       &gImageHandle,
                       &gAppleEventProtocolGuid,
                       EFI_NATIVE_INTERFACE,
                       (VOID *)&mAppleEventProtocol
                       );

  if (!EFI_ERROR (Status)) {
    InternalCreateQueueEvent ();

    Status = EventCreateSimplePointerInstallNotifyEvent ();
  }

  if (EFI_ERROR (Status)) {
    AppleEventUnload ();
    return NULL;
  }

  return &mAppleEventProtocol;
}

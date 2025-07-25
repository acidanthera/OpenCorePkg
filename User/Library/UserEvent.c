/** @file
  Copyright (c) 2025, Pavel Naberezhnev. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserEvent.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef __MACH__
  #include <mach/clock.h>
  #include <mach/mach.h>
#else
  #include <time.h>
#endif

// Event Descriptor
struct USER_EVENT {
  // It is unused descriptor?
  BOOLEAN             IsClosed;
  // Event is signaled?
  BOOLEAN             Signaled;

  // Event type
  UINT32              Type;
  // Event TPL Level
  EFI_TPL             TplLevel;

  // Callback function
  EFI_EVENT_NOTIFY    NotifyFn;
  // Callback context
  VOID                *NotifyCtx;

  // Timer trigger time
  UINT64              TimerEmit;
  // Timer trigger delta
  UINT64              TimerDelta;
};

// for once init event subsystem
static BOOLEAN  event_need_init = 1;
// current TPL level
static EFI_TPL  CurTPL = 0;
// event descriptor's array
static struct USER_EVENT  events[USER_EVENT_MAXNUM];

STATIC
UINT64
UserEventGetTimeNow (
  VOID
  )
{
  UINT64  TimeNow = 0;

 #if defined (_WIN32)

  time_t  TimeRes = time (NULL);

  if (TimeRes < 0) {
    return TimeNow;
  }

  TimeNow = (UINT64)TimeRes;
  TimeNow = TimeNow*1000000000;

 #elif defined (__MACH__)
  clock_serv_t     cclock;
  mach_timespec_t  now;

  host_get_clock_service (mach_host_self (), SYSTEM_CLOCK, &cclock);
  clock_get_time (cclock, &now);
  mach_port_deallocate (mach_task_self (), cclock);

  TimeNow = (UINT64)now.tv_sec*1000000000 + now.tv_nsec;

 #else
  struct timespec  now;

  clock_gettime (CLOCK_MONOTONIC, &now);
  TimeNow = (UINT64)now.tv_sec*1000000000 + now.tv_nsec;

 #endif

  return TimeNow;
}

// event allocator
STATIC
struct USER_EVENT *
UserEventAlloc (
  VOID
  )
{
  struct USER_EVENT  *res = NULL;

  for (INTN I = 0; I < USER_EVENT_MAXNUM; I++) {
    if (events[I].IsClosed) {
      events[I].IsClosed = 0;
      res                = &(events[I]);
      break;
    }
  }

  return res;
}

// internal event dispatcher
STATIC
VOID
UserEventDispatch (
  VOID
  )
{
  UINT64  TimeNow;

  TimeNow = UserEventGetTimeNow ();

  for (INTN I = 0; I < USER_EVENT_MAXNUM; I++) {
    if (events[I].IsClosed) {
      // skip unused decriptors
      continue;
    }

    // it is an active timer?
    if ((events[I].Type & EVT_TIMER) && events[I].TimerEmit) {
      if (TimeNow >= events[I].TimerEmit) {
        // set signaled!
        events[I].Signaled = 1;

        // it is an periodic?
        if (events[I].TimerDelta) {
          // set new time for trigger
          events[I].TimerEmit += events[I].TimerDelta;
        } else {
          events[I].TimerEmit = 0;
        }
      }
    }

    // signal?
    if (events[I].Signaled && (events[I].Type & EVT_NOTIFY_SIGNAL)) {
      // valid TPL?
      if (CurTPL <= events[I].TplLevel) {
        // clear state
        events[I].Signaled = 0;
        // emit handler
        events[I].NotifyFn ((EFI_EVENT)&events[I], events[I].NotifyCtx);
      }
    }
  }
}

// init event subsystem
STATIC
VOID
UserEventInitializer (
  VOID
  )
{
  for (INTN I = 0; I < USER_EVENT_MAXNUM; I++) {
    // set unused state
    events[I].IsClosed  = 1;
    events[I].TplLevel  = 0;
    events[I].TimerEmit = 0;
  }

  // init current TPL level
  CurTPL = TPL_APPLICATION;
}

// wrap around pthread_once() call
STATIC
VOID
UserEventInit (
  VOID
  )
{
  if (event_need_init) {
    event_need_init = 0;
    UserEventInitializer ();
  }
}

/**
  Creates an event in a group.

  @param[in]  Type                The type of event to create and its mode and attributes.
  @param[in]  NotifyTpl           The task priority level of event notifiactions, if needed.
  @param[in]  NotifyFunction      Pointer to the event's notification function, if any.
  @param[in]  NotifyContext       Pointer to the notification function's context; corresponds to
                                  to parameter Context in the notificatuin function.
  @param[in]  EventGroup          Pointer to the unique identifier of the group to which this
                                  event belongs. Maybe NULL.
  @param[out] Event               Pointer to the newly created event if the call succeeds.

  @retval EFI_SUCCESS             The event structure was created.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_INVALID_PARAMETER   Event is NULL.
  @retval EFI_INVALID_PARAMETER   Type has an unsupported bit set.
  @retval EFI_INVALID_PARAMETER   Type has both EVT_NOTIFY_SIGNAL and EVT_NOTIFY_WAIT set.
  @retval EFI_INVALID_PARAMETER   Type has either EVT_NOTIFY_SIGNAL or EVT_NOTIFY_WAIT set and
                                  NotifyFunction is NULL.
  @retval EFI_INVALID_PARAMETER   Type has either EVT_NOTIFY_SIGNAL or EVT_NOTIFY_WAIT set and
                                  NotifyTpl is not a supported TPL level.
  @retval EFI_OUT_OF_RESOURCES    The event could not be allocated.
**/
EFI_STATUS
EFIAPI
UserCreateEventEx (
  IN UINT32            Type,
  IN EFI_TPL           NotifyTpl,
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN CONST VOID        *NotifyContext,
  IN CONST EFI_GUID    *EventGroup,
  OUT EFI_EVENT        *Event
  )
{
  struct USER_EVENT  *ev = NULL;

  // init event subsystem
  UserEventInit ();

  // check input params
  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Type & EVT_NOTIFY_SIGNAL) && (Type & EVT_NOTIFY_WAIT)) {
    // The Type param can't combine EVT_NOTIFY_SIGNAL and EVT_NOTIFY_WAIT flags
    return EFI_INVALID_PARAMETER;
  }

  if (Type & (EVT_NOTIFY_SIGNAL | EVT_NOTIFY_WAIT)) {
    // is signal or wait emit type
    if (NotifyFunction == NULL) {
      // callback function isn't set
      return EFI_INVALID_PARAMETER;
    }

    if (!(NotifyTpl < USER_EVENT_MAXTPL)) {
      // invalid required TPL
      return EFI_INVALID_PARAMETER;
    }
  }

  // try alloc event descriptor
  ev = UserEventAlloc ();

  if (ev == NULL) {
    // have no mem
    *Event = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  // set event descriptor fields
  ev->Type      = Type;
  ev->TplLevel  = NotifyTpl;
  ev->Signaled  = 0;
  ev->NotifyFn  = NotifyFunction;
  ev->NotifyCtx = (VOID *)NotifyContext;
  ev->TimerEmit = 0;

  *Event = (EFI_EVENT)ev;
  return EFI_SUCCESS;
}

/**
  Creates an event.

  @param[in]  Type                The type of event to create and its mode and attributes.
  @param[in]  NotifyTpl           The task priority level of event notifications, if needed.
  @param[in]  NotifyFunction      Pointer to the event's notification function, if any.
  @param[in]  NotifyContext       Pointer to the notification function's context; corresponds to
                                  parameter Context in the notification function.
  @param[out] Event               Pointer to the newly created event if the call succeeds.

  @retval EFI_SUCCESS             The event structure was created.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_INVALID_PARAMETER   Event is NULL.
  @retval EFI_INVALID_PARAMETER   Type has an unsupported bit set.
  @retval EFI_INVALID_PARAMETER   Type has both EVT_NOTIFY_SINGAL and EVT_NOTIFY_WAIT set.
  @retval EFI_INVALID_PARAMETER   Type has either EVT_NOTIFY_SIGNAL or EVT_NOTIFY_WAIT set and
                                  NotifyFunction is NULL.
  @retval EFI_INVALID_PARAMETER   Type has either EVT_NOTIFY_SIGNAL or EVT_NOTIFY_WAIT set and
                                  NotifyTpl is not a supported TPL level.
  @retval EFI_OUT_OF_RESOURCES    The event could not be allocated.
**/
EFI_STATUS
EFIAPI
UserCreateEvent (
  IN UINT32            Type,
  IN EFI_TPL           NotifyTpl,
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN VOID              *NotifyContext,
  OUT EFI_EVENT        *Event
  )
{
  return UserCreateEventEx (
           Type,
           NotifyTpl,
           NotifyFunction,
           NotifyContext,
           NULL,
           Event
           );
}

/**
  Closes an event.

  @param[in]  Event     The event to close.

  @retval EFI_SUCCESS   The event has been closed.
**/
EFI_STATUS
EFIAPI
UserCloseEvent (
  IN EFI_EVENT  Event
  )
{
  struct USER_EVENT  *ev = NULL;

  // init event subsystem
  UserEventInit ();

  ASSERT (Event != NULL);
  // type cast
  ev = (struct USER_EVENT *)Event;

  // clear event state
  ev->IsClosed  = 1;
  ev->TimerEmit = 0;
  ev->TplLevel  = 0;

  return EFI_SUCCESS;
}

/**
  Signals an event.

  @param[in]  Event     The event to signal.

  @retval EFI_SUCCESS   The event was signaled.
**/
EFI_STATUS
EFIAPI
UserSignalEvent (
  IN EFI_EVENT  Event
  )
{
  struct USER_EVENT  *ev = NULL;

  // init event subsystem
  UserEventInit ();

  ASSERT (Event != NULL);

  ev = (struct USER_EVENT *)Event;
  ASSERT (!(ev->IsClosed));

  if (!(ev->IsClosed)) {
    ev->Signaled = 1;
  }

  UserEventDispatch ();

  return EFI_SUCCESS;
}

/**
  Checks whether an event is in the signaled state.

  @param[in]  Event               The event to check.

  @retval EFI_SUCCESS             The event is in the signaled state.
  @retval EFI_NOT_READY           The event is not in the signaled state.
  @retval EFI_INVALID_PARAMETER   Event is of type EVT_NOTIFY_SIGNAL.
**/
EFI_STATUS
EFIAPI
UserCheckEvent (
  IN EFI_EVENT  Event
  )
{
  struct USER_EVENT  *ev = NULL;
  EFI_STATUS         Status;

  // init event subsystem
  UserEventInit ();

  ASSERT (Event != NULL);

  ev = (struct USER_EVENT *)Event;
  ASSERT (!(ev->IsClosed));

  if (ev->Type & EVT_NOTIFY_SIGNAL) {
    return EFI_INVALID_PARAMETER;
  }

  UserEventDispatch ();

  if (ev->Signaled) {
    // Signaled?
    ev->Signaled = 0;
    Status       = EFI_SUCCESS;
  } else {
    // It isn't signaled event, but can be EVT_NOTIFY_WAIT type
    // then we need emit callback function if TPL is valid
    if ((ev->Type & EVT_NOTIFY_WAIT) && (CurTPL <= ev->TplLevel)) {
      ev->NotifyFn ((EFI_EVENT)ev, ev->NotifyCtx);
    }

    Status = EFI_NOT_READY;
  }

  return Status;
}

/**
  Stops execution untill an event is signaled.

  @param[in]  NumberOfEvents      The number of events in the Events array.
  @param[in]  Events              An array of EFI_EVENT.
  @param[out] Index               Pointer to the index of the event which satisfied the
                                  wait condition.

  @retval EFI_SUCCESS             The event indicated by Index was signaled.
  @retval EFI_INVALID_PARAMETER   NumberOfEvents is 0.
  @retval EFI_INVALID_PARAMETER   The event indicated by Index is of type EVT_NOTIFY_SIGNAL.
  @retval EFI_UNSUPPORTED         The current TPL is not TPL_APPLICATION.
**/
EFI_STATUS
EFIAPI
UserWaitForEvent (
  IN UINTN      NumberOfEvents,
  IN EFI_EVENT  *Events,
  OUT UINTN     *Index
  )
{
  struct USER_EVENT  **evs = NULL;
  struct USER_EVENT  *ev   = NULL;

  UserEventInit ();

  if (CurTPL != TPL_APPLICATION) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (Events != NULL);
  ASSERT (Index != NULL);

  evs = (struct USER_EVENT **)Events;

  if (!NumberOfEvents) {
    // check input param
    return EFI_INVALID_PARAMETER;
  }

WAIT_LOOP:

  UserEventDispatch ();

  for (UINTN I = 0; I < NumberOfEvents; I++) {
    ev = evs[I];

    if (ev == NULL) {
      continue;
    }

    if (ev->IsClosed) {
      continue;
    }

    if (ev->Type & EVT_NOTIFY_SIGNAL) {
      *Index = I;
      return EFI_INVALID_PARAMETER;
    }

    if (ev->Signaled) {
      *Index       = I;
      ev->Signaled = 0;
      return EFI_SUCCESS;
    } else {
      if ((ev->Type & EVT_NOTIFY_WAIT) && (CurTPL <= ev->TplLevel)) {
        ev->NotifyFn ((EFI_EVENT)ev, ev->NotifyCtx);
      }
    }
  }

  // blocking wait
  goto WAIT_LOOP;
}

/**
  Sets the type of timer and the trigger time for a timer event.

  @param[in]  Event               The timer event that is to be signaled at the specified
                                  time.
  @param[in]  Type                The type of time is specified in TriggerTime.
  @param[in]  TriggerTime         The number if 100ns units untill the timer expires.

  @retval EFI_SUCCESS             The event has been set to be signaled at the requested
                                  time.
  @retval EFI_INVALID_PARAMETER   Event or Type is not valid.
**/
EFI_STATUS
EFIAPI
UserSetTimer (
  IN EFI_EVENT        Event,
  IN EFI_TIMER_DELAY  Type,
  IN UINT64           TriggerTime
  )
{
  UINT64             TimeNow = 0;
  struct USER_EVENT  *ev     = NULL;

  UserEventInit ();

  // check input params
  ASSERT (Event != NULL);
  ev = (struct USER_EVENT *)Event;
  ASSERT (!(ev->IsClosed));

  if (!(ev->Type & EVT_TIMER)) {
    return EFI_INVALID_PARAMETER;
  }

  TimeNow = UserEventGetTimeNow ();

  switch (Type) {
    case TimerCancel:
      ev->TimerEmit = 0;
      break;

    case TimerRelative:
      ev->TimerEmit  = TimeNow + TriggerTime * 100;
      ev->TimerDelta = 0;
      break;

    case TimerPeriodic:
      ev->TimerEmit  = TimeNow + TriggerTime * 100;
      ev->TimerDelta = TriggerTime * 100;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  UserEventDispatch ();
  return EFI_SUCCESS;
}

/**
  Raises a task's priority level and returns its previous level.

  @param[in]  NewTpl  The new task priority level. It must be greather than or
                      equal to the current TPL.

  @return             The previous TPL value.
**/
EFI_TPL
EFIAPI
UserRaiseTPL (
  IN EFI_TPL  NewTpl
  )
{
  EFI_TPL  old;

  UserEventInit ();

  UserEventDispatch ();

  old = CurTPL;
  if ((NewTpl >= old) && (NewTpl < USER_EVENT_MAXTPL)) {
    CurTPL = NewTpl;
  }

  return old;
}

/**
  Restores a task's priority level to its previous value.

  @param[in]  OldTpl  The previous task priority level to restore.
**/
VOID
EFIAPI
UserRestoreTPL (
  IN EFI_TPL  OldTpl
  )
{
  UserEventInit ();
  ASSERT (OldTpl <= CurTPL);

  if (OldTpl < USER_EVENT_MAXTPL) {
    CurTPL = OldTpl;
  }

  UserEventDispatch ();
}

/**
  Immediately triggers event dispatch.

  @return   True if at least one event is detected in the signaled state,
            else false.
**/
BOOLEAN
UserEventDispatchNow (
  VOID
  )
{
  UINT64   TimeNow;
  BOOLEAN  res = FALSE;

  UserEventInit ();

  TimeNow = UserEventGetTimeNow ();

  for (INTN I = 0; I < USER_EVENT_MAXNUM; I++) {
    if (events[I].IsClosed) {
      continue;
    }

    res = TRUE;
    if ((events[I].Type & EVT_TIMER) && events[I].TimerEmit) {
      if (TimeNow >= events[I].TimerEmit) {
        events[I].Signaled = 1;
        if (events[I].TimerDelta) {
          events[I].TimerEmit += events[I].TimerDelta;
        } else {
          events[I].TimerEmit = 0;
        }
      }
    }

    if (events[I].Signaled && (events[I].Type & EVT_NOTIFY_SIGNAL)) {
      if (CurTPL <= events[I].TplLevel) {
        events[I].Signaled = 0;
        events[I].NotifyFn ((EFI_EVENT)&events[I], events[I].NotifyCtx);
      }
    }
  }

  return res;
}

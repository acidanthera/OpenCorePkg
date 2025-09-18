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

/**
  Event descriptor

  IsClosed    Is it an unused descriptor?
  Signaled    Is it a signaled event?
  Type        The type of Event
  TplLevel    The TPL Level of Event
  NotifyFn    A callback function
  NotifyCtx   A callback context
  TimerEmit   The timer trigger time
  TimerDelta  The timer triger delta
**/
typedef struct {
  BOOLEAN             IsClosed;
  BOOLEAN             Signaled;
  UINT32              Type;
  EFI_TPL             TplLevel;
  EFI_EVENT_NOTIFY    NotifyFn;
  VOID                *NotifyCtx;
  UINT64              TimerEmit;
  UINT64              TimerDelta;
} USER_EVENT;

/**
  Local variables

  mEventNeedInit  Do we need to initialize the event subsystem?
  mCurTPL         The current TPL value
  mEvents         The static array of Event descriptor
**/
STATIC BOOLEAN     mEventNeedInit = TRUE;
STATIC EFI_TPL     mCurTPL        = 0;
STATIC USER_EVENT  mEvents[USER_EVENT_MAXNUM];

/**
  Returns current time in nanoseconds
**/
STATIC
UINT64
UserEventGetTimeNow (
  VOID
  )
{
  UINT64  TimeNow;

  TimeNow = 0;

 #if defined (_WIN32)

  time_t  TimeRes = time (NULL);

  if (TimeRes < 0) {
    return TimeNow;
  }

  TimeNow = (UINT64)TimeRes;
  TimeNow = TimeNow*1000000000;

 #elif defined (__MACH__)
  clock_serv_t     Cclock;
  mach_timespec_t  Now;

  host_get_clock_service (mach_host_self (), SYSTEM_CLOCK, &Cclock);
  clock_get_time (Cclock, &Now);
  mach_port_deallocate (mach_task_self (), Cclock);

  TimeNow = (UINT64)Now.tv_sec*1000000000 + Now.tv_nsec;

 #else
  struct timespec  Now;

  clock_gettime (CLOCK_MONOTONIC, &Now);
  TimeNow = (UINT64)Now.tv_sec*1000000000 + Now.tv_nsec;

 #endif

  return TimeNow;
}

/**
  Allocates an Event descriptor
**/
STATIC
USER_EVENT *
UserEventAlloc (
  VOID
  )
{
  INTN  Index;

  for (Index = 0; Index < USER_EVENT_MAXNUM; Index++) {
    if (mEvents[Index].IsClosed == TRUE) {
      mEvents[Index].IsClosed = 0;
      return (&(mEvents[Index]));
    }
  }

  return NULL;
}

/**
  Dispatches signaled events and triggers active timers
**/
STATIC
VOID
UserEventDispatch (
  VOID
  )
{
  UINT64  TimeNow;
  INTN    Index;

  TimeNow = UserEventGetTimeNow ();

  for (Index = 0; Index < USER_EVENT_MAXNUM; Index++) {
    if (mEvents[Index].IsClosed == TRUE) {
      //
      // Skip unused descriptors
      //
      continue;
    }

    //
    // Is it an active timer?
    //
    if (((mEvents[Index].Type & EVT_TIMER) != 0) && (mEvents[Index].TimerEmit != 0)) {
      if (TimeNow >= mEvents[Index].TimerEmit) {
        //
        // Signal Event
        //
        mEvents[Index].Signaled = TRUE;

        //
        // Is it a periodic timer?
        //
        if (mEvents[Index].TimerDelta != 0) {
          //
          // Update trigger time
          //
          mEvents[Index].TimerEmit += mEvents[Index].TimerDelta;
        } else {
          mEvents[Index].TimerEmit = 0;
        }
      }
    }

    //
    //  Is it a signaled event?
    //
    if ((mEvents[Index].Signaled == TRUE) && ((mEvents[Index].Type & EVT_NOTIFY_SIGNAL) != 0)) {
      //
      // Is TPL of Event valid?
      //
      if (mCurTPL <= mEvents[Index].TplLevel) {
        //
        // Reset state
        //
        mEvents[Index].Signaled = FALSE;
        //
        // Call handler
        //
        mEvents[Index].NotifyFn ((EFI_EVENT)&mEvents[Index], mEvents[Index].NotifyCtx);
      }
    }
  }
}

/**
  Inits the Event subsystem
**/
STATIC
VOID
UserEventInitializer (
  VOID
  )
{
  INTN  Index;

  for (Index = 0; Index < USER_EVENT_MAXNUM; Index++) {
    mEvents[Index].IsClosed  = TRUE;
    mEvents[Index].TplLevel  = 0;
    mEvents[Index].TimerEmit = 0;
  }

  //
  // Init current TPL
  //
  mCurTPL = TPL_APPLICATION;
}

/**
  Calls UserEventInitializer() if needed
**/
STATIC
VOID
UserEventInit (
  VOID
  )
{
  if (mEventNeedInit == TRUE) {
    mEventNeedInit = FALSE;
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
  IN EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,
  IN CONST VOID        *NotifyContext OPTIONAL,
  IN CONST EFI_GUID    *EventGroup OPTIONAL,
  OUT EFI_EVENT        *Event
  )
{
  USER_EVENT  *Ev;

  UserEventInit ();

  //
  // Check input params
  //
  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (((Type & EVT_NOTIFY_SIGNAL) != 0) && ((Type & EVT_NOTIFY_WAIT) != 0)) {
    //
    // The Type param can't combine EVT_NOTIFY_SIGNAL and EVT_NOTIFY_WAIT flags
    //
    return EFI_INVALID_PARAMETER;
  }

  if ((Type & (EVT_NOTIFY_SIGNAL | EVT_NOTIFY_WAIT)) != 0) {
    //
    // Event type is signal or event
    //
    if (NotifyFunction == NULL) {
      //
      // callback function isn't set
      //
      return EFI_INVALID_PARAMETER;
    }

    if (NotifyTpl >= USER_EVENT_MAXTPL) {
      //
      // Required TPL is invalid
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Try to allocate an Event descriptor
  //
  Ev = UserEventAlloc ();
  if (Ev == NULL) {
    *Event = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Ev->Type      = Type;
  Ev->TplLevel  = NotifyTpl;
  Ev->Signaled  = 0;
  Ev->NotifyFn  = NotifyFunction;
  Ev->NotifyCtx = (VOID *)NotifyContext;
  Ev->TimerEmit = 0;

  *Event = (EFI_EVENT)Ev;
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
  IN EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,
  IN VOID              *NotifyContext OPTIONAL,
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
  USER_EVENT  *Ev;

  UserEventInit ();

  ASSERT (Event != NULL);

  Ev = (USER_EVENT *)Event;

  Ev->IsClosed  = TRUE;
  Ev->TimerEmit = 0;
  Ev->TplLevel  = 0;

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
  USER_EVENT  *Ev;

  UserEventInit ();

  ASSERT (Event != NULL);

  Ev = (USER_EVENT *)Event;
  ASSERT (Ev->IsClosed == FALSE);

  if (Ev->IsClosed == FALSE) {
    Ev->Signaled = TRUE;
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
  USER_EVENT  *Ev;
  EFI_STATUS  Status;

  UserEventInit ();

  ASSERT (Event != NULL);

  Ev = (USER_EVENT *)Event;
  ASSERT (Ev->IsClosed == FALSE);

  if ((Ev->Type & EVT_NOTIFY_SIGNAL) != 0) {
    //
    // The Event can't have the EVT_NOTIFY_SIGNAL type
    //
    return EFI_INVALID_PARAMETER;
  }

  UserEventDispatch ();

  if (Ev->Signaled == TRUE) {
    Ev->Signaled = FALSE;
    Status       = EFI_SUCCESS;
  } else {
    //
    // It isn't signaled event, but it can have the EVT_NOTIFY_WAIT type
    // In that case, we need to emit callback function if TPL is valid
    //
    if (((Ev->Type & EVT_NOTIFY_WAIT) != 0) && (mCurTPL <= Ev->TplLevel)) {
      Ev->NotifyFn ((EFI_EVENT)Ev, Ev->NotifyCtx);
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
  USER_EVENT  **Evs;
  USER_EVENT  *Ev;
  UINTN       Jndex;

  UserEventInit ();

  if (mCurTPL != TPL_APPLICATION) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (Events != NULL);
  ASSERT (Index != NULL);

  Evs = (USER_EVENT **)Events;

  if (NumberOfEvents == 0) {
    return EFI_INVALID_PARAMETER;
  }

  while (TRUE) {
    //
    // Blocking wait
    //

    UserEventDispatch ();

    for (Jndex = 0; Jndex < NumberOfEvents; Jndex++) {
      Ev = Evs[Jndex];

      if (Ev == NULL) {
        continue;
      }

      if (Ev->IsClosed == TRUE) {
        continue;
      }

      if ((Ev->Type & EVT_NOTIFY_SIGNAL) != 0) {
        *Index = Jndex;
        return EFI_INVALID_PARAMETER;
      }

      if (Ev->Signaled == TRUE) {
        *Index       = Jndex;
        Ev->Signaled = FALSE;
        return EFI_SUCCESS;
      } else {
        if (((Ev->Type & EVT_NOTIFY_WAIT) != 0) && (mCurTPL <= Ev->TplLevel)) {
          Ev->NotifyFn ((EFI_EVENT)Ev, Ev->NotifyCtx);
        }
      }
    }
  }
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
  UINT64      TimeNow;
  USER_EVENT  *Ev;

  UserEventInit ();

  // check input params
  ASSERT (Event != NULL);
  Ev = (USER_EVENT *)Event;
  ASSERT (Ev->IsClosed == FALSE);

  if ((Ev->Type & EVT_TIMER) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  TimeNow = UserEventGetTimeNow ();

  switch (Type) {
    case TimerCancel:
      Ev->TimerEmit = 0;
      break;

    case TimerRelative:
      Ev->TimerEmit  = TimeNow + TriggerTime * 100;
      Ev->TimerDelta = 0;
      break;

    case TimerPeriodic:
      Ev->TimerEmit  = TimeNow + TriggerTime * 100;
      Ev->TimerDelta = TriggerTime * 100;
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
  EFI_TPL  Old;

  UserEventInit ();

  UserEventDispatch ();

  Old = mCurTPL;
  if ((NewTpl >= Old) && (NewTpl < USER_EVENT_MAXTPL)) {
    mCurTPL = NewTpl;
  }

  return Old;
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
  ASSERT (OldTpl <= mCurTPL);

  if (OldTpl < USER_EVENT_MAXTPL) {
    mCurTPL = OldTpl;
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
  BOOLEAN  Res;
  INTN     Index;

  UserEventInit ();

  TimeNow = UserEventGetTimeNow ();

  Res = FALSE;
  for (Index = 0; Index < USER_EVENT_MAXNUM; Index++) {
    if (mEvents[Index].IsClosed == TRUE) {
      continue;
    }

    Res = TRUE;
    if (((mEvents[Index].Type & EVT_TIMER) != 0) && (mEvents[Index].TimerEmit != 0)) {
      if (TimeNow >= mEvents[Index].TimerEmit) {
        mEvents[Index].Signaled = TRUE;
        if (mEvents[Index].TimerDelta != 0) {
          mEvents[Index].TimerEmit += mEvents[Index].TimerDelta;
        } else {
          mEvents[Index].TimerEmit = 0;
        }
      }
    }

    if ((mEvents[Index].Signaled == TRUE) && ((mEvents[Index].Type & EVT_NOTIFY_SIGNAL) != 0)) {
      if (mCurTPL <= mEvents[Index].TplLevel) {
        mEvents[Index].Signaled = FALSE;
        mEvents[Index].NotifyFn ((EFI_EVENT)&mEvents[Index], mEvents[Index].NotifyCtx);
      }
    }
  }

  return Res;
}

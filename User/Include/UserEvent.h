/** @file
  Copyright (c) 2025, Pavel Naberezhnev. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef USER_EVENT_H
#define USER_EVENT_H

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#define USER_EVENT_MAXNUM  (512)
#define USER_EVENT_MAXTPL  (32)

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
  );

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
  );

/**
  Closes an event.

  @param[in]  Event     The event to close.

  @retval EFI_SUCCESS   The event has been closed.
**/
EFI_STATUS
EFIAPI
UserCloseEvent (
  IN EFI_EVENT  Event
  );

/**
  Signals an event.

  @param[in]  Event     The event to signal.

  @retval EFI_SUCCESS   The event was signaled.
**/
EFI_STATUS
EFIAPI
UserSignalEvent (
  IN EFI_EVENT  Event
  );

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
  );

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
  );

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
  );

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
  );

/**
  Restores a task's priority level to its previous value.

  @param[in]  OldTpl  The previous task priority level to restore.
**/
VOID
EFIAPI
UserRestoreTPL (
  IN EFI_TPL  OldTpl
  );

/**
  Immediately triggers event dispatch.

  @return   True if at least one event is detected in the signaled state,
            else false.
**/
BOOLEAN
UserEventDispatchNow (
  VOID
  );

#endif

/** @file
Copyright (C) 2015 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_EVENT_LIB_H
#define APPLE_EVENT_LIB_H

// EventLibCreateTimerEvent
EFI_EVENT
EventLibCreateTimerEvent (
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN VOID              *NotifyContext,
  IN UINT64            TriggerTime,
  IN BOOLEAN           SignalPeriodic,
  IN EFI_TPL           NotifyTpl
  );

// EventLibCreateNotifyTimerEvent
EFI_EVENT
EventLibCreateNotifyTimerEvent (
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN VOID              *NotifyContext,
  IN UINT64            TriggerTime,
  IN BOOLEAN           SignalPeriodic
  );

// EventLibCancelEvent
VOID
EventLibCancelEvent (
  IN EFI_EVENT  Event
  );

#endif // APPLE_EVENT_LIB_H

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

#ifndef APPLE_EVENT_INTERNAL_H_
#define APPLE_EVENT_INTERNAL_H_

#include <IndustryStandard/AppleHid.h>

#include <Protocol/AppleEvent.h>

// EventCreateKeyStrokePollEvent
EFI_STATUS
EventCreateKeyStrokePollEvent (
  VOID
  );

// EventCancelKeyStrokePollEvent
VOID
EventCancelKeyStrokePollEvent (
  VOID
  );

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
EventIsCapsLockOnImpl (
  IN OUT BOOLEAN  *CLockOn
  );

// EventCreateSimplePointerInstallNotifyEvent
EFI_STATUS
EventCreateSimplePointerInstallNotifyEvent (
  VOID
  );

// EventCreateSimplePointerInstallNotifyEvent
VOID
EventCloseSimplePointerInstallNotifyEvent (
  VOID
  );

// EventCancelSimplePointerPollEvent
VOID
EventCancelSimplePointerPollEvent (
  VOID
  );

// EventCreateSimplePointerPollEvent
EFI_STATUS
EventCreateSimplePointerPollEvent (
  VOID
  );

// EventSimplePointerDesctructor
VOID
EventSimplePointerDesctructor (
  VOID
  );

// EventCreateAppleEventQueueInfo
APPLE_EVENT_INFORMATION *
EventCreateAppleEventQueueInfo (
  IN APPLE_EVENT_DATA    EventData,
  IN APPLE_EVENT_TYPE    EventType,
  IN DIMENSION           *PointerPosition,
  IN APPLE_MODIFIER_MAP  Modifiers
  );

// EventAddEventToQueue
VOID
EventAddEventToQueue (
  IN APPLE_EVENT_INFORMATION  *Information
  );

// EventCreateEventQueue
EFI_STATUS
EventCreateEventQueue (
  IN APPLE_EVENT_DATA    EventData,
  IN APPLE_EVENT_TYPE    EventType,
  IN APPLE_MODIFIER_MAP  Modifiers
  );

// InternalSignalAndCloseQueueEvent
VOID
InternalSignalAndCloseQueueEvent (
  VOID
  );

// EventSetCursorPositionImpl
EFI_STATUS
EventSetCursorPositionImpl (
  IN DIMENSION  *Position
  );

// InternalCreateQueueEvent
VOID
InternalCreateQueueEvent (
  VOID
  );

// InternalFlagAllEventsReady
VOID
InternalFlagAllEventsReady (
  VOID
  );

// InternalSignalEvents
VOID
InternalSignalEvents (
  IN APPLE_EVENT_INFORMATION  *Information
  );

// InternalRemoveUnregisteredEvents
VOID
InternalRemoveUnregisteredEvents (
  VOID
  );

// InternalGetModifierStrokes
APPLE_MODIFIER_MAP
InternalGetModifierStrokes (
  VOID
  );

// EventInputKeyFromAppleKeyCode
VOID
EventInputKeyFromAppleKeyCode (
  IN  APPLE_KEY_CODE  AppleKeyCode,
  OUT EFI_INPUT_KEY   *InputKey,
  IN  BOOLEAN         Shifted
  );

// InternalSetKeyDelays
VOID
InternalSetKeyDelays (
  IN  BOOLEAN         CustomDelays,
  IN  UINT16          KeyInitialDelay,
  IN  UINT16          KeySubsequentDelay
  );

VOID
InternalSetPointerSpeed (
  IN UINT16 PointerSpeedDiv,
  IN UINT16 PointerSpeedMul
  );

extern UINT32 mPointerSpeedMul;
extern UINT32 mPointerSpeedDiv;

#endif // APPLE_EVENT_INTERNAL_H_

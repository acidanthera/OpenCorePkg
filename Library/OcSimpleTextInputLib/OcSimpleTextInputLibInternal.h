/** @file
  SimpleTextInputEx protocol implementation on top of AppleEvent and SimpleTextInput protocols.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_SIMPLE_TEXT_INPUT_LIB_INTERNAL_H
#define OC_SIMPLE_TEXT_INPUT_LIB_INTERNAL_H

#include <Protocol/AppleEvent.h>
#include <Protocol/AppleKeyMapAggregator.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

typedef struct {
  VOID                       *NotifyHandle;
  EFI_KEY_DATA               KeyData;
  EFI_KEY_NOTIFY_FUNCTION    KeyNotificationFunction;
  LIST_ENTRY                 Link;
} OC_KEY_NOTIFY_ENTRY;

typedef struct {
  APPLE_EVENT_PROTOCOL                 *AppleEvent;
  APPLE_EVENT_HANDLE                   AppleEventKeyHandle;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL    *KeyMapAggregator;

  EFI_KEY_TOGGLE_STATE                 KeyToggleState;

  LIST_ENTRY                           NotifyList;
} OC_SIMPLE_TEXT_INPUT_EX_CONTEXT;

#endif // OC_SIMPLE_TEXT_INPUT_LIB_INTERNAL_H

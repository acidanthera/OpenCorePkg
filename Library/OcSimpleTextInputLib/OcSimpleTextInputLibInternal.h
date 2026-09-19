/** @file
  Simple Text Input Ex protocol implementation on top of Apple Event protocol.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_SIMPLE_TEXT_INPUT_LIB_INTERNAL_H
#define OC_SIMPLE_TEXT_INPUT_LIB_INTERNAL_H

#include <Protocol/AppleEvent.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

typedef struct {
  VOID                       *NotifyHandle;
  EFI_KEY_DATA               KeyData;
  EFI_KEY_NOTIFY_FUNCTION    KeyNotificationFunction;
  LIST_ENTRY                 Link;
} OC_KEY_NOTIFY_ENTRY;

typedef struct {
  APPLE_EVENT_PROTOCOL    *AppleEvent;
  APPLE_EVENT_HANDLE      AppleEventKeyHandle;

  UINT32                  LastShiftState;
  EFI_KEY_TOGGLE_STATE    KeyToggleState;

  LIST_ENTRY              NotifyList;
} OC_SIMPLE_TEST_INPUT_EX_CONTEXT;

#endif // OC_SIMPLE_TEXT_INPUT_LIB_INTERNAL_H

/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/AppleEvent.h>
#include <Protocol/AppleKeyMapAggregator.h>

#include <Library/DebugLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../GuiIo.h"

#define IS_POWER_2(x)  (((x) & ((x) - 1)) == 0 && (x) != 0)

struct GUI_KEY_CONTEXT_ {
  APPLE_EVENT_PROTOCOL               *AppleEvent;
  APPLE_EVENT_HANDLE                 AppleEventHandle;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;
  OC_PICKER_CONTEXT                  *Context;
  UINT8                              EventQueueHead;
  UINT8                              EventQueueTail;
  GUI_KEY_EVENT                      EventQueue[16];
};

STATIC
VOID
InternalQueueKeyEvent (
  IN OUT GUI_KEY_CONTEXT          *Context,
  IN     EFI_INPUT_KEY            InputKey,
  IN     USB_HID_KB_MODIFIER_MAP  Modifiers
  )
{
  UINT32 Tail;
  //
  // Tail can be accessed concurrently, so increment atomically.
  // Due to the modulus, wraparounds do not matter. The size of the queue must
  // be a power of two for this to hold.
  //
  STATIC_ASSERT (
    IS_POWER_2 (ARRAY_SIZE (Context->EventQueue)),
    "The pointer event queue must have a power of two length."
    );

  Tail = Context->EventQueueTail % ARRAY_SIZE (Context->EventQueue);
  Context->EventQueue[Tail].Key       = InputKey;
  Context->EventQueue[Tail].Modifiers = Modifiers;

  ++Context->EventQueueTail;
  if (Context->EventQueueHead % ARRAY_SIZE (Context->EventQueue) == Context->EventQueueTail % ARRAY_SIZE (Context->EventQueue)) {
    DEBUG ((DEBUG_INFO, "OCUI: Full key queue\n"));
  }
}

BOOLEAN
GuiKeyGetEvent (
  IN OUT GUI_KEY_CONTEXT  *Context,
  OUT    GUI_KEY_EVENT    *Event
  )
{
  //
  // EventQueueHead cannot be accessed concurrently.
  //
  if (Context->EventQueueHead == Context->EventQueueTail) {
    return FALSE;
  }

  CopyMem (
    Event,
    &Context->EventQueue[Context->EventQueueHead % ARRAY_SIZE (Context->EventQueue)],
    sizeof (*Event)
    );
  //
  // Due to the modulus, wraparounds do not matter. The size of the queue must
  // be a power of two for this to hold.
  //
  STATIC_ASSERT (
    IS_POWER_2 (ARRAY_SIZE (Context->EventQueue)),
    "The pointer event queue must have a power of two length."
    );
  ++Context->EventQueueHead;

  return TRUE;
}

STATIC
VOID
EFIAPI 
InternalAppleEventNotification (
  IN APPLE_EVENT_INFORMATION  *Information,
  IN VOID                     *NotifyContext
  )
{
  GUI_KEY_CONTEXT *Context;

  Context = NotifyContext;

  ASSERT ((Information->EventType & APPLE_ALL_KEYBOARD_EVENTS) != 0);

  if ((Information->EventType & APPLE_EVENT_TYPE_KEY_DOWN) != 0) {
    ASSERT ((USB_HID_KB_MODIFIER_MAP) Information->Modifiers == Information->Modifiers);
    InternalQueueKeyEvent (
      Context,
      Information->EventData.KeyData->InputKey,
      (USB_HID_KB_MODIFIER_MAP) Information->Modifiers
      );
  }
}

GUI_KEY_CONTEXT *
GuiKeyConstruct (
  IN OC_PICKER_CONTEXT  *PickerContext
  )
{
  // TODO: alloc on the fly?
  STATIC GUI_KEY_CONTEXT Context;

  EFI_STATUS Status;

  Context.KeyMap  = OcAppleKeyMapInstallProtocols (FALSE);
  Context.Context = PickerContext;
  if (Context.KeyMap == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: Missing AppleKeyMapAggregator\n"));
    return NULL;
  }

  Status = OcHandleProtocolFallback (
    gST->ConsoleInHandle,
    &gAppleEventProtocolGuid,
    (VOID **)&Context.AppleEvent
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCUI: Missing AppleEvent\n"));
    return NULL;
  }

  if (Context.AppleEvent->Revision >= APPLE_EVENT_PROTOCOL_REVISION) {
    Status = Context.AppleEvent->RegisterHandler (
      APPLE_ALL_KEYBOARD_EVENTS,
      InternalAppleEventNotification,
      &Context.AppleEventHandle,
      &Context
      );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "OCUI: AppleEvent %u is unsupported - %r\n",
      Context.AppleEvent->Revision,
      Status
      ));
    return NULL;
  }

  return &Context;
}

VOID
EFIAPI
GuiKeyReset (
  IN OUT GUI_KEY_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  //
  // Flush console here?
  //
}

VOID
GuiKeyDestruct (
  IN GUI_KEY_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->AppleEvent != NULL);

  Context->AppleEvent->UnregisterHandler (Context->AppleEventHandle);
  ZeroMem (Context, sizeof (*Context));
}

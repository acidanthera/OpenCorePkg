/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/AppleKeyMapAggregator.h>

#include <Library/DebugLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../GuiIo.h"

GUI_KEY_CONTEXT *
GuiKeyConstruct (
  IN OC_PICKER_CONTEXT  *PickerContext
  )
{
  STATIC GUI_KEY_CONTEXT  mContext;
  mContext.Context = PickerContext;

  return &mContext;
}

BOOLEAN
GuiKeyGetEvent (
  IN OUT GUI_KEY_CONTEXT  *Context,
  OUT    GUI_KEY_EVENT    *Event
  )
{
  ASSERT (Context != NULL);
  ASSERT (Event != NULL);

  Context->Context->HotKeyContext->GetKeyInfo (
    Context->Context,
    Context->KeyFilter,
    Event
    );
  
  if (Context->KeyFilter == OC_PICKER_KEYS_FOR_PICKER
   && Context->OcModifiers != Event->OcModifiers) {
    Context->OcModifiers = Event->OcModifiers;
    return TRUE;
  }

  if (Context->KeyFilter == OC_PICKER_KEYS_FOR_TYPING
   && Event->UnicodeChar != L'\0') {
    return TRUE;
  }

  return Event->OcKeyCode != OC_INPUT_NO_ACTION;
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
  Context->Context->HotKeyContext->FlushTypingBuffer (Context->Context);
}

VOID
GuiKeyDestruct (
  IN GUI_KEY_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ZeroMem (Context, sizeof (*Context));
}

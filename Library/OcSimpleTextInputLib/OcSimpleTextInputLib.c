/** @file
  SimpleTextInputEx protocol implementation on top of AppleEvent and SimpleTextInput protocols.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "OcSimpleTextInputLibInternal.h"

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OcSimpleTextInputLib.h>

STATIC  OC_SIMPLE_TEXT_INPUT_EX_CONTEXT  mOcInExContext;
STATIC  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *mTextIn;

STATIC
VOID
OcInExAppleEventModifiersConvert (
  IN APPLE_MODIFIER_MAP  AppleModifiers,
  OUT UINT32             *EfiShiftState
  )
{
  UINT32  ShiftState;

  ShiftState = EFI_SHIFT_STATE_VALID;

  if (AppleModifiers & APPLE_MODIFIER_RIGHT_SHIFT) {
    ShiftState |= EFI_RIGHT_SHIFT_PRESSED;
  }

  if (AppleModifiers & APPLE_MODIFIER_LEFT_SHIFT) {
    ShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }

  if (AppleModifiers & APPLE_MODIFIER_RIGHT_CONTROL) {
    ShiftState |= EFI_RIGHT_CONTROL_PRESSED;
  }

  if (AppleModifiers & APPLE_MODIFIER_LEFT_CONTROL) {
    ShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }

  if (AppleModifiers & APPLE_MODIFIER_RIGHT_OPTION) {
    ShiftState |= EFI_RIGHT_ALT_PRESSED;
  }

  if (AppleModifiers & APPLE_MODIFIER_LEFT_OPTION) {
    ShiftState |= EFI_LEFT_ALT_PRESSED;
  }

  if (AppleModifiers & APPLE_MODIFIER_RIGHT_COMMAND) {
    ShiftState |= EFI_RIGHT_LOGO_PRESSED;
  }

  if (AppleModifiers & APPLE_MODIFIER_LEFT_COMMAND) {
    ShiftState |= EFI_LEFT_LOGO_PRESSED;
  }

  *EfiShiftState = ShiftState;
}

STATIC
UINT32
OcInExGetShiftState (
  VOID
  )
{
  EFI_STATUS          Status;
  APPLE_MODIFIER_MAP  Modifiers;
  APPLE_KEY_CODE      KeyCodes[16];
  UINTN               NumberOfKeyCodes;
  UINT32              ShiftState;

  Modifiers        = 0;
  NumberOfKeyCodes = ARRAY_SIZE (KeyCodes);

  Status = mOcInExContext.KeyMapAggregator->GetKeyStrokes (
                                              mOcInExContext.KeyMapAggregator,
                                              &Modifiers,
                                              &NumberOfKeyCodes,
                                              KeyCodes
                                              );
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    Modifiers = 0;
  }

  OcInExAppleEventModifiersConvert (Modifiers, &ShiftState);
  return ShiftState;
}

STATIC
EFI_KEY_TOGGLE_STATE
OcInExGetToggleState (
  VOID
  )
{
  EFI_KEY_TOGGLE_STATE  ToggleState;
  BOOLEAN               CapsLockOn;

  ToggleState = mOcInExContext.KeyToggleState | EFI_TOGGLE_STATE_VALID;

  CapsLockOn = FALSE;
  if (!EFI_ERROR (mOcInExContext.AppleEvent->IsCapsLockOn (&CapsLockOn)) && CapsLockOn) {
    ToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }

  return ToggleState;
}

STATIC
VOID
EFIAPI
OcInExAppleEventKeyHandler (
  IN APPLE_EVENT_INFORMATION  *Information,
  IN VOID                     *NotifyContext
  )
{
  OC_SIMPLE_TEXT_INPUT_EX_CONTEXT  *Context;
  EFI_KEY_DATA                     KeyData;
  LIST_ENTRY                       *Link;
  LIST_ENTRY                       *Next;
  OC_KEY_NOTIFY_ENTRY              *Entry;

  Context = (OC_SIMPLE_TEXT_INPUT_EX_CONTEXT *)NotifyContext;

  if (  ((Information->EventType & APPLE_EVENT_TYPE_KEY_DOWN) == 0)
     || (Information->EventData.KeyData == NULL))
  {
    return;
  }

  if (IsListEmpty (&Context->NotifyList)) {
    return;
  }

  ZeroMem (&KeyData, sizeof (KeyData));
  KeyData.Key = Information->EventData.KeyData->InputKey;
  OcInExAppleEventModifiersConvert (Information->Modifiers, &KeyData.KeyState.KeyShiftState);
  KeyData.KeyState.KeyToggleState = OcInExGetToggleState ();

  for (Link = GetFirstNode (&Context->NotifyList);
       !IsNull (&Context->NotifyList, Link);
       Link = Next)
  {
    Next  = GetNextNode (&Context->NotifyList, Link);
    Entry = BASE_CR (Link, OC_KEY_NOTIFY_ENTRY, Link);

    if (  (Entry->KeyData.Key.UnicodeChar != KeyData.Key.UnicodeChar)
       || (Entry->KeyData.Key.ScanCode != KeyData.Key.ScanCode))
    {
      continue;
    }

    if (  ((Entry->KeyData.KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) != 0)
       && (Entry->KeyData.KeyState.KeyShiftState != KeyData.KeyState.KeyShiftState))
    {
      continue;
    }

    if (  ((Entry->KeyData.KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != 0)
       && (Entry->KeyData.KeyState.KeyToggleState != KeyData.KeyState.KeyToggleState))
    {
      continue;
    }

    Entry->KeyNotificationFunction (&KeyData);
  }
}

STATIC
EFI_STATUS
EFIAPI
Reset (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  if (mTextIn == NULL) {
    return EFI_NOT_READY;
  }

  return mTextIn->Reset (mTextIn, ExtendedVerification);
}

STATIC
EFI_STATUS
EFIAPI
ReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  EFI_STATUS  Status;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mTextIn == NULL) {
    return EFI_NOT_READY;
  }

  ZeroMem (KeyData, sizeof (*KeyData));

  Status = mTextIn->ReadKeyStroke (mTextIn, &KeyData->Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyData->KeyState.KeyShiftState  = OcInExGetShiftState ();
  KeyData->KeyState.KeyToggleState = OcInExGetToggleState ();

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
WaitForKeyEx (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (mTextIn == NULL) {
    return;
  }

  Status = gBS->CheckEvent (mTextIn->WaitForKey);
  if (!EFI_ERROR (Status)) {
    gBS->SignalEvent (Event);
  }
}

STATIC
EFI_STATUS
EFIAPI
SetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  mOcInExContext.KeyToggleState = *KeyToggleState;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
RegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  )
{
  OC_KEY_NOTIFY_ENTRY  *NotifyEntry;
  LIST_ENTRY           *Link;
  EFI_TPL              OldTpl;

  if ((KeyData == NULL) || (KeyNotificationFunction == NULL) || (NotifyHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  for (Link = GetFirstNode (&mOcInExContext.NotifyList);
       !IsNull (&mOcInExContext.NotifyList, Link);
       Link = GetNextNode (&mOcInExContext.NotifyList, Link))
  {
    NotifyEntry = BASE_CR (Link, OC_KEY_NOTIFY_ENTRY, Link);

    if (  (NotifyEntry->KeyNotificationFunction == KeyNotificationFunction)
       && (NotifyEntry->KeyData.Key.UnicodeChar == KeyData->Key.UnicodeChar)
       && (NotifyEntry->KeyData.Key.ScanCode == KeyData->Key.ScanCode)
       && (NotifyEntry->KeyData.KeyState.KeyShiftState == KeyData->KeyState.KeyShiftState)
       && (NotifyEntry->KeyData.KeyState.KeyToggleState == KeyData->KeyState.KeyToggleState))
    {
      *NotifyHandle = NotifyEntry->NotifyHandle;
      gBS->RestoreTPL (OldTpl);
      return EFI_SUCCESS;
    }
  }

  NotifyEntry = AllocateZeroPool (sizeof (OC_KEY_NOTIFY_ENTRY));
  if (NotifyEntry == NULL) {
    gBS->RestoreTPL (OldTpl);
    return EFI_OUT_OF_RESOURCES;
  }

  NotifyEntry->KeyData                 = *KeyData;
  NotifyEntry->KeyNotificationFunction = KeyNotificationFunction;
  NotifyEntry->NotifyHandle            = NotifyEntry;

  InsertTailList (&mOcInExContext.NotifyList, &NotifyEntry->Link);

  *NotifyHandle = NotifyEntry;

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  LIST_ENTRY           *Link;
  OC_KEY_NOTIFY_ENTRY  *NotifyEntry;
  EFI_TPL              OldTpl;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  for (Link = GetFirstNode (&mOcInExContext.NotifyList);
       !IsNull (&mOcInExContext.NotifyList, Link);
       Link = GetNextNode (&mOcInExContext.NotifyList, Link))
  {
    NotifyEntry = BASE_CR (Link, OC_KEY_NOTIFY_ENTRY, Link);

    if (NotifyEntry->NotifyHandle == NotificationHandle) {
      RemoveEntryList (&NotifyEntry->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (NotifyEntry);
      return EFI_SUCCESS;
    }
  }

  gBS->RestoreTPL (OldTpl);
  return EFI_INVALID_PARAMETER;
}

STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  mSimpleTextInputEx = {
  Reset,
  ReadKeyStrokeEx,
  NULL,
  SetState,
  RegisterKeyNotify,
  UnregisterKeyNotify
};

EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *
OcSimpleTextInputExInstallProtocol (
  VOID
  )
{
  EFI_STATUS                         Status;
  APPLE_EVENT_PROTOCOL               *AppleEvent;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMapAggregator;
  EFI_HANDLE                         NewHandle;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *Existing;

  Existing = NULL;

  Status = gBS->HandleProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&Existing
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSTI: SimpleTextInputEx is already present\n"));
    return Existing;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **)&mTextIn
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: Unable to find SimpleTextIn protocol instance - %r\n", Status));
    return NULL;
  }

  KeyMapAggregator = NULL;

  Status = gBS->LocateProtocol (&gAppleKeyMapAggregatorProtocolGuid, NULL, (VOID **)&KeyMapAggregator);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: Unable to find AppleKeyMapAggregator protocol instance - %r\n", Status));
    return NULL;
  }

  AppleEvent = NULL;

  Status = gBS->LocateProtocol (&gAppleEventProtocolGuid, NULL, (VOID **)&AppleEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: Unable to find AppleEvent protocol instance - %r\n", Status));
    return NULL;
  }

  ZeroMem (&mOcInExContext, sizeof (mOcInExContext));
  mOcInExContext.AppleEvent       = AppleEvent;
  mOcInExContext.KeyMapAggregator = KeyMapAggregator;
  mOcInExContext.KeyToggleState   = EFI_TOGGLE_STATE_VALID;
  InitializeListHead (&mOcInExContext.NotifyList);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  WaitForKeyEx,
                  &mOcInExContext,
                  &mSimpleTextInputEx.WaitForKeyEx
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: CreateEvent failed - %r\n", Status));
    return NULL;
  }

  Status = mOcInExContext.AppleEvent->RegisterHandler (
                                        APPLE_EVENT_TYPE_KEY_DOWN,
                                        OcInExAppleEventKeyHandler,
                                        &mOcInExContext.AppleEventKeyHandle,
                                        &mOcInExContext
                                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: AppleEvent RegisterHandler failed - %r\n", Status));
    gBS->CloseEvent (mSimpleTextInputEx.WaitForKeyEx);
    return NULL;
  }

  NewHandle = gST->ConsoleInHandle;
  Status    = gBS->InstallMultipleProtocolInterfaces (
                     &NewHandle,
                     &gEfiSimpleTextInputExProtocolGuid,
                     &mSimpleTextInputEx,
                     NULL
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: InstallMultipleProtocolInterfaces failed - %r\n", Status));
    mOcInExContext.AppleEvent->UnregisterHandler (mOcInExContext.AppleEventKeyHandle);
    gBS->CloseEvent (mSimpleTextInputEx.WaitForKeyEx);
    return NULL;
  }

  return &mSimpleTextInputEx;
}

/** @file
  Simple Text Input Ex protocol implementation on top of Apple Event protocol.

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
#include <Library/OcMiscLib.h>

STATIC  OC_SIMPLE_TEXT_INPUT_EX_CONTEXT  mOcInExContext;
STATIC  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *mOriginalTextInput;

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
VOID
EFIAPI
OcInExAppleEventKeyHandler (
  IN APPLE_EVENT_INFORMATION  *Information,
  IN VOID                     *NotifyContext
  )
{
  OC_SIMPLE_TEXT_INPUT_EX_CONTEXT  *Context;

  Context = (OC_SIMPLE_TEXT_INPUT_EX_CONTEXT *)NotifyContext;

  OcInExAppleEventModifiersConvert (Information->Modifiers, &Context->LastShiftState);
}

STATIC
EFI_STATUS
EFIAPI
OcInputResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  if (mOriginalTextInput == NULL) {
    return EFI_NOT_READY;
  }

  return mOriginalTextInput->Reset (mOriginalTextInput, ExtendedVerification);
}

STATIC
EFI_STATUS
EFIAPI
OcInputReadKeyEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  EFI_STATUS  Status;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mOriginalTextInput == NULL) {
    return EFI_NOT_READY;
  }

  ZeroMem (KeyData, sizeof (*KeyData));

  Status = mOriginalTextInput->ReadKeyStroke (mOriginalTextInput, &KeyData->Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyData->KeyState.KeyShiftState  = mOcInExContext.LastShiftState;
  KeyData->KeyState.KeyToggleState = mOcInExContext.KeyToggleState | EFI_TOGGLE_STATE_VALID;

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
OcWaitForKeyEx (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (mOriginalTextInput == NULL) {
    return;
  }

  Status = gBS->CheckEvent (mOriginalTextInput->WaitForKey);
  if (!EFI_ERROR (Status)) {
    gBS->SignalEvent (Event);
  }
}

STATIC
EFI_STATUS
EFIAPI
OcSetState (
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
OcRegisterKeyStrokeNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  )
{
  OC_KEY_NOTIFY_ENTRY  *NotifyEntry;

  if ((KeyData == NULL) || (KeyNotificationFunction == NULL) || (NotifyHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NotifyEntry = AllocateZeroPool (sizeof (OC_KEY_NOTIFY_ENTRY));
  if (NotifyEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NotifyEntry->KeyData                 = *KeyData;
  NotifyEntry->KeyNotificationFunction = KeyNotificationFunction;
  NotifyEntry->NotifyHandle            = NotifyEntry;

  InsertTailList (&mOcInExContext.NotifyList, &NotifyEntry->Link);

  *NotifyHandle = NotifyEntry;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OcUnRegisterKeyStrokeNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  LIST_ENTRY           *Link;
  OC_KEY_NOTIFY_ENTRY  *NotifyEntry;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode (&mOcInExContext.NotifyList);
       !IsNull (&mOcInExContext.NotifyList, Link);
       Link = GetNextNode (&mOcInExContext.NotifyList, Link))
  {
    NotifyEntry = BASE_CR (Link, OC_KEY_NOTIFY_ENTRY, Link);

    if (NotifyEntry->NotifyHandle == NotificationHandle) {
      RemoveEntryList (&NotifyEntry->Link);
      FreePool (NotifyEntry);
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  mOcSimpleTextInputEx = {
  OcInputResetEx,
  OcInputReadKeyEx,
  NULL,
  OcSetState,
  OcRegisterKeyStrokeNotify,
  OcUnRegisterKeyStrokeNotify
};

EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *
OcSimpleTextInputExInstallProtocol (
  VOID
  )
{
  EFI_STATUS                         Status;
  APPLE_EVENT_PROTOCOL               *AppleEvent;
  EFI_HANDLE                         NewHandle;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *Existing;

  Status = gBS->HandleProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&Existing
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCSTI: SimpleTextInputEx is already present\n"));
    return Existing;
  }

  //
  // Ensure that we have Apple Event first
  //
  Status = gBS->LocateProtocol (&gAppleEventProtocolGuid, NULL, (VOID **)&AppleEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: Unable to find Apple Event protocol instance - %r\n", Status));
    return NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **)&mOriginalTextInput
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: Unable to find SimpleTextIn protocol instance - %r\n", Status));
    return NULL;
  }

  ZeroMem (&mOcInExContext, sizeof (mOcInExContext));
  mOcInExContext.AppleEvent     = AppleEvent;
  mOcInExContext.KeyToggleState = EFI_TOGGLE_STATE_VALID;
  mOcInExContext.LastShiftState = EFI_SHIFT_STATE_VALID;
  InitializeListHead (&mOcInExContext.NotifyList);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  OcWaitForKeyEx,
                  &mOcInExContext,
                  &mOcSimpleTextInputEx.WaitForKeyEx
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: CreateEvent failed - %r\n", Status));
    return NULL;
  }

  Status = mOcInExContext.AppleEvent->RegisterHandler (
                                        APPLE_EVENT_TYPE_KEY_DOWN | APPLE_EVENT_TYPE_KEY_UP,
                                        OcInExAppleEventKeyHandler,
                                        &mOcInExContext.AppleEventKeyHandle,
                                        &mOcInExContext
                                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: AppleEvent RegisterHandler failed - %r\n", Status));
    gBS->CloseEvent (mOcSimpleTextInputEx.WaitForKeyEx);
    return NULL;
  }

  NewHandle = gST->ConsoleInHandle;
  Status    = gBS->InstallMultipleProtocolInterfaces (
                     &NewHandle,
                     &gEfiSimpleTextInputExProtocolGuid,
                     &mOcSimpleTextInputEx,
                     NULL
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCSTI: InstallMultipleProtocolInterfaces failed - %r\n", Status));
    mOcInExContext.AppleEvent->UnregisterHandler (mOcInExContext.AppleEventKeyHandle);
    gBS->CloseEvent (mOcSimpleTextInputEx.WaitForKeyEx);
    return NULL;
  }

  return &mOcSimpleTextInputEx;
}

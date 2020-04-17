/** @file
  AmiEfiKeycode to KeyMapDb translator.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include "AIK.h"

#include <Library/DebugLib.h>
#include <Library/OcInputLib.h>
#include <Library/UefiBootServicesTableLib.h>

AIK_SELF gAikSelf;

STATIC
VOID
EFIAPI
AIKProtocolArriveHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  AIK_SELF    *Keycode;

  Keycode = (AIK_SELF *) Context;

  if (Keycode == NULL || Keycode->OurJobIsDone) {
    DEBUG ((DEBUG_INFO, "AIKProtocolArriveHandler got null handler or called when done\n"));
    return;
  }

  Status = AIKInstall (Keycode);
  if (!EFI_ERROR (Status)) {
    //
    // We are successful, so can remove the protocol polling event if any
    //
    AIKProtocolArriveUninstall (Keycode);
  } else {
    DEBUG ((DEBUG_INFO, "AIKProtocolArriveHandler AIKInstall failed - %r\n", Status));
  }
}

EFI_STATUS
AIKProtocolArriveInstall (
  AIK_SELF  *Keycode
  )
{
  EFI_STATUS    Status;
  VOID          *Registration;

  Status = EFI_SUCCESS;

  if (Keycode->KeyMapDbArriveEvent == NULL) {
    Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, AIKProtocolArriveHandler, Keycode, &Keycode->KeyMapDbArriveEvent);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "KeyMapDbArriveEvent creation failed - %r\n", Status));
    } else {
      Status = gBS->RegisterProtocolNotify (&gAppleKeyMapDatabaseProtocolGuid, Keycode->KeyMapDbArriveEvent, &Registration);

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "KeyMapDbArriveEvent registration failed - %r\n", Status));
        gBS->CloseEvent (Keycode->KeyMapDbArriveEvent);
        Keycode->KeyMapDbArriveEvent = NULL;
      }
    }
  }

  return Status;
}

VOID
AIKProtocolArriveUninstall (
  AIK_SELF  *Keycode
  )
{
  if (Keycode->KeyMapDbArriveEvent != NULL) {
    gBS->CloseEvent (Keycode->KeyMapDbArriveEvent);
    Keycode->KeyMapDbArriveEvent = NULL;
  }
}

VOID
EFIAPI
AIKPollKeyboardHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  AIK_SELF          *Keycode;
  UINT64            Counter;
  UINTN             Index;
  AMI_EFI_KEY_DATA  KeyData;
  EFI_STATUS        Status;

  Keycode = (AIK_SELF *) Context;
  if (Keycode == NULL || Keycode->OurJobIsDone) {
    DEBUG ((DEBUG_INFO, "AIKPollKeyboardHandler got null handler or called when done\n"));
    return;
  }

  Keycode->InPollKeyboardEvent = TRUE;

  //
  // Counter is here for debugging purposes only.
  //
  Counter = AIKTargetRefresh (&Keycode->Target);

  //
  // Some implementations return "partial key", which is a modifier without
  // a key. When a modifier is held, we will get partial keys infinitely, so make sure
  // we break after some time.
  //
  Index = 0;

  do {
    Status = AIKSourceGrabEfiKey (
      &Keycode->Source,
      &KeyData,
      Keycode->KeyFiltering
      );
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "AIK: Key scan 0x%X uni 0x%X Shift 0x%X toggle 0x%X at %Lu\n",
        KeyData.Key.ScanCode,
        KeyData.Key.UnicodeChar,
        KeyData.KeyState.KeyShiftState,
        KeyData.KeyState.KeyToggleState,
        Counter
        ));
      AIKDataWriteEntry (&Keycode->Data, &KeyData);
      AIKTargetWriteEntry (&Keycode->Target, &KeyData);
    } else if (Status == EFI_UNSUPPORTED) {
      //
      // Filtered key, do not abort.
      //
      Status = EFI_SUCCESS;
    }

    Index++;
  } while (!EFI_ERROR (Status) && Index < AIK_KEY_POLL_LIMIT);

  AIKTargetSubmit (&Keycode->Target);

  Keycode->InPollKeyboardEvent = FALSE;
}

EFI_STATUS
AIKInstall (
  AIK_SELF  *Keycode
  )
{
  EFI_STATUS  Status;

  Status = AIKTargetInstall (
             &Keycode->Target,
             Keycode->KeyForgotThreshold,
             Keycode->KeyMergeThreshold
             );

  if (EFI_ERROR (Status)) {
    //
    // Working AppleKeyMapAggregator is not here yet.
    //
    return Status;
  }

  Status = AIKSourceInstall (&Keycode->Source, Keycode->Mode);

  if (EFI_ERROR (Status)) {
    //
    // Cannot work with these sources.
    //
    AIKTargetUninstall (&Keycode->Target);
    return Status;
  }

  AIKDataReset (&Keycode->Data);

  Status = gBS->CreateEvent (
    EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
    AIKPollKeyboardHandler,
    Keycode, &Keycode->PollKeyboardEvent
    );

  if (!EFI_ERROR (Status)) {
    Status = gBS->SetTimer (Keycode->PollKeyboardEvent, TimerPeriodic, AIK_KEY_POLL_INTERVAL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "AIKPollKeyboardHandler timer setting failed - %r\n", Status));
      gBS->CloseEvent (Keycode->PollKeyboardEvent);
      Keycode->PollKeyboardEvent = NULL;
    }
  } else {
    DEBUG ((DEBUG_INFO, "AIKPollKeyboardHandler event creation failed - %r\n", Status));
  }

  if (EFI_ERROR (Status)) {
    AIKSourceUninstall (&Keycode->Source);
    AIKTargetUninstall (&Keycode->Target);
  }

  return Status;
}

VOID
AIKUninstall (
  AIK_SELF  *Keycode
  )
{
  Keycode->OurJobIsDone = TRUE;

  AIKProtocolArriveUninstall (Keycode);

  if (Keycode->PollKeyboardEvent) {
    gBS->SetTimer (Keycode->PollKeyboardEvent, TimerCancel, 0);
    gBS->CloseEvent (Keycode->PollKeyboardEvent);
    Keycode->PollKeyboardEvent = NULL;
  }

  AIKSourceUninstall (&Keycode->Source);
  AIKTargetUninstall (&Keycode->Target);
}

EFI_STATUS
OcAppleGenericInputKeycodeInit (
  IN OC_INPUT_KEY_MODE  Mode,
  IN UINT8              KeyForgotThreshold,
  IN UINT8              KeyMergeThreshold,
  IN BOOLEAN            KeySwap,
  IN BOOLEAN            KeyFiltering
  )
{
  EFI_STATUS                Status;

  AIKTranslateConfigure (KeySwap);

  gAikSelf.Mode               = Mode;
  gAikSelf.KeyForgotThreshold = KeyForgotThreshold;
  gAikSelf.KeyMergeThreshold  = KeyMergeThreshold;
  gAikSelf.KeyFiltering       = KeyFiltering;
  Status = AIKInstall (&gAikSelf);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AIKInstall failed - %r\n", Status));

    //
    // No AppleKeyMapAggregator present, install on its availability.
    //
    Status = AIKProtocolArriveInstall (&gAikSelf);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "AIK is NOT waiting for protocols - %r\n", Status));
    }
  }

  return Status;
}

EFI_STATUS
OcAppleGenericInputKeycodeExit (
  VOID
  )
{
  AIKUninstall (&gAikSelf);
  return EFI_SUCCESS;
}

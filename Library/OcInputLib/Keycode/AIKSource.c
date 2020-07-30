/** @file
  Key provider

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AIKSource.h"
#include "AIKShim.h"

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcInputLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
AIKSourceGrabEfiKey (
  AIK_SOURCE        *Source,
  AMI_EFI_KEY_DATA  *KeyData,
  BOOLEAN           KeyFiltering
  )
{
  EFI_STATUS      Status;
  EFI_EVENT       Event;

  ZeroMem (KeyData, sizeof (*KeyData));

  if (Source->AmiKeycode != NULL && Source->AmiReadEfikey) {
    Status = Source->AmiReadEfikey (Source->AmiKeycode, KeyData);
    Event = Source->AmiKeycode->WaitForKeyEx;
  } else if (Source->TextInputEx != NULL && Source->TextReadKeyStrokeEx) {
    Status = Source->TextReadKeyStrokeEx (Source->TextInputEx, (EFI_KEY_DATA *) KeyData);
    Event = Source->TextInputEx->WaitForKeyEx;
  } else if (Source->TextInput != NULL && Source->TextReadKeyStroke) {
    Status = Source->TextReadKeyStroke (Source->TextInput, &KeyData->Key);
    Event = Source->TextInput->WaitForKey;
  } else {
    //
    // Should never happen.
    //
    Status = EFI_NOT_FOUND;
    Event  = NULL;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Some boards like GA Z77P-D3 may return uninitialised data in EFI_INPUT_KEY.
  // Try to reduce the effects by discarding definitely invalid symbols.
  // See: tables 107 and 108 in UEFI spec describing EFI Scan Codes.
  //
  if (KeyFiltering) {
    if ((KeyData->Key.UnicodeChar & ~0xFFU) != 0) {
      return EFI_UNSUPPORTED;
    }

    switch (KeyData->Key.ScanCode) {
      case SCAN_NULL:
      case SCAN_UP:
      case SCAN_DOWN:
      case SCAN_RIGHT:
      case SCAN_LEFT:
      case SCAN_HOME:
      case SCAN_END:
      case SCAN_INSERT:
      case SCAN_DELETE:
      case SCAN_PAGE_UP:
      case SCAN_PAGE_DOWN:
      case SCAN_F1:
      case SCAN_F2:
      case SCAN_F3:
      case SCAN_F4:
      case SCAN_F5:
      case SCAN_F6:
      case SCAN_F7:
      case SCAN_F8:
      case SCAN_F9:
      case SCAN_F10:
      case SCAN_ESC:
      case SCAN_F11:
      case SCAN_F12:
      case SCAN_PAUSE:
      case SCAN_F13:
      case SCAN_F14:
      case SCAN_F15:
      case SCAN_F16:
      case SCAN_F17:
      case SCAN_F18:
      case SCAN_F19:
      case SCAN_F20:
      case SCAN_F21:
      case SCAN_F22:
      case SCAN_F23:
      case SCAN_F24:
      case SCAN_MUTE:
      case SCAN_VOLUME_UP:
      case SCAN_VOLUME_DOWN:
      case SCAN_BRIGHTNESS_UP:
      case SCAN_BRIGHTNESS_DOWN:
      case SCAN_SUSPEND:
      case SCAN_HIBERNATE:
      case SCAN_TOGGLE_DISPLAY:
      case SCAN_RECOVERY:
      case SCAN_EJECT:
        break;
      default:
        return EFI_UNSUPPORTED;
    }
  }

  if (Event != NULL) {
    gBS->SignalEvent (Event);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
AIKSourceInstall (
  AIK_SOURCE         *Source,
  OC_INPUT_KEY_MODE  Mode
  )
{
  EFI_STATUS                 Status;

  if (Source->AmiKeycode != NULL || Source->TextInput != NULL || Source->TextInputEx != NULL) {
    return EFI_SUCCESS;
  }

  Source->ConSplitHandler = gST->ConsoleInHandle;

  if (Mode == OcInputKeyModeAuto || Mode == OcInputKeyModeAmi) {
    Status = gBS->HandleProtocol (Source->ConSplitHandler, &gAmiEfiKeycodeProtocolGuid,
      (VOID * *)& Source->AmiKeycode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCII: AmiEfiKeycodeProtocol is unavailable on gST->ConsoleHandle - %r\n", Status));
    }
  }

  if (Mode == OcInputKeyModeAuto || Mode == OcInputKeyModeV1) {
    Status = gBS->HandleProtocol (Source->ConSplitHandler, &gEfiSimpleTextInProtocolGuid,
      (VOID * *)& Source->TextInput);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCII: EfiSimpleTextInProtocol is unavailable on gST->ConsoleHandle - %r\n", Status));
    }
  }

  if (Mode == OcInputKeyModeAuto || Mode == OcInputKeyModeV2) {
    Status = gBS->HandleProtocol (Source->ConSplitHandler, &gEfiSimpleTextInputExProtocolGuid,
      (VOID * *)& Source->TextInputEx);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCII: EfiSimpleTextInputExProtocol is unavailable on gST->ConsoleHandle - %r\n", Status));
    }
  }

  if (Source->AmiKeycode == NULL && Source->TextInput == NULL && Source->TextInputEx == NULL) {
    DEBUG ((DEBUG_INFO, "OCII: No ConSplitter input protocol is unavailable\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "OCII: gST->ConIn %p vs found %p\n", gST->ConIn, Source->TextInput));

  if (Source->AmiKeycode) {
    Source->AmiReadEfikey                 = Source->AmiKeycode->ReadEfikey;
    Source->AmiWait                       = Source->AmiKeycode->WaitForKeyEx;
    Source->AmiKeycode->ReadEfikey        = AIKShimAmiKeycodeReadEfikey;
    Status = gBS->CreateEvent (
      EVT_NOTIFY_WAIT, TPL_NOTIFY, AIKShimWaitForKeyHandler,
      NULL, &Source->AmiKeycode->WaitForKeyEx
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCII: AmiEfiKeycodeProtocol WaitForKey replace failed - %r", Status));
      Source->AmiKeycode->WaitForKeyEx = Source->AmiWait;
      Source->AmiWait = NULL;
    }
  }

  if (Source->TextInput) {
    Source->TextReadKeyStroke             = Source->TextInput->ReadKeyStroke;
    Source->TextWait                      = Source->TextInput->WaitForKey;
    Source->TextInput->ReadKeyStroke      = AIKShimTextInputReadKeyStroke;
    Status = gBS->CreateEvent (
      EVT_NOTIFY_WAIT, TPL_NOTIFY, AIKShimWaitForKeyHandler,
      NULL, &Source->TextInput->WaitForKey
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCII: EfiSimpleTextInProtocol WaitForKey replace failed - %r", Status));
      Source->TextInput->WaitForKey = Source->TextWait;
      Source->TextWait = NULL;
    }
  }

  if (Source->TextInputEx) {
    Source->TextWaitEx                    = Source->TextInputEx->WaitForKeyEx;
    Source->TextReadKeyStrokeEx           = Source->TextInputEx->ReadKeyStrokeEx;
    Source->TextInputEx->ReadKeyStrokeEx  = AIKShimTextInputReadKeyStrokeEx;
    Status = gBS->CreateEvent (
      EVT_NOTIFY_WAIT, TPL_NOTIFY, AIKShimWaitForKeyHandler,
      NULL, &Source->TextInputEx->WaitForKeyEx
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCII: EfiSimpleTextInputExProtocol WaitForKey replace failed - %r", Status));
      Source->TextInputEx->WaitForKeyEx = Source->TextWaitEx;
      Source->TextWaitEx = NULL;
    }
  }

  return EFI_SUCCESS;
}

VOID
AIKSourceUninstall (
  AIK_SOURCE  *Source
  )
{
  if (Source->AmiKeycode) {
    Source->AmiKeycode->ReadEfikey        = Source->AmiReadEfikey;
    if (Source->AmiWait != NULL && Source->AmiWait != Source->AmiKeycode->WaitForKeyEx) {
      gBS->CloseEvent (Source->AmiKeycode->WaitForKeyEx);
      Source->AmiKeycode->WaitForKeyEx = Source->AmiWait;
    }
    Source->AmiReadEfikey  = NULL;
    Source->AmiWait        = NULL;
    Source->AmiKeycode     = NULL;
  }

  if (Source->TextInput) {
    Source->TextInput->ReadKeyStroke      = Source->TextReadKeyStroke;
    if (Source->TextWait != NULL && Source->TextWait != Source->TextInput->WaitForKey) {
      gBS->CloseEvent (Source->TextInput->WaitForKey);
      Source->TextInput->WaitForKey = Source->TextWait;
    }
    Source->TextInput         = NULL;
    Source->TextWait          = NULL;
    Source->TextReadKeyStroke = NULL;
  }

  if (Source->TextInputEx) {
    Source->TextInputEx->ReadKeyStrokeEx  = Source->TextReadKeyStrokeEx;
    if (Source->TextWaitEx != NULL && Source->TextWaitEx != Source->TextInputEx->WaitForKeyEx) {
      gBS->CloseEvent (Source->TextInputEx->WaitForKeyEx);
      Source->TextInputEx->WaitForKeyEx = Source->TextWaitEx;
    }
    Source->TextInputEx         = NULL;
    Source->TextWaitEx          = NULL;
    Source->TextReadKeyStrokeEx = NULL;
  }

  if (Source->ConSplitHandler != NULL) {
    gBS->DisconnectController (Source->ConSplitHandler, NULL, NULL);
    Source->ConSplitHandler = NULL;
  }
}


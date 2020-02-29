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
  AMI_EFI_KEY_DATA  *KeyData
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

  if (Event != NULL) {
    gBS->SignalEvent (Event);
  }

  return Status;
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
      DEBUG ((DEBUG_INFO, "AmiEfiKeycodeProtocol is unavailable on gST->ConsoleHandle - %r\n", Status));
    }
  }

  if (Mode == OcInputKeyModeAuto || Mode == OcInputKeyModeV1) {
    Status = gBS->HandleProtocol (Source->ConSplitHandler, &gEfiSimpleTextInProtocolGuid,
      (VOID * *)& Source->TextInput);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "EfiSimpleTextInProtocol is unavailable on gST->ConsoleHandle - %r\n", Status));
    }
  }

  if (Mode == OcInputKeyModeAuto || Mode == OcInputKeyModeV2) {
    Status = gBS->HandleProtocol (Source->ConSplitHandler, &gEfiSimpleTextInputExProtocolGuid,
      (VOID * *)& Source->TextInputEx);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "EfiSimpleTextInputExProtocol is unavailable on gST->ConsoleHandle - %r\n", Status));
    }
  }

  if (Source->AmiKeycode == NULL && Source->TextInput == NULL && Source->TextInputEx == NULL) {
    DEBUG ((DEBUG_INFO, "No ConSplitter input protocol is unavailable\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "gST->ConIn %p vs found %p\n", gST->ConIn, Source->TextInput));

  //
  // We additionally reset the protocols as our buffers are empty, and we do not want old data.
  //
  if (Source->AmiKeycode) {
    Source->AmiKeycode->Reset (Source->AmiKeycode, FALSE);
    Source->AmiReset                      = Source->AmiKeycode->Reset;
    Source->AmiReadEfikey                 = Source->AmiKeycode->ReadEfikey;
    Source->AmiWait                       = Source->AmiKeycode->WaitForKeyEx;
    Source->AmiKeycode->Reset             = AIKShimAmiKeycodeReset;
    Source->AmiKeycode->ReadEfikey        = AIKShimAmiKeycodeReadEfikey;
    Status = gBS->CreateEvent (
      EVT_NOTIFY_WAIT, TPL_NOTIFY, AIKShimWaitForKeyHandler,
      NULL, &Source->AmiKeycode->WaitForKeyEx
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "AmiEfiKeycodeProtocol WaitForKey replace failed - %r", Status));
      Source->AmiKeycode->WaitForKeyEx = Source->AmiWait;
      Source->AmiWait = NULL;
    }
  }

  if (Source->TextInput) {
    Source->TextInput->Reset (Source->TextInput, FALSE);
    Source->TextReset                     = Source->TextInput->Reset;
    Source->TextReadKeyStroke             = Source->TextInput->ReadKeyStroke;
    Source->TextWait                      = Source->TextInput->WaitForKey;
    Source->TextInput->Reset              = AIKShimTextInputReset;
    Source->TextInput->ReadKeyStroke      = AIKShimTextInputReadKeyStroke;
    Status = gBS->CreateEvent (
      EVT_NOTIFY_WAIT, TPL_NOTIFY, AIKShimWaitForKeyHandler,
      NULL, &Source->TextInput->WaitForKey
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "EfiSimpleTextInProtocol WaitForKey replace failed - %r", Status));
      Source->TextInput->WaitForKey = Source->TextWait;
      Source->TextWait = NULL;
    }
  }

  if (Source->TextInputEx) {
    Source->TextInputEx->Reset (Source->TextInputEx, FALSE);
    Source->TextResetEx                   = Source->TextInputEx->Reset;
    Source->TextWaitEx                    = Source->TextInputEx->WaitForKeyEx;
    Source->TextReadKeyStrokeEx           = Source->TextInputEx->ReadKeyStrokeEx;
    Source->TextInputEx->Reset            = AIKShimTextInputResetEx;
    Source->TextInputEx->ReadKeyStrokeEx  = AIKShimTextInputReadKeyStrokeEx;
    Status = gBS->CreateEvent (
      EVT_NOTIFY_WAIT, TPL_NOTIFY, AIKShimWaitForKeyHandler,
      NULL, &Source->TextInputEx->WaitForKeyEx
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "EfiSimpleTextInputExProtocol WaitForKey replace failed - %r", Status));
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
    Source->AmiKeycode->Reset             = Source->AmiReset;
    Source->AmiKeycode->ReadEfikey        = Source->AmiReadEfikey;
    if (Source->AmiWait != NULL && Source->AmiWait != Source->AmiKeycode->WaitForKeyEx) {
      gBS->CloseEvent (Source->AmiKeycode->WaitForKeyEx);
      Source->AmiKeycode->WaitForKeyEx = Source->AmiWait;
    }
    Source->AmiReset       = NULL;
    Source->AmiReadEfikey  = NULL;
    Source->AmiWait        = NULL;
    Source->AmiKeycode     = NULL;
  }

  if (Source->TextInput) {
    Source->TextInput->Reset              = Source->TextReset;
    Source->TextInput->ReadKeyStroke      = Source->TextReadKeyStroke;
    if (Source->TextWait != NULL && Source->TextWait != Source->TextInput->WaitForKey) {
      gBS->CloseEvent (Source->TextInput->WaitForKey);
      Source->TextInput->WaitForKey = Source->TextWait;
    }
    Source->TextInput         = NULL;
    Source->TextReset         = NULL;
    Source->TextWait          = NULL;
    Source->TextReadKeyStroke = NULL;
  }

  if (Source->TextInputEx) {
    Source->TextInputEx->Reset            = Source->TextResetEx;
    Source->TextInputEx->ReadKeyStrokeEx  = Source->TextReadKeyStrokeEx;
    if (Source->TextWaitEx != NULL && Source->TextWaitEx != Source->TextInputEx->WaitForKeyEx) {
      gBS->CloseEvent (Source->TextInputEx->WaitForKeyEx);
      Source->TextInputEx->WaitForKeyEx = Source->TextWaitEx;
    }
    Source->TextInputEx         = NULL;
    Source->TextResetEx         = NULL;
    Source->TextWaitEx          = NULL;
    Source->TextReadKeyStrokeEx = NULL;
  }

  if (Source->ConSplitHandler != NULL) {
    gBS->DisconnectController (Source->ConSplitHandler, NULL, NULL);
    Source->ConSplitHandler = NULL;
  }
}


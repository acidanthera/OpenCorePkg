/** @file
  Key shimming code

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AIK.h"
#include "AIKShim.h"

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
AIKShimAmiKeycodeReadEfikey (
  IN  AMI_EFIKEYCODE_PROTOCOL  *This,
  OUT AMI_EFI_KEY_DATA         *KeyData
  )
{
  DEBUG ((DEBUG_VERBOSE, "AIKAmiKeycodeReadEfikey %p %p ours %p event %d",
    This, KeyData, gAikSelf.Source.AmiKeycode, gAikSelf.InPollKeyboardEvent));

  if (This == NULL || KeyData == NULL || gAikSelf.OurJobIsDone) {
    return EFI_INVALID_PARAMETER;
  }

  if (This == gAikSelf.Source.AmiKeycode && !gAikSelf.InPollKeyboardEvent) {
    //
    // Do not touch any protocol but ours.
    //
    return AIKDataReadEntry (&gAikSelf.Data, KeyData);
  }

  return gAikSelf.Source.AmiReadEfikey (This, KeyData);
}

EFI_STATUS
EFIAPI
AIKShimTextInputReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
{
  EFI_STATUS        Status;
  AMI_EFI_KEY_DATA  AmiKeyData;

  DEBUG ((DEBUG_VERBOSE, "AIKTextInputReadKeyStroke %p %p ours %p event %d",
    This, Key, gAikSelf.Source.TextInput, gAikSelf.InPollKeyboardEvent));

  if (This == NULL || Key == NULL || gAikSelf.OurJobIsDone) {
    return EFI_INVALID_PARAMETER;
  }

  if (This == gAikSelf.Source.TextInput && !gAikSelf.InPollKeyboardEvent) {
    //
    // Do not touch any protocol but ours.
    //
    Status = AIKDataReadEntry (&gAikSelf.Data, &AmiKeyData);
    if (!EFI_ERROR (Status)) {
      //
      // 'Partial' keys should not be returned by SimpleTextInput protocols.
      //
      if (AmiKeyData.Key.ScanCode == 0 && AmiKeyData.Key.UnicodeChar == 0
        && (AmiKeyData.KeyState.KeyToggleState & KEY_STATE_EXPOSED)) {
        Status = EFI_NOT_READY;
      } else {
        CopyMem (Key, &AmiKeyData.Key, sizeof (AmiKeyData.Key));
      }
    }

    return Status;
  }

  return gAikSelf.Source.TextReadKeyStroke (This, Key);
}

EFI_STATUS
EFIAPI
AIKShimTextInputReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  EFI_STATUS        Status;
  AMI_EFI_KEY_DATA  AmiKeyData;

  DEBUG ((DEBUG_VERBOSE, "AIKTextInputReadKeyStrokeEx %p %p ours %p event %d",
    This, KeyData, gAikSelf.Source.TextInputEx, gAikSelf.InPollKeyboardEvent));

  if (This == NULL || KeyData == NULL || gAikSelf.OurJobIsDone) {
    return EFI_INVALID_PARAMETER;
  }

  if (This == gAikSelf.Source.TextInputEx && !gAikSelf.InPollKeyboardEvent) {
    //
    // Do not touch any protocol but ours.
    //
    Status = AIKDataReadEntry (&gAikSelf.Data, &AmiKeyData);
    if (!EFI_ERROR (Status)) {
      CopyMem (&KeyData->Key, &AmiKeyData.Key, sizeof (AmiKeyData.Key));
      CopyMem (&KeyData->KeyState, &AmiKeyData.KeyState, sizeof (AmiKeyData.KeyState));
    }

    return Status;
  }

  return gAikSelf.Source.TextReadKeyStrokeEx (This, KeyData);
}

VOID
EFIAPI
AIKShimWaitForKeyHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if (gAikSelf.OurJobIsDone) {
    DEBUG ((DEBUG_INFO, "AIKShimWaitForKeyHandler got null handler or called when done\n"));
    return;
  }

  if (!AIKDataEmpty (&gAikSelf.Data)) {
    DEBUG ((DEBUG_VERBOSE, "Caught KeyBufferSize non-zero\n"));
    gBS->SignalEvent (Event);
  }
}

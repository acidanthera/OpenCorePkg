/** @file
  Key consumer

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AIKTarget.h"
#include "AIKTranslate.h"

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
AIKTargetInstall (
  IN OUT AIK_TARGET  *Target,
  IN     UINT8       KeyForgotThreshold
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  Target->KeyForgotThreshold = KeyForgotThreshold;

  if (Target->KeyMapDb == NULL) {
    Status = gBS->LocateProtocol (&gAppleKeyMapDatabaseProtocolGuid, NULL, (VOID **) &Target->KeyMapDb);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "AppleKeyMapDatabaseProtocol is unavailable - %r\n", Status));
      return EFI_NOT_FOUND;
    }

    Status = Target->KeyMapDb->CreateKeyStrokesBuffer (
      Target->KeyMapDb, AIK_TARGET_BUFFER_SIZE, &Target->KeyMapDbIndex
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "CreateKeyStrokesBuffer failed - %r\n", Status));
      Target->KeyMapDb = NULL;
    }
  }

  return Status;
}

VOID
AIKTargetUninstall (
  IN OUT AIK_TARGET  *Target
  )
{
  if (Target->KeyMapDb != NULL) {
    Target->KeyMapDb->RemoveKeyStrokesBuffer (Target->KeyMapDb, Target->KeyMapDbIndex);
    Target->KeyMapDb = NULL;
  }

  Target->NumberOfKeys = 0;
  Target->Modifiers = 0;
  Target->ModifierCounter = 0;
  ZeroMem (Target->Keys, sizeof (Target->Keys));
  ZeroMem (Target->KeyCounters, sizeof (Target->KeyCounters));
}

UINT64
AIKTargetRefresh (
  IN OUT AIK_TARGET  *Target
  )
{
  UINTN  Index;
  UINTN  Left;

  Target->Counter++;

  for (Index = 0; Index < Target->NumberOfKeys; Index++) {
    //
    // We reported this key Target->KeyForgetThreshold times, time to say goodbye.
    //
    if (Target->KeyCounters[Index] + Target->KeyForgotThreshold <= Target->Counter) {
      Left = Target->NumberOfKeys - (Index + 1);
      if (Left > 0) {
        CopyMem (
          &Target->KeyCounters[Index],
          &Target->KeyCounters[Index+1],
          sizeof (Target->KeyCounters[0]) * Left);
        CopyMem (
          &Target->Keys[Index],
          &Target->Keys[Index+1],
          sizeof (Target->Keys[0]) * Left);
      }
      Target->NumberOfKeys--;
    }
  }

  //
  // No keys were pressed, so we did not enter AIKTargetWriteEntry.
  // However, we still need to reset modifiers after time.
  //
  if (Target->ModifierCounter + Target->KeyForgotThreshold <= Target->Counter) {
    Target->Modifiers = 0;
  }

  return Target->Counter;
}

VOID
AIKTargetWriteEntry (
  IN OUT AIK_TARGET        *Target,
  IN     AMI_EFI_KEY_DATA  *KeyData
  )
{
  APPLE_MODIFIER_MAP  Modifiers;
  APPLE_KEY_CODE      Key;
  UINTN               Index;
  UINTN               InsertIndex;
  UINT64              OldestCounter;

  AIKTranslate (KeyData, &Modifiers, &Key);

  Target->Modifiers = Modifiers;
  Target->ModifierCounter = Target->Counter;

  if (Key == UsbHidUndefined) {
    //
    // This is just a modifier or an unsupported key.
    //
    return;
  }

  for (Index = 0; Index < Target->NumberOfKeys; Index++) {
    if (Target->Keys[Index] == Key) {
      //
      // This key was added previously, just update its counter.
      //
      Target->KeyCounters[Index] = Target->Counter;
      return;
    }
  }

  InsertIndex = Target->NumberOfKeys;

  //
  // This should not happen, but we have no room, replace the oldest key.
  //
  if (InsertIndex == AIK_TARGET_BUFFER_SIZE) {
    InsertIndex   = 0;
    OldestCounter = Target->KeyCounters[InsertIndex];
    for (Index = 1; Index < Target->NumberOfKeys; Index++) {
      if (OldestCounter > Target->KeyCounters[Index]) {
        OldestCounter = Target->KeyCounters[Index];
        InsertIndex   = Index;
      }
    }
    Target->NumberOfKeys--;
  }

  //
  // Insert the new key
  //
  Target->Keys[InsertIndex] = Key;
  Target->KeyCounters[InsertIndex] = Target->Counter;
  Target->NumberOfKeys++;
}

VOID
AIKTargetSubmit (
  IN OUT AIK_TARGET  *Target
  )
{
  EFI_STATUS  Status;

  if (Target->KeyMapDb != NULL) {
    Status = Target->KeyMapDb->SetKeyStrokeBufferKeys (
      Target->KeyMapDb,
      Target->KeyMapDbIndex,
      Target->Modifiers,
      Target->NumberOfKeys,
      Target->Keys
      );
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to submit keys to AppleMapDb - %r", Status));
  }
}

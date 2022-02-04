/** @file
  AppleKeyMapAggregator edge detection

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Protocol/AppleKeyMapAggregator.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcTimerLib.h>
#include <Library/TimerLib.h>

VOID
OcFreeKeyRepeatContext (
  OC_KEY_REPEAT_CONTEXT                     **Context
  )
{
  ASSERT (Context != NULL);

  if (*Context == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "OCKM: Freeing key repeat context %p %p %p\n", *Context, (*Context)->KeysHeld, (*Context)->KeyHeldTimes));

  FreePool ((*Context)->KeyHeldTimes);
  FreePool ((*Context)->KeysHeld);
  FreePool (*Context);

  *Context = NULL;
}

EFI_STATUS
OcInitKeyRepeatContext (
     OUT OC_KEY_REPEAT_CONTEXT              **Context,
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN     UINTN                              MaxKeysHeld,
  IN     UINT64                             InitialDelay,
  IN     UINT64                             SubsequentDelay,
  IN     BOOLEAN                            PreventInitialRepeat
  )
{
  EFI_STATUS                        Status;
  APPLE_MODIFIER_MAP                Modifiers;
  UINTN                             NumKeysUp;
  UINTN                             NumKeysDown;

  DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: Allocating %d for repeat context\n", sizeof(OC_KEY_REPEAT_CONTEXT)));
  *Context = AllocatePool(sizeof(OC_KEY_REPEAT_CONTEXT));
  if (*Context == NULL) {
    DEBUG ((DEBUG_ERROR, "OCKM: Cannot allocate repeat context\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if (MaxKeysHeld == 0) {
    (*Context)->KeysHeld = NULL;  
  } else {
    DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: Allocating %d for keys held\n", MaxKeysHeld * sizeof((*Context)->KeysHeld[0])));
    (*Context)->KeysHeld = AllocatePool(MaxKeysHeld * sizeof((*Context)->KeysHeld[0]));
    if ((*Context)->KeysHeld == NULL) {
      DEBUG ((DEBUG_ERROR, "OCKM: Cannot allocate keys held\n"));
      FreePool (*Context);
      *Context = NULL;
      return EFI_OUT_OF_RESOURCES;
    }

    DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: Allocating %d for key held times\n", MaxKeysHeld * sizeof((*Context)->KeyHeldTimes[0])));
    (*Context)->KeyHeldTimes = AllocatePool(MaxKeysHeld * sizeof((*Context)->KeyHeldTimes[0]));
    if ((*Context)->KeyHeldTimes == NULL) {
      DEBUG ((DEBUG_ERROR, "OCKM: Cannot allocate key held times\n"));
      FreePool ((*Context)->KeysHeld);
      FreePool (*Context);
      *Context = NULL;
      return EFI_OUT_OF_RESOURCES;
    }
  }
 
  (*Context)->KeyMap           = KeyMap;

  (*Context)->NumKeysHeld      = 0;
  (*Context)->MaxKeysHeld      = MaxKeysHeld;

  (*Context)->InitialDelay     = InitialDelay;
  (*Context)->SubsequentDelay  = SubsequentDelay;
  (*Context)->PreviousTime     = 0;

  if (MaxKeysHeld == 0 || PreventInitialRepeat == FALSE) {
    Status = EFI_SUCCESS;
  } else {
    //
    // Prevent any keys which are down at init from appearring as immediate down key strokes or even repeating until released and then pressed again
    //
    NumKeysDown = MaxKeysHeld;

    Status = OcGetUpDownKeys (
      *Context,
      &Modifiers,
      &NumKeysUp, NULL,
      &NumKeysDown, NULL,
      MAX_INT64 >> 1 // == far future, creates massive but manageable negative delta on next call
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCKM: InitKeyRepeatContext initial GetUpDownKeys call - %r\n", Status));
      OcFreeKeyRepeatContext (Context);
      return Status;
    }
  }

  DEBUG ((DEBUG_INFO, "OCKM: Allocated key repeat context %p %p %p\n", *Context, (*Context)->KeysHeld, (*Context)->KeyHeldTimes));

  return Status;
}

EFI_STATUS
EFIAPI
OcGetUpDownKeys (
  IN OUT OC_KEY_REPEAT_CONTEXT              *RepeatContext,
     OUT APPLE_MODIFIER_MAP                 *Modifiers,
  IN OUT UINTN                              *NumKeysUp,
     OUT APPLE_KEY_CODE                     *KeysUp       OPTIONAL,
  IN OUT UINTN                              *NumKeysDown,
     OUT APPLE_KEY_CODE                     *KeysDown     OPTIONAL,
  IN     UINT64                             CurrentTime
  )
{
  EFI_STATUS                        Status;
  UINTN                             NumRawKeys;
  APPLE_KEY_CODE                    RawKeys[OC_KEY_MAP_DEFAULT_SIZE];
  UINTN                             NumKeysHeldInCopy;
  UINTN                             MaxKeysHeldInCopy;
  APPLE_KEY_CODE                    KeysHeldCopy[OC_HELD_KEYS_DEFAULT_SIZE];
  INT64                             KeyHeldTimesCopy[OC_HELD_KEYS_DEFAULT_SIZE];
  UINTN                             Index;
  UINTN                             Index2;
  APPLE_KEY_CODE                    Key;
  INT64                             KeyTime;
  UINT64                            DeltaTime;
  BOOLEAN                           FoundHeldKey;

  ASSERT (Modifiers != NULL);
  ASSERT (NumKeysUp != NULL);
  ASSERT (NumKeysDown != NULL);
  ASSERT (RepeatContext != NULL);
  ASSERT (RepeatContext->KeyMap != NULL);
  ASSERT (RepeatContext->NumKeysHeld <= RepeatContext->MaxKeysHeld);

  ASSERT_EQUALS (RepeatContext->KeysHeld == NULL, RepeatContext->KeyHeldTimes == NULL);

  DeltaTime = CurrentTime - RepeatContext->PreviousTime;
  RepeatContext->PreviousTime = CurrentTime;

  if (RepeatContext->KeysHeld != NULL) {
    //
    // All held keys could potentially go into keys up
    // 
    if (KeysUp != NULL && RepeatContext->MaxKeysHeld > *NumKeysUp) {
      DEBUG ((DEBUG_ERROR, "OCKM: MaxKeysHeld %d exceeds NumKeysUp %d\n", RepeatContext->MaxKeysHeld, *NumKeysUp));
      return EFI_UNSUPPORTED;
    }

    //
    // All requested keys could potentially go into held keys
    // (NumKeysDown is always used as the requested number of keys to scan, even if there is no KeysDown buffer to return down keycodes)
    //
    if (KeysDown != NULL && *NumKeysDown > RepeatContext->MaxKeysHeld) {
      DEBUG ((DEBUG_ERROR, "OCKM: Number of keys requested %d exceeds MaxKeysHeld %d\n", *NumKeysDown, RepeatContext->MaxKeysHeld));
      return EFI_UNSUPPORTED;
    }

    //
    // Clone live entries of KeysHeld buffer 
    //
    MaxKeysHeldInCopy = ARRAY_SIZE (KeysHeldCopy);
    if (RepeatContext->MaxKeysHeld > MaxKeysHeldInCopy) {
      DEBUG ((DEBUG_ERROR, "OCKM: MaxKeysHeld %d exceeds supported copy space %d\n", RepeatContext->MaxKeysHeld, MaxKeysHeldInCopy));
      return EFI_UNSUPPORTED;
    }

    CopyMem (KeysHeldCopy, RepeatContext->KeysHeld, RepeatContext->NumKeysHeld * sizeof(RepeatContext->KeysHeld[0]));
    CopyMem (KeyHeldTimesCopy, RepeatContext->KeyHeldTimes, RepeatContext->NumKeysHeld * sizeof(RepeatContext->KeyHeldTimes[0]));
    NumKeysHeldInCopy = RepeatContext->NumKeysHeld;
  } else {
    NumKeysHeldInCopy = 0;
  }

  NumRawKeys  = ARRAY_SIZE (RawKeys);
  if (*NumKeysDown > NumRawKeys) {
    DEBUG ((DEBUG_ERROR, "OCKM: Number of keys requested %d exceeds supported raw key bufsize %d\n", *NumKeysDown, NumRawKeys));
    return EFI_UNSUPPORTED;
  }

  Status = RepeatContext->KeyMap->GetKeyStrokes (
    RepeatContext->KeyMap,
    Modifiers,
    &NumRawKeys,
    RawKeys
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (KeysDown != NULL && NumRawKeys > *NumKeysDown) {
    return EFI_BUFFER_TOO_SMALL;
  }

  DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: [%p] %ld %ld %ld\n", RepeatContext, RepeatContext->InitialDelay, RepeatContext->SubsequentDelay, DeltaTime));
  DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: [%p] I u:%d d:%d n:%d h:%d r:%d m:%d\n", RepeatContext, *NumKeysUp, *NumKeysDown, RepeatContext->MaxKeysHeld, RepeatContext->NumKeysHeld, NumRawKeys, *Modifiers));

  *NumKeysUp = 0;
  *NumKeysDown = 0;
  RepeatContext->NumKeysHeld = 0;

  //
  // Loop through all keys which are currently down
  //
  for (Index = 0; Index < NumRawKeys; ++Index) {
    Key = RawKeys[Index];
    if (RepeatContext->KeysHeld != NULL) {
      RepeatContext->KeysHeld[RepeatContext->NumKeysHeld] = Key;
    }

    FoundHeldKey = FALSE;
    for (Index2 = 0; Index2 < NumKeysHeldInCopy; Index2++) {
      if (KeysHeldCopy[Index2] == Key) {
        FoundHeldKey = TRUE;
        KeyTime = KeyHeldTimesCopy[Index2] + DeltaTime;
        KeysHeldCopy[Index2] = 0; // Mark held key as still down
        DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: [%p] Still down 0x%X %ld\n", RepeatContext, Key, KeyTime));
        if (RepeatContext->InitialDelay != 0 && KeyTime >= 0) {
          if (KeysDown != NULL) {
            KeysDown[*NumKeysDown] = Key;
          }
          (*NumKeysDown)++;
          KeyTime -= RepeatContext->SubsequentDelay;
          DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: [%p] Repeating 0x%X %ld\n", RepeatContext, Key, KeyTime));
        }
        break;
      }
    }

    if (!FoundHeldKey) {
      if (KeysDown != NULL) {
        KeysDown[*NumKeysDown] = Key;
      }
      (*NumKeysDown)++;
      KeyTime = -(INT64) RepeatContext->InitialDelay;
      DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: [%p] New down 0x%X %ld\n", RepeatContext, Key, KeyTime));
    }

    if (RepeatContext->KeysHeld != NULL) {
      RepeatContext->KeyHeldTimes[RepeatContext->NumKeysHeld] = KeyTime;
      RepeatContext->NumKeysHeld++;
    }
  }

  //
  // Process remaining held keys
  //
  for (Index = 0; Index < NumKeysHeldInCopy; Index++) {
    Key = KeysHeldCopy[Index];
    if (Key != 0) {
      DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: [%p] Gone up 0x%X %ld\n", RepeatContext, Key, KeyHeldTimesCopy[Index]));
      if (KeysUp != NULL) {
        KeysUp[*NumKeysUp] = Key;
      }
      (*NumKeysUp)++;
    }
  }

  DEBUG ((OC_TRACE_UPDOWNKEYS, "OCKM: [%p] O u:%d d:%d n:%d h:%d r:%d\n", RepeatContext, *NumKeysUp, *NumKeysDown, RepeatContext->MaxKeysHeld, RepeatContext->NumKeysHeld, NumRawKeys));

  return Status;
}

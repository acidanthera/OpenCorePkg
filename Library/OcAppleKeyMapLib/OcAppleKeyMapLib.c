/** @file

AppleKeyMapAggregator

Copyright (c) 2018, vit9696. All rights reserved.
Additions (downkeys support) copyright (c) 2021 Mike Beaton. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if defined(OC_TARGET_DEBUG) || defined(OC_TARGET_NOOPT)
//#define DEBUG_DETAIL
#endif
#define OC_SHOW_RUNNING_KEYS

#include <AppleMacEfi.h>
#include <IndustryStandard/AppleHid.h>
#include <Protocol/AppleKeyMapAggregator.h>
#include <Protocol/AppleKeyMapDatabase.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#if defined(OC_SHOW_RUNNING_KEYS)
#include <Library/OcBootManagementLib.h>
#include <Library/UefiLib.h>
#endif

#if defined(OC_SHOW_RUNNING_KEYS)
STATIC INT32                mRunningColumn;

#define MAX_RUNNING_COLUMN  80

#define UP_DOWN_HELD_DISPLAY_OFFSET   4
#define ID_DISPLAY_OFFSET             UP_DOWN_HELD_DISPLAY_OFFSET
#define X_DISPLAY_OFFSET              5
#define MODIFIER_DISPLAY_OFFSET       6
#endif

// KEY_MAP_AGGREGATOR_DATA_SIGNATURE
#define KEY_MAP_AGGREGATOR_DATA_SIGNATURE  \
  SIGNATURE_32 ('K', 'e', 'y', 'A')

// KEY_MAP_AGGREGATOR_DATA_FROM_AGGREGATOR_THIS
#define KEY_MAP_AGGREGATOR_DATA_FROM_AGGREGATOR_THIS(This)  \
  CR (                                                      \
    (This),                                                 \
    KEY_MAP_AGGREGATOR_DATA,                                \
    Aggregator,                                             \
    KEY_MAP_AGGREGATOR_DATA_SIGNATURE                       \
    )

// KEY_MAP_AGGREGATOR_DATA_FROM_DATABASE_THIS
#define KEY_MAP_AGGREGATOR_DATA_FROM_DATABASE_THIS(This)  \
  CR (                                                    \
    (This),                                               \
    KEY_MAP_AGGREGATOR_DATA,                              \
    Database,                                             \
    KEY_MAP_AGGREGATOR_DATA_SIGNATURE                     \
    )

// KEY_MAP_AGGREGATOR_DATA
typedef struct {
  UINTN                             Signature;
  UINTN                             NextKeyStrokeIndex;
  APPLE_KEY_CODE                    *KeyCodeBuffer;
  UINTN                             KeyCodeBufferLength;
  LIST_ENTRY                        KeyStrokesInfoList;
  APPLE_KEY_MAP_DATABASE_PROTOCOL   Database;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL Aggregator;
} KEY_MAP_AGGREGATOR_DATA;

// APPLE_KEY_STROKES_INFO_SIGNATURE
#define APPLE_KEY_STROKES_INFO_SIGNATURE  SIGNATURE_32 ('K', 'e', 'y', 'S')

// APPLE_KEY_STROKES_INFO_FROM_LIST_ENTRY
#define APPLE_KEY_STROKES_INFO_FROM_LIST_ENTRY(Entry)  \
  CR (                                                 \
    (Entry),                                           \
    APPLE_KEY_STROKES_INFO,                            \
    Link,                                              \
    APPLE_KEY_STROKES_INFO_SIGNATURE                   \
    )

#define SIZE_OF_APPLE_KEY_STROKES_INFO  \
  OFFSET_OF (APPLE_KEY_STROKES_INFO, KeyCodes)

// APPLE_KEY_STROKES_INFO
typedef struct {
  UINTN              Signature;
  LIST_ENTRY         Link;
  UINTN              Index;
  UINTN              KeyCodeBufferLength;
  UINTN              NumberOfKeyCodes;
  APPLE_MODIFIER_MAP Modifiers;
  APPLE_KEY_CODE     KeyCodes[];
} APPLE_KEY_STROKES_INFO;

// InternalGetKeyStrokesByIndex
STATIC
APPLE_KEY_STROKES_INFO *
InternalGetKeyStrokesByIndex (
  IN KEY_MAP_AGGREGATOR_DATA  *KeyMapAggregatorData,
  IN UINTN                    Index
  )
{
  APPLE_KEY_STROKES_INFO *KeyStrokesInfo;

  LIST_ENTRY             *Entry;
  APPLE_KEY_STROKES_INFO *KeyStrokesInfoWalker;

  KeyStrokesInfo = NULL;

  for (
    Entry = GetFirstNode (&KeyMapAggregatorData->KeyStrokesInfoList);
    !IsNull (&KeyMapAggregatorData->KeyStrokesInfoList, Entry);
    Entry = GetNextNode (&KeyMapAggregatorData->KeyStrokesInfoList, Entry)
    ) {
    KeyStrokesInfoWalker = APPLE_KEY_STROKES_INFO_FROM_LIST_ENTRY (Entry);

    if (KeyStrokesInfoWalker->Index == Index) {
      KeyStrokesInfo = KeyStrokesInfoWalker;

      break;
    }
  }

  return KeyStrokesInfo;
}

EFI_STATUS
OcInitKeyRepeatContext(
     OUT OC_KEY_REPEAT_CONTEXT              *Context,
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN     UINTN                              MaxKeysHeld,
  IN     OC_HELD_KEY_INFO                   *KeysHeld       OPTIONAL,
  IN     UINT64                             InitialDelay,
  IN     UINT64                             SubsequentDelay,
  IN     BOOLEAN                            PreventInitialRepeat
  )
{
  EFI_STATUS                        Status;
  APPLE_MODIFIER_MAP                Modifiers;
  UINTN                             NumKeysUp;
  UINTN                             NumKeysDown;
 
  Context->KeyMap           = KeyMap;

  Context->NumKeysHeld      = 0;
  Context->MaxKeysHeld      = MaxKeysHeld;
  Context->KeysHeld         = KeysHeld;

  Context->InitialDelay     = InitialDelay;
  Context->SubsequentDelay  = SubsequentDelay;
  Context->PreviousTime     = 0;

  if (KeysHeld == NULL || PreventInitialRepeat == FALSE) {
    Status = EFI_SUCCESS;
  } else {
    //
    // Prevent any keys which are down at init from appearring as immediate down key strokes or even repeating until released and then pressed again
    //
    NumKeysDown = MaxKeysHeld;

    Status = OcGetUpDownKeys (
      Context,
      &Modifiers,
      &NumKeysUp, NULL,
      &NumKeysDown, NULL,
      MAX_INT64 >> 1 // == far future, creates massive but manageable -ve delta on next call
      );
  }

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
  OC_HELD_KEY_INFO                  KeysHeldCopy[OC_HELD_KEYS_DEFAULT_SIZE];
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

  DETAIL_DEBUG ((DEBUG_INFO, "OCKM: [%p] %ld %ld %ld\n", RepeatContext, RepeatContext->InitialDelay, RepeatContext->SubsequentDelay, DeltaTime));
  DETAIL_DEBUG ((DEBUG_INFO, "OCKM: [%p] I u:%d d:%d n:%d h:%d r:%d m:%d\n", RepeatContext, *NumKeysUp, *NumKeysDown, RepeatContext->MaxKeysHeld, RepeatContext->NumKeysHeld, NumRawKeys, *Modifiers));

  *NumKeysUp = 0;
  *NumKeysDown = 0;
  RepeatContext->NumKeysHeld = 0;

  //
  // Loop through all keys which are currently down
  //
  for (Index = 0; Index < NumRawKeys; ++Index) {
    Key = RawKeys[Index];
    if (RepeatContext->KeysHeld != NULL) {
      RepeatContext->KeysHeld[RepeatContext->NumKeysHeld].Key = Key;
    }

    FoundHeldKey = FALSE;
    for (Index2 = 0; Index2 < NumKeysHeldInCopy; Index2++) {
      if (KeysHeldCopy[Index2].Key == Key) {
        FoundHeldKey = TRUE;
        KeyTime = KeysHeldCopy[Index2].KeyTime + DeltaTime;
        KeysHeldCopy[Index2].Key = 0; // Mark held key as still down
        DETAIL_DEBUG ((DEBUG_INFO, "OCKM: [%p] Still down 0x%X %ld\n", RepeatContext, Key, KeyTime));
        if (RepeatContext->InitialDelay != 0 && KeyTime >= 0) {
          if (KeysDown != NULL) {
            KeysDown[*NumKeysDown] = Key;
          }
          (*NumKeysDown)++;
          KeyTime -= RepeatContext->SubsequentDelay;
          DETAIL_DEBUG ((DEBUG_INFO, "OCKM: [%p] Repeating 0x%X %ld\n", RepeatContext, Key, KeyTime));
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
      DETAIL_DEBUG ((DEBUG_INFO, "OCKM: [%p] New down 0x%X %ld\n", RepeatContext, Key, KeyTime));
    }

    if (RepeatContext->KeysHeld != NULL) {
      RepeatContext->KeysHeld[RepeatContext->NumKeysHeld].KeyTime = KeyTime;
      RepeatContext->NumKeysHeld++;
    }
  }

  //
  // Process remaining held keys
  //
  for (Index = 0; Index < NumKeysHeldInCopy; Index++) {
    Key = KeysHeldCopy[Index].Key;
    if (Key != 0) {
      DETAIL_DEBUG ((DEBUG_INFO, "OCKM: [%p] Gone up 0x%X %ld\n", RepeatContext, Key, KeysHeldCopy[Index].KeyTime));
      if (KeysUp != NULL) {
        KeysUp[*NumKeysUp] = Key;
      }
      (*NumKeysUp)++;
    }
  }

  DETAIL_DEBUG ((DEBUG_INFO, "OCKM: [%p] O u:%d d:%d n:%d h:%d r:%d\n", RepeatContext, *NumKeysUp, *NumKeysDown, RepeatContext->MaxKeysHeld, RepeatContext->NumKeysHeld, NumRawKeys));

  return Status;
}

// InternalGetKeyStrokes
/** Returns all pressed keys and key modifiers into the appropiate buffers.

  @param[in]     This              A pointer to the protocol instance.
  @param[out]    Modifiers         The modifiers manipulating the given keys.
  @param[in,out] NumberOfKeyCodes  On input the number of keys allocated.
                                   On output the number of keys returned into
                                   KeyCodes.
  @param[out]    KeyCodes          A Pointer to a caller-allocated buffer in
                                   which the pressed keys get returned.

  @retval EFI_SUCCESS              The pressed keys have been returned into
                                   KeyCodes.
  @retval EFI_BUFFER_TOO_SMALL     The memory required to return the value exceeds
                                   the size of the allocated Buffer.
                                   The required number of keys to allocate to
                                   complete the operation has been returned into
                                   NumberOfKeyCodes.
  @retval other                    An error returned by a sub-operation.
**/
STATIC
EFI_STATUS
EFIAPI
InternalGetKeyStrokes (
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *This,
     OUT APPLE_MODIFIER_MAP                 *Modifiers,
  IN OUT UINTN                              *NumberOfKeyCodes,
     OUT APPLE_KEY_CODE                     *KeyCodes OPTIONAL
  )
{
  EFI_STATUS              Status;

  KEY_MAP_AGGREGATOR_DATA *KeyMapAggregatorData;
  LIST_ENTRY              *Entry;
  APPLE_KEY_STROKES_INFO  *KeyStrokesInfo;
  BOOLEAN                 Result;
  APPLE_MODIFIER_MAP      DbModifiers;
  UINTN                   DbNumberOfKeyCodestrokes;
  UINTN                   Index;
  UINTN                   Index2;
  APPLE_KEY_CODE          Key;

  KeyMapAggregatorData = KEY_MAP_AGGREGATOR_DATA_FROM_AGGREGATOR_THIS (This);

  DbModifiers              = 0;
  DbNumberOfKeyCodestrokes = 0;

  for (
    Entry = GetFirstNode (&KeyMapAggregatorData->KeyStrokesInfoList);
    !IsNull (&KeyMapAggregatorData->KeyStrokesInfoList, Entry);
    Entry = GetNextNode (&KeyMapAggregatorData->KeyStrokesInfoList, Entry)
    ) {
    KeyStrokesInfo = APPLE_KEY_STROKES_INFO_FROM_LIST_ENTRY (Entry);

    DbModifiers |= KeyStrokesInfo->Modifiers;

    for (Index = 0; Index < KeyStrokesInfo->NumberOfKeyCodes; ++Index) {
      Key = KeyStrokesInfo->KeyCodes[Index];

      for (Index2 = 0; Index2 < DbNumberOfKeyCodestrokes; ++Index2) {
        if (KeyMapAggregatorData->KeyCodeBuffer[Index2] == Key) {
          break;
        }
      }

      if (Index2 == DbNumberOfKeyCodestrokes) {
        KeyMapAggregatorData->KeyCodeBuffer[DbNumberOfKeyCodestrokes] = Key;
        ++DbNumberOfKeyCodestrokes;
      }
    }
  }

  Result = (BOOLEAN)(DbNumberOfKeyCodestrokes > *NumberOfKeyCodes);

  *NumberOfKeyCodes = DbNumberOfKeyCodestrokes;

  Status = EFI_BUFFER_TOO_SMALL;

  if (!Result) {
    *Modifiers = DbModifiers;

    Status = EFI_SUCCESS;

    if (KeyCodes != NULL) {
      CopyMem (
        (VOID *)KeyCodes,
        (VOID *)KeyMapAggregatorData->KeyCodeBuffer,
        (DbNumberOfKeyCodestrokes * sizeof (*KeyCodes))
        );
    }
  }

  return Status;
}

BOOLEAN
OcKeyMapHasKeys (
  IN CONST APPLE_KEY_CODE  *Keys,
  IN UINTN                 NumKeys,
  IN CONST APPLE_KEY_CODE  *CheckKeys,
  IN UINTN                 NumCheckKeys,
  IN BOOLEAN               ExactMatch
  )
{
  UINTN CheckIndex;
  UINTN Index;

  if (ExactMatch && NumKeys != NumCheckKeys) {
    return FALSE;
  }

  for (CheckIndex = 0; CheckIndex < NumCheckKeys; ++CheckIndex) {
    for (Index = 0; Index < NumKeys; ++Index) {
      if (CheckKeys[CheckIndex] == Keys[Index]) {
        break;
      }
    }

    if (NumKeys == Index) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
OcKeyMapHasKey (
  IN CONST APPLE_KEY_CODE  *Keys,
  IN UINTN                 NumKeys,
  IN CONST APPLE_KEY_CODE  KeyCode
  )
{
  return OcKeyMapHasKeys (Keys, NumKeys, &KeyCode, 1, FALSE);
}

#if defined(OC_SHOW_RUNNING_KEYS)
//
// Defined here not in HotKeySupport.c to allow successful link to
// OcKeyMapFlush in apps that only need OcAppleKeyMapLib.
//
VOID
OcShowRunningKeys (
  UINTN                     NumKeysUp,
  UINTN                     NumKeysDown,
  UINTN                     NumKeysHeld,
  APPLE_MODIFIER_MAP        Modifiers,
  CHAR16                    CallerID,
  BOOLEAN                   UsingDownkeys
  )
{
  INT32                              RestoreRow;
  INT32                              RestoreColumn;

  CHAR16                             Code[3];

  Code[1]        = L' ';
  Code[2]        = L'\0';

  if (mRunningColumn < 0 || mRunningColumn >= MAX_RUNNING_COLUMN) {
    mRunningColumn = 0;
  }

  RestoreRow     = gST->ConOut->Mode->CursorRow;
  RestoreColumn  = gST->ConOut->Mode->CursorColumn;

  if (UsingDownkeys) {
    //
    // Show downkeys info when in use
    //
    gST->ConOut->SetCursorPosition (gST->ConOut, mRunningColumn, RestoreRow + UP_DOWN_HELD_DISPLAY_OFFSET);
    if (NumKeysUp > 0) {
      Code[0] = L'U';
    } else if (NumKeysDown > 0) {
      Code[0] = L'D';
    } else if (NumKeysHeld > 0) {
      Code[0] = L'-';
    } else {
      Code[0] = L' ';
    }
    gST->ConOut->OutputString (gST->ConOut, Code);
  } else {
    //
    // Show caller ID for flush or non-flush
    //
    gST->ConOut->SetCursorPosition (gST->ConOut, mRunningColumn, RestoreRow + ID_DISPLAY_OFFSET);
    Code[0] = CallerID;
    gST->ConOut->OutputString (gST->ConOut, Code);
  }

  //
  // Key held info
  //
  gST->ConOut->SetCursorPosition (gST->ConOut, mRunningColumn, RestoreRow + X_DISPLAY_OFFSET);
  if (NumKeysHeld > 0) {
    Code[0] = L'X';
  } else {
    Code[0] = L'.';
  }
  gST->ConOut->OutputString (gST->ConOut, Code);

  //
  // Modifiers info
  //
  gST->ConOut->SetCursorPosition (gST->ConOut, mRunningColumn, RestoreRow + MODIFIER_DISPLAY_OFFSET);
  if (Modifiers == 0) {
    Code[0] = L' ';
    gST->ConOut->OutputString (gST->ConOut, Code);
  } else {
    Print(L"%d", Modifiers);
  }

  mRunningColumn++;

  gST->ConOut->SetCursorPosition (gST->ConOut, RestoreColumn, RestoreRow);
}
#endif

VOID
OcKeyMapFlush (
  IN APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN APPLE_KEY_CODE                     Key,
  IN BOOLEAN                            FlushConsole,
  IN BOOLEAN                            EnableFlush,
  IN BOOLEAN                            ShowRunningKeys
  )
{
  EFI_STATUS          Status;
  UINTN               NumKeys;
  APPLE_MODIFIER_MAP  Modifiers;
  EFI_INPUT_KEY       EfiKey;
  APPLE_KEY_CODE      Keys[OC_KEY_MAP_DEFAULT_SIZE];

  if (!EnableFlush) {
    return;
  }

  ASSERT (KeyMap != NULL);

  //
  // Key flush support for use on systems with EFI driver level key repeat; on newer
  // and Apple hardware, will hang until all keys (including control keys) come up.
  //
  while (TRUE) {
    NumKeys = ARRAY_SIZE (Keys);
    DETAIL_DEBUG ((DEBUG_INFO, "OCKM: 3 %d\n", NumKeys));
    Status = KeyMap->GetKeyStrokes (
      KeyMap,
      &Modifiers,
      &NumKeys,
      Keys
      );
    DETAIL_DEBUG ((DEBUG_INFO, "OCKM: 4 %d\n", NumKeys));

#if defined(OC_SHOW_RUNNING_KEYS)
    if (ShowRunningKeys) {
      OcShowRunningKeys (0, 0, NumKeys, Modifiers, L'f', FALSE); // flush
    }
#endif

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCKM: GetKeyStrokes failure - %r\n", Status));
      break;
    }

    if (Key != 0 && !OcKeyMapHasKey (Keys, NumKeys, Key) && Modifiers == 0) {
      DETAIL_DEBUG ((DEBUG_INFO, "OCKM: Break on key 0x%X\n", Key));
      break;
    }

    if (Key == 0 && NumKeys == 0 && Modifiers == 0) {
      DETAIL_DEBUG ((DEBUG_INFO, "OCKM: Break on empty\n"));
      break;
    }

    MicroSecondDelay (10);
  }

  if (FlushConsole) {
    do {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &EfiKey);
    } while (!EFI_ERROR (Status));

    //
    // This one is required on APTIO IV after holding OPT key.
    // Interestingly it does not help adding this after OPT key handling.
    //
    gST->ConIn->Reset (gST->ConIn, FALSE);
  }
}

// InternalContainsKeyStrokes
/** Returns whether or not a list of keys and their modifiers are part of the
    database of pressed keys.

  @param[in]      This          A pointer to the protocol instance.
  @param[in]      Modifiers     The modifiers manipulating the given keys.
  @param[in]      NumberOfKeyCodes  The number of keys present in KeyCodes.
  @param[in,out]  KeyCodes          The list of keys to check for.  The children
                                may be sorted in the process.
  @param[in]      ExactMatch    Specifies whether Modifiers and KeyCodes should be
                                exact matches or just contained.

  @return                Returns whether or not a list of keys and their
                         modifiers are part of the database of pressed keys.
  @retval EFI_SUCCESS    The queried keys are part of the database.
  @retval EFI_NOT_FOUND  The queried keys could not be found.
**/
STATIC
EFI_STATUS
EFIAPI
InternalContainsKeyStrokes (
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *This,
  IN     APPLE_MODIFIER_MAP                 Modifiers,
  IN     UINTN                              NumberOfKeyCodes,
  IN OUT APPLE_KEY_CODE                     *KeyCodes,
  IN     BOOLEAN                            ExactMatch
  )
{
  EFI_STATUS         Status;
  BOOLEAN            Result;

  UINTN              DbNumberOfKeyCodes;
  APPLE_MODIFIER_MAP DbModifiers;
  APPLE_KEY_CODE     DbKeyCodes[8];

  DbNumberOfKeyCodes = ARRAY_SIZE (DbKeyCodes);
  Status             = This->GetKeyStrokes (
                               This,
                               &DbModifiers,
                               &DbNumberOfKeyCodes,
                               DbKeyCodes
                               );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ExactMatch) {
    if (DbModifiers != Modifiers) {
      return FALSE;
    }
  } else if ((DbModifiers & Modifiers) != Modifiers) {
    return FALSE;
  }

  Result = OcKeyMapHasKeys (
             DbKeyCodes,
             DbNumberOfKeyCodes,
             KeyCodes,
             NumberOfKeyCodes,
             ExactMatch
             );
  if (!Result) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

// KeyMapCreateKeyStrokesBuffer
/** Creates a new key set with a given number of keys allocated.  The index
    within the database is returned.

  @param[in]  This          A pointer to the protocol instance.
  @param[in]  BufferLength  The amount of keys to allocate for the key set.
  @param[out] Index         The assigned index of the created key set.

  @return                       Returned is the status of the operation.
  @retval EFI_SUCCESS           A key set with the given number of keys
                                allocated has been created.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval other                 An error returned by a sub-operation.
**/
STATIC
EFI_STATUS
EFIAPI
InternalCreateKeyStrokesBuffer (
  IN  APPLE_KEY_MAP_DATABASE_PROTOCOL  *This,
  IN  UINTN                            BufferLength,
  OUT UINTN                            *Index
  )
{
  EFI_STATUS              Status;

  KEY_MAP_AGGREGATOR_DATA *KeyMapAggregatorData;
  UINTN                   TotalBufferLength;
  APPLE_KEY_CODE          *Buffer;
  APPLE_KEY_STROKES_INFO  *KeyStrokesInfo;

  KeyMapAggregatorData = KEY_MAP_AGGREGATOR_DATA_FROM_DATABASE_THIS (This);

  if (KeyMapAggregatorData->KeyCodeBuffer != NULL) {
    gBS->FreePool ((VOID *)KeyMapAggregatorData->KeyCodeBuffer);
  }

  TotalBufferLength = (KeyMapAggregatorData->KeyCodeBufferLength + BufferLength);

  KeyMapAggregatorData->KeyCodeBufferLength = TotalBufferLength;

  Buffer = AllocateZeroPool (TotalBufferLength * sizeof (*Buffer));

  KeyMapAggregatorData->KeyCodeBuffer = Buffer;

  Status = EFI_OUT_OF_RESOURCES;

  if (Buffer != NULL) {
    KeyStrokesInfo = AllocateZeroPool (
                       SIZE_OF_APPLE_KEY_STROKES_INFO
                         + (BufferLength * sizeof (*Buffer))
                       );

    Status = EFI_OUT_OF_RESOURCES;

    if (KeyStrokesInfo != NULL) {
      KeyStrokesInfo->Signature           = APPLE_KEY_STROKES_INFO_SIGNATURE;
      KeyStrokesInfo->KeyCodeBufferLength = BufferLength;
      KeyStrokesInfo->Index               = KeyMapAggregatorData->NextKeyStrokeIndex++;

      InsertTailList (
        &KeyMapAggregatorData->KeyStrokesInfoList,
        &KeyStrokesInfo->Link
        );

      Status = EFI_SUCCESS;

      *Index = KeyStrokesInfo->Index;
    }
  }

  return Status;
}

// KeyMapRemoveKeyStrokesBuffer
/** Removes a key set specified by its index from the database.

  @param[in] This   A pointer to the protocol instance.
  @param[in] Index  The index of the key set to remove.

  @return                Returned is the status of the operation.
  @retval EFI_SUCCESS    The specified key set has been removed.
  @retval EFI_NOT_FOUND  No key set could be found for the given index.
  @retval other          An error returned by a sub-operation.
**/
STATIC
EFI_STATUS
EFIAPI
InternalRemoveKeyStrokesBuffer (
  IN APPLE_KEY_MAP_DATABASE_PROTOCOL  *This,
  IN UINTN                            Index
  )
{
  EFI_STATUS              Status;

  KEY_MAP_AGGREGATOR_DATA *KeyMapAggregatorData;
  APPLE_KEY_STROKES_INFO  *KeyStrokesInfo;

  KeyMapAggregatorData = KEY_MAP_AGGREGATOR_DATA_FROM_DATABASE_THIS (This);

  KeyStrokesInfo = InternalGetKeyStrokesByIndex (
                     KeyMapAggregatorData,
                     Index
                     );

  Status = EFI_NOT_FOUND;

  if (KeyStrokesInfo != NULL) {
    KeyMapAggregatorData->KeyCodeBufferLength -= KeyStrokesInfo->KeyCodeBufferLength;

    RemoveEntryList (&KeyStrokesInfo->Link);
    gBS->FreePool ((VOID *)KeyStrokesInfo);

    Status = EFI_SUCCESS;
  }

  return Status;
}

// KeyMapSetKeyStrokeBufferKeys
/** Sets the keys of a key set specified by its index to the given KeyCodes
    Buffer.

  @param[in] This              A pointer to the protocol instance.
  @param[in] Index             The index of the key set to edit.
  @param[in] Modifiers         The key modifiers manipulating the given keys.
  @param[in] NumberOfKeyCodes  The number of keys contained in KeyCodes.
  @param[in] KeyCodes          An array of keys to add to the specified key
                               set.

  @return                       Returned is the status of the operation.
  @retval EFI_SUCCESS           The given keys were set for the specified key
                                set.
  @retval EFI_OUT_OF_RESOURCES  The memory necessary to complete the operation
                                could not be allocated.
  @retval EFI_NOT_FOUND         No key set could be found for the given index.
  @retval other                 An error returned by a sub-operation.
**/
STATIC
EFI_STATUS
EFIAPI
InternalSetKeyStrokeBufferKeys (
  IN APPLE_KEY_MAP_DATABASE_PROTOCOL  *This,
  IN UINTN                            Index,
  IN APPLE_MODIFIER_MAP               Modifiers,
  IN UINTN                            NumberOfKeyCodes,
  IN APPLE_KEY_CODE                   *KeyCodes
  )
{
  EFI_STATUS               Status;

  KEY_MAP_AGGREGATOR_DATA *KeyMapAggregatorData;
  APPLE_KEY_STROKES_INFO  *KeyStrokesInfo;

  KeyMapAggregatorData = KEY_MAP_AGGREGATOR_DATA_FROM_DATABASE_THIS (This);

  KeyStrokesInfo = InternalGetKeyStrokesByIndex (
                     KeyMapAggregatorData,
                     Index
                     );

  Status = EFI_NOT_FOUND;

  if (KeyStrokesInfo != NULL) {
    Status = EFI_OUT_OF_RESOURCES;

    if (KeyStrokesInfo->KeyCodeBufferLength >= NumberOfKeyCodes) {
      KeyStrokesInfo->NumberOfKeyCodes = NumberOfKeyCodes;
      KeyStrokesInfo->Modifiers        = Modifiers;

      CopyMem (
        (VOID *)&KeyStrokesInfo->KeyCodes[0],
        (VOID *)KeyCodes,
        (NumberOfKeyCodes * sizeof (*KeyCodes))
        );

      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

STATIC APPLE_KEY_MAP_DATABASE_PROTOCOL *mKeyMapDatabase = NULL;

/**
  Returns the previously install Apple Key Map Database protocol.

  @retval installed or located protocol or NULL
**/
APPLE_KEY_MAP_DATABASE_PROTOCOL *
OcAppleKeyMapGetDatabase (
  VOID
  )
{
  return mKeyMapDatabase;
}

/**
  Install and initialise Apple Key Map protocols.

  @param[in] Reinstall  Overwrite installed protocols.

  @retval installed or located protocol or NULL
**/
APPLE_KEY_MAP_AGGREGATOR_PROTOCOL *
OcAppleKeyMapInstallProtocols (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                        Status;
  EFI_STATUS                        Status2;
  KEY_MAP_AGGREGATOR_DATA           *KeyMapAggregatorData;
  APPLE_KEY_MAP_DATABASE_PROTOCOL   *Database;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL *Aggregator;
  EFI_HANDLE                        NewHandle;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleKeyMapDatabaseProtocolGuid);
    Status2 = OcUninstallAllProtocolInstances (&gAppleKeyMapAggregatorProtocolGuid);
    if (EFI_ERROR (Status) || EFI_ERROR (Status2)) {
      DEBUG ((DEBUG_ERROR, "OCKM: Uninstall failed: %r/%r\n", Status, Status2));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleKeyMapDatabaseProtocolGuid,
      NULL,
      (VOID *)&Database
      );
    Status2 = gBS->LocateProtocol (
      &gAppleKeyMapAggregatorProtocolGuid,
      NULL,
      (VOID *)&Aggregator
      );

    if (!EFI_ERROR (Status2)) {
      //
      // VMware Fusion has no KeyMapDatabase, and it is intended.
      //
      if (!EFI_ERROR (Status)) {
        mKeyMapDatabase = Database;
      }
      return Aggregator;
    } else if (!EFI_ERROR (Status)) {
      //
      // Installed KeyMapDatabase makes no sense, however.
      //
      return NULL;
    }
  }

  KeyMapAggregatorData = AllocateZeroPool (sizeof (*KeyMapAggregatorData));

  if (KeyMapAggregatorData == NULL) {
    return NULL;
  }

  KeyMapAggregatorData->Signature          = KEY_MAP_AGGREGATOR_DATA_SIGNATURE;
  KeyMapAggregatorData->NextKeyStrokeIndex = 3000;

  KeyMapAggregatorData->Database.Revision               = APPLE_KEY_MAP_DATABASE_PROTOCOL_REVISION;
  KeyMapAggregatorData->Database.CreateKeyStrokesBuffer = InternalCreateKeyStrokesBuffer;
  KeyMapAggregatorData->Database.RemoveKeyStrokesBuffer = InternalRemoveKeyStrokesBuffer;
  KeyMapAggregatorData->Database.SetKeyStrokeBufferKeys = InternalSetKeyStrokeBufferKeys;

  KeyMapAggregatorData->Aggregator.Revision           = APPLE_KEY_MAP_AGGREGATOR_PROTOCOL_REVISION;
  KeyMapAggregatorData->Aggregator.GetKeyStrokes      = InternalGetKeyStrokes;
  KeyMapAggregatorData->Aggregator.ContainsKeyStrokes = InternalContainsKeyStrokes;

  InitializeListHead (&KeyMapAggregatorData->KeyStrokesInfoList);

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gAppleKeyMapDatabaseProtocolGuid,
    (VOID *)&KeyMapAggregatorData->Database,
    &gAppleKeyMapAggregatorProtocolGuid,
    (VOID *)&KeyMapAggregatorData->Aggregator,
    NULL
    );
  if (EFI_ERROR (Status)) {
    FreePool (KeyMapAggregatorData);
    return NULL;
  }

  mKeyMapDatabase = &KeyMapAggregatorData->Database;
  return &KeyMapAggregatorData->Aggregator;
}

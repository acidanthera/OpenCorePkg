/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.
  Additions (downkeys support) copyright (C) 2021 Mike Beaton. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_APPLE_KEY_MAP_LIB_H
#define OC_APPLE_KEY_MAP_LIB_H

#include <Protocol/AppleKeyMapDatabase.h>
#include <Protocol/AppleKeyMapAggregator.h>

/**
  Default buffer size for key map.
**/
#define OC_KEY_MAP_DEFAULT_SIZE 8

/**
  Default buffer size for held keys in downkeys support.
**/
#define OC_HELD_KEYS_DEFAULT_SIZE 8

/**
  Default initial and subsequent key repeat delays in milliseconds.
**/
#define OC_DOWNKEYS_DEFAULT_INITIAL_DELAY    350
#define OC_DOWNKEYS_DEFAULT_SUBSEQUENT_DELAY 80

/**
  Info for tracking key repeat timing in downkeys support.
**/
typedef struct {
  INT64             KeyTime;
  APPLE_KEY_CODE    Key;
} OC_HELD_KEY_INFO;

/**
  Repeat key context.
**/
typedef struct {
  UINT64                             InitialDelay;
  UINT64                             SubsequentDelay;
  UINT64                             PreviousTime;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;
  OC_HELD_KEY_INFO                   *KeysHeld;
  UINTN                              NumKeysHeld;
  UINTN                              MaxKeysHeld;
} OC_KEY_REPEAT_CONTEXT;

/** Initialise OC key repeat context. (No later  matching uninitialize call exists or is required.)

  @param[out] Context                 Key repeat context structure to initialise.
  @param[in]  KeyMap                  The key map aggregator protocol instance to use.
  @param[in]  MaxKeysHeld             Number of entries in user allocated held keys buffer.
  @param[in]  KeysHeld                Caller allocated held keys buffer to use.
                                      Optional, but if not provided OcGetUpDownKeys can only
                                      return the underlying data from KeyMap->GetKeyStrokes
                                      (i.e. up key, held key and repeat handling are not supported).
  @param[in]  InitialDelay            Key repeat initial delay.
                                      Same time units as CurrentTime passed to subsequent
                                      calls of OcGetUpDownKeys.
  @param[in]  SubsequentDelay         Key repeat subsequent delay.
                                      Same time units as CurrentTime passed to subsequent
                                      calls of OcGetUpDownKeys.
  @param[in]  PreventInitialRepeat    If true, set up buffer so that keys held at init do not
                                      repeat until released and then pressed again.

  @retval EFI_SUCCESS                 The repeat context has been initialised.
  @retval other                       An error returned by a sub-operation.
**/
EFI_STATUS
OcInitKeyRepeatContext(
     OUT OC_KEY_REPEAT_CONTEXT              *Context,
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN     UINTN                              MaxKeysHeld,
  IN     OC_HELD_KEY_INFO                   *KeysHeld       OPTIONAL,
  IN     UINT64                             InitialDelay,
  IN     UINT64                             SubsequentDelay,
  IN     BOOLEAN                            PreventInitialRepeat
  );

/** Returns all key transitions and key modifiers into the appropiate buffers.

  @param[in,out]  RepeatContext       On input the previous (or newly initialised) repeat context.
                                      On output the newly updated key repeat context; values of
                                      interest to the caller are not intended to be read from the
                                      context, but from the other parameters above.
  @param[out]     Modifiers           The key modifiers currently applicable.
  @param[in,out]  NumKeysUp           On input the number of keys allocated for KeysUp. Ignored
                                      if KeysUp is null.
                                      On output the number of up keys detected, with their keycodes
                                      returned into KeysUp if present.
  @param[out]     KeysUp              A Pointer to a optional caller-allocated buffer in which new
                                      up keycodes get returned.
  @param[in,out]  NumKeysDown         On input with KeysDown non-null, the number of keys
                                      allocated for KeysDown and the total number of keys to check.
                                      On input and with KeysDown null, this parameter still
                                      represents the total number of keys to check.
                                      On output the number of down keys detected, with their
                                      keycodes returned into KeysDown if present.
  @param[out]     KeysDown            A Pointer to an optional caller-allocated buffer in which new
                                      down keycodes get returned.
  @param[in]  CurrentTime             Current time (arbitrary units with sufficient resolution).
                                      Need not be valid on initial call only.

  @retval EFI_SUCCESS                 The requested key transitions and states have been returned
                                      or updated as requested in the various out parameters.
  @retval EFI_UNSUPPORTED             Unsupported buffer/size combination.
  @retval other                       An error returned by a sub-operation.
**/
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
  );

/**
  Returns the previously install Apple Key Map Database protocol.

  @retval installed or located protocol or NULL
**/
APPLE_KEY_MAP_DATABASE_PROTOCOL *
OcAppleKeyMapGetDatabase (
  VOID
  );

/**
  Install and initialise Apple Key Map protocols.

  @param[in] Reinstall  Overwrite installed protocols.

  @retval installed or located protocol or NULL
**/
APPLE_KEY_MAP_AGGREGATOR_PROTOCOL *
OcAppleKeyMapInstallProtocols (
  IN BOOLEAN  Reinstall
  );

/**
  Checks whether or not a list of keys is contained within another.

  @param[in] Keys            The reference keys.
  @param[in] NumKeys         The number of keys in Keys.
  @param[in] CheckKeys       The keys to locate in Keys.
  @param[in] NumCheckKeys    The number of keys in CheckKeys.
  @param[in] ExactMatch      Specifies whether matches must be exact.

  @returns  Whether the reference keys contain the checked keys.

**/
BOOLEAN
OcKeyMapHasKeys (
  IN CONST APPLE_KEY_CODE  *Keys,
  IN UINTN                 NumKeys,
  IN CONST APPLE_KEY_CODE  *CheckKeys,
  IN UINTN                 NumCheckKeys,
  IN BOOLEAN               ExactMatch
  );

/**
  Checks whether or not a KeyCode is contained within Keys.

  @param[in] Keys            The reference keys.
  @param[in] NumKeys         The number of keys in Keys.
  @param[in] KeyCode         The key to locate in Keys.

  @returns  Whether the reference keys contain the checked key.

**/
BOOLEAN
OcKeyMapHasKey (
  IN CONST APPLE_KEY_CODE  *Keys,
  IN UINTN                 NumKeys,
  IN CONST APPLE_KEY_CODE  KeyCode
  );

/**
  Performs keyboard input flush.

  @param[in] KeyMap             Apple Key Map Aggregator protocol.
  @param[in] Key                Key to wait for removal or 0.
  @param[in] FlushConsole       Also flush console input.
  @param[in] EnableFlush        If false, return immediately and do nothing.
  @param[in] ShowRunningKeys    If true, emit flush GetKeyStrokes calls to
                                running keys debug view in builtin picker.
**/
VOID
OcKeyMapFlush (
  IN APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN APPLE_KEY_CODE                     Key,
  IN BOOLEAN                            FlushConsole,
  IN BOOLEAN                            EnableFlush,
  IN BOOLEAN                            ShowRunningKeys
  );

#endif // OC_APPLE_KEY_MAP_LIB_H

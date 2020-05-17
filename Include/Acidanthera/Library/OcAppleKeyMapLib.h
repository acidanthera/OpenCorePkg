/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

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

  @param[in] KeyMap        Apple Key Map Aggregator protocol.
  @param[in] Key           Key to wait for removal or 0.
  @param[in] FlushConsole  Also flush console input.
**/
VOID
OcKeyMapFlush (
  IN APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN APPLE_KEY_CODE                     Key,
  IN BOOLEAN                            FlushConsole
  );

#endif // OC_APPLE_KEY_MAP_LIB_H

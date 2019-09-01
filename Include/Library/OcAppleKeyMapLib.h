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
  Checks for key modifier presence.

  @param[in]  KeyMapAggregator  Apple Key Map Aggregator protocol.
  @param[in]  ModifierLeft      Primary key modifer.
  @param[in]  ModifierRight     Secondary key modifer, optional.

  @retval  TRUE if either modifier is set.
**/
BOOLEAN
OcKeyMapHasModifier (
  IN APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMapAggregator,
  IN APPLE_MODIFIER_MAP                 ModifierLeft,
  IN APPLE_MODIFIER_MAP                 ModifierRight  OPTIONAL
  );

/**
  Checks for key presence.

  @param[in]  KeyMapAggregator  Apple Key Map Aggregator protocol.
  @param[in]  KeyCode           Key code.

  @retval  TRUE if key code is set.
**/
BOOLEAN
OcKeyMapHasKey (
  IN APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMapAggregator,
  IN APPLE_KEY_CODE                     KeyCode
  );

#endif // OC_APPLE_KEY_MAP_LIB_H

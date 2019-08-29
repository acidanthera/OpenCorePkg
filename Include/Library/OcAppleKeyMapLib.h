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
  Install and initialise Apple Key Map protocols.

  @param[out] KeyMapDatabase    On success, installed or located protocol.
  @param[out] KeyMapAggregator  On success, installed or located protocol.
  @param[in]  Reinstall         Overwrite installed protocols.

  @returns Success status
**/
BOOLEAN
OcAppleKeyMapInstallProtocols (
  OUT APPLE_KEY_MAP_DATABASE_PROTOCOL    **KeyMapDatabase,
  OUT APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  **KeyMapAggregator,
  IN  BOOLEAN                            Reinstall
  );

#endif // OC_APPLE_KEY_MAP_LIB_H

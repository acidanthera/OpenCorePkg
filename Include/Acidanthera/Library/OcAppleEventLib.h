/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_APPLE_EVENT_LIB_H
#define OC_APPLE_EVENT_LIB_H

#include <IndustryStandard/AppleHid.h>

#include <Protocol/AppleEvent.h>

/**
  Install and initialise Apple Event protocol.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
APPLE_EVENT_PROTOCOL *
OcAppleEventInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_APPLE_EVENT_LIB_H

/** @file
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_UNICODE_COLLATION_ENG_LIB_H
#define OC_UNICODE_COLLATION_ENG_LIB_H

#include <Protocol/UnicodeCollation.h>

/**
  The user Entry Point for English module.

  This function initializes unicode character mapping and then installs Unicode
  Collation & Unicode Collation 2 Protocols based on the feature flags.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed protocol.
  @retval NULL  There was an error installing the protocol.
**/
EFI_UNICODE_COLLATION_PROTOCOL *
OcUnicodeCollationEngInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_UNICODE_COLLATION_ENG_LIB_H

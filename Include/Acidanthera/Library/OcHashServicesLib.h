/** @file
  Author: Joel Hoener <athre0z@zyantific.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_HASH_SERVICES_LIB_H
#define OC_HASH_SERVICES_LIB_H

#include <Protocol/ServiceBinding.h>

/**
  Install and initialise EFI Service Binding protocol.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed protocol.
  @retval NULL  There was an error installing the protocol.
**/
EFI_SERVICE_BINDING_PROTOCOL *
OcHashServicesInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_HASH_SERVICES_LIB_H

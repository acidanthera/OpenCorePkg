/** @file
  Copyright (C) 2019-2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_CONFIGURATION_CONSTANTS_H
#define OC_CONFIGURATION_CONSTANTS_H

//
// Define any additional configuration constants required by generated config header file
//

#define OCS_EXPOSE_BOOT_PATH   1U
#define OCS_EXPOSE_VERSION_VAR 2U
#define OCS_EXPOSE_VERSION_UI  4U
#define OCS_EXPOSE_OEM_INFO    8U
#define OCS_EXPOSE_VERSION     (OCS_EXPOSE_VERSION_VAR | OCS_EXPOSE_VERSION_UI)
#define OCS_EXPOSE_ALL_BITS (\
  OCS_EXPOSE_BOOT_PATH  | OCS_EXPOSE_VERSION_VAR | \
  OCS_EXPOSE_VERSION_UI | OCS_EXPOSE_OEM_INFO)

typedef enum {
  OcsVaultOptional = 0,
  OcsVaultBasic    = 1,
  OcsVaultSecure   = 2,
} OCS_VAULT_MODE;

#endif // OC_CONFIGURATION_CONSTANTS_H

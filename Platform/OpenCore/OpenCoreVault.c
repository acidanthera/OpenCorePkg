/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>

#pragma pack(push, 1)

typedef struct {
  CHAR8           StartMagic[16];
  RSA_PUBLIC_KEY  VaultKey;
  CHAR8           EndMagic[16];
} OC_BUILTIN_VAULT_KEY;

#pragma pack(pop)

STATIC
OC_BUILTIN_VAULT_KEY
mOpenCoreVaultKey = {
  .StartMagic = {'=', 'B', 'E', 'G', 'I', 'N', ' ', 'O', 'C', ' ', 'V', 'A', 'U', 'L', 'T', '='},
  .EndMagic   = {'=', '=', 'E', 'N', 'D', ' ', 'O', 'C', ' ', 'V', 'A', 'U', 'L', 'T', '=', '='}
};

RSA_PUBLIC_KEY *
OcGetVaultKey (
  IN  OC_BOOTSTRAP_PROTOCOL *Bootstrap
  )
{
  UINT32    Index;
  BOOLEAN   AllZero;

  //
  // Return previously discovered key.
  //
  if (Bootstrap->VaultKey == NULL) {
    //
    // TODO: Perhaps try to get the key from firmware too?
    //

    AllZero = TRUE;
    for (Index = 0; sizeof (RSA_PUBLIC_KEY); ++Index) {
      if (((UINT8 *) &mOpenCoreVaultKey.VaultKey)[Index] != 0) {
        AllZero = FALSE;
        break;
      }
    }

    if (!AllZero) {
      Bootstrap->VaultKey = &mOpenCoreVaultKey.VaultKey;
    }
  }

  return Bootstrap->VaultKey;
}


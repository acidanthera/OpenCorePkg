/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_APPLE_KEYS_LIB_H
#define OC_APPLE_KEYS_LIB_H

#include <Library/OcGuardLib.h>
#include <Library/OcCryptoLib.h>

#define NUM_OF_PK 2

typedef struct APPLE_PK_ENTRY_ {
  UINT8                   Hash[SHA256_DIGEST_SIZE];
  CONST OC_RSA_PUBLIC_KEY *PublicKey;
} APPLE_PK_ENTRY;

extern CONST APPLE_PK_ENTRY PkDataBase[NUM_OF_PK];

extern CONST UINT8 gAppleX86SecureBootRootCaCert[];
extern CONST UINTN gAppleX86SecureBootRootCaCertSize;

#endif // OC_APPLE_KEYS_LIB_H

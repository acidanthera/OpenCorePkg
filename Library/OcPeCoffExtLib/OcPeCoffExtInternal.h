/** @file
  Provides additional services for Apple PE/COFF image.

  Copyright (c) 2018, savvas. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OC_PE_COFF_EXT_INTERNAL_H
#define OC_PE_COFF_EXT_INTERNAL_H

#include <Library/OcPeCoffExtLib.h>

#define APPLE_SIGNATURE_SECENTRY_SIZE 8

//
// Signature context
//
typedef struct APPLE_SIGNATURE_CONTEXT_ {
  UINT8                            PublicKey[256];
  UINT8                            PublicKeyHash[32];
  UINT8                            Signature[256];
} APPLE_SIGNATURE_CONTEXT;

#endif // OC_PE_COFF_EXT_INTERNAL_H

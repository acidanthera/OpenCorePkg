/** @file
  Provides additional services for Apple PE/COFF image.

  Copyright (c) 2018, savvas. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OC_PE_COFF_EXT_INTERNAL_H
#define OC_PE_COFF_EXT_INTERNAL_H

#include <Library/OcPeCoffExtLib.h>
#include <Library/OcCryptoLib.h>

STATIC_ASSERT (
  OFFSET_OF (EFI_IMAGE_NT_HEADERS64, CheckSum)
  == OFFSET_OF(EFI_IMAGE_NT_HEADERS32, CheckSum),
  "CheckSum is expected to be at the same place"
  );

#define APPLE_CHECKSUM_OFFSET OFFSET_OF (EFI_IMAGE_NT_HEADERS64, CheckSum)
#define APPLE_CHECKSUM_SIZE sizeof (UINT32)

//
// Signature context
//
typedef struct APPLE_SIGNATURE_CONTEXT_ {
  OC_RSA_PUBLIC_KEY                *PublicKey;
  UINT8                            Signature[256];
} APPLE_SIGNATURE_CONTEXT;

#endif // OC_PE_COFF_EXT_INTERNAL_H

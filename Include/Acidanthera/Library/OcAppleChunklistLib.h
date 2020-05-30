/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_CHUNKLIST_LIB_H
#define APPLE_CHUNKLIST_LIB_H

#include <IndustryStandard/AppleChunklist.h>

#include <Library/OcAppleRamDiskLib.h>
#include <Library/OcCryptoLib.h>

//
// Chunklist context.
//
typedef struct OC_APPLE_CHUNKLIST_CONTEXT_ {
  UINTN                       ChunkCount;
  CONST APPLE_CHUNKLIST_CHUNK *Chunks;
  APPLE_CHUNKLIST_SIG         *Signature;
  UINT8                       Hash[SHA256_DIGEST_SIZE];
} OC_APPLE_CHUNKLIST_CONTEXT;

//
// Chunklist functions.
//

/**
  Initializes a chunklist context.

  @param[out] Context           The Context to initialize.
  @param[in]  Buffer            A pointer to a buffer containing the chunklist data.
  @param[in]  BufferSize        The size of the buffer specified in Buffer.

  @retval EFI_SUCCESS           The Context was intialized successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       The chunklist is unsupported.
**/
BOOLEAN
OcAppleChunklistInitializeContext (
     OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN OUT VOID                        *Buffer,
  IN     UINT32                      BufferSize
  );

BOOLEAN
OcAppleChunklistVerifySignature (
  IN OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN     CONST OC_RSA_PUBLIC_KEY     *PublicKey
  );

/**
  Verifies the specified data against a chunklist context.

  @param[in] Context            The Context to verify against.
  @param[in] ExtentTable        A pointer to the RAM disk extent table to be
                                verified.

  @retval EFI_SUCCESS           The data was verified successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_END_OF_FILE       The end of Buffer was reached.
  @retval EFI_COMPROMISED_DATA  The data failed verification.
**/
BOOLEAN
OcAppleChunklistVerifyData (
  IN OUT OC_APPLE_CHUNKLIST_CONTEXT         *Context,
  IN     CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable
  );

#endif // APPLE_CHUNKLIST_LIB_H

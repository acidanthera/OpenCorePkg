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

#include <Library/OcCryptoLib.h>

//
// Chunklist context.
//
typedef struct OC_APPLE_CHUNKLIST_CONTEXT_ {
  APPLE_CHUNKLIST_HEADER  *Header;
  UINTN                   FileSize;
  APPLE_CHUNKLIST_CHUNK   *Chunks;
  APPLE_CHUNKLIST_SIG     *Signature;
  UINT8                   Hash[SHA256_DIGEST_SIZE];
} OC_APPLE_CHUNKLIST_CONTEXT;

//
// Chunklist functions.
//

/**
  Initializes a chunklist context.

  @param[in]  Buffer            A pointer to a buffer containing the chunklist data.
  @param[in]  Length            The length of the buffer specified in Buffer.
  @param[out] Context           The Context to initialize.

  @retval EFI_SUCCESS           The Context was intialized successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       The chunklist is unsupported.
**/
EFI_STATUS
EFIAPI
OcAppleChunklistInitializeContext (
  IN  VOID                        *Buffer,
  IN  UINTN                       Length,
  OUT OC_APPLE_CHUNKLIST_CONTEXT  *Context
  );

BOOLEAN
OcAppleChunklistVerifySignature (
  IN OC_APPLE_CHUNKLIST_CONTEXT  *Context,
  IN RSA_PUBLIC_KEY              *PublicKey
  );

/**
  Verifies the specified data against a chunklist context.

  @param[in] Context            The Context to verify against.
  @param[in] Buffer             A pointer to a buffer containing the data to be verified.
  @param[in] Length             The length of the buffer specified in Buffer.

  @retval EFI_SUCCESS           The data was verified successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_END_OF_FILE       The end of Buffer was reached.
  @retval EFI_COMPROMISED_DATA  The data failed verification.
**/
EFI_STATUS
EFIAPI
OcAppleChunklistVerifyData (
  IN OC_APPLE_CHUNKLIST_CONTEXT *Context,
  IN VOID                       *Buffer,
  IN UINTN                      Length
  );

#endif // APPLE_CHUNKLIST_LIB_H

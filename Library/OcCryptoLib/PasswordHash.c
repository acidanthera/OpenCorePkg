/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcCryptoLib.h>

VOID
OcHashPasswordSha512 (
  IN  CONST UINT8  *Password,
  IN  UINT32       PasswordSize,
  IN  CONST UINT8  *Salt,
  IN  UINT32       SaltSize,
  OUT UINT8        *Hash
  )
{
  UINT32         Index;
  SHA512_CONTEXT ShaContext;

  ASSERT (Password != NULL);
  ASSERT (PasswordSize > 0);
  ASSERT (Hash != NULL);

  Sha512Init   (&ShaContext);
  Sha512Update (&ShaContext, Password, PasswordSize);
  Sha512Update (&ShaContext, Salt, SaltSize);
  Sha512Final  (&ShaContext, Hash);
  //
  // The hash function is applied iteratively to slow down bruteforce attacks.
  // The iteration count has been chosen to take roughly three seconds on
  // modern hardware.
  //
  for (Index = 0; Index < 5000000; ++Index) {
    Sha512Init   (&ShaContext);
    Sha512Update (&ShaContext, Hash, SHA512_DIGEST_SIZE);
    //
    // Password and Salt are re-added into hashing to, in case of a hash
    // collision, again yield a unique hash in the subsequent iteration.
    //
    Sha512Update (&ShaContext, Password, PasswordSize);
    Sha512Update (&ShaContext, Salt, SaltSize);
    Sha512Final  (&ShaContext, Hash);
  }
  SecureZeroMem (&ShaContext, sizeof (ShaContext));
}

/**
  Verify Password and Salt against RefHash.  The used hash function is SHA-512,
  thus the caller must ensure RefHash is at least 64 bytes in size.

  @param[in] Password      The entered password to verify.
  @param[in] PasswordSize  The size, in bytes, of Password.
  @param[in] Salt          The cryptographic salt appended to Password on hash.
  @param[in] SaltSize      The size, in bytes, of Salt.
  @param[in] RefHash       The SHA-512 hash of the reference password and Salt.

  @returns Whether Password and Salt cryptographically match RefHash.

**/
BOOLEAN
OcVerifyPasswordSha512 (
  IN CONST UINT8  *Password,
  IN UINT32       PasswordSize,
  IN CONST UINT8  *Salt,
  IN UINT32       SaltSize,
  IN CONST UINT8  *RefHash
  )
{
  BOOLEAN Result;
  UINT8   VerifyHash[SHA512_DIGEST_SIZE];

  ASSERT (Password != NULL);
  ASSERT (PasswordSize > 0);
  ASSERT (RefHash != NULL);

  OcHashPasswordSha512 (Password, PasswordSize, Salt, SaltSize, VerifyHash);
  Result = SecureCompareMem (RefHash, VerifyHash, SHA512_DIGEST_SIZE) == 0;
  SecureZeroMem (VerifyHash, SHA512_DIGEST_SIZE);

  return Result;
}

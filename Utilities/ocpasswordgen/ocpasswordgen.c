/** @file
  Copyright (c) 2020, Marvin Häuser. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdio.h>

#include <Library/DebugLib.h>
#include <Library/OcCryptoLib.h>
#include <UserPseudoRandom.h>

int
ENTRY_POINT (
  void
  )
{
  CHAR8   Char;
  UINT8   Password[OC_PASSWORD_MAX_LEN];
  UINT8   PasswordLen;
  UINT32  Salt[4];
  UINT8   Index;
  UINT8   PasswordHash[SHA512_DIGEST_SIZE];

  DEBUG ((DEBUG_ERROR, "Please enter your password: "));

  for (PasswordLen = 0; PasswordLen < OC_PASSWORD_MAX_LEN; ++PasswordLen) {
    Char = getchar ();
    if (Char == EOF || Char == '\n') {
      break;
    }

    Password[PasswordLen] = (UINT8) Char;
  }

  for (Index = 0; Index < ARRAY_SIZE (Salt); ++Index) {
    Salt[Index] = pseudo_random ();
  }

  OcHashPasswordSha512 (
    Password,
    PasswordLen,
    (UINT8 *) Salt,
    sizeof (Salt),
    PasswordHash
    );

  DEBUG ((DEBUG_ERROR, "\nPasswordHash: <"));
  for (Index = 0; Index < sizeof (PasswordHash); ++Index) {
    DEBUG ((DEBUG_ERROR, "%02x", PasswordHash[Index]));
  }

  printf (">\nPasswordSalt: <");
  for (Index = 0; Index < sizeof (Salt); ++Index) {
    DEBUG ((DEBUG_ERROR, "%02x", ((UINT8 *) Salt)[Index]));
  }
  DEBUG ((DEBUG_ERROR, ">\n"));

  SecureZeroMem (Password, sizeof (Password));
  SecureZeroMem (PasswordHash, sizeof (PasswordHash));
  SecureZeroMem (&PasswordLen, sizeof (PasswordLen));

  return 0;
}

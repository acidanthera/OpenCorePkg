/** @file
  Copyright (C) 2021, ISP RAS. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef SHA2_AVX_H
#define SHA2_AVX_H

#include <Library/OcDebugLogLib.h>
#include <Library/OcCryptoLib.h>

#define UNPACK64(x, str)                         \
  do {                                           \
    *((str) + 7) = (UINT8) (x);                  \
    *((str) + 6) = (UINT8) RShiftU64 ((x),  8);  \
    *((str) + 5) = (UINT8) RShiftU64 ((x), 16);  \
    *((str) + 4) = (UINT8) RShiftU64 ((x), 24);  \
    *((str) + 3) = (UINT8) RShiftU64 ((x), 32);  \
    *((str) + 2) = (UINT8) RShiftU64 ((x), 40);  \
    *((str) + 1) = (UINT8) RShiftU64 ((x), 48);  \
    *((str) + 0) = (UINT8) RShiftU64 ((x), 56);  \
  } while(0)

#define PACK64(str, x)                           \
  do {                                           \
    *(x) =    ((UINT64) *((str) + 7))            \
           | LShiftU64 (*((str) + 6),  8)        \
           | LShiftU64 (*((str) + 5), 16)        \
           | LShiftU64 (*((str) + 4), 24)        \
           | LShiftU64 (*((str) + 3), 32)        \
           | LShiftU64 (*((str) + 2), 40)        \
           | LShiftU64 (*((str) + 1), 48)        \
           | LShiftU64 (*((str) + 0), 56);       \
  } while (0)

typedef struct {
  UINT64 State[8];
  UINT64 Length;
  UINT64 TotalLength;
  UINT8  Block[2 * SHA512_BLOCK_SIZE];
} SHA512_STATE;

typedef SHA512_STATE SHA384_STATE;

//
// Sha 384 Init State
//
STATIC CONST UINT64 SHA384_H0[8] = {
  0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL,
  0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
  0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
  0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL
};

//
// Sha 512 Init State
//
STATIC CONST UINT64 SHA512_H0[8] = {
  0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
  0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
  0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
  0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

VOID
Sha512Avx (
  IN OUT UINT8        *Hash,
  IN     CONST UINT8  *Data,
  IN     UINTN        Len
  );

VOID
Sha384Avx (
  IN OUT UINT8        *Hash,
  IN     CONST UINT8  *Data,
  IN     UINTN        Len
  );

BOOLEAN
EFIAPI
IsAvxSupported ();

#endif // SHA2_AVX_H

/** @file
  Random number generator services that uses RdRand instruction access
  to provide high-quality random numbers.
  In addition to that we provide interfaces that generate pseudo random
  numbers through TSC, which can be used when RdRand is not available.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Copyright (c) 2019, vit9696

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcRngLib.h>

/**
  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

  To the extent possible under law, the author has dedicated all copyright
  and related and neighboring rights to this software to the public domain
  worldwide. This software is distributed without any warranty.

  See <http://creativecommons.org/publicdomain/zero/1.0/>.

  This is xoshiro256** 1.0, one of our all-purpose, rock-solid
  generators. It has excellent (sub-ns) speed, a state (256 bits) that is
  large enough for any parallel application, and it passes all tests we
  are aware of.

  For generating just floating-point numbers, xoshiro256+ is even faster.

  The state must be seeded so that it is not everywhere zero. If you have
  a 64-bit seed, we suggest to seed a splitmix64 generator and use its
  output to fill mXorShiroState.
**/

STATIC UINT64 mXorShiroState[4];

VOID
XorShiro256Init (
  VOID
  )
{
  UINTN  Index;

  //
  // TODO: Try to find better entropy source and consider using CSPRNG
  // in some future (e.g. ChaCha20).
  //
  for (Index = 0; Index < sizeof (mXorShiroState); ++Index) {
    ((UINT8 *) mXorShiroState)[Index] = (UINT8) (AsmReadTsc () & 0xFFU);
  }
}

UINT64
XorShiro256Next (
  VOID
  )
{
  UINT64 Result;
  UINT64 T;

  Result = LRotU64 (mXorShiroState[1] * 5, 7) * 9;
  T      = mXorShiroState[1] << 17;

  mXorShiroState[2] ^= mXorShiroState[0];
  mXorShiroState[3] ^= mXorShiroState[1];
  mXorShiroState[1] ^= mXorShiroState[2];
  mXorShiroState[0] ^= mXorShiroState[3];

  mXorShiroState[2] ^= T;

  mXorShiroState[3] = LRotU64 (mXorShiroState[3], 45);

  return Result;
}

//
// Hardware RNG generator availability.
//
STATIC BOOLEAN mRdRandAvailable = FALSE;

//
// Bit mask used to determine if RdRand instruction is supported.
//
#define RDRAND_MASK                  BIT30

//
// Limited retry number when valid random data is returned.
// Uses the recommended value defined in Section 7.3.17 of "Intel 64 and IA-32
// Architectures Software Developer's Mannual".
//
#define RDRAND_RETRY_LIMIT           10

/**
  The constructor function checks whether or not RDRAND instruction is supported
  by the host hardware.

  The constructor function checks whether or not RDRAND instruction is supported.
  It will ASSERT() if RDRAND instruction is not supported.
  It will always return RETURN_SUCCESS.

  @retval RETURN_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
RETURN_STATUS
EFIAPI
OcRngLibConstructor (
  VOID
  )
{
  UINT32  RegEcx;

  //
  // Determine RDRAND support by examining bit 30 of the ECX register returned by
  // CPUID. A value of 1 indicates that processor support RDRAND instruction.
  //
  AsmCpuid (1, 0, 0, &RegEcx, 0);
  mRdRandAvailable = (RegEcx & RDRAND_MASK) == RDRAND_MASK;

  //
  // Initialize PRNG.
  //
  XorShiro256Init ();

  return RETURN_SUCCESS;
}

/**
  Generates a 16-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 16-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber16 (
  OUT     UINT16                    *Rand
  )
{
  UINT32  Index;

  ASSERT (Rand != NULL);

  if (!mRdRandAvailable) {
    return FALSE;
  }

  //
  // A loop to fetch a 16 bit random value with a retry count limit.
  //
  for (Index = 0; Index < RDRAND_RETRY_LIMIT; Index++) {
    if (AsmRdRand16 (Rand)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Generates a 32-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 32-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber32 (
  OUT     UINT32                    *Rand
  )
{
  UINT32  Index;

  ASSERT (Rand != NULL);

  if (!mRdRandAvailable) {
    return FALSE;
  }

  //
  // A loop to fetch a 32 bit random value with a retry count limit.
  //
  for (Index = 0; Index < RDRAND_RETRY_LIMIT; Index++) {
    if (AsmRdRand32 (Rand)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Generates a 64-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 64-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber64 (
  OUT     UINT64                    *Rand
  )
{
  UINT32  Index;

  ASSERT (Rand != NULL);

  if (!mRdRandAvailable) {
    return FALSE;
  }

  //
  // A loop to fetch a 64 bit random value with a retry count limit.
  //
  for (Index = 0; Index < RDRAND_RETRY_LIMIT; Index++) {
    if (AsmRdRand64 (Rand)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Generates a 128-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 128-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber128 (
  OUT     UINT64                    *Rand
  )
{
  ASSERT (Rand != NULL);

  if (!mRdRandAvailable) {
    return FALSE;
  }

  //
  // Read first 64 bits
  //
  if (!GetRandomNumber64 (Rand)) {
    return FALSE;
  }

  //
  // Read second 64 bits
  //
  return GetRandomNumber64 (++Rand);
}

/**
  Generates a 16-bit pseudo random number.

  @retval     16-bit pseudo random number.
**/
UINT16
EFIAPI
GetPseudoRandomNumber16 (
  VOID
  )
{
  UINT16  Rand;

  if (!GetRandomNumber16 (&Rand)) {
    Rand = (UINT16) (XorShiro256Next () & 0xFFFFULL);
  }

  return Rand;
}

/**
  Generates a 32-bit pseudo random number.

  @retval     32-bit pseudo random number.
**/
UINT32
EFIAPI
GetPseudoRandomNumber32 (
  VOID
  )
{
  UINT32  Rand;

  if (!GetRandomNumber32 (&Rand)) {
    Rand = (UINT32) (XorShiro256Next () & 0xFFFFFFFFULL);
  }

  return Rand;
}

/**
  Generates a 64-bit pseudo random number.

  @retval     64-bit pseudo random number.
**/
UINT64
EFIAPI
GetPseudoRandomNumber64 (
  VOID
  )
{
  UINT64  Rand;

  if (!GetRandomNumber64 (&Rand)) {
    Rand = XorShiro256Next ();
  }

  return Rand;
}

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
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcRngLib.h>

#include <Register/Intel/Cpuid.h>

#include "OcRngInternals.h"

STATIC OC_RNG_CONTEXT mRng;

STATIC
VOID
ChaChaRngStir (
  IN  UINT32  BytesNeeded
  )
{
  //
  // Implementation design based on arc4random from FreeBSD.
  //

  UINTN    Index;
  UINT32   KeySeed[CHACHA_KEY_SIZE / sizeof (UINT32)];
  UINT32   IvSeed[CHACHA_IV_SIZE / sizeof (UINT32)];
  BOOLEAN  Result;

  STATIC_ASSERT (CHACHA_KEY_SIZE % sizeof (UINT32) == 0, "Unexpected key size");
  STATIC_ASSERT (CHACHA_IV_SIZE % sizeof (UINT32) == 0, "Unexpected key size");

  ASSERT (BytesNeeded <= MAX_BYTES_TO_EMIT);

  //
  // Do not reseed if we are initialised and do not yet need a reseed.
  //
  if (mRng.PrngInitialised && mRng.BytesTillReseed >= BytesNeeded) {
    mRng.BytesTillReseed -= BytesNeeded;
    return;
  }

  //
  // Generate seeds.
  //
  for (Index = 0; Index < ARRAY_SIZE (KeySeed); ++Index) {
    Result = GetRandomNumber32 (&KeySeed[Index]);
    if (!Result) {
      ASSERT (FALSE);
      CpuDeadLoop ();
    }
  }

  for (Index = 0; Index < ARRAY_SIZE (IvSeed); ++Index) {
    Result = GetRandomNumber32 (&IvSeed[Index]);
    if (!Result) {
      ASSERT (FALSE);
      CpuDeadLoop ();
    }
  }

  //
  // Reinitialize if we are making a second loop.
  //
  if (mRng.PrngInitialised) {
    ZeroMem (mRng.Buffer, sizeof (mRng.Buffer));
    //
    // Fill buffer with keystream.
    //
    ChaChaCryptBuffer (&mRng.ChaCha, mRng.Buffer, mRng.Buffer, sizeof (mRng.Buffer));
    //
    // Mix in RNG data.
    //
    for (Index = 0; Index < ARRAY_SIZE (KeySeed); ++Index) {
      mRng.Buffer[Index] ^= KeySeed[Index];
    }
    for (Index = 0; Index < ARRAY_SIZE (IvSeed); ++Index) {
      mRng.Buffer[ARRAY_SIZE (KeySeed) + Index] ^= IvSeed[Index];
    }
  } else {
    mRng.PrngInitialised = TRUE;
  }

  //
  // Setup ChaCha context.
  //
  ChaChaInitCtx (&mRng.ChaCha, (UINT8 *) KeySeed, (UINT8 *) IvSeed, 0);

  SecureZeroMem (KeySeed, sizeof (KeySeed));
  SecureZeroMem (IvSeed, sizeof (IvSeed));

  mRng.BytesTillReseed = MAX_BYTES_TO_EMIT - BytesNeeded;
  mRng.BytesInBuffer = 0;

  ZeroMem (mRng.Buffer, sizeof (mRng.Buffer));
}

STATIC
VOID
ChaChaRngGenerate (
  OUT UINT8   *Data,
  IN  UINT32  Size
  )
{
  UINT32  CurrentSize;

  ASSERT (Size <= MAX_BYTES_TO_EMIT);

  ChaChaRngStir (Size);

  while (Size > 0) {
    if (mRng.BytesInBuffer > 0) {
      CurrentSize = MIN (Size, mRng.BytesInBuffer);

      CopyMem (Data, mRng.Buffer + sizeof (mRng.Buffer) - mRng.BytesInBuffer, CurrentSize);
      ZeroMem (mRng.Buffer + sizeof (mRng.Buffer) - mRng.BytesInBuffer, CurrentSize);

      Data               += CurrentSize;
      Size               -= CurrentSize;
      mRng.BytesInBuffer -= CurrentSize;
    }

    if (mRng.BytesInBuffer == 0) {
      ZeroMem (mRng.Buffer, sizeof (mRng.Buffer));
      //
      // Fill buffer with keystream.
      //
      ChaChaCryptBuffer (&mRng.ChaCha, mRng.Buffer, mRng.Buffer, sizeof (mRng.Buffer));
      //
      // Immediately reinit for backtracking resistance.
      //
      ChaChaInitCtx (&mRng.ChaCha, &mRng.Buffer[0], &mRng.Buffer[CHACHA_KEY_SIZE], 0);
      ZeroMem (mRng.Buffer, CHACHA_KEY_SIZE + CHACHA_IV_SIZE);
      mRng.BytesInBuffer = sizeof (mRng.Buffer) - CHACHA_KEY_SIZE - CHACHA_IV_SIZE;
    }
  }
}

STATIC
UINT64
GetEntropyBits (
  IN UINTN  Bits
  )
{
  UINTN   Index;
  UINT64  Entropy;
  UINT64  Tmp;

  //
  // Uses non-deterministic CPU execution.
  // REF: https://static.lwn.net/images/conf/rtlws11/random-hardware.pdf
  // REF: https://www.osadl.org/fileadmin/dam/presentations/RTLWS11/okech-inherent-randomness.pdf
  // REF: http://lkml.iu.edu/hypermail/linux/kernel/1909.3/03714.html
  //
  Entropy = 0;
  for (Index = 0; Index < Bits; ++Index) {
    Tmp = AsmReadTsc () + AsmAddRngJitter (AsmReadTsc ());
    if ((Tmp & BIT0) != 0) {
      Entropy |= LShiftU64 (1, Index);
    }
  }

  return Entropy;
}

/**
  The constructor function checks whether or not RDRAND instruction is supported
  by the host hardware.

  The constructor function checks whether or not RDRAND instruction is supported.
  It will ASSERT() if RDRAND instruction is not supported.
  It will always return EFI_SUCCESS.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OcRngLibConstructor (
  VOID
  )
{
  CPUID_VERSION_INFO_ECX RegEcx;

  //
  // Determine RDRAND support by examining bit 30 of the ECX register returned by
  // CPUID. A value of 1 indicates that processor support RDRAND instruction.
  //
  AsmCpuid (1, 0, 0, &RegEcx.Uint32, 0);
  mRng.HardwareRngAvailable = RegEcx.Bits.RDRAND != 0;

  //
  // Initialize PRNG.
  //
  ChaChaRngStir (0);

  return EFI_SUCCESS;
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

  if (mRng.HardwareRngAvailable) {
    //
    // A loop to fetch a 16 bit random value with a retry count limit.
    //
    for (Index = 0; Index < RDRAND_RETRY_LIMIT; Index++) {
      if (AsmRdRand16 (Rand)) {
        return TRUE;
      }
    }
  }

  *Rand = (UINT16) GetEntropyBits (sizeof (UINT16) * OC_CHAR_BIT);
  return TRUE;
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

  if (mRng.HardwareRngAvailable) {
    //
    // A loop to fetch a 32 bit random value with a retry count limit.
    //
    for (Index = 0; Index < RDRAND_RETRY_LIMIT; Index++) {
      if (AsmRdRand32 (Rand)) {
        return TRUE;
      }
    }
  }

  *Rand = (UINT32) GetEntropyBits (sizeof (UINT32) * OC_CHAR_BIT);
  return TRUE;
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

  if (mRng.HardwareRngAvailable) {
    //
    // A loop to fetch a 64 bit random value with a retry count limit.
    //
    for (Index = 0; Index < RDRAND_RETRY_LIMIT; Index++) {
      if (AsmRdRand64 (Rand)) {
        return TRUE;
      }
    }
  }

  *Rand = GetEntropyBits (sizeof (UINT64) * OC_CHAR_BIT);
  return TRUE;
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

  if (mRng.HardwareRngAvailable) {
    //
    // Read 64 bits twice
    //
    if (GetRandomNumber64 (&Rand[0])
      && GetRandomNumber64 (&Rand[1])) {
      return TRUE;
    }
  }

  Rand[0] = GetEntropyBits (sizeof (UINT64) * OC_CHAR_BIT);
  Rand[1] = GetEntropyBits (sizeof (UINT64) * OC_CHAR_BIT);
  return TRUE;
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

  ChaChaRngGenerate ((UINT8 *) &Rand, sizeof (Rand));

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

  ChaChaRngGenerate ((UINT8 *) &Rand, sizeof (Rand));

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

  ChaChaRngGenerate ((UINT8 *) &Rand, sizeof (Rand));

  return Rand;
}

/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_RNG_INTERNALS_H
#define OC_RNG_INTERNALS_H

#include <Library/OcCryptoLib.h>

/**
  Performs delay on target platform to cause microoperating scheduling
  and introduce timing entropy.

  @param[in]  Value  Dummy value.

  @retval  Dummy value.
**/
UINT64
EFIAPI
AsmAddRngJitter (
  IN UINT64  Value
  );

//
// Limited retry number when valid random data is returned.
// Uses the recommended value defined in Section 7.3.17 of "Intel 64 and IA-32
// Architectures Software Developer's Mannual".
//
#define RDRAND_RETRY_LIMIT           10

//
// Maximum amount of bytes emitted by PRNG before next reseed.
//
#define MAX_BYTES_TO_EMIT            1600000

//
// Maximum bytes in one buffer.
//
#define MAX_BYTES_IN_BUF             (16*64)

/**
  Random Number Generator context.
**/
typedef struct OC_RNG_CONTEXT_ {
  //
  // Hardware random number generator available.
  //
  BOOLEAN         HardwareRngAvailable;
  //
  // Done initialising pseudo random number generator.
  //
  BOOLEAN         PrngInitialised;
  //
  // Amount of bytes to emit before next reseed.
  //
  UINT32          BytesTillReseed;
  //
  // Amount of bytes in the current buffer.
  //
  UINT32          BytesInBuffer;
  //
  // Current CPRNG buffer.
  //
  UINT8           Buffer[MAX_BYTES_IN_BUF];
  //
  // ChaCha context.
  //
  CHACHA_CONTEXT  ChaCha;
} OC_RNG_CONTEXT;

#endif // OC_RNG_INTERNALS_H

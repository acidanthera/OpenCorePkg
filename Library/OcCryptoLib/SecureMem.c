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

#include <Library/DebugLib.h>

INTN
SecureCompareMem (
  IN CONST VOID  *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  //
  // Based on libsodium secure_memcmp function implementation.
  //
  UINTN                          Index;
  //
  // The loop variables are volatile to prevent compiler optimizations, such as
  // security-hurting simplifications and intrinsics insertion.
  //
  volatile CONST UINT8 *volatile Destination;
  volatile CONST UINT8 *volatile Source;
  volatile UINT8                 XorDiff;

  if (Length > 0) {
    ASSERT (DestinationBuffer != NULL);
    ASSERT (SourceBuffer != NULL);
    ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN) DestinationBuffer));
    ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN) SourceBuffer));
  }

  Destination = (volatile CONST UINT8 *) DestinationBuffer;
  Source      = (volatile CONST UINT8 *) SourceBuffer;

  XorDiff = 0;
  for (Index = 0; Index < Length; ++Index) {
    //
    // XOR is used to prevent comparison result based branches causing
    // slightly different execution times. A XOR operation only yields 0 when
    // both operands are equal.
    // Do not break early on mismatch to not leak information about prefixes.
    // The AND operation with 0xFFU is performed to have a result promotion to
    // unsigned int rather than int to ensure a safe cast.
    //
    XorDiff |= (UINT8)((Destination[Index] ^ (Source[Index])) & 0xFFU);
  }
  //
  // This is implemented as an arithmetic operation to have an uniform
  // execution time for success and failure cases.
  //
  // For XorDiff = 0, the subtraction wraps around and leads to a value of
  // UINT_MAX. All other values, as XorDiff is unsigned, must be greater than 0
  // and hence cannot wrap around. This means extracting bit 8 of the
  // operation's result will always yield 1 for XorDiff = 0 and always yield 0
  // for XorDiff != 0. This is then cast to INTN, which is safe because it
  // can only ever be 0 or 1, to finally yield the appropiate return value.
  //
  return ((INTN)(1U & ((XorDiff - 1U) >> 8U))) - 1;
}

VOID *
SecureZeroMem (
  OUT VOID   *Buffer,
  IN  UINTN  Length
  )
{
  volatile UINT8 *Destination;

  if (Length == 0) {
    return Buffer;
  }

  ASSERT (Buffer != NULL);
  ASSERT (Length <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Destination = (volatile UINT8 *) Buffer;

  while (Length--) {
    *Destination++ = 0;
  }

  return Buffer;
}

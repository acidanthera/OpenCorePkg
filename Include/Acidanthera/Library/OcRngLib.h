/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_RNG_LIB_H
#define OC_RNG_LIB_H

#include <Library/RngLib.h>

/**
  Generates a 16-bit pseudo random number.
  This generator is still guaranteed to be cryptographically secure
  by the use of CPRNG with entropy.

  @retval     16-bit pseudo random number.
**/
UINT16
EFIAPI
GetPseudoRandomNumber16 (
  VOID
  );

/**
  Generates a 32-bit pseudo random number.
  This generator is still guaranteed to be cryptographically secure
  by the use of CPRNG with entropy.

  @retval     32-bit pseudo random number.
**/
UINT32
EFIAPI
GetPseudoRandomNumber32 (
  VOID
  );

/**
  Generates a 64-bit pseudo random number.
  This generator is still guaranteed to be cryptographically secure
  by the use of CPRNG with entropy.

  @retval     64-bit pseudo random number.
**/
UINT64
EFIAPI
GetPseudoRandomNumber64 (
  VOID
  );

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
  );

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
  );

#endif // OC_RNG_LIB_H

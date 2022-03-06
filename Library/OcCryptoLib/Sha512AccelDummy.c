/** @file
  Copyright (C) 2021, ISP RAS. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "Sha2Internal.h"

#if defined(OC_CRYPTO_SUPPORTS_SHA384) || defined(OC_CRYPTO_SUPPORTS_SHA512)
VOID
EFIAPI
Sha512TransformAccel (
  IN OUT UINT64      *State,
  IN     CONST UINT8 *Data,
  IN     UINTN       BlockNb
  )
{
  (VOID) State;
  (VOID) Data;
  (VOID) BlockNb;
  ASSERT (FALSE);
}
#endif

BOOLEAN
EFIAPI
TryEnableAccel (
  VOID
  )
{
  mIsAccelEnabled = FALSE;
  return FALSE;
}

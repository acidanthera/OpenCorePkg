/** @file

OcGuardLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/OcGuardLib.h>

//
// Currently no architectures provide UINTN and INTN different from 32-bit or 64-bit
// integers. For this reason they are the only variants supported and are enforced here.
// The lib must be C99-compliant, thus no _Static_assert.
//

typedef BOOLEAN SignedIntnMustBe32or64[sizeof (INTN) == sizeof (INT64)
  || sizeof (INTN) == sizeof (INT32) ? 1 : -1];

typedef BOOLEAN UnsignedIntnMustBe32or64[sizeof (UINTN) == sizeof (UINT64)
  || sizeof (UINTN) == sizeof (UINT32) ? 1 : -1];

BOOLEAN
(OcOverflowAddUN) (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  )
{
  if (sizeof (UINTN) == sizeof (UINT64)) {
    return OcOverflowAddU64 (A, B, (UINT64 *)Result);
  }

  return OcOverflowAddU32 ((UINT32)A, (UINT32)B, (UINT32 *)Result);
}

BOOLEAN
(OcOverflowSubUN) (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  )
{
  if (sizeof (UINTN) == sizeof (UINT64)) {
    return OcOverflowSubU64 (A, B, (UINT64 *)Result);
  }

  return OcOverflowSubU32 ((UINT32)A, (UINT32)B, (UINT32 *)Result);
}

BOOLEAN
(OcOverflowMulUN) (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  )
{
  if (sizeof (UINTN) == sizeof (UINT64)) {
    return OcOverflowMulU64 (A, B, (UINT64 *)Result);
  }

  return OcOverflowMulU32 ((UINT32)A, (UINT32)B, (UINT32 *)Result);
}

BOOLEAN
(OcOverflowAddSN) (
  INTN  A,
  INTN  B,
  INTN  *Result
  )
{
  if (sizeof (INTN) == sizeof (INT64)) {
    return OcOverflowAddS64 (A, B, (INT64 *)Result);
  }

  return OcOverflowAddS32 ((INT32)A, (INT32)B, (INT32 *)Result);
}

BOOLEAN
(OcOverflowSubSN) (
  INTN  A,
  INTN  B,
  INTN  *Result
  )
{
  if (sizeof (INTN) == sizeof (INT64)) {
    return OcOverflowSubS64 (A, B, (INT64 *)Result);
  }

  return OcOverflowSubS32 ((INT32)A, (INT32)B, (INT32 *)Result);
}

BOOLEAN
(OcOverflowMulSN) (
  INTN  A,
  INTN  B,
  INTN  *Result
  )
{
  if (sizeof (INTN) == sizeof (INT64)) {
    return OcOverflowMulS64 (A, B, (INT64 *)Result);
  }

  return OcOverflowMulS32 ((INT32)A, (INT32)B, (INT32 *)Result);
}

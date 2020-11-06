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

#include <Library/BaseLib.h>
#include <Library/OcGuardLib.h>

#include "GuardInternals.h"

BOOLEAN
OcOverflowAddUN (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  )
{
#if defined(OC_HAS_TYPE_GENERIC_BUILTINS)
  return __builtin_add_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_64)
  return __builtin_uaddll_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_32)
  return __builtin_uadd_overflow(A, B, Result);
#else
  if (sizeof (UINTN) == sizeof (UINT64)) {
    return OcOverflowAddU64 (A, B, (UINT64 *)Result);
  }

  return OcOverflowAddU32 ((UINT32)A, (UINT32)B, (UINT32 *)Result);
#endif
}

BOOLEAN
OcOverflowSubUN (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  )
{
#if defined(OC_HAS_TYPE_GENERIC_BUILTINS)
  return __builtin_sub_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_64)
  return __builtin_usubll_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_32)
  return __builtin_usub_overflow(A, B, Result);
#else
  if (sizeof (UINTN) == sizeof (UINT64)) {
    return OcOverflowSubU64 (A, B, (UINT64 *)Result);
  }

  return OcOverflowSubU32 ((UINT32)A, (UINT32)B, (UINT32 *)Result);
#endif
}

BOOLEAN
OcOverflowMulUN (
  UINTN  A,
  UINTN  B,
  UINTN  *Result
  )
{
#if defined(OC_HAS_TYPE_GENERIC_BUILTINS)
  return __builtin_mul_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_64)
  return __builtin_umulll_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_32)
  return __builtin_umul_overflow(A, B, Result);
#else
  if (sizeof (UINTN) == sizeof (UINT64)) {
    return OcOverflowMulU64 (A, B, (UINT64 *)Result);
  }

  return OcOverflowMulU32 ((UINT32)A, (UINT32)B, (UINT32 *)Result);
#endif
}

BOOLEAN
OcOverflowAddSN (
  INTN  A,
  INTN  B,
  INTN  *Result
  )
{
#if defined(OC_HAS_TYPE_GENERIC_BUILTINS)
  return __builtin_add_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_64)
  return __builtin_saddll_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_32)
  return __builtin_sadd_overflow(A, B, Result);
#else
  if (sizeof (INTN) == sizeof (INT64)) {
    return OcOverflowAddS64 (A, B, (INT64 *)Result);
  }

  return OcOverflowAddS32 ((INT32)A, (INT32)B, (INT32 *)Result);
#endif
}

BOOLEAN
OcOverflowSubSN (
  INTN  A,
  INTN  B,
  INTN  *Result
  )
{
#if defined(OC_HAS_TYPE_GENERIC_BUILTINS)
  return __builtin_sub_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_64)
  return __builtin_ssubll_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_32)
  return __builtin_ssub_overflow(A, B, Result);
#else
  if (sizeof (INTN) == sizeof (INT64)) {
    return OcOverflowSubS64 (A, B, (INT64 *)Result);
  }

  return OcOverflowSubS32 ((INT32)A, (INT32)B, (INT32 *)Result);
#endif
}

BOOLEAN
OcOverflowMulSN (
  INTN  A,
  INTN  B,
  INTN  *Result
  )
{
#if defined(OC_HAS_TYPE_GENERIC_BUILTINS)
  return __builtin_mul_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_64)
  return __builtin_smulll_overflow(A, B, Result);
#elif defined(OC_HAS_TYPE_SPECIFIC_BUILTINS_32)
  return __builtin_smul_overflow(A, B, Result);
#else
  if (sizeof (INTN) == sizeof (INT64)) {
    return OcOverflowMulS64 (A, B, (INT64 *)Result);
  }

  return OcOverflowMulS32 ((INT32)A, (INT32)B, (INT32 *)Result);
#endif
}

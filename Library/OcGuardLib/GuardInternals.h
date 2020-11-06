/** @file
  This header provides internal OcGuardLib definitions.

  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_GUARD_INTERNALS_H
#define OC_GUARD_INTERNALS_H

#if defined(__has_builtin)
  #if __has_builtin(__builtin_add_overflow) && __has_builtin(__builtin_sub_overflow) && __has_builtin(__builtin_mul_overflow)
    #define OC_HAS_TYPE_GENERIC_BUILTINS 1
  #endif
#endif

#if defined(__GNUC__) || defined(__clang__)
  #if defined(MDE_CPU_AARCH64) || defined(MDE_CPU_X64)
    STATIC_ASSERT (sizeof (INTN) == 8,  "INTN is expected to be 8 bytes");
    STATIC_ASSERT (sizeof (UINTN) == 8, "UINTN is expected to be 8 bytes");
    #define OC_HAS_TYPE_SPECIFIC_BUILTINS_64 1
  #elif defined(MDE_CPU_ARM) || defined(MDE_CPU_IA32)
    STATIC_ASSERT (sizeof (INTN) == 4,  "INTN is expected to be 4 bytes");
    STATIC_ASSERT (sizeof (UINTN) == 4, "UINTN is expected to be 4 bytes");
    #define OC_HAS_TYPE_SPECIFIC_BUILTINS_32 1
  #endif

  STATIC_ASSERT (sizeof (int) == 4,                "int is expected to be 4 bytes");
  STATIC_ASSERT (sizeof (unsigned) == 4,           "unsigned is expected to be 4 bytes");
  STATIC_ASSERT (sizeof (long long) == 8,          "long long is expected to be 8 bytes");
  STATIC_ASSERT (sizeof (unsigned long long) == 8, "unsigned long long is expected to be 8 bytes");
  #define OC_HAS_TYPE_SPECIFIC_BUILTINS 1
#endif

//
// Currently no architectures provide UINTN and INTN different from 32-bit or 64-bit
// integers. For this reason they are the only variants supported and are enforced here.
//

STATIC_ASSERT (
  sizeof (INTN) == sizeof (INT64) || sizeof (INTN) == sizeof (INT32),
  "INTN must be 32 or 64 Bits wide."
  );

STATIC_ASSERT (
  sizeof (UINTN) == sizeof (UINT64) || sizeof (UINTN) == sizeof (UINT32),
  "UINTN must be 32 or 64 Bits wide."
  );

#endif // OC_GUARD_INTERNALS_H

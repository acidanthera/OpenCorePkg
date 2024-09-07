/** @file
  64-bit Math Worker Function.
  The 32-bit versions of C compiler generate calls to library routines
  to handle 64-bit math. These functions use non-standard calling conventions.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

/* https://gcc.gnu.org/onlinedocs/gccint/Integer-library-routines.html */
__attribute__ ((__used__))
unsigned long long
__umoddi3 (
  unsigned long long  A,
  unsigned long long  B
  )
{
  unsigned long long  Reminder;

  DivU64x64Remainder ((UINT64)A, (UINT64)B, (UINT64 *)&Reminder);

  return Reminder;
}

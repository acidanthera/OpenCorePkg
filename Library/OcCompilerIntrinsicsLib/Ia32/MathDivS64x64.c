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
long long
__divdi3 (
  long long  A,
  long long  B
  )
{
  return DivS64x64Remainder ((INT64)A, (INT64)B, NULL);
}

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
#include <Library/OcGuardLib.h>

#include "../BigNumLib.h"

#if defined(_MSC_VER) && !defined(__clang__)
  #include <intrin.h>
  #pragma intrinsic(_umul128)
#endif

OC_BN_WORD
BigNumWordMul64 (
  OUT OC_BN_WORD  *Hi,
  IN  OC_BN_WORD  A,
  IN  OC_BN_WORD  B
  )
{
  ASSERT (OC_BN_WORD_SIZE == sizeof (UINT64));
  ASSERT (Hi != NULL);

#if !defined(_MSC_VER) || defined(__clang__)
  //
  // Clang and GCC support the __int128 type for edk2 builds.
  //
  unsigned __int128 Result = (unsigned __int128)A * B;
  *Hi = (OC_BN_WORD)(Result >> OC_BN_WORD_NUM_BITS);
  return (OC_BN_WORD)Result;
#else
  //
  // MSVC does not support the __int128 type for edk2 builds, but a proprietary
  // intrinsic function declared above.
  //
  return _umul128 (A, B, Hi);
#endif
}

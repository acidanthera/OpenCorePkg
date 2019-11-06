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

OC_BN_WORD
BigNumWordMul64 (
  OUT OC_BN_WORD  *Hi,
  IN  OC_BN_WORD  A,
  IN  OC_BN_WORD  B
  )
{
  ASSERT (OC_BN_WORD_SIZE == sizeof (UINT64));
  ASSERT (Hi != NULL);
  //
  // This is to be used when the used compiler lacks support for both the
  // __int128 type and a suiting intrinsic to perform the calculation.
  //
  // Source: https://stackoverflow.com/a/31662911
  //
  CONST OC_BN_WORD SubWordShift = OC_BN_WORD_NUM_BITS / 2;
  CONST OC_BN_WORD SubWordMask  = ((OC_BN_WORD)1U << SubWordShift) - 1;

  OC_BN_WORD ALo;
  OC_BN_WORD AHi;
  OC_BN_WORD BLo;
  OC_BN_WORD BHi;

  OC_BN_WORD P0;
  OC_BN_WORD P1;
  OC_BN_WORD P2;
  OC_BN_WORD P3;

  OC_BN_WORD Cy;

  ALo = A & SubWordMask;
  AHi = A >> SubWordShift;
  BLo = B & SubWordMask;
  BHi = B >> SubWordShift;

  P0 = ALo * BLo;
  P1 = ALo * BHi;
  P2 = AHi * BLo;
  P3 = AHi * BHi;

  Cy = (((P0 >> SubWordShift) + (P1 & SubWordMask) + (P2 & SubWordMask)) >> SubWordShift) & SubWordMask;

  *Hi = P3 + (P1 >> SubWordShift) + (P2 >> SubWordShift) + Cy;
  return P0 + (P1 << SubWordShift) + (P2 << SubWordShift);
}

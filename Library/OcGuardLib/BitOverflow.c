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
// The implementations provided try not to be obviously slow, but primarily
// target C99 compliance rather than performance.
//

BOOLEAN
(OcOverflowAddU32) (
  UINT32  A,
  UINT32  B,
  UINT32  *Result
  )
{
  UINT64  Temp;

  //
  // I believe casting will be faster on X86 at least.
  //
  Temp    = (UINT64) A + B;
  *Result = (UINT32) Temp;
  if (Temp <= MAX_UINT32) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowSubU32) (
  UINT32  A,
  UINT32  B,
  UINT32  *Result
  )
{
  *Result = A - B;
  if (B <= A) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowMulU32) (
  UINT32  A,
  UINT32  B,
  UINT32  *Result
  )
{
  UINT64  Temp;

  Temp    = (UINT64) A * B;
  *Result = (UINT32) Temp;
  if (Temp <= MAX_UINT32) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowAddS32) (
  INT32  A,
  INT32  B,
  INT32  *Result
  )
{
  INT64  Temp;

  Temp    = (INT64) A + B;
  *Result = (INT32) Temp;
  if (Temp >= MIN_INT32 && Temp <= MAX_INT32) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowSubS32) (
  INT32  A,
  INT32  B,
  INT32  *Result
  )
{
  INT64  Temp;

  Temp    = (INT64) A - B;
  *Result = (INT32) Temp;
  if (Temp >= MIN_INT32 && Temp <= MAX_INT32) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowMulS32) (
  INT32  A,
  INT32  B,
  INT32  *Result
  )
{
  INT64  Temp;

  Temp    = (INT64) A * B;
  *Result = (INT32) Temp;
  if (Temp >= MIN_INT32 && Temp <= MAX_INT32) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowAddU64) (
  UINT64  A,
  UINT64  B,
  UINT64  *Result
  )
{
  *Result = A + B;
  if (MAX_UINT64 - A >= B) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowSubU64) (
  UINT64  A,
  UINT64  B,
  UINT64  *Result
  )
{
  *Result = A - B;
  if (B <= A) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
(OcOverflowMulU64) (
  UINT64  A,
  UINT64  B,
  UINT64  *Result
  )
{
  UINT64   AHi;
  UINT64   ALo;
  UINT64   BHi;
  UINT64   BLo;
  UINT64   LoBits;
  UINT64   HiBits1;
  UINT64   HiBits2;
  BOOLEAN  Overflow;

  //
  // Based on the 2nd option written by Charphacy, believed to be the fastest portable on x86.
  // See: https://stackoverflow.com/a/26320664
  // Implements overflow checking by a series of up to 3 multiplications.
  //

  AHi = A >> 32ULL;
  ALo = A & MAX_UINT32;
  BHi = B >> 32ULL;
  BLo = B & MAX_UINT32;

  LoBits = ALo * BLo;
  if (AHi == 0 && BHi == 0) {
    *Result = LoBits;
    return FALSE; 
  }

  Overflow = AHi > 0 && BHi > 0;
  HiBits1  = ALo * BHi;
  HiBits2  = AHi * BLo;

  *Result = LoBits + ((HiBits1 + HiBits2) << 32ULL);
  return Overflow || *Result < LoBits || (HiBits1 >> 32ULL) != 0 || (HiBits2 >> 32ULL) != 0;
}

BOOLEAN
(OcOverflowAddS64) (
  INT64  A,
  INT64  B,
  INT64  *Result
  )
{
  if ((B <= 0 || A <= MAX_INT64 - B) && (B >= 0 || A >= MIN_INT64 - B)) {
    *Result = A + B;
    return FALSE;
  }

  //
  // Assign some defined value to *Result.
  //
  *Result = 0;
  return TRUE;
}

BOOLEAN
(OcOverflowSubS64) (
  INT64  A,
  INT64  B,
  INT64  *Result
  )
{
  if ((B >= 0 || A <= MAX_INT64 + B) && (B <= 0 || A >= MIN_INT64 + B)) {
    *Result = A - B;
    return FALSE;
  }

  //
  // Assign some defined value to *Result.
  //
  *Result = 0;
  return TRUE;
}

BOOLEAN
(OcOverflowMulS64) (
  INT64  A,
  INT64  B,
  INT64  *Result
  )
{
  UINT64  AU;
  UINT64  BU;
  UINT64  ResultU;

  //
  // It hurts to implement it without unsigned multiplication, maybe rewrite it one day.
  // The idea taken from BaseSafeIntLib.
  //

#define OC_ABS_64(X) (((X) < 0) ? (((UINT64) (-((X) + 1))) + 1) : (UINT64) (X))

  AU = OC_ABS_64 (A);
  BU = OC_ABS_64 (B);

  if (OcOverflowMulU64 (AU, BU, &ResultU)) {
    *Result = 0;
    return TRUE;
  }

  //
  // Split into positive and negative results and check just one range.
  //
  if ((A < 0) == (B < 0)) {
    if (ResultU <= MAX_INT64) {
      *Result = (INT64) ResultU;
      return FALSE;
    }
  } else {
    if (ResultU < OC_ABS_64 (MIN_INT64)) {
      *Result = -((INT64) ResultU);
      return FALSE;
    } else if (ResultU == OC_ABS_64 (MIN_INT64)) {
      *Result = MIN_INT64;
      return FALSE;
    }
  }

  *Result = 0;
  return TRUE;
}

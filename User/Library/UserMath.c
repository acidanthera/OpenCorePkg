/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>

UINT64
EFIAPI
DivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  ASSERT (Divisor != 0);
  
  return Dividend / Divisor;
}

UINT64
EFIAPI
DivU64x32Remainder (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor,
  OUT     UINT32                    *Remainder OPTIONAL
  )
{
  ASSERT (Divisor != 0);

  if (Remainder != NULL) {
    *Remainder = (UINT32)(Dividend % Divisor);
  }
  return Dividend / Divisor;
}

INT64
EFIAPI
DivS64x64Remainder (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder  OPTIONAL
  )
{
  ASSERT (Divisor != 0);

  if (Remainder != NULL) {
    *Remainder = Dividend % Divisor;
  }
  return Dividend / Divisor;
}

UINT64
EFIAPI
DivU64x64Remainder (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder OPTIONAL
  )
{
  ASSERT (Divisor != 0);
  
  if (Remainder != NULL) {
    *Remainder = Dividend % Divisor;
  }
  return Dividend / Divisor;
}

UINT64
EFIAPI
LShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  return Operand << Count;
}

UINT64
EFIAPI
RShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  return Operand >> Count;
}

UINT64
EFIAPI
MultU64x32 (
  IN      UINT64                    Multiplicand,
  IN      UINT32                    Multiplier
  )
{
  return Multiplicand * Multiplier;
}

INT64
EFIAPI
MultS64x64 (
  IN      INT64                     Multiplicand,
  IN      INT64                     Multiplier
  )
{
  return Multiplicand * Multiplier;
}

UINT64
EFIAPI
MultU64x64 (
  IN      UINT64                    Multiplicand,
  IN      UINT64                    Multiplier
  )
{
  return Multiplicand * Multiplier;
}

UINT64
SwapBytes64 (
  UINT64                    Operand
  )
{
  return __builtin_bswap64 (Operand);
}
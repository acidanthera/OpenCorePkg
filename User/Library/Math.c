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
  if (Remainder != NULL) {
    *Remainder = (UINT32)(Dividend % Divisor);
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

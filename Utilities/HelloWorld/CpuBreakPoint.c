#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>

VOID
EFIAPI
__debugbreak (
  VOID
  )
{
  ASSERT (FALSE);
  while (TRUE);
}
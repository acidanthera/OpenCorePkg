#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>

VOID
EFIAPI
CpuBreakpoint (
  VOID
  )
{
  ASSERT (FALSE);
  while (TRUE);
}
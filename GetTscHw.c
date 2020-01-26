#include <Base.h>

#include <Library/OcCpuLib.h>

/**
  Calculate the TSC frequency

  @retval  The calculated TSC frequency.
**/
UINT64
GuiGetTSCFrequency (
  VOID
  )
{
  return OcGetTSCFrequency ();
}

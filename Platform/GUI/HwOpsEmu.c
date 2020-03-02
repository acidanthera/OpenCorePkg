#include <Base.h>

#include <Library/MtrrLib.h>

BOOLEAN
EFIAPI
GuiSaveAndDisableInterrupts (
  VOID
  )
{
  return FALSE;
}

VOID
EFIAPI
GuiEnableInterrupts (
  VOID
  )
{
  ;
}

RETURN_STATUS
EFIAPI
GuiMtrrSetMemoryAttribute (
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  )
{
  return RETURN_SUCCESS;
}

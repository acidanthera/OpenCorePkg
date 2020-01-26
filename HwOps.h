#ifndef HW_OPS_H
#define HW_OPS_H

#include <Base.h>

#include <Library/MtrrLib.h>

BOOLEAN
EFIAPI
GuiSaveAndDisableInterrupts (
  VOID
  );

VOID
EFIAPI
GuiEnableInterrupts (
  VOID
  );

RETURN_STATUS
EFIAPI
GuiMtrrSetMemoryAttribute (
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  );


#endif // HW_OPS_H

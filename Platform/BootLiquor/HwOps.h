/** @file
  This file is part of BootLiquor, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

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

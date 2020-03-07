/** @file
  This file is part of BootLiquor, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/MtrrLib.h>

BOOLEAN
EFIAPI
GuiSaveAndDisableInterrupts (
  VOID
  )
{
  return SaveAndDisableInterrupts ();
}

VOID
EFIAPI
GuiEnableInterrupts (
  VOID
  )
{
  EnableInterrupts ();
}

RETURN_STATUS
EFIAPI
GuiMtrrSetMemoryAttribute (
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  )
{
  return MtrrSetMemoryAttribute (
           BaseAddress,
           Length,
           Attribute
           );
}

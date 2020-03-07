/** @file
  This file is part of BootLiquor, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

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

/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

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

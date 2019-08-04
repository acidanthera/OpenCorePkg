/** @file
  Reset System Library functions for OVMF

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>

VOID
DirectRestCold (
  VOID
  )
{
  volatile UINTN Index;
  IoWrite8 (0xCF9, BIT2 | BIT1); // 1st choice: PIIX3 RCR, RCPU|SRST

  for (Index = 0; Index < 100; ++Index) {
    ;
  }

  IoWrite8 (0x64, 0xfe);         // 2nd choice: keyboard controller
  CpuDeadLoop ();
}

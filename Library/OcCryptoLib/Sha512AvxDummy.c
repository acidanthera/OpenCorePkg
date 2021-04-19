/** @file
  Copyright (C) 2021, ISP RAS. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>

extern BOOLEAN mIsAvxEnabled;

VOID
EFIAPI
Sha512TransformAvx (
  IN OUT UINT64      *State,
  IN     CONST UINT8 *Data,
  IN     UINTN       BlockNb
  )
{
  ASSERT (FALSE);
}

BOOLEAN
EFIAPI
TryEnableAvx (
  VOID
  )
{
  mIsAvxEnabled = FALSE;
  return FALSE;
}

/** @file

OcGuardLib

Copyright (c) 2020, Download-Fritz

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/OcGuardLib.h>

BOOLEAN
OcOverflowAlignUpU32 (
  UINT32  Value,
  UINT32  Alignment,
  UINT32  *Result
  )
{
  BOOLEAN Status;

  Status   = OcOverflowAddU32 (Value, Alignment - 1U, Result);
  *Result &= ~(Alignment - 1U);

  return Status;
}

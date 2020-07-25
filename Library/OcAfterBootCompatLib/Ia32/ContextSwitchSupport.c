/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "../BootCompatInternal.h"

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

VOID
AppleMapPlatformSaveState (
  IN OUT ASM_SUPPORT_STATE  *AsmState,
     OUT ASM_KERNEL_JUMP    *KernelJump
  )
{
  //
  // Currently unsupported as not required.
  // The warning is present in AppleMapPrepareKernelJump when quirks are accidentally enabled.
  //
}

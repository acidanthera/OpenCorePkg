/** @file
  Copyright (C) 2020, Goldfish64. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _VBIOS_PATCH_H
#define _VBIOS_PATCH_H

#include "BiosVideo.h"

BOOLEAN
PatchIntelVbiosCustom (
  IN UINT8      *Vbios,
  IN UINTN      VbiosSize,
  IN UINT16     Height,
  IN UINT16     Width
  );

#endif

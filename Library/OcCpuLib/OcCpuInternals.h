/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_CPU_INTERNALS_H
#define OC_CPU_INTERNALS_H

/**
  Returns microcode revision for Intel CPUs.

  @retval  microcode revision
**/
UINT32
EFIAPI
AsmReadIntelMicrocodeRevision (
  VOID
  );

#endif // OC_CPU_INTERNALS_H

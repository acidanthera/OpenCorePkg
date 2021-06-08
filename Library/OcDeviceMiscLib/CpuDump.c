/** @file
  Copyright (C) 2021, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <IndustryStandard/ProcessorInfo.h>
#include <Library/DebugLib.h>
#include <Library/OcDeviceMiscLib.h>

EFI_STATUS
OcCpuDump (
  IN OC_CPU_INFO        *CpuInfo,
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  ASSERT (CpuInfo != NULL);
  ASSERT (Root != NULL);

  //
  // TODO
  //
  if (CpuInfo->Model >= CPU_MODEL_NEHALEM && CpuInfo->Model != CPU_MODEL_BONNELL) {

  } else {
    
  }

  return EFI_SUCCESS;
}

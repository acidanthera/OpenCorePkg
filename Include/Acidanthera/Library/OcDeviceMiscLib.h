/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_DEVICE_MISC_LIB_H
#define OC_DEVICE_MISC_LIB_H

#include <Uefi.h>
#include <Library/OcStringLib.h>

/**
  Release UEFI ownership from USB controllers at booting.
**/
EFI_STATUS
ReleaseUsbOwnership (
  VOID
  );

/**
  Perform cold reboot directly bypassing UEFI services. Does not return.
  Supposed to work in any modern physical or virtual environment.
**/
VOID
DirectResetCold (
  VOID
  );

/**
  Reset HDA TCSEL to TC0 state.
**/
VOID
ResetAudioTrafficClass (
  VOID
  );

/**
  Force enables HPET timer.
**/
VOID
ActivateHpetSupport (
  VOID
  );

/**
  Upgrade UEFI version to 2.x.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcForgeUefiSupport (
  VOID
  );

/**
  Reload Option ROMs.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcReloadOptionRoms (
  VOID
  );

#endif // OC_DEVICE_MISC_LIB_H

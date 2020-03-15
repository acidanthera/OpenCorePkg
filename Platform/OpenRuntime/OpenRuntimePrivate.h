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

#ifndef OPEN_RUNTIME_PRIVATE_H
#define OPEN_RUNTIME_PRIVATE_H

#include <Uefi.h>
#include <Protocol/OcFirmwareRuntime.h>

/**
  Main runtime services configuration.
**/
extern OC_FWRT_CONFIG  gMainConfig;

/**
  Override runtime services configuration.
**/
extern OC_FWRT_CONFIG  gOverrideConfig;

/**
  Current active runtime services configuration (main or override).
**/
extern OC_FWRT_CONFIG  *gCurrentConfig;

#define OC_GL_BOOT_OPTION_START 0xF000

VOID
RedirectRuntimeServices (
  VOID
  );

EFI_STATUS
EFIAPI
FwOnGetVariable (
  IN  EFI_GET_VARIABLE  GetVariable,
  OUT EFI_GET_VARIABLE  *OrgGetVariable  OPTIONAL
  );


#endif // FIRMWARE_RUNTIME_SERVICES_PRIVATE_H

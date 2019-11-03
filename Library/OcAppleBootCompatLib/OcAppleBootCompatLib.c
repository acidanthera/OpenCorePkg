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

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleBootCompatLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcMiscLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/OcAppleBootCompat.h>

#include "BootCompatInternal.h"

/**
  Apple Boot Compatibility protocol instance. Its GUID matches
  with legacy AptioMemoryFix protocol, allowing us to avoid
  conflicts between the two.
**/
STATIC OC_APPLE_BOOT_COMPAT_PROTOCOL mOcAppleBootCompatProtocol = {
  OC_APPLE_BOOT_COMPAT_PROTOCOL_REVISION
};

/**
  Apple Boot Compatibility context. This context is used throughout
  the library. Must be accessed through GetBootCompatContext ().
**/
STATIC BOOT_COMPAT_CONTEXT  mOcAppleBootCompatContext;

STATIC
EFI_STATUS
InstallAbcProtocol (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;
  EFI_HANDLE  Handle;

  Status = gBS->LocateProtocol (
    &gOcAppleBootCompatProtocolGuid,
    NULL,
    &Interface
    );

  if (!EFI_ERROR (Status)) {
    //
    // Ensure we do not run with AptioMemoryFix.
    // It also checks for attempts to install this protocol twice.
    //
    DEBUG ((DEBUG_WARN, "OCABC: Found legacy AptioMemoryFix driver!\n"));
    return EFI_ALREADY_STARTED;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gOcAppleBootCompatProtocolGuid,
    &mOcAppleBootCompatProtocol,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCABC: protocol install failure - %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

BOOT_COMPAT_CONTEXT *
GetBootCompatContext (
  VOID
  )
{
  return &mOcAppleBootCompatContext;
}

EFI_STATUS
OcAbcInitialize (
  IN OC_ABC_SETTINGS  *Settings
  )
{
  EFI_STATUS           Status;
  BOOT_COMPAT_CONTEXT  *BootCompat;

  Status = InstallAbcProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BootCompat = GetBootCompatContext ();

  CopyMem (
    &BootCompat->Settings,
    Settings,
    sizeof (BootCompat->Settings)
    );

  InstallServiceOverrides (BootCompat);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcHandleKernelProtectionZone (
  IN BOOLEAN          Allocate
  )
{
  BOOT_COMPAT_CONTEXT  *BootCompat;

  BootCompat = GetBootCompatContext ();

  return AppleSlideHandleBalloonState (
    BootCompat,
    Allocate
    );
}

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
#include <Library/OcAfterBootCompatLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcMiscLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/OcAfterBootCompat.h>

#include "BootCompatInternal.h"

/**
  Apple Boot Compatibility protocol instance. Its GUID matches
  with legacy AptioMemoryFix protocol, allowing us to avoid
  conflicts between the two.
**/
STATIC OC_AFTER_BOOT_COMPAT_PROTOCOL mOcAfterBootCompatProtocol = {
  OC_AFTER_BOOT_COMPAT_PROTOCOL_REVISION
};

/**
  Apple Boot Compatibility context. This context is used throughout
  the library. Must be accessed through GetBootCompatContext ().
**/
STATIC BOOT_COMPAT_CONTEXT  mOcAfterBootCompatContext;

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
    &gOcAfterBootCompatProtocolGuid,
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
    &gOcAfterBootCompatProtocolGuid,
    &mOcAfterBootCompatProtocol,
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
  return &mOcAfterBootCompatContext;
}

EFI_STATUS
OcAbcInitialize (
  IN OC_ABC_SETTINGS  *Settings,
  IN OC_CPU_INFO      *CpuInfo
  )
{
  EFI_STATUS           Status;
  BOOT_COMPAT_CONTEXT  *BootCompat;
  UINTN                LowMemory;
  UINTN                TotalMemory;

  Status = InstallAbcProtocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCABC: ALRBL %d RTDFRG %d DEVMMIO %d NOSU %d NOVRWR %d NOSB %d FBSIG %d NOHBMAP %d SMSLIDE %d WRUNPROT %d\n",
    Settings->AllowRelocationBlock,
    Settings->AvoidRuntimeDefrag,
    Settings->DevirtualiseMmio,
    Settings->DisableSingleUser,
    Settings->ForceBooterSignature,
    Settings->DisableVariableWrite,
    Settings->ProtectSecureBoot,
    Settings->DiscardHibernateMap,
    Settings->EnableSafeModeSlide,
    Settings->EnableWriteUnprotector
    ));

  DEBUG ((
    DEBUG_INFO,
    "OCABC: FEXITBS %d PRMRG %d CSLIDE %d MSLIDE %d PRSRV %d RBMAP %d VMAP %d APPLOS %d RTPERMS %d\n",
    Settings->ForceExitBootServices,
    Settings->ProtectMemoryRegions,
    Settings->ProvideCustomSlide,
    Settings->ProvideMaxSlide,
    Settings->ProtectUefiServices,
    Settings->RebuildAppleMemoryMap,
    Settings->SetupVirtualMap,
    Settings->SignalAppleOS,
    Settings->SyncRuntimePermissions
    ));

  DEBUG_CODE_BEGIN ();
  TotalMemory = OcCountFreePages (&LowMemory);
  DEBUG ((
    DEBUG_INFO,
    "OCABC: Firmware has %Lu free pages (%Lu in lower 4 GB)\n",
    (UINT64) TotalMemory,
    (UINT64) LowMemory
    ));
  DEBUG_CODE_END ();

  BootCompat = GetBootCompatContext ();

  CopyMem (
    &BootCompat->Settings,
    Settings,
    sizeof (BootCompat->Settings)
    );

  BootCompat->CpuInfo = CpuInfo;

  InstallServiceOverrides (BootCompat);

  return EFI_SUCCESS;
}

/** @file
  Copyright (C) 2019-2024, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "../BootCompatInternal.h"

#include <Guid/OcVariable.h>
#include <IndustryStandard/AppleHibernate.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDeviceTreeLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

VOID
AppleMapPrepareKernelJump32 (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat
  )
{
  UINT8    *KernelEntry;
  UINT32   Offset;
  BOOLEAN  Found;

  //
  // There is no reason to patch the kernel when we do not need it.
  //
  if (  !BootCompat->Settings.AvoidRuntimeDefrag
     && !BootCompat->Settings.DiscardHibernateMap
     && !BootCompat->Settings.AllowRelocationBlock
     && !BootCompat->Settings.DisableSingleUser
     && !BootCompat->Settings.ForceBooterSignature)
  {
    return;
  }

  //
  // TODO: Add support for 32-bit hibernation some day.
  //
  if (BootCompat->ServiceState.AppleHibernateWake) {
    RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: Cannot patch 32-bit kernel in hibernate wake!\n"));
    CpuDeadLoop ();
  }

  //
  // There are no call gates in 32-bit mode.
  //
  ASSERT (BootCompat->ServiceState.OldKernelCallGate == 0);

  //
  // Search for kernel entry point.
  // This sequence seems to be relatively stable for 10.4 ~ 10.6.
  //
  STATIC CONST UINT8  mEntryBytes[] = {
    0x66, 0x8C, 0xDB, ///< mov bx, ds
    0x8E, 0xC3,       ///< mov es, ebx
    0x89, 0xC5,       ///< mov ebp, eax
  };

  Offset = 0;

  if (BootCompat->KernelState.RelocationBlock != 0) {
    Found = FindPattern (
              mEntryBytes,
              NULL,
              sizeof (mEntryBytes),
              (VOID *)(UINTN)BootCompat->KernelState.RelocationBlock,
              BootCompat->KernelState.RelocationBlockUsed,
              &Offset
              );
  } else {
    Found = FindPattern (
              mEntryBytes,
              NULL,
              sizeof (mEntryBytes),
              (VOID *)(UINTN)KERNEL_BASE_PADDR,
              BASE_2MB,
              &Offset
              );
  }

  if (!Found) {
    RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: No 32-bit kernel entry!\n"));
    CpuDeadLoop ();
  }

  KernelEntry    = (VOID *)(UINTN)(KERNEL_BASE_PADDR + Offset);
  KernelEntry[0] = 0xE8; ///< call
  Offset         = (UINTN)((UINTN)AsmAppleMapPrepareKernelState32 - (UINTN)KernelEntry - 5);
  CopyMem (&KernelEntry[1], &Offset, sizeof (UINT32));
}

UINTN
EFIAPI
AppleMapPrepareKernelState32 (
  IN UINTN  Args
  )
{
  BOOT_COMPAT_CONTEXT  *BootCompat;

  BootCompat = GetBootCompatContext ();

  if (BootCompat->ServiceState.AppleHibernateWake) {
    //
    // TODO: This is not really implemented.
    //
    AppleMapPrepareForHibernateWake (
      BootCompat,
      Args
      );
  } else {
    AppleMapPrepareForBooting (
      BootCompat,
      (VOID *)Args
      );
  }

  if (BootCompat->KernelState.RelocationBlock != 0) {
    Args -= (UINTN)(BootCompat->KernelState.RelocationBlock - KERNEL_BASE_PADDR);

    //
    // FIXME: This should be done via trampoline as we may overwrite ourselves.
    // See RelocationCallGate.nasm for more details.
    //
    CopyMem (
      (VOID *)(UINTN)KERNEL_BASE_PADDR,
      (VOID *)(UINTN)BootCompat->KernelState.RelocationBlock,
      BootCompat->KernelState.RelocationBlockUsed
      );
  }

  return Args;
}

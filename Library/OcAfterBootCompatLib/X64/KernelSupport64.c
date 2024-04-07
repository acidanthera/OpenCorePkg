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
AppleMapPrepareKernelJump64 (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN     EFI_PHYSICAL_ADDRESS  CallGate,
  IN     UINTN                 HookAddress
  )
{
  CALL_GATE_JUMP  *CallGateJump;

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
  // Check whether we have address and abort if not.
  //
  if (CallGate == 0) {
    RUNTIME_DEBUG ((DEBUG_ERROR, "OCABC: Failed to find call gate address\n"));
    return;
  }

  CallGateJump = (VOID *)(UINTN)CallGate;

  //
  // Move call gate jump bytes front.
  // Performing this on the EfiBootRt KCG may bork the binary, but right now
  // only corrupts an unused string.
  //
  CopyMem (
    CallGateJump + 1,
    CallGateJump,
    ESTIMATED_CALL_GATE_SIZE
    );
  //
  // lea r8, [rip+XXX]
  // Passes KCG as third argument to be relocatable. macOS 13 Developer Beta 1
  // copies the KCG into a separately allocated buffer.
  //
  CallGateJump->LeaRip.Command[0] = 0x4C;
  CallGateJump->LeaRip.Command[1] = 0x8D;
  CallGateJump->LeaRip.Command[2] = 0x05;
  CallGateJump->LeaRip.Argument   = sizeof (*CallGateJump) - sizeof (CallGateJump->LeaRip);
  CallGateJump->Jmp.Command       = 0x25FF;
  CallGateJump->Jmp.Argument      = 0x0;
  CallGateJump->Jmp.Address       = HookAddress;
}

STATIC
UINTN
EFIAPI
AppleMapPrepareKernelStateWorker64 (
  IN UINTN             *Args,
  IN UINTN             EntryPoint,
  IN KERNEL_CALL_GATE  CallGate,
  IN UINTN             *Arg1,
  IN UINTN             Arg2
  )
{
  BOOT_COMPAT_CONTEXT  *BootCompatContext;

  BootCompatContext = GetBootCompatContext ();

  if (BootCompatContext->ServiceState.AppleHibernateWake) {
    AppleMapPrepareForHibernateWake (
      BootCompatContext,
      *Args
      );
  } else {
    AppleMapPrepareForBooting (
      BootCompatContext,
      (VOID *)*Args
      );
  }

  if (BootCompatContext->KernelState.RelocationBlock != 0) {
    //
    // Does not return.
    //
    AppleRelocationCallGate64 (
      Args,
      BootCompatContext,
      CallGate,
      Arg1,
      Arg2
      );
  }

  return CallGate (*Arg1, Arg2);
}

EFI_STATUS
EFIAPI
AppleMapPrepareKernelStateNew64 (
  IN     UINTN                       SystemTable,
  IN OUT APPLE_EFI_BOOT_RT_KCG_ARGS  *KcgArguments,
  IN     KERNEL_CALL_GATE            CallGate
  )
{
  return AppleMapPrepareKernelStateWorker64 (
           &KcgArguments->Args,
           KcgArguments->EntryPoint,
           CallGate,
           &SystemTable,
           (UINTN)KcgArguments
           );
}

UINTN
EFIAPI
AppleMapPrepareKernelStateOld64 (
  IN UINTN             Args,
  IN UINTN             EntryPoint,
  IN KERNEL_CALL_GATE  CallGate
  )
{
  return AppleMapPrepareKernelStateWorker64 (
           &Args,
           EntryPoint,
           CallGate,
           &Args,
           EntryPoint
           );
}

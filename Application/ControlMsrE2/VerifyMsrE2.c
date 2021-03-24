/** @file
  Verify MSR 0xE2 status on all the processors.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Register/Msr.h>
#include <Protocol/MpService.h>

STATIC EFI_MP_SERVICES_PROTOCOL  *mMpServices;
STATIC UINTN                     mHasLockedCores;
STATIC UINTN                     mHasUnlockedCores;

VOID
EFIAPI
ReadMsrE2 (
  IN  VOID  *Buffer
  )
{
  UINTN                                           ProcNum = 0;
  MSR_BROADWELL_PKG_CST_CONFIG_CONTROL_REGISTER   Value;
  EFI_STATUS                                      Status;

  Status = mMpServices->WhoAmI (mMpServices, &ProcNum);

  if (EFI_ERROR (Status)) {
    Print (L"Failed to detect CPU Number\n");
    return;
  }

  Value.Uint64 = AsmReadMsr64 (MSR_BROADWELL_PKG_CST_CONFIG_CONTROL);

  Print (L"CPU%02d has MSR 0xE2: 0x%016LX\n", ProcNum, Value.Uint64);

  if (Value.Bits.CFGLock) {
    mHasLockedCores = 1;
  } else {
    mHasUnlockedCores = 1;
  }
}

EFI_STATUS
EFIAPI
VerifyMSRE2 (
  VOID
  )
{
  EFI_STATUS  Status;

  Print (L"Looking up EFI_MP_SERVICES_PROTOCOL...\n");

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **) &mMpServices);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to find EFI_MP_SERVICES_PROTOCOL - %r\n", Status);
    return Status;
  }

  Print (L"Checking MSR 0xE2 on all CPUs. Values must be SAME!!!\n");

  ReadMsrE2 (NULL);

  Print (L"Starting All APs to verify 0xE2 register...\n", Status);

  Status = mMpServices->StartupAllAPs (mMpServices, ReadMsrE2, TRUE, NULL, 5000000, NULL, NULL);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to StartupAllAPs - %r\n", Status);
    return Status;
  }

  Print (L"Done checking MSR 0xE2 register, compare the values printed!\n");

  if (mHasLockedCores && mHasUnlockedCores) {
    Print (L"This firmware has BORKED MSR 0xE2 register!\n");
    Print (L"Some cores are locked, some are not!!!\n");
  } else if (mHasUnlockedCores) {
    Print (L"This firmware has UNLOCKED MSR 0xE2 register!\n");
  } else {
    Print (L"This firmware has LOCKED MSR 0xE2 register!\n");
  }

  return EFI_SUCCESS;
}

/** @file
  Test bless support.

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
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcSmbiosLib.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/DevicePathPropertyDatabase.h>


#include <Uefi.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS
EFIAPI
TestBless (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                  Status;
  APPLE_BOOT_POLICY_PROTOCOL                  *AppleBootPolicy;
  OC_BOOT_ENTRY                               *Entries;
  OC_BOOT_ENTRY                               *Chosen;
  UINTN                                       EntryCount;
  OC_PICKER_CONTEXT                           PickerContext;

  Print (L"TestBless\n");

  AppleBootPolicy = OcAppleBootPolicyInstallProtocol (TRUE);

  Print (L"OcAppleBootPolicyInstallProtocol is %p\n", AppleBootPolicy);

  if (AppleBootPolicy == NULL) {
    Print (L"Locate gAppleBootPolicyProtocolGuid failed\n");
    return EFI_NOT_FOUND;
  }

  ZeroMem (&PickerContext, sizeof (PickerContext));

  while (TRUE) {

    Status = OcScanForBootEntries (
      AppleBootPolicy,
      &PickerContext,
      &Entries,
      &EntryCount,
      NULL,
      TRUE
      );

    if (EFI_ERROR (Status)) {
      Print (L"Locate OcScanForBootEntries failed %r\n", Status);
      return Status;
    }

    Status = OcShowSimpleBootMenu (&PickerContext, Entries, EntryCount, OC_SCAN_DEFAULT_POLICY, &Chosen);

    if (EFI_ERROR (Status) && Status != EFI_ABORTED) {
      Print (L"OcShowSimpleBootMenu failed - %r\n", Status);
      OcFreeBootEntries (Entries, EntryCount);
      return Status;
    }

    if (!EFI_ERROR (Status)) {
      Print (L"Should boot from %s\n", Chosen->Name);
      gBS->Stall (5000000);
    }

    OcFreeBootEntries (Entries, EntryCount);
  }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN  Index;

  WaitForKeyPress (L"Press any key...");

  for (Index = 0; Index < SystemTable->NumberOfTableEntries; ++Index) {
    Print (L"Table %u is %g\n", (UINT32) Index, &SystemTable->ConfigurationTable[Index].VendorGuid);
  }

  Print (L"This is test app...\n");

  TestBless (ImageHandle, SystemTable);

  return EFI_SUCCESS;
}

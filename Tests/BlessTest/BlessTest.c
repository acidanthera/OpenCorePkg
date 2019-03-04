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
#include <Library/OcProtocolLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcSmbiosLib.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/DevicePathPropertyDatabase.h>


#include <Uefi.h>
#include <Library/DebugLib.h>
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
  UINTN                                       Index;
  CHAR16                                      *TextualDevicePath1;
  CHAR16                                      *TextualDevicePath2;
  CHAR16                                      *BootEntryName;
  CHAR16                                      *BootPathName;
  OC_BOOT_ENTRY                               *Entries;
  UINTN                                       EntryCount;
  
  Print (L"TestBless\n");

  Status = UninstallAllProtocolInstances (&gAppleBootPolicyProtocolGuid);

  Print (L"UninstallAllProtocolInstances is %r\n", Status);

  Status = OcAppleBootPolicyInstallProtocol (ImageHandle, SystemTable);

  Print (L"OcAppleBootPolicyInstallProtocol is %r\n", Status);

  Status = gBS->LocateProtocol (
    &gAppleBootPolicyProtocolGuid,
    NULL,
    (VOID **) &AppleBootPolicy
    );

  if (EFI_ERROR (Status)) {
    Print (L"Locate gAppleBootPolicyProtocolGuid failed %r\n", Status);
    return Status;
  }

  Status = OcScanForBootEntries (
    AppleBootPolicy,
    0,
    &Entries,
    &EntryCount
    );

  if (EFI_ERROR (Status)) {
    Print (L"Locate OcScanForBootEntries failed %r\n", Status);
    return Status;
  }

  Print (L"Located %u bless entries\n", (UINT32) EntryCount);

  for (Index = 0; Index < EntryCount; ++Index) {
    Status = OcDescribeBootEntry (
      AppleBootPolicy,
      Entries[Index].DevicePath,
      &BootEntryName,
      &BootPathName
      );

    if (EFI_ERROR (Status)) {
      FreePool (Entries[Index].DevicePath);
      continue;
    }

    TextualDevicePath1 = ConvertDevicePathToText (Entries[Index].DevicePath, FALSE, FALSE);
    TextualDevicePath2 = ConvertDevicePathToText (Entries[Index].DevicePath, TRUE, TRUE);

    Print (L"%u. Bless Entry <%s> (on handle %p, dmg %d)\n",
      (UINT32) Index, BootEntryName, &Entries[Index], Entries[Index].PrefersDmgBoot);
    Print (L"Full path: %s\n", TextualDevicePath1 ? TextualDevicePath1 : L"<null>");
    Print (L"Short path: %s\n", TextualDevicePath2 ? TextualDevicePath2 : L"<null>");
    Print (L"Dir path: %s\n", BootPathName);

    if (TextualDevicePath1 != NULL) {
      FreePool (TextualDevicePath1);
    }

    if (TextualDevicePath2 != NULL) {
      FreePool (TextualDevicePath2);
    }

    FreePool (Entries[Index].DevicePath);
    FreePool (BootEntryName);
    FreePool (BootPathName);
  }

  FreePool (Entries);

  return EFI_SUCCESS;
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

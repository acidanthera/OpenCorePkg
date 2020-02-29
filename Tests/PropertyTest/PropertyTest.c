/** @file
  Test device property support.

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
#include <Library/OcAppleBootPolicyLib.h>
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
TestDeviceProperties (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                  Status;
  EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *PropertyDatabase;
  EFI_DEVICE_PATH_PROTOCOL                    *DevicePath;
  CHAR16                                      *TextualDevicePath;
  UINTN                                       ReadSize;

  UINT8                                       Data[4] = {0x11, 0x22, 0x33, 0x44};
  UINT8                                       ReadData[4];

  Print (L"TestDeviceProperties\n");

  PropertyDatabase = OcDevicePathPropertyInstallProtocol (TRUE);
  if (PropertyDatabase == NULL) {
    Print (L"OcDevicePathPropertyInstallProtocol is missing\n");
    return EFI_UNSUPPORTED;
  }

  DevicePath = ConvertTextToDevicePath (L"PciRoot(0x0)/Pci(0x11,0x0)/Pci(0x1,0x0)");

  Print (L"Binary PciRoot(0x0)/Pci(0x11,0x0)/Pci(0x1,0x0) is %p\n", DevicePath);

  TextualDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);

  Print (L"Textual is is %s\n", TextualDevicePath ? TextualDevicePath : L"<null>");

  Status = PropertyDatabase->SetProperty (
    PropertyDatabase,
    DevicePath,
    L"to-the-sky",
    Data,
    sizeof (Data)
    );

  Print (L"Set to-the-sky is %r\n", Status);

  ZeroMem (ReadData, sizeof (ReadData));
  ReadSize = sizeof (ReadData);

  Status = PropertyDatabase->GetProperty (
    PropertyDatabase,
    DevicePath,
    L"to-the-sky",
    ReadData,
    &ReadSize
    );
  
  Print (L"Get to-the-sky is %r %u <%02x %02x %02x %02x>\n", Status, (UINT32) ReadSize, ReadData[0], ReadData[1], ReadData[2], ReadData[3]);

  /* {
    static UINT8 Data[4] = {0x01, 0x02, 0x03, 0x04};
    Status = PropertyDatabase->SetProperty (
      PropertyDatabase,
      ConvertTextToDevicePath (L"PciRoot(0x0)/Pci(0x11,0x0)/Pci(0x1,0x0)"),
      L"PropertyName",
      Data,
      sizeof (Data)
      );
  } */

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

  TestDeviceProperties (ImageHandle, SystemTable);

  return EFI_SUCCESS;
}

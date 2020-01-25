/** @file
  Test smbios support.

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
#include <Library/OcCpuLib.h>

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

STATIC GUID SystemUUID = {0x5BC82C38, 0x4DB6, 0x4883, {0x85, 0x2E, 0xE7, 0x8D, 0x78, 0x0A, 0x6F, 0xE6}};
STATIC UINT8 BoardType = 0xA; // Motherboard (BaseBoardTypeMotherBoard)
STATIC UINT8 MemoryFormFactor = 0xD; // SODIMM, 0x9 for DIMM (MemoryFormFactorSodimm)
STATIC UINT8 ChassisType = 0xD; // All in one (MiscChassisTypeAllInOne)
STATIC UINT32 PlatformFeature = 1;
STATIC OC_SMBIOS_DATA Data = {
  .BIOSVendor = NULL, // Do not change BIOS Vendor
  .BIOSVersion = "134.0.0.0.0",
  .BIOSReleaseDate = "12/08/2017",
  .SystemManufacturer = NULL, // Do not change System Manufacturer
  .SystemProductName = "iMac14,2",
  .SystemVersion = "1.0",
  .SystemSerialNumber = "SU77OPENCORE",
  .SystemUUID = &SystemUUID,
  .SystemSKUNumber = "Mac-27ADBB7B4CEE8E61",
  .SystemFamily = "iMac",
  .BoardManufacturer = NULL, // Do not change Board Manufacturer
  .BoardProduct = "Mac-27ADBB7B4CEE8E61",
  .BoardVersion = "iMac14,2",
  .BoardSerialNumber = "SU77PEPELATZWAFFE",
  .BoardAssetTag = "",
  .BoardLocationInChassis = "Part Component",
  .BoardType = &BoardType,
  .MemoryFormFactor = &MemoryFormFactor,
  .ChassisType = &ChassisType,
  .ChassisManufacturer = NULL, // Do not change Chassis Manufacturer
  .ChassisVersion = "Mac-27ADBB7B4CEE8E61",
  .ChassisSerialNumber = "SU77OPENCORE",
  .ChassisAssetTag = "iMac-Aluminum",
  .FirmwareFeatures = 0xE00FE137,
  .FirmwareFeaturesMask = 0xFF1FFF3F,
  .ProcessorType = NULL, // Will be calculated automatically
  .PlatformFeature = &PlatformFeature
};

EFI_STATUS
EFIAPI
TestSmbios (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS       Status;
  OC_CPU_INFO      CpuInfo;
  OC_SMBIOS_TABLE  SmbiosTable;
  OcCpuScanProcessor (&CpuInfo);
  Status = OcSmbiosTablePrepare (&SmbiosTable);
  if (!EFI_ERROR (Status)) {
    OcSmbiosCreate (&SmbiosTable, &Data, OcSmbiosUpdateCreate, &CpuInfo);
    OcSmbiosTableFree (&SmbiosTable);
  }

  return Status;
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

  TestSmbios (ImageHandle, SystemTable);

  return EFI_SUCCESS;
}

/** @file
  Test data hub support.

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
#include <Library/OcDataHubLib.h>

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
#include <IndustryStandard/ProcessorInfo.h>

STATIC GUID SystemUUID = {0x5BC82C38, 0x4DB6, 0x4883, {0x85, 0x2E, 0xE7, 0x8D, 0x78, 0x0A, 0x6F, 0xE6}};
STATIC UINT8 BoardRevision = 1;
STATIC UINT64 StartupPowerEvents = 0;
STATIC UINT64 InitialTSC = 0;
STATIC UINT32 DevicePathsSupported = 1;
STATIC UINT8 SmcRevision[OC_SMC_REVISION_SIZE] = {0x02, 0x15, 0x0f, 0x00, 0x00, 0x07};
STATIC UINT8 SmcBranch[OC_SMC_BRANCH_SIZE] = {0x6a, 0x31, 0x37, 0x00, 0x00, 00, 0x00, 0x00};
STATIC UINT8 SmcPlatform[OC_SMC_PLATFORM_SIZE] = {0x6a, 0x31, 0x36, 0x6a, 0x31, 0x37, 0x00, 0x00};

STATIC OC_DATA_HUB_DATA Data = {
  .PlatformName = "platform",
  .SystemProductName = "iMac14,2",
  .SystemSerialNumber = "SU77OPENCORE",
  .SystemUUID = &SystemUUID,
  .BoardProduct = "Mac-27ADBB7B4CEE8E61",
  .BoardRevision = &BoardRevision,
  .StartupPowerEvents = &StartupPowerEvents,
  .InitialTSC = &InitialTSC,
  .FSBFrequency = NULL,
  .ARTFrequency = NULL,
  .DevicePathsSupported = &DevicePathsSupported,
  .SmcRevision = &SmcRevision[0],
  .SmcBranch = &SmcBranch[0],
  .SmcPlatform = &SmcPlatform[0]
};

EFI_STATUS
EFIAPI
TestDataHub (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  OC_CPU_INFO           CpuInfo;
  EFI_DATA_HUB_PROTOCOL *DataHub;
  DataHub = OcDataHubInstallProtocol (TRUE);
  if (DataHub == NULL) {
    DEBUG ((DEBUG_WARN, "OC: Failed to install Data Hub\n"));
    return EFI_NOT_FOUND;
  }
  OcCpuScanProcessor (&CpuInfo);
  UpdateDataHub (DataHub , &Data, &CpuInfo);

  //TODO: put elsewhere
  {
    STATIC UINT32 Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    STATIC CHAR8 Mlb[] = "MLBSERIAL";
    STATIC UINT8 Rom[] = {0, 0, 0, 0, 0, 0};
    STATIC UINT32 FirmwareFeatures = 0xE00FE137;
    STATIC UINT32 FirmwareFeaturesMask = 0xFF1FFF3F;
    STATIC UINT32 CsrActiveConfig = 0;
    STATIC CHAR8 SecurityMode[] = "full";

    gRT->SetVariable (L"MLB", &gAppleVendorVariableGuid, Attributes, AsciiStrLen (Mlb), Mlb);
    gRT->SetVariable (L"ROM", &gAppleVendorVariableGuid, Attributes, sizeof (Rom), Rom);
    gRT->SetVariable (L"FirmwareFeatures", &gAppleVendorVariableGuid, Attributes, sizeof (FirmwareFeatures), &FirmwareFeatures);
    gRT->SetVariable (L"FirmwareFeaturesMask", &gAppleVendorVariableGuid, Attributes, sizeof (FirmwareFeaturesMask), &FirmwareFeaturesMask);
    gRT->SetVariable (L"csr-active-config", &gAppleBootVariableGuid, Attributes, sizeof (CsrActiveConfig), &CsrActiveConfig);
    gRT->SetVariable (L"security-mode", &gAppleBootVariableGuid, Attributes, sizeof (SecurityMode), SecurityMode);
  }

  //TODO: this is done by AMF or ConSplitter normally, here it temporarily exists for legacy.
  {
    EFI_STATUS  Status;
    VOID        *Gop;

    Gop = NULL;
    Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, &Gop);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Missing GOP on ConsoleOutHandle - %r\n", Status));
      Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, &Gop);

      if (!EFI_ERROR (Status)) {
        Status = gBS->InstallMultipleProtocolInterfaces (
          &gST->ConsoleOutHandle,
          &gEfiGraphicsOutputProtocolGuid,
          Gop,
          NULL
          );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN, "Failed to install GOP on ConsoleOutHandle - %r\n", Status));
        }
      } else {
        DEBUG ((DEBUG_WARN, "Missing GOP entirely - %r\n", Status));
      }
    }
  }

  //TODO: also put elsewhere, boot.efi kills watchdog only in FV2 UI.
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //TODO: put this elsewhere, fixes early reboot on APTIO IV (Ivy/Haswell).
  OcCpuCorrectFlexRatio (&CpuInfo);

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

  TestDataHub (ImageHandle, SystemTable);

  return EFI_SUCCESS;
}

/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>
#include <Uefi.h>

#include <Guid/OcLogVariable.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcBootstrap.h>

#include <Library/DebugLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConfigurationLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcStorageLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>

STATIC
OC_GLOBAL_CONFIG
mOpenCoreConfiguration;

STATIC
OC_STORAGE_CONTEXT
mOpenCoreStorage;

STATIC
EFI_IMAGE_START
mOcOriginalStartImage;

STATIC
UINT32
mOpenCoreStartImageNest;

STATIC
EFI_STATUS
EFIAPI
OcEfiStartImage (
  IN  EFI_HANDLE                  ImageHandle,
  OUT UINTN                       *ExitDataSize,
  OUT CHAR16                      **ExitData    OPTIONAL
  )
{
  EFI_STATUS   Status;

  ++mOpenCoreStartImageNest;

  //
  // We do not know what OS is that, probably booted macOS from shell.
  // Apply all the fixtures once, they are harmless for any OS.
  //
  if (mOpenCoreStartImageNest == 1) {
    OcLoadAcpiSupport (&mOpenCoreStorage, &mOpenCoreConfiguration);
    OcLoadKernelSupport (&mOpenCoreStorage, &mOpenCoreConfiguration);
  }

  Status = mOcOriginalStartImage (
    ImageHandle,
    ExitDataSize,
    ExitData
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Boot failed - %r\n", Status));
  }

  if (mOpenCoreStartImageNest == 1) {
    OcUnloadKernelSupport ();
  }

  --mOpenCoreStartImageNest;

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OcStartImage (
  IN  OC_BOOT_ENTRY               *Chosen,
  IN  EFI_HANDLE                  ImageHandle,
  OUT UINTN                       *ExitDataSize,
  OUT CHAR16                      **ExitData    OPTIONAL
  )
{
  EFI_STATUS   Status;

  ++mOpenCoreStartImageNest;

  if (mOpenCoreStartImageNest == 1) {
    //
    // Some make their ACPI tables incompatible with Windows after modding them for macOS.
    // While obviously it is their fault, here we provide a quick and dirty workaround.
    //
    if (!Chosen->IsWindows || !mOpenCoreConfiguration.Acpi.Quirks.IgnoreForWindows) {
      OcLoadAcpiSupport (&mOpenCoreStorage, &mOpenCoreConfiguration);
    }

    //
    // Do not waste time for kext injection, when we are Windows.
    //
    if (!Chosen->IsWindows) {
      OcLoadKernelSupport (&mOpenCoreStorage, &mOpenCoreConfiguration);
    }
  }

  Status = mOcOriginalStartImage (
    ImageHandle,
    ExitDataSize,
    ExitData
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Boot failed - %r\n", Status));
  }

  if (mOpenCoreStartImageNest == 1 && !Chosen->IsWindows) {
    OcUnloadKernelSupport ();
  }

  --mOpenCoreStartImageNest;

  return Status;
}

STATIC
VOID
OcStoreLoadPath (
  IN EFI_DEVICE_PATH_PROTOCOL  *LoadPath OPTIONAL
  )
{
  EFI_STATUS  Status;
  CHAR16      *DevicePath;
  CHAR8       OutPath[256];

  if (LoadPath != NULL) {
    DevicePath = ConvertDevicePathToText (LoadPath, FALSE, FALSE);
    if (DevicePath != NULL) {
      AsciiSPrint (OutPath, sizeof (OutPath), "%s", DevicePath);
      FreePool (DevicePath);
    } else {
      LoadPath = NULL;
    }
  }

  if (LoadPath == NULL) {
    AsciiSPrint (OutPath, sizeof (OutPath), "Unknown");
  }

  Status = gRT->SetVariable (
    OC_LOG_VARIABLE_PATH,
    &gOcLogVariableGuid,
    OPEN_CORE_NVRAM_ATTR,
    AsciiStrSize (OutPath),
    OutPath
    );

  DEBUG ((
    EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
    "OC: Setting NVRAM %g:%a = %a - %r\n",
    &gOcLogVariableGuid,
    OC_LOG_VARIABLE_PATH,
    OutPath,
    Status
    ));
}

STATIC
VOID
OcMain (
  IN OC_STORAGE_CONTEXT        *Storage,
  IN EFI_DEVICE_PATH_PROTOCOL  *LoadPath OPTIONAL
  )
{
  EFI_STATUS                Status;
  CHAR8                     *Config;
  UINT32                    ConfigSize;
  OC_CPU_INFO               CpuInfo;

  Config = OcStorageReadFileUnicode (
    Storage,
    OPEN_CORE_CONFIG_PATH,
    &ConfigSize
    );

  if (Config != NULL) {
    DEBUG ((DEBUG_INFO, "OC: Loaded configuration of %u bytes\n", ConfigSize));

    Status = OcConfigurationInit (&mOpenCoreConfiguration, Config, ConfigSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to parse configuration!\n"));
    }

    FreePool (Config);
  } else {
    DEBUG ((DEBUG_ERROR, "OC: Failed to load configuration!\n"));
  }

  OcConfigureLogProtocol (
    mOpenCoreConfiguration.Misc.Debug.Target,
    mOpenCoreConfiguration.Misc.Debug.Delay,
    (UINTN) mOpenCoreConfiguration.Misc.Debug.DisplayLevel,
    (UINTN) mOpenCoreConfiguration.Misc.Security.HaltLevel
    );

  if (mOpenCoreConfiguration.Misc.Debug.ExposeBootPath) {
    OcStoreLoadPath (LoadPath);
  }

  OcCpuScanProcessor (&CpuInfo);
  OcLoadUefiSupport (Storage, &mOpenCoreConfiguration, &CpuInfo);
  OcLoadPlatformSupport (&mOpenCoreConfiguration, &CpuInfo);
  OcLoadDevPropsSupport (&mOpenCoreConfiguration);
  OcLoadNvramSupport (&mOpenCoreConfiguration);

  //
  // This is required to catch UEFI Shell boot if any.
  // We do it as late as possible to let other drivers install their hooks.
  //
  mOcOriginalStartImage = gBS->StartImage;
  gBS->StartImage = OcEfiStartImage;
  gBS->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);

  Status = OcRunSimpleBootMenu (
    OC_SCAN_DEFAULT_POLICY,
    OC_LOAD_DEFAULT_POLICY,
    5,
    OcStartImage
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to show boot menu!\n"));
  }
}

STATIC
VOID
EFIAPI
OcBootstrapRerun (
  IN OC_BOOTSTRAP_PROTOCOL            *This,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN EFI_DEVICE_PATH_PROTOCOL         *LoadPath OPTIONAL
  )
{
  EFI_STATUS          Status;

  DEBUG ((DEBUG_INFO, "OC: ReRun executed!\n"));

  //
  // FIXME: Key should not be NULL, but be a statically defined RSA key
  // updated via bin patching prior to signing OpenCore.efi.
  //
  Status = OcStorageInitFromFs (
    &mOpenCoreStorage,
    FileSystem,
    OPEN_CORE_ROOT_PATH,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to open root FS - %r!\n", Status));
    return;
  }

  OcMain (&mOpenCoreStorage, LoadPath);

  OcStorageFree (&mOpenCoreStorage);
}

STATIC
OC_BOOTSTRAP_PROTOCOL
mOpenCoreBootStrap = {
  OC_BOOTSTRAP_PROTOCOL_REVISION,
  OcBootstrapRerun
};

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem;
  EFI_HANDLE                        BootstrapHandle;
  OC_BOOTSTRAP_PROTOCOL             *Bootstrap;
  EFI_DEVICE_PATH_PROTOCOL          *AbsPath;

  DEBUG ((DEBUG_INFO, "OC: Starting OpenCore...\n"));

  //
  // We have just started by bootstrap or manually at EFI/OC/OpenCore.efi.
  // When bootstrap runs us, we only install the protocol.
  // Otherwise we do self start.
  //

  Bootstrap = NULL;
  Status = gBS->LocateProtocol (
    &gOcBootstrapProtocolGuid,
    NULL,
    (VOID **) &Bootstrap
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OC: Found previous image, aborting\n"));
    return EFI_ALREADY_STARTED;
  }

  LoadedImage = NULL;
  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to locate loaded image - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  BootstrapHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &BootstrapHandle,
    &gOcBootstrapProtocolGuid,
    &mOpenCoreBootStrap,
    NULL
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install bootstrap protocol - %r\n", Status));
    return Status;
  }

  DebugPrintDevicePath (DEBUG_INFO, "OC: Booter path", LoadedImage->FilePath);

  if (LoadedImage->FilePath == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Booted from bootstrap\n"));
    return EFI_SUCCESS;
  }

  //
  // Obtain the file system device path
  //
  FileSystem = LocateFileSystem (
    LoadedImage->DeviceHandle,
    LoadedImage->FilePath
    );

  AbsPath = AbsoluteDevicePath (LoadedImage->DeviceHandle, LoadedImage->FilePath);

  //
  // Return success in either case to let rerun work afterwards.
  //
  if (FileSystem != NULL) {
    mOpenCoreBootStrap.ReRun (&mOpenCoreBootStrap, FileSystem, AbsPath);
    DEBUG ((DEBUG_ERROR, "OC: Failed to boot\n"));
  } else {
    DEBUG ((DEBUG_ERROR, "OC: Failed to locate file system\n"));
  }

  if (AbsPath != NULL) {
    FreePool (AbsPath);
  }

  return EFI_SUCCESS;
}

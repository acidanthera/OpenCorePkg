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

#include <Library/OcMainLib.h>
#include <Uefi.h>

#include <Guid/OcVariable.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcBootstrap.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/VMwareDebug.h>

#include <Library/DebugLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConfigurationLib.h>
#include <Library/OcConsoleLib.h>
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
OC_CPU_INFO
mOpenCoreCpuInfo;

STATIC
UINT8
mOpenCoreBooterHash[SHA1_DIGEST_SIZE];

STATIC
OC_RSA_PUBLIC_KEY *
mOpenCoreVaultKey;

STATIC
OC_PRIVILEGE_CONTEXT
mOpenCorePrivilege;

STATIC
EFI_HANDLE
mStorageHandle;

STATIC
EFI_DEVICE_PATH_PROTOCOL *
mStoragePath;

STATIC
CHAR16 *
mStorageRoot;

STATIC
EFI_STATUS
EFIAPI
OcStartImage (
  IN  OC_BOOT_ENTRY               *Chosen,
  IN  EFI_HANDLE                  ImageHandle,
  OUT UINTN                       *ExitDataSize,
  OUT CHAR16                      **ExitData    OPTIONAL,
  IN  BOOLEAN                     LaunchInText
  )
{
  EFI_STATUS                       Status;
  EFI_CONSOLE_CONTROL_SCREEN_MODE  OldMode;

  OldMode = OcConsoleControlSetMode (
    LaunchInText ? EfiConsoleControlScreenText : EfiConsoleControlScreenGraphics
    );

  Status = gBS->StartImage (
    ImageHandle,
    ExitDataSize,
    ExitData
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Boot failed - %r\n", Status));
  }

  OcConsoleControlSetMode (OldMode);

  return Status;
}

STATIC
VOID
OcMain (
  IN OC_STORAGE_CONTEXT        *Storage,
  IN EFI_DEVICE_PATH_PROTOCOL  *LoadPath
  )
{
  EFI_STATUS                Status;
  OC_PRIVILEGE_CONTEXT      *Privilege;

  DEBUG ((DEBUG_INFO, "OC: OcMiscEarlyInit...\n"));
  Status = OcMiscEarlyInit (
    Storage,
    &mOpenCoreConfiguration,
    mOpenCoreVaultKey
    );

  if (EFI_ERROR (Status)) {
    return;
  }

  OcCpuScanProcessor (&mOpenCoreCpuInfo);

  DEBUG ((DEBUG_INFO, "OC: OcLoadNvramSupport...\n"));
  OcLoadNvramSupport (Storage, &mOpenCoreConfiguration);
  DEBUG ((DEBUG_INFO, "OC: OcMiscMiddleInit...\n"));
  OcMiscMiddleInit (
    Storage,
    &mOpenCoreConfiguration,
    mStorageRoot,
    LoadPath,
    mStorageHandle,
    mOpenCoreConfiguration.Booter.Quirks.ForceBooterSignature ? mOpenCoreBooterHash : NULL
    );
  DEBUG ((DEBUG_INFO, "OC: OcLoadUefiSupport...\n"));
  OcLoadUefiSupport (Storage, &mOpenCoreConfiguration, &mOpenCoreCpuInfo, mOpenCoreBooterHash);
  DEBUG_CODE_BEGIN ();
  DEBUG ((DEBUG_INFO, "OC: OcMiscLoadSystemReport...\n"));
  OcMiscLoadSystemReport (&mOpenCoreConfiguration, mStorageHandle);
  DEBUG_CODE_END ();
  DEBUG ((DEBUG_INFO, "OC: OcLoadAcpiSupport...\n"));
  OcLoadAcpiSupport (&mOpenCoreStorage, &mOpenCoreConfiguration);
  DEBUG ((DEBUG_INFO, "OC: OcLoadPlatformSupport...\n"));
  OcLoadPlatformSupport (&mOpenCoreConfiguration, &mOpenCoreCpuInfo);
  DEBUG ((DEBUG_INFO, "OC: OcLoadDevPropsSupport...\n"));
  OcLoadDevPropsSupport (&mOpenCoreConfiguration);
  DEBUG ((DEBUG_INFO, "OC: OcMiscLateInit...\n"));
  OcMiscLateInit (Storage, &mOpenCoreConfiguration);
  DEBUG ((DEBUG_INFO, "OC: OcLoadKernelSupport...\n"));
  OcLoadKernelSupport (&mOpenCoreStorage, &mOpenCoreConfiguration, &mOpenCoreCpuInfo);

  if (mOpenCoreConfiguration.Misc.Security.EnablePassword) {
    mOpenCorePrivilege.CurrentLevel = OcPrivilegeUnauthorized;
    mOpenCorePrivilege.Hash         = mOpenCoreConfiguration.Misc.Security.PasswordHash;
    mOpenCorePrivilege.Salt         = OC_BLOB_GET (&mOpenCoreConfiguration.Misc.Security.PasswordSalt);
    mOpenCorePrivilege.SaltSize     = mOpenCoreConfiguration.Misc.Security.PasswordSalt.Size;

    Privilege = &mOpenCorePrivilege;
  } else {
    Privilege = NULL;
  }

  DEBUG ((DEBUG_INFO, "OC: All green, starting boot management...\n"));

  OcMiscBoot (
    &mOpenCoreStorage,
    &mOpenCoreConfiguration,
    Privilege,
    OcStartImage,
    mOpenCoreConfiguration.Uefi.Quirks.RequestBootVarRouting,
    mStorageHandle
    );
}

STATIC
EFI_STATUS
OcBootstrap (
  IN EFI_HANDLE                       DeviceHandle,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN EFI_DEVICE_PATH_PROTOCOL         *LoadPath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingPath;
  UINTN                     StoragePathSize;

  mOpenCoreVaultKey = OcGetVaultKey ();
  mStorageHandle = DeviceHandle;

  //
  // Calculate root path (never freed).
  //
  RemainingPath = NULL;
  mStorageRoot = OcCopyDevicePathFullName (LoadPath, &RemainingPath);
  //
  // Skipping this or later failing to call UnicodeGetParentDirectory means
  // we got valid path to the root of the partition. This happens when
  // OpenCore.efi was loaded from e.g. firmware and then bootstrapped
  // on a different partition.
  //
  if (mStorageRoot == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to get launcher path\n"));
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_INFO, "OC: Storage root %s\n", mStorageRoot));

  ASSERT (RemainingPath != NULL);

  if (!UnicodeGetParentDirectory (mStorageRoot)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to get launcher root path\n"));
    FreePool (mStorageRoot);
    return EFI_UNSUPPORTED;
  }

  StoragePathSize = (UINTN) RemainingPath - (UINTN) LoadPath;
  mStoragePath = AllocatePool (StoragePathSize + END_DEVICE_PATH_LENGTH);
  if (mStoragePath == NULL) {
    FreePool (mStorageRoot);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (mStoragePath, LoadPath, StoragePathSize);
  SetDevicePathEndNode ((UINT8 *) mStoragePath + StoragePathSize);

  Status = OcStorageInitFromFs (
    &mOpenCoreStorage,
    FileSystem,
    mStorageHandle,
    mStoragePath,
    mStorageRoot,
    mOpenCoreVaultKey
    );

  if (!EFI_ERROR (Status)) {
    OcMain (&mOpenCoreStorage, LoadPath);
    OcStorageFree (&mOpenCoreStorage);
  } else {
    DEBUG ((DEBUG_ERROR, "OC: Failed to open root FS - %r!\n", Status));
    if (Status == EFI_SECURITY_VIOLATION) {
      CpuDeadLoop (); ///< Should not return.
    }
  }

  return Status;
}

STATIC
EFI_HANDLE
EFIAPI
OcGetLoadHandle (
  IN OC_BOOTSTRAP_PROTOCOL            *This
  )
{
  return mStorageHandle;
}

STATIC
OC_BOOTSTRAP_PROTOCOL
mOpenCoreBootStrap = {
  .Revision      = OC_BOOTSTRAP_PROTOCOL_REVISION,
  .GetLoadHandle = OcGetLoadHandle,
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

  if (LoadedImage->DeviceHandle == NULL) {
    DEBUG ((DEBUG_INFO, "OC: Missing boot device\n"));
    //
    // This is not critical as boot path may be complete.
    //
  }

  if (LoadedImage->FilePath == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Missing boot path\n"));
    return EFI_INVALID_PARAMETER;
  }

  DebugPrintDevicePath (DEBUG_INFO, "OC: Booter path", LoadedImage->FilePath);

  //
  // Obtain the file system device path
  //
  FileSystem = LocateFileSystem (
    LoadedImage->DeviceHandle,
    LoadedImage->FilePath
    );
  if (FileSystem == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to locate file system\n"));
    return EFI_INVALID_PARAMETER;
  }

  AbsPath = AbsoluteDevicePath (LoadedImage->DeviceHandle, LoadedImage->FilePath);
  if (AbsPath == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to allocate absolute path\n"));
    return EFI_OUT_OF_RESOURCES;
  }  

  DebugPrintDevicePath (DEBUG_INFO, "OC: Absolute booter path", LoadedImage->FilePath);

  BootstrapHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &BootstrapHandle,
    &gOcBootstrapProtocolGuid,
    &mOpenCoreBootStrap,
    NULL
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to install bootstrap protocol - %r\n", Status));
    FreePool (AbsPath);
    return Status;
  }

  OcBootstrap (LoadedImage->DeviceHandle, FileSystem, AbsPath);
  DEBUG ((DEBUG_ERROR, "OC: Failed to boot\n"));
  CpuDeadLoop ();

  return EFI_SUCCESS;
}

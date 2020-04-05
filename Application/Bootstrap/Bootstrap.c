/** @file
  Bootstrap OpenCore driver.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>
#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcBootstrap.h>

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>

STATIC
EFI_STATUS
LoadOpenCore (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  EFI_HANDLE                       ParentImageHandle,
  OUT EFI_HANDLE                       *ImageHandle
  )
{
  EFI_STATUS                 Status;
  VOID                       *Buffer;
  UINT32                     BufferSize;

  ASSERT (FileSystem != NULL);
  ASSERT (ParentImageHandle != NULL);
  ASSERT (ImageHandle != NULL);

  BufferSize = 0;
  Buffer = ReadFile (FileSystem, OPEN_CORE_DRIVER_PATH, &BufferSize, BASE_16MB);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to locate valid OpenCore image - %p!\n", Buffer));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "BS: Read OpenCore image of %Lu bytes\n", (UINT64) BufferSize));

  //
  // Run OpenCore image
  //
  *ImageHandle = NULL;
  Status = gBS->LoadImage (
    FALSE,
    ParentImageHandle,
    NULL,
    Buffer,
    BufferSize,
    ImageHandle
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to load OpenCore image - %r\n", Status));
    FreePool (Buffer);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "BS: Loaded OpenCore image at %p handle\n", *ImageHandle));

  Status = gBS->StartImage (
    *ImageHandle,
    NULL,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "BS: Failed to start OpenCore image - %r\n", Status));
    gBS->UnloadImage (*ImageHandle);
  }

  FreePool (Buffer);

  return Status;
}

STATIC
EFI_STATUS
StartOpenCore (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem,
  IN EFI_HANDLE                        LoadHandle,
  IN EFI_DEVICE_PATH_PROTOCOL          *LoadPath  OPTIONAL
  )
{
  EFI_STATUS                Status;
  OC_BOOTSTRAP_PROTOCOL     *Bootstrap;
  EFI_DEVICE_PATH_PROTOCOL  *AbsPath;

  Bootstrap = NULL;
  Status = gBS->LocateProtocol (
    &gOcBootstrapProtocolGuid,
    NULL,
    (VOID **) &Bootstrap
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "BS: Failed to locate bootstrap protocol - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  if (Bootstrap->Revision != OC_BOOTSTRAP_PROTOCOL_REVISION) {
    DEBUG ((
      DEBUG_WARN,
      "BS: Unsupported bootstrap protocol %u vs %u\n",
      Bootstrap->Revision,
      OC_BOOTSTRAP_PROTOCOL_REVISION
      ));
    return EFI_UNSUPPORTED;
  }

  AbsPath = AbsoluteDevicePath (LoadHandle, LoadPath);

  Status = Bootstrap->ReRun (Bootstrap, FileSystem, AbsPath);

  if (AbsPath != NULL) {
    FreePool (AbsPath);
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
  EFI_STATUS                        Status;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem;
  EFI_HANDLE                        OcImageHandle;


  DEBUG ((DEBUG_INFO, "BS: Starting OpenCore...\n"));

  //
  // We have just started at EFI/BOOT/BOOTx64.efi.
  // We need to run OpenCore on this partition as it failed automatically.
  // The image is optionally located at OPEN_CORE_DRIVER_PATH file.
  //

  LoadedImage = NULL;
  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to locate loaded image - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  DebugPrintDevicePath (DEBUG_INFO, "BS: Booter path", LoadedImage->FilePath);

  //
  // Obtain the file system device path
  //
  FileSystem = LocateFileSystem (
    LoadedImage->DeviceHandle,
    LoadedImage->FilePath
    );

  if (FileSystem == NULL) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to obtain own file system\n"));
    return EFI_NOT_FOUND;
  }

  //
  // Try to start previously loaded OpenCore
  //

  DEBUG ((DEBUG_INFO, "BS: Trying to start loaded OpenCore image...\n"));
  Status = StartOpenCore (FileSystem, LoadedImage->DeviceHandle, LoadedImage->FilePath);
  if (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "BS: Trying to load OpenCore image...\n"));
  Status = LoadOpenCore (FileSystem, ImageHandle, &OcImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "BS: Failed to load OpenCore from disk - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  Status = StartOpenCore (FileSystem, LoadedImage->DeviceHandle, LoadedImage->FilePath);
  DEBUG ((DEBUG_WARN, "BS: Failed to start OpenCore image - %r\n", Status));

  return Status;
}

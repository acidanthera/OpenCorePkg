/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
LocateFileSystem (
  IN  EFI_HANDLE                         DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath     OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  if (DeviceHandle == NULL) {
    //
    // Locate DeviceHandle if we have none (idea by dmazar).
    //
    if (FilePath == NULL) {
      DEBUG ((DEBUG_WARN, "No device handle or path to proceed\n"));
      return NULL;
    }

    Status = gBS->LocateDevicePath (
      &gEfiSimpleFileSystemProtocolGuid,
      &FilePath,
      &DeviceHandle
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to locate device handle over path - %r\n", Status));
      return NULL;
    }
  }

  Status = gBS->HandleProtocol (
    DeviceHandle,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return FileSystem;
}

EFI_FILE_PROTOCOL *
LocateRootVolume (
  IN  EFI_HANDLE                         DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath     OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *RootVolume;

  FileSystem = LocateFileSystem (DeviceHandle, FilePath);
  if (FileSystem == NULL) {
    return NULL;
  }

  Status = FileSystem->OpenVolume (FileSystem, &RootVolume);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return RootVolume;
}

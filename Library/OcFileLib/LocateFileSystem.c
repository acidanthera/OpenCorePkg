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
#include <Library/OcDebugLogLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcDevicePathLib.h>
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

  DEBUG_CODE_BEGIN ();
  DEBUG ((DEBUG_INFO, "OCFS: Trying to locate filesystem on %p %p\n", DeviceHandle, FilePath));
  if (FilePath != NULL) {
    DebugPrintDevicePath (DEBUG_INFO, "OCFS: Filesystem DP", FilePath);
  }
  DEBUG_CODE_END ();

  if (DeviceHandle == NULL) {
    //
    // Locate DeviceHandle if we have none (idea by dmazar).
    //
    if (FilePath == NULL) {
      DEBUG ((DEBUG_WARN, "OCFS: No device handle or path to proceed\n"));
      return NULL;
    }

    Status = gBS->LocateDevicePath (
      &gEfiSimpleFileSystemProtocolGuid,
      &FilePath,
      &DeviceHandle
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCFS: Failed to locate device handle over path - %r\n", Status));
      return NULL;
    }
  }

  Status = gBS->HandleProtocol (
    DeviceHandle,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCFS: No filesystem on device handle %p\n", DeviceHandle));
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

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
LocateFileSystemByGuid (
  IN CONST GUID  *Guid
  )
{
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFs;

  EFI_STATUS                      Status;

  UINTN                           NumHandles;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           Index;

  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  CONST HARDDRIVE_DEVICE_PATH     *HardDrive;

  ASSERT (Guid != NULL);

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  SimpleFs = NULL;

  for (Index = 0; Index < NumHandles; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    HardDrive = (HARDDRIVE_DEVICE_PATH *)(
                  FindDevicePathNodeWithType (
                    DevicePath,
                    MEDIA_DEVICE_PATH,
                    MEDIA_HARDDRIVE_DP
                    )
                  );
    if ((HardDrive == NULL) || (HardDrive->SignatureType != 0x02)) {
      continue;
    }

    if (CompareGuid (Guid, (GUID *)HardDrive->Signature)) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiSimpleFileSystemProtocolGuid,
                      (VOID **)&SimpleFs
                      );
      if (EFI_ERROR (Status)) {
        SimpleFs = NULL;
      }

      break;
    }
  }

  FreePool (HandleBuffer);
  return SimpleFs;
}

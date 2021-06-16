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

#include <Library/OcMainLib.h>
#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>

STATIC
EFI_STATUS
LoadOpenCore (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  EFI_HANDLE                       DeviceHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL         *LoaderDevicePath
  )
{
  EFI_STATUS                 Status;
  VOID                       *Buffer;
  UINTN                      LoaderPathSize;
  UINTN                      PrefixPathSize;
  UINTN                      RootPathLength;
  CHAR16                     *LoaderPath;
  UINT32                     BufferSize;
  EFI_DEVICE_PATH_PROTOCOL   *ImagePath;

  ASSERT (FileSystem != NULL);

  Buffer = NULL;
  BufferSize = 0;

  LoaderPath = OcCopyDevicePathFullName (LoaderDevicePath, NULL);

  ImagePath = NULL;

  //
  // Try relative path: EFI\\XXX\\Subdir\\Launcher.efi -> EFI\\XXX\\OpenCore.efi
  //
  if (LoaderPath != NULL) {
    LoaderPathSize = StrSize (LoaderPath);
    if (UnicodeGetParentDirectory (LoaderPath)
      && UnicodeGetParentDirectory (LoaderPath)) {
      DEBUG ((DEBUG_INFO, "BS: Relative path - %s\n", LoaderPath));
      PrefixPathSize = StrSize (LoaderPath);
      if (LoaderPathSize - PrefixPathSize >= L_STR_SIZE (OPEN_CORE_APP_PATH)) {
        RootPathLength = PrefixPathSize / sizeof (CHAR16) - 1;
        LoaderPath[RootPathLength] = '\\';
        CopyMem (&LoaderPath[RootPathLength + 1], OPEN_CORE_APP_PATH, L_STR_SIZE (OPEN_CORE_APP_PATH));
        Buffer = ReadFile (FileSystem, LoaderPath, &BufferSize, BASE_16MB);
        DEBUG ((DEBUG_INFO, "BS: Startup path - %s (%p)\n", LoaderPath, Buffer));

        if (Buffer != NULL) {
          ImagePath = FileDevicePath (DeviceHandle, LoaderPath);
          if (ImagePath == NULL) {
            DEBUG ((DEBUG_INFO, "BS: File DP allocation failure, aborting\n"));
            FreePool (Buffer);
            Buffer = NULL;
          }
        }
      }
    }

    FreePool (LoaderPath);
  }

  //
  // Try absolute path: EFI\\BOOT\\BOOTx64.efi -> EFI\\OC\\OpenCore.efi
  //
  if (Buffer == NULL) {
    DEBUG ((
      DEBUG_INFO,
      "BS: Fallback to absolute path - %s\n",
      OPEN_CORE_ROOT_PATH L"\\" OPEN_CORE_APP_PATH
      ));

    Buffer = ReadFile (
      FileSystem,
      OPEN_CORE_ROOT_PATH L"\\" OPEN_CORE_APP_PATH,
      &BufferSize,
      BASE_16MB
      );
    if (Buffer != NULL) {
      //
      // Failure to allocate this one is not too critical, as we will still be able
      // to choose it as a default path.
      //
      ImagePath = FileDevicePath (DeviceHandle, OPEN_CORE_ROOT_PATH L"\\" OPEN_CORE_APP_PATH);
    }
  }

  if (Buffer == NULL) {
    ASSERT (ImagePath == NULL);
    DEBUG ((DEBUG_ERROR, "BS: Failed to locate valid OpenCore image - %p!\n", Buffer));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "BS: Read OpenCore image of %Lu bytes\n", (UINT64) BufferSize));

  //
  // Run OpenCore image
  //
  Status = OcLoadAndRunImage (
    ImagePath,
    Buffer,
    BufferSize,
    NULL,
    NULL
    );

  DEBUG ((DEBUG_ERROR, "BS: Failed to start OpenCore image - %r\n", Status));
  FreePool (Buffer);
  FreePool (ImagePath);

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

  DEBUG ((DEBUG_INFO, "BS: Starting OpenCore application...\n"));

  //
  // We have just started at EFI/BOOT/BOOTx64.efi.
  // We need to run OpenCore on this partition as it failed automatically.
  // The image is optionally located at OPEN_CORE_APP_PATH file.
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

  DEBUG ((DEBUG_INFO, "BS: Trying to load OpenCore image...\n"));
  Status = LoadOpenCore (FileSystem, LoadedImage->DeviceHandle, LoadedImage->FilePath);
  DEBUG ((DEBUG_WARN, "BS: Failed to load OpenCore from disk - %r\n", Status));
  return Status;
}

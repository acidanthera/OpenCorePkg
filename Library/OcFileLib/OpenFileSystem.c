/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

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
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "OcFileLibInternal.h"

// OpenFileSystem
/** Read file from device path

  @param[in] DevicePath   The whole device path to the file.
  @param[out] FileSystem  The size of the file read or 0

  @retval  A pointer to a buffer containing the file read or NULL
**/
EFI_STATUS
OpenFileSystem (
  IN     EFI_DEVICE_PATH_PROTOCOL         **DevicePath,
  IN OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **FileSystem
  )
{
  EFI_STATUS Status;
  EFI_HANDLE Handle;

  // Get our DeviceHandle

  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid,
    DevicePath,
    &Handle);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not locate EfiSimpleFileSystemProtocol for device handle %X - %r\n", DevicePath, Status));
  } else {
    // Open the SimpleFileSystem Protocol

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)FileSystem
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Could not open EfiSimpleFileSystemProtocol for device handle - %r\n", Status));
    }
  }

  return Status;
}

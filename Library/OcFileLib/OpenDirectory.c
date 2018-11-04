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

// OpenDirectory
/** Read file from device path

  Open
   EFI_FILE_MODE_READ
  Create
   (EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE)

  @param[in] DevicePath  The whole device path to the file.
  @param[out] FileSize   The size of the file read or 0

  @retval  A pointer to a buffer containing the file read or NULL
**/
EFI_STATUS
OpenDirectory (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *DirectoryPath,
  IN  UINT64                    Mode,
  IN  UINT64                    Attributes,
  OUT EFI_FILE                  **DirectoryHandle OPTIONAL
  )
{
  EFI_STATUS                      Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE                        *FileSystemRoot;
  EFI_FILE                        *Directory;

  Status = EFI_INVALID_PARAMETER;

  if ((DevicePath != NULL) && (DirectoryPath != NULL)) {
    // Open the Filesystem on our DeviceHandle.

    FileSystem = NULL;
    Status     = OpenFileSystem (
                   &DevicePath,
                   &FileSystem
                   );

    if (!EFI_ERROR (Status)) {
      // We neeed to open the target volume to be able to load files from it.
      // What we get is the filesystem root. This function also has to be called
      // if any further calls to FileSystem return EFI_MEDIA_CHANGED to indicate
      // that our volume has changed.

      do {
        FileSystemRoot = NULL;
        Status         = FileSystem->OpenVolume (FileSystem, &FileSystemRoot);


        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN, "Could not open the file system - %r\n", Status));

          break;
        }

        // Now we can try to open the target file on our filesystem.

        Directory = NULL;
        Status    = FileSystemRoot->Open (
                                      FileSystemRoot,
                                      &Directory,
                                      DirectoryPath,
                                      Mode,
                                      (Attributes | EFI_FILE_DIRECTORY)
                                      );

        if (!EFI_ERROR (Status)) {
          break;
        }
      } while (Status == EFI_MEDIA_CHANGED);
    }

    if (DirectoryHandle != NULL) {
      *DirectoryHandle = Directory;
    } else {
       Directory->Close (Directory);
    }

  }

  return Status;
}

/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

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

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>

#include "OcFileLibInternal.h"

// GetFileInfo
/**

  @param[in] DevicePath  A pointer to the device path to device.
  @param[in] Directory   A pointer to the directory that contains the file.
  @param[in] FileName    A pointer to the the filename.
  @param[out] FileInfo   A pointer to the FILE_INFO structure returned or NULL

  @retval EFI_SUCCESS  The FILE_INFO structure was successfully returned.
**/
EFI_STATUS
GetFileInfo (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *Directory,
  IN  CHAR16                    *FileName,
  OUT EFI_FILE_INFO             **FileInfo
  )
{
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE                        *FileSystemRoot;

  EFI_STATUS                      Status;
  EFI_FILE                        *File;
  UINTN                           FileInfoSize;
  CHAR16                          *FilePath;
  UINTN                           FilePathSize;

  Status = EFI_INVALID_PARAMETER;

  if ((DevicePath != NULL) && (Directory != NULL) && (FileName != NULL)) {

    Status       = EFI_OUT_OF_RESOURCES;
    FilePathSize = StrSize (Directory) + StrSize (FileName);
    FilePath     = AllocateZeroPool (FilePathSize);

    if (FilePath != NULL) {

      CHAR16 *TempFilePath = FilePath;

      while (*Directory != 0) {
        *(TempFilePath++) = *(Directory++);
      }

      while (*FileName != 0) {
        *(TempFilePath++) = *(FileName++);
      }

      *TempFilePath = 0;

      // Open the Filesystem on our DeviceHandle.

      FileSystem = NULL;
      Status     = OpenFileSystem (
                     &DevicePath,
                     &FileSystem
                     );

      if (!EFI_ERROR (Status)) {
        // We need to open the target volume to be able to load files from it.
        // What we get is the filesystem root. This function also has to be called
        // if any further calls to FileSystem return EFI_MEDIA_CHANGED to indicate
        // that our volume has changed.

        do {
          FileSystemRoot = NULL;
          Status         = FileSystem->OpenVolume (
                                         FileSystem,
                                         &FileSystemRoot
                                         );

          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_WARN, "Could not open the file system - %r\n", Status));
            
            break;
          }

          // Now we can try to open the target file on our filesystem.

          File   = NULL;
          Status = FileSystemRoot->Open (
                                     FileSystemRoot,
                                     &File,
                                     FilePath,
                                     EFI_FILE_MODE_READ,
                                     EFI_FILE_READ_ONLY
                                     );

          if (!EFI_ERROR (Status)) {
            // Try to retrieve information of our file.

            FileInfoSize = 0;
            Status       = File->GetInfo (
                                   File,
                                   &gEfiFileInfoGuid,
                                   &FileInfoSize,
                                   NULL
                                   );

            if (Status == EFI_BUFFER_TOO_SMALL) {
              // The first call to this function we get the right size of the
              // FileInfo buffer, so we allocate it with that size and call the function again.
              //
              // Some drivers do not count 0 at the end of file name

              *FileInfo = AllocateZeroPool (FileInfoSize + sizeof (CHAR16));

              if (*FileInfo != NULL) {
                Status = File->GetInfo (
                                 File,
                                 &gEfiFileInfoGuid,
                                 &FileInfoSize,
                                 *FileInfo
                                 );
              }
            }
          }

          if (!EFI_ERROR (Status)) {
            break;
          }

          // If we get the EFI_MEDIA_CHANGED error, we need to reopen the volume by calling
          // OpenVolume() on our DeviceHandle
        } while (Status == EFI_MEDIA_CHANGED);
      } 

      FreePool ((VOID *)FilePath);
    }
  }

  return Status;
}

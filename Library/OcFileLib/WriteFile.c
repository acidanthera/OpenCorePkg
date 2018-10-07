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
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Macros.h>

#include "OcFileLibInternal.h"

// SaveFile
/** Save filebuffer to device path

  @param[in]  DevicePath     The device path for the device.
  @param[in]  DirectoryPath  The directory path to place the file on the device.
  @param[in]  FilePath       The filename to use when saving the file on the device.
  @param[out] FileBuffer     A pointer to a buffer containing the file read or NULL
  @param[out] FileSize       The size of the file buffer to save

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
EFI_STATUS
SaveFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *Directory,
  IN  CHAR16                    *FileName,
  IN  VOID                      *FileBuffer,
  IN  UINTN                     FileSize
  )
{
  EFI_STATUS    Status;
  CHAR16        *FilePath;
  UINTN         FilePathSize;

  DEBUG_FUNCTION_ENTRY (DEBUG_VERBOSE);

  Status = EFI_INVALID_PARAMETER;

  if (DevicePath != NULL && Directory != NULL && FileName != NULL && FileName != NULL && FileSize != 0) {

    Status       = EFI_OUT_OF_RESOURCES;
    FilePathSize = StrSize (Directory) + StrSize (FileName);
    FilePath     = AllocateZeroPool (FilePathSize);

    if (FilePath != NULL) {

      StrCpyS (FilePath, FilePathSize, Directory);
      StrCatS (FilePath, FilePathSize, FileName);

      Status = WriteFilePath (
                 DevicePath,
                 FilePath,
                 FileBuffer,
                 FileSize,
                 EFI_FILE_ARCHIVE
                 );

      FreePool (FilePath);

    }
  }

  DEBUG_FUNCTION_RETURN (DEBUG_VERBOSE);

  return Status;
}

// WriteFilePath
/** Write file to device path

  @param[in]  DevicePath     The device path to the device to create the file on.
  @param[in]  FilePath       The full path to use when writing the file on the device.
  @param[in]  FileBuffer     A pointer to a buffer containing the file contents.
  @param[in]  FileSize       The size of the file buffer to save
  @param[in]  Attributes

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
EFI_STATUS
WriteFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *FilePath,
  IN  VOID                      *FileBuffer,
  IN  UINTN                     FileSize,
  IN  UINT64                    Attributes
  )
{
  EFI_STATUS              Status;
  EFI_FILE                *Folder;
  EFI_FILE                *File;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

  Status = EFI_INVALID_PARAMETER;

  // Allow creation of zero length files

  if (DevicePath != NULL && FilePath != NULL && FileBuffer != NULL) {

    // Open the Filesystem on our DevicePath.

    FileSystem = NULL;
    Status     = OpenFileSystem (
                   &DevicePath,
                   &FileSystem
                   );

    if (!EFI_ERROR (Status)) {

Reopen:

      Folder = NULL;
      Status = FileSystem->OpenVolume (
                             FileSystem,
                             &Folder
                             );

      if (!EFI_ERROR (Status)) {

        // Now we can try to open the target file on our filesystem.

        File   = NULL;
        Status = Folder->Open (
                           Folder,
                           &File,
                           FilePath,
                           EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                           Attributes
                           );

        if (EFI_ERROR (Status)) {

          if (Status == EFI_MEDIA_CHANGED) {
            goto Reopen;
          }

        } else {

          Status = File->Write (
                           File,
                           &FileSize,
                           FileBuffer
                           );
        }

        if (File != NULL) {
          File->Close (File);        
        }

      }

      if (Folder != NULL) {
        Folder->Close (Folder);                
      }

    }

  }

  return Status;

}

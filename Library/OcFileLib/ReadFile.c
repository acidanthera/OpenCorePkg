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

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcDevicePathLib.h>

#include "OcFileLibInternal.h"

// ReadFileFromDevicePath
/** Read file from device path

  @param[in] DevicePath  The whole device path to the file.
  @param[out] FileSize   The size of the file read or 0

  @retval  A pointer to a buffer containing the file read or NULL
**/
VOID *
ReadFileFromDevicePath (
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN OUT UINTN                     *FileSize
  )
{
  VOID                            *FileBuffer;

  EFI_STATUS                      Status;
  EFI_HANDLE                      Handle;
  EFI_FILE_HANDLE                 FileHandle;
  EFI_FILE_HANDLE                 LastHandle;
  EFI_FILE_INFO                   *FileInfo;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL        *OrigDevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePathNode;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  UINTN                           FileInfoSize;

  FileBuffer = NULL;

  if ((DevicePath != NULL) && (FileSize != NULL)) {
    OrigDevicePathNode = DuplicateDevicePath (DevicePath);

    if (OrigDevicePathNode != NULL) {
      DevicePathNode = OrigDevicePathNode;
      Status         = gBS->LocateDevicePath (
                              &gEfiSimpleFileSystemProtocolGuid,
                              &DevicePathNode,
                              &Handle
                              );

      if (!EFI_ERROR (Status)) {
        Volume = NULL;
        Status = gBS->HandleProtocol (
                        Handle,
                        &gEfiSimpleFileSystemProtocolGuid,
                        (VOID**)&Volume
                        );

        if (!EFI_ERROR (Status)) {
          // Open the volume to get the file system handle
          Status = Volume->OpenVolume (Volume, &FileHandle);

          if (!EFI_ERROR (Status)) {
            // Duplicate the device path to avoid the access to unaligned device path node.
            // Because the device path consists of one or more FILE PATH MEDIA DEVICE PATH
            // nodes, It assures the fields in device path nodes are 2 byte aligned.
            TempDevicePathNode = DuplicateDevicePath (DevicePathNode);
            Status             = EFI_OUT_OF_RESOURCES;

            if (TempDevicePathNode != NULL) {
              // Parse each MEDIA_FILEPATH_DP node. There may be more than one, since the
              // directory information and filename can be seperate. The goal is to inch
              // our way down each device path node and close the previous node

              DevicePathNode = TempDevicePathNode;

              while (!IsDevicePathEnd (DevicePathNode)) {
                if ((DevicePathType (DevicePathNode) != MEDIA_DEVICE_PATH)
                 || (DevicePathSubType (DevicePathNode) != MEDIA_FILEPATH_DP)) {
                  Status = EFI_UNSUPPORTED;

                  break;
                }

                LastHandle = FileHandle;
                FileHandle = NULL;

                Status = LastHandle->Open (
                                       LastHandle,
                                       &FileHandle,
                                       ((FILEPATH_DEVICE_PATH *)DevicePathNode)->PathName,
                                       EFI_FILE_MODE_READ,
                                       0
                                       );

                // Close the previous node
                LastHandle->Close (LastHandle);

                if (EFI_ERROR (Status)) {
                  FileHandle = NULL;

                  break;
                }

                DevicePathNode = NextDevicePathNode (DevicePathNode);
              }

              if (!EFI_ERROR (Status)) {
                // We have found the file. Now we need to read it. Before we can read the file we need to
                // figure out how big the file is.

                FileInfoSize = 0;
                Status       = FileHandle->GetInfo (
                                             FileHandle,
                                             &gEfiFileInfoGuid,
                                             &FileInfoSize,
                                             NULL
                                             );

                if (Status == EFI_BUFFER_TOO_SMALL) {
                  // Some drivers do not count 0 at the end of file name
                  FileInfo = AllocatePool (FileInfoSize + sizeof (CHAR16));

                  if (FileInfo != NULL) {
                    Status = FileHandle->GetInfo (
                                           FileHandle,
                                           &gEfiFileInfoGuid,
                                           &FileInfoSize,
                                           FileInfo
                                           );

                    if (!EFI_ERROR (Status)) {
                      // Allocate space for the file
                      FileBuffer = AllocatePool ((UINTN)FileInfo->FileSize);

                      if (FileBuffer != NULL) {
                        // Read the file into the buffer we allocated

                        *FileSize = (UINTN)FileInfo->FileSize;
                        Status    = FileHandle->Read (
                                                   FileHandle,
                                                   FileSize,
                                                   FileBuffer
                                                   );

                        if (EFI_ERROR (Status)) {
                          FreePool ((VOID *)FileBuffer);

                          FileBuffer = NULL;
                          *FileSize  = 0;
                        }
                      }
                    }

                    FreePool ((VOID *)FileInfo);
                  }
                }
              }

              FreePool ((VOID *)TempDevicePathNode);
            }

            if (FileHandle != NULL) {
              FileHandle->Close (FileHandle);
            }
          }
        }
      }

      FreePool ((VOID *)OrigDevicePathNode);
    }
  }

  return FileBuffer;
}

// ReadFile
/** Read file from device path

  @param[in] DevicePath   The device path for the device.
  @param[in] FilePath     The full path to the file on the device.
  @param[out] FileBuffer  A pointer to a buffer containing the file read or NULL
  @param[out] FileSize    The size of the file read or 0

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
EFI_STATUS
ReadFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *FilePath,
  OUT VOID                      **FileBuffer,
  OUT UINTN                     *FileBufferSize
  )
{
  EFI_STATUS               Status;

  EFI_DEVICE_PATH_PROTOCOL *WholeDevicePath;

  Status = EFI_INVALID_PARAMETER;

  if ((DevicePath != NULL) && (FilePath != NULL) && (FileBuffer != NULL) && (FileBufferSize != NULL)) {
    Status = EFI_NOT_FOUND;

    *FileBuffer     = NULL;
    *FileBufferSize = 0;

    WholeDevicePath = AppendFileNameDevicePath (DevicePath, FilePath);

    if (WholeDevicePath != NULL) {
      *FileBuffer = ReadFileFromDevicePath (WholeDevicePath, FileBufferSize);

      if (*FileBuffer != NULL) {
        Status = EFI_SUCCESS;
      }

      FreePool (WholeDevicePath);
    }
  }

  return Status;
}


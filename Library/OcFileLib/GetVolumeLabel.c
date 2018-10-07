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
#include <Guid/FileSystemVolumeLabelInfo.h>

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

// GetVolumeLabel
/**

  @param[in] DevicePath    A pointer to the device path to retrieve the volume label from.
  @param[out] VolumeLabel  A pointer to the NULL terminated unicode volume label.

  @retval EFI_SUCCESS  The volume label was successfully returned.
**/
EFI_STATUS
GetVolumeLabel (
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN OUT CHAR16                    **VolumeLabel
  )
{
  EFI_STATUS                      Status;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_HANDLE                 File;
  EFI_FILE_SYSTEM_VOLUME_LABEL    *VolumeInfo;
  UINTN                           FileSize;

  Status = EFI_INVALID_PARAMETER;

  if ((DevicePath != NULL) && (VolumeLabel != NULL)) {
    // Open the Filesystem on our DevicePath.

    FileSystem = NULL;
    Status     = OpenFileSystem (
                   &DevicePath,
                   &FileSystem
                   );

    if (!EFI_ERROR (Status)) {
      File   = NULL;
      Status = FileSystem->OpenVolume (
                             FileSystem,
                             &File
                             );

      if (!EFI_ERROR (Status)) {
        FileSize   = 0;
        VolumeInfo = NULL;
        Status     = File->GetInfo (
                             File,
                             &gEfiFileSystemVolumeLabelInfoIdGuid,
                             &FileSize,
                             VolumeInfo
                             );

        if (Status == EFI_BUFFER_TOO_SMALL) {
          VolumeInfo = AllocateZeroPool (FileSize);
          Status     = File->GetInfo (
                               File,
                               &gEfiFileSystemVolumeLabelInfoIdGuid,
                               &FileSize,
                               VolumeInfo
                               );

          if (!EFI_ERROR (Status)) {
            *VolumeLabel = AllocateCopyPool (StrSize (VolumeInfo->VolumeLabel), VolumeInfo->VolumeLabel);

            FreePool ((VOID *)VolumeInfo);
          }
        }
      }
    }
  }

  return Status;
}

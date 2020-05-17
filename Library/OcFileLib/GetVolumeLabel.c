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
#include <Guid/FileSystemVolumeLabelInfo.h>

#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

CHAR16 *
GetVolumeLabel (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem
  )
{
  EFI_STATUS                      Status;

  EFI_FILE_HANDLE                 Volume;
  EFI_FILE_SYSTEM_VOLUME_LABEL    *VolumeInfo;
  UINTN                           VolumeLabelSize;

  ASSERT (FileSystem != NULL);

  Volume   = NULL;
  Status = FileSystem->OpenVolume (
                         FileSystem,
                         &Volume
                         );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  VolumeInfo = GetFileInfo (
    Volume,
    &gEfiFileSystemVolumeLabelInfoIdGuid,
    sizeof (EFI_FILE_SYSTEM_VOLUME_LABEL),
    &VolumeLabelSize
    );

  Volume->Close (Volume);

  STATIC_ASSERT (
    OFFSET_OF(EFI_FILE_SYSTEM_VOLUME_LABEL, VolumeLabel) == 0,
    "Expected EFI_FILE_SYSTEM_VOLUME_LABEL to represent CHAR16 string!"
    );

  if (VolumeInfo != NULL) {
    if (VolumeLabelSize >= sizeof (CHAR16) && VolumeInfo->VolumeLabel[0] != L'\0') {
      //
      // The spec requires disk label to be NULL-terminated, but it
      // was unclear whether the size should contain terminator or not.
      // Some old HFS Plus drivers provide volume label size without
      // terminating \0 (though they do append it). These drivers must
      // not be used, but we try not to die when debugging is off.
      //
      if (VolumeInfo->VolumeLabel[VolumeLabelSize / sizeof (CHAR16) - 1] != '\0'
        || VolumeLabelSize > OC_MAX_VOLUME_LABEL_SIZE * sizeof (CHAR16)) {
        DEBUG ((DEBUG_ERROR, "OCFS: Found unterminated or too long volume label!"));
        FreePool (VolumeInfo);
        return AllocateCopyPool (sizeof (L"INVALID"), L"INVALID");
      } else {
        UnicodeFilterString (VolumeInfo->VolumeLabel, TRUE);
        return VolumeInfo->VolumeLabel;
      }
    }
    FreePool (VolumeInfo);
  }

  return AllocateCopyPool (sizeof (L"NO NAME"), L"NO NAME");
}

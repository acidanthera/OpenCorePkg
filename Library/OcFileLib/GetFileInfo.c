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

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>

VOID *
GetFileInfo (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  EFI_GUID           *InformationType,
  IN  UINTN              MinFileInfoSize,
  OUT UINTN              *RealFileInfoSize  OPTIONAL
  )
{
  VOID       *FileInfoBuffer;

  UINTN      FileInfoSize;
  EFI_STATUS Status;

  FileInfoSize   = 0;
  FileInfoBuffer = NULL;

  Status = File->GetInfo (
                   File,
                   InformationType,
                   &FileInfoSize,
                   NULL
                   );

  if (Status == EFI_BUFFER_TOO_SMALL && FileInfoSize >= MinFileInfoSize) {
    //
    // Some drivers (i.e. built-in 32-bit Apple HFS driver) may possibly omit null terminators from file info data.
    //
    if (CompareGuid (InformationType, &gEfiFileInfoGuid) && OcOverflowAddUN (FileInfoSize, sizeof (CHAR16), &FileInfoSize)) {
      return NULL;
    }
    FileInfoBuffer = AllocateZeroPool (FileInfoSize);

    if (FileInfoBuffer != NULL) {
      Status = File->GetInfo (
                       File,
                       InformationType,
                       &FileInfoSize,
                       FileInfoBuffer
                       );

      if (!EFI_ERROR (Status)) {
        if (RealFileInfoSize != NULL) {
          *RealFileInfoSize = FileInfoSize;
        }
      } else {
        FreePool (FileInfoBuffer);

        FileInfoBuffer = NULL;
      }
    }
  }

  return FileInfoBuffer;
}
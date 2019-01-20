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
ReadFile (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *FilePath,
  OUT UINTN                            *FileSize  OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EFI_FILE_HANDLE                 Volume;
  EFI_FILE_HANDLE                 FileHandle;
  EFI_FILE_INFO                   *FileInfo;
  UINT8                           *FileBuffer;
  UINTN                           FileBufferSize;
  UINTN                           FileReadSize;

  ASSERT (FileSystem != NULL);
  ASSERT (FilePath != NULL);

  Status = FileSystem->OpenVolume (
    FileSystem,
    &Volume
    );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = Volume->Open (
    Volume,
    &FileHandle,
    (CHAR16 *) FilePath,
    EFI_FILE_MODE_READ,
    0
    );

  if (EFI_ERROR (Status)) {
    Volume->Close (Volume);
    return NULL;
  }

  FileInfo = GetFileInfo (
    FileHandle,
    &gEfiFileInfoGuid,
    sizeof (EFI_FILE_INFO),
    NULL
    );

  if (FileInfo == NULL
    || OcOverflowAddUN(FileInfo->FileSize, sizeof (CHAR16), &FileBufferSize)) {
    FileHandle->Close (FileHandle);
    Volume->Close (Volume);
    return NULL;
  }

  FileBuffer = AllocatePool (FileBufferSize);

  if (FileBuffer != NULL) {
    FileReadSize = FileInfo->FileSize;
    Status = FileHandle->Read (
      FileHandle,
      &FileReadSize,
      FileBuffer
      );

    if (!EFI_ERROR (Status) && FileReadSize == FileInfo->FileSize) {
      FileBuffer[FileInfo->FileSize] = 0;
      FileBuffer[FileInfo->FileSize + 1] = 0;
      if (FileSize != NULL) {
        *FileSize = FileInfo->FileSize;
      }
    } else {
      FreePool (FileBuffer);
      FileBuffer = NULL;
    }
  }

  FileHandle->Close (FileHandle);
  Volume->Close (Volume);
  return FileBuffer;
}

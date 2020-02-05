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
  OUT UINT32                           *FileSize   OPTIONAL,
  IN  UINT32                           MaxFileSize OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EFI_FILE_HANDLE                 Volume;
  EFI_FILE_HANDLE                 FileHandle;
  UINT8                           *FileBuffer;
  UINT32                          FileBufferSize;
  UINT32                          FileReadSize;

  ASSERT (FileSystem != NULL);
  ASSERT (FilePath != NULL);

  Status = FileSystem->OpenVolume (
    FileSystem,
    &Volume
    );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = SafeFileOpen (
    Volume,
    &FileHandle,
    (CHAR16 *) FilePath,
    EFI_FILE_MODE_READ,
    0
    );

  Volume->Close (Volume);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = GetFileSize (
    FileHandle,
    &FileReadSize
    );
  if (EFI_ERROR (Status)
    || OcOverflowAddU32 (FileReadSize, sizeof (CHAR16), &FileBufferSize)
    || (MaxFileSize > 0 && FileReadSize > MaxFileSize)) {
    FileHandle->Close (FileHandle);
    return NULL;
  }

  FileBuffer = AllocatePool (FileBufferSize);
  if (FileBuffer != NULL) {
    Status = GetFileData (
      FileHandle,
      0,
      FileReadSize,
      FileBuffer
      );

    if (!EFI_ERROR (Status)) {
      FileBuffer[FileReadSize] = 0;
      FileBuffer[FileReadSize + 1] = 0;
      if (FileSize != NULL) {
        *FileSize = FileReadSize;
      }
    } else {
      FreePool (FileBuffer);
      FileBuffer = NULL;
    }
  }

  FileHandle->Close (FileHandle);
  return FileBuffer;
}

EFI_STATUS
ReadFileSize (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *FilePath,
  OUT UINT32                           *Size
  )
{
  EFI_STATUS                      Status;
  EFI_FILE_HANDLE                 Volume;
  EFI_FILE_HANDLE                 FileHandle;

  ASSERT (FileSystem != NULL);
  ASSERT (FilePath != NULL);
  ASSERT (Size != NULL);

  Status = FileSystem->OpenVolume (
    FileSystem,
    &Volume
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SafeFileOpen (
    Volume,
    &FileHandle,
    (CHAR16 *) FilePath,
    EFI_FILE_MODE_READ,
    0
    );

  Volume->Close (Volume);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetFileSize (
    FileHandle,
    Size
    );

  return Status;
}

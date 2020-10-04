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

VOID *
ReadFileFromFile (
  IN  EFI_FILE_PROTOCOL   *RootFile,
  IN  CONST CHAR16        *FilePath,
  OUT UINT32              *FileSize OPTIONAL,
  IN  UINT32              MaxFileSize OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_FILE_PROTOCOL     *File;
  UINT32                Size;
  UINT8                 *FileBuffer;

  ASSERT (RootFile != NULL);
  ASSERT (FilePath != NULL);

  Status = SafeFileOpen (
    RootFile,
    &File,
    (CHAR16 *) FilePath,
    EFI_FILE_MODE_READ,
    0
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = GetFileSize (File, &Size);
  if (EFI_ERROR (Status)
    || Size >= MAX_UINT32 - 1
    || (MaxFileSize > 0 && Size > MaxFileSize)) {
    File->Close (File);
    return NULL;
  }

  FileBuffer = AllocatePool (Size + 2);
  if (FileBuffer == NULL) {
    File->Close (File);
    return NULL;
  }

  Status = GetFileData (File, 0, Size, FileBuffer);
  File->Close (File);
  if (EFI_ERROR (Status)) {
    FreePool (FileBuffer);
    return NULL;
  }

  FileBuffer[Size]     = 0;
  FileBuffer[Size + 1] = 0;

  if (FileSize != NULL) {
    *FileSize = Size;
  }

  return FileBuffer;
}

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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>

EFI_STATUS
ReadFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       ReadSize;

  Status = File->SetPosition (File, Position);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ReadSize = Size;
  Status = File->Read (File, &ReadSize, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ReadSize != Size) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ReadFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  EFI_STATUS  Status;
  UINT64      Position;

  Status = File->SetPosition (File, 0xFFFFFFFFFFFFFFFFULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = File->GetPosition (File, &Position);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((UINT32) Position != Position) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Size = (UINT32) Position;

  return EFI_SUCCESS;
}

EFI_STATUS
ReadFileModifcationTime (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT EFI_TIME           *Time
  )
{
  EFI_STATUS     Status;
  UINTN          BufferSize;
  EFI_FILE_INFO  *FileInfo;

  BufferSize = 0;
  Status = File->GetInfo (File, &gEfiFileInfoGuid, &BufferSize, NULL);

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  if (BufferSize < sizeof (EFI_FILE_INFO)) {
    return EFI_INVALID_PARAMETER;
  }

  FileInfo = (EFI_FILE_INFO *) AllocatePool (BufferSize);
  if (FileInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = File->GetInfo (File, &gEfiFileInfoGuid, &BufferSize, FileInfo);
  if (EFI_ERROR (Status)) {
    FreePool (FileInfo);
    return Status;
  }

  CopyMem (Time, &FileInfo->ModificationTime, sizeof (*Time));
  FreePool (FileInfo);

  return EFI_SUCCESS;
}

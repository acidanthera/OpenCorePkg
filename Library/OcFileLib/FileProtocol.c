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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
GetFileData (
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
  File->SetPosition (File, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ReadSize != Size) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetFileSize (
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
  File->SetPosition (File, 0);
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
GetFileModificationTime (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT EFI_TIME           *Time
  )
{
  EFI_FILE_INFO  *FileInfo;

  FileInfo = GetFileInfo (File, &gEfiFileInfoGuid, 0, NULL);
  if (FileInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Time, &FileInfo->ModificationTime, sizeof (*Time));
  FreePool (FileInfo);

  return EFI_SUCCESS;
}

EFI_STATUS
FindWritableFileSystem (
  IN OUT EFI_FILE_PROTOCOL  **WritableFs
  )
{
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            HandleCount;
  UINTN                            Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs;
  EFI_FILE_PROTOCOL                *Fs;
  EFI_FILE_PROTOCOL                *File;

  //
  // Locate all the simple file system devices in the system.
  //
  EFI_STATUS Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
      HandleBuffer[Index],
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID **) &SimpleFs
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "OCFS: FindWritableFileSystem gBS->HandleProtocol[%u] returned %r\n",
        (UINT32) Index,
        Status
        ));
      continue;
    }
    
    Status = SimpleFs->OpenVolume (SimpleFs, &Fs);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "OCFS: FindWritableFileSystem SimpleFs->OpenVolume[%u] returned %r\n",
        (UINT32) Index,
        Status
        ));
      continue;
    }
    
    //
    // We cannot test if the file system is writeable without attempting to create some file.
    //
    Status = SafeFileOpen (
      Fs,
      &File,
      L"octest.fil",
      EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
      0
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "OCFS: FindWritableFileSystem Fs->Open[%u] returned %r\n",
        (UINT32) Index,
        Status
        ));
      continue;
    }
    
    //
    // Delete the temporary file and report the found file system.
    //
    Fs->Delete (File);
    *WritableFs = Fs;
    break;
  }
  
  gBS->FreePool (HandleBuffer);
  
  return Status;
}

EFI_STATUS
SetFileData (
  IN EFI_FILE_PROTOCOL  *WritableFs OPTIONAL,
  IN CONST CHAR16       *FileName,
  IN CONST VOID         *Buffer,
  IN UINT32             Size
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *Fs;
  EFI_FILE_PROTOCOL  *File;
  UINTN              WrittenSize;

  if (WritableFs == NULL) {
    Status = FindWritableFileSystem (&Fs);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "OCFS: WriteFileData can't find writable FS\n"));
      return Status;
    }
  } else {
    Fs = WritableFs;
  }

  Status = SafeFileOpen (
    Fs,
    &File,
    (CHAR16 *) FileName,
    EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
    0
    );

  if (!EFI_ERROR (Status)) {
    WrittenSize = Size;
    Status = File->Write (File, &WrittenSize, (VOID *) Buffer);
    Fs->Close (File);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "OCFS: WriteFileData file->Write returned %r\n", Status));
    } else if (WrittenSize != Size) {
      DEBUG ((
        DEBUG_VERBOSE,
        "WriteFileData: File->Write truncated %u to %u\n",
        Status,
        Size,
        (UINT32) WrittenSize
        ));
      Status = EFI_BAD_BUFFER_SIZE;
    }
  } else {
    DEBUG ((DEBUG_VERBOSE, "OCFS: WriteFileData Fs->Open of %s returned %r\n", FileName, Status));
  }

  if (WritableFs == NULL) {
    Fs->Close (Fs);
  } 

  return Status;
}

EFI_STATUS
AllocateCopyFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT8              **Buffer,
  OUT UINT32             *BufferSize
  )
{
  EFI_STATUS    Status;
  UINT8         *FileBuffer;
  UINT32        ReadSize;

  //
  // Get full file data.
  //
  Status = GetFileSize (File, &ReadSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileBuffer = AllocatePool (ReadSize);
  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = GetFileData (File, 0, ReadSize, FileBuffer);
  if (EFI_ERROR (Status)) {
    FreePool (FileBuffer);
    return Status;
  }

  *Buffer     = FileBuffer;
  *BufferSize = ReadSize;
  return EFI_SUCCESS;
}

/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <NTFS.h>
#include <Helper.h>

#include <UserFile.h>
#include <UserGlobalVar.h>

UINTN        mFuzzOffset;
UINTN        mFuzzSize;
CONST UINT8  *mFuzzPointer;

EFI_STATUS
EFIAPI
FuzzReadDisk (
  IN  EFI_DISK_IO_PROTOCOL  *This,
  IN  UINT32                MediaId,
  IN  UINT64                Offset,
  IN  UINTN                 BufferSize,
  OUT VOID                  *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((mFuzzSize - mFuzzOffset) < BufferSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Buffer, mFuzzPointer, BufferSize);
  mFuzzPointer += BufferSize;
  mFuzzOffset  += BufferSize;

  return EFI_SUCCESS;
}

VOID
FreeAll (
  IN CHAR16  *FileName,
  IN EFI_FS  *Instance
  )
{
  FreePool (FileName);

  if (Instance != NULL) {
    if (Instance->DiskIo != NULL) {
      FreePool (Instance->DiskIo);
    }

    if (Instance->BlockIo != NULL) {
      if (Instance->BlockIo->Media != NULL) {
        FreePool (Instance->BlockIo->Media);
      }

      FreePool (Instance->BlockIo);
    }

    if (Instance->RootIndex != NULL) {
      FreeAttr (&Instance->RootIndex->Attr);
      FreeAttr (&Instance->MftStart->Attr);
      FreePool (Instance->RootIndex->FileRecord);
      FreePool (Instance->MftStart->FileRecord);
      FreePool (Instance->RootIndex->File);
    }

    FreePool (Instance);
  }
}

INT32
LLVMFuzzerTestOneInput (
  CONST UINT8  *FuzzData,
  UINTN        FuzzSize
  )
{
  EFI_STATUS         Status;
  EFI_FS             *Instance;
  EFI_FILE_PROTOCOL  *This;
  UINTN              BufferSize;
  VOID               *Buffer;
  VOID               *TmpBuffer;
  EFI_FILE_PROTOCOL  *NewHandle;
  CHAR16             *FileName;
  VOID               *Info;
  UINTN              Len;
  UINT64             Position;

  mFuzzOffset  = 0;
  mFuzzPointer = FuzzData;
  mFuzzSize    = FuzzSize;

  Instance   = NULL;
  BufferSize = 100;

  //
  // Construct File Name
  //
  FileName = AllocateZeroPool (BufferSize);
  if (FileName == NULL) {
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (FileName, BufferSize);

  if ((mFuzzSize - mFuzzOffset) < BufferSize) {
    FreeAll (FileName, Instance);
    return 0;
  }

  CopyMem (FileName, mFuzzPointer, BufferSize - 2);
  mFuzzPointer += BufferSize - 2;
  mFuzzOffset  += BufferSize - 2;

  //
  // Construct File System
  //
  Instance = AllocateZeroPool (sizeof (EFI_FS));
  if (Instance == NULL) {
    FreeAll (FileName, Instance);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Instance, sizeof (EFI_FS));

  Instance->DiskIo = AllocateZeroPool (sizeof (EFI_DISK_IO_PROTOCOL));
  if (Instance->DiskIo == NULL) {
    FreeAll (FileName, Instance);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Instance->DiskIo, sizeof (EFI_DISK_IO_PROTOCOL));

  Instance->DiskIo->ReadDisk = FuzzReadDisk;

  Instance->BlockIo = AllocateZeroPool (sizeof (EFI_BLOCK_IO_PROTOCOL));
  if (Instance->BlockIo == NULL) {
    FreeAll (FileName, Instance);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Instance->BlockIo, sizeof (EFI_BLOCK_IO_PROTOCOL));

  Instance->BlockIo->Media = AllocateZeroPool (sizeof (EFI_BLOCK_IO_MEDIA));
  if (Instance->BlockIo->Media == NULL) {
    FreeAll (FileName, Instance);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Instance->BlockIo->Media, sizeof (EFI_BLOCK_IO_MEDIA));

  Instance->EfiFile.Revision    = EFI_FILE_PROTOCOL_REVISION2;
  Instance->EfiFile.Open        = FileOpen;
  Instance->EfiFile.Close       = FileClose;
  Instance->EfiFile.Delete      = FileDelete;
  Instance->EfiFile.Read        = FileRead;
  Instance->EfiFile.Write       = FileWrite;
  Instance->EfiFile.GetPosition = FileGetPosition;
  Instance->EfiFile.SetPosition = FileSetPosition;
  Instance->EfiFile.GetInfo     = FileGetInfo;
  Instance->EfiFile.SetInfo     = FileSetInfo;
  Instance->EfiFile.Flush       = FileFlush;

  Status = NtfsMount (Instance);
  if (EFI_ERROR (Status)) {
    FreeAll (FileName, Instance);
    return 0;
  }

  This = (EFI_FILE_PROTOCOL *)Instance->RootIndex->File;

  //
  // Test Ntfs Driver
  //
  Status = FileOpen (This, &NewHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (Status == EFI_SUCCESS) {
    Buffer     = AllocateZeroPool (100);
    BufferSize = 100;
    if (Buffer == NULL) {
      FreeAll (FileName, Instance);
      return 0;
    }

    Status = FileRead (NewHandle, &BufferSize, Buffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      TmpBuffer = ReallocatePool (100, BufferSize, Buffer);
      if (TmpBuffer == NULL) {
        FreePool (Buffer);
        FreeAll (FileName, Instance);
        return 0;
      }

      Buffer = TmpBuffer;

      ASAN_CHECK_MEMORY_REGION (Buffer, BufferSize);

      FileRead (NewHandle, &BufferSize, Buffer);
    }

    FileWrite (NewHandle, &BufferSize, Buffer);

    FileFlush (NewHandle);

    FreePool (Buffer);

    Len    = 0;
    Info   = NULL;
    Status = FileGetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      if (Info == NULL) {
        FreeAll (FileName, Instance);
        return 0;
      }

      FileGetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
      FreePool (Info);
    }

    Len    = 0;
    Status = FileGetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      if (Info == NULL) {
        FreeAll (FileName, Instance);
        return 0;
      }

      FileGetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
      FreePool (Info);
    }

    Len    = 0;
    Status = FileGetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      if (Info == NULL) {
        FreeAll (FileName, Instance);
        return 0;
      }

      FileGetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
      FreePool (Info);
    }

    FileSetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, Len, Info);

    FileGetPosition (NewHandle, &Position);
    while (!EFI_ERROR (FileSetPosition (NewHandle, Position))) {
      ++Position;
    }

    FileDelete (NewHandle);
  }

  FreeAll (FileName, Instance);

  return 0;
}

int
ENTRY_POINT (
  int   argc,
  char  **argv
  )
{
  uint32_t  f;
  uint8_t   *b;

  if ((b = UserReadFile ((argc > 1) ? argv[1] : "in.bin", &f)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail\n"));
    return -1;
  }

  LLVMFuzzerTestOneInput (b, f);
  FreePool (b);
  return 0;
}

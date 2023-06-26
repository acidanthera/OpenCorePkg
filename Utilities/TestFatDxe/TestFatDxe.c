/** @file
  Copyright (c) 2023, Savva Mitrofanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Fat.h>

#include <UserFile.h>
#include <UserGlobalVar.h>
#include <UserMemory.h>
#include <UserUnicodeCollation.h>
#include <string.h>

extern EFI_UNICODE_COLLATION_PROTOCOL  *mUnicodeCollationInterface;

STATIC EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *mEfiSfsInterface;

UINTN        mFuzzOffset;
UINTN        mFuzzSize;
CONST UINT8  *mFuzzPointer;

EFI_STATUS
EFIAPI
FuzzBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

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
    return EFI_DEVICE_ERROR;
  }

  CopyMem (Buffer, mFuzzPointer, BufferSize);
  mFuzzPointer += BufferSize;
  mFuzzOffset  += BufferSize;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WrapInstallMultipleProtocolInterfaces (
  IN OUT EFI_HANDLE  *Handle,
  ...
  )
{
  VA_LIST     Args;
  EFI_STATUS  Status;
  EFI_GUID    *Protocol;
  VOID        *Interface;

  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VA_START (Args, Handle);
  for (Status = EFI_SUCCESS; !EFI_ERROR (Status);) {
    //
    // If protocol is NULL, then it's the end of the list
    //
    Protocol = VA_ARG (Args, EFI_GUID *);
    if (Protocol == NULL) {
      break;
    }

    Interface = VA_ARG (Args, VOID *);

    //
    // If this is Sfs protocol then save interface into global state
    //
    if (CompareGuid (Protocol, &gEfiSimpleFileSystemProtocolGuid)) {
      mEfiSfsInterface = Interface;
    }
  }

  VA_END (Args);

  return Status;
}

VOID
FreeAll (
  IN CHAR16             *FileName,
  IN FAT_VOLUME         *Volume,
  IN EFI_FILE_PROTOCOL  *RootDirInstance
  )
{
  FreePool (FileName);

  //
  // Closes root directory
  //
  if (RootDirInstance != NULL) {
    FatClose (RootDirInstance);
  }

  //
  // Abandon volume structure
  //
  if (Volume != NULL) {
    if (Volume->DiskIo != NULL) {
      FreePool (Volume->DiskIo);
    }

    if (Volume->BlockIo != NULL) {
      if (Volume->BlockIo->Media != NULL) {
        FreePool (Volume->BlockIo->Media);
      }

      FreePool (Volume->BlockIo);
    }

    if (Volume->Root != NULL) {
      FatSetVolumeError (
        Volume->Root,
        EFI_NO_MEDIA
        );
    }

    Volume->Valid = FALSE;

    FatCleanupVolume (Volume, NULL, EFI_SUCCESS, NULL);
  }
}

STATIC
INT32
TestFatDxe (
  CONST UINT8  *FuzzData,
  UINTN        FuzzSize
  )
{
  EFI_STATUS             Status;
  EFI_FILE_PROTOCOL      *VolumeRootDir;
  UINTN                  BufferSize;
  VOID                   *Buffer;
  EFI_FILE_PROTOCOL      *NewHandle;
  CHAR16                 *FileName;
  VOID                   *Info;
  UINTN                  Len;
  UINT64                 Position;
  EFI_DISK_IO_PROTOCOL   *DiskIo;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  FAT_VOLUME             *Volume;
  FAT_IFILE              *FileInstance;
  UINT64                 FileSize;

  EFI_HANDLE  DeviceHandle;

  BufferSize = 100;

  mFuzzOffset      = 0;
  mFuzzSize        = FuzzSize;
  mFuzzPointer     = FuzzData;
  mEfiSfsInterface = NULL;
  Volume           = NULL;
  VolumeRootDir    = NULL;
  Position         = 0;

  DeviceHandle = (EFI_HANDLE)0xDEADDEADULL;

  //
  // Construct File Name
  //
  FileName = AllocateZeroPool (BufferSize);
  if (FileName == NULL) {
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (FileName, BufferSize);

  if ((mFuzzSize - mFuzzOffset) < BufferSize) {
    FreeAll (FileName, Volume, VolumeRootDir);
    return 0;
  }

  CopyMem (FileName, mFuzzPointer, BufferSize - 2);
  mFuzzPointer += BufferSize - 2;
  mFuzzOffset  += BufferSize - 2;

  //
  // Construct BlockIo and DiskIo interfaces
  //
  DiskIo = AllocateZeroPool (sizeof (EFI_DISK_IO_PROTOCOL));
  if (DiskIo == NULL) {
    FreeAll (FileName, Volume, VolumeRootDir);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (DiskIo, sizeof (EFI_DISK_IO_PROTOCOL));

  DiskIo->ReadDisk = FuzzReadDisk;

  BlockIo = AllocateZeroPool (sizeof (EFI_BLOCK_IO_PROTOCOL));
  if (BlockIo == NULL) {
    FreePool (DiskIo);
    FreeAll (FileName, Volume, VolumeRootDir);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (BlockIo, sizeof (EFI_BLOCK_IO_PROTOCOL));

  BlockIo->Media = AllocateZeroPool (sizeof (EFI_BLOCK_IO_MEDIA));
  if (BlockIo->Media == NULL) {
    FreePool (DiskIo);
    FreePool (BlockIo);
    FreeAll (FileName, Volume, VolumeRootDir);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (BlockIo->Media, sizeof (EFI_BLOCK_IO_MEDIA));

  BlockIo->FlushBlocks = FuzzBlockIoFlushBlocks;

  //
  // Allocate volume
  //
  Status = FatAllocateVolume (DeviceHandle, DiskIo, NULL, BlockIo);

  if (EFI_ERROR (Status)) {
    FreePool (BlockIo->Media);
    FreePool (BlockIo);
    FreePool (DiskIo);
    FreeAll (FileName, Volume, VolumeRootDir);
    return 0;
  }

  //
  // Open volume root dir
  //
  Status = FatOpenVolume (mEfiSfsInterface, &VolumeRootDir);

  if (EFI_ERROR (Status)) {
    FreeAll (FileName, Volume, VolumeRootDir);
    return 0;
  }

  Volume = VOLUME_FROM_VOL_INTERFACE (mEfiSfsInterface);

  //
  // Open File
  //
  Status = FatOpen (VolumeRootDir, &NewHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (Status == EFI_SUCCESS) {
    Buffer     = NULL;
    BufferSize = 0;
    Status     = FatRead (NewHandle, &BufferSize, Buffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Buffer = AllocateZeroPool (BufferSize);
      if (Buffer == NULL) {
        FatClose (NewHandle);
        FreeAll (FileName, Volume, VolumeRootDir);
        return 0;
      }

      ASAN_CHECK_MEMORY_REGION (Buffer, BufferSize);

      FatRead (NewHandle, &BufferSize, Buffer);

      FatWrite (NewHandle, &BufferSize, Buffer);

      FatFlush (NewHandle);

      FreePool (Buffer);
    }

    //
    // Set/Get file info
    //
    Len    = 0;
    Info   = NULL;
    Status = FatGetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      if (Info == NULL) {
        FatClose (NewHandle);
        FreeAll (FileName, Volume, VolumeRootDir);
        return 0;
      }

      Status = FatGetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
      if (!EFI_ERROR (Status)) {
        //
        // Try to set this info back
        //
        FatSetInfo (NewHandle, &gEfiFileInfoGuid, Len, Info);
        FreePool (Info);
      }
    }

    Len    = 0;
    Info   = NULL;
    Status = FatGetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      if (Info == NULL) {
        FatClose (NewHandle);
        FreeAll (FileName, Volume, VolumeRootDir);
        return 0;
      }

      Status = FatGetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
      if (!EFI_ERROR (Status)) {
        //
        // Try to set this info back
        //
        FatSetInfo (NewHandle, &gEfiFileSystemInfoGuid, Len, Info);
        FreePool (Info);
      }
    }

    Len    = 0;
    Info   = NULL;
    Status = FatGetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      if (Info == NULL) {
        FatClose (NewHandle);
        FreeAll (FileName, Volume, VolumeRootDir);
        return 0;
      }

      Status = FatGetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
      if (!EFI_ERROR (Status)) {
        //
        // Try to set this info back
        //
        FatSetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, Len, Info);
        FreePool (Info);
      }
    }

    //
    // Test position functions
    //
    FatGetPosition (NewHandle, &Position);
    FatSetPosition (NewHandle, Position);

    //
    // Trying to reach the end of file and read/write it
    //
    Position = (UINT64)-1;
    Status   = FatSetPosition (NewHandle, Position);
    if (!EFI_ERROR (Status)) {
      Buffer     = NULL;
      BufferSize = 0;
      Status     = FatRead (NewHandle, &BufferSize, Buffer);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Buffer = AllocateZeroPool (BufferSize);
        if (Buffer == NULL) {
          FatClose (NewHandle);
          FreeAll (FileName, Volume, VolumeRootDir);
          return 0;
        }

        ASAN_CHECK_MEMORY_REGION (Buffer, BufferSize);

        FatRead (NewHandle, &BufferSize, Buffer);

        FatWrite (NewHandle, &BufferSize, Buffer);

        FatFlush (NewHandle);

        FreePool (Buffer);
      }
    }

    //
    // Trying to reach out of bound of FileSize
    //
    FileInstance = IFILE_FROM_FHAND (NewHandle);
    FileSize     = FileInstance->OFile->FileSize;
    if (FileSize < (UINT64)-1 - 1) {
      Position = FileSize + 1;
      Status   = FatSetPosition (NewHandle, Position);
      if (!EFI_ERROR (Status)) {
        Buffer     = NULL;
        BufferSize = 0;
        Status     = FatRead (NewHandle, &BufferSize, Buffer);

        ASSERT (Status == EFI_DEVICE_ERROR);
      }
    }

    FatDelete (NewHandle);
  }

  FreeAll (FileName, Volume, VolumeRootDir);

  return 0;
}

INT32
LLVMFuzzerTestOneInput (
  CONST UINT8  *FuzzData,
  UINTN        FuzzSize
  )
{
  VOID  *NewData;

 #ifdef MEMORY_MUTATIONS
  UINT32  ConfigSize;
 #endif

  if (FuzzSize == 0) {
    return 0;
  }

  // Set up unicode collation protocol
  UserUnicodeCollationInstallProtocol (&mUnicodeCollationInterface);

  //
  // Override InstallMultipleProtocolInterfaces with custom wrapper
  //
  gBS->InstallMultipleProtocolInterfaces = WrapInstallMultipleProtocolInterfaces;

 #ifdef MEMORY_MUTATIONS
  ConfigSize = 0;
  ConfigureMemoryAllocations (FuzzData, FuzzSize, &ConfigSize);
 #endif
  SetPoolAllocationSizeLimit (BASE_1GB | BASE_2GB);

  NewData = AllocatePool (FuzzSize);
  if (NewData != NULL) {
    CopyMem (NewData, FuzzData, FuzzSize);
    TestFatDxe (NewData, FuzzSize);
    FreePool (NewData);
  }

  DEBUG ((
    DEBUG_POOL | DEBUG_PAGE,
    "UMEM: Allocated %u pools %u pages\n",
    (UINT32)mPoolAllocations,
    (UINT32)mPageAllocations
    ));

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

/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/
#include <Ext4Dxe.h>

#include <UserFile.h>
#include <UserGlobalVar.h>
#include <UserMemory.h>
#include <string.h>

#define MAP_TABLE_SIZE  0x100
#define CHAR_FAT_VALID  0x01

#define TO_UPPER(a)  (CHAR16) ((a) <= 0xFF ? mEngUpperMap[a] : (a))

UINT8  _gPcd_FixedAtBuild_PcdUefiVariableDefaultLang[4]         = { 101, 110, 103, 0 };
UINT8  _gPcd_FixedAtBuild_PcdUefiVariableDefaultPlatformLang[6] = { 101, 110, 45, 85, 83, 0 };

UINTN        mFuzzOffset;
UINTN        mFuzzSize;
CONST UINT8  *mFuzzPointer;

CHAR8  mEngUpperMap[MAP_TABLE_SIZE];
CHAR8  mEngLowerMap[MAP_TABLE_SIZE];
CHAR8  mEngInfoMap[MAP_TABLE_SIZE];

CHAR8  mOtherChars[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  '\\',
  '.',
  '_',
  '^',
  '$',
  '~',
  '!',
  '#',
  '%',
  '&',
  '-',
  '{',
  '}',
  '(',
  ')',
  '@',
  '`',
  '\'',
  '\0'
};

VOID
UnicodeCollationInitializeMappingTables (
  VOID
  )
{
  UINTN  Index;
  UINTN  Index2;

  //
  // Initialize mapping tables for the supported languages
  //
  for (Index = 0; Index < MAP_TABLE_SIZE; Index++) {
    mEngUpperMap[Index] = (CHAR8)Index;
    mEngLowerMap[Index] = (CHAR8)Index;
    mEngInfoMap[Index]  = 0;

    if (((Index >= 'a') && (Index <= 'z')) || ((Index >= 0xe0) && (Index <= 0xf6)) || ((Index >= 0xf8) && (Index <= 0xfe))) {
      Index2               = Index - 0x20;
      mEngUpperMap[Index]  = (CHAR8)Index2;
      mEngLowerMap[Index2] = (CHAR8)Index;

      mEngInfoMap[Index]  |= CHAR_FAT_VALID;
      mEngInfoMap[Index2] |= CHAR_FAT_VALID;
    }
  }

  for (Index = 0; mOtherChars[Index] != 0; Index++) {
    Index2               = mOtherChars[Index];
    mEngInfoMap[Index2] |= CHAR_FAT_VALID;
  }
}

/**
  Performs a case-insensitive comparison of two Null-terminated strings.

  @param  This Protocol instance pointer.
  @param  Str1 A pointer to a Null-terminated string.
  @param  Str2 A pointer to a Null-terminated string.

  @retval 0   Str1 is equivalent to Str2
  @retval > 0 Str1 is lexically greater than Str2
  @retval < 0 Str1 is lexically less than Str2

**/
INTN
StriColl (
  IN CHAR16  *Str1,
  IN CHAR16  *Str2
  )
{
  while (*Str1 != 0) {
    if (TO_UPPER (*Str1) != TO_UPPER (*Str2)) {
      break;
    }

    Str1 += 1;
    Str2 += 1;
  }

  return TO_UPPER (*Str1) - TO_UPPER (*Str2);
}

/**
   Initialises Unicode collation, which is needed for case-insensitive string comparisons
   within the driver (a good example of an application of this is filename comparison).

   @param[in]      DriverHandle    Handle to the driver image.

   @retval EFI_SUCCESS   Unicode collation was successfully initialised.
   @retval !EFI_SUCCESS  Failure.
**/
EFI_STATUS
Ext4InitialiseUnicodeCollation (
  EFI_HANDLE  DriverHandle
  )
{
  UnicodeCollationInitializeMappingTables ();
  return EFI_SUCCESS;
}

/**
   Does a case-insensitive string comparison. Refer to EFI_UNICODE_COLLATION_PROTOCOL's StriColl
   for more details.

   @param[in]      Str1   Pointer to a null terminated string.
   @param[in]      Str2   Pointer to a null terminated string.

   @retval 0   Str1 is equivalent to Str2.
   @retval >0  Str1 is lexically greater than Str2.
   @retval <0  Str1 is lexically less than Str2.
**/
INTN
Ext4StrCmpInsensitive (
  IN CHAR16  *Str1,
  IN CHAR16  *Str2
  )
{
  return StriColl (Str1, Str2);
}

EFI_STATUS
EFIAPI
EfiLibInstallAllDriverProtocols2 (
  IN CONST EFI_HANDLE ImageHandle,
  IN CONST EFI_SYSTEM_TABLE *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE DriverBindingHandle,
  IN CONST EFI_COMPONENT_NAME_PROTOCOL *ComponentName, OPTIONAL
  IN CONST EFI_COMPONENT_NAME2_PROTOCOL       *ComponentName2, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION2_PROTOCOL *DriverConfiguration2, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   *DriverDiagnostics2    OPTIONAL
  )
{
  abort ();
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
EfiLibUninstallAllDriverProtocols2 (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN CONST EFI_COMPONENT_NAME_PROTOCOL *ComponentName, OPTIONAL
  IN CONST EFI_COMPONENT_NAME2_PROTOCOL       *ComponentName2, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration, OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION2_PROTOCOL *DriverConfiguration2, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   *DriverDiagnostics2    OPTIONAL
  )
{
  abort ();
  return EFI_NOT_FOUND;
}

VOID
FreeAll (
  IN CHAR16          *FileName,
  IN EXT4_PARTITION  *Part
  )
{
  FreePool (FileName);

  if (Part != NULL) {
    if (Part->DiskIo != NULL) {
      FreePool (Part->DiskIo);
    }

    if (Part->BlockIo != NULL) {
      if (Part->BlockIo->Media != NULL) {
        FreePool (Part->BlockIo->Media);
      }

      FreePool (Part->BlockIo);
    }

    if (Part->Root != NULL) {
      Ext4UnmountAndFreePartition (Part);
    } else if (Part != NULL) {
      FreePool (Part);
    }
  }
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
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (Buffer, mFuzzPointer, BufferSize);
  mFuzzPointer += BufferSize;
  mFuzzOffset  += BufferSize;

  return EFI_SUCCESS;
}

INT32
LLVMFuzzerTestOneInput (
  CONST UINT8  *FuzzData,
  UINTN        FuzzSize
  )
{
  EFI_STATUS         Status;
  EXT4_PARTITION     *Part;
  EXT4_FILE          *File;
  EFI_FILE_PROTOCOL  *This;
  UINTN              BufferSize;
  VOID               *Buffer;
  EFI_FILE_PROTOCOL  *NewHandle;
  CHAR16             *FileName;
  VOID               *Info;
  UINTN              Len;
  UINT64             Position;
  UINT64             FileSize;

  mFuzzOffset  = 0;
  mFuzzPointer = FuzzData;
  mFuzzSize    = FuzzSize;

  Part       = NULL;
  BufferSize = 100;

  //
  // Construct file name
  //
  FileName = AllocateZeroPool (BufferSize);
  if (FileName == NULL) {
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (FileName, BufferSize);

  if ((mFuzzSize - mFuzzOffset) < BufferSize) {
    FreeAll (FileName, Part);
    return 0;
  }

  CopyMem (FileName, mFuzzPointer, BufferSize - 2);
  mFuzzPointer += BufferSize - 2;
  mFuzzOffset  += BufferSize - 2;

  //
  // Construct File System
  //
  Part = AllocateZeroPool (sizeof (EXT4_PARTITION));
  if (Part == NULL) {
    FreeAll (FileName, Part);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Part, sizeof (EXT4_PARTITION));

  InitializeListHead (&Part->OpenFiles);

  Part->DiskIo = AllocateZeroPool (sizeof (EFI_DISK_IO_PROTOCOL));
  if (Part->DiskIo == NULL) {
    FreeAll (FileName, Part);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Part->DiskIo, sizeof (EFI_DISK_IO_PROTOCOL));

  Part->DiskIo->ReadDisk = FuzzReadDisk;

  Part->BlockIo = AllocateZeroPool (sizeof (EFI_BLOCK_IO_PROTOCOL));
  if (Part->BlockIo == NULL) {
    FreeAll (FileName, Part);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Part->BlockIo, sizeof (EFI_BLOCK_IO_PROTOCOL));

  Part->BlockIo->Media = AllocateZeroPool (sizeof (EFI_BLOCK_IO_MEDIA));
  if (Part->BlockIo->Media == NULL) {
    FreeAll (FileName, Part);
    return 0;
  }

  ASAN_CHECK_MEMORY_REGION (Part->BlockIo->Media, sizeof (EFI_BLOCK_IO_MEDIA));

  Status = Ext4OpenSuperblock (Part);
  if (EFI_ERROR (Status)) {
    FreeAll (FileName, Part);
    return 0;
  }

  Part->Interface.Revision   = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Part->Interface.OpenVolume = Ext4OpenVolume;

  This = (EFI_FILE_PROTOCOL *)Part->Root;

  //
  // Test Ext4Dxe driver
  //
  Status = Ext4Open (This, &NewHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (Status == EFI_SUCCESS) {
    Buffer     = NULL;
    BufferSize = 0;
    Status     = Ext4ReadFile (NewHandle, &BufferSize, Buffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Buffer = AllocateZeroPool (BufferSize);
      if (Buffer == NULL) {
        FreeAll (FileName, Part);
        return 0;
      }

      ASAN_CHECK_MEMORY_REGION (Buffer, BufferSize);

      Ext4ReadFile (NewHandle, &BufferSize, Buffer);

      Ext4WriteFile (NewHandle, &BufferSize, Buffer);

      FreePool (Buffer);
    }

    Len    = 0;
    Info   = NULL;
    Status = Ext4GetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      Ext4GetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
      FreePool (Info);
    }

    Len    = 0;
    Status = Ext4GetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      Ext4GetInfo (NewHandle, &gEfiFileSystemInfoGuid, &Len, Info);
      FreePool (Info);
    }

    Len    = 0;
    Status = Ext4GetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (Len);
      Ext4GetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, &Len, Info);
      FreePool (Info);
    }

    Ext4SetInfo (NewHandle, &gEfiFileSystemVolumeLabelInfoIdGuid, Len, Info);

    //
    // Test position functions
    //
    Ext4GetPosition (NewHandle, &Position);
    Ext4SetPosition (NewHandle, Position);

    //
    // Trying to reach the end of file and read/write it
    //
    Position = (UINT64)-1;
    Status   = Ext4SetPosition (NewHandle, Position);
    if (!EFI_ERROR (Status)) {
      Buffer     = NULL;
      BufferSize = 0;
      Status     = Ext4ReadFile (NewHandle, &BufferSize, Buffer);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Buffer = AllocateZeroPool (BufferSize);
        if (Buffer == NULL) {
          FreeAll (FileName, Part);
          return 0;
        }

        ASAN_CHECK_MEMORY_REGION (Buffer, BufferSize);

        Ext4ReadFile (NewHandle, &BufferSize, Buffer);

        Ext4WriteFile (NewHandle, &BufferSize, Buffer);

        FreePool (Buffer);
      }
    }

    //
    // Trying to reach out of bound of FileSize
    //
    File     = EXT4_FILE_FROM_THIS (NewHandle);
    FileSize = EXT4_INODE_SIZE (File->Inode);
    if (FileSize < (UINT64)-1 - 1) {
      Position = FileSize + 1;
      Ext4SetPosition (NewHandle, Position);
    }

    Ext4Delete (NewHandle);
  }

  FreeAll (FileName, Part);

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

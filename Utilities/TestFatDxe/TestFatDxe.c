/** @file
  Copyright (c) 2023, Savva Mitrofanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Fat.h>

#include <UserFile.h>
#include <UserGlobalVar.h>
#include <UserMemory.h>
#include <string.h>

extern EFI_UNICODE_COLLATION_PROTOCOL  *mUnicodeCollationInterface;

STATIC EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *mEfiSfsInterface;

#define MAP_TABLE_SIZE  0x100
#define CHAR_FAT_VALID  0x01
#define TO_UPPER(a)  (CHAR16) ((a) <= 0xFF ? mEngUpperMap[a] : (a))
#define TO_LOWER(a)  (CHAR16) ((a) <= 0xFF ? mEngLowerMap[a] : (a))

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

INTN
StriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN CHAR16                          *Str1,
  IN CHAR16                          *Str2
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

VOID
EFIAPI
StrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN OUT CHAR16                      *Str
  )
{
  while (*Str != 0) {
    *Str = TO_LOWER (*Str);
    Str += 1;
  }
}

VOID
EFIAPI
StrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN OUT CHAR16                      *Str
  )
{
  while (*Str != 0) {
    *Str = TO_UPPER (*Str);
    Str += 1;
  }
}

BOOLEAN
EFIAPI
MetaiMatch (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN CHAR16                          *String,
  IN CHAR16                          *Pattern
  )
{
  CHAR16  CharC;
  CHAR16  CharP;
  CHAR16  Index3;

  for ( ; ;) {
    CharP    = *Pattern;
    Pattern += 1;

    switch (CharP) {
      case 0:
        //
        // End of pattern.  If end of string, TRUE match
        //
        if (*String != 0) {
          return FALSE;
        } else {
          return TRUE;
        }

      case '*':
        //
        // Match zero or more chars
        //
        while (*String != 0) {
          if (MetaiMatch (This, String, Pattern)) {
            return TRUE;
          }

          String += 1;
        }

        return MetaiMatch (This, String, Pattern);

      case '?':
        //
        // Match any one char
        //
        if (*String == 0) {
          return FALSE;
        }

        String += 1;
        break;

      case '[':
        //
        // Match char set
        //
        CharC = *String;
        if (CharC == 0) {
          //
          // syntax problem
          //
          return FALSE;
        }

        Index3 = 0;
        CharP  = *Pattern++;
        while (CharP != 0) {
          if (CharP == ']') {
            return FALSE;
          }

          if (CharP == '-') {
            //
            // if range of chars, get high range
            //
            CharP = *Pattern;
            if ((CharP == 0) || (CharP == ']')) {
              //
              // syntax problem
              //
              return FALSE;
            }

            if ((TO_UPPER (CharC) >= TO_UPPER (Index3)) && (TO_UPPER (CharC) <= TO_UPPER (CharP))) {
              //
              // if in range, it's a match
              //
              break;
            }
          }

          Index3 = CharP;
          if (TO_UPPER (CharC) == TO_UPPER (CharP)) {
            //
            // if char matches
            //
            break;
          }

          CharP = *Pattern++;
        }

        //
        // skip to end of match char set
        //
        while ((CharP != 0) && (CharP != ']')) {
          CharP    = *Pattern;
          Pattern += 1;
        }

        String += 1;
        break;

      default:
        CharC = *String;
        if (TO_UPPER (CharC) != TO_UPPER (CharP)) {
          return FALSE;
        }

        String += 1;
        break;
    }
  }
}

VOID
EFIAPI
FatToStr (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN UINTN                           FatSize,
  IN CHAR8                           *Fat,
  OUT CHAR16                         *String
  )
{
  //
  // No DBCS issues, just expand and add null terminate to end of string
  //
  while ((*Fat != 0) && (FatSize != 0)) {
    *String  = *Fat;
    String  += 1;
    Fat     += 1;
    FatSize -= 1;
  }

  *String = 0;
}

BOOLEAN
EFIAPI
StrToFat (
  IN EFI_UNICODE_COLLATION_PROTOCOL  *This,
  IN CHAR16                          *String,
  IN UINTN                           FatSize,
  OUT CHAR8                          *Fat
  )
{
  BOOLEAN  SpecialCharExist;

  SpecialCharExist = FALSE;
  while ((*String != 0) && (FatSize != 0)) {
    //
    // Skip '.' or ' ' when making a fat name
    //
    if ((*String != '.') && (*String != ' ')) {
      //
      // If this is a valid fat char, move it.
      // Otherwise, move a '_' and flag the fact that the name needs a long file name.
      //
      if ((*String < MAP_TABLE_SIZE) && ((mEngInfoMap[*String] & CHAR_FAT_VALID) != 0)) {
        *Fat = mEngUpperMap[*String];
      } else {
        *Fat             = '_';
        SpecialCharExist = TRUE;
      }

      Fat     += 1;
      FatSize -= 1;
    }

    String += 1;
  }

  //
  // Do not terminate that fat string
  //
  return SpecialCharExist;
}

STATIC CHAR8  UnicodeLanguages[6] = "en";

EFI_UNICODE_COLLATION_PROTOCOL  gInternalUnicode2Eng = {
  StriColl,
  MetaiMatch,
  StrLwr,
  StrUpr,
  FatToStr,
  StrToFat,
  UnicodeLanguages
};

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

#ifdef MEMORY_MUTATIONS
STATIC
VOID
ConfigureMemoryAllocations (
  IN  CONST UINT8  *Data,
  IN  UINTN        Size
  )
{
  UINT32  Off;

  mPoolAllocationIndex = 0;
  mPageAllocationIndex = 0;

  //
  // Limit single pool allocation size to 3GB
  //
  SetPoolAllocationSizeLimit (BASE_1GB | BASE_2GB);

  Off = sizeof (UINT64);
  if (Size >= Off) {
    CopyMem (&mPoolAllocationMask, &Data[Size - Off], sizeof (UINT64));
  } else {
    mPoolAllocationMask = MAX_UINT64;
  }

  Off += sizeof (UINT64);
  if (Size >= Off) {
    CopyMem (&mPageAllocationMask, &Data[Size - Off], sizeof (UINT64));
  } else {
    mPageAllocationMask = MAX_UINT64;
  }
}

#endif

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

  if (FuzzSize == 0) {
    return 0;
  }

  // Set up unicode collation protocol
  UnicodeCollationInitializeMappingTables ();
  mUnicodeCollationInterface = &gInternalUnicode2Eng;

  //
  // Override InstallMultipleProtocolInterfaces with custom wrapper
  //
  gBS->InstallMultipleProtocolInterfaces = WrapInstallMultipleProtocolInterfaces;

 #ifdef MEMORY_MUTATIONS
  ConfigureMemoryAllocations (FuzzData, FuzzSize);
 #else
  SetPoolAllocationSizeLimit (BASE_1GB | BASE_2GB);
 #endif

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

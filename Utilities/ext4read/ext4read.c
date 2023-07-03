/** @file
  Copyright (c) 2023, Savva Mitrofanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Ext4Dxe.h>

#include <string.h>
#include <UserFile.h>
#include <UserGlobalVar.h>
#include <UserMemory.h>
#include <UserUnicodeCollation.h>

STATIC FILE                             *mImageFp;
STATIC UINT64                           mImageSize;
STATIC EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *mEfiSfsInterface;

void
PrintHex (
  const void  *data,
  size_t      size
  )
{
  char    ascii[17];
  size_t  i, j;

  ascii[16] = '\0';
  for (i = 0; i < size; ++i) {
    printf ("%02X ", ((unsigned char *)data)[i]);
    if ((((unsigned char *)data)[i] >= ' ') && (((unsigned char *)data)[i] <= '~')) {
      ascii[i % 16] = ((unsigned char *)data)[i];
    } else {
      ascii[i % 16] = '.';
    }

    if (((i + 1) % 8 == 0) || (i + 1 == size)) {
      printf (" ");
      if ((i + 1) % 16 == 0) {
        printf ("|  %s \n", ascii);
      } else if (i + 1 == size) {
        ascii[(i + 1) % 16] = '\0';
        if ((i + 1) % 16 <= 8) {
          printf (" ");
        }

        for (j = (i + 1) % 16; j < 16; ++j) {
          printf ("   ");
        }

        printf ("|  %s \n", ascii);
      }
    }
  }
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
  OcUnicodeCollationInitializeMappingTables ();
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
  return EngStriColl (NULL, Str1, Str2);
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
UserReadDisk (
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

  if (Offset > mImageSize) {
    return EFI_INVALID_PARAMETER;
  }

  if ((mImageSize - Offset) < BufferSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (fseek (mImageFp, Offset, SEEK_SET) != 0) {
    return EFI_VOLUME_CORRUPTED;
  }

  if (fread (Buffer, BufferSize, 1, mImageFp) != 1) {
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;
}

int
ENTRY_POINT (
  int   argc,
  char  **argv
  )
{
  FILE                   *ImageFp;
  UINT64                 ImageSize;
  EFI_STATUS             Status;
  EFI_FILE_INFO          *Info;
  UINTN                  Len;
  CHAR16                 *FileName;
  EFI_FILE_PROTOCOL      *This;
  EFI_DISK_IO_PROTOCOL   *DiskIo;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_HANDLE             DeviceHandle;
  EXT4_PARTITION         *Part;
  UINTN                  BufferSize;
  VOID                   *Buffer;
  VOID                   *TmpBuffer;
  EFI_FILE_PROTOCOL      *NewHandle;

  SetPoolAllocationSizeLimit (BASE_1GB | BASE_2GB);

  DeviceHandle = (EFI_HANDLE)0xDEADBEAFULL;

  gBS->InstallMultipleProtocolInterfaces = WrapInstallMultipleProtocolInterfaces;

  if ((argc < 2) || (argc != 3)) {
    DEBUG ((DEBUG_ERROR, "Usage: ./ext4read ([image path] [file path])*\n"));
    return -1;
  }

  //
  // Open Ext4 user image
  //
  ImageFp = fopen (argv[1], "rb");

  if (ImageFp == NULL) {
    DEBUG ((DEBUG_ERROR, "Can't open image file\n"));
    return EXIT_FAILURE;
  }

  if (fseek (ImageFp, 0, SEEK_END) != 0) {
    DEBUG ((DEBUG_ERROR, "Can't set file position\n"));
    fclose (ImageFp);
    return EXIT_FAILURE;
  }

  ImageSize = ftell (ImageFp);
  if (ImageSize <= 0) {
    DEBUG ((DEBUG_ERROR, "Incorrect file size\n"));
    fclose (ImageFp);
    return EXIT_FAILURE;
  }

  if (fseek (ImageFp, 0, SEEK_SET) != 0) {
    DEBUG ((DEBUG_ERROR, "Can't rewind file position to the start\n"));
    fclose (ImageFp);
    return EXIT_FAILURE;
  }

  mImageFp   = ImageFp;
  mImageSize = ImageSize;

  //
  // Prepare Filename to read from partition
  //
  FileName = AllocateZeroPool (strlen (argv[2]) * sizeof (CHAR16) + 1);

  if (FileName == NULL) {
    DEBUG ((DEBUG_ERROR, "Can't allocate space for unicode filename\n"));
    return EXIT_FAILURE;
  }

  Status = AsciiStrToUnicodeStrS (argv[2], FileName, strlen (argv[2]) + 1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Can't convert given filename into UnicodeStr\n"));
    fclose (ImageFp);
    FreePool (FileName);
    return EXIT_FAILURE;
  }

  //
  // Construct BlockIo and DiskIo interfaces
  //
  DiskIo = AllocateZeroPool (sizeof (EFI_DISK_IO_PROTOCOL));
  if (DiskIo == NULL) {
    fclose (ImageFp);
    FreePool (FileName);
    return EXIT_FAILURE;
  }

  DiskIo->ReadDisk = UserReadDisk;

  BlockIo = AllocateZeroPool (sizeof (EFI_BLOCK_IO_PROTOCOL));
  if (BlockIo == NULL) {
    fclose (ImageFp);
    FreePool (FileName);
    FreePool (DiskIo);
    return EXIT_FAILURE;
  }

  BlockIo->Media = AllocateZeroPool (sizeof (EFI_BLOCK_IO_MEDIA));
  if (BlockIo->Media == NULL) {
    fclose (ImageFp);
    FreePool (FileName);
    FreePool (DiskIo);
    FreePool (BlockIo);
    return EXIT_FAILURE;
  }

  //
  // Check Ext4 SuperBlock magic like it done
  // in Ext4IsBindingSupported routine
  //
  if (!Ext4SuperblockCheckMagic (DiskIo, BlockIo)) {
    DEBUG ((DEBUG_WARN, "[ext4] Superblock contains bad magic \n"));
    return EXIT_FAILURE;
  }

  //
  // Open partition
  //
  Status = Ext4OpenPartition (DeviceHandle, DiskIo, NULL, BlockIo);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[ext4] Error mounting: %r\n", Status));
    fclose (ImageFp);
    FreePool (FileName);
    FreePool (BlockIo->Media);
    FreePool (BlockIo);
    FreePool (DiskIo);
    return EXIT_FAILURE;
  }

  Part = (EXT4_PARTITION *)mEfiSfsInterface;

  This = (EFI_FILE_PROTOCOL *)Part->Root;

  Status = Ext4Open (This, &NewHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[ext4] Couldn't open file %s \n", FileName));
    return EXIT_FAILURE;
  }

  //
  // Get FileInfo
  //
  Len    = 0;
  Info   = NULL;
  Status = Ext4GetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Info = AllocateZeroPool (Len);
    if (Info == NULL) {
      fclose (ImageFp);
      FreeAll (FileName, Part);
      return EXIT_FAILURE;
    }

    Status = Ext4GetInfo (NewHandle, &gEfiFileInfoGuid, &Len, Info);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Couldn't get file information \n"));
      FreePool (Info);
      fclose (ImageFp);
      FreeAll (FileName, Part);
      return EXIT_FAILURE;
    }
  }

  Buffer     = AllocateZeroPool (Info->FileSize);
  BufferSize = Info->FileSize;
  if (Buffer == NULL) {
    FreePool (Info);
    fclose (ImageFp);
    FreeAll (FileName, Part);
    return EXIT_FAILURE;
  }

  //
  // Read file from image
  //
  Status = Ext4ReadFile (NewHandle, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    TmpBuffer = ReallocatePool (Info->FileSize, BufferSize, Buffer);
    if (TmpBuffer == NULL) {
      FreePool (Buffer);
      FreePool (Info);
      fclose (ImageFp);
      FreeAll (FileName, Part);
      return EXIT_FAILURE;
    }

    Buffer = TmpBuffer;

    Status = Ext4ReadFile (NewHandle, &BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Couldn't read file with Status: %r \n", Status));
      FreePool (Info);
      fclose (ImageFp);
      FreeAll (FileName, Part);
      return EXIT_FAILURE;
    }
  }

  FreePool (Info);

  //
  // Print file contents
  //
  if (Buffer != NULL) {
    PrintHex (Buffer, BufferSize);
  }

  FreePool (Buffer);
  fclose (ImageFp);
  FreeAll (FileName, Part);
  return EXIT_SUCCESS;
}

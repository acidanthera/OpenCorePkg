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
#include <Protocol/OcBootstrap.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>

// Define EPOCH (1970-JANUARY-01) in the Julian Date representation
#define EPOCH_JULIAN_DATE                               2440588

// Seconds per unit
#define SEC_PER_MIN                                     ((UINTN)    60)
#define SEC_PER_HOUR                                    ((UINTN)  3600)
#define SEC_PER_DAY                                     ((UINTN) 86400)
#define SEC_PER_MONTH                                   ((UINTN)  2,592,000)
#define SEC_PER_YEAR                                    ((UINTN) 31,536,000)

STATIC
UINTN
EfiGetEpochDays (
  IN  EFI_TIME  *Time
  )
{
  UINTN a;
  UINTN y;
  UINTN m;
  UINTN JulianDate;  // Absolute Julian Date representation of the supplied Time
  UINTN EpochDays;   // Number of days elapsed since EPOCH_JULIAN_DAY

  a = (14 - Time->Month) / 12 ;
  y = Time->Year + 4800 - a;
  m = Time->Month + (12*a) - 3;

  JulianDate = Time->Day + ((153*m + 2)/5) + (365*y) + (y/4) - (y/100) + (y/400) - 32045;

  ASSERT (JulianDate >= EPOCH_JULIAN_DATE);
  EpochDays = JulianDate - EPOCH_JULIAN_DATE;

  return EpochDays;
}

STATIC
UINT32
EfiTimeToEpoch (
  IN  EFI_TIME  *Time
  )
{
  UINT32 EpochDays;   // Number of days elapsed since EPOCH_JULIAN_DAY
  UINT32 EpochSeconds;

  EpochDays = (UINT32)EfiGetEpochDays (Time);

  EpochSeconds = (EpochDays * SEC_PER_DAY) + ((UINTN)Time->Hour * SEC_PER_HOUR) + (Time->Minute * SEC_PER_MIN) + Time->Second;

  return EpochSeconds;
}

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
  UINTN       RequestedSize;

  while (Size > 0) {
    Status = File->SetPosition (File, Position);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // We are required to read in 1 MB portions, because otherwise some
    // systems namely MacBook7,1 will not read file data from APFS volumes
    // but will pretend they did. Repeoduced with BootKernelExtensions.kc.
    //
    ReadSize = RequestedSize = MIN (Size, BASE_1MB);
    Status = File->Read (File, &ReadSize, Buffer);
    if (EFI_ERROR (Status)) {
      File->SetPosition (File, 0);
      return Status;
    }

    if (ReadSize != RequestedSize) {
      File->SetPosition (File, 0);
      return EFI_BAD_BUFFER_SIZE;
    }

    Position += (UINT32) ReadSize;
    Buffer   += ReadSize;
    Size     -= (UINT32) ReadSize;
  }

  File->SetPosition (File, 0);

  return EFI_SUCCESS;
}

EFI_STATUS
GetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  EFI_STATUS     Status;
  UINT64         Position;
  EFI_FILE_INFO  *FileInfo;

  Status = File->SetPosition (File, 0xFFFFFFFFFFFFFFFFULL);
  if (EFI_ERROR (Status)) {
    //
    // Some drivers, like EfiFs, return EFI_UNSUPPORTED when trying to seek
    // past the file size. Use slow method via attributes for them.
    //
    FileInfo = GetFileInfo (File, &gEfiFileInfoGuid, sizeof (*FileInfo), NULL);
    if (FileInfo != NULL) {
      if ((UINT32) FileInfo->FileSize == FileInfo->FileSize) {
        *Size = (UINT32) FileInfo->FileSize;
        Status = EFI_SUCCESS;
      }
      FreePool (FileInfo);
    }
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

BOOLEAN
IsWritableFileSystem (
  IN EFI_FILE_PROTOCOL  *Fs
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *File;

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
    return FALSE;
  }
  
  //
  // Delete the temporary file and report the found file system.
  //
  Fs->Delete (File);
  return TRUE;
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

    if (IsWritableFileSystem (Fs)) {
      FreePool (HandleBuffer);
      *WritableFs = Fs;
      return EFI_SUCCESS;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "OCFS: FindWritableFileSystem Fs->Open[%u] failed\n",
      (UINT32) Index
      ));
  }

  FreePool (HandleBuffer);
  return EFI_NOT_FOUND;
}

EFI_STATUS
FindWritableOcFileSystem (
  OUT EFI_FILE_PROTOCOL  **FileSystem
  )
{
  EFI_STATUS             Status;
  OC_BOOTSTRAP_PROTOCOL  *Bootstrap;
  EFI_HANDLE             PreferedHandle;

  PreferedHandle = NULL;

  Status = gBS->LocateProtocol (
    &gOcBootstrapProtocolGuid,
    NULL,
    (VOID **) &Bootstrap
    );
  if (!EFI_ERROR (Status) && Bootstrap->Revision == OC_BOOTSTRAP_PROTOCOL_REVISION) {
    PreferedHandle = Bootstrap->GetLoadHandle (Bootstrap);
  }

  if (PreferedHandle != NULL) {
    *FileSystem = LocateRootVolume (PreferedHandle, NULL);
  } else {
    *FileSystem = NULL;
  }

  DEBUG ((DEBUG_INFO, "OCFS: Preferred handle is %p found fs %p\n", PreferedHandle, *FileSystem));

  if (*FileSystem == NULL) {
    return FindWritableFileSystem (FileSystem);
  }

  return EFI_SUCCESS;
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

  ASSERT (FileName != NULL);
  ASSERT (Buffer != NULL);

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

VOID
DirectorySeachContextInit (
  IN OUT DIRECTORY_SEARCH_CONTEXT *Context
  )
{
  ASSERT (Context != NULL);

  ZeroMem (Context, sizeof (*Context));
}

EFI_STATUS
GetNewestFileFromDirectory (
  IN OUT DIRECTORY_SEARCH_CONTEXT *Context,
  IN     EFI_FILE_PROTOCOL        *Directory,
  IN     CHAR16                   *FileNameStartsWith OPTIONAL,
     OUT EFI_FILE_INFO            **FileInfo
  )
{
  EFI_STATUS        Status;
  EFI_FILE_INFO     *FileInfoCurrent;
  EFI_FILE_INFO     *FileInfoLatest;
  UINTN             FileInfoSize;
  UINT32            EpochCurrent;
  UINTN             Index;

  UINTN             LatestIndex;
  UINT32            LatestEpoch;

  ASSERT (Context != NULL);
  ASSERT (Directory != NULL);
  ASSERT (FileInfo != NULL);

  LatestIndex = 0;
  LatestEpoch = 0;

  //
  // Ensure this is a directory.
  //
  FileInfoCurrent = GetFileInfo (Directory, &gEfiFileInfoGuid, 0, NULL);
  if (FileInfoCurrent == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (!(FileInfoCurrent->Attribute & EFI_FILE_DIRECTORY)) {
    FreePool (FileInfoCurrent);
    return EFI_INVALID_PARAMETER;
  }
  FreePool (FileInfoCurrent);

  //
  // Allocate two FILE_INFO structures.
  // The first is used for all entries, and the second is used only for the
  // latest, and eventually returned to the caller.
  //
  FileInfoCurrent = AllocatePool (SIZE_1KB);
  if (FileInfoCurrent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  FileInfoLatest = AllocateZeroPool (SIZE_1KB);
  if (FileInfoLatest == NULL) {
    FreePool (FileInfoCurrent);
    return EFI_OUT_OF_RESOURCES;
  }

  Directory->SetPosition (Directory, 0);
  Index = 0;

  do {
    //
    // Apple's HFS+ driver does not adhere to the spec and will return zero for
    // EFI_BUFFER_TOO_SMALL. EFI_FILE_INFO structures larger than 1KB are
    // unrealistic as the filename is the only variable.
    //
    FileInfoSize = SIZE_1KB - sizeof (CHAR16);
    Status = Directory->Read (Directory, &FileInfoSize, FileInfoCurrent);
    if (EFI_ERROR (Status)) {
      Directory->SetPosition (Directory, 0);
      FreePool (FileInfoCurrent);
      FreePool (FileInfoLatest);
      return Status;
    }

    if (FileInfoSize > 0) {
      //
      // Skip any files that do not start with the desired filename.
      //
      if (FileNameStartsWith != NULL) {
        if (StrnCmp (FileInfoCurrent->FileName, FileNameStartsWith, StrLen (FileNameStartsWith)) != 0) {
          continue;
        }
      }

      //
      // Get time of current entry.
      // We want to skip any entries that have been returned as "latest" in prior calls.
      //
      EpochCurrent = EfiTimeToEpoch (&FileInfoCurrent->ModificationTime);
      DEBUG ((DEBUG_VERBOSE, "OCFS: Current file %s with time %u\n", FileInfoCurrent->FileName, EpochCurrent));

      //
      // Skip any entries that are newer than our previously matched entry.
      //
      if (Context->PreviousTime > 0 && EpochCurrent > Context->PreviousTime) {
        DEBUG ((DEBUG_VERBOSE, "OCFS: Skipping file %s due to time %u > %u\n", FileInfoCurrent->FileName, EpochCurrent, Context->PreviousTime));
        continue;
      }

      //
      // Skip any entries that have the same time as our previously
      // matched entry and were found previously.
      //
      // ASSUMPTION: Entries are in the same order each time the directory is iterated.
      //
      if (EpochCurrent == Context->PreviousTime) {
        if (Index <= Context->PreviousIndex) {
          DEBUG ((DEBUG_VERBOSE, "OCFS: Skipping file %s with due to index %u <= %u\n", FileInfoCurrent->FileName, Index, Context->PreviousIndex));
          Index++;
          continue;
        }
      } else {
        //
        // Reset index counter if the time is different from the last.
        //
        Index = 0;
      }

      //
      // Store latest entry.
      //
      if (FileInfoLatest->FileName[0] == '\0' || EpochCurrent > EfiTimeToEpoch (&FileInfoLatest->ModificationTime)) {
        CopyMem (FileInfoLatest, FileInfoCurrent, FileInfoSize);
        LatestIndex = Index;
        LatestEpoch = EfiTimeToEpoch (&FileInfoLatest->ModificationTime);

        DEBUG ((DEBUG_VERBOSE, "OCFS: Stored newest file %s\n", FileInfoCurrent->FileName));
      }
    }
  } while (FileInfoSize > 0);

  Directory->SetPosition (Directory, 0);
  FreePool (FileInfoCurrent);

  if (FileInfoLatest->FileName[0] == '\0') {
    FreePool (FileInfoLatest);

    DEBUG ((DEBUG_VERBOSE, "OCFS: No matching files found\n"));
    return EFI_NOT_FOUND;
  }

  *FileInfo               = FileInfoLatest;
  Context->PreviousIndex  = LatestIndex;
  Context->PreviousTime   = LatestEpoch;

  return EFI_SUCCESS;
}

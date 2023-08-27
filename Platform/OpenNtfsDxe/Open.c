/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Helper.h"

extern INT64  mIndexCounter;

EFI_STATUS
EFIAPI
FileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  )
{
  EFI_STATUS     Status;
  EFI_NTFS_FILE  *File;
  EFI_FS         *FileSystem;
  EFI_NTFS_FILE  *NewFile;
  CHAR16         Path[MAX_PATH];
  CHAR16         CleanPath[MAX_PATH];
  CHAR16         *DirName;
  UINTN          Index;
  UINTN          Length;
  BOOLEAN        AbsolutePath;

  ASSERT (This != NULL);
  ASSERT (NewHandle != NULL);
  ASSERT (FileName != NULL);

  File         = (EFI_NTFS_FILE *)This;
  FileSystem   = File->FileSystem;
  AbsolutePath = (*FileName == L'\\');

  ZeroMem (Path, sizeof (Path));

  if (OpenMode != EFI_FILE_MODE_READ) {
    DEBUG ((DEBUG_INFO, "NTFS: File '%s' can only be opened in read-only mode\n", FileName));
    return EFI_WRITE_PROTECTED;
  }

  if ((*FileName == 0) || (CompareMem (FileName, L".", sizeof (L".")) == 0)) {
    ++File->RefCount;
    *NewHandle = This;
    return EFI_SUCCESS;
  }

  if (AbsolutePath) {
    Length = 0;
  } else {
    Length = StrnLenS (File->Path, MAX_PATH);
    if (Length == MAX_PATH) {
      DEBUG ((DEBUG_INFO, "NTFS: Path is too long.\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = StrCpyS (Path, MAX_PATH, File->Path);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "NTFS: Could not copy string.\n"));
      return Status;
    }

    if ((Length == 0) || (Path[Length - 1U] != '/')) {
      Path[Length++] = L'/';
    }
  }

  Status = StrCpyS (&Path[Length], MAX_PATH - Length, FileName);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not copy FileName `%s`.\n", FileName));
    return Status;
  }

  Index = StrnLenS (Path, MAX_PATH);
  while (Index > Length) {
    --Index;
    if (Path[Index] == L'\\') {
      Path[Index] = L'/';
    }
  }

  ZeroMem (CleanPath, sizeof (CleanPath));
  Status = RelativeToAbsolute (CleanPath, Path);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NewFile = AllocateZeroPool (sizeof (EFI_NTFS_FILE));
  if (NewFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewFile->FileSystem = FileSystem;
  CopyMem (&NewFile->EfiFile, &FileSystem->EfiFile, sizeof (EFI_FILE_PROTOCOL));

  NewFile->Path = AllocateZeroPool (StrnSizeS (CleanPath, MAX_PATH));
  if (NewFile->Path == NULL) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not instantiate path\n"));
    FreePool (NewFile);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (NewFile->Path, CleanPath, StrnSizeS (CleanPath, MAX_PATH));

  Length = StrnLenS (CleanPath, MAX_PATH);
  if (Length == MAX_PATH) {
    DEBUG ((DEBUG_INFO, "NTFS: CleanPath is too long.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Index = Length;
  while (Index > 0) {
    --Index;
    if (CleanPath[Index] == L'/') {
      CleanPath[Index] = 0;
      break;
    }
  }

  DirName           = (Index == 0) ? L"/" : CleanPath;
  NewFile->BaseName = (Index == Length) ? L"\0" : &NewFile->Path[Index + 1U];

  Status = NtfsDir (FileSystem, DirName, NewFile, INFO_HOOK);
  if (EFI_ERROR (Status)) {
    FreePool (NewFile->Path);
    FreePool (NewFile);
    return Status;
  }

  if (!NewFile->IsDir) {
    Status = NtfsOpen (NewFile);
    if (EFI_ERROR (Status)) {
      FreePool (NewFile->Path);
      FreePool (NewFile);
      return Status;
    }
  }

  NewFile->RefCount++;
  *NewHandle = &NewFile->EfiFile;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FileReadDir (
  IN EFI_NTFS_FILE  *File,
  OUT VOID          *Data,
  IN OUT UINTN      *Size
  )
{
  EFI_STATUS     Status;
  EFI_FILE_INFO  *Info;
  CHAR16         Path[MAX_PATH];
  EFI_NTFS_FILE  *TmpFile = NULL;
  UINTN          Length;

  ASSERT (File != NULL);
  ASSERT (Size != NULL);
  ASSERT ((Data != NULL) || ((Data == NULL) && (*Size == 0)));

  Info = (EFI_FILE_INFO *)Data;

  if (*Size < MINIMUM_INFO_LENGTH) {
    *Size = MINIMUM_INFO_LENGTH;
    return EFI_BUFFER_TOO_SMALL;
  }

  ZeroMem (Path, sizeof (Path));
  ZeroMem (Data, *Size);
  Info->Size = *Size;

  Status = StrCpyS (Path, MAX_PATH, File->Path);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not copy string.\n"));
    return Status;
  }

  Length = StrnLenS (Path, MAX_PATH);
  if (Length == MAX_PATH) {
    DEBUG ((DEBUG_INFO, "NTFS: Path is too long.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Length == 0) || (Path[Length - 1U] != L'/')) {
    Path[Length] = L'/';
    Length++;
  }

  mIndexCounter = (INT64)File->DirIndex;
  if (Length == 0) {
    Status = NtfsDir (File->FileSystem, L"/", Data, DIR_HOOK);
  } else {
    Status = NtfsDir (File->FileSystem, File->Path, Data, DIR_HOOK);
  }

  if (mIndexCounter >= 0) {
    //
    // No entries left
    //
    *Size = 0;
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Directory listing failed.\n"));
    return Status;
  }

  Info->FileSize     = 0;
  Info->PhysicalSize = 0;

  if ((Info->Attribute & EFI_FILE_DIRECTORY) == 0) {
    TmpFile = AllocateZeroPool (sizeof (EFI_NTFS_FILE));
    if (TmpFile == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    TmpFile->FileSystem = File->FileSystem;
    CopyMem (&TmpFile->EfiFile, &File->FileSystem->EfiFile, sizeof (EFI_FILE_PROTOCOL));

    if ((MAX_PATH - Length) < StrSize (Info->FileName)) {
      DEBUG ((DEBUG_INFO, "NTFS: FileName is too long.\n"));
      FreePool (TmpFile);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (&Path[Length], Info->FileName, StrSize (Info->FileName));
    TmpFile->Path = Path;

    Status = NtfsOpen (TmpFile);
    if (EFI_ERROR (Status)) {
      // DEBUG ((DEBUG_INFO, "NTFS: Unable to obtain the size of '%s'\n", Info->FileName));
    } else {
      Info->FileSize     = TmpFile->RootFile.DataAttributeSize;
      Info->PhysicalSize = TmpFile->RootFile.DataAttributeSize;
    }

    FreePool (TmpFile);
  }

  *Size = (UINTN)Info->Size;
  ++File->DirIndex;

  // DEBUG ((DEBUG_INFO, "NTFS:   Entry[%d]: '%s' %s\n", File->DirIndex - 1U, Info->FileName,
  //    (Info->Attribute & EFI_FILE_DIRECTORY) ? L"<DIR>" : L""));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
Read (
  IN  EFI_NTFS_FILE  *File,
  OUT VOID           *Data,
  IN  UINTN          *Size
  )
{
  EFI_STATUS  Status;
  UINT64      Remaining;
  NTFS_FILE   *BaseMftRecord;

  ASSERT (File != NULL);
  ASSERT (Size != NULL);
  ASSERT ((Data != NULL) || ((Data == NULL) && (*Size == 0)));

  Remaining = (File->RootFile.DataAttributeSize > File->Offset) ?
              File->RootFile.DataAttributeSize - File->Offset : 0;

  if (*Size > Remaining) {
    *Size = (UINTN)Remaining;
  }

  BaseMftRecord = &File->RootFile;

  Status = ReadAttr (&BaseMftRecord->Attr, Data, File->Offset, *Size);
  if (EFI_ERROR (Status)) {
    *Size = 0;
    FreeAttr (&BaseMftRecord->Attr);
    return Status;
  }

  File->Offset += *Size;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FileRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  EFI_NTFS_FILE  *File;

  ASSERT (This != NULL);
  ASSERT (BufferSize != NULL);
  ASSERT ((Buffer != NULL) || ((Buffer == NULL) && (*BufferSize == 0)));

  File = (EFI_NTFS_FILE *)This;

  if (File->IsDir) {
    return FileReadDir (File, Buffer, BufferSize);
  }

  return Read (File, Buffer, BufferSize);
}

EFI_STATUS
EFIAPI
FileWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  return EFI_WRITE_PROTECTED;
}

EFI_STATUS
EFIAPI
FileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  FileClose (This);

  return EFI_WARN_DELETE_FAILURE;
}

EFI_STATUS
EFIAPI
FileFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  return EFI_ACCESS_DENIED;
}

EFI_STATUS
EFIAPI
FileClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_NTFS_FILE  *File;

  ASSERT (This != NULL);

  File = (EFI_NTFS_FILE *)This;

  if (&File->RootFile == File->FileSystem->RootIndex) {
    return EFI_SUCCESS;
  }

  if (--File->RefCount == 0) {
    FreeFile (&File->RootFile);
    FreeFile (&File->MftFile);

    if (File->Path != NULL) {
      FreePool (File->Path);
    }

    FreePool (File);
  }

  return EFI_SUCCESS;
}

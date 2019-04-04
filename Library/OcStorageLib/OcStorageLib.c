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
#include <Library/OcStringLib.h>
#include <Library/OcStorageLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
OcStorageInitFromFs (
  OUT OC_STORAGE_CONTEXT               *Context,
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *Path,
  IN  VOID                             *StorageKey OPTIONAL
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *RootVolume;

  ZeroMem (Context, sizeof (*Context));

  if (StorageKey != NULL) {
    //
    // TODO: implement secure storage
    //
    return EFI_UNSUPPORTED;
  }

  Status = FileSystem->OpenVolume (FileSystem, &RootVolume);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = RootVolume->Open (
    RootVolume,
    &Context->StorageRoot,
    (CHAR16 *) Path,
    EFI_FILE_MODE_READ,
    0
    );

  RootVolume->Close (RootVolume);

  return Status;
}

VOID
OcStorageFree (
  IN OUT OC_STORAGE_CONTEXT            *Context
  )
{
  if (Context->StorageRoot != NULL) {
    Context->StorageRoot->Close (Context->StorageRoot);
    Context->StorageRoot = NULL;
  }
}

VOID *
OcStorageReadFileUnicode (
  IN  OC_STORAGE_CONTEXT               *Context,
  IN  CONST CHAR16                     *FilePath,
  OUT UINT32                           *FileSize OPTIONAL
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *File;
  UINT32             Size;
  UINT8              *FileBuffer;

  if (Context->StorageRoot == NULL) {
    //
    // TODO: expand support for other contexts.
    //
    return NULL;
  }

  Status = Context->StorageRoot->Open (
    Context->StorageRoot,
    &File,
    (CHAR16 *) FilePath,
    EFI_FILE_MODE_READ,
    0
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = ReadFileSize (File, &Size);
  if (EFI_ERROR (Status) || Size >= MAX_UINT32 - 1) {
    File->Close (File);
    return NULL;
  }

  FileBuffer = AllocatePool (Size + 2);
  if (FileBuffer == NULL) {
    File->Close (File);
    return NULL;
  }

  Status = ReadFileData (File, 0, Size, FileBuffer);
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

VOID *
OcStorageReadFileAscii (
  IN  OC_STORAGE_CONTEXT               *Context,
  IN  CONST CHAR8                      *FilePath,
  OUT UINT32                           *FileSize OPTIONAL
  )
{
  CHAR16  *UnicodePath;
  VOID    *FileBuffer;

  UnicodePath = AsciiStrCopyToUnicode (FilePath, 0);
  if (UnicodePath == NULL) {
    return NULL;
  }

  FileBuffer = OcStorageReadFileUnicode (
    Context,
    UnicodePath,
    FileSize
    );

  FreePool (UnicodePath);

  return FileBuffer;
}

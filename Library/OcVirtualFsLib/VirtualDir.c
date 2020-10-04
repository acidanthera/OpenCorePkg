/** @file
  Copyright (C) 2020, Goldfish64. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcVirtualFsLib.h>

#include <Guid/FileInfo.h>

#include "VirtualFsInternal.h"

STATIC
EFI_STATUS
EFIAPI
VirtualDirOpen (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                  *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes
  )
{
  EFI_STATUS         Status;
  VIRTUAL_DIR_DATA   *Data;

  if (This == NULL || NewHandle == NULL || FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_DIR_FROM_PROTOCOL (This);

  if (Data->UnderlyingProtocol != NULL) {
    Status = Data->UnderlyingProtocol->Open (
      Data->UnderlyingProtocol,
      NewHandle,
      FileName,
      OpenMode,
      Attributes
      );
    if (!EFI_ERROR (Status)) {
      return CreateRealFile (*NewHandle, NULL, TRUE, NewHandle);
    }
    return Status;
  }

  //
  // Cannot currently handle any virtualized files in directory.
  //
  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS         Status;
  VIRTUAL_DIR_DATA   *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_DIR_FROM_PROTOCOL (This);

  if (Data->UnderlyingProtocol != NULL) {
    Status = Data->UnderlyingProtocol->Close (
      Data->UnderlyingProtocol
    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  VirtualDirFree (This);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS         Status;

  Status = VirtualDirClose (This);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Virtual directories cannot be deleted.
  //
  return EFI_WARN_DELETE_FAILURE;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirRead (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
     OUT VOID                 *Buffer
  )
{
  EFI_STATUS            Status;
  VIRTUAL_DIR_DATA      *Data;
  LIST_ENTRY            *DirEntryLink;
  VIRTUAL_DIR_ENTRY     *DirEntry;
  UINTN                 ReadSize;
  UINTN                 FileStrSize;

  if (This == NULL
    || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_DIR_FROM_PROTOCOL (This);

  //
  // If our entry position is zero, try to read underlying protocol first.
  //
  ReadSize = *BufferSize;
  if (Data->CurrentEntry == NULL && Data->UnderlyingProtocol != NULL) {
    Status = Data->UnderlyingProtocol->Read (
      Data->UnderlyingProtocol,
      BufferSize,
      Buffer
      );

      if (EFI_ERROR (Status) || *BufferSize != 0) {
        return Status;
      }
  }

  //
  // Ensure we have any entries at all.
  //
  if (IsListEmpty (&Data->Entries)) {
    *BufferSize = 0;
    return EFI_SUCCESS;
  }

  //
  // End of directory.
  //
  if (Data->CurrentEntry != NULL && IsNodeAtEnd (&Data->Entries, Data->CurrentEntry)) {
    Data->CurrentEntry = NULL;

    *BufferSize = 0;
    return EFI_SUCCESS;
  }

  *BufferSize = ReadSize;
  DirEntryLink = Data->CurrentEntry != NULL ? GetNextNode (&Data->Entries, Data->CurrentEntry) : GetFirstNode (&Data->Entries);

  //
  // Get next file info structure.
  //
  DirEntry = GET_VIRTUAL_DIR_ENTRY_FROM_LINK (DirEntryLink);
  if (DirEntry->FileInfo->Size < SIZE_OF_EFI_FILE_INFO) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Ensure entry size matches string length.
  //
  FileStrSize = StrSize (DirEntry->FileInfo->FileName);
  if (OcOverflowAddUN (SIZE_OF_EFI_FILE_INFO, FileStrSize, &ReadSize) || ReadSize != DirEntry->FileInfo->Size) {
    return EFI_DEVICE_ERROR; 
  }

  if (*BufferSize < ReadSize) {
    *BufferSize = ReadSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Copy entry to buffer and advance to next entry.
  //
  CopyMem (Buffer, DirEntry->FileInfo, ReadSize);
  *BufferSize = ReadSize;

  Data->CurrentEntry = DirEntryLink;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirWrite (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
{
  //
  // Directories are not writeable.
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirSetPosition (
  IN EFI_FILE_PROTOCOL        *This,
  IN UINT64                   Position
  )
{
  EFI_STATUS         Status;
  VIRTUAL_DIR_DATA   *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Data = VIRTUAL_DIR_FROM_PROTOCOL (This);

  //
  // Non-zero requests are not supported for directories.
  //
  if (Position > 0) {
    return EFI_UNSUPPORTED;
  }

  Status = EFI_SUCCESS;
  if (Data->UnderlyingProtocol != NULL) {
    Status = Data->UnderlyingProtocol->SetPosition (
      Data->UnderlyingProtocol,
      0
      );
  }

  if (!EFI_ERROR (Status)) {
    Data->CurrentEntry = NULL;
  }
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirGetPosition (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT UINT64                  *Position
  )
{
  //
  // Not valid for directories.
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirGetInfo (
  IN  EFI_FILE_PROTOCOL       *This,
  IN  EFI_GUID                *InformationType,
  IN  OUT UINTN               *BufferSize,
  OUT VOID                    *Buffer
  )
{
  VIRTUAL_DIR_DATA   *Data;
  UINTN              InfoSize;  
  UINTN              NameSize;
  EFI_FILE_INFO      *FileInfo;
  BOOLEAN            Fits;

  if (This == NULL
    || InformationType == NULL
    || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_DIR_FROM_PROTOCOL (This);

  //
  // Get underlying protocol info.
  //
  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    STATIC_ASSERT (
      sizeof (FileInfo->FileName) == sizeof (CHAR16),
      "Header changed, flexible array member is now supported"
      );

    NameSize    = StrSize (Data->FileName);
    InfoSize    = SIZE_OF_EFI_FILE_INFO + NameSize;
    Fits        = *BufferSize >= InfoSize;
    *BufferSize = InfoSize;

    if (!Fits) {
      return EFI_BUFFER_TOO_SMALL;
    }

    if (Buffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    FileInfo = (EFI_FILE_INFO *) Buffer;

    ZeroMem (FileInfo, InfoSize - NameSize);
    FileInfo->Size = InfoSize;

    CopyMem (&FileInfo->CreateTime, &Data->ModificationTime, sizeof (FileInfo->ModificationTime));
    CopyMem (&FileInfo->LastAccessTime, &Data->ModificationTime, sizeof (FileInfo->ModificationTime));
    CopyMem (&FileInfo->ModificationTime, &Data->ModificationTime, sizeof (FileInfo->ModificationTime));

    //
    // Return zeroes for timestamps.
    //
    FileInfo->Attribute    = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
    CopyMem (&FileInfo->FileName[0], Data->FileName, NameSize);

    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirSetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
  //
  // Virtual directories are not writeable, this applies to info.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirFlush (
  IN EFI_FILE_PROTOCOL        *This
  )
{
  //
  // Virtual directories are not writeable.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirOpenEx (
  IN     EFI_FILE_PROTOCOL    *This,
  OUT    EFI_FILE_PROTOCOL    **NewHandle,
  IN     CHAR16               *FileName,
  IN     UINT64               OpenMode,
  IN     UINT64               Attributes,
  IN OUT EFI_FILE_IO_TOKEN    *Token
  )
{
  EFI_STATUS         Status;

  //
  // Ignore asynchronous interface for now.
  //
  // WARN: Unlike Open for OpenEx UEFI 2.7A explicitly dicates EFI_NO_MEDIA for
  //  "The specified file could not be found on the device." error case.
  //  We do not care for simplicity.
  //
  Status = VirtualDirOpen (
    This,
    NewHandle,
    FileName,
    OpenMode,
    Attributes
    );

  if (!EFI_ERROR (Status) && Token->Event != NULL) {
    Token->Status = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirReadEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  EFI_STATUS         Status;

  if (This == NULL
    || Token == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = VirtualDirRead (This, Token->Buffer, &Token->BufferSize);

  if (!EFI_ERROR (Status) && Token->Event != NULL) {
    Token->Status = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirWriteEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  //
  // Directories are not writeable.
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualDirFlushEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  //
  // Virtual directories are not writeable.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
CONST
EFI_FILE_PROTOCOL
mVirtualDirProtocolTemplate = {
  .Revision    = EFI_FILE_PROTOCOL_REVISION2,
  .Open        = VirtualDirOpen,
  .Close       = VirtualDirClose,
  .Delete      = VirtualDirDelete,
  .Read        = VirtualDirRead,
  .Write       = VirtualDirWrite,
  .GetPosition = VirtualDirGetPosition,
  .SetPosition = VirtualDirSetPosition,
  .GetInfo     = VirtualDirGetInfo,
  .SetInfo     = VirtualDirSetInfo,
  .Flush       = VirtualDirFlush,
  .OpenEx      = VirtualDirOpenEx,
  .ReadEx      = VirtualDirReadEx,
  .WriteEx     = VirtualDirWriteEx,
  .FlushEx     = VirtualDirFlushEx
};

EFI_STATUS
VirtualDirCreateOverlay (
  IN  CHAR16             *FileName,
  IN  CONST EFI_TIME     *ModificationTime OPTIONAL,
  IN  EFI_FILE_PROTOCOL  *UnderlyingFile OPTIONAL,
  OUT EFI_FILE_PROTOCOL  **File
  )
{
  VIRTUAL_DIR_DATA  *Data;

  ASSERT (FileName != NULL);
  ASSERT (File != NULL);

  Data = AllocatePool (sizeof (VIRTUAL_DIR_DATA));

  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Data->Signature           = VIRTUAL_DIR_DATA_SIGNATURE;
  Data->FileName            = FileName;
  Data->CurrentEntry        = NULL;
  Data->UnderlyingProtocol  = UnderlyingFile;
  InitializeListHead (&Data->Entries);
  CopyMem (&Data->Protocol, &mVirtualDirProtocolTemplate, sizeof (Data->Protocol));
  if (ModificationTime != NULL) {
    CopyMem (&Data->ModificationTime, ModificationTime, sizeof (*ModificationTime));
  } else {
    ZeroMem (&Data->ModificationTime, sizeof (*ModificationTime));
  }

  *File = &Data->Protocol;
  return EFI_SUCCESS;
}

EFI_STATUS
VirtualDirCreateOverlayFileNameCopy (
  IN  CONST CHAR16       *FileName,
  IN  CONST EFI_TIME     *ModificationTime OPTIONAL,
  IN  EFI_FILE_PROTOCOL  *UnderlyingFile OPTIONAL,
  OUT EFI_FILE_PROTOCOL  **File
  )
{
  EFI_STATUS          Status;
  CHAR16              *FileNameCopy;

  ASSERT (FileName != NULL);
  ASSERT (File != NULL);

  FileNameCopy = AllocateCopyPool (StrSize (FileName), FileName);
  if (FileNameCopy == NULL) {
    DEBUG ((DEBUG_WARN, "OCVFS: Failed to allocate directory name (%a) copy\n", FileName));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = VirtualDirCreateOverlay (FileNameCopy, ModificationTime, UnderlyingFile, File);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCVFS: Failed to virtualise directory (%a)\n", FileName));
    FreePool (FileNameCopy);
    return EFI_OUT_OF_RESOURCES;
  }
  return Status;
}

EFI_STATUS
VirtualDirAddEntry (
  IN EFI_FILE_PROTOCOL    *This,
  IN EFI_FILE_INFO        *FileInfo
  )
{
  VIRTUAL_DIR_DATA  *Data;
  VIRTUAL_DIR_ENTRY *NewEntry;

  ASSERT (This != NULL);
  ASSERT (FileInfo != NULL);

  Data = VIRTUAL_DIR_FROM_PROTOCOL (This);

  NewEntry = AllocateZeroPool (sizeof (*NewEntry));
  if (NewEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewEntry->Signature = VIRTUAL_DIR_ENTRY_SIGNATURE;
  NewEntry->FileInfo = FileInfo;

  InsertTailList (&Data->Entries, &NewEntry->Link);
  return EFI_SUCCESS;
}

VOID
VirtualDirFree (
  IN EFI_FILE_PROTOCOL    *This
  )
{
  VIRTUAL_DIR_DATA    *Data;
  LIST_ENTRY          *DirEntryLink;
  VIRTUAL_DIR_ENTRY   *DirEntry;

  ASSERT (This != NULL);

  Data = VIRTUAL_DIR_FROM_PROTOCOL (This);

  while (!IsListEmpty (&Data->Entries)) {
    DirEntryLink = GetFirstNode (&Data->Entries);
    DirEntry = GET_VIRTUAL_DIR_ENTRY_FROM_LINK (DirEntryLink);
    RemoveEntryList (DirEntryLink);

    if (DirEntry->FileInfo != NULL) {
      FreePool (DirEntry->FileInfo);
    }
    FreePool (DirEntry);
  }

  FreePool (Data->FileName);
  FreePool (Data);
}

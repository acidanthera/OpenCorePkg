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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcVirtualFsLib.h>

#include <Guid/FileInfo.h>

#define VIRTUAL_FILE_DATA_SIGNATURE  \
  SIGNATURE_32 ('V', 'F', 'S', 'f')

#define VIRTUAL_FILE_FROM_PROTOCOL(This) \
  CR (                           \
    This,                        \
    VIRTUAL_FILE_DATA,           \
    Protocol,                    \
    VIRTUAL_FILE_DATA_SIGNATURE  \
    )

typedef struct {
  UINT32              Signature;
  CHAR16              *FileName;
  UINT8               *FileBuffer;
  UINT64              FileSize;
  UINT64              FilePosition;
  EFI_TIME            ModificationTime;
  EFI_FILE_PROTOCOL   Protocol;
} VIRTUAL_FILE_DATA;

STATIC
EFI_STATUS
EFIAPI
VirtualFileOpen (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                  *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes
  )
{
  //
  // Virtual files are not directories and cannot be reopened.
  // TODO: May want to handle parent directory paths.
  //
  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  VIRTUAL_FILE_DATA  *Data;

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  FreePool (Data->FileBuffer);
  FreePool (Data->FileName);
  FreePool (Data);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  VirtualFileClose (This);
  //
  // Virtual files cannot be deleted.
  //
  return EFI_WARN_DELETE_FAILURE;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileRead (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
     OUT VOID                 *Buffer
  )
{
  VIRTUAL_FILE_DATA  *Data;
  UINTN              ReadSize;

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->FilePosition > Data->FileSize) {
    //
    // On entry, the current file position is beyond the end of the file.
    //
    return EFI_DEVICE_ERROR;
  }

  ReadSize = Data->FileSize - Data->FilePosition;

  if (*BufferSize >= ReadSize) {
    *BufferSize = ReadSize;
  } else {
    ReadSize = *BufferSize;
  }

  if (ReadSize > 0) {
    CopyMem (Buffer, &Data->FileBuffer[Data->FilePosition], ReadSize);
    Data->FilePosition += ReadSize;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileWrite (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
{
  //
  // Virtual files are not writeable.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileSetPosition (
  IN EFI_FILE_PROTOCOL        *This,
  IN UINT64                   Position
  )
{
  VIRTUAL_FILE_DATA  *Data;

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Position == 0xFFFFFFFFFFFFFFFFULL) {
    Data->FilePosition = Data->FileSize;
  } else {
    //
    // Seeking past the end of the file is allowed.
    //
    Data->FilePosition = Position;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileGetPosition (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT UINT64                  *Position
  )
{
  VIRTUAL_FILE_DATA  *Data;

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);
  *Position = Data->FilePosition;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileGetInfo (
  IN  EFI_FILE_PROTOCOL       *This,
  IN  EFI_GUID                *InformationType,
  IN  OUT UINTN               *BufferSize,
  OUT VOID                    *Buffer
  )
{
  VIRTUAL_FILE_DATA  *Data;
  UINTN              InfoSize;
  UINTN              NameSize;
  EFI_FILE_INFO      *FileInfo;
  BOOLEAN            Fits;

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    OC_INLINE_STATIC_ASSERT (
      sizeof (FileInfo->FileName) == sizeof (CHAR16),
      "Header changed, flexible array member is now supported"
      );

    FileInfo    = (EFI_FILE_INFO *) Buffer;
    NameSize    = StrSize (Data->FileName);
    InfoSize    = sizeof (EFI_FILE_INFO) - sizeof (CHAR16) + NameSize;
    Fits        = *BufferSize >= InfoSize;
    *BufferSize = InfoSize;

    if (!Fits) {
      return EFI_BUFFER_TOO_SMALL;
    }

    ZeroMem (FileInfo, InfoSize - NameSize);
    FileInfo->Size         = InfoSize;
    FileInfo->FileSize     = Data->FileSize;
    FileInfo->PhysicalSize = Data->FileSize;

    CopyMem (&FileInfo->CreateTime, &Data->ModificationTime, sizeof (FileInfo->ModificationTime));
    CopyMem (&FileInfo->LastAccessTime, &Data->ModificationTime, sizeof (FileInfo->ModificationTime));
    CopyMem (&FileInfo->ModificationTime, &Data->ModificationTime, sizeof (FileInfo->ModificationTime));

    //
    // Return zeroes for timestamps.
    //
    FileInfo->Attribute    = EFI_FILE_READ_ONLY;
    CopyMem (&FileInfo->FileName[0], Data->FileName, NameSize);

    return EFI_SUCCESS;
  }

  //
  // TODO: return some dummy data for EFI_FILE_SYSTEM_INFO?
  //

  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileSetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
  //
  // Virtual files are not writeable, this applies to info.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileFlush (
  IN EFI_FILE_PROTOCOL        *This
  )
{
  //
  // Virtual files are not writeable.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileOpenEx (
  IN     EFI_FILE_PROTOCOL    *This,
  OUT    EFI_FILE_PROTOCOL    **NewHandle,
  IN     CHAR16               *FileName,
  IN     UINT64               OpenMode,
  IN     UINT64               Attributes,
  IN OUT EFI_FILE_IO_TOKEN    *Token
  )
{
  //
  // Virtual files are not directories and cannot be reopened.
  // TODO: May want to handle parent directory paths.
  // WARN: Unlike Open for OpenEx UEFI 2.7A explicitly dicates EFI_NO_MEDIA for
  //  "The specified file could not be found on the device." error case.
  //
  return EFI_NO_MEDIA;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileReadEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  EFI_STATUS  Status;

  Status = VirtualFileRead (This, Token->Buffer, &Token->BufferSize);

  if (!EFI_ERROR (Status) && Token->Event != NULL) {
    Token->Status = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileWriteEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  //
  // Virtual files are not writeable.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileFlushEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  //
  // Virtual files are not writeable.
  //
  return EFI_WRITE_PROTECTED;
}

STATIC
CONST
EFI_FILE_PROTOCOL
mVirtualFileProtocolTemplate = {
  .Revision    = EFI_FILE_PROTOCOL_REVISION2,
  .Open        = VirtualFileOpen,
  .Close       = VirtualFileClose,
  .Delete      = VirtualFileDelete,
  .Read        = VirtualFileRead,
  .Write       = VirtualFileWrite,
  .GetPosition = VirtualFileGetPosition,
  .SetPosition = VirtualFileSetPosition,
  .GetInfo     = VirtualFileGetInfo,
  .SetInfo     = VirtualFileSetInfo,
  .Flush       = VirtualFileFlush,
  .OpenEx      = VirtualFileOpenEx,
  .ReadEx      = VirtualFileReadEx,
  .WriteEx     = VirtualFileWriteEx,
  .FlushEx     = VirtualFileFlushEx
};

EFI_STATUS
CreateVirtualFile (
  IN     CHAR16             *FileName,
  IN     VOID               *FileBuffer,
  IN     UINT64             FileSize,
  IN     EFI_TIME           *ModificationTime OPTIONAL,
  IN OUT EFI_FILE_PROTOCOL  **File
  )
{
  VIRTUAL_FILE_DATA  *Data;

  ASSERT (FileName != NULL);
  ASSERT (FileBuffer != NULL);
  ASSERT (File != NULL);

  Data = AllocatePool (sizeof (VIRTUAL_FILE_DATA));

  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Data->Signature    = VIRTUAL_FILE_DATA_SIGNATURE;
  Data->FileName     = FileName;
  Data->FileBuffer   = FileBuffer;
  Data->FileSize     = FileSize;
  Data->FilePosition = 0;
  CopyMem (&Data->Protocol, &mVirtualFileProtocolTemplate, sizeof (Data->Protocol));
  if (ModificationTime != NULL) {
    CopyMem (&Data->ModificationTime, ModificationTime, sizeof (*ModificationTime));
  } else {
    ZeroMem(&Data->ModificationTime, sizeof (*ModificationTime));
  }

  *File = &Data->Protocol;

  return EFI_SUCCESS;
}

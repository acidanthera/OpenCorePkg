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
#include <Library/OcFileLib.h>
#include <Library/OcVirtualFsLib.h>

#include <Guid/FileInfo.h>

#include "VirtualFsInternal.h"

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
  EFI_STATUS         Status;
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL || NewHandle == NULL || FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OpenCallback != NULL) {
    return Data->OpenCallback (
      Data->OriginalProtocol,
      NewHandle,
      FileName,
      OpenMode,
      Attributes
      );
  }

  if (Data->OriginalProtocol != NULL) {
    Status = SafeFileOpen (
      Data->OriginalProtocol,
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
  EFI_STATUS         Status;
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    FreePool (Data->FileBuffer);
    FreePool (Data->FileName);
    FreePool (Data);

    return EFI_SUCCESS;
  }

  Status = Data->OriginalProtocol->Close (
    Data->OriginalProtocol
    );
  FreePool (Data);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS         Status;
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    FreePool (Data->FileBuffer);
    FreePool (Data->FileName);
    FreePool (Data);
    //
    // Virtual files cannot be deleted.
    //
    return EFI_WARN_DELETE_FAILURE;
  }

  Status = Data->OriginalProtocol->Close (
    Data->OriginalProtocol
    );
  FreePool (Data);

  return Status;
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
  UINT64             ReadSize;
  UINTN              ReadBufferSize;

  if (This == NULL
    || BufferSize == NULL
    || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    if (Data->FilePosition > Data->FileSize) {
      //
      // On entry, the current file position is beyond the end of the file.
      //
      return EFI_DEVICE_ERROR;
    }

    ReadSize = (Data->FileSize - Data->FilePosition);

    if (*BufferSize >= ReadSize) {
      *BufferSize    = (UINTN)ReadSize;
      ReadBufferSize = (UINTN)ReadSize;
    } else {
      ReadBufferSize = *BufferSize;
    }

    if (ReadBufferSize > 0) {
      CopyMem (Buffer, &Data->FileBuffer[Data->FilePosition], ReadBufferSize);
      Data->FilePosition += ReadBufferSize;
    }

    return EFI_SUCCESS;
  }

  //
  // TODO: we may want to provide ReadCallback for directory info update.
  //

  return Data->OriginalProtocol->Read (
    Data->OriginalProtocol,
    BufferSize,
    Buffer
    );
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
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    //
    // Virtual files are not writeable.
    //
    return EFI_WRITE_PROTECTED;
  }

  return Data->OriginalProtocol->Write (
    Data->OriginalProtocol,
    BufferSize,
    Buffer
    );
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

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
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

  return Data->OriginalProtocol->SetPosition (
    Data->OriginalProtocol,
    Position
    );
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

  if (This == NULL
    || Position == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    *Position = Data->FilePosition;
    return EFI_SUCCESS;
  }

  return Data->OriginalProtocol->GetPosition (
    Data->OriginalProtocol,
    Position
    );
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
  EFI_STATUS         Status;
  VIRTUAL_FILE_DATA  *Data;
  UINTN              InfoSize;
  UINTN              NameSize;
  EFI_FILE_INFO      *FileInfo;
  BOOLEAN            Fits;

  if (This == NULL
    || InformationType == NULL
    || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
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

  Status = Data->OriginalProtocol->GetInfo (
    Data->OriginalProtocol,
    InformationType,
    BufferSize,
    Buffer
    );

  DEBUG ((DEBUG_VERBOSE, "Getting file info %g with now BufferSize %u mode gave - %r\n",
    InformationType, (UINT32) *BufferSize, Status));

  if (!EFI_ERROR (Status) && CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    DEBUG ((DEBUG_VERBOSE, "Got file size %u\n", (UINT32) ((EFI_FILE_INFO *) Buffer)->FileSize));
  }

  return Status;
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
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    //
    // Virtual files are not writeable, this applies to info.
    //
    return EFI_WRITE_PROTECTED;
  }

  return Data->OriginalProtocol->SetInfo (
    Data->OriginalProtocol,
    InformationType,
    BufferSize,
    Buffer
    );
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileFlush (
  IN EFI_FILE_PROTOCOL        *This
  )
{
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    //
    // Virtual files are not writeable.
    //
    return EFI_WRITE_PROTECTED;
  }

  return Data->OriginalProtocol->Flush (
    Data->OriginalProtocol
    );
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
  EFI_STATUS         Status;

  //
  // Ignore asynchronous interface for now.
  //
  // Virtual files are not directories and cannot be reopened.
  // TODO: May want to handle parent directory paths.
  // WARN: Unlike Open for OpenEx UEFI 2.7A explicitly dicates EFI_NO_MEDIA for
  //  "The specified file could not be found on the device." error case.
  //  We do not care for simplicity.
  //
  Status = VirtualFileOpen (
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
VirtualFileReadEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  EFI_STATUS         Status;
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL
    || Token == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    Status = VirtualFileRead (This, Token->Buffer, &Token->BufferSize);

    if (!EFI_ERROR (Status) && Token->Event != NULL) {
      Token->Status = EFI_SUCCESS;
      gBS->SignalEvent (Token->Event);
    }
  } else {
    Status = Data->OriginalProtocol->ReadEx (
      This,
      Token
      );
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
  VIRTUAL_FILE_DATA  *Data;

  ASSERT (This != NULL);

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    //
    // Virtual files are not writeable.
    //
    return EFI_WRITE_PROTECTED;
  }

  return Data->OriginalProtocol->WriteEx (
    This,
    Token
    );
}

STATIC
EFI_STATUS
EFIAPI
VirtualFileFlushEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  VIRTUAL_FILE_DATA  *Data;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = VIRTUAL_FILE_FROM_PROTOCOL (This);

  if (Data->OriginalProtocol == NULL) {
    //
    // Virtual files are not writeable.
    //
    return EFI_WRITE_PROTECTED;
  }

  return Data->OriginalProtocol->FlushEx (
    This,
    Token
    );
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
  IN  CHAR16             *FileName,
  IN  VOID               *FileBuffer,
  IN  UINT64             FileSize,
  IN  CONST EFI_TIME     *ModificationTime OPTIONAL,
  OUT EFI_FILE_PROTOCOL  **File
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

  Data->Signature        = VIRTUAL_FILE_DATA_SIGNATURE;
  Data->FileName         = FileName;
  Data->FileBuffer       = FileBuffer;
  Data->FileSize         = FileSize;
  Data->FilePosition     = 0;
  Data->OpenCallback     = NULL;
  Data->OriginalProtocol = NULL;
  CopyMem (&Data->Protocol, &mVirtualFileProtocolTemplate, sizeof (Data->Protocol));
  if (ModificationTime != NULL) {
    CopyMem (&Data->ModificationTime, ModificationTime, sizeof (*ModificationTime));
  } else {
    ZeroMem (&Data->ModificationTime, sizeof (*ModificationTime));
  }

  *File = &Data->Protocol;

  return EFI_SUCCESS;
}

EFI_STATUS
CreateVirtualFileFileNameCopy (
  IN  CONST CHAR16       *FileName,
  IN  VOID               *FileBuffer,
  IN  UINT64             FileSize,
  IN  CONST EFI_TIME     *ModificationTime OPTIONAL,
  OUT EFI_FILE_PROTOCOL  **File
  )
{
  EFI_STATUS          Status;
  CHAR16              *FileNameCopy;

  ASSERT (FileName != NULL);
  ASSERT (File != NULL);

  FileNameCopy = AllocateCopyPool (StrSize (FileName), FileName);
  if (FileNameCopy == NULL) {
    DEBUG ((DEBUG_WARN, "OCVFS: Failed to allocate file name (%s) copy\n", FileName));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = CreateVirtualFile (FileNameCopy, FileBuffer, FileSize, ModificationTime, File);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCVFS: Failed to virtualise file (%s)\n", FileName));
    FreePool (FileNameCopy);
    return EFI_OUT_OF_RESOURCES;
  }
  return Status;
}

STATIC
VOID
InternalInitVirtualVolumeData (
  IN OUT VIRTUAL_FILE_DATA  *Data,
  IN     EFI_FILE_OPEN      OpenCallback
  )
{
  ZeroMem (Data, sizeof (*Data));
  Data->Signature    = VIRTUAL_FILE_DATA_SIGNATURE;
  Data->OpenCallback = OpenCallback;
  CopyMem (&Data->Protocol, &mVirtualFileProtocolTemplate, sizeof (Data->Protocol));
}

EFI_STATUS
CreateRealFile (
  IN  EFI_FILE_PROTOCOL  *OriginalFile OPTIONAL,
  IN  EFI_FILE_OPEN      OpenCallback OPTIONAL,
  IN  BOOLEAN            CloseOnFailure,
  OUT EFI_FILE_PROTOCOL  **File
  )
{
  VIRTUAL_FILE_DATA  *Data;

  ASSERT (File != NULL);

  Data = AllocatePool (sizeof (VIRTUAL_FILE_DATA));

  if (Data == NULL) {
    if (CloseOnFailure) {
      ASSERT (OriginalFile != NULL);
      OriginalFile->Close (OriginalFile);
    }
    return EFI_OUT_OF_RESOURCES;
  }

  InternalInitVirtualVolumeData (Data, OpenCallback);
  Data->OriginalProtocol = OriginalFile;

  *File = &Data->Protocol;

  return EFI_SUCCESS;
}

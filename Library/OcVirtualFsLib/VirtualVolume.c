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
#include <Library/OcGuardLib.h>
#include <Library/OcVirtualFsLib.h>

#include <Guid/FileInfo.h>

#define VIRTUAL_VOLUME_DATA_SIGNATURE  \
  SIGNATURE_32 ('V', 'F', 'S', 'v')

#define VIRTUAL_VOLUME_FROM_FILESYSTEM_PROTOCOL(This) \
  CR (                             \
    This,                          \
    VIRTUAL_FILESYSTEM_DATA,       \
    FileSystem,                    \
    VIRTUAL_VOLUME_DATA_SIGNATURE  \
    )

#define VIRTUAL_VOLUME_FROM_FILE_PROTOCOL(This) \
  CR (                             \
    This,                          \
    VIRTUAL_FILESYSTEM_DATA,       \
    RootVolume,                    \
    VIRTUAL_VOLUME_DATA_SIGNATURE  \
    )

typedef struct {
  UINT32                           Signature;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *OriginalFileSystem;
  EFI_FILE_PROTOCOL                *OriginalRootVolume;
  EFI_FILE_OPEN                    OpenCallback;
  EFI_FILE_PROTOCOL                RootVolume;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  FileSystem;
} VIRTUAL_FILESYSTEM_DATA;

//
// Mere reallocatable array, never freed (cannot be, unless protocol uninstallation is hooked).
//
STATIC VIRTUAL_FILESYSTEM_DATA  **mVirtualFileSystems;
STATIC UINTN                    mVirtualFileSystemsUsed;
STATIC UINTN                    mVirtualFileSystemsAllocated;

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeOpen (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                  *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OpenCallback (
    Data->OriginalRootVolume,
    NewHandle,
    FileName,
    OpenMode,
    Attributes
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->Close (
    Data->OriginalRootVolume
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->Delete (
    Data->OriginalRootVolume
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeRead (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  //
  // TODO: we may want to provide ReadCallback for directory info update.
  //

  Status = Data->OriginalRootVolume->Read (
    Data->OriginalRootVolume,
    BufferSize,
    Buffer
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeWrite (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->Write (
    Data->OriginalRootVolume,
    BufferSize,
    Buffer
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeSetPosition (
  IN EFI_FILE_PROTOCOL        *This,
  IN UINT64                   Position
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->SetPosition (
    Data->OriginalRootVolume,
    Position
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeGetPosition (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT UINT64                  *Position
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->GetPosition (
    Data->OriginalRootVolume,
    Position
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeGetInfo (
  IN  EFI_FILE_PROTOCOL       *This,
  IN  EFI_GUID                *InformationType,
  IN  OUT UINTN               *BufferSize,
  OUT VOID                    *Buffer
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->GetInfo (
    Data->OriginalRootVolume,
    InformationType,
    BufferSize,
    Buffer
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeSetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->SetInfo (
    Data->OriginalRootVolume,
    InformationType,
    BufferSize,
    Buffer
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeFlush (
  IN EFI_FILE_PROTOCOL        *This
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->Flush (
    Data->OriginalRootVolume
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeOpenEx (
  IN     EFI_FILE_PROTOCOL    *This,
  OUT    EFI_FILE_PROTOCOL    **NewHandle,
  IN     CHAR16               *FileName,
  IN     UINT64               OpenMode,
  IN     UINT64               Attributes,
  IN OUT EFI_FILE_IO_TOKEN    *Token
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  //
  // Ignore asynchronous interface for now.
  //
  Status = Data->OpenCallback (
    Data->OriginalRootVolume,
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
VirtualVolumeReadEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->ReadEx (
    Data->OriginalRootVolume,
    Token
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeWriteEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->WriteEx (
    Data->OriginalRootVolume,
    Token
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualVolumeFlushEx (
  IN EFI_FILE_PROTOCOL      *This,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILE_PROTOCOL (This);

  Status = Data->OriginalRootVolume->FlushEx (
    Data->OriginalRootVolume,
    Token
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFsOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *This,
  OUT EFI_FILE_PROTOCOL                 **Root
  )
{
  EFI_STATUS               Status;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  Data = VIRTUAL_VOLUME_FROM_FILESYSTEM_PROTOCOL (This);

  Status = Data->OriginalFileSystem->OpenVolume (
    Data->OriginalFileSystem,
    &Data->OriginalRootVolume
    );

  if (!EFI_ERROR (Status)) {
    *Root = &Data->RootVolume;
  }

  return Status;
}

STATIC
CONST
EFI_FILE_PROTOCOL
mVirtualVolumeProtocolTemplate = {
  .Revision    = EFI_FILE_PROTOCOL_REVISION2,
  .Open        = VirtualVolumeOpen,
  .Close       = VirtualVolumeClose,
  .Delete      = VirtualVolumeDelete,
  .Read        = VirtualVolumeRead,
  .Write       = VirtualVolumeWrite,
  .GetPosition = VirtualVolumeGetPosition,
  .SetPosition = VirtualVolumeSetPosition,
  .GetInfo     = VirtualVolumeGetInfo,
  .SetInfo     = VirtualVolumeSetInfo,
  .Flush       = VirtualVolumeFlush,
  .OpenEx      = VirtualVolumeOpenEx,
  .ReadEx      = VirtualVolumeReadEx,
  .WriteEx     = VirtualVolumeWriteEx,
  .FlushEx     = VirtualVolumeFlushEx
};

STATIC
CONST
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
mVirtualFileSystemProtocolTemplate = {
  .Revision    = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  .OpenVolume  = VirtualFsOpenVolume
};

EFI_STATUS
CreateVirtualFs (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *OriginalFileSystem,
  IN     EFI_FILE_OPEN                    OpenCallback,
  IN OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **NewFileSystem
  )
{
  UINTN                    Index;
  VIRTUAL_FILESYSTEM_DATA  **NewVirtualFileSystems;
  VIRTUAL_FILESYSTEM_DATA  *Data;

  //
  // Return cached data first.
  //
  for (Index = 0; Index < mVirtualFileSystemsUsed; ++Index) {
    if (mVirtualFileSystems[Index]->OriginalFileSystem == OriginalFileSystem) {
      *NewFileSystem = &mVirtualFileSystems[Index]->FileSystem;
      return EFI_SUCCESS;
    }
  }

  //
  // Allocate more slots.
  //
  if (mVirtualFileSystemsUsed == mVirtualFileSystemsAllocated) {
    NewVirtualFileSystems = AllocatePool (
      (mVirtualFileSystemsAllocated * 2 + 1) * sizeof (mVirtualFileSystems[0])
      );
    if (NewVirtualFileSystems == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (mVirtualFileSystems != NULL) {
      CopyMem (
        &NewVirtualFileSystems[0],
        &mVirtualFileSystems[0],
        mVirtualFileSystemsUsed * sizeof (mVirtualFileSystems[0])
        );
      FreePool (mVirtualFileSystems);
    }

    mVirtualFileSystemsAllocated = mVirtualFileSystemsAllocated * 2 + 1;
    mVirtualFileSystems          = NewVirtualFileSystems;
  }

  Data = AllocatePool (sizeof (*Data));
  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Data->Signature          = VIRTUAL_VOLUME_DATA_SIGNATURE;
  Data->OriginalFileSystem = OriginalFileSystem;
  Data->OriginalRootVolume = NULL;
  Data->OpenCallback       = OpenCallback;
  CopyMem (&Data->RootVolume, &mVirtualVolumeProtocolTemplate, sizeof (Data->RootVolume));
  CopyMem (&Data->FileSystem, &mVirtualFileSystemProtocolTemplate, sizeof (Data->FileSystem));

  mVirtualFileSystems[mVirtualFileSystemsUsed] = Data;
  ++mVirtualFileSystemsUsed;

  *NewFileSystem = &Data->FileSystem;

  return EFI_SUCCESS;
}

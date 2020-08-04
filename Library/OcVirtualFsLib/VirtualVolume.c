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

#include "VirtualFsInternal.h"

//
// Mere reallocatable array, never freed (cannot be, unless protocol uninstallation is hooked).
//
STATIC VIRTUAL_FILESYSTEM_DATA  **mVirtualFileSystems;
STATIC UINTN                    mVirtualFileSystemsUsed;
STATIC UINTN                    mVirtualFileSystemsAllocated;

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
  EFI_FILE_PROTOCOL        *NewFile;

  Data = VIRTUAL_VOLUME_FROM_FILESYSTEM_PROTOCOL (This);

  Status = Data->OriginalFileSystem->OpenVolume (
    Data->OriginalFileSystem,
    &NewFile
    );

  if (!EFI_ERROR (Status)) {
    return CreateRealFile (NewFile, Data->OpenCallback, TRUE, Root);
  }

  return Status;
}

STATIC
CONST
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
mVirtualFileSystemProtocolTemplate = {
  .Revision    = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  .OpenVolume  = VirtualFsOpenVolume
};

EFI_STATUS
CreateVirtualFs (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *OriginalFileSystem,
  IN  EFI_FILE_OPEN                    OpenCallback,
  OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **NewFileSystem
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
  Data->OpenCallback       = OpenCallback;
  CopyMem (&Data->FileSystem, &mVirtualFileSystemProtocolTemplate, sizeof (Data->FileSystem));

  mVirtualFileSystems[mVirtualFileSystemsUsed] = Data;
  ++mVirtualFileSystemsUsed;

  *NewFileSystem = &Data->FileSystem;

  return EFI_SUCCESS;
}

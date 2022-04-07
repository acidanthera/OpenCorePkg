/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Driver.h"

EFI_STATUS
EFIAPI
FileGetPosition (
  IN  EFI_FILE_PROTOCOL *This,
  OUT UINT64            *Position
  )
{
  EFI_NTFS_FILE  *File;

  ASSERT (This != NULL);

  File = (EFI_NTFS_FILE *) This;

  *Position = File->IsDir ? File->DirIndex : File->Offset;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FileSetPosition (
  IN EFI_FILE_PROTOCOL *This,
  IN UINT64            Position
  )
{
  EFI_NTFS_FILE  *File;
  UINT64         FileSize;

  ASSERT (This != NULL);

  File = (EFI_NTFS_FILE *) This;

  if (File->IsDir) {
    if (Position != 0) {
      return EFI_INVALID_PARAMETER;
    }

    File->DirIndex = 0;
    return EFI_SUCCESS;
  }

  FileSize = File->RootFile.DataAttributeSize;

  if (Position == 0xFFFFFFFFFFFFFFFFULL) {
    Position = FileSize;
  } else if (Position > FileSize) {
    DEBUG ((DEBUG_INFO, "NTFS: '%s': Cannot seek to #%Lx of %Lx\n", File->Path, Position, FileSize));
    return EFI_UNSUPPORTED;
  }

  File->Offset = Position;

  return EFI_SUCCESS;
}

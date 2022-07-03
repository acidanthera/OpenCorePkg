/** @file
  Misc file utils.

  Copyright (c) 2022, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

BOOLEAN
OcFileExists (
  IN    CONST EFI_FILE_PROTOCOL  *Directory,
  IN    CONST CHAR16             *FileName
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *File;

  ASSERT (Directory != NULL);
  ASSERT (FileName != NULL);

  Status = OcSafeFileOpen (
             Directory,
             &File,
             (CHAR16 *)FileName,
             EFI_FILE_MODE_READ,
             0
             );

  if (!EFI_ERROR (Status)) {
    File->Close (File);
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
OcDeleteFile (
  IN EFI_FILE_PROTOCOL  *Directory,
  IN CONST CHAR16       *FileName
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *File;

  ASSERT (Directory != NULL);
  ASSERT (FileName != NULL);

  Status = OcSafeFileOpen (Directory, &File, FileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (!EFI_ERROR (Status)) {
    Status = File->Delete (File);
  }

  return Status;
}

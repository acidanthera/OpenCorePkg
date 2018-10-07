/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OcFileLibInternal.h"

// GetNextDirEntry
/**

  @param[in] DevicePath
  @param[out] DirEntry
  @param[in] SearchMask

  @retval EFI_SUCCESS  The volume label was successfully returned.
**/
EFI_STATUS
GetNextDirEntry (
  IN  EFI_FILE       *Directory,
  OUT EFI_FILE_INFO  **DirEntry,
  IN  UINT64         SearchMask
  )
{
  EFI_STATUS    Status;

  EFI_FILE_INFO *FileInfo;
  UINTN         FileInfoSize;
  UINTN         BufferSize;

  Status = EFI_INVALID_PARAMETER;

  if ((Directory != NULL) && (DirEntry != NULL)) {
    // Some fs drivers dont support passing a buffersize of 0 so
    // allocate buffer for FILE_INFO structure including a maximum
    // filename length of 255 chars.
    BufferSize = sizeof (*FileInfo) + (255 * sizeof (CHAR16));
    FileInfo   = AllocatePool (BufferSize);
    Status     = EFI_OUT_OF_RESOURCES;

    if (FileInfo != NULL) {
      // Iterate the directory entries for a match
      do {
        // Handle buggy FS driver implementations by passing a buffer
        // thats already allocated.

        FileInfoSize = BufferSize;
        Status       = Directory->Read (Directory, &FileInfoSize, FileInfo);

        if (!EFI_ERROR (Status) && (FileInfoSize != 0)) {
          if (((SearchMask == FILE_ITERATE_DIRECTORIES)
            && ((SearchMask & FileInfo->Attribute) == EFI_FILE_DIRECTORY))
           || ((SearchMask != FILE_ITERATE_DIRECTORIES)
            && ((SearchMask & FileInfo->Attribute) == FileInfo->Attribute))) {
            *DirEntry = FileInfo;

            Status = EFI_SUCCESS;

            break;
          }
        } else {
          // No match found so free the buffer

          Status = EFI_NOT_FOUND;

          FreePool ((VOID *)FileInfo);

          break;
        }
      } while (TRUE);
    }
  }

  return Status;
}

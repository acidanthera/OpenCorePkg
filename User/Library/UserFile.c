/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserFile.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

UINT8 *
UserReadFile (
  IN  CONST CHAR8 *FileName,
  OUT UINT32      *Size
  )
{
  FILE   *FilePtr;
  INT64  FileSize;
  UINT8  *Buffer;

  ASSERT (FileName != NULL);
  ASSERT (Size != NULL);

  FilePtr = fopen (FileName, "rb");
  if (FilePtr == NULL) {
    return NULL;
  }

  fseek (FilePtr, 0, SEEK_END);
  FileSize = ftell (FilePtr);
  fseek (FilePtr, 0, SEEK_SET);

  Buffer = AllocatePool (FileSize + 1);
  if (Buffer == NULL
    || (FileSize > 0 && fread (Buffer, FileSize, 1, FilePtr) != 1)) {
    abort ();
  }
  fclose (FilePtr);

  Buffer[FileSize] = 0;
  *Size = FileSize;

  return Buffer;
}

VOID
UserWriteFile (
  IN  CONST CHAR8 *FileName,
  IN  VOID        *Data,
  IN  UINT32      Size
  )
{
  FILE   *FilePtr;

  FilePtr = fopen (FileName, "wb");

  if (FilePtr == NULL) {
    abort ();
  }
  
  if (fwrite (Data, Size, 1, FilePtr) != 1) {
    abort ();
  }

  fclose (FilePtr);
}

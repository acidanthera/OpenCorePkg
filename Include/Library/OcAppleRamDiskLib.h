/** @file
  Apple RAM Disk library.

Copyright (C) 2019, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_APPLE_RAM_DISK_LIB_H
#define OC_APPLE_RAM_DISK_LIB_H

#include <Protocol/AppleRamDisk.h>
#include <Protocol/SimpleFileSystem.h>

/**
  Request allocation of Size bytes in extents table.

  @param[in]  Size          Requested memory size.
  @param[in]  MemoryType    Requested memory type.

  @retval Allocated extent table.
**/
CONST APPLE_RAM_DISK_EXTENT_TABLE *
OcAppleRamDiskAllocate (
  IN UINTN            Size,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Read RAM disk data.

  @param[in]  ExtentTable Allocated extent table.
  @param[in]  Offset      Offset in RAM disk.
  @param[in]  Size        Amount of data to read.
  @param[out] Buffer      Resulting data.

  @retval TRUE on success.
**/
BOOLEAN
OcAppleRamDiskRead (
  IN  CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN  UINTN                              Offset,
  IN  UINTN                              Size,
  OUT VOID                               *Buffer
  );

/**
  Write RAM disk data.

  @param[in]  ExtentTable Allocated extent table.
  @param[in]  Offset      Offset in RAM disk.
  @param[in]  Size        Amount of data to write.
  @param[in]  Buffer      Source data.

  @retval TRUE on success.
**/
BOOLEAN
OcAppleRamDiskWrite (
  IN CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN UINTN                              Offset,
  IN UINTN                              Size,
  IN CONST VOID                         *Buffer
  );

/**
  Load file into RAM disk as it is.

  @param[in]  ExtentTable Allocated extent table.
  @param[in]  File        File protocol open for reading.
  @param[in]  FileSize    Amount of data to write.

  @retval TRUE on success.
**/
BOOLEAN
OcAppleRamDiskLoadFile (
  IN OUT CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN     EFI_FILE_PROTOCOL                  *File,
  IN     UINTN                              FileSize
  );

/**
  Free RAM disk.

  @param[in]  ExtentTable Allocated extent table.
**/
VOID
OcAppleRamDiskFree (
  IN CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable
  );

#endif // OC_APPLE_RAM_DISK_LIB_H

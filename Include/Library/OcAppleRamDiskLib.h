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

APPLE_RAM_DISK_EXTENT_TABLE *
OcAppleRamDiskAllocate (
  IN UINTN            Size,
  IN EFI_MEMORY_TYPE  MemoryType
  );

BOOLEAN
OcAppleRamDiskRead (
  IN  APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN  UINT64                       Offset,
  IN  UINTN                        Size,
  OUT VOID                         *Buffer
  );

BOOLEAN
OcAppleRamDiskWrite (
  IN APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN UINT64                       Offset,
  IN UINTN                        Size,
  IN CONST VOID                   *Buffer
  );

BOOLEAN
OcAppleRamDiskLoadFile (
  IN OUT APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN     EFI_FILE_PROTOCOL            *File,
  IN     UINTN                        FileSize
  );

VOID
OcAppleRamDiskFree (
  IN APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable
  );

#endif // OC_APPLE_RAM_DISK_LIB_H

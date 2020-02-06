/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_DISK_IMAGE_LIB_H_
#define APPLE_DISK_IMAGE_LIB_H_

#include <IndustryStandard/AppleDiskImage.h>

#include <Protocol/SimpleFileSystem.h>

#include <Library/OcAppleChunklistLib.h>
#include <Library/OcAppleRamDiskLib.h>

//
// Disk image context.
//
typedef struct {
    CONST APPLE_RAM_DISK_EXTENT_TABLE *ExtentTable;

    UINTN                             SectorCount;

    UINT32                            BlockCount;
    APPLE_DISK_IMAGE_BLOCK_DATA       **Blocks;
} OC_APPLE_DISK_IMAGE_CONTEXT;

BOOLEAN
OcAppleDiskImageInitializeContext (
  OUT OC_APPLE_DISK_IMAGE_CONTEXT        *Context,
  IN  CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN  UINTN                              FileSize
  );

BOOLEAN
OcAppleDiskImageInitializeFromFile (
  OUT OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  EFI_FILE_PROTOCOL            *File
  );

VOID
OcAppleDiskImageFreeContext (
  IN OC_APPLE_DISK_IMAGE_CONTEXT *Context
  );
VOID
OcAppleDiskImageFreeFile (
  IN OC_APPLE_DISK_IMAGE_CONTEXT  *Context
  );

BOOLEAN
OcAppleDiskImageVerifyData (
  IN OUT OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN OUT OC_APPLE_CHUNKLIST_CONTEXT   *ChunklistContext
  );

BOOLEAN
OcAppleDiskImageRead (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  UINTN                        Lba,
  IN  UINTN                        BufferSize,
  OUT VOID                         *Buffer
  );

EFI_HANDLE
OcAppleDiskImageInstallBlockIo (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT     *Context,
  IN  UINTN                           FileSize,
  OUT CONST EFI_DEVICE_PATH_PROTOCOL  **DevicePath OPTIONAL,
  OUT UINTN                           *DevicePathSize OPTIONAL
  );

VOID
OcAppleDiskImageUninstallBlockIo (
  IN OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN VOID                         *BlockIoHandle
  );

#endif

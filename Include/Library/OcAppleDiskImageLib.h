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

//
// Disk image block context.
//
typedef struct {
    UINT16                          Attributes;
    CHAR8                           *CfName;
    CHAR8                           *Name;
    UINT32                          Id;
    APPLE_DISK_IMAGE_BLOCK_DATA     *BlockData;
} OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT;

//
// Disk image context.
//
typedef struct {
    // Source buffer.
    VOID                            *Buffer;
    UINTN                           Length;

    // Disk image info.
    APPLE_DISK_IMAGE_TRAILER        Trailer;
    UINT32                          BlockCount;
    OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT  *Blocks;

    // Block I/O info.
    EFI_HANDLE                      BlockIoHandle;
} OC_APPLE_DISK_IMAGE_CONTEXT;

EFI_STATUS
EFIAPI
OcAppleDiskImageInitializeContext(
    IN  VOID *Buffer,
    IN  UINTN BufferLength,
    OUT OC_APPLE_DISK_IMAGE_CONTEXT **Context);

EFI_STATUS
EFIAPI
OcAppleDiskImageFreeContext(
    IN OC_APPLE_DISK_IMAGE_CONTEXT *Context);

EFI_STATUS
EFIAPI
OcAppleDiskImageRead(
    IN  OC_APPLE_DISK_IMAGE_CONTEXT *Context,
    IN  EFI_LBA Lba,
    IN  UINTN BufferSize,
    OUT VOID *Buffer);

EFI_STATUS
EFIAPI
OcAppleDiskImageInstallBlockIo(
    IN OC_APPLE_DISK_IMAGE_CONTEXT *Context);

EFI_STATUS
EFIAPI
OcAppleDiskImageUninstallBlockIo(
    IN OC_APPLE_DISK_IMAGE_CONTEXT *Context);

#endif

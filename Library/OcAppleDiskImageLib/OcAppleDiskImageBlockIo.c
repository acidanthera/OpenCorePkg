/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Protocol/BlockIo.h>

#include <Library/DebugLib.h>
#include <Library/OcAppleDiskImageLib.h>

#include "OcAppleDiskImageLibInternal.h"

//
// Block I/O protocol template.
//
EFI_BLOCK_IO_PROTOCOL mDiskImageBlockIo = {
  EFI_BLOCK_IO_PROTOCOL_REVISION,
  NULL,
  DiskImageBlockIoReset,
  DiskImageBlockIoReadBlocks,
  DiskImageBlockIoWriteBlocks,
  DiskImageBlockIoFlushBlocks
};

//
// Block I/O protocol functions.
//
EFI_STATUS
EFIAPI
DiskImageBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DiskImageBlockIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  ) 
{
  OC_APPLE_DISK_IMAGE_MOUNTED_DATA  *DiskImageData;

  ASSERT (This != NULL);
  ASSERT (Buffer != NULL);

  //
  // Read data from image.
  //
  DiskImageData = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS(This);
  return OcAppleDiskImageRead(DiskImageData->ImageContext, Lba, BufferSize, Buffer);
}

EFI_STATUS
EFIAPI
DiskImageBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
{
  return EFI_WRITE_PROTECTED;
}

EFI_STATUS
EFIAPI
DiskImageBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  ) 
{
  return EFI_SUCCESS;
}

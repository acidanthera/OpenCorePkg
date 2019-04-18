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
#include <Protocol/AppleRamDisk.h>
#include <Protocol/AppleDiskImage.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OcAppleDiskImageLibInternal.h"

#define DMG_FILE_PATH_LEN  (L_STR_LEN (L"DMG_.dmg") + 16 + 1)

#pragma pack(1)

typedef PACKED struct {
  VENDOR_DEVICE_PATH Vendor;
  UINT32             Key;
} DMG_CONTROLLER_DEVICE_PATH;

typedef PACKED struct {
  VENDOR_DEFINED_DEVICE_PATH Vendor;
  UINT64                     Length;
} DMG_SIZE_DEVICE_PATH;

typedef PACKED struct {
  EFI_DEVICE_PATH_PROTOCOL Header;
  ///
  /// A NULL-terminated Path string including directory and file names.
  ///
  CHAR16                   PathName[DMG_FILE_PATH_LEN];
} DMG_FILEPATH_DEVICE_PATH;

typedef PACKED struct {
  APPLE_RAM_DISK_DP          RamDisk;
  DMG_FILEPATH_DEVICE_PATH   FilePath;
  DMG_SIZE_DEVICE_PATH       Size;
  EFI_DEVICE_PATH_PROTOCOL   End;
} DMG_DEVICE_PATH;

#pragma pack()

#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE  \
  SIGNATURE_32('D','m','g','I')

#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS(This)  \
  BASE_CR (                                               \
    (This),                                               \
    OC_APPLE_DISK_IMAGE_MOUNTED_DATA,                     \
    BlockIo                                               \
    )

typedef struct {
  UINT32                      Signature;

  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO_MEDIA          BlockIoMedia;
  DMG_DEVICE_PATH             DevicePath;

  OC_APPLE_DISK_IMAGE_CONTEXT *ImageContext;
  APPLE_RAM_DISK_EXTENT_TABLE *RamDmgHeader;
} OC_APPLE_DISK_IMAGE_MOUNTED_DATA;

STATIC
EFI_STATUS
EFIAPI
DiskImageBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  return EFI_SUCCESS;
}

STATIC
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
  OC_APPLE_DISK_IMAGE_MOUNTED_DATA *DiskImageData;
  BOOLEAN                          Result;

  if ((This == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BufferSize % APPLE_DISK_IMAGE_SECTOR_SIZE) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  DiskImageData = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS (This);
  if (DiskImageData->Signature == 0) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (DiskImageData->Signature == OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE);

  if (Lba >= DiskImageData->ImageContext->SectorCount) {
    return EFI_INVALID_PARAMETER;
  }

  Result = OcAppleDiskImageRead (
             DiskImageData->ImageContext,
             Lba,
             BufferSize,
             Buffer
             );
  if (!Result) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
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

STATIC
EFI_STATUS
EFIAPI
DiskImageBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  ) 
{
  return EFI_SUCCESS;
}

STATIC UINT32 mDmgCounter; ///< FIXME: This should exist on a protocol basis!

STATIC
VOID
InternalConstructDmgDevicePath (
  IN OUT OC_APPLE_DISK_IMAGE_MOUNTED_DATA  *DiskImageData,
  IN     EFI_PHYSICAL_ADDRESS              RamDmgAddress
  )
{
  UINT64          DmgSize;
  DMG_DEVICE_PATH *DevPath;

  ASSERT (DiskImageData != NULL);

  DmgSize = DiskImageData->ImageContext->Length;
  DevPath = &DiskImageData->DevicePath;

  DevPath->RamDisk.Vendor.Header.Type    = HARDWARE_DEVICE_PATH;
  DevPath->RamDisk.Vendor.Header.SubType = HW_VENDOR_DP;
  CopyMem (
    &DevPath->RamDisk.Vendor.Guid,
    &gAppleRamDiskProtocolGuid,
    sizeof (DevPath->RamDisk.Vendor.Guid)
    );
  DevPath->RamDisk.Counter               = mDmgCounter++;
  SetDevicePathNodeLength (
    &DevPath->RamDisk.Vendor,
    sizeof (DevPath->RamDisk.Vendor) + sizeof (DevPath->RamDisk.Counter)
    );

  DevPath->RamDisk.MemMap.Header.Type     = HARDWARE_DEVICE_PATH;
  DevPath->RamDisk.MemMap.Header.SubType  = HW_MEMMAP_DP;
  DevPath->RamDisk.MemMap.MemoryType      = EfiACPIMemoryNVS;
  DevPath->RamDisk.MemMap.StartingAddress = RamDmgAddress;
  DevPath->RamDisk.MemMap.EndingAddress   = RamDmgAddress + sizeof (APPLE_RAM_DISK_EXTENT);
  SetDevicePathNodeLength (&DevPath->RamDisk.MemMap, sizeof (DevPath->RamDisk.MemMap));

  DevPath->FilePath.Header.Type    = MEDIA_DEVICE_PATH;
  DevPath->FilePath.Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (&DevPath->FilePath, sizeof (DevPath->FilePath));
  UnicodeSPrint (
    DevPath->FilePath.PathName,
    sizeof (DevPath->FilePath.PathName),
    L"DMG_%16X.dmg",
    DmgSize
    );

  DevPath->Size.Vendor.Header.Type    = MESSAGING_DEVICE_PATH;
  DevPath->Size.Vendor.Header.SubType = MSG_VENDOR_DP;
  DevPath->Size.Length                = DmgSize;
  SetDevicePathNodeLength (&DevPath->Size, sizeof (DevPath->Size));
  CopyMem (
    &DevPath->Size.Vendor.Guid,
    &gAppleDiskImageProtocolGuid,
    sizeof (DevPath->Size.Vendor.Guid)
    );

  SetDevicePathEndNode (&DevPath->End);

  ASSERT (
    IsDevicePathValid ((EFI_DEVICE_PATH_PROTOCOL *)DevPath, sizeof (*DevPath))
    );
}

STATIC CONST EFI_BLOCK_IO_PROTOCOL mDiskImageBlockIo = {
  EFI_BLOCK_IO_PROTOCOL_REVISION,
  NULL,
  DiskImageBlockIoReset,
  DiskImageBlockIoReadBlocks,
  DiskImageBlockIoWriteBlocks,
  DiskImageBlockIoFlushBlocks
};

EFI_HANDLE
OcAppleDiskImageInstallBlockIo (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT     *Context,
  OUT CONST EFI_DEVICE_PATH_PROTOCOL  **DevicePath OPTIONAL,
  OUT UINTN                           *DevicePathSize OPTIONAL
  )
{
  EFI_HANDLE                       BlockIoHandle;

  EFI_STATUS                       Status;
  OC_APPLE_DISK_IMAGE_MOUNTED_DATA *DiskImageData;
  UINTN                            NumRamDmgPages;
  EFI_PHYSICAL_ADDRESS             RamDmgAddress;
  APPLE_RAM_DISK_EXTENT_TABLE      *RamDmgHeader;

  ASSERT (Context != NULL);

  NumRamDmgPages = EFI_SIZE_TO_PAGES (sizeof (*RamDmgHeader));
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIMemoryNVS,
                  NumRamDmgPages,
                  &RamDmgAddress
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  RamDmgHeader = (APPLE_RAM_DISK_EXTENT_TABLE *)(UINTN)RamDmgAddress;
  ZeroMem (RamDmgHeader, sizeof (*RamDmgHeader));

  RamDmgHeader->Signature            = APPLE_RAM_DISK_EXTENT_SIGNATURE;
  RamDmgHeader->Signature2           = APPLE_RAM_DISK_EXTENT_SIGNATURE;
  RamDmgHeader->Version              = APPLE_RAM_DISK_EXTENT_VERSION;
  RamDmgHeader->ExtentCount          = 1;
  RamDmgHeader->Extents[0].Start     = Context->Buffer;
  RamDmgHeader->Extents[0].Length    = Context->Length;
  DEBUG ((
    DEBUG_VERBOSE,
    "DMG extent @ %p, length 0x%lx\n",
    RamDmgHeader->Extents[0].Start,
    RamDmgHeader->Extents[0].Length
    ));

  DiskImageData = AllocateZeroPool (sizeof (*DiskImageData));
  if (DiskImageData == NULL) {
    gBS->FreePages (RamDmgAddress, NumRamDmgPages);
    return NULL;
  }

  DiskImageData->Signature    = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE;
  DiskImageData->ImageContext = Context;
  DiskImageData->RamDmgHeader = RamDmgHeader;
  CopyMem (
    &DiskImageData->BlockIo,
    &mDiskImageBlockIo,
    sizeof (DiskImageData->BlockIo)
    );

  DiskImageData->BlockIo.Media             = &DiskImageData->BlockIoMedia;
  DiskImageData->BlockIoMedia.MediaPresent = TRUE;
  DiskImageData->BlockIoMedia.ReadOnly     = TRUE;
  DiskImageData->BlockIoMedia.BlockSize    = APPLE_DISK_IMAGE_SECTOR_SIZE;
  DiskImageData->BlockIoMedia.LastBlock    = (Context->SectorCount - 1);

  InternalConstructDmgDevicePath (DiskImageData, RamDmgAddress);

  BlockIoHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &BlockIoHandle,
                  &gEfiBlockIoProtocolGuid,
                  &DiskImageData->BlockIo,
                  &gEfiDevicePathProtocolGuid,
                  &DiskImageData->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (DiskImageData);
    gBS->FreePages (RamDmgAddress, NumRamDmgPages);
    return NULL;
  }

  Status = gBS->ConnectController (BlockIoHandle, NULL, NULL, TRUE);
  if (EFI_ERROR (Status)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    BlockIoHandle,
                    &gEfiDevicePathProtocolGuid,
                    &DiskImageData->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &DiskImageData->BlockIo,
                    NULL
                    );
    if (!EFI_ERROR (Status)) {
      FreePool (DiskImageData);
    } else {
      DiskImageData->Signature = 0;
    }
    gBS->FreePages (RamDmgAddress, NumRamDmgPages);
    return NULL;
  }

  if (DevicePath != NULL) {
    *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&DiskImageData->DevicePath;

    if (DevicePathSize != NULL) {
      *DevicePathSize = sizeof (DiskImageData->DevicePath);
    }
  }

  return BlockIoHandle;
}

VOID
OcAppleDiskImageUninstallBlockIo (
  IN OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN VOID                         *BlockIoHandle
  )
{
  EFI_STATUS                       Status;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  OC_APPLE_DISK_IMAGE_MOUNTED_DATA *DiskImageData;

  ASSERT (Context != NULL);
  ASSERT (BlockIoHandle != NULL);

  Status = gBS->HandleProtocol (
                  BlockIoHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  DiskImageData = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS (BlockIo);

  gBS->FreePages (
         (EFI_PHYSICAL_ADDRESS)(UINTN)DiskImageData->RamDmgHeader,
         EFI_SIZE_TO_PAGES (sizeof (*DiskImageData->RamDmgHeader))
         );

  Status  = gBS->DisconnectController (BlockIoHandle, NULL, NULL);
  Status |= gBS->UninstallMultipleProtocolInterfaces (
                  BlockIoHandle,
                  &gEfiBlockIoProtocolGuid,
                  &DiskImageData->BlockIo,
                  &gEfiDevicePathProtocolGuid,
                  &DiskImageData->DevicePath,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    FreePool (DiskImageData);
  } else {
    DiskImageData->Signature = 0;
  }
}

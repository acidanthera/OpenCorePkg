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

#include <Protocol/AppleDiskImage.h>
#include <Protocol/AppleRamDisk.h>
#include <Protocol/BlockIo.h>

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

#pragma pack(push, 1)

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
  APPLE_RAM_DISK_DP_HEADER   RamDisk;
  DMG_FILEPATH_DEVICE_PATH   FilePath;
  DMG_SIZE_DEVICE_PATH       Size;
  EFI_DEVICE_PATH_PROTOCOL   End;
} DMG_DEVICE_PATH;

#pragma pack(pop)

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
             (UINTN)Lba,
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
  IN     UINTN                             FileSize
  )
{
  UINT64          RamDmgAddress;
  DMG_DEVICE_PATH *DevPath;
  CHAR16          *UnicodeDevPath;

  ASSERT (DiskImageData != NULL);
  ASSERT (DiskImageData->ImageContext);

  RamDmgAddress = (UINTN)DiskImageData->ImageContext->ExtentTable;

  DevPath = &DiskImageData->DevicePath;

  DevPath->RamDisk.Vendor.Vendor.Header.Type    = HARDWARE_DEVICE_PATH;
  DevPath->RamDisk.Vendor.Vendor.Header.SubType = HW_VENDOR_DP;
  CopyGuid (
    &DevPath->RamDisk.Vendor.Vendor.Guid,
    &gAppleRamDiskProtocolGuid
    );
  DevPath->RamDisk.Vendor.Counter               = mDmgCounter++;
  SetDevicePathNodeLength (
    &DevPath->RamDisk.Vendor,
    sizeof (DevPath->RamDisk.Vendor)
    );

  DevPath->RamDisk.MemMap.Header.Type     = HARDWARE_DEVICE_PATH;
  DevPath->RamDisk.MemMap.Header.SubType  = HW_MEMMAP_DP;
  DevPath->RamDisk.MemMap.MemoryType      = EfiACPIMemoryNVS;
  DevPath->RamDisk.MemMap.StartingAddress = RamDmgAddress;
  DevPath->RamDisk.MemMap.EndingAddress   = RamDmgAddress + sizeof (APPLE_RAM_DISK_EXTENT_TABLE);
  SetDevicePathNodeLength (&DevPath->RamDisk.MemMap, sizeof (DevPath->RamDisk.MemMap));

  DevPath->FilePath.Header.Type    = MEDIA_DEVICE_PATH;
  DevPath->FilePath.Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (&DevPath->FilePath, sizeof (DevPath->FilePath));
  UnicodeSPrint (
    DevPath->FilePath.PathName,
    sizeof (DevPath->FilePath.PathName),
    L"DMG_%16X.dmg",
    FileSize
    );

  DevPath->Size.Vendor.Header.Type    = MESSAGING_DEVICE_PATH;
  DevPath->Size.Vendor.Header.SubType = MSG_VENDOR_DP;
  DevPath->Size.Length                = FileSize;
  SetDevicePathNodeLength (&DevPath->Size, sizeof (DevPath->Size));
  CopyGuid (
    (VOID *)&DevPath->Size.Vendor.Guid,
    &gAppleDiskImageProtocolGuid
    );

  SetDevicePathEndNode (&DevPath->End);

  DEBUG_CODE_BEGIN ();
  ASSERT (
    IsDevicePathValid ((EFI_DEVICE_PATH_PROTOCOL *) DevPath, sizeof (*DevPath))
    );

  UnicodeDevPath = ConvertDevicePathToText ((EFI_DEVICE_PATH_PROTOCOL *)DevPath, FALSE, FALSE);
  DEBUG ((DEBUG_INFO, "OCDI: Built DMG DP: %s\n", UnicodeDevPath != NULL ? UnicodeDevPath : L"<NULL>"));
  if (UnicodeDevPath != NULL) {
    FreePool (UnicodeDevPath);
  }
  DEBUG_CODE_END ();
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
  IN  UINTN                           FileSize,
  OUT CONST EFI_DEVICE_PATH_PROTOCOL  **DevicePath OPTIONAL,
  OUT UINTN                           *DevicePathSize OPTIONAL
  )
{
  EFI_HANDLE                       BlockIoHandle;

  EFI_STATUS                       Status;
  OC_APPLE_DISK_IMAGE_MOUNTED_DATA *DiskImageData;

  ASSERT (Context != NULL);
  ASSERT (FileSize > 0);

  DiskImageData = AllocateZeroPool (sizeof (*DiskImageData));
  if (DiskImageData == NULL) {
    DEBUG ((DEBUG_INFO, "OCDI: Failed to allocate DMG mount context\n"));
    return NULL;
  }

  DiskImageData->Signature    = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE;
  DiskImageData->ImageContext = Context;
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

  InternalConstructDmgDevicePath (DiskImageData, FileSize);

  BlockIoHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &BlockIoHandle,
                  &gEfiDevicePathProtocolGuid,
                  &DiskImageData->DevicePath,
                  &gEfiBlockIoProtocolGuid,
                  &DiskImageData->BlockIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDI: Failed to install protocols %r\n", Status));
    FreePool (DiskImageData);
    return NULL;
  }

  Status = gBS->ConnectController (BlockIoHandle, NULL, NULL, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDI: Failed to connect DMG handle %r\n", Status));

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
      DEBUG ((DEBUG_INFO, "OCDI: Failed to uninstall protocols %r\n", Status));
      DiskImageData->Signature = 0;
    }
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
    DEBUG ((DEBUG_INFO, "OCDI: Invalid handle for Block I/O uninstall\n"));
    return;
  }

  DiskImageData = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS (BlockIo);

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
    DEBUG ((
      DEBUG_INFO,
      "OCDI: Failed to disconnect DMG controller or uninstal protocols\n"
      ));
    DiskImageData->Signature = 0;
  }
}

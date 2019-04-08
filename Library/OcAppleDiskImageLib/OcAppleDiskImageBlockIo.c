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

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OcAppleDiskImageLibInternal.h"

#define DMG_CONTROLLER_DP_GUID   \
  { 0x957932CC, 0x7E8E, 0x433B,  \
    { 0x8F, 0x41, 0xD3, 0x91, 0xEA, 0x3C, 0x10, 0xF8 } }

#define DMG_SIZE_DP_GUID         \
  { 0x004B07E8, 0x0B9C, 0x427E,  \
    { 0xB0, 0xD4, 0xA4, 0x66, 0xE6, 0xE5, 0x7A, 0x62 } }

#define DMG_FILE_PATH_LEN  (L_STR_LEN (L"DMG_.dmg") + 16 + 1)

#pragma pack(1)

typedef PACKED struct {
  EFI_DEVICE_PATH_PROTOCOL Header;
  EFI_GUID                 Guid;
  UINT32                   Key;
} DMG_CONTROLLER_DEVICE_PATH;

typedef PACKED struct {
  EFI_DEVICE_PATH_PROTOCOL Header;
  EFI_GUID                 Guid;
  UINT64                   Length;
} DMG_SIZE_DEVICE_PATH;

typedef PACKED struct {
  EFI_DEVICE_PATH_PROTOCOL Header;
  ///
  /// A NULL-terminated Path string including directory and file names.
  ///
  CHAR16                   PathName[DMG_FILE_PATH_LEN];
} DMG_FILEPATH_DEVICE_PATH;

typedef PACKED struct {
  DMG_CONTROLLER_DEVICE_PATH Controller;
  MEMMAP_DEVICE_PATH         MemMap;
  DMG_FILEPATH_DEVICE_PATH   FilePath;
  DMG_SIZE_DEVICE_PATH       Size;
  EFI_DEVICE_PATH_PROTOCOL   End;
} DMG_DEVICE_PATH;

#pragma pack()

#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE  \
  SIGNATURE_32('D','m','g','I')

#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS(This)  \
  CR (                                                    \
    (This),                                               \
    OC_APPLE_DISK_IMAGE_MOUNTED_DATA,                     \
    BlockIo,                                              \
    OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE            \
    )

typedef struct {
  UINT32                      Signature;

  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO_MEDIA          BlockIoMedia;
  DMG_DEVICE_PATH             DevicePath;

  OC_APPLE_DISK_IMAGE_CONTEXT *ImageContext;
  RAM_DMG_HEADER              *RamDmgHeader;
} OC_APPLE_DISK_IMAGE_MOUNTED_DATA;

STATIC CONST EFI_GUID mDmgControllerDpGuid = DMG_CONTROLLER_DP_GUID;
STATIC CONST EFI_GUID mDmgSizeDpGuid       = DMG_SIZE_DP_GUID;

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

  DevPath->Controller.Header.Type    = HARDWARE_DEVICE_PATH;
  DevPath->Controller.Header.SubType = HW_VENDOR_DP;
  DevPath->Controller.Key            = 0;
  CopyMem (&DevPath->Controller.Guid, &mDmgControllerDpGuid, sizeof (DevPath->Controller.Guid));
  SetDevicePathNodeLength (&DevPath->Controller, sizeof (DevPath->Controller));

  DevPath->MemMap.Header.Type     = HARDWARE_DEVICE_PATH;
  DevPath->MemMap.Header.SubType  = HW_MEMMAP_DP;
  DevPath->MemMap.MemoryType      = EfiACPIMemoryNVS;
  DevPath->MemMap.StartingAddress = RamDmgAddress;
  DevPath->MemMap.EndingAddress   = (RamDmgAddress + sizeof (RAM_DMG_HEADER));
  SetDevicePathNodeLength (&DevPath->MemMap, sizeof (DevPath->MemMap));

  DevPath->FilePath.Header.Type = MEDIA_DEVICE_PATH;
  DevPath->FilePath.Header.Type = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (&DevPath->FilePath, sizeof (DevPath->FilePath));
  UnicodeSPrint (
    DevPath->FilePath.PathName,
    sizeof (DevPath->FilePath.PathName),
    L"DMG_%16X.dmg",
    DmgSize
    );

  DevPath->Size.Header.Type    = MESSAGING_DEVICE_PATH;
  DevPath->Size.Header.SubType = MSG_VENDOR_DP;
  DevPath->Size.Length         = DmgSize;
  CopyMem (&DevPath->Size.Guid, &mDmgSizeDpGuid, sizeof (DevPath->Size.Guid));
  SetDevicePathNodeLength (&DevPath->Size, sizeof (DevPath->Size));

  SetDevicePathEndNode (&DevPath->End);
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
  RAM_DMG_HEADER                   *RamDmgHeader;

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

  RamDmgHeader = (RAM_DMG_HEADER *)(UINTN)RamDmgAddress;
  ZeroMem (RamDmgHeader, sizeof (*RamDmgHeader));

  RamDmgHeader->Signature            = RAM_DMG_SIGNATURE;
  RamDmgHeader->Signature2           = RAM_DMG_SIGNATURE;
  RamDmgHeader->Version              = RAM_DMG_VERSION;
  RamDmgHeader->ExtentCount          = 1;
  RamDmgHeader->ExtentInfo[0].Start  = (UINT64)(UINTN)Context->Buffer;
  RamDmgHeader->ExtentInfo[0].Length = Context->Length;
  DEBUG ((
    DEBUG_INFO,
    "DMG extent @ 0x%lx, length 0x%lx\n",
    RamDmgHeader->ExtentInfo[0].Start,
    RamDmgHeader->ExtentInfo[0].Length
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
    gBS->UninstallMultipleProtocolInterfaces (
           BlockIoHandle,
           &gEfiBlockIoProtocolGuid,
           &DiskImageData->BlockIo,
           &gEfiDevicePathProtocolGuid,
           &DiskImageData->DevicePath,
           NULL
           );
    FreePool (DiskImageData);
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

BOOLEAN
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
    return FALSE;
  }

  Status = gBS->DisconnectController (BlockIoHandle, NULL, NULL);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  DiskImageData = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS (BlockIo);
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  BlockIoHandle,
                  &gEfiBlockIoProtocolGuid,
                  &DiskImageData->BlockIo,
                  &gEfiDevicePathProtocolGuid,
                  &DiskImageData->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  gBS->FreePages (
         (EFI_PHYSICAL_ADDRESS)(UINTN)DiskImageData->RamDmgHeader,
         EFI_SIZE_TO_PAGES (sizeof (*DiskImageData->RamDmgHeader))
         );

  FreePool (DiskImageData);

  return TRUE;
}

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

//
// Mounted disk image private data.
//
#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE SIGNATURE_32('D','m','g','I')
typedef struct {
    // Signature.
    UINTN Signature;

    // Protocols.
    EFI_BLOCK_IO_PROTOCOL BlockIo;
    EFI_BLOCK_IO_MEDIA    BlockIoMedia;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    EFI_HANDLE Handle;

    // Disk image data.
    OC_APPLE_DISK_IMAGE_CONTEXT *ImageContext;
    RAM_DMG_HEADER *RamDmgHeader;
} OC_APPLE_DISK_IMAGE_MOUNTED_DATA;
#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS(This) \
    CR(This, OC_APPLE_DISK_IMAGE_MOUNTED_DATA, BlockIo, OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE)

#define DMG_FILE_PATH_LEN  (L_STR_LEN (L"DMG_.dmg") + 16 + 1)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL Header;
  ///
  /// A NULL-terminated Path string including directory and file names.
  ///
  CHAR16                   PathName[DMG_FILE_PATH_LEN];
} DMG_FILEPATH_DEVICE_PATH;

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

EFI_STATUS
EFIAPI
OcAppleDiskImageInstallBlockIo(
    IN OC_APPLE_DISK_IMAGE_CONTEXT *Context) {

    // Create variables.
    EFI_STATUS Status;
    OC_APPLE_DISK_IMAGE_MOUNTED_DATA *DiskImageData;

    // RAM DMG.
    EFI_PHYSICAL_ADDRESS RamDmgPhysAddr;
    RAM_DMG_HEADER *RamDmgHeader = NULL;

    // Device path.
    EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL;
    EFI_DEVICE_PATH_PROTOCOL *DevicePathNew = NULL;
    DMG_CONTROLLER_DEVICE_PATH DevicePathDmgController;
    MEMMAP_DEVICE_PATH DevicePathMemMap;
    DMG_FILEPATH_DEVICE_PATH DevicePathFilePath;
    DMG_SIZE_DEVICE_PATH DevicePathDmgSize;

    // If a parameter is invalid, return error.
    if (!Context)
        return EFI_INVALID_PARAMETER;
    if (Context->BlockIoHandle)
        return EFI_ALREADY_STARTED;

    // Create DMG controller device node.
    DevicePathDmgController.Header.Type    = HARDWARE_DEVICE_PATH;
    DevicePathDmgController.Header.SubType = HW_VENDOR_DP;
    SetDevicePathNodeLength (&DevicePathDmgController, sizeof (DevicePathDmgController));

    DevicePathDmgController.Guid = gDmgControllerDpGuid;
    DevicePathDmgController.Key = 0;

    // Start device path with controller node.
    DevicePath = AppendDevicePathNode(NULL, &DevicePathDmgController.Header);
    if (!DevicePath) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Allocate page for RAM DMG header used by boot.efi.
    Status = gBS->AllocatePages(AllocateAnyPages, EfiACPIMemoryNVS, EFI_SIZE_TO_PAGES(sizeof(RAM_DMG_HEADER)), &RamDmgPhysAddr);
    if (EFI_ERROR(Status)) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }
    RamDmgHeader = (RAM_DMG_HEADER*)RamDmgPhysAddr;
    ZeroMem(RamDmgHeader, sizeof(RAM_DMG_HEADER));

    // Fill RAM DMG header.
    RamDmgHeader->Signature = RamDmgHeader->Signature2 = RAM_DMG_SIGNATURE;
    RamDmgHeader->Version = RAM_DMG_VERSION;
    RamDmgHeader->ExtentCount = 1;
    RamDmgHeader->ExtentInfo[0].Start = (UINT64)Context->Buffer;
    RamDmgHeader->ExtentInfo[0].Length = Context->Length;
    DEBUG((DEBUG_INFO, "DMG extent @ 0x%lx, length 0x%lx\n", RamDmgHeader->ExtentInfo[0].Start, RamDmgHeader->ExtentInfo[0].Length));

    // Allocate memmap node.
    DevicePathMemMap.Header.Type    = HARDWARE_DEVICE_PATH;
    DevicePathMemMap.Header.SubType = HW_MEMMAP_DP;
    SetDevicePathNodeLength (&DevicePathMemMap, sizeof (DevicePathMemMap));

    // Set memmap node properties.
    DevicePathMemMap.MemoryType = EfiACPIMemoryNVS;
    DevicePathMemMap.StartingAddress = RamDmgPhysAddr;
    DevicePathMemMap.EndingAddress = DevicePathMemMap.StartingAddress + sizeof(RAM_DMG_HEADER);

    // Add memmap node to device path.
    DevicePathNew = AppendDevicePathNode(DevicePath, &DevicePathMemMap.Header);
    if (!DevicePathNew) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Free original device path and use the new one.
    FreePool(DevicePath);
    DevicePath = DevicePathNew;
    DevicePathNew = NULL;

    // Allocate filepath node. Length is struct length (includes null terminator) and name length.
    DevicePathFilePath.Header.Type = MEDIA_DEVICE_PATH;
    DevicePathFilePath.Header.Type = MEDIA_FILEPATH_DP;
    SetDevicePathNodeLength (&DevicePathFilePath, sizeof (DevicePathFilePath));

    UnicodeSPrint (DevicePathFilePath.PathName, sizeof (DevicePathFilePath.PathName), L"DMG_%16X.dmg", Context->Length);

    // Add filepath node to device path.
    DevicePathNew = AppendDevicePathNode(DevicePath, &DevicePathFilePath.Header);
    if (!DevicePathNew) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Free original device path and use the new one.
    FreePool(DevicePath);
    DevicePath = DevicePathNew;
    DevicePathNew = NULL;

    // Allocate DMG size node.
    DevicePathDmgSize.Header.Type    = MESSAGING_DEVICE_PATH;
    DevicePathDmgSize.Header.SubType = MSG_VENDOR_DP;
    SetDevicePathNodeLength (&DevicePathDmgSize, sizeof (DevicePathDmgSize));

    DevicePathDmgSize.Guid = gDmgSizeDpGuid;
    DevicePathDmgSize.Length = Context->Length;

    // Add filepath node to device path.
    DevicePathNew = AppendDevicePathNode(DevicePath, &DevicePathDmgSize.Header);
    if (!DevicePathNew) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Free original device path and use the new one.
    FreePool(DevicePath);
    DevicePath = DevicePathNew;
    DevicePathNew = NULL;

    // Allocate installed DMG info.
    DiskImageData = AllocateZeroPool(sizeof(OC_APPLE_DISK_IMAGE_MOUNTED_DATA));
    if (!DiskImageData) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Fill disk image data.
    DiskImageData->Signature = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE;
    CopyMem(&(DiskImageData->BlockIo), &mDiskImageBlockIo, sizeof(EFI_BLOCK_IO_PROTOCOL));
    DiskImageData->DevicePath = DevicePath;
    DiskImageData->Handle = NULL;
    DiskImageData->ImageContext = Context;
    DiskImageData->RamDmgHeader = RamDmgHeader;

    // Allocate media info.
    DiskImageData->BlockIo.Media = &DiskImageData->BlockIoMedia;

    // Fill media info.
    DiskImageData->BlockIoMedia.MediaPresent = TRUE;
    DiskImageData->BlockIoMedia.ReadOnly = TRUE;
    DiskImageData->BlockIoMedia.BlockSize = APPLE_DISK_IMAGE_SECTOR_SIZE;
    DiskImageData->BlockIoMedia.LastBlock = Context->Trailer.SectorCount - 1;

    // Install protocols on child.
    Status = gBS->InstallMultipleProtocolInterfaces(&(DiskImageData->Handle),
        &gEfiBlockIoProtocolGuid, &DiskImageData->BlockIo,
        &gEfiDevicePathProtocolGuid, DiskImageData->DevicePath, NULL);
    if (EFI_ERROR(Status))
        goto DONE_ERROR;

    // Connect controller.
    Status = gBS->ConnectController(DiskImageData->Handle, NULL, NULL, TRUE);
    if (EFI_ERROR(Status))
        goto DONE_ERROR;

    // Success.
    Context->BlockIoHandle = DiskImageData->Handle;
    Status = EFI_SUCCESS;
    goto DONE;

DONE_ERROR:
    // Free data.
    if (DevicePath)
        FreePool(DevicePath);
    if (RamDmgHeader)
        gBS->FreePages(RamDmgPhysAddr, EFI_SIZE_TO_PAGES(sizeof(RAM_DMG_HEADER)));

DONE:
    return Status;
}

EFI_STATUS
EFIAPI
OcAppleDiskImageUninstallBlockIo (
  IN OC_APPLE_DISK_IMAGE_CONTEXT *Context
  )
{
  EFI_STATUS                        Status;
  EFI_BLOCK_IO_PROTOCOL             *BlockIo;
  OC_APPLE_DISK_IMAGE_MOUNTED_DATA  *DiskImageData;

  ASSERT (Context != NULL);
  if (Context->BlockIoHandle == NULL)
    return EFI_NOT_STARTED;

  //
  // Get block I/O.
  //
  Status = gBS->HandleProtocol (Context->BlockIoHandle, &gEfiBlockIoProtocolGuid, (VOID**)&BlockIo);
  if (EFI_ERROR (Status))
    return Status;

  //
  // Get disk image data.
  //
  DiskImageData = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS (BlockIo);

  //
  // Ensure context matches.
  //
  if (DiskImageData->Handle != Context->BlockIoHandle || DiskImageData->ImageContext != Context)
    return EFI_INVALID_PARAMETER;

  //
  // Disconnect controller.
  //
  Status = gBS->DisconnectController (DiskImageData->Handle, NULL, NULL);
  if (EFI_ERROR (Status))
    return Status;

  //
  // Uninstall protocols.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (DiskImageData->Handle,
    &gEfiBlockIoProtocolGuid, &(DiskImageData->BlockIo),
    &gEfiDevicePathProtocolGuid, DiskImageData->DevicePath, NULL);
  if (EFI_ERROR (Status))
    return Status;
  Context->BlockIoHandle = NULL;

  // Free device path and RAM DMG header pages.
  if (DiskImageData->DevicePath != NULL)
    FreePool (DiskImageData->DevicePath);
  if (DiskImageData->RamDmgHeader != NULL)
    FreePages (DiskImageData->RamDmgHeader, EFI_SIZE_TO_PAGES (sizeof(RAM_DMG_HEADER)));

  //
  // Free disk image data.
  //
  FreePool (DiskImageData);
  return EFI_SUCCESS;
}

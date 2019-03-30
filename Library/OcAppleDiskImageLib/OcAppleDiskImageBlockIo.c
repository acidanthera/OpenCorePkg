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

#define DMG_FILE_PATH_LEN  (L_STR_LEN (L"DMG_.dmg") + 16 + 1)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL Header;
  ///
  /// A NULL-terminated Path string including directory and file names.
  ///
  CHAR16                   PathName[DMG_FILE_PATH_LEN];
} DMG_FILEPATH_DEVICE_PATH;

typedef struct {
  DMG_CONTROLLER_DEVICE_PATH Controller;
  MEMMAP_DEVICE_PATH         MemMap;
  DMG_FILEPATH_DEVICE_PATH   FilePath;
  DMG_SIZE_DEVICE_PATH       Size;
  EFI_DEVICE_PATH_PROTOCOL   End;
} DMG_DEVICE_PATH;

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
    DMG_DEVICE_PATH       DevicePath;
    EFI_HANDLE Handle;

    // Disk image data.
    OC_APPLE_DISK_IMAGE_CONTEXT *ImageContext;
    RAM_DMG_HEADER *RamDmgHeader;
} OC_APPLE_DISK_IMAGE_MOUNTED_DATA;
#define OC_APPLE_DISK_IMAGE_MOUNTED_DATA_FROM_THIS(This) \
    CR(This, OC_APPLE_DISK_IMAGE_MOUNTED_DATA, BlockIo, OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE)

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

EFI_STATUS
EFIAPI
OcAppleDiskImageInstallBlockIo(
    IN OC_APPLE_DISK_IMAGE_CONTEXT *Context) {

    // Create variables.
    EFI_STATUS Status;
    OC_APPLE_DISK_IMAGE_MOUNTED_DATA *DiskImageData = NULL;

    // RAM DMG.
    EFI_PHYSICAL_ADDRESS RamDmgPhysAddr;
    RAM_DMG_HEADER *RamDmgHeader = NULL;

    // Device path.
    DMG_DEVICE_PATH *DevicePath = NULL;

    // If a parameter is invalid, return error.
    if (!Context)
        return EFI_INVALID_PARAMETER;
    if (Context->BlockIoHandle)
        return EFI_ALREADY_STARTED;

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

    // Allocate installed DMG info.
    DiskImageData = AllocateZeroPool(sizeof(OC_APPLE_DISK_IMAGE_MOUNTED_DATA));
    if (!DiskImageData) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Fill disk image data.
    DiskImageData->Signature = OC_APPLE_DISK_IMAGE_MOUNTED_DATA_SIGNATURE;
    CopyMem(&(DiskImageData->BlockIo), &mDiskImageBlockIo, sizeof(EFI_BLOCK_IO_PROTOCOL));
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

    DevicePath = &DiskImageData->DevicePath;

    // Create DMG controller device node.
    DevicePath->Controller.Header.Type    = HARDWARE_DEVICE_PATH;
    DevicePath->Controller.Header.SubType = HW_VENDOR_DP;
    SetDevicePathNodeLength (&DevicePath->Controller, sizeof (DevicePath->Controller));

    DevicePath->Controller.Guid = gDmgControllerDpGuid;
    DevicePath->Controller.Key = 0;

    // Allocate memmap node.
    DevicePath->MemMap.Header.Type    = HARDWARE_DEVICE_PATH;
    DevicePath->MemMap.Header.SubType = HW_MEMMAP_DP;
    SetDevicePathNodeLength (&DevicePath->MemMap, sizeof (DevicePath->MemMap));

    // Set memmap node properties.
    DevicePath->MemMap.MemoryType = EfiACPIMemoryNVS;
    DevicePath->MemMap.StartingAddress = RamDmgPhysAddr;
    DevicePath->MemMap.EndingAddress = DevicePath->MemMap.StartingAddress + sizeof(RAM_DMG_HEADER);

    // Allocate filepath node. Length is struct length (includes null terminator) and name length.
    DevicePath->FilePath.Header.Type = MEDIA_DEVICE_PATH;
    DevicePath->FilePath.Header.Type = MEDIA_FILEPATH_DP;
    SetDevicePathNodeLength (&DevicePath->FilePath, sizeof (DevicePath->FilePath));

    UnicodeSPrint (DevicePath->FilePath.PathName, sizeof (DevicePath->FilePath.PathName), L"DMG_%16X.dmg", Context->Length);

    // Allocate DMG size node.
    DevicePath->Size.Header.Type    = MESSAGING_DEVICE_PATH;
    DevicePath->Size.Header.SubType = MSG_VENDOR_DP;
    SetDevicePathNodeLength (&DevicePath->Size, sizeof (DevicePath->Size));

    DevicePath->Size.Guid = gDmgSizeDpGuid;
    DevicePath->Size.Length = Context->Length;

    SetDevicePathEndNode (&DevicePath->End);

    // Install protocols on child.
    Status = gBS->InstallMultipleProtocolInterfaces(&(DiskImageData->Handle),
        &gEfiBlockIoProtocolGuid, &DiskImageData->BlockIo,
        &gEfiDevicePathProtocolGuid, &DiskImageData->DevicePath, NULL);
    if (EFI_ERROR(Status))
        goto DONE_ERROR;

    // Connect controller.
    Status = gBS->ConnectController(DiskImageData->Handle, NULL, NULL, TRUE);
    if (EFI_ERROR(Status))
        goto DONE_ERROR;

    // Success.
    Context->BlockIoHandle = DiskImageData->Handle;
    return EFI_SUCCESS;

DONE_ERROR:
    // Free data.
    if (DiskImageData)
        FreePool(DiskImageData);
    if (RamDmgHeader)
        gBS->FreePages(RamDmgPhysAddr, EFI_SIZE_TO_PAGES(sizeof(RAM_DMG_HEADER)));

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
    &gEfiDevicePathProtocolGuid, &DiskImageData->DevicePath, NULL);
  if (EFI_ERROR (Status))
    return Status;
  Context->BlockIoHandle = NULL;

  // Free RAM DMG header pages.
  if (DiskImageData->RamDmgHeader != NULL)
    FreePages (DiskImageData->RamDmgHeader, EFI_SIZE_TO_PAGES (sizeof(RAM_DMG_HEADER)));

  //
  // Free disk image data.
  //
  FreePool (DiskImageData);
  return EFI_SUCCESS;
}

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
#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OcAppleDiskImageLibInternal.h"
#include "zlib/zlib.h"

EFI_STATUS
EFIAPI
OcAppleDiskImageInitializeContext (
  IN  VOID                          *Buffer,
  IN  UINTN                         BufferLength,
  OUT OC_APPLE_DISK_IMAGE_CONTEXT   **Context
  )
{
  EFI_STATUS                          Status;
  OC_APPLE_DISK_IMAGE_CONTEXT         *DmgContext = NULL;
  UINTN                               DmgLength;
  UINT8                               *BufferBytes = NULL;
  UINT8                               *BufferBytesCurrent = NULL;
  APPLE_DISK_IMAGE_TRAILER            *BufferTrailer;
  APPLE_DISK_IMAGE_TRAILER            Trailer;
  UINT32                              DmgBlockCount;
  OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT   *DmgBlocks = NULL;

  ASSERT (Buffer != NULL);
  ASSERT (BufferLength > sizeof (APPLE_DISK_IMAGE_TRAILER));
  ASSERT (Context != NULL);

  //
  // Look for trailer signature.
  //
  BufferBytes = (UINT8*)Buffer;
  BufferBytesCurrent = BufferBytes + BufferLength - sizeof (UINT32);
  BufferTrailer = NULL;
  while (BufferBytesCurrent >= BufferBytes) {
    // Check for trailer signature.
    if (*((UINT32*)BufferBytesCurrent) == SwapBytes32 (APPLE_DISK_IMAGE_MAGIC)) {
      BufferTrailer = (APPLE_DISK_IMAGE_TRAILER*)BufferBytesCurrent;
      DmgLength = BufferBytesCurrent - BufferBytes + sizeof (APPLE_DISK_IMAGE_TRAILER);
      break;
    }

    // Move to previous byte.
    BufferBytesCurrent--;
  }

  //
  // If trailer not found, fail.
  //
  if (BufferTrailer == NULL)
      return EFI_UNSUPPORTED;

  //
  // Get trailer.
  //
  CopyMem (&Trailer, BufferTrailer, sizeof (APPLE_DISK_IMAGE_TRAILER));
  Trailer.Signature = SwapBytes32 (Trailer.Signature);
  Trailer.Version = SwapBytes32 (Trailer.Version);
  Trailer.HeaderSize = SwapBytes32 (Trailer.HeaderSize);
  Trailer.Flags = SwapBytes32 (Trailer.Flags);

  //
  // Ensure signature and size are valid.
  //
  if (Trailer.Signature != APPLE_DISK_IMAGE_MAGIC ||
      Trailer.HeaderSize != sizeof (APPLE_DISK_IMAGE_TRAILER)) {
      Status = EFI_UNSUPPORTED;
      goto DONE_ERROR;
  }

  // Swap main fields.
  Trailer.RunningDataForkOffset = SwapBytes64 (Trailer.RunningDataForkOffset);
  Trailer.DataForkOffset = SwapBytes64 (Trailer.DataForkOffset);
  Trailer.DataForkLength = SwapBytes64 (Trailer.DataForkLength);
  Trailer.RsrcForkOffset = SwapBytes64 (Trailer.RsrcForkOffset);
  Trailer.RsrcForkLength = SwapBytes64 (Trailer.RsrcForkLength);
  Trailer.SegmentNumber = SwapBytes32 (Trailer.SegmentNumber);
  Trailer.SegmentCount = SwapBytes32 (Trailer.SegmentCount);

  // Swap data fork checksum.
  Trailer.DataForkChecksum.Type = SwapBytes32 (Trailer.DataForkChecksum.Type);
  Trailer.DataForkChecksum.Size = SwapBytes32 (Trailer.DataForkChecksum.Size);
  for (UINTN i = 0; i < APPLE_DISK_IMAGE_CHECKSUM_SIZE; i++)
    Trailer.DataForkChecksum.Data[i] = SwapBytes32 (Trailer.DataForkChecksum.Data[i]);

  // Swap XML info.
  Trailer.XmlOffset = SwapBytes64 (Trailer.XmlOffset);
  Trailer.XmlLength = SwapBytes64 (Trailer.XmlLength);

  // Swap main checksum.
  Trailer.Checksum.Type = SwapBytes32 (Trailer.Checksum.Type);
  Trailer.Checksum.Size = SwapBytes32 (Trailer.Checksum.Size);
  for (UINTN i = 0; i < APPLE_DISK_IMAGE_CHECKSUM_SIZE; i++)
    Trailer.Checksum.Data[i] = SwapBytes32 (Trailer.Checksum.Data[i]);

  // Swap addition fields.
  Trailer.ImageVariant = SwapBytes32 (Trailer.ImageVariant);
  Trailer.SectorCount = SwapBytes64 (Trailer.SectorCount);

  // If data fork checksum is CRC32, verify it.
  if (Trailer.DataForkChecksum.Type == APPLE_DISK_IMAGE_CHECKSUM_TYPE_CRC32) {
    Status = VerifyCrc32 (((UINT8*)Buffer) + Trailer.DataForkOffset,
      Trailer.DataForkLength, Trailer.DataForkChecksum.Data[0]);
    if (EFI_ERROR (Status))
        goto DONE_ERROR;
  }

  //
  // Ensure XML offset/length is valid and in range.
  //
  if (Trailer.XmlOffset == 0 || Trailer.XmlOffset >= (DmgLength - sizeof (APPLE_DISK_IMAGE_TRAILER)) ||
    Trailer.XmlLength == 0 || (Trailer.XmlOffset + Trailer.XmlLength) > (DmgLength - sizeof (APPLE_DISK_IMAGE_TRAILER))) {
    Status = EFI_UNSUPPORTED;
    goto DONE_ERROR;
  }

  //
  // Parse XML.
  //
  Status = ParsePlist (Buffer, Trailer.XmlOffset, Trailer.XmlLength, &DmgBlockCount, &DmgBlocks);
  if (EFI_ERROR(Status))
    goto DONE_ERROR;

  //
  // Allocate DMG file structure.
  //
  DmgContext = AllocateZeroPool (sizeof (OC_APPLE_DISK_IMAGE_CONTEXT));
  if (!DmgContext) {
    Status = EFI_OUT_OF_RESOURCES;
    goto DONE_ERROR;
  }

  // Fill DMG file structure.
  DmgContext->Buffer = Buffer;
  DmgContext->Length = DmgLength;
  CopyMem (&(DmgContext->Trailer), &Trailer, sizeof (APPLE_DISK_IMAGE_TRAILER));
  DmgContext->BlockCount = DmgBlockCount;
  DmgContext->Blocks = DmgBlocks;

  *Context = DmgContext;
  Status = EFI_SUCCESS;
  goto DONE;

DONE_ERROR:
  if (DmgBlocks != NULL)
    FreePool (DmgBlocks);

DONE:
  return Status;
}

EFI_STATUS
EFIAPI
OcAppleDiskImageFreeContext (
  IN OC_APPLE_DISK_IMAGE_CONTEXT  *Context
  )
{
  UINT64                              Index;
  OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT   *CurrentBlockContext;

  ASSERT (Context != NULL);

  // Free blocks.
  if (Context->Blocks) {
    for (Index = 0; Index < Context->BlockCount; Index++) {
      // Get block.
      CurrentBlockContext = Context->Blocks + Index;

      // Free block data.
      if (CurrentBlockContext->CfName != NULL)
        FreePool (CurrentBlockContext->CfName);
      if (CurrentBlockContext->Name != NULL)
        FreePool (CurrentBlockContext->Name);
      if (CurrentBlockContext->BlockData != NULL)
        FreePool(CurrentBlockContext->BlockData);
    }
    FreePool (Context->Blocks);
  }

  FreePool (Context);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcAppleDiskImageRead(
    IN  OC_APPLE_DISK_IMAGE_CONTEXT *Context,
    IN  EFI_LBA Lba,
    IN  UINTN BufferSize,
    OUT VOID *Buffer) {

    // Create variables.
    EFI_STATUS Status;

    // Chunk to read.
    APPLE_DISK_IMAGE_BLOCK_DATA *BlockData;
    APPLE_DISK_IMAGE_CHUNK *Chunk;
    UINTN ChunkTotalLength;
    UINTN ChunkLength;
    UINTN ChunkOffset;
    UINT8 *ChunkData;
    UINT8 *ChunkDataCurrent;

    // Buffer.
    EFI_LBA LbaCurrent;
    EFI_LBA LbaOffset;
    EFI_LBA LbaLength;
    UINTN RemainingBufferSize;
    UINTN BufferChunkSize;
    UINT8 *BufferCurrent;

    // zlib data.
    z_stream ZlibStream;
    INT32 ZlibStatus;

    // Check if parameters are valid.
    if (!Context || !Buffer)
        return EFI_INVALID_PARAMETER;

    // Check that sector is in range.
    if (Lba >= Context->Trailer.SectorCount)
        return EFI_INVALID_PARAMETER;

    // Read blocks.
    LbaCurrent = Lba;
    RemainingBufferSize = BufferSize;
    BufferCurrent = Buffer;
    while (RemainingBufferSize) {
        // Determine block in DMG.
        Status = GetBlockChunk(Context, LbaCurrent, &BlockData, &Chunk);
        if (EFI_ERROR(Status)) {
            Status = EFI_DEVICE_ERROR;
            goto DONE;
        }

        // Determine offset into source DMG.
        LbaOffset = LbaCurrent - DMG_SECTOR_START_ABS(BlockData, Chunk);
        LbaLength = Chunk->SectorCount - LbaOffset;
        ChunkOffset = LbaOffset * APPLE_DISK_IMAGE_SECTOR_SIZE;
        ChunkTotalLength = (UINTN)Chunk->SectorCount * APPLE_DISK_IMAGE_SECTOR_SIZE;
        ChunkLength = ChunkTotalLength - ChunkOffset;

        // If the buffer size is bigger than the chunk, there will be more chunks to get.
        BufferChunkSize = RemainingBufferSize;
        if (BufferChunkSize > ChunkLength)
            BufferChunkSize = ChunkLength;

        // Determine type.
        switch(Chunk->Type) {
            // No data, write zeroes.
            case APPLE_DISK_IMAGE_CHUNK_TYPE_ZERO:
            case APPLE_DISK_IMAGE_CHUNK_TYPE_IGNORE:
                // Zero destination buffer.
                ZeroMem(BufferCurrent, BufferChunkSize);
                break;

            // Raw data, write data as-is.
            case APPLE_DISK_IMAGE_CHUNK_TYPE_RAW:
                // Determine pointer to source data.
                ChunkData = ((UINT8 *)Context->Buffer + BlockData->DataOffset + Chunk->CompressedOffset);
                ChunkDataCurrent = ChunkData + ChunkOffset;

                // Copy to destination buffer.
                CopyMem(BufferCurrent, ChunkDataCurrent, BufferChunkSize);
                ChunkData = ChunkDataCurrent = NULL;
                break;

            // zlib-compressed data, inflate and write uncompressed data.
            case APPLE_DISK_IMAGE_CHUNK_TYPE_ZLIB:
                // Allocate buffer for inflated data.
                ChunkData = AllocateZeroPool(ChunkTotalLength);
                ChunkDataCurrent = ChunkData + ChunkOffset;

                // Initialize zlib stream.
                ZeroMem(&ZlibStream, sizeof(z_stream));
                ZlibStatus = inflateInit(&ZlibStream);
                if (ZlibStatus != Z_OK) {
                    Status = EFI_DEVICE_ERROR;
                    goto DONE;
                }

                // Set stream parameters.
                ZlibStream.avail_in = (UINT32)Chunk->CompressedLength;
                ZlibStream.next_in = ((Bytef*)Context->Buffer + BlockData->DataOffset + Chunk->CompressedOffset);
                ZlibStream.avail_out = (UINT32)ChunkTotalLength;
                ZlibStream.next_out = (Bytef*)ChunkData;

                // Inflate chunk and close stream.
                ZlibStatus = inflate(&ZlibStream, Z_NO_FLUSH);
                inflateEnd(&ZlibStream);

                // If inflation reported an error, fail.
                if (!((ZlibStatus == Z_OK) || (ZlibStatus == Z_STREAM_END))) {
                    FreePool(ChunkData);
                    Status = EFI_DEVICE_ERROR;
                    goto DONE;
                }

                // Copy to destination buffer.
                CopyMem(BufferCurrent, ChunkDataCurrent, BufferChunkSize);
                FreePool(ChunkData);
                ChunkData = ChunkDataCurrent = NULL;
                break;

            // Unknown chunk type.
            default:
                Status = EFI_DEVICE_ERROR;
                goto DONE;
        }

        // Move to next chunk.
        RemainingBufferSize -= BufferChunkSize;
        BufferCurrent += BufferChunkSize;
        LbaCurrent += LbaLength;
    }

    // Success.
    Status = EFI_SUCCESS;

DONE:
    ASSERT_EFI_ERROR(Status);
    return Status;
}

#define DMG_FILE_PATH_LEN  (L_STR_LEN (L"DMG_.dmg") + 16 + 1)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL Header;
  ///
  /// A NULL-terminated Path string including directory and file names.
  ///
  CHAR16                   PathName[DMG_FILE_PATH_LEN];
} DMG_FILEPATH_DEVICE_PATH;

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
    DiskImageData->BlockIo.Media = AllocateZeroPool(sizeof(EFI_BLOCK_IO_MEDIA));
    if (!DiskImageData->BlockIo.Media) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Fill media info.
    DiskImageData->BlockIo.Media->MediaPresent = TRUE;
    DiskImageData->BlockIo.Media->ReadOnly = TRUE;
    DiskImageData->BlockIo.Media->BlockSize = APPLE_DISK_IMAGE_SECTOR_SIZE;
    DiskImageData->BlockIo.Media->LastBlock = Context->Trailer.SectorCount - 1;

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

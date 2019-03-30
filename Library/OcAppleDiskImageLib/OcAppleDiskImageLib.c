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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcCompressionLib.h>

#include "OcAppleDiskImageLibInternal.h"

STATIC
VOID
InternalSwapTrailerData (
  IN OUT APPLE_DISK_IMAGE_TRAILER  *Trailer
  )
{
  ASSERT (Trailer != NULL);

  Trailer->Signature = SwapBytes32 (Trailer->Signature);
  Trailer->Version = SwapBytes32 (Trailer->Version);
  Trailer->HeaderSize = SwapBytes32 (Trailer->HeaderSize);
  Trailer->Flags = SwapBytes32 (Trailer->Flags);

  // Swap main fields.
  Trailer->RunningDataForkOffset = SwapBytes64 (Trailer->RunningDataForkOffset);
  Trailer->DataForkOffset = SwapBytes64 (Trailer->DataForkOffset);
  Trailer->DataForkLength = SwapBytes64 (Trailer->DataForkLength);
  Trailer->RsrcForkOffset = SwapBytes64 (Trailer->RsrcForkOffset);
  Trailer->RsrcForkLength = SwapBytes64 (Trailer->RsrcForkLength);
  Trailer->SegmentNumber = SwapBytes32 (Trailer->SegmentNumber);
  Trailer->SegmentCount = SwapBytes32 (Trailer->SegmentCount);

  // Swap data fork checksum.
  Trailer->DataForkChecksum.Type = SwapBytes32 (Trailer->DataForkChecksum.Type);
  Trailer->DataForkChecksum.Size = SwapBytes32 (Trailer->DataForkChecksum.Size);
  for (UINTN i = 0; i < APPLE_DISK_IMAGE_CHECKSUM_SIZE; i++)
    Trailer->DataForkChecksum.Data[i] = SwapBytes32 (Trailer->DataForkChecksum.Data[i]);

  // Swap XML info.
  Trailer->XmlOffset = SwapBytes64 (Trailer->XmlOffset);
  Trailer->XmlLength = SwapBytes64 (Trailer->XmlLength);

  // Swap main checksum.
  Trailer->Checksum.Type = SwapBytes32 (Trailer->Checksum.Type);
  Trailer->Checksum.Size = SwapBytes32 (Trailer->Checksum.Size);
  for (UINTN i = 0; i < APPLE_DISK_IMAGE_CHECKSUM_SIZE; i++)
    Trailer->Checksum.Data[i] = SwapBytes32 (Trailer->Checksum.Data[i]);

  // Swap addition fields.
  Trailer->ImageVariant = SwapBytes32 (Trailer->ImageVariant);
  Trailer->SectorCount = SwapBytes64 (Trailer->SectorCount);
}

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
  UINT32                              Crc32;

  ASSERT (Buffer != NULL);
  ASSERT (BufferLength > 0);
  ASSERT (Context != NULL);

  if (BufferLength <= sizeof (APPLE_DISK_IMAGE_TRAILER)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Look for trailer signature.
  //
  BufferBytes = (UINT8*)Buffer;
  BufferBytesCurrent = BufferBytes + BufferLength - sizeof (APPLE_DISK_IMAGE_TRAILER);
  BufferTrailer = NULL;
  while (BufferBytesCurrent >= BufferBytes) {
    // Check for trailer signature.
    if (ReadUnaligned32 ((UINT32*)BufferBytesCurrent) == SwapBytes32 (APPLE_DISK_IMAGE_MAGIC)) {
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
  InternalSwapTrailerData (&Trailer);

  //
  // Ensure signature and size are valid.
  //
  if (Trailer.Signature != APPLE_DISK_IMAGE_MAGIC ||
      Trailer.HeaderSize != sizeof (APPLE_DISK_IMAGE_TRAILER)) {
      return EFI_UNSUPPORTED;
  }

  // If data fork checksum is CRC32, verify it.
  if (Trailer.DataForkChecksum.Type == APPLE_DISK_IMAGE_CHECKSUM_TYPE_CRC32) {
    Crc32 = CalculateCrc32 (
              ((UINT8*)Buffer + Trailer.DataForkOffset),
              Trailer.DataForkLength
              );
    if (Crc32 != Trailer.DataForkChecksum.Data[0])
        return EFI_COMPROMISED_DATA;
  }

  //
  // Ensure XML offset/length is valid and in range.
  //
  if (Trailer.XmlOffset == 0 || Trailer.XmlOffset >= (DmgLength - sizeof (APPLE_DISK_IMAGE_TRAILER)) ||
    Trailer.XmlLength == 0 || (Trailer.XmlOffset + Trailer.XmlLength) > (DmgLength - sizeof (APPLE_DISK_IMAGE_TRAILER))) {
    return EFI_UNSUPPORTED;
  }

  //
  // Parse XML.
  //
  Status = ParsePlist (Buffer, Trailer.XmlOffset, Trailer.XmlLength, &DmgBlockCount, &DmgBlocks);
  if (EFI_ERROR(Status))
    return Status;

  //
  // Allocate DMG file structure.
  //
  DmgContext = AllocateZeroPool (sizeof (OC_APPLE_DISK_IMAGE_CONTEXT));
  if (!DmgContext) {
    FreePool (DmgBlocks);
    return EFI_OUT_OF_RESOURCES;
  }

  // Fill DMG file structure.
  DmgContext->Buffer = Buffer;
  DmgContext->Length = DmgLength;
  CopyMem (&(DmgContext->Trailer), &Trailer, sizeof (APPLE_DISK_IMAGE_TRAILER));
  DmgContext->BlockCount = DmgBlockCount;
  DmgContext->Blocks = DmgBlocks;

  *Context = DmgContext;
  return EFI_SUCCESS;
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
    UINTN OutSize;

    ASSERT (Context != NULL);
    ASSERT (Buffer != NULL);
    ASSERT (Lba < Context->Trailer.SectorCount);

    // Read blocks.
    LbaCurrent = Lba;
    RemainingBufferSize = BufferSize;
    BufferCurrent = Buffer;
    while (RemainingBufferSize) {
        // Determine block in DMG.
        Status = GetBlockChunk(Context, LbaCurrent, &BlockData, &Chunk);
        if (EFI_ERROR(Status)) {
            return EFI_DEVICE_ERROR;
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
                ChunkData = (Context->Buffer + BlockData->DataOffset + Chunk->CompressedOffset);
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
                OutSize = DecompressZLIB (
                            ChunkData,
                            ChunkTotalLength,
                            (Context->Buffer + BlockData->DataOffset + Chunk->CompressedOffset),
                            Chunk->CompressedLength
                            );
                if (OutSize != ChunkTotalLength) {
                  FreePool (ChunkData);
                  return EFI_DEVICE_ERROR;
                }

                // Copy to destination buffer.
                CopyMem(BufferCurrent, ChunkDataCurrent, BufferChunkSize);
                FreePool(ChunkData);
                ChunkData = ChunkDataCurrent = NULL;
                break;

            // Unknown chunk type.
            default:
                return EFI_DEVICE_ERROR;
        }

        // Move to next chunk.
        RemainingBufferSize -= BufferChunkSize;
        BufferCurrent += BufferChunkSize;
        LbaCurrent += LbaLength;
    }

    return EFI_SUCCESS;
}

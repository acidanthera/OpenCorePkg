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
#include <Library/OcCompressionLib.h>

#include "OcAppleDiskImageLibInternal.h"

STATIC
VOID
InternalSwapTrailerData (
  IN OUT APPLE_DISK_IMAGE_TRAILER  *Trailer
  )
{
  UINTN Index;

  ASSERT (Trailer != NULL);

  Trailer->Signature  = SwapBytes32 (Trailer->Signature);
  Trailer->Version    = SwapBytes32 (Trailer->Version);
  Trailer->HeaderSize = SwapBytes32 (Trailer->HeaderSize);
  Trailer->Flags      = SwapBytes32 (Trailer->Flags);

  Trailer->RunningDataForkOffset = SwapBytes64 (Trailer->RunningDataForkOffset);
  Trailer->DataForkOffset        = SwapBytes64 (Trailer->DataForkOffset);
  Trailer->DataForkLength        = SwapBytes64 (Trailer->DataForkLength);
  Trailer->RsrcForkOffset        = SwapBytes64 (Trailer->RsrcForkOffset);
  Trailer->RsrcForkLength        = SwapBytes64 (Trailer->RsrcForkLength);
  Trailer->SegmentNumber         = SwapBytes32 (Trailer->SegmentNumber);
  Trailer->SegmentCount          = SwapBytes32 (Trailer->SegmentCount);

  Trailer->DataForkChecksum.Type = SwapBytes32 (Trailer->DataForkChecksum.Type);
  Trailer->DataForkChecksum.Size = SwapBytes32 (Trailer->DataForkChecksum.Size);
  for (Index = 0; Index < APPLE_DISK_IMAGE_CHECKSUM_SIZE; ++Index) {
    Trailer->DataForkChecksum.Data[Index] = SwapBytes32 (
                                              Trailer->DataForkChecksum.Data[Index]
                                              );
  }

  Trailer->XmlOffset = SwapBytes64 (Trailer->XmlOffset);
  Trailer->XmlLength = SwapBytes64 (Trailer->XmlLength);

  Trailer->Checksum.Type = SwapBytes32 (Trailer->Checksum.Type);
  Trailer->Checksum.Size = SwapBytes32 (Trailer->Checksum.Size);
  for (Index = 0; Index < APPLE_DISK_IMAGE_CHECKSUM_SIZE; ++Index) {
    Trailer->Checksum.Data[Index] = SwapBytes32 (Trailer->Checksum.Data[Index]);
  }

  Trailer->ImageVariant = SwapBytes32 (Trailer->ImageVariant);
  Trailer->SectorCount  = SwapBytes64 (Trailer->SectorCount);
}

EFI_STATUS
EFIAPI
OcAppleDiskImageInitializeContext (
  IN  VOID                         *Buffer,
  IN  UINTN                        BufferLength,
  OUT OC_APPLE_DISK_IMAGE_CONTEXT  **Context
  )
{
  EFI_STATUS                        Status;
  OC_APPLE_DISK_IMAGE_CONTEXT       *DmgContext;
  UINTN                             TrailerOffset;
  UINT8                             *BufferBytes;
  UINT8                             *BufferBytesCurrent;
  APPLE_DISK_IMAGE_TRAILER          *BufferTrailer;
  APPLE_DISK_IMAGE_TRAILER          Trailer;
  UINT32                            DmgBlockCount;
  OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT *DmgBlocks;
  UINT32                            Crc32;
  UINT32                            SwappedSig;

  ASSERT (Buffer != NULL);
  ASSERT (BufferLength > 0);
  ASSERT (Context != NULL);

  if (BufferLength <= sizeof (APPLE_DISK_IMAGE_TRAILER)) {
    return EFI_UNSUPPORTED;
  }

  SwappedSig = SwapBytes32 (APPLE_DISK_IMAGE_MAGIC);

  BufferBytes   = (UINT8*)Buffer;
  BufferTrailer = NULL;

  for (
    BufferBytesCurrent = (BufferBytes + (BufferLength - sizeof (APPLE_DISK_IMAGE_TRAILER)));
    BufferBytesCurrent >= BufferBytes;
    --BufferBytesCurrent
    ) {
    if (ReadUnaligned32 ((UINT32 *)BufferBytesCurrent) == SwappedSig) {
      BufferTrailer = (APPLE_DISK_IMAGE_TRAILER *)BufferBytesCurrent;
      TrailerOffset = (BufferBytesCurrent - BufferBytes);
      break;
    }
  }

  if (BufferTrailer == NULL) {
    return EFI_UNSUPPORTED;
  }

  CopyMem (&Trailer, BufferTrailer, sizeof (Trailer));
  InternalSwapTrailerData (&Trailer);

  if (Trailer.HeaderSize != sizeof (APPLE_DISK_IMAGE_TRAILER)
   || (Trailer.XmlOffset == 0)
   || (Trailer.XmlOffset >= TrailerOffset)
   || (Trailer.XmlLength == 0)
   || ((Trailer.XmlOffset + Trailer.XmlLength) > TrailerOffset)) {
    return EFI_UNSUPPORTED;
  }

  if (Trailer.DataForkChecksum.Type == APPLE_DISK_IMAGE_CHECKSUM_TYPE_CRC32) {
    Crc32 = CalculateCrc32 (
              (BufferBytes + Trailer.DataForkOffset),
              Trailer.DataForkLength
              );
    if (Crc32 != Trailer.DataForkChecksum.Data[0]) {
      return EFI_COMPROMISED_DATA;
    }
  }

  Status = InternalParsePlist (
             Buffer,
             (UINT32)Trailer.XmlOffset,
             (UINT32)Trailer.XmlLength,
             &DmgBlockCount,
             &DmgBlocks
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DmgContext = AllocateZeroPool (sizeof (*DmgContext));
  if (DmgContext == NULL) {
    FreePool (DmgBlocks);
    return EFI_OUT_OF_RESOURCES;
  }

  DmgContext->Buffer     = BufferBytes;
  DmgContext->Length     = (TrailerOffset + sizeof (APPLE_DISK_IMAGE_TRAILER));
  DmgContext->BlockCount = DmgBlockCount;
  DmgContext->Blocks     = DmgBlocks;
  CopyMem (&DmgContext->Trailer, &Trailer, sizeof (DmgContext->Trailer));

  *Context = DmgContext;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcAppleDiskImageFreeContext (
  IN OC_APPLE_DISK_IMAGE_CONTEXT  *Context
  )
{
  UINT64                            Index;
  OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT *CurrentBlockContext;

  ASSERT (Context != NULL);

  for (Index = 0; Index < Context->BlockCount; ++Index) {
    CurrentBlockContext = &Context->Blocks[Index];

    FreePool (CurrentBlockContext->CfName);
    FreePool (CurrentBlockContext->Name);
    FreePool (CurrentBlockContext->BlockData);
  }

  FreePool (Context->Blocks);
  FreePool (Context);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcAppleDiskImageRead (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  EFI_LBA                      Lba,
  IN  UINTN                        BufferSize,
  OUT VOID                         *Buffer
  )
{
  EFI_STATUS                  Status;

  APPLE_DISK_IMAGE_BLOCK_DATA *BlockData;
  APPLE_DISK_IMAGE_CHUNK      *Chunk;
  UINT64                      ChunkTotalLength;
  UINT64                      ChunkLength;
  UINT64                      ChunkOffset;
  UINT8                       *ChunkData;
  UINT8                       *ChunkDataCurrent;

  EFI_LBA                     LbaCurrent;
  EFI_LBA                     LbaOffset;
  EFI_LBA                     LbaLength;
  UINTN                       RemainingBufferSize;
  UINTN                       BufferChunkSize;
  UINT8                       *BufferCurrent;

  UINTN                       OutSize;

  ASSERT (Context != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (Lba < Context->Trailer.SectorCount);

  LbaCurrent          = Lba;
  RemainingBufferSize = BufferSize;
  BufferCurrent       = Buffer;

  while (RemainingBufferSize > 0) {
      Status = InternalGetBlockChunk (Context, LbaCurrent, &BlockData, &Chunk);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      LbaOffset        = (LbaCurrent - DMG_SECTOR_START_ABS (BlockData, Chunk));
      LbaLength        = (Chunk->SectorCount - LbaOffset);
      ChunkOffset      = (LbaOffset * APPLE_DISK_IMAGE_SECTOR_SIZE);
      ChunkTotalLength = (Chunk->SectorCount * APPLE_DISK_IMAGE_SECTOR_SIZE);
      ChunkLength      = (ChunkTotalLength - ChunkOffset);

      BufferChunkSize = (UINTN)MIN (RemainingBufferSize, ChunkLength);

      switch (Chunk->Type) {
        case APPLE_DISK_IMAGE_CHUNK_TYPE_ZERO:
        case APPLE_DISK_IMAGE_CHUNK_TYPE_IGNORE:
        {
          ZeroMem (BufferCurrent, BufferChunkSize);
          break;
        }

        case APPLE_DISK_IMAGE_CHUNK_TYPE_RAW:
        {
          ChunkData = (Context->Buffer + BlockData->DataOffset + Chunk->CompressedOffset);
          ChunkDataCurrent = (ChunkData + ChunkOffset);

          CopyMem (BufferCurrent, ChunkDataCurrent, BufferChunkSize);
          break;
        }

        case APPLE_DISK_IMAGE_CHUNK_TYPE_ZLIB:
        {
          ChunkData = AllocateZeroPool (ChunkTotalLength);
          if (ChunkData == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

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

          CopyMem (BufferCurrent, ChunkDataCurrent, BufferChunkSize);
          FreePool (ChunkData);
          break;
        }

        default:
        {
          return EFI_UNSUPPORTED;
        }
      }

      RemainingBufferSize -= BufferChunkSize;
      BufferCurrent       += BufferChunkSize;
      LbaCurrent          += LbaLength;
  }

  return EFI_SUCCESS;
}

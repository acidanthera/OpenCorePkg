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

#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleChunklistLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>

#include "OcAppleDiskImageLibInternal.h"

BOOLEAN
OcAppleDiskImageInitializeContext (
  OUT OC_APPLE_DISK_IMAGE_CONTEXT        *Context,
  IN  CONST APPLE_RAM_DISK_EXTENT_TABLE  *ExtentTable,
  IN  UINTN                              FileSize
  )
{
  BOOLEAN                     Result;
  UINTN                       TrailerOffset;
  APPLE_DISK_IMAGE_TRAILER    Trailer;
  UINT32                      DmgBlockCount;
  APPLE_DISK_IMAGE_BLOCK_DATA **DmgBlocks;
  UINT32                      SwappedSig;
  UINT64                      OffsetTop;

  UINT32                      HeaderSize;
  UINT64                      DataForkOffset;
  UINT64                      DataForkLength;
  UINT32                      SegmentCount;
  APPLE_DISK_IMAGE_CHECKSUM   DataForkChecksum;
  UINT64                      XmlOffset;
  UINT64                      XmlLength;
  UINT64                      SectorCount;

  CHAR8                       *PlistData;

  ASSERT (Context != NULL);
  ASSERT (ExtentTable != NULL);
  ASSERT (FileSize > 0);

  if (FileSize <= sizeof (Trailer)) {
    DEBUG ((
      DEBUG_INFO,
      "OCDI: DMG file size error: %u/%u\n",
      FileSize,
      (UINT32) sizeof (Trailer)
      ));
    return FALSE;
  }

  SwappedSig = SwapBytes32 (APPLE_DISK_IMAGE_MAGIC);

  TrailerOffset = (FileSize - sizeof (Trailer));

  Result = OcAppleRamDiskRead (
             ExtentTable,
             TrailerOffset,
             sizeof (Trailer),
             &Trailer
             );
  if (!Result || (Trailer.Signature != SwappedSig)) {
    DEBUG ((
      DEBUG_INFO,
      "OCDI: DMG trailer error: %d - %Lx/%Lx - %X/%X\n",
      Result,
      (UINT64) TrailerOffset,
      (UINT64) FileSize,
      SwappedSig,
      Trailer.Signature
      ));
    return FALSE;
  }

  HeaderSize            = SwapBytes32 (Trailer.HeaderSize);
  DataForkOffset        = SwapBytes64 (Trailer.DataForkOffset);
  DataForkLength        = SwapBytes64 (Trailer.DataForkLength);
  SegmentCount          = SwapBytes32 (Trailer.SegmentCount);
  XmlOffset             = SwapBytes64 (Trailer.XmlOffset);
  XmlLength             = SwapBytes64 (Trailer.XmlLength);
  SectorCount           = SwapBytes64 (Trailer.SectorCount);
  DataForkChecksum.Size = SwapBytes32 (Trailer.DataForkChecksum.Size);

  if ((HeaderSize != sizeof (Trailer))
   || (XmlLength == 0)
   || (XmlLength > MAX_UINT32)
   || (DataForkChecksum.Size > (sizeof (DataForkChecksum.Data) * 8))
   || (SectorCount == 0)) {
    DEBUG ((
      DEBUG_INFO,
      "OCDI: DMG context error: %u/%Lu/%Lu/%u/%u\n",
      HeaderSize,
      XmlLength,
      SectorCount,
      DataForkChecksum.Size
      ));
    return FALSE;
  }

  if ((SegmentCount != 0) && (SegmentCount != 1)) {
    DEBUG ((DEBUG_ERROR, "OCDI: Multiple segments are unsupported\n"));
    return FALSE;
  }

  Result = OcOverflowMulU64 (
             SectorCount,
             APPLE_DISK_IMAGE_SECTOR_SIZE,
             &OffsetTop
             );
  if (Result || (OffsetTop > MAX_UINTN)) {
    DEBUG ((DEBUG_INFO, "OCDI: DMG sector error: %Lu %Lu\n", SectorCount, OffsetTop));
    return FALSE;
  }

  Result = OcOverflowAddU64 (
             XmlOffset,
             XmlLength,
             &OffsetTop
             );
  if (Result || (OffsetTop > TrailerOffset)) {
    DEBUG ((DEBUG_INFO, "OCDI: DMG xml error: %Lu %Lu %Lu %Lu\n", XmlOffset, XmlLength, OffsetTop, TrailerOffset));
    return FALSE;
  }

  Result = OcOverflowAddU64 (
             DataForkOffset,
             DataForkLength,
             &OffsetTop
             );
  if (Result || (OffsetTop > TrailerOffset)) {
    DEBUG ((DEBUG_INFO, "OCDI: DMG data error: %Lu %Lu %Lu %Lu\n", DataForkOffset, DataForkLength, OffsetTop, TrailerOffset));
    return FALSE;
  }

  PlistData = AllocatePool ((UINT32)XmlLength);
  if (PlistData == NULL) {
    DEBUG ((DEBUG_INFO, "OCDI: DMG plist alloc error: %Lu\n", XmlLength));
    return FALSE;
  }

  Result = OcAppleRamDiskRead (
             ExtentTable,
             (UINTN)XmlOffset,
             (UINTN)XmlLength,
             PlistData
             );
  if (!Result) {
    DEBUG ((DEBUG_INFO, "OCDI: DMG plist read error: %Lu %Lu\n", XmlOffset, XmlLength));
    FreePool (PlistData);
    return FALSE;
  }

  Result = InternalParsePlist (
             PlistData,
             (UINT32)XmlLength,
             (UINTN)SectorCount,
             (UINTN)DataForkOffset,
             (UINTN)DataForkLength,
             &DmgBlockCount,
             &DmgBlocks
             );

  FreePool (PlistData);

  if (!Result) {
    DEBUG ((DEBUG_INFO, "OCDI: DMG plist parse error: %Lu %Lu\n", XmlOffset, XmlLength));
    return FALSE;
  }

  Context->ExtentTable = ExtentTable;
  Context->BlockCount  = DmgBlockCount;
  Context->Blocks      = DmgBlocks;
  Context->SectorCount = (UINTN)SectorCount;

  return TRUE;
}

BOOLEAN
OcAppleDiskImageInitializeFromFile (
  OUT OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  EFI_FILE_PROTOCOL            *File
  )
{
  EFI_STATUS                        Status;
  BOOLEAN                           Result;

  UINT32                            FileSize;
  CONST APPLE_RAM_DISK_EXTENT_TABLE *ExtentTable;

  ASSERT (Context != NULL);
  ASSERT (File != NULL);

  Status = GetFileSize (File, &FileSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCDI: Failed to retrieve DMG file size\n"));
    return FALSE;
  }

  ExtentTable = OcAppleRamDiskAllocate (FileSize, EfiACPIMemoryNVS);
  if (ExtentTable == NULL) {
    DEBUG ((DEBUG_INFO, "OCDI: Failed to allocate DMG data\n"));
    return FALSE;
  }

  Result = OcAppleRamDiskLoadFile (ExtentTable, File, FileSize);
  if (!Result) {
    DEBUG ((DEBUG_INFO, "OCDI: Failed to load DMG file\n"));

    OcAppleRamDiskFree (ExtentTable);
    return FALSE;
  }

  Result = OcAppleDiskImageInitializeContext (Context, ExtentTable, FileSize);
  if (!Result) {
    DEBUG ((DEBUG_INFO, "OCDI: Failed to initialise DMG context\n"));

    OcAppleRamDiskFree (ExtentTable);
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
OcAppleDiskImageVerifyData (
  IN OUT OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN OUT OC_APPLE_CHUNKLIST_CONTEXT   *ChunklistContext
  )
{
  ASSERT (Context != NULL);
  ASSERT (ChunklistContext != NULL);

  return OcAppleChunklistVerifyData (
           ChunklistContext,
           Context->ExtentTable
           );
}

VOID
OcAppleDiskImageFreeContext (
  IN OC_APPLE_DISK_IMAGE_CONTEXT  *Context
  )
{
  UINT32 Index;

  ASSERT (Context != NULL);

  for (Index = 0; Index < Context->BlockCount; ++Index) {
    FreePool (Context->Blocks[Index]);
  }

  FreePool (Context->Blocks);
}

VOID
OcAppleDiskImageFreeFile (
  IN OC_APPLE_DISK_IMAGE_CONTEXT  *Context
  )
{
  OcAppleRamDiskFree (Context->ExtentTable);
  OcAppleDiskImageFreeContext (Context);
}

BOOLEAN
OcAppleDiskImageRead (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  UINTN                        Lba,
  IN  UINTN                        BufferSize,
  OUT VOID                         *Buffer
  )
{
  BOOLEAN                     Result;

  APPLE_DISK_IMAGE_BLOCK_DATA *BlockData;
  APPLE_DISK_IMAGE_CHUNK      *Chunk;
  UINT64                      ChunkTotalLength;
  UINT64                      ChunkLength;
  UINT64                      ChunkOffset;
  UINT8                       *ChunkData;
  UINT8                       *ChunkDataCompressed;

  UINTN                       LbaCurrent;
  UINTN                       LbaOffset;
  UINTN                       LbaLength;
  UINTN                       RemainingBufferSize;
  UINTN                       BufferChunkSize;
  UINT8                       *BufferCurrent;

  UINTN                       OutSize;

  ASSERT (Context != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (Lba < Context->SectorCount);

  LbaCurrent          = Lba;
  RemainingBufferSize = BufferSize;
  BufferCurrent       = Buffer;

  while (RemainingBufferSize > 0) {
    Result = InternalGetBlockChunk (Context, LbaCurrent, &BlockData, &Chunk);
    if (!Result) {
      return FALSE;
    }

    LbaOffset = (LbaCurrent - (UINTN)DMG_SECTOR_START_ABS (BlockData, Chunk));
    LbaLength = ((UINTN)Chunk->SectorCount - LbaOffset);

    Result = OcOverflowMulU64 (
               LbaOffset,
               APPLE_DISK_IMAGE_SECTOR_SIZE,
               &ChunkOffset
               );
    if (Result) {
      return FALSE;
    }

    Result = OcOverflowMulU64 (
               Chunk->SectorCount,
               APPLE_DISK_IMAGE_SECTOR_SIZE,
               &ChunkTotalLength
               );
    if (Result) {
      return FALSE;
    }

    ChunkLength = (ChunkTotalLength - ChunkOffset);

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
        Result = OcAppleRamDiskRead (
                   Context->ExtentTable,
                   (UINTN)(Chunk->CompressedOffset + ChunkOffset),
                   BufferChunkSize,
                   BufferCurrent
                   );
        if (!Result) {
          return FALSE;
        }

        break;
      }

      case APPLE_DISK_IMAGE_CHUNK_TYPE_ZLIB:
      {
        ChunkData = AllocatePool ((UINTN)(ChunkTotalLength + Chunk->CompressedLength));
        if (ChunkData == NULL) {
          return FALSE;
        }

        ChunkDataCompressed = (ChunkData + (UINTN)ChunkTotalLength);
        Result = OcAppleRamDiskRead (
                   Context->ExtentTable,
                   (UINTN)Chunk->CompressedOffset,
                   (UINTN)Chunk->CompressedLength,
                   ChunkDataCompressed
                   );
        if (!Result) {
          FreePool (ChunkData);
          return FALSE;
        }

        OutSize = DecompressZLIB (
                    ChunkData,
                    (UINTN)ChunkTotalLength,
                    ChunkDataCompressed,
                    (UINTN)Chunk->CompressedLength
                    );
        if (OutSize != (UINTN)ChunkTotalLength) {
          FreePool (ChunkData);
          return FALSE;
        }

        CopyMem (BufferCurrent, (ChunkData + ChunkOffset), BufferChunkSize);
        FreePool (ChunkData);
        break;
      }

      default:
      {
        DEBUG ((
          DEBUG_ERROR,
          "OCDI: Compression type %x unsupported\n",
          Chunk->Type
          ));
        return FALSE;
      }
    }

    RemainingBufferSize -= BufferChunkSize;
    BufferCurrent       += BufferChunkSize;
    LbaCurrent          += LbaLength;
  }

  return TRUE;
}

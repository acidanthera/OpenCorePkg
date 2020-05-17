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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcXmlLib.h>

#include "OcAppleDiskImageLibInternal.h"

STATIC
BOOLEAN
InternalFindPlistDictChild (
  IN  XML_NODE  *Node,
  IN  CHAR8     *KeyName,
  OUT XML_NODE  **Key,
  OUT XML_NODE  **Value
  )
{
  UINT32      ChildCount;
  XML_NODE    *ChildValue;
  XML_NODE    *ChildKey;
  CONST CHAR8 *ChildKeyName;
  UINT32      Index;

  ASSERT (Node != NULL);
  ASSERT (KeyName != NULL);
  ASSERT (Key != NULL);
  ASSERT (Value != NULL);

  ChildCount = PlistDictChildren (Node);
  for (Index = 0; Index < ChildCount; ++Index) {
    ChildKey = PlistDictChild (Node, Index, &ChildValue);
    if (ChildKey == NULL) {
      break;
    }

    ChildKeyName = PlistKeyValue (ChildKey);
    if (ChildKeyName == NULL) {
      break;
    }

    if (AsciiStrCmp (ChildKeyName, KeyName) == 0) {
      *Key   = ChildKey;
      *Value = ChildValue;
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
BOOLEAN
InternalSwapBlockData (
  IN OUT APPLE_DISK_IMAGE_BLOCK_DATA  *BlockData,
  IN     UINT32                       MaxSize,
  IN     UINTN                        SectorCount,
  IN     UINTN                        DataForkOffset,
  IN     UINTN                        DataForkSize
  )
{
  UINT32                 ChunksSize;
  UINTN                  MaxOffset;
  BOOLEAN                Result;
  APPLE_DISK_IMAGE_CHUNK *Chunk;
  UINT32                 Index;
  UINT64                 BlockSectorTop;
  UINT64                 ChunkSectorTop;
  UINT64                 OffsetTop;

  ASSERT (MaxSize >= sizeof (*BlockData));

  BlockData->ChunkCount = SwapBytes32 (BlockData->ChunkCount);

  Result = OcOverflowMulU32 (
             BlockData->ChunkCount,
             sizeof (*BlockData->Chunks),
             &ChunksSize
             );
  if (Result || (ChunksSize > (MaxSize - sizeof (*BlockData)))) {
    return FALSE;
  }

  MaxOffset = (DataForkOffset + DataForkSize);

  BlockData->Version          = SwapBytes32 (BlockData->Version);
  BlockData->SectorNumber     = SwapBytes64 (BlockData->SectorNumber);
  BlockData->SectorCount      = SwapBytes64 (BlockData->SectorCount);
  BlockData->DataOffset       = SwapBytes64 (BlockData->DataOffset);
  BlockData->BuffersNeeded    = SwapBytes32 (BlockData->BuffersNeeded);
  BlockData->BlockDescriptors = SwapBytes32 (BlockData->BlockDescriptors);
  BlockData->Checksum.Type    = SwapBytes32 (BlockData->Checksum.Type);
  BlockData->Checksum.Size    = SwapBytes32 (BlockData->Checksum.Size);

  if (BlockData->DataOffset > DataForkOffset
   || (BlockData->Checksum.Size > (sizeof (BlockData->Checksum.Data) * 8))
   || (BlockData->DataOffset > MaxOffset)) {
    return FALSE;
  }

  Result = OcOverflowAddU64 (
             BlockData->SectorNumber,
             BlockData->SectorCount,
             &BlockSectorTop
             );
  if (Result || BlockSectorTop > SectorCount) {
    DEBUG ((DEBUG_ERROR, "OCDI: Block sectors exceed DMG sectors %lu %lu\n", BlockSectorTop, SectorCount));
    return FALSE;
  }

  for (Index = 0; Index < APPLE_DISK_IMAGE_CHECKSUM_SIZE; ++Index) {
    BlockData->Checksum.Data[Index] = SwapBytes32 (
                                        BlockData->Checksum.Data[Index]
                                        );
  }

  for (Index = 0; Index < BlockData->ChunkCount; ++Index) {
    Chunk = &BlockData->Chunks[Index];

    Chunk->Type             = SwapBytes32 (Chunk->Type);
    Chunk->Comment          = SwapBytes32 (Chunk->Comment);
    Chunk->SectorNumber     = SwapBytes64 (Chunk->SectorNumber);
    Chunk->SectorCount      = SwapBytes64 (Chunk->SectorCount);
    Chunk->CompressedOffset = SwapBytes64 (Chunk->CompressedOffset);
    Chunk->CompressedLength = SwapBytes64 (Chunk->CompressedLength);

    Result = OcOverflowAddU64 (
               Chunk->SectorNumber,
               Chunk->SectorCount,
               &ChunkSectorTop
               );
    if (Result || (ChunkSectorTop > BlockSectorTop)) {
      return FALSE;
    }

    Result = OcOverflowAddU64 (
               Chunk->CompressedOffset,
               Chunk->CompressedLength,
               &OffsetTop
               );
    if (Result || (OffsetTop > MaxOffset)) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
InternalParsePlist (
  IN  CHAR8                        *Plist,
  IN  UINT32                       PlistSize,
  IN  UINTN                        SectorCount,
  IN  UINTN                        DataForkOffset,
  IN  UINTN                        DataForkSize,
  OUT UINT32                       *BlockCount,
  OUT APPLE_DISK_IMAGE_BLOCK_DATA  ***Blocks
  )
{
  BOOLEAN                     Result;

  XML_DOCUMENT                *XmlPlistDoc;
  XML_NODE                    *NodeRoot;
  XML_NODE                    *NodeResourceForkKey;
  XML_NODE                    *NodeResourceForkValue;
  XML_NODE                    *NodeBlockListKey;
  XML_NODE                    *NodeBlockListValue;

  XML_NODE                    *NodeBlockDict;
  XML_NODE                    *BlockDictChildKey;
  XML_NODE                    *BlockDictChildValue;
  UINT32                      BlockDictChildDataSize;

  UINT32                      NumDmgBlocks;
  UINT32                      DmgBlocksSize;
  APPLE_DISK_IMAGE_BLOCK_DATA **DmgBlocks;
  APPLE_DISK_IMAGE_BLOCK_DATA *Block;

  UINT32                      Index;

  ASSERT (Plist != NULL);
  ASSERT (PlistSize > 0);
  ASSERT (BlockCount != NULL);
  ASSERT (Blocks != NULL);

  DmgBlocks = NULL;

  XmlPlistDoc = NULL;

  XmlPlistDoc = XmlDocumentParse (Plist, PlistSize, FALSE);
  if (XmlPlistDoc == NULL) {
    Result = FALSE;
    goto DONE_ERROR;
  }

  NodeRoot = PlistDocumentRoot (XmlPlistDoc);
  if (NodeRoot == NULL) {
    Result = FALSE;
    goto DONE_ERROR;
  }

  Result = InternalFindPlistDictChild (
             NodeRoot,
             DMG_PLIST_RESOURCE_FORK_KEY,
             &NodeResourceForkKey,
             &NodeResourceForkValue
             );
  if (!Result) {
    goto DONE_ERROR;
  }

  Result = InternalFindPlistDictChild (
             NodeResourceForkValue,
             DMG_PLIST_BLOCK_LIST_KEY,
             &NodeBlockListKey,
             &NodeBlockListValue
             );
  if (!Result) {
    goto DONE_ERROR;
  }

  NumDmgBlocks = XmlNodeChildren (NodeBlockListValue);
  if (NumDmgBlocks == 0) {
    Result = FALSE;
    goto DONE_ERROR;
  }

  Result = !OcOverflowMulU32 (NumDmgBlocks, sizeof (*DmgBlocks), &DmgBlocksSize);
  if (!Result) { ///< Result must be FALSE on error, it's checked at DONE_ERROR
    goto DONE_ERROR;
  }

  DmgBlocks = AllocatePool (DmgBlocksSize);
  if (DmgBlocks == NULL) {
    Result = FALSE;
    goto DONE_ERROR;
  }

  for (Index = 0; Index < NumDmgBlocks; ++Index) {
    NodeBlockDict = XmlNodeChild (NodeBlockListValue, Index);

    Result = InternalFindPlistDictChild (
               NodeBlockDict,
               DMG_PLIST_DATA,
               &BlockDictChildKey,
               &BlockDictChildValue
               );
    if (!Result) {
      goto DONE_ERROR;
    }

    Result = PlistDataSize (BlockDictChildValue, &BlockDictChildDataSize);
    if (!Result || (BlockDictChildDataSize < sizeof (*Block))) {
      Result = FALSE;
      goto DONE_ERROR;
    }

    Block = AllocatePool (BlockDictChildDataSize);
    if (Block == NULL) {
      Result = FALSE;
      goto DONE_ERROR;
    }

    DmgBlocks[Index] = Block;

    Result = PlistDataValue (
               BlockDictChildValue,
               (UINT8 *)Block,
               &BlockDictChildDataSize
               );
    if (!Result) {
      FreePool (Block);
      goto DONE_ERROR;
    }

    Result = InternalSwapBlockData (
               Block,
               BlockDictChildDataSize,
               SectorCount,
               DataForkOffset,
               DataForkSize
               );
    if (!Result) {
      FreePool (Block);
      goto DONE_ERROR;
    }
  }

  *BlockCount = NumDmgBlocks;
  *Blocks     = DmgBlocks;
  Result      = TRUE;

DONE_ERROR:
  if (!Result && (DmgBlocks != NULL)) {
    while ((Index--) != 0) {
      FreePool (DmgBlocks[Index]);
    }

    FreePool (DmgBlocks);
  }

  if (XmlPlistDoc != NULL) {
    XmlDocumentFree (XmlPlistDoc);
  }

  return Result;
}

BOOLEAN
InternalGetBlockChunk (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  UINTN                        Lba,
  OUT APPLE_DISK_IMAGE_BLOCK_DATA  **Data,
  OUT APPLE_DISK_IMAGE_CHUNK       **Chunk
  )
{
  UINT32                      BlockIndex;
  UINT32                      ChunkIndex;
  APPLE_DISK_IMAGE_BLOCK_DATA *BlockData;
  APPLE_DISK_IMAGE_CHUNK      *BlockChunk;

  for (BlockIndex = 0; BlockIndex < Context->BlockCount; ++BlockIndex) {
    BlockData = Context->Blocks[BlockIndex];

    if ((Lba >= BlockData->SectorNumber)
     && (Lba < (BlockData->SectorNumber + BlockData->SectorCount))) {
      for (ChunkIndex = 0; ChunkIndex < BlockData->ChunkCount; ++ChunkIndex) {
        BlockChunk = &BlockData->Chunks[ChunkIndex];

        if ((Lba >= DMG_SECTOR_START_ABS (BlockData, BlockChunk))
         && (Lba < (DMG_SECTOR_START_ABS (BlockData, BlockChunk) + BlockChunk->SectorCount))) {
          *Data  = BlockData;
          *Chunk = BlockChunk;
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}

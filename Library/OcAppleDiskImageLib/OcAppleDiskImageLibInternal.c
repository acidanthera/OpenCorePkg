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
#include <Library/OcXmlLib.h>

#include "OcAppleDiskImageLibInternal.h"

STATIC
EFI_STATUS
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
      ChildKey     = PlistDictChild (Node, Index, &ChildValue);
      ChildKeyName = PlistKeyValue (ChildKey);

      if ((ChildKeyName != NULL)
        && (AsciiStrCmp (ChildKeyName, KeyName) == 0)) {
        *Key   = ChildKey;
        *Value = ChildValue;
        return EFI_SUCCESS;
      }
  }

  return EFI_NOT_FOUND;
}

STATIC
VOID
InternalSwapBlockData (
  IN OUT APPLE_DISK_IMAGE_BLOCK_DATA  *BlockData
  )
{
  APPLE_DISK_IMAGE_CHUNK *Chunk;
  UINT32                 Index;

  BlockData->Version          = SwapBytes32 (BlockData->Version);
  BlockData->SectorNumber     = SwapBytes64 (BlockData->SectorNumber);
  BlockData->SectorCount      = SwapBytes64 (BlockData->SectorCount);
  BlockData->DataOffset       = SwapBytes64 (BlockData->DataOffset);
  BlockData->BuffersNeeded    = SwapBytes32 (BlockData->BuffersNeeded);
  BlockData->BlockDescriptors = SwapBytes32 (BlockData->BlockDescriptors);
  BlockData->Checksum.Type    = SwapBytes32 (BlockData->Checksum.Type);
  BlockData->Checksum.Size    = SwapBytes32 (BlockData->Checksum.Size);

  for (Index = 0; Index < APPLE_DISK_IMAGE_CHECKSUM_SIZE; ++Index) {
    BlockData->Checksum.Data[Index] = SwapBytes32 (
                                        BlockData->Checksum.Data[Index]
                                        );
  }

  BlockData->ChunkCount = SwapBytes32 (BlockData->ChunkCount);

  for (Index = 0; Index < BlockData->ChunkCount; ++Index) {
    Chunk = &BlockData->Chunks[Index];

    Chunk->Type             = SwapBytes32 (Chunk->Type);
    Chunk->Comment          = SwapBytes32 (Chunk->Comment);
    Chunk->SectorNumber     = SwapBytes64 (Chunk->SectorNumber);
    Chunk->SectorCount      = SwapBytes64 (Chunk->SectorCount);
    Chunk->CompressedOffset = SwapBytes64 (Chunk->CompressedOffset);
    Chunk->CompressedLength = SwapBytes64 (Chunk->CompressedLength);
  }
}

EFI_STATUS
InternalParsePlist (
  IN  VOID                               *Buffer,
  IN  UINT32                             XmlOffset,
  IN  UINT32                             XmlLength,
  OUT UINT32                             *BlockCount,
  OUT OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT  **Blocks
  )
{
  EFI_STATUS                        Status;

  CHAR8                             *XmlPlistBuffer;
  XML_DOCUMENT                      *XmlPlistDoc;
  XML_NODE                          *NodeRoot;
  XML_NODE                          *NodeResourceForkKey;
  XML_NODE                          *NodeResourceForkValue;
  XML_NODE                          *NodeBlockListKey;
  XML_NODE                          *NodeBlockListValue;

  XML_NODE                          *NodeBlockDict;
  XML_NODE                          *BlockDictChildKey;
  XML_NODE                          *BlockDictChildValue;
  UINT32                            BlockDictChildDataSize;

  UINT32                            NumDmgBlocks;
  OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT *DmgBlocks;
  OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT *Block;

  UINT32 Index;

  ASSERT (Buffer != NULL);
  ASSERT (XmlLength > 0);
  ASSERT (BlockCount != NULL);
  ASSERT (Blocks != NULL);

  XmlPlistDoc = NULL;

  XmlPlistBuffer = AllocateCopyPool (XmlLength, ((UINT8 *)Buffer + XmlOffset));
  if (XmlPlistBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto DONE_ERROR;
  }

  XmlPlistDoc = XmlDocumentParse (XmlPlistBuffer, XmlLength, FALSE);
  if (XmlPlistDoc == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto DONE_ERROR;
  }

  NodeRoot = PlistDocumentRoot (XmlPlistDoc);
  if (NodeRoot == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto DONE_ERROR;
  }

  Status = InternalFindPlistDictChild (
             NodeRoot,
             DMG_PLIST_RESOURCE_FORK_KEY,
             &NodeResourceForkKey,
             &NodeResourceForkValue
             );
  if (EFI_ERROR (Status)) {
    goto DONE_ERROR;
  }

  Status = InternalFindPlistDictChild (
             NodeResourceForkValue,
             DMG_PLIST_BLOCK_LIST_KEY,
             &NodeBlockListKey,
             &NodeBlockListValue
             );
  if (EFI_ERROR (Status)) {
    goto DONE_ERROR;
  }

  NumDmgBlocks = XmlNodeChildren (NodeBlockListValue);
  DmgBlocks    = AllocateZeroPool (NumDmgBlocks * sizeof (*DmgBlocks));
  if (DmgBlocks == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto DONE_ERROR;
  }

  for (Index = 0; Index < NumDmgBlocks; ++Index) {
    Block = &DmgBlocks[Index];

    NodeBlockDict = XmlNodeChild (NodeBlockListValue, Index);

    // TODO they are actually string.
    Status = InternalFindPlistDictChild (
                NodeBlockDict,
                DMG_PLIST_ATTRIBUTES,
                &BlockDictChildKey,
                &BlockDictChildValue
                );
    if (EFI_ERROR (Status)) {
      goto DONE_ERROR;
    }

    PlistIntegerValue (
      BlockDictChildValue,
      &Block->Attributes,
      sizeof(Block->Attributes),
      FALSE
      );

    Status = InternalFindPlistDictChild (
                NodeBlockDict,
                DMG_PLIST_CFNAME,
                &BlockDictChildKey,
                &BlockDictChildValue
                );
    if (EFI_ERROR (Status)) {
      goto DONE_ERROR;
    }

    BlockDictChildDataSize = 0;
    PlistStringSize (BlockDictChildValue, &BlockDictChildDataSize);
    Block->CfName = AllocateZeroPool (BlockDictChildDataSize);
    if (Block->CfName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto DONE_ERROR;
    }
    PlistStringValue (
      BlockDictChildValue,
      Block->CfName,
      &BlockDictChildDataSize
      );

    Status = InternalFindPlistDictChild (
                NodeBlockDict,
                DMG_PLIST_NAME,
                &BlockDictChildKey,
                &BlockDictChildValue
                );
    if (EFI_ERROR (Status)) {
      goto DONE_ERROR;
    }

    BlockDictChildDataSize = 0;
    PlistStringSize (BlockDictChildValue, &BlockDictChildDataSize);
    Block->Name = AllocateZeroPool (BlockDictChildDataSize);
    if (Block->Name == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto DONE_ERROR;
    }
    PlistStringValue (
      BlockDictChildValue,
      Block->Name,
      &BlockDictChildDataSize
      );

    Status = InternalFindPlistDictChild (
                NodeBlockDict,
                DMG_PLIST_ID,
                &BlockDictChildKey,
                &BlockDictChildValue
                );
    if (EFI_ERROR (Status)) {
      goto DONE_ERROR;
    }

    PlistIntegerValue (
      BlockDictChildValue,
      &Block->Id,
      sizeof (Block->Id),
      FALSE
      );

    Status = InternalFindPlistDictChild (
                NodeBlockDict,
                DMG_PLIST_DATA,
                &BlockDictChildKey,
                &BlockDictChildValue
                );
    if (EFI_ERROR (Status)) {
      goto DONE_ERROR;
    }

    BlockDictChildDataSize = 0;
    PlistDataSize (BlockDictChildValue, &BlockDictChildDataSize);
    Block->BlockData = AllocateZeroPool (BlockDictChildDataSize);
    if (Block->BlockData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto DONE_ERROR;
    }

    PlistDataValue (
      BlockDictChildValue,
      (UINT8*)Block->BlockData,
      &BlockDictChildDataSize
      );

    InternalSwapBlockData (Block->BlockData);
  }

  *BlockCount = NumDmgBlocks;
  *Blocks     = DmgBlocks;
  Status      = EFI_SUCCESS;

DONE_ERROR:

  if (XmlPlistDoc != NULL) {
    XmlDocumentFree (XmlPlistDoc);
  }

  if (XmlPlistBuffer != NULL) {
    FreePool (XmlPlistBuffer);
  }

  return Status;
}

EFI_STATUS
InternalGetBlockChunk (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  EFI_LBA                      Lba,
  OUT APPLE_DISK_IMAGE_BLOCK_DATA  **Data,
  OUT APPLE_DISK_IMAGE_CHUNK       **Chunk
  )
{
  UINT32                      BlockIndex;
  UINT32                      ChunkIndex;
  APPLE_DISK_IMAGE_BLOCK_DATA *BlockData;
  APPLE_DISK_IMAGE_CHUNK      *BlockChunk;

  for (BlockIndex = 0; BlockIndex < Context->BlockCount; ++BlockIndex) {
    BlockData = Context->Blocks[BlockIndex].BlockData;

    if ((Lba >= BlockData->SectorNumber) &&
        (Lba < (BlockData->SectorNumber + BlockData->SectorCount))) {
      for (ChunkIndex = 0; ChunkIndex < BlockData->ChunkCount; ++ChunkIndex) {
        BlockChunk = &BlockData->Chunks[ChunkIndex];

        if ((Lba >= DMG_SECTOR_START_ABS (BlockData, BlockChunk))
         && (Lba < (DMG_SECTOR_START_ABS (BlockData, BlockChunk) + BlockChunk->SectorCount))) {
          *Data  = BlockData;
          *Chunk = BlockChunk;
          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_NOT_FOUND;
}

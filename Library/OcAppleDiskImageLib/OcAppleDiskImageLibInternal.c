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

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcXmlLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OcAppleDiskImageLibInternal.h"

// DP GUIDs.
EFI_GUID gDmgControllerDpGuid = DMG_CONTROLLER_DP_GUID;
EFI_GUID gDmgSizeDpGuid = DMG_SIZE_DP_GUID;

EFI_STATUS
EFIAPI
FindPlistDictChild(
    IN  XML_NODE *Node,
    IN  CHAR8 *KeyName,
    OUT XML_NODE **Key,
    OUT XML_NODE **Value) {
    // Create variables.
    UINT32 ChildCount;
    XML_NODE *ChildValue = NULL;
    XML_NODE *ChildKey = NULL;
    CONST CHAR8 *ChildKeyStr = NULL;

    ASSERT (Node != NULL);
    ASSERT (KeyName != NULL);
    ASSERT (Key != NULL);
    ASSERT (Value != NULL);

    // Search for key.
    ChildCount = PlistDictChildren(Node);
    for (UINT32 i = 0; i < ChildCount; i++) {
        // Get child key/value and key name.
        ChildKey = PlistDictChild(Node, i, &ChildValue);
        ChildKeyStr = PlistKeyValue(ChildKey);

        // Check if key matches.
        if (ChildKeyStr && !AsciiStrCmp(ChildKeyStr, KeyName)) {
            *Key = ChildKey;
            *Value = ChildValue;
            return EFI_SUCCESS;
        }
    }

    // If we get here, we couldn't find it.
    return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
ParsePlist(
    IN  VOID *Buffer,
    IN  UINT64 XmlOffset,
    IN  UINT64 XmlLength,
    OUT UINT32 *BlockCount,
    OUT OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT **Blocks) {

    // Create variables.
    EFI_STATUS Status;

    // Plist buffer.
    CHAR8 *XmlPlistBuffer = NULL;
    XML_DOCUMENT *XmlPlistDoc = NULL;
    XML_NODE *XmlPlistNodeRoot = NULL;
    XML_NODE *XmlPlistNodeResourceForkKey = NULL;
    XML_NODE *XmlPlistNodeResourceForkValue = NULL;
    XML_NODE *XmlPlistNodeBlockListKey = NULL;
    XML_NODE *XmlPlistNodeBlockListValue = NULL;

    // Plist blocks.
    XML_NODE *XmlPlistNodeBlockDict = NULL;
    XML_NODE *XmlPlistBlockDictChildKey = NULL;
    XML_NODE *XmlPlistBlockDictChildValue = NULL;
    UINT32 XmlPlistBlockDictChildDataSize;

    // DMG blocks.
    UINT32 DmgBlockCount;
    OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT *DmgBlocks;

    ASSERT (Buffer != NULL);
    ASSERT (XmlLength > 0);
    ASSERT (BlockCount != NULL);
    ASSERT (Blocks != NULL);

    // Allocate buffer for plist and copy plist to it.
    XmlPlistBuffer = AllocateCopyPool(XmlLength, (UINT8*)Buffer + XmlOffset);
    if (!XmlPlistBuffer) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Open plist and get root node.
    XmlPlistDoc = XmlDocumentParse(XmlPlistBuffer, (UINT32)XmlLength, FALSE);
    if (!XmlPlistDoc) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }
    XmlPlistNodeRoot = PlistDocumentRoot(XmlPlistDoc);
    if (!XmlPlistNodeRoot) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Get resource fork dictionary.
    Status = FindPlistDictChild(XmlPlistNodeRoot, DMG_PLIST_RESOURCE_FORK_KEY,
        &XmlPlistNodeResourceForkKey, &XmlPlistNodeResourceForkValue);
    if (EFI_ERROR(Status))
        goto DONE_ERROR;

    // Get block list dictionary.
    Status = FindPlistDictChild(XmlPlistNodeResourceForkValue, DMG_PLIST_BLOCK_LIST_KEY,
        &XmlPlistNodeBlockListKey, &XmlPlistNodeBlockListValue);
    if (EFI_ERROR(Status))
        goto DONE_ERROR;

    // Get block count and allocate space for blocks.
    DmgBlockCount = XmlNodeChildren(XmlPlistNodeBlockListValue);
    DmgBlocks = AllocateZeroPool(DmgBlockCount * sizeof(OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT));
    if (!DmgBlocks) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE_ERROR;
    }

    // Get blocks in plist.
    for (UINT32 b = 0; b < DmgBlockCount; b++) {
        // Get dictionary.
        XmlPlistNodeBlockDict = XmlNodeChild(XmlPlistNodeBlockListValue, b);

        // Get attributes. TODO they are actually string.
        Status = FindPlistDictChild(XmlPlistNodeBlockDict, DMG_PLIST_ATTRIBUTES,
            &XmlPlistBlockDictChildKey, &XmlPlistBlockDictChildValue);
        if (EFI_ERROR(Status))
            goto DONE_ERROR;
        PlistIntegerValue(XmlPlistBlockDictChildValue,
            &DmgBlocks[b].Attributes, sizeof(DmgBlocks[b].Attributes), FALSE);

        // Get CFName node.
        Status = FindPlistDictChild(XmlPlistNodeBlockDict, DMG_PLIST_CFNAME,
            &XmlPlistBlockDictChildKey, &XmlPlistBlockDictChildValue);
        if (EFI_ERROR(Status))
            goto DONE_ERROR;

        // Allocate CFName string and get it.
        XmlPlistBlockDictChildDataSize = 0;
        PlistStringSize(XmlPlistBlockDictChildValue, &XmlPlistBlockDictChildDataSize);
        DmgBlocks[b].CfName = AllocateZeroPool(XmlPlistBlockDictChildDataSize);
        if (!DmgBlocks[b].CfName) {
            Status = EFI_OUT_OF_RESOURCES;
            goto DONE_ERROR;
        }
        PlistStringValue(XmlPlistBlockDictChildValue,
            DmgBlocks[b].CfName, &XmlPlistBlockDictChildDataSize);

        // Get Name node.
        Status = FindPlistDictChild(XmlPlistNodeBlockDict, DMG_PLIST_NAME,
            &XmlPlistBlockDictChildKey, &XmlPlistBlockDictChildValue);
        if (EFI_ERROR(Status))
            goto DONE_ERROR;

        // Allocate Name string and get it.
        XmlPlistBlockDictChildDataSize = 0;
        PlistStringSize(XmlPlistBlockDictChildValue, &XmlPlistBlockDictChildDataSize);
        DmgBlocks[b].Name = AllocateZeroPool(XmlPlistBlockDictChildDataSize);
        if (!DmgBlocks[b].Name) {
            Status = EFI_OUT_OF_RESOURCES;
            goto DONE_ERROR;
        }
        PlistStringValue(XmlPlistBlockDictChildValue,
            DmgBlocks[b].Name, &XmlPlistBlockDictChildDataSize);

        // Get ID.
        Status = FindPlistDictChild(XmlPlistNodeBlockDict, DMG_PLIST_ID,
            &XmlPlistBlockDictChildKey, &XmlPlistBlockDictChildValue);
        if (EFI_ERROR(Status))
            goto DONE_ERROR;
        PlistIntegerValue(XmlPlistBlockDictChildValue,
            &DmgBlocks[b].Id, sizeof(DmgBlocks[b].Id), FALSE);

        // Get block data.
        Status = FindPlistDictChild(XmlPlistNodeBlockDict, DMG_PLIST_DATA,
            &XmlPlistBlockDictChildKey, &XmlPlistBlockDictChildValue);
        if (EFI_ERROR(Status))
            goto DONE_ERROR;

        // Allocate block data and get it.
        XmlPlistBlockDictChildDataSize = 0;
        PlistDataSize(XmlPlistBlockDictChildValue, &XmlPlistBlockDictChildDataSize);
        DmgBlocks[b].BlockData = AllocateZeroPool(XmlPlistBlockDictChildDataSize);
        if (!DmgBlocks[b].BlockData) {
            Status = EFI_OUT_OF_RESOURCES;
            goto DONE_ERROR;
        }
        PlistDataValue(XmlPlistBlockDictChildValue,
            (UINT8*)DmgBlocks[b].BlockData, &XmlPlistBlockDictChildDataSize);

        // Swap block fields.
        DmgBlocks[b].BlockData->Version = SwapBytes32(DmgBlocks[b].BlockData->Version);
        DmgBlocks[b].BlockData->SectorNumber = SwapBytes64(DmgBlocks[b].BlockData->SectorNumber);
        DmgBlocks[b].BlockData->SectorCount = SwapBytes64(DmgBlocks[b].BlockData->SectorCount);
        DmgBlocks[b].BlockData->DataOffset = SwapBytes64(DmgBlocks[b].BlockData->DataOffset);
        DmgBlocks[b].BlockData->BuffersNeeded = SwapBytes32(DmgBlocks[b].BlockData->BuffersNeeded);
        DmgBlocks[b].BlockData->BlockDescriptors = SwapBytes32(DmgBlocks[b].BlockData->BlockDescriptors);

        // Swap checksum.
        DmgBlocks[b].BlockData->Checksum.Type = SwapBytes32(DmgBlocks[b].BlockData->Checksum.Type);
        DmgBlocks[b].BlockData->Checksum.Size = SwapBytes32(DmgBlocks[b].BlockData->Checksum.Size);
        for (UINTN i = 0; i < APPLE_DISK_IMAGE_CHECKSUM_SIZE; i++)
            DmgBlocks[b].BlockData->Checksum.Data[i] = SwapBytes32(DmgBlocks[b].BlockData->Checksum.Data[i]);

        // Swap chunks.
        DmgBlocks[b].BlockData->ChunkCount = SwapBytes32(DmgBlocks[b].BlockData->ChunkCount);
        for (UINT32 c = 0; c < DmgBlocks[b].BlockData->ChunkCount; c++) {
            DmgBlocks[b].BlockData->Chunks[c].Type = SwapBytes32(DmgBlocks[b].BlockData->Chunks[c].Type);
            DmgBlocks[b].BlockData->Chunks[c].Comment = SwapBytes32(DmgBlocks[b].BlockData->Chunks[c].Comment);
            DmgBlocks[b].BlockData->Chunks[c].SectorNumber = SwapBytes64(DmgBlocks[b].BlockData->Chunks[c].SectorNumber);
            DmgBlocks[b].BlockData->Chunks[c].SectorCount = SwapBytes64(DmgBlocks[b].BlockData->Chunks[c].SectorCount);
            DmgBlocks[b].BlockData->Chunks[c].CompressedOffset = SwapBytes64(DmgBlocks[b].BlockData->Chunks[c].CompressedOffset);
            DmgBlocks[b].BlockData->Chunks[c].CompressedLength = SwapBytes64(DmgBlocks[b].BlockData->Chunks[c].CompressedLength);
        }
    }

    // Success.
    *BlockCount = DmgBlockCount;
    *Blocks = DmgBlocks;
    Status = EFI_SUCCESS;
    goto DONE;

DONE_ERROR:

DONE:
    // Free XML.
    if (XmlPlistDoc)
        XmlDocumentFree(XmlPlistDoc);
    if (XmlPlistBuffer)
        FreePool(XmlPlistBuffer);
    return Status;
}

EFI_STATUS
EFIAPI
VerifyCrc32(
    IN VOID *Buffer,
    IN UINTN Length,
    IN UINT32 Checksum) {

    // Create variables.
    UINT32 Crc32 = 0;

    ASSERT (Buffer != NULL);
    ASSERT (Length > 0);

    // Calculate CRC32 of full image.
    Crc32 = CalculateCrc32(Buffer, Length);

    // Ensure CRC32 matches.
    if (Crc32 != Checksum)
        return EFI_COMPROMISED_DATA;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetBlockChunk(
    IN  OC_APPLE_DISK_IMAGE_CONTEXT *Context,
    IN  EFI_LBA Lba,
    OUT APPLE_DISK_IMAGE_BLOCK_DATA **Data,
    OUT APPLE_DISK_IMAGE_CHUNK **Chunk) {

    // Create variables.
    APPLE_DISK_IMAGE_BLOCK_DATA *BlockData;
    APPLE_DISK_IMAGE_CHUNK *BlockChunk;

    // Search for chunk.
    for (UINT32 b = 0; b < Context->BlockCount; b++) {
        // Get block data.
        BlockData = Context->Blocks[b].BlockData;

        // Is the desired sector part of this block?
        if ((Lba >= BlockData->SectorNumber) &&
            (Lba < (BlockData->SectorNumber + BlockData->SectorCount))) {
            // Determine chunk.
            for (UINT32 c = 0; c < BlockData->ChunkCount; c++) {
                // Get chunk.
                BlockChunk = BlockData->Chunks + c;

                // Is the desired sector part of this chunk?
                if ((Lba >= DMG_SECTOR_START_ABS(BlockData, BlockChunk)) &&
                    (Lba < (DMG_SECTOR_START_ABS(BlockData, BlockChunk) + BlockChunk->SectorCount))) {
                    // Found the chunk.
                    *Data = BlockData;
                    *Chunk = BlockChunk;
                    return EFI_SUCCESS;
                }
            }
        }
    }

    // Couldn't find chunk.
    return EFI_NOT_FOUND;
}

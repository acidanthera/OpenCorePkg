/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_DISK_IMAGE_LIB_INTERNAL_H_
#define APPLE_DISK_IMAGE_LIB_INTERNAL_H_

#define BASE_256B  0x0100U
#define SIZE_512B  0x0200U

#define DMG_SECTOR_START_ABS(b, c) (((b)->SectorNumber) + ((c)->SectorNumber))

#define DMG_PLIST_RESOURCE_FORK_KEY  "resource-fork"
#define DMG_PLIST_BLOCK_LIST_KEY     "blkx"
#define DMG_PLIST_ATTRIBUTES         "Attributes"
#define DMG_PLIST_CFNAME             "CFName"
#define DMG_PLIST_DATA               "Data"
#define DMG_PLIST_ID                 "ID"
#define DMG_PLIST_NAME               "Name"

#pragma pack(1)

#define RAM_DMG_SIGNATURE  0x544E5458444D4152ULL // "RAMDXTNT"
#define RAM_DMG_VERSION    0x010000U

typedef PACKED struct {
  UINT64 Start;
  UINT64 Length;
} RAM_DMG_EXTENT_INFO;

typedef PACKED struct {
  UINT64              Signature;
  UINT32              Version;
  UINT32              ExtentCount;
  RAM_DMG_EXTENT_INFO ExtentInfo[0xFE];
  UINT64              Reserved;
  UINT64              Signature2;
} RAM_DMG_HEADER;

#pragma pack()

BOOLEAN
InternalParsePlist (
  IN  VOID                               *Buffer,
  IN  UINT32                             XmlOffset,
  IN  UINT32                             XmlLength,
  OUT UINT32                             *BlockCount,
  OUT OC_APPLE_DISK_IMAGE_BLOCK_CONTEXT  **Blocks
  );

BOOLEAN
InternalGetBlockChunk (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  UINT64                       Lba,
  OUT APPLE_DISK_IMAGE_BLOCK_DATA  **Data,
  OUT APPLE_DISK_IMAGE_CHUNK       **Chunk
  );

#endif

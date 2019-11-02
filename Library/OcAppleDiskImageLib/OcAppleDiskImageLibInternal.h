/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_DISK_IMAGE_LIB_INTERNAL_H
#define APPLE_DISK_IMAGE_LIB_INTERNAL_H

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

BOOLEAN
InternalParsePlist (
  IN  CHAR8                        *Plist,
  IN  UINT32                       PlistSize,
  IN  UINTN                        SectorCount,
  IN  UINTN                        DataForkOffset,
  IN  UINTN                        DataForkSize,
  OUT UINT32                       *BlockCount,
  OUT APPLE_DISK_IMAGE_BLOCK_DATA  ***Blocks
  );

BOOLEAN
InternalGetBlockChunk (
  IN  OC_APPLE_DISK_IMAGE_CONTEXT  *Context,
  IN  UINTN                        Lba,
  OUT APPLE_DISK_IMAGE_BLOCK_DATA  **Data,
  OUT APPLE_DISK_IMAGE_CHUNK       **Chunk
  );

#endif // APPLE_DISK_IMAGE_LIB_INTERNAL_H

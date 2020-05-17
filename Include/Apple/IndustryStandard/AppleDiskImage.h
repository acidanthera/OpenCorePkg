/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_DISK_IMAGE_H
#define APPLE_DISK_IMAGE_H

//
// Magic number used to identify the disk image trailer.
//
#define APPLE_DISK_IMAGE_MAGIC              0x6B6F6C79

//
// Supported disk image version.
//
#define APPLE_DISK_IMAGE_VERSION            4

//
// Checksum length.
//
#define APPLE_DISK_IMAGE_CHECKSUM_SIZE      32

//
// Magic number used to identify the disk image block data.
//
#define APPLE_DISK_IMAGE_BLOCK_DATA_MAGIC   0x6D697368

//
// Sector size.
//
#define APPLE_DISK_IMAGE_SECTOR_SIZE        512

//
// Disk image checksum type.
//
#define APPLE_DISK_IMAGE_CHECKSUM_TYPE_CRC32    0x00000002

//
// Disk image chunk type.
//
#define APPLE_DISK_IMAGE_CHUNK_TYPE_ZERO        0x00000000
#define APPLE_DISK_IMAGE_CHUNK_TYPE_RAW         0x00000001
#define APPLE_DISK_IMAGE_CHUNK_TYPE_IGNORE      0x00000002
#define APPLE_DISK_IMAGE_CHUNK_TYPE_ADC         0x80000004
#define APPLE_DISK_IMAGE_CHUNK_TYPE_ZLIB        0x80000005
#define APPLE_DISK_IMAGE_CHUNK_TYPE_BZ2         0x80000006
#define APPLE_DISK_IMAGE_CHUNK_TYPE_COMMENT     0x7FFFFFFE
#define APPLE_DISK_IMAGE_CHUNK_TYPE_LAST        0xFFFFFFFF

#pragma pack(push, 1)

//
// Disk image checksum.
//
typedef struct APPLE_DISK_IMAGE_CHECKSUM_ {
    UINT32  Type;
    UINT32  Size;
    UINT32  Data[APPLE_DISK_IMAGE_CHECKSUM_SIZE];
} APPLE_DISK_IMAGE_CHECKSUM;

//
// Disk image chunk (in XML).
//
typedef struct APPLE_DISK_IMAGE_CHUNK_ {
    UINT32  Type;
    UINT32  Comment;
    UINT64  SectorNumber;
    UINT64  SectorCount;
    UINT64  CompressedOffset;
    UINT64  CompressedLength;
} APPLE_DISK_IMAGE_CHUNK;

//
// Disk image block data (in XML).
//
typedef struct APPLE_DISK_IMAGE_BLOCK_DATA_ {
    UINT32                      Signature;
    UINT32                      Version;
    UINT64                      SectorNumber;
    UINT64                      SectorCount;

    UINT64                      DataOffset;
    UINT32                      BuffersNeeded;
    UINT32                      BlockDescriptors;
    UINT32                      Reserved[6];
    APPLE_DISK_IMAGE_CHECKSUM   Checksum;

    UINT32                      ChunkCount;
    APPLE_DISK_IMAGE_CHUNK      Chunks[];
} APPLE_DISK_IMAGE_BLOCK_DATA;

//
// Disk image trailer.
//
typedef struct APPLE_DISK_IMAGE_TRAILER_ {
    UINT32                      Signature;
    UINT32                      Version;
    UINT32                      HeaderSize;
    UINT32                      Flags;
    UINT64                      RunningDataForkOffset;
    UINT64                      DataForkOffset;
    UINT64                      DataForkLength;
    UINT64                      RsrcForkOffset;
    UINT64                      RsrcForkLength;
    UINT32                      SegmentNumber;
    UINT32                      SegmentCount;
    GUID                        SegmentId;
    APPLE_DISK_IMAGE_CHECKSUM   DataForkChecksum;

    UINT64                      XmlOffset;
    UINT64                      XmlLength;
    UINT8                       Reserved1[120];

    APPLE_DISK_IMAGE_CHECKSUM   Checksum;
    UINT32                      ImageVariant;
    UINT64                      SectorCount;
    UINT32                      Reserved2[3];
} APPLE_DISK_IMAGE_TRAILER;

#pragma pack(pop)

#endif // APPLE_DISK_IMAGE_H

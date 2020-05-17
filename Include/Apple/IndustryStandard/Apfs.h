/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APFS_H
#define APFS_H

//
// APFS object types.
//
#define APFS_OBJECT_TYPE_NX_SUPERBLOCK       0x00000001U
#define APFS_OBJECT_TYPE_BTREE               0x00000002U
#define APFS_OBJECT_TYPE_BTREE_NODE          0x00000003U
#define APFS_OBJECT_TYPE_SPACEMAN            0x00000005U
#define APFS_OBJECT_TYPE_SPACEMAN_CAB        0x00000006U
#define APFS_OBJECT_TYPE_SPACEMAN_CIB        0x00000007U
#define APFS_OBJECT_TYPE_SPACEMAN_BITMAP     0x00000008U
#define APFS_OBJECT_TYPE_SPACEMAN_FREE_QUEUE 0x00000009U
#define APFS_OBJECT_TYPE_EXTENT_LIST_TREE    0x0000000aU
#define APFS_OBJECT_TYPE_OMAP                0x0000000bU
#define APFS_OBJECT_TYPE_CHECKPOINT_MAP      0x0000000cU
#define APFS_OBJECT_TYPE_FS                  0x0000000dU
#define APFS_OBJECT_TYPE_FSTREE              0x0000000eU
#define APFS_OBJECT_TYPE_BLOCKREFTREE        0x0000000fU
#define APFS_OBJECT_TYPE_SNAPMETATREE        0x00000010U
#define APFS_OBJECT_TYPE_NX_REAPER           0x00000011U
#define APFS_OBJECT_TYPE_NX_REAP_LIST        0x00000012U
#define APFS_OBJECT_TYPE_OMAP_SNAPSHOT       0x00000013U
#define APFS_OBJECT_TYPE_EFI_JUMPSTART       0x00000014U
#define APFS_OBJECT_TYPE_FUSION_MIDDLE_TREE  0x00000015U
#define APFS_OBJECT_TYPE_NX_FUSION_WBC       0x00000016U
#define APFS_OBJECT_TYPE_NX_FUSION_WBC_LIST  0x00000017U
#define APFS_OBJECT_TYPE_ER_STATE            0x00000018U
#define APFS_OBJECT_TYPE_GBITMAP             0x00000019U
#define APFS_OBJECT_TYPE_GBITMAP_TREE        0x0000001aU
#define APFS_OBJECT_TYPE_GBITMAP_BLOCK       0x0000001bU
#define APFS_OBJECT_TYPE_INVALID             0x00000000U
#define APFS_OBJECT_TYPE_TEST                0x000000ffU

//
// APFS object flags.
//
#define APFS_OBJ_VIRTUAL                     0x00000000U
#define APFS_OBJ_EPHEMERAL                   0x80000000U
#define APFS_OBJ_PHYSICAL                    0x40000000U
#define APFS_OBJ_NOHEADER                    0x20000000U
#define APFS_OBJ_ENCRYPTED                   0x10000000U
#define APFS_OBJ_NONPERSISTENT               0x08000000U

//
// Container Superblock definitions
//
#define APFS_NX_SIGNATURE                    SIGNATURE_32 ('N', 'X', 'S', 'B')
#define APFS_NX_MAX_FILE_SYSTEMS             100
#define APFS_NX_EPH_INFO_COUNT               4
#define APFS_NX_EPH_MIN_BLOCK_COUNT          8
#define APFS_NX_MAX_FILE_SYSTEM_EPH_STRUCTS  4
#define APFS_NX_TX_MIN_CHECKPOINT_COUNT      4
#define APFS_NX_EPH_INFO_VERSION_1           1
#define APFS_NX_NUM_COUNTERS                 32
#define APFS_NX_MINIMUM_BLOCK_SIZE           BASE_4KB
#define APFS_NX_DEFAULT_BLOCK_SIZE           BASE_4KB
#define APFS_NX_MAXIMUM_BLOCK_SIZE           BASE_64KB

//
// EfiBootRecord block definitions
//
#define APFS_NX_EFI_JUMPSTART_MAGIC   SIGNATURE_32 ('J', 'S', 'D', 'R')
#define APFS_NX_EFI_JUMPSTART_VERSION 1

#define APFS_MAX_HIST           8
#define APFS_MODIFIED_NAMELEN   32
#define APFS_VOLNAME_LEN        256

//
// Fusion things
//
#define APFS_FUSION_TIER2_DEVICE_BYTE_ADDR 0x4000000000000000ULL

//
// Driver version
//
#define APFS_DRIVER_VERSION_MAGIC  SIGNATURE_32 ('A', 'P', 'F', 'S')

#pragma pack(push, 1)

/**
  APFS driver version structure.
**/
typedef struct APFS_DRIVER_VERSION_ {
  ///
  /// Matches APFS_DRIVER_VERSION_MAGIC.
  ///
  UINT32   Magic;
  ///
  /// Matches MajorImageVersion << 16 | MinorImageVersion.
  ///
  UINT32   ImageVersion;
  ///
  /// Actual driver version.
  ///
  UINT64   Version;
  ///
  /// In ISO format (YYYY/MM/DD), e.g. 2020/03/06.
  ///
  CHAR8    Date[16];
  ///
  /// In ISO format (HH:MM:SS), e.g. 23:26:03.
  ///
  CHAR8    Time[16];
} APFS_DRIVER_VERSION;

/**
  A range of physical addresses.
**/
typedef struct PhysicalRange_ {
  INT64     StartPhysicalAddr;
  UINT64    BlockCount;
} APFS_PHYSICAL_RANGE;

/**
  Information about how the volume encryption key (VEK)
  is used to encrypt a file.
**/
typedef struct {
  UINT16  MajorVersion;
  UINT16  MinorVersion;
  UINT32  CpFlags;
  UINT32  PersistentClass;
  UINT32  KeyOsVersion;
  UINT16  KeyRevision;
  UINT16  KeyLength;
} APFS_WRAPPED_CRYPTO_STATE;

/**
  Information about a program that modified the volume.
**/
typedef struct {
  UINT8     Id[APFS_MODIFIED_NAMELEN];
  UINT64    Timestamp;
  UINT64    LastXid;
} APFS_MODIFIED_BY;

/**
  Any object is prefixed with this header.
**/
typedef struct APFS_OBJ_PHYS_ {
  //
  // The Fletcher 64 checksum of the object.
  //
  UINT64             Checksum;
  //
  // The object's identifier.
  // NXSB=01 00.
  // APSB=02 04, 06 04, and 08 04.
  //
  UINT64             ObjectOid;
  //
  // The identifier of the most recent transaction that this object
  // was modified in.
  //
  UINT64             ObjectXid;
  //
  // The object's type and flags.
  //
  UINT32             ObjectType;
  //
  // The object's subtype.
  // Subtypes indicate the type of data stored in a data structure such as a
  // B-tree. For example, a node in a B-tree that contains volume records has
  // a type of OBJECT_TYPE_BTREE_NODE and a subtype of OBJECT_TYPE_FS.
  //
  UINT32             ObjectSubType;
} APFS_OBJ_PHYS;

/**
  NXSB Container Superblock
  The container superblock is the entry point to the filesystem.
  Because of the structure with containers and flexible volumes,
  allocation needs to handled on a container level.
  The container superblock contains information on the blocksize,
  the number of blocks and pointers to the spacemanager for this task.
  Additionally the block IDs of all volumes are stored in the superblock.
  To map block IDs to block offsets a pointer to a block map b-tree is stored.
  This b-tree contains entries for each volume with its ID and offset.
**/
typedef struct APFS_NX_SUPERBLOCK_ {
  APFS_OBJ_PHYS        BlockHeader;
  //
  // Magic: NXSB
  //
  UINT32               Magic;
  //
  // The logical block size used in the Apple File System container.
  // This size is often the same as the block size used by the underlying
  // storage device, but it can also be an integer multiple of the deviceʼs block size.
  //
  UINT32               BlockSize;
  //
  // Number of blocks in the container
  //
  UINT64               TotalBlocks;
  UINT64               Features;
  UINT64               ReadOnlyCompatibleFeatures;
  UINT64               IncompatibleFeatures;
  EFI_GUID             Uuid;
  UINT64               NextOid;
  UINT64               NextXid;
  UINT32               XpDescBlocks;
  UINT32               XpDataBlocks;
  INT64                XpDescBase;
  INT64                XpDataBase;
  UINT32               XpDescNext;
  UINT32               XpDataNext;
  UINT32               XpDescIndex;
  UINT32               XpDescLen;
  UINT32               XpDataIndex;
  UINT32               XpDataLen;
  UINT64               SpacemanOid;
  UINT64               ObjectMapOid;
  UINT64               ReaperOid;
  UINT32               TestType;
  UINT32               MaxFileSystems;
  UINT64               FileSystemOid[APFS_NX_MAX_FILE_SYSTEMS];
  UINT64               Counters[APFS_NX_NUM_COUNTERS];
  APFS_PHYSICAL_RANGE  BlockedOutPhysicalRange;
  UINT64               EvictMappingTreeOid;
  UINT64               Flags;
  //
  // Pointer to JSDR block
  //
  UINT64               EfiJumpStart;
  EFI_GUID             FusionUuid;
  APFS_PHYSICAL_RANGE  KeyLocker;
  UINT64               EphermalInfo[APFS_NX_EPH_INFO_COUNT];
  UINT64               TestOid;
  UINT64               FusionMtIod;
  UINT64               FusionWbcOid;
  APFS_PHYSICAL_RANGE  FusionWbc;
} APFS_NX_SUPERBLOCK;

STATIC_ASSERT (sizeof (APFS_NX_SUPERBLOCK) == 1384, "APFS_NX_SUPERBLOCK has unexpected size");

/**
  APSB volume header structure
**/
typedef struct APFS_APFS_SUPERBLOCK_ {
  APFS_OBJ_PHYS              BlockHeader;
  //
  // Volume Superblock magic
  // Magic: APSB
  //
  UINT32                     Magic;
  //
  // Volume#. First volume start with 0, (0x00)
  //
  UINT32                     FsIndex;
  UINT64                     Features;
  UINT64                     ReadOnlyCompatibleFeatures;
  UINT64                     IncompatibleFeatures;
  UINT64                     UnmountTime;
  //
  // Size of volume in blocks. Last volume has no
  // size set and has available the rest of the blocks
  //
  UINT64                     FsReserveBlockCount;
  UINT64                     FsQuotaBlockCount;
  //
  // Blocks in use in this volume.
  //
  UINT64                     FsAllocCount;
  APFS_WRAPPED_CRYPTO_STATE  MetaCrypto;
  UINT32                     RootTreeType;
  UINT32                     ExtentrefTreeType;
  UINT32                     SnapMetaTreeType;
  UINT64                     OmapOid;
  UINT64                     RootTreeId;
  UINT64                     ExtentrefTreeOid;
  UINT64                     SnapMetaTreeOid;
  UINT64                     RevertToXid;
  UINT64                     RevertToSblockOid;
  UINT64                     NextObjId;
  UINT64                     NumFiles;
  UINT64                     NumDirectories;
  UINT64                     NumSymlinks;
  UINT64                     NumOtherFsObjects;
  UINT64                     NumSnapshots;
  UINT64                     TotalBlocksAllocated;
  UINT64                     TotalBlocksFreed;
  GUID                       VolumeUuid;
  UINT64                     LastModTime;
  UINT64                     FsFlags;
  APFS_MODIFIED_BY           FormattedBy;
  APFS_MODIFIED_BY           ModifiedBy[APFS_MAX_HIST];
  UINT8                      VolumeName[APFS_VOLNAME_LEN];
  UINT32                     NextDocId;
  UINT16                     Role;
  UINT16                     Reserved;
  UINT64                     RootToXid;
  UINT64                     ErStateOid;
} APFS_APFS_SUPERBLOCK;

/**
  JSDR block structure
**/
typedef struct APFS_NX_EFI_JUMPSTART_ {
  APFS_OBJ_PHYS        BlockHeader;
  //
  // A number that can be used to verify that you're reading an instance of
  // APFS_EFI_BOOT_RECORD
  //
  UINT32               Magic;
  //
  // The version of this data structure
  //
  UINT32               Version;
  //
  // The length in bytes, of the embedded EFI driver
  //
  UINT32               EfiFileLen;
  //
  // Num of extents in the array
  //
  UINT32               NumExtents;
  //
  // Reserved
  // Populate this field with 0 when you create a new instance,
  // and preserve its value when you modify an existing instance.
  //
  UINT64               Reserved[16];
  //
  // Apfs driver physical range location
  //
  APFS_PHYSICAL_RANGE  RecordExtents[];
} APFS_NX_EFI_JUMPSTART;

#pragma pack(pop)

#endif // APFS_H

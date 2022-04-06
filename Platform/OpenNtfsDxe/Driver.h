/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#ifndef DRIVER_H
#define DRIVER_H

#define NTFS_MAX_MFT            4096
#define NTFS_MAX_IDX            16384
#define COMPRESSION_BLOCK       4096
#define NTFS_DRIVER_VERSION     0x00020000
#define MAX_PATH                1024
#define MINIMUM_INFO_LENGTH     (sizeof (EFI_FILE_INFO) + MAX_PATH * sizeof(CHAR16))
#define MINIMUM_FS_INFO_LENGTH  (sizeof (EFI_FILE_SYSTEM_INFO) + MAX_PATH * sizeof(CHAR16))
#define ATTRIBUTES_END_MARKER   0xFFFFFFFF
#define MAX_FILE_SIZE           (MAX_UINT32 & ~7ULL)

/**
  ************
  *** FILE ***
  ************
  * Everything on an NTFS volume is a File. Files are composed of Attributes
    (usually 4: $STANDARD_INFORMATION, $FILE_NAME, $SECURITY_DESCRIPTOR, $DATA).

  * Information about all the Attributes available in a volume is stored in
    a File named $AttrDef.

  * File named $MFT (Master File Table) is an index of every File on the volume.
    The description of each File is packed into FILE Records.
    A FILE Record is built up from a header, several variable length Attributes
    and an end marker (simply 0xFFFFFFFF).
    The first FILE Record that describes a given file is called
    the Base FILE Record and the others are called Extension FILE Records.
    Only the Base FILE Record is used for referencing the File it describes.
    Attribute named $ATTRIBUTE_LIST provides the references
    to all the Extension FILE Records.

  * The $MFT lists the Boot Sector File ($Boot), located at the beginning of the disk.
    $Boot also lists where to find the $MFT. The $MFT also lists itself.

  * Every Attribute has a standard header and specific fields.
    The header stores information about the Attribute's type, size,
    name (optional) and whether it is resident (in $MFT), or not.

  * Non-resident Attributes are stored in intervals of Clusters called Data Runs.
    Each Data Run is represented by its starting Cluster and its length.

  * The number of sectors that make up a Cluster is always a power of 2,
    and this number is fixed when the volume is formatted.
    The Cluster Size for a volume is stored in a File named $Boot.

  * Each Cluster in a volume is given a sequential number.
    This is its Logical Cluster Number.
    LCN 0 (zero) refers to the first cluster in the volume (the Boot Sector).

  * Each Cluster of a non-resident stream is given a sequential number.
    This is its Virtual Cluster Number.
    VCN 0 (zero) refers to the first cluster of the stream.

  *************
  *** INDEX ***
  *************
  * Under NTFS every object on the volume is a file, even directories.
    A directory is a sequence of Index Entries containing a filename Attribute.
    An Index Entry is created for each $FILE_NAME Attribute
    of each file contained in the folder.

  * $INDEX_ROOT Attribute is the root node of the B+ tree that implements
    an index (e.g. a directory). This file attribute is always resident.

  * Every Index Record has a standard header and a set of blocks containing
    an Index Key and Index Data. The size of an Index Record is defined
    in $Boot File and always seems to be 4KB.

  * $INDEX_ALLOCATION Attribute is the storage location for all sub-nodes
    of the B+ tree that implements an index (e.g. a directory). It is simply a
    sequence of all Index Records that belong to the Index.
    This file attribute is never resident.

  * A sequence of Index Entries follows each INDEX_HEADER structure.
    Together they make up a complete index. The index follows either
    an $INDEX_ROOT Attribute or an $INDEX_ALLOCATION Attribute.

  ***************
  *** RUNLIST ***
  ***************
  * Runlist is a table written in the content part of a non-resident file Attribute,
    which allows to have access to its stream. (Table 4.10.)

  * The Runlist is a sequence of elements: each element stores an offset to the
    starting LCN of the previous element and the length in clusters of a Data Run.

  * The layout of the Runlist must take account of the data compression:
    the set of VCNs containing the stream of a compressed file attribute
    is divided in compression units (also called chunks) of 16 clusters.

  * Data are compressed using a modified LZ77 algorithm.
    The basic idea is that substrings of the block which have been seen before
    are compressed by referencing the string rather than mentioning it again.

  * Only $DATA Attribute can be compressed, or sparse, and only when it is non-resident.
**/

//
// Table 2.1. Standard NTFS Attribute Types
//
enum {
  AT_STANDARD_INFORMATION = 0x10,
  AT_ATTRIBUTE_LIST       = 0x20,
  AT_FILENAME             = 0x30,
  AT_VOLUME_VERSION       = 0x40,
  AT_SECURITY_DESCRIPTOR  = 0x50,
  AT_VOLUME_NAME          = 0x60,
  AT_VOLUME_INFORMATION   = 0x70,
  AT_DATA                 = 0x80,
  AT_INDEX_ROOT           = 0x90,
  AT_INDEX_ALLOCATION     = 0xA0,
  AT_BITMAP               = 0xB0,
  AT_SYMLINK              = 0xC0,
  AT_EA_INFORMATION       = 0xD0,
  AT_EA                   = 0xE0,
  AT_PROPERTY_SET         = 0xF0,
};

//
// Table 2.6. File Flags (Also called attributes in DOS terminology).
//
enum {
  ATTR_READ_ONLY   = 0x1,
  ATTR_HIDDEN      = 0x2,
  ATTR_SYSTEM      = 0x4,
  ATTR_ARCHIVE     = 0x20,
  ATTR_DEVICE      = 0x40,
  ATTR_NORMAL      = 0x80,
  ATTR_TEMPORARY   = 0x100,
  ATTR_SPARSE      = 0x200,
  ATTR_REPARSE     = 0x400,
  ATTR_COMPRESSED  = 0x800,
  ATTR_OFFLINE     = 0x1000,
  ATTR_NOT_INDEXED = 0x2000,
  ATTR_ENCRYPTED   = 0x4000,
  ATTR_DIRECTORY   = 0x10000000,
  ATTR_INDEX_VIEW  = 0x20000000
};

//
// Table 3.1. Layout of files on the Volume (Inodes).
//
enum {
  MFT_FILE     =  0,
  MFTMIRR_FILE =  1,
  LOGFILE_FILE =  2,
  VOLUME_FILE  =  3,
  ATTRDEF_FILE =  4,
  ROOT_FILE    =  5,
  BITMAP_FILE  =  6,
  BOOT_FILE    =  7,
  BADCLUS_FILE =  8,
  QUOTA_FILE   =  9,
  UPCASE_FILE  = 10,
};

enum {
  NTFS_AF_ALST     = 1,
  NTFS_AF_MFT_FILE = 2,
  NTFS_AF_GPOS     = 4,
};

//
// Table 4.6. Attribute flags
//
enum {
  FLAG_COMPRESSED = 0x0001,
  FLAG_ENCRYPTED  = 0x4000,
  FLAG_SPARSE     = 0x8000
};

//
// Table 4.22. File record flags
//
enum {
  IS_IN_USE      = 0x01,
  IS_A_DIRECTORY = 0x02,
};

//
// Table 2.30. Data entry flags
//
enum {
  SUB_NODE         = 0x01,
  LAST_INDEX_ENTRY = 0x02,
};

//
// 13.2. Possible Namespaces
//
enum {
  POSIX     = 0,
  WINDOWS32 = 1,
  DOS       = 2,
  WIN32_DOS = 3,
};

//
// Table 2.36. Reparse Tag Flags
//
enum {
  IS_ALIAS        = 0x20000000,
  IS_HIGH_LATENCY = 0x40000000,
  IS_MICROSOFT    = 0x80000000,
  NSS             = 0x68000005,
  NSS_RECOVER     = 0x68000006,
  SIS             = 0x68000007,
  DFS             = 0x68000008,
  MOUNT_POINT     = 0x88000003,
  HSM             = 0xA8000004,
  SYMBOLIC_LINK   = 0xE8000000,
};

#pragma pack(1)
/**
   Table 4.21. Layout of a File Record
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x00   | 4    | Magic number 'FILE'
   0x04   | 2    | Offset to the update sequence
   0x06   | 2    | Size in words of Update Sequence Number & Array (S)
   0x08   | 8    | $LogFile Sequence Number (LSN)
   0x10   | 2    | Sequence number
   0x12   | 2    | Hard link count
   0x14   | 2    | Offset to the first Attribute
   0x16   | 2    | Flags
   0x18   | 4    | Real size of the FILE record
   0x1C   | 4    | Allocated size of the FILE record (a multiple of the Cluster size)
   0x20   | 8    | File reference to the base FILE record
   0x28   | 2    | Next Attribute Id
          | 2    | Update Sequence Number (a)
          | 2S-2 | Update Sequence Array (a)
   --------------------------------------------------------------------
**/
typedef struct {
  CHAR8  Magic[4];
  UINT16 UpdateSequenceOffset;
  UINT16 S_Size;
  UINT64 LSN;
  UINT16 SequenceNumber;
  UINT16 HardLinkCount;
  UINT16 AttributeOffset;
  UINT16 Flags;
  UINT32 RealSize;
  UINT32 AllocatedSize;
  UINT64 BaseFileRecord;
  UINT16 NextAttributeId;
} FILE_RECORD_HEADER;

/**
   Table 4.3. Layout of a resident named Attribute Header
   ____________________________________________________________________
   Offset | Size | Value | Description
   --------------------------------------------------------------------
   0x00   | 4    |       | Attribute Type (e.g. 0x90, 0xB0)
   0x04   | 4    |       | Length (including this header)
   0x08   | 1    | 0x00  | Non-resident flag
   0x09   | 1    | N     | Name length
   0x0A   | 2    | 0x18  | Offset to the Name
   0x0C   | 2    | 0x00  | Flags
   0x0E   | 2    |       | Attribute Id (a)
   0x10   | 4    | L     | Length of the Attribute
   0x14   | 2    |2N+0x18| Offset to the Attribute (b)
   0x16   | 1    |       | Indexed flag
   0x17   | 1    | 0x00  | Padding
   0x18   | 2N   |Unicode| The Attribute's Name
   2N+0x18|  L   |       | The Attribute (c)
   --------------------------------------------------------------------
**/
typedef struct {
  UINT32 Type;
  UINT32 Length;
  UINT8  NonResFlag;
  UINT8  NameLength;
  UINT16 NameOffset;
  UINT16 Flags;
  UINT16 AttributeId;
  UINT32 InfoLength;
  UINT16 InfoOffset;
  UINT8  IndexedFlag;
  UINT8  Padding;
} ATTR_HEADER_RES;

/**
   Table 4.5. Layout of a non-resident named Attribute Header
   ____________________________________________________________________
   Offset | Size | Value | Description
   --------------------------------------------------------------------
   0x00   | 4    |       | Attribute Type (e.g. 0x80, 0xA0)
   0x04   | 4    |       | Length (including this header)
   0x08   | 1    | 0x01  | Non-resident flag
   0x09   | 1    | N     | Name length
   0x0A   | 2    | 0x40  | Offset to the Name
   0x0C   | 2    |       | Flags
   0x0E   | 2    |       | Attribute Id (a)
   0x10   | 8    |       | Starting VCN
   0x18   | 8    |       | Last VCN
   0x20   | 2    |2N+0x40| Offset to the Data Runs (b)
   0x22   | 2    |       | Compression Unit Size (c)
   0x24   | 4    | 0x00  | Padding
   0x28   | 8    |       | Allocated size of the attribute (a multiple of the Cluster size)
   0x30   | 8    |       | Real size of the attribute
   0x38   | 8    |       | Initialized data size of the stream (e)
   0x40   | 2N   |Unicode| The Attribute's Name
   2N+0x40| ...  |       | Data Runs (b)
   --------------------------------------------------------------------
**/
typedef struct {
  UINT32 Type;
  UINT32 Length;
  UINT8  NonResFlag;
  UINT8  NameLength;
  UINT16 NameOffset;
  UINT16 Flags;
  UINT16 AttributeId;
  UINT64 StartingVCN;
  UINT64 LastVCN;
  UINT16 DataRunsOffset;
  UINT16 CompressionUnitSize;
  UINT32 Padding;
  UINT64 AllocatedSize;
  UINT64 RealSize;
  UINT64 InitializedDataSize;
} ATTR_HEADER_NONRES;

/**
   Table 2.4. Layout of the $ATTRIBUTE_LIST (0x20) attribute
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   ~      | ~    | Standard Attribute Header
   0x00   | 4    | Type
   0x04   | 2    | Record length
   0x06   | 1    | Name length (N)
   0x07   | 1    | Offset to Name (a)
   0x08   | 8    | Starting VCN (b)
   0x10   | 8    | Base File Reference of the attribute (Table 4.23)
   0x18   | 2    | Attribute Id (c)
   0x1A   | 2N   | Name in Unicode (if N > 0)
   --------------------------------------------------------------------
**/
typedef struct {
  UINT32 Type;
  UINT16 RecordLength;
  UINT8  NameLength;
  UINT8  NameOffset;
  UINT64 StartingVCN;
  UINT64 BaseFileReference;
  UINT16 AttributeId;
} ATTR_LIST_RECORD;

/**
 * Table 4.23. Layout of a file reference
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x00   | 6    | FILE record number
   0x06   | 2    | Sequence number
   --------------------------------------------------------------------

 * Table 2.29. Index Entry
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x00   | 8    | File reference
   0x08   | 2    | Length of the index entry (L)
   0x0A   | 2    | Length of the stream (S)
   0x0C   | 1    | Flags
   --------------------------------------------------------------------
   The next field is only present when the last entry flag is not set
   --------------------------------------------------------------------
   0x10   | S    | Stream - A copy of the body of the indexed attribute
   0x10+S | ?    | Align 8
   --------------------------------------------------------------------
   The next field is only present when the sub-node flag is set
   --------------------------------------------------------------------
   L-8    | 8    | VCN of the sub-node in the Index Allocation
   --------------------------------------------------------------------
**/
typedef struct {
  UINT8  FileRecordNumber[6];
  UINT16 SequenceNumber;
  UINT16 IndexEntryLength;
  UINT16 StreamLength;
  UINT8  Flags;
  UINT8  Padding[3];
} INDEX_ENTRY;

/**
   Table 4.26. Layout of a Standard Index Header
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x00   | 4    | Magic number 'INDX'
   0x04   | 2    | Offset to the Update Sequence
   0x06   | 2    | Size in words of the Update Sequence (S)
   0x08   | 8    | $LogFile sequence number
   0x10   | 8    | VCN of this Index record in the Index Allocation
   0x18   | 4    | Offset to the Index Entries (a)
   0x1C   | 4    | Size of Index Entries (a)
   0x20   | 4    | Allocated size of the Index Entries (a)
   0x24   | 1    | 1 if not leaf node (b)
   0x25   | 3    | Padding (always zero)
   0x28   | 2    | Update sequence
   0x2A   | 2S-2 | Update sequence array
   X      | Y    | Index Entry
   X+Y    | Z    | Next Index Entry
   ~      | ~    | ...
   --------------------------------------------------------------------
   (a) These values are relative to 0x18
**/
typedef struct {
  CHAR8  Magic[4];
  UINT16 UpdateSequenceOffset;
  UINT16 S_Size;
  UINT64 LSN;
  UINT64 IndexRecordVCN;
} INDEX_HEADER;

typedef struct {
  INDEX_HEADER Header;
  UINT32       IndexEntriesOffset;
  UINT32       IndexEntriesSize;
  UINT32       IndexEntriesAllocated;
  UINT8        IsNotLeafNode;
  UINT8        Padding[3];
} INDEX_RECORD_HEADER;

/**
 * Table 2.24. Layout of the $INDEX_ROOT (0x90) Attribute (Index Root)
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   ~      | ~    | Standard Attribute Header
   0x00   | 4    | Attribute Type
   0x04   | 4    | Collation Rule
   0x08   | 4    | Size of Index Allocation Entry (bytes)
   0x0C   | 1    | Clusters per Index Record
   0x0D   | 3    | Padding (Align to 8 bytes)
   --------------------------------------------------------------------

 * Table 2.25. Layout of the $INDEX_ROOT (0x90) Attribute (Index Header)
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x00   | 4    | Offset to first Index Entry
   0x04   | 4    | Total size of the Index Entries
   0x08   | 4    | Allocated size of the Index Entries
   0x0C   | 1    | Flags
   0x0D   | 3    | Padding (align to 8 bytes)
   --------------------------------------------------------------------
**/
typedef struct {
  UINT32 Type;
  UINT32 CollationRule;
  UINT32 IndexAllocationSize;
  UINT8  IndexRecordClusters;
  UINT8  Padding[3];
} INDEX_ROOT;

typedef struct {
  INDEX_ROOT Root;
  UINT32     FirstEntryOffset;
  UINT32     EntriesTotalSize;
  UINT32     EntriesAllocatedSize;
  UINT8      Flags;
  UINT8      Padding[3];
} ATTR_INDEX_ROOT;

/**
   Table 2.5. Layout of the $FILE_NAME (0x30) attribute
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   ~      | ~    | Standard Attribute Header
   0x00   | 8    | File reference to the base record of the parent directory.
   0x08   | 8    | C Time - File Creation
   0x10   | 8    | A Time - File Altered
   0x18   | 8    | M Time - MFT Changed
   0x20   | 8    | R Time - File Read
   0x28   | 8    | Allocated size of the file (a multiple of the Cluster size)
   0x30   | 8    | Real size of the file (size of the unnamed $Data Attribute)
   0x38   | 4    | Flags, e.g. Directory, compressed, hidden
   0x3c   | 4    | Used by EAs and Reparse
   0x40   | 1    | Filename length in characters (L)
   0x41   | 1    | Filename namespace
   0x42   | 2L   | File name in Unicode (not null terminated)
   --------------------------------------------------------------------
**/
typedef struct {
  UINT64 ParentDir;
  UINT64 CreationTime;
  UINT64 AlteredTime;
  UINT64 ChangedMftTime;
  UINT64 ReadTime;
  UINT64 AllocatedSize;
  UINT64 RealSize;
  UINT32 Flags;
  UINT32 Reparse;
  UINT8  FilenameLen;
  UINT8  Namespace;
} ATTR_FILE_NAME;

typedef struct {
  INT32             Flags;
  UINT8             *ExtensionMftRecord;
  UINT8             *NonResAttrList;
  UINT8             *Current;
  UINT8             *Next;
  UINT8             *Last;
  struct _NTFS_FILE *BaseMftRecord;
} NTFS_ATTR;

typedef struct _NTFS_FILE {
  UINT8                  *FileRecord;
  UINT64                 DataAttributeSize;
  UINT64                 AlteredTime;
  UINT64                 Inode;
  BOOLEAN                InodeRead;
  NTFS_ATTR              Attr;
  struct _EFI_NTFS_FILE  *File;
} NTFS_FILE;

typedef struct _EFI_NTFS_FILE {
  EFI_FILE_PROTOCOL      EfiFile;
  BOOLEAN                IsDir;
  INT64                  DirIndex;
  INT32                  Mtime;
  CHAR16                 *Path;
  CHAR16                 *BaseName;
  UINT64                 Offset;
  UINT32                 RefCount;
  NTFS_FILE              RootFile;
  NTFS_FILE              MftFile;
  struct _EFI_FS         *FileSystem;
} EFI_NTFS_FILE;

typedef struct _EFI_FS {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL FileIoInterface;
  EFI_FILE_PROTOCOL               EfiFile;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_DISK_IO_PROTOCOL            *DiskIo;
  EFI_DEVICE_PATH                 *DevicePath;
  UINT64                          FirstMftRecord;
  NTFS_FILE                       *RootIndex;
  NTFS_FILE                       *MftStart;
} EFI_FS;

/**
   Table 3.19. Layout of the $Boot File's $DATA Attribute
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x0000 | 3    | Jump to the boot loader routine
   0x0003 | 8    | System Id: "NTFS    "
   0x000B | 2    | Bytes per sector
   0x000D | 1    | Sectors per cluster
   0x000E | 7    | Unused
   0x0015 | 1    | Media descriptor (a)
   0x0016 | 2    | Unused
   0x0018 | 2    | Sectors per track
   0x001A | 2    | Number of heads
   0x001C | 8    | Unused
   0x0024 | 4    | Usually 80 00 80 00 (b)
   0x0028 | 8    | Number of sectors in the volume
   0x0030 | 8    | LCN of VCN 0 of the $MFT
   0x0038 | 8    | LCN of VCN 0 of the $MFTMirr
   0x0040 | 4    | Clusters per MFT Record (c)
   0x0044 | 4    | Clusters per Index Record (c)
   0x0048 | 8    | Volume serial number
   ~      | ~    | ~
   0x0200 |      | Windows NT Loader
   --------------------------------------------------------------------
**/
typedef struct {
  UINT8  BootLoaderJump[3];
  UINT8  SystemId[8];
  UINT16 BytesPerSector;
  UINT8  SectorsPerCluster;
  UINT8  Unused1[7];
  UINT8  MediaDescriptor;
  UINT16 Unused2;
  UINT16 SectorsPerTrack;
  UINT16 HeadsNumber;
  UINT64 Unused3;
  UINT32 Usually;
  UINT64 VolumeSectorsNumber;
  UINT64 MftLcn;
  UINT64 MftMirrLcn;
  INT8   MftRecordClusters;
  UINT8  Unused4[3];
  INT8   IndexRecordClusters;
  UINT8  Unused5[3];
  UINT64 VolumeSerialNumber;
} BOOT_FILE_DATA;

typedef struct {
  UINT64 Vcn;
  UINT64 Lcn;
} UNIT_ELEMENT;

typedef struct {
  EFI_FS        *FileSystem;
  UINT8         Head;
  UINT8         Tail;
  UNIT_ELEMENT  Elements[16];
  UINT64        CurrentVcn;
  UINT8         *Cluster;
  UINT64        ClusterOffset;
  UINT64        SavedPosition;
  UINT8         *ClearTextBlock;
} COMPRESSED;

typedef struct {
  BOOLEAN    IsSparse;
  UINT64     TargetVcn;
  UINT64     CurrentVcn;
  UINT64     CurrentLcn;
  UINT64     NextVcn;
  UINT8      *NextDataRun;
  NTFS_ATTR  *Attr;
  COMPRESSED Unit;
} RUNLIST;

/**
 * Table 2.32. Layout of the $REPARSE_POINT (0xC0) attribute (Microsoft Reparse Point)
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   ~      | ~    | Standard Attribute Header
   0x00   | 4    | Reparse Type (and Flags)
   0x04   | 2    | Reparse Data Length
   0x06   | 2    | Padding (align to 8 bytes)
   0x08   | V    | Reparse Data (a)
   --------------------------------------------------------------------

 * Table 2.34. Symbolic Link Reparse Data
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x00   | 2    | Substitute Name Offset
   0x02   | 2    | Substitute Name Length
   0x04   | 2    | Print Name Offset
   0x08   | 2    | Print Name Length
   0x10   | V    | Path Buffer
   --------------------------------------------------------------------
**/
typedef struct {
  UINT32 Type;
  UINT16 DataLength;
  UINT16 Padding;
  UINT16 SubstituteOffset;
  UINT16 SubstituteLength;
  UINT16 PrintOffset;
  UINT16 PrintLength;
} SYMLINK;
#pragma pack()

#endif // DRIVER_H

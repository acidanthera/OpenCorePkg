/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_FILE_LIB_H
#define OC_FILE_LIB_H

// Include the abstracted protocol for its definitions

#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DevicePath.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>

/**
  Maximum safe volume label size.
**/
#define OC_MAX_VOLUME_LABEL_SIZE 64

/**
  Locate file system from Device handle or path.

  @param[in]  DeviceHandle  Device handle.
  @param[in]  FilePath      Device path.

  @retval  simple file system protocol or NULL.
**/
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
LocateFileSystem (
  IN  EFI_HANDLE                         DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath     OPTIONAL
  );

/**
  Locate root volume from Device handle or path.

  @param[in]  DeviceHandle  Device handle.
  @param[in]  FilePath      Device path.

  @retval  opened file protocol or NULL.
**/
EFI_FILE_PROTOCOL *
LocateRootVolume (
  IN  EFI_HANDLE                         DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath     OPTIONAL
  );

/**
  Locate file system from GUID.

  @param[in]  Guid  GUID of the volume to locate.

  @retval  simple file system protocol or NULL.
**/
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
LocateFileSystemByGuid (
  IN CONST GUID  *Guid
  );

/**
  Retrieves volume label.

  @param[in]  FileSystem   A pointer to the file system protocol of the volume.

  @retval A pointer to the NULL terminated unicode volume label.
**/
CHAR16 *
GetVolumeLabel (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem
  );

/**
  Opens a new file relative to the source file's location.
  This function is equivalent to EFI_FILE_OPEN but has additional restrictions
  to provide board compatibility. Currently the only restriction is
  no trailing slash in the filename due to issues in FAT drivers.

  - Multiple boards, namely ASUS P8H61-M and P8H61-M LX2 will not
    open directories with trailing slash. It is irrelevant whether
    front slash is present for them.
    For example, it means that L"EFI\\OC\\" or L"\\EFI\\OC\\" will both
    fail to open, while L"EFI\\OC" and L"\\EFI\\OC" will open fine.
  - Most newer boards on APTIO IV do handle directories with trailing
    slash, however, their driver will modify passed string by removing
    the slash by \0.

  @param  Protocol   File protocol instance.
  @param  NewHandle  Pointer for returned handle.
  @param  FileName   Null-terminated file name.
  @param  OpenMode   File open mode.
  @param  Attributes Attributes for the newly created file.

  @retval EFI_SUCCESS for successfully opened file.
*/
EFI_STATUS
SafeFileOpen (
  IN  EFI_FILE_PROTOCOL       *Protocol,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CONST CHAR16            *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes
  );

/**
  Read file from device path with implicit double (2 byte) null termination.
  Null termination does not affect the returned file size.
  Depending on the implementation 0 byte files may return null.

  @param[in]  FileSystem   A pointer to the file system protocol of the volume.
  @param[in]  FilePath     The full path to the file on the device.
  @param[out] FileSize     The size of the file read (optional).
  @param[in]  MaxFileSize  Upper file size bound (optional).

  @retval A pointer to a buffer containing file read or NULL.
**/
VOID *
ReadFile (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *FilePath,
  OUT UINT32                           *FileSize OPTIONAL,
  IN  UINT32                           MaxFileSize OPTIONAL
  );

/**
  Determine file size if it is less than 4 GB.

  @param[in]  FileSystem   A pointer to the file system protocol of the volume.
  @param[in]  FilePath     The full path to the file on the device.
  @param[out] Size         32-bit file size.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
ReadFileSize (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *FilePath,
  OUT UINT32                           *Size
  );

/**
  Read exact amount of bytes from EFI_FILE_PROTOCOL at specified position.

  @param[in]  File         A pointer to the file protocol.
  @param[in]  Position     Position to read data from.
  @param[in]  Size         The size of the data read.
  @param[out] Buffer       A pointer to previously allocated buffer to read data to.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
GetFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  );

/**
  Write exact amount of bytes to a newly created file in EFI_FILE_PROTOCOL.
  Please note, that several filesystems (or drivers) may limit file name length.

  @param[in] WritableFs   A pointer to the file protocol, any will be tried if NULL.
  @param[in] FileName     File name (possibly with path) to write.
  @param[in] Buffer       A pointer with the data to be written.
  @param[in] Size         Amount of data to be written.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
SetFileData (
  IN EFI_FILE_PROTOCOL  *WritableFs OPTIONAL,
  IN CONST CHAR16       *FileName,
  IN CONST VOID         *Buffer,
  IN UINT32             Size
  );

/**
  Read all bytes from EFI_FILE_PROTOCOL and return a buffer.

  @param[in]  File         A pointer to the file protocol.
  @param[out] Buffer       A pointer for the returned buffer.
  @param[out] BufferSize   A pointer for the size of the returned buffer.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
AllocateCopyFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT8              **Buffer,
  OUT UINT32             *BufferSize
  );

/**
  Get file information of specified type.

  @param[in]  File               A pointer to file handle.
  @param[in]  InformationType    A pointer to file info GUID.
  @param[in]  MinFileInfoSize    Minimal size of the info provided.
  @param[out] RealFileInfoSize   Actual info size read (optional).

  @retval read file info or NULL.
**/
VOID *
GetFileInfo (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  EFI_GUID           *InformationType,
  IN  UINTN              MinFileInfoSize,
  OUT UINTN              *RealFileInfoSize  OPTIONAL
  );

/**
  Determine file size if it is less than 4 GB.

  @param[in]  File         A pointer to the file protocol.
  @param[out] Size         32-bit file size.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
GetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  );

/**
  Determine file modification time.

  @param[in]  File         A pointer to the file protocol.
  @param[out] Time         Modification time.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
GetFileModificationTime (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT EFI_TIME           *Time
  );

/**
  Determine writeable filesystem.

  @param[in,out]  WritableFs   First found writeable file system.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
FindWritableFileSystem (
  IN OUT EFI_FILE_PROTOCOL  **WritableFs
  );

/**
  Open a file or directory by file system handle and path.
  See OcOpenFileByDevicePath() for more details.

  @param[in]  FileSystemHandle     File System handle.
  @param[in]  RemainingDevicePath  The remaining Device Path (must be all file
                                   path nodes).
  @param[out] File                 Resulting file protocol.
  @param[in]  OpenMode             File open mode.
  @param[in]  Attributes           File attributes.

  @retval EFI_SUCCESS on succesful open.
**/
EFI_STATUS
EFIAPI
OcOpenFileByRemainingDevicePath (
  IN  EFI_HANDLE                      FileSystemHandle,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath,
  OUT EFI_FILE_PROTOCOL               **File,
  IN  UINT64                          OpenMode,
  IN  UINT64                          Attributes
  );

/**
  Open a file or directory by device path. This is a modified
  version of EfiOpenFileByDevicePath function, which handles paths
  with trailing slashes, that cause Open failure on old firmwares.
  EfiOpenFileByDevicePath is additionally not available in UDK.

  See more details at:
  https://github.com/tianocore/edk2/commit/768b611136d0f2b99a99e446c089d1a30c3fa5d5

  @param[in,out]  FilePath    Device path protocol.
  @param[out]     File        Resulting file protocol.
  @param[in]      OpenMode    File open mode.
  @param[in]      Attributes  File attributes.

  @retval EFI_SUCCESS on succesful open.
**/
EFI_STATUS
EFIAPI
OcOpenFileByDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  OUT    EFI_FILE_PROTOCOL         **File,
  IN     UINT64                    OpenMode,
  IN     UINT64                    Attributes
  );

/**
  Retrieve the disk's device handle from a partition's Device Path.

  @param[in] HdDevicePath  The Device Path of the partition.

  @retval device handle or NULL
**/
EFI_HANDLE
OcPartitionGetDiskHandle (
  IN EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath
  );

/**
  Locate the disk's EFI System Partition.

  @param[in]  DiskDevicePath     The Device Path of the disk to scan.
  @param[out] EspDevicePathSize  The size of the returned Device Path.
  @param[out] EspDeviceHandle    Device handle of the returned partition.

  @return The device path protocol from the discovered handle or NULL.
**/
EFI_DEVICE_PATH_PROTOCOL *
OcDiskFindSystemPartitionPath (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *DiskDevicePath,
  OUT UINTN                           *EspDevicePathSize,
  OUT EFI_HANDLE                      *EspDeviceHandle
  );

/**
  Disk I/O context.
**/
typedef struct {
  EFI_BLOCK_IO_PROTOCOL      *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL     *BlockIo2;
  UINT32                     MediaId;
  UINT32                     BlockSize;
} OC_DISK_CONTEXT;

/**
  Initialize disk I/O context.

  @param[out]  Context      Disk I/O context to intialize.
  @param[in]   DiskHandle   Disk handle with protocols.
  @param[in]   UseBlockIo2  Try to use BlockIo2 protocol if available.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcDiskInitializeContext (
  OUT OC_DISK_CONTEXT  *Context,
  IN  EFI_HANDLE       DiskHandle,
  IN  BOOLEAN          UseBlockIo2
  );

/**
  Read information from disk.

  @param[in]  Context     Disk I/O context.
  @param[in]  Lba         LBA number to read from.
  @param[in]  BufferSize  Buffer size allocated in Buffer.
  @param[out] Buffer      Buffer to store data in.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcDiskRead (
  IN  OC_DISK_CONTEXT  *Context,
  IN  UINT64           Lba,
  IN  UINTN            BufferSize,
  OUT VOID             *Buffer
  );

/**
  OC partition list.
**/
typedef struct {
  UINT32              NumPartitions;
  UINT32              PartitionEntrySize;
  EFI_PARTITION_ENTRY FirstEntry[];
} OC_PARTITION_ENTRIES;

/**
  Retrieve the disk GPT partitions, if applicable.

  @param[in]  DiskHandle   Disk device handle to retrive partition table from.
  @param[in]  UseBlockIo2  Use 2nd revision of Block I/O if available.

  @retval partition entry list or NULL.
**/
CONST OC_PARTITION_ENTRIES *
OcGetDiskPartitions (
  IN EFI_HANDLE  DiskHandle,
  IN BOOLEAN     UseBlockIo2
  );

/**
  Retrieve the partition's GPT information, if applicable.
  Calls to this function undergo internal lazy caching.

  @param[in] FsHandle  The device handle of the partition to retrieve info of.

  @retval partition entry or NULL
**/
CONST EFI_PARTITION_ENTRY *
OcGetGptPartitionEntry (
  IN EFI_HANDLE  FsHandle
  );

/**
  Unblocks all partition handles without a File System protocol attached from
  driver connection, if applicable.
**/
VOID
OcUnblockUnmountedPartitions (
  VOID
  );

/**
  Creates a device path for a firmware file.

  @param[in]  FileGuid  Firmware file GUID.

  @retval device path allocated from pool on success.
  @retval NULL on failure (e.g. when a file is not present).
**/
EFI_DEVICE_PATH_PROTOCOL *
CreateFvFileDevicePath (
  IN EFI_GUID  *FileGuid
  );

#endif // OC_FILE_LIB_H

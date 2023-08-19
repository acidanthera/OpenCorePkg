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

#include <IndustryStandard/Mbr.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DevicePath.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>

/**
  Maximum safe volume label size.
**/
#define OC_MAX_VOLUME_LABEL_SIZE  64

/**
  Maximum safe content flavour size.
**/
#define OC_MAX_CONTENT_FLAVOUR_SIZE  64

typedef struct {
  UINT32    PreviousTime;
  UINTN     PreviousIndex;
} DIRECTORY_SEARCH_CONTEXT;

/**
  Locate file system from Device handle or path.

  @param[in]  DeviceHandle  Device handle.
  @param[in]  FilePath      Device path.

  @retval  simple file system protocol or NULL.
**/
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
OcLocateFileSystem (
  IN  EFI_HANDLE                DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath     OPTIONAL
  );

/**
  Locate root volume from Device handle or path.

  @param[in]  DeviceHandle  Device handle.
  @param[in]  FilePath      Device path.

  @retval  opened file protocol or NULL.
**/
EFI_FILE_PROTOCOL *
OcLocateRootVolume (
  IN  EFI_HANDLE                DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath     OPTIONAL
  );

/**
  Locate file system from GUID.

  @param[in]  Guid  GUID of the volume to locate.

  @retval  simple file system protocol or NULL.
**/
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
OcLocateFileSystemByGuid (
  IN CONST GUID  *Guid
  );

/**
  Retrieves volume label.

  @param[in]  FileSystem   A pointer to the file system protocol of the volume.

  @retval A pointer to the NULL terminated unicode volume label.
**/
CHAR16 *
OcGetVolumeLabel (
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

  @param  Directory  File protocol instance of parent directory.
  @param  NewHandle  Pointer for returned handle.
  @param  FileName   Null-terminated file name or relative path.
  @param  OpenMode   File open mode.
  @param  Attributes Attributes for the newly created file.

  @retval EFI_SUCCESS for successfully opened file.
**/
EFI_STATUS
OcSafeFileOpen (
  IN     CONST EFI_FILE_PROTOCOL  *Directory,
  OUT       EFI_FILE_PROTOCOL     **NewHandle,
  IN     CONST CHAR16             *FileName,
  IN     CONST UINT64             OpenMode,
  IN     CONST UINT64             Attributes
  );

/**
  Report existence of file relative to source file's location.

  @param  Directory  File protocol instance of parent directory.
  @param  FileName   Null-terminated file name or relative path.

  @retval TRUE when file exists.
**/
BOOLEAN
OcFileExists (
  IN    CONST EFI_FILE_PROTOCOL  *Directory,
  IN    CONST CHAR16             *FileName
  );

/**
  Delete child file relative to source file's location.

  @param  Directory  File protocol instance of parent directory.
  @param  FileName   Null-terminated file name or relative path.

  @retval EFI_SUCCESS     File successfully deleted.
  @retval EFI_NOT_FOUND   File was not present.
  @retval other           Other error opening or deleting file.
**/
EFI_STATUS
OcDeleteFile (
  IN EFI_FILE_PROTOCOL  *Directory,
  IN CONST CHAR16       *FileName
  );

/**
  Read file from file system with implicit double (2 byte) null termination.
  Null termination does not affect the returned file size.
  Depending on the implementation 0 byte files may return null.

  @param[in]  FileSystem   A pointer to the file system protocol of the volume.
  @param[in]  FilePath     The full path to the file on the device.
  @param[out] FileSize     The size of the file read (optional).
  @param[in]  MaxFileSize  Upper file size bound (optional).

  @retval A pointer to a buffer containing file read or NULL.
**/
VOID *
OcReadFile (
  IN     CONST EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN     CONST CHAR16                           *FilePath,
  OUT       UINT32                              *FileSize OPTIONAL,
  IN     CONST UINT32                           MaxFileSize OPTIONAL
  );

/**
  Read file from file protocol with implicit double (2 byte) null termination.
  Null termination does not affect the returned file size.
  Depending on the implementation 0 byte files may return null.

  @param[in]  RootDirectory A pointer to the file protocol of the directory.
  @param[in]  FilePath      The full path to the file on the device.
  @param[out] FileSize      The size of the file read (optional).
  @param[in]  MaxFileSize   Upper file size bound (optional).

  @retval A pointer to a buffer containing file read or NULL.
**/
VOID *
OcReadFileFromDirectory (
  IN      CONST EFI_FILE_PROTOCOL  *RootDirectory,
  IN      CONST CHAR16             *FilePath,
  OUT       UINT32                 *FileSize   OPTIONAL,
  IN            UINT32             MaxFileSize OPTIONAL
  );

/**
  Determine file size if it is less than 4 GB.

  @param[in]  FileSystem   A pointer to the file system protocol of the volume.
  @param[in]  FilePath     The full path to the file on the device.
  @param[out] Size         32-bit file size.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcReadFileSize (
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
OcGetFileData (
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
OcSetFileData (
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
OcAllocateCopyFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT8              **Buffer,
  OUT UINT32             *BufferSize
  );

/**
  Initialize DIRECTORY_SEARCH_CONTEXT.

  @param[in,out]  Context     A pointer to the DIRECTORY_SEARCH_CONTEXT.
**/
VOID
OcDirectorySeachContextInit (
  IN OUT DIRECTORY_SEARCH_CONTEXT  *Context
  );

/**
  Gets the next newest file from the specified directory.

  @param[in,out]  Context               Context.
  @param[in]      Directory             The directory EFI_FILE_PROTOCOL instance.
  @param[in]      FileNameStartsWith    Skip files starting with this value.
  @param[out]     FileInfo              EFI_FILE_INFO allocated from pool memory.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcGetNewestFileFromDirectory (
  IN OUT DIRECTORY_SEARCH_CONTEXT  *Context,
  IN     EFI_FILE_PROTOCOL         *Directory,
  IN     CHAR16                    *FileNameStartsWith OPTIONAL,
  OUT EFI_FILE_INFO                **FileInfo
  );

/**
  Ensure specified file is directory or file as specified by IsDirectory.

  @param[in]      File                  The file to check.
  @param[in]      IsDirectory           Require that file is directory.

  @retval EFI_SUCCESS                   File is directory/file as specified.
  @retval EFI_INVALID_PARAMETER         File is not directory/file as specified.
**/
EFI_STATUS
OcEnsureDirectoryFile (
  IN     EFI_FILE_PROTOCOL  *File,
  IN     BOOLEAN            IsDirectory
  );

/**
  Process directory item.

  Note: Successful processing must return EFI_SUCCESS or EFI_NOT_FOUND, or further
  processing will be aborted.

  Return EFI_NOT_FOUND to continue processing but act if no file found.

  @param[in]      Directory             Parent directory file handle.
  @param[in]      FileInfo              EFI_FILE_INFO allocated from pool memory,
                                        will be freed after this call,
                                        data to preserve must be copied.
  @param[in]      FileInfoSize          FileInfoSize.
  @param[in,out]  Context               Optional application-specific context.

  @retval EFI_SUCCESS                   File found and successfully processed.
  @retval EFI_NOT_FOUND                 (Act as if) no matching file was found.
  @retval other                         Error processing file (aborts directory scan).
**/
typedef
EFI_STATUS
(*OC_PROCESS_DIRECTORY_ENTRY) (
  EFI_FILE_HANDLE  Directory,
  EFI_FILE_INFO    *FileInfo,
  UINTN            FileInfoSize,
  VOID             *Context        OPTIONAL
  );

/**
  Scan directory, calling specified procedure for each directory entry.

  @param[in]      Directory             The directory to scan.
  @param[in]      ProcessEntry          Process entry, called for each directory entry matching filter.
  @param[in,out]  Context               Optional application-specific context.

  @retval EFI_NOT_FOUND                 Successful processing, no entries matching filter were found.
  @retval EFI_SUCCESS                   Successful processing, at least one entry matching filter was found.
  @retval EFI_OUT_OF_RESOURCES          Out of memory.
  @retval other                         Other error returned by file system or ProcessEntry during processing
**/
EFI_STATUS
OcScanDirectory (
  IN      EFI_FILE_HANDLE             Directory,
  IN      OC_PROCESS_DIRECTORY_ENTRY  ProcessEntry,
  IN OUT  VOID                        *Context            OPTIONAL
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
OcGetFileInfo (
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
OcGetFileSize (
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
OcGetFileModificationTime (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT EFI_TIME           *Time
  );

/**
  Check if filesystem is writable.

  @param[in]  Fs   File system to check.

  @retval TRUE on success.
**/
BOOLEAN
OcIsWritableFileSystem (
  IN EFI_FILE_PROTOCOL  *Fs
  );

/**
  Find writable filesystem.

  @param[in,out]  WritableFs   First found writeable file system.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcFindWritableFileSystem (
  IN OUT EFI_FILE_PROTOCOL  **WritableFs
  );

/**
  Find writable filesystem from Bootstrap.

  @param[out]  FileSystem   Pointer to first found writeable file system.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcFindWritableOcFileSystem (
  OUT EFI_FILE_PROTOCOL  **FileSystem
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
  with trailing slashes, that cause Open failure on old firmware.
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
  Retrieve the disk's Device Path from a partition's Device Path.

  @param[in] HdDevicePath  The Device Path of the partition.

  @retval Device Path or NULL
**/
EFI_DEVICE_PATH_PROTOCOL *
OcDiskGetDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath
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
  Retrieve the partition's device handle from a partition's Device Path.

  @param[in] HdDevicePath  The Device Path of the partition.

**/
EFI_HANDLE
OcPartitionGetPartitionHandle (
  IN EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath
  );

/**
  Check if disk is a CD-ROM device.

  @param[in] DiskDevicePath  The Device Path of the disk.

  @retval Device Path or NULL
**/
BOOLEAN
OcIsDiskCdRom (
  IN EFI_DEVICE_PATH_PROTOCOL  *DiskDevicePath
  );

/**
  Read El-Torito boot sector from CD-ROM device.

  @param[in]  DiskDevicePath  The Device Path of the disk.
  @param[out] Buffer          Pointer to pool-allocated buffer containing the boot sector data.
  @param[out] BufferSize      Size of Buffer.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcDiskReadElTorito (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DiskDevicePath,
  OUT UINT8                     **Buffer,
  OUT UINTN                     *BufferSize
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
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL    *BlockIo2;
  UINT32                    MediaId;
  UINT32                    BlockSize;
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
  Write information to disk.

  @param[in]  Context     Disk I/O context.
  @param[in]  Lba         LBA number to write to.
  @param[in]  BufferSize  Buffer size allocated in Buffer.
  @param[out] Buffer      Buffer containing data to write.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcDiskWrite (
  IN OC_DISK_CONTEXT  *Context,
  IN UINT64           Lba,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  );

/**
  OC partition list.
**/
typedef struct {
  UINT32                 NumPartitions;
  UINT32                 PartitionEntrySize;
  EFI_PARTITION_ENTRY    FirstEntry[];
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
  Retrieve the disk MBR table, if applicable.

  @param[in]  DiskHandle      Disk device handle to retrive MBR partition table from.
  @param[in]  CheckPartitions Check partition layout. This should be FALSE for a PBR.

  @retval MBR partition table or NULL.
**/
MASTER_BOOT_RECORD *
OcGetDiskMbrTable (
  IN EFI_HANDLE  DiskHandle,
  IN BOOLEAN     CheckPartitions
  );

/**
  Retrieve the MBR partition index for the specified partition.

  @param[in]  PartitionHandle   Partition device handle to retrieve MBR partition index for.
  @param[out] PartitionIndex    Pointer to store partition index in.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcDiskGetMbrPartitionIndex (
  IN  EFI_HANDLE  PartitionHandle,
  OUT UINT8       *PartitionIndex
  );

/**
  Mark specified MBR partition as active.

  @param[in]  DiskHandle        Disk device handle containing MBR partition table
  @param[in]  PartitionIndex    MBR partition index.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcDiskMarkMbrPartitionActive (
  IN  EFI_HANDLE  DiskHandle,
  IN  UINT8       PartitionIndex
  );

/**
  Locate the disk's active MBR partition.

  @param[in]  DiskDevicePath            The Device Path of the disk to scan.
  @param[out] PartitionDevicePathSize   The size of the returned Device Path.
  @param[out] PartitionDeviceHandle     Device handle of the returned partition.

  @return The device path protocol from the discovered handle or NULL.
**/
EFI_DEVICE_PATH_PROTOCOL *
OcDiskFindActiveMbrPartitionPath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DiskDevicePath,
  OUT UINTN                     *PartitionDevicePathSize,
  OUT EFI_HANDLE                *PartitionDeviceHandle
  );

/**
  Creates a device path for a firmware file.

  @param[in]  FileGuid  Firmware file GUID.

  @retval device path allocated from pool on success.
  @retval NULL on failure (e.g. when a file is not present).
**/
EFI_DEVICE_PATH_PROTOCOL *
OcCreateFvFileDevicePath (
  IN EFI_GUID  *FileGuid
  );

/**
  Reads firmware file section to pool-allocated buffer.

  @param[in]  FileGuid      Firmware file GUID.
  @param[in]  SectionType   Section type to read.
  @param[out] FileSize      Size of the section read.

  @return file contents allocated from pool.
  @retval NULL on failure (e.g. when a file is not present).
**/
VOID *
OcReadFvFileSection (
  IN  EFI_GUID  *FileGuid,
  IN  UINT8     SectionType,
  OUT UINT32    *FileSize
  );

#endif // OC_FILE_LIB_H

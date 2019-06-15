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

  @param[in]  FileSystem   A pointer to the file system protocol of the volume.

  @retval A pointer to the NULL terminated unicode volume label.
**/
CHAR16 *
GetVolumeLabel (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem
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

  @param[in]  File         A pointer to the file protocol.
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
  Get file information of specified type.

  @param[in]  FileHandle         A pointer to file handle.
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
GetFileModifcationTime (
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

**/
EFI_HANDLE
OcPartitionGetDiskHandle (
  IN EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath
  );

/**
  Locate the disk's EFI System Partition.

  @param[in] DiskDevicePath  The Device Path of the disk to scan.

**/
EFI_DEVICE_PATH_PROTOCOL *
OcDiskFindSystemPartitionPath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DiskDevicePath
  );

/**
  Retrieve the partition's GPT information, if applicable

  @param[in] FsHandle  The device handle of the partition to retrieve info of.

**/
CONST EFI_PARTITION_ENTRY *
OcGetGptPartitionEntry (
  IN EFI_HANDLE  FsHandle
  );

#endif // OC_FILE_LIB_H

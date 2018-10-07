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

#ifndef OC_FILE_LIB_H_
#define OC_FILE_LIB_H_

// Include the abstracted protocol for its definitions

#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>

#define FILE_ITERATE_DIRECTORIES  EFI_FILE_DIRECTORY
#define FILE_ITERATE_FILES        (EFI_FILE_VALID_ATTR ^ EFI_FILE_DIRECTORY)

// FileExists
/**

  @param[in] DevicePath  A pointer to the device path to check for the file.
  @param[in] FilePath    A pointer to the NULL terminated unicode file name.

  @retval TRUE   The file path was found on the device.
  @retval FALSE  The file path was not found on the device.
**/
BOOLEAN
FileExists (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CHAR16                    *FilePath
  );

// FolderFileExists
/**

  @param[in] DevicePath     A pointer to the device path to check for the file.
  @param[in] DirectoryPath  A pointer to the NULL terminated ascii directory name.
  @param[in] FilePath       A pointer to the NULL terminated ascii file name.

  @retval TRUE   The file path was found on the device.
  @retval FALSE  The file path was not found on the device.
**/
BOOLEAN
FolderFileExists (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CHAR8                     *DirectoryPath OPTIONAL,
  IN CHAR8                     *FilePath OPTIONAL
  );

// GetFileInfo
/**

  @param[in]  DevicePath  A pointer to the device path to device.
  @param[in]  Directory   A pointer to the directory that contains the file.
  @param[in]  FileName    A pointer to the the filename.
  @param[out] FileInfo    A pointer to the FILE_INFO structure returned or NULL

  @retval EFI_SUCCESS  The FILE_INFO structure was successfully returned.
**/
EFI_STATUS
GetFileInfo (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *Directory,
  IN  CHAR16                    *FileName,
  OUT EFI_FILE_INFO             **FileInfo
  );

// GetNextDirEntry
/**

  @param[in]  DevicePath
  @param[out] DirEntry
  @param[in]  SearchMask

  @retval EFI_SUCCESS  The volume label was successfully returned.
**/
EFI_STATUS
GetNextDirEntry (
  IN  EFI_FILE       *Directory,
  OUT EFI_FILE_INFO  **DirEntry,
  IN  UINT64         SearchMask
  );

// GetVolumeLabel
/**

  @param[in]  DevicePath   A pointer to the device path to retrieve the volume label from.
  @param[out] VolumeLabel  A pointer to the NULL terminated unicode volume label.

  @retval EFI_SUCCESS  The volume label was successfully returned.
**/
EFI_STATUS
GetVolumeLabel (
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN OUT CHAR16                    **VolumeLabel
  );

// OpenDirectory
/** Read file from device path

  @param[in]  DevicePath  The whole device path to the file.
  @param[out] FileSize    The size of the file read or 0

  @retval  A pointer to a buffer containing the file read or NULL
 **/
EFI_STATUS
OpenDirectory (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *DirectoryPath,
  IN  UINT64                    Mode,
  IN  UINT64                    Attributes,
  OUT EFI_FILE                  **DirectoryHandle OPTIONAL
  );

// OpenFileSystem
/** Read file from device path

  @param[in]  DevicePath  The whole device path to the file.
  @param[out] FileSystem  The size of the file read or 0

  @retval EFI_SUCCESS  The Filesystem was successfully opened and returned.
**/
EFI_STATUS
OpenFileSystem (
  IN     EFI_DEVICE_PATH_PROTOCOL         **DevicePath,
  IN OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **FileSystem
  );

// ReadFileFromDevicePath
/** Read file from device path

  @param[in]  DevicePath  The whole device path to the file.
  @param[out] FileSize    The size of the file read or 0

  @retval  A pointer to a buffer containing the file read or NULL
**/
VOID *
ReadFileFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT UINTN                     *FileSize
  );

// ReadFile
/** Read file from device path

  @param[in]  DevicePath  The device path for the device.
  @param[in]  FilePath    The full path to the file on the device.
  @param[out] FileBuffer  A pointer to a buffer containing the file read or NULL
  @param[out] FileSize    The size of the file read or 0

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
EFI_STATUS
ReadFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *FilePath,
  OUT VOID                      **FileBuffer,
  OUT UINTN                     *FileBufferSize
  );

// ReadFvFile
/** Read firmware file from device path

  @param[in]  FvFileGuid  The guid of the firmware file to read.
  @param[out] FileBuffer  The size of the file read or 0
  @param[out] FileSize    The size of the file read or 0

  @retval  A pointer to a buffer containing the file read or NULL
**/
EFI_STATUS
ReadFvFile (
  IN  GUID   *FvFileGuid,
  OUT VOID   **FileBuffer,
  OUT UINTN  *FileSize
  );

// SaveFile
/** Save filebuffer to device path

  @param[in]  DevicePath     The device path for the device.
  @param[in]  DirectoryPath  The directory path to place the file on the device.
  @param[in]  FilePath       The filename to use when saving the file on the device.
  @param[out] FileBuffer     A pointer to a buffer containing the file read or NULL
  @param[out] FileSize       The size of the file buffer to save

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
EFI_STATUS
SaveFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *Directory,
  IN  CHAR16                    *FileName,
  IN  VOID                      *FileBuffer,
  IN  UINTN                     FileSize
  );

// WriteFilePath
/** Write file to device path

  @param[in]  DevicePath     The device path to the device to create the file on.
  @param[in]  FilePath       The full path to use when writing the file on the device.
  @param[in]  FileBuffer     A pointer to a buffer containing the file contents.
  @param[in]  FileSize       The size of the file buffer to save
  @param[in]  Attributes

  @retval EFI_SUCCESS  The platform detection executed successfully.
**/
EFI_STATUS
WriteFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR16                    *FilePath,
  IN  VOID                      *FileBuffer,
  IN  UINTN                     FileSize,
  IN  UINT64                    Attributes
  );

#endif // OC_FILE_LIB_H_

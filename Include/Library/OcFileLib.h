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
#include <Protocol/DevicePath.h>

/**

  @param[in]  FileHandle         A pointer to file handle.
  @param[in]  InformationType    A pointer to file info GUID.
  @param[in]  MinFileInfoSize    Minimal size of the info provided.
  @param[out] RealFileInfoSize   Actual info size read (optional).

  @retval read file info or NULL.
**/
VOID *
GetFileInfo (
  IN  EFI_FILE_PROTOCOL  *FileHandle,
  IN  EFI_GUID           *InformationType,
  IN  UINTN              MinFileInfoSize,
  OUT UINTN              *RealFileInfoSize  OPTIONAL
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

  @retval A pointer to a buffer containing file read or NULL.
**/
VOID *
ReadFile (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *FilePath,
  OUT UINTN                            *FileSize OPTIONAL
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
ReadFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  );

/**
  Determine file size if it is less than 4 GB.

  @param[in]  File         A pointer to the file protocol.
  @param[out] Size         32-bit file size.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
ReadFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  );

#endif // OC_FILE_LIB_H_

/** @file
  Apple Single File protocol.

Copyright (C) 2019, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_SINGLE_FILE_PROTOCOL_H
#define APPLE_SINGLE_FILE_PROTOCOL_H

#include <Guid/FileInfo.h>

/**
  Apple Single File protocol GUID.
  7542A2BB-D260-4CC2-896E-D613CD515069
**/
#define APPLE_SINGLE_FILE_PROTOCOL_GUID \
  { 0x7542A2BB, 0xD260, 0x4CC2,        \
    { 0x89, 0x6E, 0xD6, 0x13, 0xCD, 0x51, 0x50, 0x69 } }

/**
  Apple single file protocol signature, "SFSx".
**/
#define APPLE_SINGLE_FILE_SIGNATURE  0x78534653U

/**
  Apple single file protocol version.
  Versions other than 1 take EFI_FILE_INFO as a FileInfo argument.
**/
#define APPLE_SINGLE_FILE_VERSION    1U

/**
  Default file system name when FileInfo is not provided at creation.
**/
#define APPLE_SINGLE_FILE_DEFAULT_FILESYSTEM_NAME L"Single File System"

/**
  Default file name when FileInfo is not provided at creation.
**/
#define APPLE_SINGLE_FILE_DEFAULT_FILE_NAME L"SomeLonelyFile"

/**
  Apple single file info.
**/
typedef struct {
  ///
  /// Unused, possibly zero.
  ///
  UINT64 Reserved;
  ///
  /// File size in bytes.
  ///
  UINT64 FileSize;
  ///
  /// File name.
  ///
  CHAR16 FileName[32];
  ///
  /// File system name, optional.
  ///
  CHAR16 FileSystemName[32];
} APPLE_SINGLE_FILE_INFO;

/**
  Create single file system on a block i/o device, usually RAM disk.
  Single file system may hold at most 1 readable and writeable file.

  @param[in]  Handle             Device handle.
  @param[out] FileInfo           File information, optional.

  @retval EFI_NO_MEDIA           No Block I/O media.
  @retval EFI_NOT_FOUND          Missing Block I/O protocol.
  @retval EFI_SUCCESS            Written single file instance succesfully.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SINGLE_FILE_CREATE) (
  IN  EFI_HANDLE              Handle,
  OUT APPLE_SINGLE_FILE_INFO  *FileInfo  OPTIONAL
  );

/**
  Apple Single File FileSystem protocol.
**/
typedef struct {
  UINT32                      Magic;
  UINT32                      Version;
  APPLE_SINGLE_FILE_CREATE    CreateFile;
} APPLE_SINGLE_FILE_PROTOCOL;

extern gAppleSingleFileProtocolGuid;

#endif // APPLE_SINGLE_FILE_PROTOCOL_H

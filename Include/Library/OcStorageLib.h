/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_STORAGE_LIB_H
#define OC_STORAGE_LIB_H

#include <Library/OcFileLib.h>

/**
  Storage abstraction context
**/
typedef struct {
  ///
  /// Storage root owned by context.
  ///
  EFI_FILE_PROTOCOL  *StorageRoot;
} OC_STORAGE_CONTEXT;

/**
  Create storage context from UEFI file system at specified path.

  @param[out]  Context     Resulting storage context.
  @param[in]   FileSystem  Storage file system.
  @param[in]   Path        Storage file system path (e.g. L"\\").
  @param[in]   Key         Storage signature verification key, optional.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcStorageInitFromFs (
  OUT OC_STORAGE_CONTEXT               *Context,
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *Path,
  IN  VOID                             *StorageKey OPTIONAL
  );

/**
  Free storage context resources.

  @param[in,out]  Context     Storage context.
**/
VOID
OcStorageFree (
  IN OUT OC_STORAGE_CONTEXT            *Context
  );

/**
  Read file from storage with implicit double (2 byte) null termination.
  Null termination does not affect the returned file size.
  Depending on the implementation 0 byte files may return null.
  If storage context was created with valid storage key, then signature
  checking will be performed

  @param[in]  Context      Storage context.
  @param[in]  FilePath     The full path to the file on the device.
  @param[out] FileSize     The size of the file read (optional).

  @retval A pointer to a buffer containing file read or NULL.
**/
VOID *
OcStorageReadFileUnicode (
  IN  OC_STORAGE_CONTEXT               *Context,
  IN  CONST CHAR16                     *FilePath,
  OUT UINT32                           *FileSize OPTIONAL
  );

/**
  OcStorageReadFileUnicode alias with support for ASCII file names.

  @param[in]  Context      Storage context.
  @param[in]  FilePath     The full path to the file on the device.
  @param[out] FileSize     The size of the file read (optional).

  @retval A pointer to a buffer containing file read or NULL.
**/
VOID *
OcStorageReadFileAscii (
  IN  OC_STORAGE_CONTEXT               *Context,
  IN  CONST CHAR8                      *FilePath,
  OUT UINT32                           *FileSize OPTIONAL
  );

#endif // OC_STORAGE_LIB_H

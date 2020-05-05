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

#ifndef OC_VIRTUAL_FS_LIB_H
#define OC_VIRTUAL_FS_LIB_H

#include <Uefi.h>

#include <Protocol/SimpleFileSystem.h>

/**
  Creates read-only EFI_FILE_PROTOCOL instance over a buffer allocated
  from pool. On success FileName and FileData ownership is transferred
  to the resulting EFI_FILE_PROTOCOL, which frees them with FreePool
  upon closing EFI_FILE_PROTOCOL.

  Resulting EFI_FILE_PROTOCOL has 2nd revision, but may be downgraded
  to 1st by updating the corresponding field.

  @param[in]      FileName         Pointer to the file's name.
  @param[in]      FileBuffer       Pointer to the file's data.
  @param[in]      FileSize         File size of FileData.
  @param[in]      ModificationTime File modification date, optional.
  @param[in,out]  File             Resulting file protocol.

  @return  EFI_SUCCESS if instance was successfully created.
**/
EFI_STATUS
CreateVirtualFile (
  IN     CHAR16             *FileName,
  IN     VOID               *FileBuffer,
  IN     UINT64             FileSize,
  IN     EFI_TIME           *ModificationTime OPTIONAL,
  IN OUT EFI_FILE_PROTOCOL  **File
  );

/**
  Creates virtual file system instance around any file.
  CreateRealFile or CreateVirtualFile must be called from
  any registered OpenCallback.

  @param[in]      OriginalFile     Pointer to the original file.
  @param[in]      OpenCallback     File open callback.
  @param[in]      CloseOnFailure   Close the original file on failure.
  @param[in,out]  File             Resulting file protocol.

  @return  EFI_SUCCESS if instance was successfully created.
  @return  EFI_SIMPLE_FILE_SYSTEM Open-compatible error return code.
**/
EFI_STATUS
CreateRealFile (
  IN     EFI_FILE_PROTOCOL  *OriginalFile OPTIONAL,
  IN     EFI_FILE_OPEN      OpenCallback OPTIONAL,
  IN     BOOLEAN            CloseOnFailure,
  IN OUT EFI_FILE_PROTOCOL  **File
  );

/**
  Create virtual file system by wrapping OriginalFileSystem
  into NewFileSystem with specified callback. Cacheable.

  @param[in]       OriginalFileSystem  Source file system.
  @param[in]       OpenCallback        File open callback.
  @param[in,out]   NewFileSystem       Wrapped file system.

  @return  EFI_SUCCESS on successful wrapping.
**/
EFI_STATUS
CreateVirtualFs (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *OriginalFileSystem,
  IN     EFI_FILE_OPEN                    OpenCallback,
  IN OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **NewFileSystem
  );

/**
  Enables virtual file system access for given BootServices
  with callback on file open.

  @param[in,out]  BootServices  Hooked EFI_BOOT_SERVICES.
  @param[in]      OpenCallback  File open callback.

  @return  EFI_SUCCESS on successful hooking.
**/
EFI_STATUS
EnableVirtualFs (
  IN OUT EFI_BOOT_SERVICES       *BootServices,
  IN     EFI_FILE_OPEN           OpenCallback
  );

/**
  Enables virtual file system access for given BootServices.

  @param[in,out]  BootServices  Hooked EFI_BOOT_SERVICES.

  @return  EFI_SUCCESS on successful unhooking.
**/
EFI_STATUS
DisableVirtualFs (
  IN OUT EFI_BOOT_SERVICES       *BootServices
  );

#endif // OC_VIRTUAL_FS_LIB_H

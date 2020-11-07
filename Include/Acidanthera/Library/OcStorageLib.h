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

#include <Library/OcCryptoLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcSerializeLib.h>

/**
  Storage vault file containing a dictionary with SHA-256 hashes for all files.
**/
#define OC_STORAGE_VAULT_PATH L"vault.plist"

/**
  RSA-2048 signature of SHA-256 hash of vault.plist.
**/
#define OC_STORAGE_VAULT_SIGNATURE_PATH L"vault.sig"

/**
  Storage vault version declaring vault compatibility.
**/
#define OC_STORAGE_VAULT_VERSION 1

/**
  Safe maximum file path in the storage.
**/
#define OC_STORAGE_SAFE_PATH_MAX 128

/**
  Structure declaration for valult file.
**/
#define OC_STORAGE_VAULT_HASH_FIELDS(_, __) \
  _(UINT8      , Hash     , [SHA256_DIGEST_SIZE] , {0}         , () )
  OC_DECLARE (OC_STORAGE_VAULT_HASH)

#define OC_STORAGE_VAULT_FILES_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_STORAGE_VAULT_HASH, _, __)
  OC_DECLARE (OC_STORAGE_VAULT_FILES)

#define OC_STORAGE_VAULT_FIELDS(_, __) \
  _(UINT32                      , Version  ,     , 0                                         , () ) \
  _(OC_STORAGE_VAULT_FILES      , Files    ,     , OC_CONSTR (OC_STORAGE_VAULT_FILES, _, __) , OC_DESTR (OC_STORAGE_VAULT_FILES))
  OC_DECLARE (OC_STORAGE_VAULT)

/**
  Storage abstraction context
**/
typedef struct {
  ///
  /// Storage file system if any.
  ///
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  ///
  /// Real storage handle for loading.
  ///
  EFI_HANDLE                       StorageHandle;
  ///
  /// Real file path for file storage, can be missing.
  ///
  EFI_DEVICE_PATH_PROTOCOL         *StoragePath;
  ///
  /// Path to the root of file storage on StoragePage.
  ///
  CONST CHAR16                     *StorageRoot;
  ///
  /// Storage file instance owned by context.
  ///
  EFI_FILE_PROTOCOL                *Storage;
  ///
  /// Device handle with storage (dummy) device path for loading.
  ///
  EFI_HANDLE                       DummyStorageHandle;
  ///
  /// Dummy file path for file storage.
  ///
  EFI_DEVICE_PATH_PROTOCOL         *DummyDevicePath;
  ///
  /// Dummy file path relative to storage.
  ///
  EFI_DEVICE_PATH_PROTOCOL         *DummyFilePath;
  ///
  /// Vault context.
  ///
  OC_STORAGE_VAULT                 Vault;
  ///
  /// Vault status.
  ///
  BOOLEAN                          HasVault;
} OC_STORAGE_CONTEXT;

/**
  Create storage context from UEFI file system at specified path.

  @param[out]  Context         Resulting storage context.
  @param[in]   FileSystem      Storage file system.
  @param[in]   StorageHandle   Storage handle, optional.
  @param[in]   StoragePath     Actual storage device path, optional.
  @param[in]   StorageRoot     Storage file system root (e.g. L"\\").
  @param[in]   StorageKey      Storage signature verification key, optional.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcStorageInitFromFs (
  OUT OC_STORAGE_CONTEXT               *Context,
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  EFI_HANDLE                       StorageHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL         *StoragePath   OPTIONAL,
  IN  CONST CHAR16                     *StorageRoot,
  IN  OC_RSA_PUBLIC_KEY                *StorageKey    OPTIONAL
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
  Check whether file exists.

  @param[in]  Context      Storage context.
  @param[in]  FilePath     The full path to the file on the device.

  @retval TRUE if file is present in vault or ondisk.
**/
BOOLEAN
OcStorageExistsFileUnicode (
  IN  OC_STORAGE_CONTEXT               *Context,
  IN  CONST CHAR16                     *FilePath
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
  Get information about the storage file when possible.

  @param[in]  Context         Storage context.
  @param[in]  FilePath        The full path to the file on the device.
  @param[out] DevicePath      Full storage file device path.
  @param[out] StorageHandle   Storage handle.
  @param[out] StoragePath     Path on the storage.
  @param[in]  RealPath        Provide real paths instead of dummy.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcStorageGetInfo (
  IN  OC_STORAGE_CONTEXT               *Context,
  IN  CONST CHAR16                     *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL         **DevicePath,
  OUT EFI_HANDLE                       *StorageHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL         **StoragePath,
  IN  BOOLEAN                          RealPath
  );

#endif // OC_STORAGE_LIB_H

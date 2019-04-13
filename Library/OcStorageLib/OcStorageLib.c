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

#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcStorageLib.h>
#include <Library/UefiBootServicesTableLib.h>

OC_STRUCTORS (OC_STORAGE_VAULT_HASH, ())
OC_MAP_STRUCTORS (OC_STORAGE_VAULT_FILES)
OC_STRUCTORS (OC_STORAGE_VAULT, ())


STATIC
OC_SCHEMA
mVaultFilesSchema = OC_SCHEMA_DATAF (NULL, UINT8 [SHA256_DIGEST_SIZE]);

///
/// WARNING: Field list must be alpabetically ordered here!
///
STATIC
OC_SCHEMA
mVaultNodesSchema[] = {
  OC_SCHEMA_MAP_IN     ("Files",   OC_STORAGE_VAULT, Files, &mVaultFilesSchema),
  OC_SCHEMA_INTEGER_IN ("Version", OC_STORAGE_VAULT, Version),
};

STATIC
OC_SCHEMA_INFO
mVaultSchema = {
  .Dict = {mVaultNodesSchema, ARRAY_SIZE (mVaultNodesSchema)}
};


STATIC
EFI_STATUS
OcStorageInitializeVault (
  IN OUT OC_STORAGE_CONTEXT  *Context,
  IN     VOID                *Vault      OPTIONAL,
  IN     UINT32              VaultSize,
  IN     RSA_PUBLIC_KEY      *StorageKey OPTIONAL,
  IN     VOID                *Signature  OPTIONAL
  )
{
  UINT8  Digest[SHA256_DIGEST_SIZE];

  if (Signature != NULL && Vault == NULL) {
    DEBUG ((DEBUG_ERROR, "OCS: Missing vault with signature\n"));
    return EFI_SECURITY_VIOLATION;
  }

  if (Vault == NULL) {
    DEBUG ((DEBUG_INFO, "OCS: Missing vault data, ignoring...\n"));
    return EFI_SUCCESS;
  }

  if (Signature != NULL) {
    ASSERT (StorageKey != NULL);

    Sha256 (Digest, Vault, VaultSize);

    if (!RsaVerify (StorageKey, Signature, Digest)) {
      DEBUG ((DEBUG_ERROR, "OCS: Invalid vault signature\n"));
      return EFI_SECURITY_VIOLATION;
    }
  }

  OC_STORAGE_VAULT_CONSTRUCT (&Context->Vault, sizeof (Context->Vault));
  if (!ParseSerialized (&Context->Vault, &mVaultSchema, Vault, VaultSize)) {
    OC_STORAGE_VAULT_DESTRUCT (&Context->Vault, sizeof (Context->Vault));
    DEBUG ((DEBUG_ERROR, "OCS: Invalid vault data\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Context->Vault.Version != OC_STORAGE_VAULT_VERSION) {
    OC_STORAGE_VAULT_DESTRUCT (&Context->Vault, sizeof (Context->Vault));
    DEBUG ((
      DEBUG_ERROR,
      "OCS: Unsupported vault data verion %u vs %u\n",
      Context->Vault.Version,
      OC_STORAGE_VAULT_VERSION
      ));
    return EFI_UNSUPPORTED;
  }

  Context->HasVault = TRUE;

  return EFI_SUCCESS;
}

STATIC
UINT8 *
OcStorageGetDigest (
  IN OUT OC_STORAGE_CONTEXT  *Context,
  IN     CONST CHAR16        *Filename
  )
{
  UINT32             Index;
  UINTN              StrIndex;
  CHAR8              *VaultFilePath;
  UINTN              FilenameSize;

  if (!Context->HasVault) {
    return NULL;
  }

  FilenameSize = StrLen (Filename) + 1;

  for (Index = 0; Index < Context->Vault.Files.Count; ++Index) {
    if (Context->Vault.Files.Keys[Index]->Size != (UINT32) FilenameSize) {
      continue;
    }

    VaultFilePath = OC_BLOB_GET (Context->Vault.Files.Keys[Index]);

    for (StrIndex = 0; StrIndex < FilenameSize; ++StrIndex) {
      if (Filename[StrIndex] != VaultFilePath[StrIndex]) {
        break;
      }
    }

    if (StrIndex == FilenameSize) {
      return &Context->Vault.Files.Values[Index]->Hash[0];
    }
  }

  return NULL;
}

EFI_STATUS
OcStorageInitFromFs (
  OUT OC_STORAGE_CONTEXT               *Context,
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *Path,
  IN  RSA_PUBLIC_KEY                   *StorageKey OPTIONAL
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *RootVolume;
  VOID               *Vault;
  VOID               *Signature;
  UINT32             DataSize;

  ZeroMem (Context, sizeof (*Context));

  Status = FileSystem->OpenVolume (FileSystem, &RootVolume);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = RootVolume->Open (
    RootVolume,
    &Context->StorageRoot,
    (CHAR16 *) Path,
    EFI_FILE_MODE_READ,
    0
    );

  RootVolume->Close (RootVolume);

  if (!EFI_ERROR (Status)) {
    if (StorageKey) {
      Signature = OcStorageReadFileUnicode (
        Context,
        OC_STORAGE_VAULT_SIGNATURE_PATH,
        &DataSize
        );

      if (Signature == NULL) {
        DEBUG ((DEBUG_ERROR, "OCS: Missing vault signature\n"));
        OcStorageFree (Context);
        return EFI_SECURITY_VIOLATION;
      }

      if (DataSize != CONFIG_RSA_KEY_SIZE) {
        DEBUG ((
          DEBUG_ERROR,
          "OCS: Vault signature size mismatch: %u vs %u\n",
          DataSize,
          CONFIG_RSA_KEY_SIZE
          ));
        FreePool (Signature);
        OcStorageFree (Context);
        return EFI_SECURITY_VIOLATION;
      }
    } else {
      Signature = NULL;
    }

    Vault = OcStorageReadFileUnicode (
      Context,
      OC_STORAGE_VAULT_PATH,
      &DataSize
      );

    Status = OcStorageInitializeVault (Context, Vault, DataSize, StorageKey, Signature);

    if (Signature != NULL) {
      FreePool (Signature);
    }

    if (Vault != NULL) {
      FreePool (Vault);
    }
  }

  return Status;
}

VOID
OcStorageFree (
  IN OUT OC_STORAGE_CONTEXT            *Context
  )
{
  if (Context->StorageRoot != NULL) {
    Context->StorageRoot->Close (Context->StorageRoot);
    Context->StorageRoot = NULL;
  }

  if (Context->HasVault) {
    OC_STORAGE_VAULT_DESTRUCT (&Context->Vault, sizeof (Context->Vault));
    Context->HasVault = FALSE;
  }
}

VOID *
OcStorageReadFileUnicode (
  IN  OC_STORAGE_CONTEXT               *Context,
  IN  CONST CHAR16                     *FilePath,
  OUT UINT32                           *FileSize OPTIONAL
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *File;
  UINT32             Size;
  UINT8              *FileBuffer;
  UINT8              *VaultDigest;
  UINT8              FileDigest[SHA256_DIGEST_SIZE];

  //
  // Using this API with empty filename is also not allowed.
  //
  ASSERT (Context != NULL);
  ASSERT (FilePath != NULL);
  ASSERT (StrLen (FilePath) > 0);

  VaultDigest = OcStorageGetDigest (Context, FilePath);

  if (Context->HasVault && VaultDigest == NULL) {
    DEBUG ((DEBUG_ERROR, "OCS: Aborting %s file access not present in vault\n", FilePath));
    return NULL;
  }

  if (Context->StorageRoot == NULL) {
    //
    // TODO: expand support for other contexts.
    //
    return NULL;
  }

  Status = Context->StorageRoot->Open (
    Context->StorageRoot,
    &File,
    (CHAR16 *) FilePath,
    EFI_FILE_MODE_READ,
    0
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = GetFileSize (File, &Size);
  if (EFI_ERROR (Status) || Size >= MAX_UINT32 - 1) {
    File->Close (File);
    return NULL;
  }

  FileBuffer = AllocatePool (Size + 2);
  if (FileBuffer == NULL) {
    File->Close (File);
    return NULL;
  }

  Status = GetFileData (File, 0, Size, FileBuffer);
  File->Close (File);
  if (EFI_ERROR (Status)) {
    FreePool (FileBuffer);
    return NULL;
  }

  if (VaultDigest != 0) {
    Sha256 (FileDigest, FileBuffer, Size);
    if (CompareMem (FileDigest, VaultDigest, SHA256_DIGEST_SIZE) != 0) {
      DEBUG ((DEBUG_ERROR, "OCS: Aborting corrupted %s file access\n", FilePath));
      FreePool (FileBuffer);
      return NULL;
    }
  }

  FileBuffer[Size]     = 0;
  FileBuffer[Size + 1] = 0;

  if (FileSize != NULL) {
    *FileSize = Size;
  }

  return FileBuffer;
}

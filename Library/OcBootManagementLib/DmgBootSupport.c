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

#include "BootManagementInternal.h"

#include <Guid/FileInfo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FileHandleLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetFirstDeviceBootFilePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DmgDevicePath,
  IN UINTN                           DmgDevicePathSize
  )
{
  EFI_DEVICE_PATH_PROTOCOL       *BootDevicePath;

  EFI_STATUS                     Status;
  INTN                           CmpResult;

  CONST EFI_DEVICE_PATH_PROTOCOL *FsDevicePath;
  UINTN                          FsDevicePathSize;

  UINTN                          NumHandles;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          Index;

  ASSERT (DmgDevicePath != NULL);
  ASSERT (DmgDevicePathSize >= END_DEVICE_PATH_LENGTH);

  BootDevicePath = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < NumHandles; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&FsDevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    FsDevicePathSize = GetDevicePathSize (FsDevicePath);

    if (FsDevicePathSize < DmgDevicePathSize) {
      continue;
    }

    CmpResult = CompareMem (
                  FsDevicePath,
                  DmgDevicePath,
                  (DmgDevicePathSize - END_DEVICE_PATH_LENGTH)
                  );
    if (CmpResult != 0) {
      continue;
    }

    Status = OcBootPolicyGetBootFileEx (
                           HandleBuffer[Index],
                           gAppleBootPolicyPredefinedPaths,
                           gAppleBootPolicyNumPredefinedPaths,
                           &BootDevicePath
                           );
    if (!EFI_ERROR (Status)) {
      break;
    }

    BootDevicePath = NULL;
  }

  FreePool (HandleBuffer);

  return BootDevicePath;
}

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetDiskImageBootFile (
  OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN  UINT32                      Policy,
  IN  UINTN                       DmgFileSize,
  IN  VOID                        *ChunklistBuffer OPTIONAL,
  IN  UINT32                      ChunklistBufferSize OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL       *DevPath;

  BOOLEAN                        Result;
  OC_APPLE_CHUNKLIST_CONTEXT     ChunklistContext;

  CONST EFI_DEVICE_PATH_PROTOCOL *DmgDevicePath;
  UINTN                          DmgDevicePathSize;

  ASSERT (Context != NULL);
  ASSERT (DmgFileSize > 0);

  if (ChunklistBuffer == NULL) {
    if ((Policy & OC_LOAD_REQUIRE_APPLE_SIGN) != 0) {
      DEBUG ((DEBUG_WARN, "OCB: Missing DMG signature, aborting\n"));
      return NULL;
    }
  } else if ((Policy & (OC_LOAD_VERIFY_APPLE_SIGN | OC_LOAD_REQUIRE_TRUSTED_KEY)) != 0) {
    ASSERT (ChunklistBufferSize > 0);

    Result = OcAppleChunklistInitializeContext (
                &ChunklistContext,
                ChunklistBuffer,
                ChunklistBufferSize
                );
    if (!Result) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Failed to initialise DMG Chunklist context\n"
        ));
      return NULL;
    }

    if ((Policy & OC_LOAD_REQUIRE_TRUSTED_KEY) != 0) {
      Result = FALSE;
      //
      // FIXME: Properly abstract OcAppleKeysLib.
      //
      if ((Policy & OC_LOAD_TRUST_APPLE_V1_KEY) != 0) {
        Result = OcAppleChunklistVerifySignature (
                   &ChunklistContext,
                   PkDataBase[0].PublicKey
                   );
      }

      if (!Result && ((Policy & OC_LOAD_TRUST_APPLE_V2_KEY) != 0)) {
        Result = OcAppleChunklistVerifySignature (
                   &ChunklistContext,
                   PkDataBase[1].PublicKey
                   );
      }

      if (!Result) {
        DEBUG ((DEBUG_WARN, "OCB: DMG is not trusted, aborting\n"));
        return NULL;
      }
    }

    Result = OcAppleDiskImageVerifyData (
               Context->DmgContext,
               &ChunklistContext
               );
    if (!Result) {
      DEBUG ((DEBUG_WARN, "OCB: DMG has been altered\n"));
      //
      // FIXME: Warn user instead of aborting when OC_LOAD_REQUIRE_TRUSTED_KEY
      //        is not set.
      //
      return NULL;
    }
  }

  Context->BlockIoHandle = OcAppleDiskImageInstallBlockIo (
                             Context->DmgContext,
                             DmgFileSize,
                             &DmgDevicePath,
                             &DmgDevicePathSize
                             );
  if (Context->BlockIoHandle == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to install DMG Block I/O\n"));
    return NULL;
  }

  DevPath = InternalGetFirstDeviceBootFilePath (
              DmgDevicePath,
              DmgDevicePathSize
              );
  if (DevPath == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to get bootable file off DMG\n"));

    OcAppleDiskImageUninstallBlockIo (
      Context->DmgContext,
      Context->BlockIoHandle
      );
    return NULL;
  }

  return DevPath;
}

STATIC
EFI_FILE_INFO *
InternalFindFirstDmgFileName (
  IN  EFI_FILE_PROTOCOL  *Directory,
  OUT UINTN              *FileNameLen
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *FileInfo;
  BOOLEAN       NoFile;
  UINTN         ExtOffset;
  UINTN         Length;
  INTN          Result;

  ASSERT (Directory != NULL);

  for (
    Status = FileHandleFindFirstFile (Directory, &FileInfo), NoFile = FALSE;
    (!EFI_ERROR (Status) && !NoFile);
    Status = FileHandleFindNextFile (Directory, FileInfo, &NoFile)
    ) {
    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
      continue;
    }

    //
    // Discard filenames that do not contain characters prior to .dmg extension.
    //
    Length = StrLen (FileInfo->FileName);
    if (Length <= L_STR_LEN (L".dmg")) {
      continue;
    }

    ExtOffset = Length - L_STR_LEN (L".dmg");
    Result    = StrCmp (&FileInfo->FileName[ExtOffset], L".dmg");
    if (Result == 0) {
      if (FileNameLen != NULL) {
        *FileNameLen = ExtOffset;
      }

      return FileInfo;
    }
  }

  return NULL;
}

STATIC
EFI_FILE_INFO *
InternalFindDmgChunklist (
  IN EFI_FILE_PROTOCOL  *Directory,
  IN CONST CHAR16       *DmgFileName,
  IN UINTN              DmgFileNameLen
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *FileInfo;
  BOOLEAN       NoFile;
  UINTN         NameLen;
  INTN          Result;
  UINTN         ChunklistFileNameLen;

  ChunklistFileNameLen = (DmgFileNameLen + L_STR_LEN (".chunklist"));

  for (
    Status = FileHandleFindFirstFile (Directory, &FileInfo), NoFile = FALSE;
    (!EFI_ERROR (Status) && !NoFile);
    Status = FileHandleFindNextFile (Directory, FileInfo, &NoFile)
    ) {
    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
      continue;
    }

    NameLen = StrLen (FileInfo->FileName);

    if (NameLen != ChunklistFileNameLen) {
      continue;
    }

    Result = StrnCmp (FileInfo->FileName, DmgFileName, DmgFileNameLen);
    if (Result != 0) {
      continue;
    }

    Result = StrCmp (&FileInfo->FileName[DmgFileNameLen], L".chunklist");
    if (Result == 0) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Found chunklist %s for DMG %s[%d]\n",
        FileInfo->FileName,
        DmgFileName,
        DmgFileNameLen
        ));
      return FileInfo;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Found no chunklist for DMG %s[%d]\n",
    DmgFileName,
    DmgFileNameLen
    ));
  return NULL;
}

EFI_DEVICE_PATH_PROTOCOL *
InternalLoadDmg (
  IN OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN     UINT32                      Policy
  )
{
  EFI_DEVICE_PATH_PROTOCOL *DevPath;

  EFI_STATUS               Status;
  BOOLEAN                  Result;

  EFI_FILE_PROTOCOL        *DmgDir;

  UINTN                    DmgFileNameLen;
  EFI_FILE_INFO            *DmgFileInfo;
  EFI_FILE_PROTOCOL        *DmgFile;
  UINT32                   DmgFileSize;

  EFI_FILE_INFO            *ChunklistFileInfo;
  EFI_FILE_PROTOCOL        *ChunklistFile;
  UINT32                   ChunklistFileSize;
  VOID                     *ChunklistBuffer;

  CHAR16 *DevPathText;

  ASSERT (Context != NULL);

  DevPath = Context->DevicePath;
  Status = OcOpenFileByDevicePath (
             &DevPath,
             &DmgDir,
             EFI_FILE_MODE_READ,
             EFI_FILE_DIRECTORY
             );
  if (EFI_ERROR (Status)) {
    DevPathText = ConvertDevicePathToText (Context->DevicePath, FALSE, FALSE);
    DEBUG ((DEBUG_INFO, "OCB: Failed to open DMG directory %s\n", DevPathText));
    if (DevPathText != NULL) {
      FreePool (DevPathText);
    }

    return NULL;
  }

  DmgFileInfo = InternalFindFirstDmgFileName (DmgDir, &DmgFileNameLen);
  if (DmgFileInfo == NULL) {
    DevPathText = ConvertDevicePathToText (Context->DevicePath, FALSE, FALSE);
    DEBUG ((DEBUG_INFO, "OCB: Unable to find any DMG at %s\n"));
    if (DevPathText != NULL) {
      FreePool (DevPathText);
    }

    DmgDir->Close (DmgDir);
    return NULL;
  }

  Status = SafeFileOpen (
    DmgDir,
    &DmgFile,
    DmgFileInfo->FileName,
    EFI_FILE_MODE_READ,
    0
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Failed to open DMG file %s - %r\n",
      DmgFileInfo->FileName,
      Status
      ));

    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    return NULL;
  }

  Status = GetFileSize (DmgFile, &DmgFileSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Failed to retrieve DMG file size - %r\n",
      Status
      ));

    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    DmgFile->Close (DmgFile);
    return NULL;
  }

  Context->DmgContext = AllocatePool (sizeof (*Context->DmgContext));
  if (Context->DmgContext == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to allocate DMG context\n"));
    return NULL;
  }

  Result = OcAppleDiskImageInitializeFromFile (Context->DmgContext, DmgFile);

  DmgFile->Close (DmgFile);

  if (!Result) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to initialise DMG from file\n"));

    FreePool (DmgFileInfo);
    FreePool (Context->DmgContext);
    DmgDir->Close (DmgDir);
    return NULL;
  }

  ChunklistBuffer   = NULL;
  ChunklistFileSize = 0;

  ChunklistFileInfo = InternalFindDmgChunklist (
                        DmgDir,
                        DmgFileInfo->FileName,
                        DmgFileNameLen
                        );
  if (ChunklistFileInfo != NULL) {
    Status = SafeFileOpen (
      DmgDir,
      &ChunklistFile,
      ChunklistFileInfo->FileName,
      EFI_FILE_MODE_READ,
      0
      );
    if (!EFI_ERROR (Status)) {
      Status = GetFileSize (ChunklistFile, &ChunklistFileSize);
      if (Status == EFI_SUCCESS) {
        ChunklistBuffer = AllocatePool (ChunklistFileSize);

        if (ChunklistBuffer == NULL) {
          ChunklistFileSize = 0;
        } else {
          Status = GetFileData (ChunklistFile, 0, ChunklistFileSize, ChunklistBuffer);
          if (EFI_ERROR (Status)) {
            FreePool (ChunklistBuffer);
            ChunklistBuffer   = NULL;
            ChunklistFileSize = 0;
          }
        }
      }

      ChunklistFile->Close (ChunklistFile);
    }

    FreePool (ChunklistFileInfo);
  }

  FreePool (DmgFileInfo);

  DmgDir->Close (DmgDir);

  DevPath = InternalGetDiskImageBootFile (
              Context,
              Policy,
              DmgFileSize,
              ChunklistBuffer,
              ChunklistFileSize
              );
  Context->DevicePath = DevPath;

  if (DevPath == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to retrieve boot file from DMG\n"));

    OcAppleDiskImageFreeFile (Context->DmgContext);
    FreePool (Context->DmgContext);
  }

  if (ChunklistBuffer != NULL) {
    FreePool (ChunklistBuffer);
  }

  return DevPath;
}

VOID
InternalUnloadDmg (
  IN INTERNAL_DMG_LOAD_CONTEXT  *DmgLoadContext
  )
{
  if (DmgLoadContext->DevicePath != NULL) {
    FreePool (DmgLoadContext->DevicePath);
    OcAppleDiskImageUninstallBlockIo (
      DmgLoadContext->DmgContext,
      DmgLoadContext->BlockIoHandle
      );
    OcAppleDiskImageFreeContext (DmgLoadContext->DmgContext);
    FreePool (DmgLoadContext->DmgContext);
    DmgLoadContext->DevicePath = NULL;
  }
}

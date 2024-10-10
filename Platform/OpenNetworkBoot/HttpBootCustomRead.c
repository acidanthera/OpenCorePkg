/** @file
  Top level LoadFile protocol handler for HTTP Boot.

  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "NetworkBootInternal.h"

#include <Library/OcConsoleLib.h>
#include <Protocol/ConsoleControl.h>

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL    *RamDiskDevicePath;
} CUSTOM_FREE_CONTEXT;

STATIC
EFI_STATUS
SetDmgPreloadDmgFile (
  IN     OC_APPLE_DISK_IMAGE_PRELOAD_CONTEXT  *DmgPreloadContext,
  IN OUT VOID                                 **Data,
  IN OUT UINT32                               *DataSize
  )
{
  EFI_STATUS  Status;

  Status = CreateVirtualFileFileNameCopy (L"__HTTPBoot__.dmg", *Data, *DataSize, NULL, &DmgPreloadContext->DmgFile);

  if (EFI_ERROR (Status)) {
    FreePool (Data);
  } else {
    DmgPreloadContext->DmgFileSize = *DataSize;
  }

  *Data     = NULL;
  *DataSize = 0;

  return Status;
}

STATIC
EFI_STATUS
SetDmgPreloadChunklist (
  IN     OC_APPLE_DISK_IMAGE_PRELOAD_CONTEXT  *DmgPreloadContext,
  IN OUT VOID                                 **Data,
  IN OUT UINT32                               *DataSize
  )
{
  DmgPreloadContext->ChunklistBuffer   = *Data;
  DmgPreloadContext->ChunklistFileSize = *DataSize;

  *Data     = NULL;
  *DataSize = 0;

  return EFI_SUCCESS;
}

STATIC
VOID
FreeDmgPreloadContext (
  IN  OC_APPLE_DISK_IMAGE_PRELOAD_CONTEXT  *DmgPreloadContext
  )
{
  if (DmgPreloadContext->ChunklistBuffer != NULL) {
    FreePool (DmgPreloadContext->ChunklistBuffer);
  }

  DmgPreloadContext->ChunklistFileSize = 0;
  if (DmgPreloadContext->DmgFile != NULL) {
    DmgPreloadContext->DmgFile->Close (DmgPreloadContext->DmgFile);
    DmgPreloadContext->DmgFile = NULL;
  }

  DmgPreloadContext->DmgFileSize = 0;
}

//
// Equivalent to lines in original BmBoot.c which free RAM disk unconditionally
// when image loaded from RAM disk exits:
// https://github.com/tianocore/edk2/blob/a6648418c1600f0a81f2914d9dd14de1adbfe598/MdeModulePkg/Library/UefiBootManagerLib/BmBoot.c#L2061-L2071
//
EFI_STATUS
EFIAPI
HttpBootCustomFree (
  IN  VOID  *Context
  )
{
  CUSTOM_FREE_CONTEXT  *CustomFreeContext;

  if (Context != NULL) {
    CustomFreeContext = Context;
    if (CustomFreeContext->RamDiskDevicePath != NULL) {
      BmDestroyRamDisk (CustomFreeContext->RamDiskDevicePath);
      FreePool (CustomFreeContext->RamDiskDevicePath);
    }

    FreePool (CustomFreeContext);
  }

  return EFI_SUCCESS;
}

//
// Within BmExpandLoadFiles:
//  - Only DevicePath will be set if we're returning a boot file on an HTTP
//    Boot native ram disk (from .iso or .img). In this case the first and
//    second calls to LoadFile occur inside this method.
//    - The boot file is then loaded from the RAM disk (via the returned
//      device path) in a subsequent call to gBS->LoadImage made by the
//      caller.
//    - The above applies in the orginal and our modified method.
//  - Our method is modified from EDK-II original so that the second LoadFile
//    call will also be made inside the method, and Data and DataSize will be
//    filled in, if a single .efi file is loaded.
//    - In the EDK-II original, final loading of .efi files is delayed to the
//      subsequent call to GetFileBufferByFilePath in BmGetLoadOptionBuffer.
//  - Note that in the HttpBootDxe LoadFile implementation (which will be
//    used by the original and our modified code), if HTTP chunked transfer
//    encoding is used then the entire file is downloaded (chunked HTTP GET)
//    and cached (in a linked list of fixed-sized download sections, not
//    corresponding in size to the actual HTTP chunks) in order to get its
//    size, before the final buffer for the file can be allocated; then within
//    the second LoadFile call the file is transferred from this cache into
//    the final allocated buffer.
//      - For non-chunked (so, more or less, normal) transfer encoding, the
//        file size is available from a simple HTTP HEAD request, then in the
//        second LoadFile call the file HTTP GET is written directly into
//        allocated buffer.
//      - So chunked transfer encoding should ideally be avoided, especially
//        for large downloads, but is supported here and in the original.
//
EFI_STATUS
EFIAPI
HttpBootCustomRead (
  IN  OC_STORAGE_CONTEXT                   *Storage,
  IN  OC_BOOT_ENTRY                        *ChosenEntry,
  OUT VOID                                 **Data,
  OUT UINT32                               *DataSize,
  OUT EFI_DEVICE_PATH_PROTOCOL             **DevicePath,
  OUT EFI_HANDLE                           *StorageHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL             **StoragePath,
  IN  OC_DMG_LOADING_SUPPORT               DmgLoading,
  OUT OC_APPLE_DISK_IMAGE_PRELOAD_CONTEXT  *DmgPreloadContext,
  OUT VOID                                 **Context
  )
{
  EFI_STATUS                Status;
  CUSTOM_FREE_CONTEXT       *CustomFreeContext;
  CHAR8                     *OtherUri;
  BOOLEAN                   GotDmgFirst;
  EFI_DEVICE_PATH_PROTOCOL  *OtherLoadFile;
  EFI_DEVICE_PATH_PROTOCOL  *OtherDevicePath;

  ASSERT (Context != NULL);
  *Context = NULL;

  CustomFreeContext = AllocateZeroPool (sizeof (CUSTOM_FREE_CONTEXT));
  if (CustomFreeContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  gDmgLoading = DmgLoading;

  OcConsoleControlSetMode (EfiConsoleControlScreenText);

  //
  // Load the first (or only) file. This method has been extended to
  // abort early (avoiding a pointless, long, slow load of a DMG) if DmgLoading
  // is disabled and the requested file extension is `.dmg` (or `.chunklist`).
  //
  *DevicePath = BmExpandLoadFiles (ChosenEntry->DevicePath, Data, DataSize, TRUE);

  if (*DevicePath == NULL) {
    FreePool (CustomFreeContext);
    return EFI_NOT_FOUND;
  }

  Status      = EFI_SUCCESS;
  GotDmgFirst = FALSE;
  OtherUri    = NULL;

  //
  // Only potentially treat first file as .dmg/.chunklist if it was loaded as
  // a normal single file. HttpBootDxe Content-Type header handling may force
  // any file, regardless of extension, to be treated as an .iso or .img and
  // loaded as a RAM disk.
  //
  if (*DataSize != 0) {
    Status = ExtractOtherUriFromDevicePath (*DevicePath, ".dmg", ".chunklist", &OtherUri, FALSE);
    if (!EFI_ERROR (Status)) {
      GotDmgFirst = TRUE;
      Status      = SetDmgPreloadDmgFile (DmgPreloadContext, Data, DataSize);
      if (EFI_ERROR (Status)) {
        FreePool (OtherUri);
        OtherUri = NULL;
      }
    } else {
      Status = ExtractOtherUriFromDevicePath (*DevicePath, ".chunklist", ".dmg", &OtherUri, FALSE);
      if (!EFI_ERROR (Status)) {
        Status = SetDmgPreloadChunklist (DmgPreloadContext, Data, DataSize);
        if (EFI_ERROR (Status)) {
          FreePool (OtherUri);
          OtherUri = NULL;
        }
      } else if (Status == EFI_NOT_FOUND) {
        Status   = EFI_SUCCESS;
        OtherUri = NULL;
      }
    }
  }

  //
  // Is there a potential matched file?
  //
  if (OtherUri != NULL) {
    //
    // Always require .dmg if .chunklist was fetched first; only fetch (and
    // require) .chunklist after .dmg when it will be used.
    //
    if (!GotDmgFirst || (DmgLoading == OcDmgLoadingAppleSigned)) {
      Status = HttpBootAddUri (ChosenEntry->DevicePath, OtherUri, OcStringFormatAscii, &OtherLoadFile);
      if (!EFI_ERROR (Status)) {
        //
        // Sort out cramped spacing between the two HTTP Boot calls.
        // Hopefully should not affect GUI-based firmware.
        //
        Print (L"\n");

        //
        // Load the second file of .dmg/.chunklist pair.
        //
        OtherDevicePath = BmExpandLoadFiles (OtherLoadFile, Data, DataSize, TRUE);
        FreePool (OtherLoadFile);
        if (OtherDevicePath == NULL) {
          DEBUG ((DEBUG_INFO, "NTBT: Failed to fetch required matching file %a\r", OtherUri));
          Status = EFI_NOT_FOUND;
        } else {
          if (GotDmgFirst) {
            FreePool (OtherDevicePath);
            Status = SetDmgPreloadChunklist (DmgPreloadContext, Data, DataSize);
          } else {
            FreePool (*DevicePath);
            *DevicePath = OtherDevicePath;
            Status      = SetDmgPreloadDmgFile (DmgPreloadContext, Data, DataSize);
          }
        }
      }
    }

    FreePool (OtherUri);
  }

  //
  // Sort out OC debug messages following HTTP Boot progress message on the same line, after completion.
  //
  Print (L"\n");

  if (EFI_ERROR (Status)) {
    FreeDmgPreloadContext (DmgPreloadContext);
    FreePool (CustomFreeContext);
    FreePool (*DevicePath);
    *DevicePath = NULL;
    return Status;
  }

  //
  // It is okay to follow EDK-II code and check for this when it might not be there.
  //
  CustomFreeContext->RamDiskDevicePath = BmGetRamDiskDevicePath (*DevicePath);
  *Context                             = CustomFreeContext;

  return EFI_SUCCESS;
}

//
// There is no possibility of DMGs, chunklists or ISOs with PXE boot.
//
EFI_STATUS
EFIAPI
PxeBootCustomRead (
  IN  OC_STORAGE_CONTEXT                   *Storage,
  IN  OC_BOOT_ENTRY                        *ChosenEntry,
  OUT VOID                                 **Data,
  OUT UINT32                               *DataSize,
  OUT EFI_DEVICE_PATH_PROTOCOL             **DevicePath,
  OUT EFI_HANDLE                           *StorageHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL             **StoragePath,
  IN  OC_DMG_LOADING_SUPPORT               DmgLoading,
  OUT OC_APPLE_DISK_IMAGE_PRELOAD_CONTEXT  *DmgPreloadContext,
  OUT VOID                                 **Context
  )
{
  OcConsoleControlSetMode (EfiConsoleControlScreenText);

  *DevicePath = BmExpandLoadFiles (ChosenEntry->DevicePath, Data, DataSize, FALSE);

  return (*DevicePath == NULL ? EFI_NOT_FOUND : EFI_SUCCESS);
}

/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS
OcLoadAndRunImage (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath   OPTIONAL,
  IN   VOID                      *Buffer       OPTIONAL,
  IN   UINTN                     BufferSize,
  OUT  EFI_HANDLE                *ImageHandle  OPTIONAL,
  IN   CHAR16                    *OptionalData OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 NewHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  //
  // Run OpenCore image
  //
  NewHandle = NULL;
  Status = gBS->LoadImage (
    FALSE,
    gImageHandle,
    DevicePath,
    Buffer,
    BufferSize,
    &NewHandle
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCM: Failed to load image - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCM: Loaded image at %p handle\n", NewHandle));

  Status = gBS->HandleProtocol (
    NewHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCM: Loaded image has DeviceHandle %p FilePath %p ours DevicePath %p\n",
      LoadedImage->DeviceHandle,
      LoadedImage->FilePath,
      DevicePath
      ));
    //
    // Some fragile firmware fail to properly set LoadedImage when buffer is provided.
    // REF: https://github.com/acidanthera/bugtracker/issues/712
    // REF: https://github.com/acidanthera/bugtracker/issues/1502
    //
    if (DevicePath != NULL && LoadedImage->DeviceHandle == NULL) {
      Status = gBS->LocateDevicePath (
        &gEfiSimpleFileSystemProtocolGuid,
        &DevicePath,
        &LoadedImage->DeviceHandle
        );

      DEBUG ((
        DEBUG_INFO,
        "OCM: LocateDevicePath on loaded handle %p - %r\n",
        LoadedImage->DeviceHandle,
        Status
        ));

      if (!EFI_ERROR (Status)) {
        if (LoadedImage->FilePath != NULL) {
          FreePool (LoadedImage->FilePath);
        }
        LoadedImage->FilePath = DuplicateDevicePath (DevicePath);
      }
    }

    if (OptionalData != NULL) {
      LoadedImage->LoadOptionsSize = (UINT32) StrSize (OptionalData);
      LoadedImage->LoadOptions     = AllocateCopyPool (LoadedImage->LoadOptionsSize, OptionalData);
    }
  }

  Status = gBS->StartImage (
    NewHandle,
    NULL,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCM: Failed to start image - %r\n", Status));
    gBS->UnloadImage (NewHandle);
  } else if (ImageHandle != NULL) {
    *ImageHandle = NewHandle;
  }

  return Status;
}

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
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
OcLoadAndRunImage (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath  OPTIONAL,
  IN   VOID                      *Buffer      OPTIONAL,
  IN   UINTN                     BufferSize,
  OUT  EFI_HANDLE                *ImageHandle OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  NewHandle;

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
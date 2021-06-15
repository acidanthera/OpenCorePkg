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
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS
OcLoadAndRunImage (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath  OPTIONAL,
  IN   VOID                      *Buffer      OPTIONAL,
  IN   UINTN                     BufferSize,
  OUT  EFI_HANDLE                *ImageHandle OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 NewHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  UINTN                      VariableSize;
  UINT8                      *Variable;
  UINT8                      *Ptr;
  CHAR16                     *Description = L"Linux Kernel Boot";
  CHAR16                     *OptionalData = L"console=ttyS0,9600n8 console=tty0 root=UUID=25e58ba8-1b62-4038-85a2-cd5ed936e3d1 rw quiet rootfstype=ext4 add_efi_memmap initrd=\\EFI\\debian\\initrd.img\0";
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
  DEBUG ((
    DEBUG_INFO,
    "OCM: DevicePath: %S\n",
    ConvertDevicePathToText(DevicePath, FALSE, FALSE)
    ));

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
  }

  VariableSize = sizeof (UINT32)
               + sizeof (UINT16)
               + StrSize (Description)
               + GetDevicePathSize (DevicePath)
               + StrSize (OptionalData);

  Variable     = AllocatePool (VariableSize);
  ASSERT (Variable != NULL);

  Ptr             = Variable;
  WriteUnaligned32 ((UINT32 *) Ptr, LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT);
  Ptr            += sizeof (UINT32);

  WriteUnaligned16 ((UINT16 *) Ptr, (UINT16) GetDevicePathSize (DevicePath));
  Ptr            += sizeof (UINT16);

  CopyMem (Ptr, Description, StrSize (Description));
  Ptr            += StrSize (Description);

  CopyMem (Ptr, DevicePath, GetDevicePathSize (DevicePath));
  Ptr            += GetDevicePathSize (DevicePath);

  CopyMem (Ptr, OptionalData, StrSize (OptionalData));

  LoadedImage->LoadOptionsSize = VariableSize;
  LoadedImage->LoadOptions     = Variable;
  LoadedImage->ParentHandle    = NULL;

  DEBUG ((
    DEBUG_INFO,
    "OCM: LoadedImage->FilePath %S\n",
    ConvertDevicePathToText(LoadedImage->FilePath, FALSE, FALSE)
    ));

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

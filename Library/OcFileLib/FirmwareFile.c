/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/FirmwareVolume.h>
#include <Protocol/DevicePath.h>

STATIC
EFI_HANDLE
GetFvFileVolume (
  IN EFI_GUID  *FvNameGuid
  )
{
  UINTN                         Index;
  EFI_STATUS                    Status;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *FirmwareVolumeInterface;
  UINTN                         NumOfHandles;
  EFI_HANDLE                    *HandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  EFI_HANDLE                    CurrentVolume;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiFirmwareVolumeProtocolGuid,
    NULL,
    &NumOfHandles,
    &HandleBuffer
    );

  //
  // TODO: Support FirmwareVolume2?
  //
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < NumOfHandles; ++Index) {
    CurrentVolume = HandleBuffer[Index];

    Status = gBS->HandleProtocol (
      CurrentVolume,
      &gEfiFirmwareVolumeProtocolGuid,
      (VOID **) &FirmwareVolumeInterface
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = FirmwareVolumeInterface->ReadFile (
      FirmwareVolumeInterface,
      FvNameGuid,
      NULL,
      &Size,
      &Type,
      &Attributes,
      &AuthenticationStatus
      );

    if (!EFI_ERROR (Status)) {
      gBS->FreePool (HandleBuffer);
      return CurrentVolume;
    }
  }

  gBS->FreePool (HandleBuffer);
  return NULL;
}

EFI_DEVICE_PATH_PROTOCOL *
CreateFvFileDevicePath (
  IN EFI_GUID  *FileGuid
  )
{
  EFI_HANDLE                         VolumeHandle;
  EFI_DEVICE_PATH_PROTOCOL           *VolumeDevicePath;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FvFileNode;

  VolumeHandle = GetFvFileVolume (FileGuid);
  if (VolumeHandle == NULL) {
    return NULL;
  }

  VolumeDevicePath = DevicePathFromHandle (VolumeHandle);
  if (VolumeDevicePath == NULL) {
    return NULL;
  }

  EfiInitializeFwVolDevicepathNode (&FvFileNode, FileGuid);
  return AppendDevicePathNode (VolumeDevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &FvFileNode);
}

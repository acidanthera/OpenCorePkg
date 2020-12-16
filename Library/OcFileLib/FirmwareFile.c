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
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/FirmwareVolume.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DevicePath.h>

STATIC
EFI_HANDLE
GetFvFileData (
  IN  EFI_GUID          *FvNameGuid,
  IN  EFI_SECTION_TYPE  SectionType,
  OUT VOID              **FileData  OPTIONAL,
  OUT UINT32            *FileSize   OPTIONAL
  )
{
  UINTN                         Index;
  EFI_STATUS                    Status;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv2;
  UINTN                         NumOfHandles;
  EFI_HANDLE                    *HandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  EFI_HANDLE                    CurrentVolume;
  BOOLEAN                       UseFv2;

  UseFv2 = FALSE;
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiFirmwareVolumeProtocolGuid,
    NULL,
    &NumOfHandles,
    &HandleBuffer
    );

  if (EFI_ERROR (Status)) {
    Status = gBS->LocateHandleBuffer (
      ByProtocol,
      &gEfiFirmwareVolume2ProtocolGuid,
      NULL,
      &NumOfHandles,
      &HandleBuffer
      );
    UseFv2 = TRUE;
  }

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < NumOfHandles; ++Index) {
    CurrentVolume = HandleBuffer[Index];

    if (UseFv2) {
      Status = gBS->HandleProtocol (
        CurrentVolume,
        &gEfiFirmwareVolume2ProtocolGuid,
        (VOID **) &Fv2
        );
    } else {
      Status = gBS->HandleProtocol (
        CurrentVolume,
        &gEfiFirmwareVolumeProtocolGuid,
        (VOID **) &Fv
        );
    }

    if (EFI_ERROR (Status)) {
      continue;
    }

    if (UseFv2) {
      Status = Fv2->ReadFile (
        Fv2,
        FvNameGuid,
        NULL,
        &Size,
        &Type,
        &Attributes,
        &AuthenticationStatus
        );
    } else {
      Status = Fv->ReadFile (
        Fv,
        FvNameGuid,
        NULL,
        &Size,
        &Type,
        &Attributes,
        &AuthenticationStatus
        );
    }

    if (!EFI_ERROR (Status)) {
      FreePool (HandleBuffer);

      if (FileData != NULL) {
        ASSERT (FileSize != NULL);

        *FileData = NULL;
        if (UseFv2) {
          Status = Fv2->ReadSection (
            Fv2,
            FvNameGuid,
            SectionType,
            0,
            FileData,
            &Size,
            &AuthenticationStatus
            );
        } else {
          Status = Fv->ReadSection (
            Fv,
            FvNameGuid,
            SectionType,
            0,
            FileData,
            &Size,
            &AuthenticationStatus
            );
        }

        if (!EFI_ERROR (Status)) {
          *FileSize = (UINT32) Size;
        } else {
          CurrentVolume = NULL;
        }
      }

      return CurrentVolume;
    }
  }

  FreePool (HandleBuffer);
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

  VolumeHandle = GetFvFileData (FileGuid, EFI_SECTION_ALL, NULL, NULL);
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

VOID *
ReadFvFileSection (
  IN  EFI_GUID          *FileGuid,
  IN  UINT8             SectionType,
  OUT UINT32            *FileSize
  )
{
  EFI_HANDLE                         VolumeHandle;
  VOID                               *FileData;

  //
  // TODO: This is quite an ugly interface, redesign.
  //
  VolumeHandle = GetFvFileData (FileGuid, SectionType, &FileData, FileSize);
  if (VolumeHandle == NULL) {
    return NULL;
  }

  return FileData;
}

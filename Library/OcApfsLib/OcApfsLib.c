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

#include "OcApfsInternal.h"
#include <Library/OcApfsLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>

STATIC VOID *mApfsNewPartitionsEventKey;

STATIC
VOID
EFIAPI
ApfsNewPartitionArrived (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS   Status;
  UINTN        BufferSize;
  EFI_HANDLE   Handle;

  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
      ByRegisterNotify,
      NULL,
      mApfsNewPartitionsEventKey,
      &BufferSize,
      &Handle
      );
    if (!EFI_ERROR (Status)) {
      OcApfsConnectDevice (Handle, TRUE);
    } else {
      break;
    }
  }
}

STATIC
EFI_STATUS
ApfsMonitorNewPartitions (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  Status = gBS->CreateEvent (
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    ApfsNewPartitionArrived,
    NULL,
    &Event
    );

  if (!EFI_ERROR (Status)) {
    Status = gBS->RegisterProtocolNotify (
      &gEfiBlockIoProtocolGuid,
      Event,
      &mApfsNewPartitionsEventKey
      );

    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (Event);
    }
  }

  return Status;
}

EFI_STATUS
OcApfsConnectParentDevice (
  IN EFI_HANDLE  Handle  OPTIONAL,
  IN BOOLEAN     VerifyPolicy
  )
{
  EFI_STATUS       Status;
  EFI_STATUS       Status2;
  UINTN            HandleCount;
  EFI_HANDLE       *HandleBuffer;
  EFI_DEVICE_PATH  *ParentDevicePath;
  EFI_DEVICE_PATH  *ChildDevicePath;
  UINTN            Index;
  UINTN            PrefixLength;

  HandleCount = 0;
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiBlockIoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  ParentDevicePath = NULL;
  PrefixLength = 0;
  if (Handle != NULL) {
    Status2 = gBS->HandleProtocol (
      Handle,
      &gEfiDevicePathProtocolGuid,
      (VOID **) &ParentDevicePath
      );
    if (!EFI_ERROR (Status2)) {
      PrefixLength = GetDevicePathSize (ParentDevicePath) - END_DEVICE_PATH_LENGTH;
    } else {
      DEBUG ((DEBUG_INFO, "OCJS: No parent device path - %r\n", Status2));
      ParentDevicePath = NULL;
    }
  }

  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;

    for (Index = 0; Index < HandleCount; ++Index) {
      if (ParentDevicePath != NULL && PrefixLength > 0) {
        Status2 = gBS->HandleProtocol (
          HandleBuffer[Index],
          &gEfiDevicePathProtocolGuid,
          (VOID **) &ChildDevicePath
          );
        if (EFI_ERROR (Status2)) {
          DEBUG ((DEBUG_INFO, "OCJS: No child device path - %r\n", Status2));
          continue;
        }

        if (CompareMem (ParentDevicePath, ChildDevicePath, PrefixLength) == 0) {
          DEBUG ((DEBUG_INFO, "OCJS: Matched device path\n"));
        } else {
          continue;
        }
      }

      Status2 = OcApfsConnectDevice (
        HandleBuffer[Index],
        VerifyPolicy
        );
      if (!EFI_ERROR (Status2)) {
        Status = Status2;
      }
    }

    FreePool (HandleBuffer);
  } else {
    DEBUG ((DEBUG_INFO, "OCJS: BlockIo buffer error - %r\n", Status));
  }

  return Status;
}

EFI_STATUS
OcApfsConnectDevices (
  IN BOOLEAN  Monitor
  )
{
  EFI_STATUS  Status;

  if (Monitor) {
    Status = ApfsMonitorNewPartitions ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCJS: Failed to setup drive monitoring - %r\n", Status));
    }
  }

  return OcApfsConnectParentDevice (NULL, TRUE);
}

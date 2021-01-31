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

#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/OcDriverConnectionLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PlatformDriverOverride.h>

//
// NULL-terminated list of driver handles that will be served by EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL.
//
STATIC EFI_HANDLE *mPriorityDrivers;

//
// Saved original EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL.GetDriver when doing override.
//
STATIC EFI_PLATFORM_DRIVER_OVERRIDE_GET_DRIVER mOrgPlatformGetDriver;

STATIC
EFI_STATUS
EFIAPI
OcPlatformGetDriver (
  IN     EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          *This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     *DriverImageHandle
  )
{
  EFI_HANDLE     *HandlePtr;
  BOOLEAN        FoundLast;

  if (ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We have no custom overrides.
  //
  if (mPriorityDrivers == NULL || mPriorityDrivers[0] == NULL) {
    //
    // Forward request to the original driver if we have it.
    //
    if (mOrgPlatformGetDriver != NULL) {
      return mOrgPlatformGetDriver (This, ControllerHandle, DriverImageHandle);
    }

    //
    // Report not found for the first request.
    //
    if (*DriverImageHandle == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // Report invalid parameter for the handle we could not have returned.
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Handle first driver in the overrides.
  //
  if (*DriverImageHandle == NULL) {
    *DriverImageHandle = mPriorityDrivers[0];
    return EFI_SUCCESS;
  }

  //
  // Otherwise lookup the current driver in the list.
  //

  FoundLast = FALSE;

  for (HandlePtr = &mPriorityDrivers[0]; HandlePtr[0] != NULL; ++HandlePtr) {
    //
    // Found driver in the list, return next one.
    //
    if (HandlePtr[0] == *DriverImageHandle) {
      *DriverImageHandle = HandlePtr[1];
      //
      // Next driver is not last, return it.
      //
      if (*DriverImageHandle != NULL) {
        return EFI_SUCCESS;
      }

      //
      // Next driver is last, exit the loop.
      //
      FoundLast = TRUE;
      break;
    }
  }

  //
  // We have no original protocol.
  //
  if (mOrgPlatformGetDriver == NULL) {
    //
    // Finalise the list by reporting not found.
    //
    if (FoundLast) {
      return EFI_NOT_FOUND;
    }

    //
    // Error as the passed handle is not valid.
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Forward the call to the original driver:
  // - if FoundLast, then it is starting to iterating original list and DriverImageHandle
  //   is nulled above.
  // - otherwise, then it is iterating the original list or is invalid.
  //
  return mOrgPlatformGetDriver (This, ControllerHandle, DriverImageHandle);
}

STATIC
EFI_STATUS
EFIAPI
OcPlatformGetDriverPath (
  IN     EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          *This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                       **DriverImagePath
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
OcPlatformDriverLoaded (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          *This,
  IN EFI_HANDLE                                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL                       *DriverImagePath,
  IN EFI_HANDLE                                     DriverImageHandle
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL
mOcPlatformDriverOverrideProtocol = {
  OcPlatformGetDriver,
  OcPlatformGetDriverPath,
  OcPlatformDriverLoaded
};

EFI_STATUS
OcRegisterDriversToHighestPriority (
  IN EFI_HANDLE  *PriorityDrivers
  )
{
  EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL  *PlatformDriverOverride;
  EFI_STATUS                             Status;
  EFI_HANDLE                             NewHandle;

  ASSERT (PriorityDrivers != NULL);

  mPriorityDrivers = PriorityDrivers;
  Status = gBS->LocateProtocol (
    &gEfiPlatformDriverOverrideProtocolGuid,
    NULL,
    (VOID **) &PlatformDriverOverride
    );

  if (!EFI_ERROR(Status)) {
    mOrgPlatformGetDriver = PlatformDriverOverride->GetDriver;
    PlatformDriverOverride->GetDriver = OcPlatformGetDriver;
    return Status;
  }

  NewHandle = NULL;
  return gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gEfiPlatformDriverOverrideProtocolGuid,
    &mOcPlatformDriverOverrideProtocol,
    NULL
    );
}

EFI_STATUS
OcConnectDrivers (
  VOID
  )
{
  EFI_STATUS                           Status;
  UINTN                                HandleCount;
  EFI_HANDLE                           *HandleBuffer;
  UINTN                                DeviceIndex;
  UINTN                                HandleIndex;
  UINTN                                ProtocolIndex;
  UINTN                                InfoIndex;
  EFI_GUID                             **ProtocolGuids;
  UINTN                                ProtocolCount;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *ProtocolInfos;
  UINTN                                ProtocolInfoCount;
  BOOLEAN                              Connect;

  //
  // We locate only handles with device paths as connecting other handles
  // will crash APTIO IV.
  //
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiDevicePathProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (DeviceIndex = 0; DeviceIndex < HandleCount; ++DeviceIndex) {
    //
    // Only connect parent handles as we connect recursively.
    // This improves the performance by more than 30 seconds
    // with drives installed into Marvell SATA controllers on APTIO IV.
    //
    Connect = TRUE;
    for (HandleIndex = 0; HandleIndex < HandleCount && Connect; ++HandleIndex) {
      //
      // Retrieve the list of all the protocols on each handle
      //
      Status = gBS->ProtocolsPerHandle (
        HandleBuffer[HandleIndex],
        &ProtocolGuids,
        &ProtocolCount
        );

      if (EFI_ERROR (Status)) {
        continue;
      }

      for (ProtocolIndex = 0; ProtocolIndex < ProtocolCount && Connect; ++ProtocolIndex) {
        //
        // Retrieve the list of agents that have opened each protocol
        //
        Status = gBS->OpenProtocolInformation (
          HandleBuffer[HandleIndex],
          ProtocolGuids[ProtocolIndex],
          &ProtocolInfos,
          &ProtocolInfoCount
          );

        if (!EFI_ERROR (Status)) {
          for (InfoIndex = 0; InfoIndex < ProtocolInfoCount && Connect; ++InfoIndex) {
            if (ProtocolInfos[InfoIndex].ControllerHandle == HandleBuffer[DeviceIndex]
              && (ProtocolInfos[InfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) == EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
              Connect = FALSE;
            }
          }
          FreePool (ProtocolInfos);
        }
      }

      FreePool (ProtocolGuids);
    }

    if (Connect) {
      //
      // We connect all handles to all drivers as otherwise fs drivers may not be seen.
      //
      gBS->ConnectController (HandleBuffer[DeviceIndex], NULL, NULL, TRUE);
    }
  }

  FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

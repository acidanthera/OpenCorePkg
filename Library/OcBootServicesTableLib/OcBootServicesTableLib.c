/** @file
  This library retrieve the EFI_BOOT_SERVICES pointer from EFI system table in
  library's constructor and allow overriding its properties.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/OcBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>

typedef struct OC_REGISTERED_PROTOCOL_ {
  EFI_GUID   *ProtocolGuid;
  VOID       *ProtocolInstance;
  BOOLEAN    Override;
} OC_REGISTERED_PROTOCOL;

EFI_HANDLE                      gImageHandle       = NULL;
EFI_SYSTEM_TABLE                *gST               = NULL;
EFI_BOOT_SERVICES               *gBS               = NULL;

STATIC EFI_CONNECT_CONTROLLER    mConnectController = NULL;
STATIC EFI_OPEN_PROTOCOL         mOpenProtocol = NULL;
STATIC EFI_LOCATE_HANDLE_BUFFER  mLocateHandleBuffer = NULL;
STATIC EFI_LOCATE_PROTOCOL       mLocateProtocol    = NULL;
STATIC OC_REGISTERED_PROTOCOL    mRegisteredProtocols[16];
STATIC UINTN                     mRegisteredProtocolCount = 0;

STATIC
EFI_STATUS
EFIAPI
OcConnectController (
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    *DriverImageHandle   OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath OPTIONAL,
  IN  BOOLEAN                       Recursive
  )
{
  EFI_STATUS  Status;
  VOID        *DevicePath;

  if (ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do not connect controllers without device paths.
  //
  // REF: https://bugzilla.tianocore.org/show_bug.cgi?id=2460
  //
  // Doing this reduces the amount of needless work during device
  // connection and resolves issues with firmware that freeze
  // when connecting handles without device paths.
  //
  Status = gBS->HandleProtocol (
    ControllerHandle,
    &gEfiDevicePathProtocolGuid,
    &DevicePath
    );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = mConnectController (
    ControllerHandle,
    DriverImageHandle,
    RemainingDevicePath,
    Recursive
    );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OcOpenProtocol (
  IN  EFI_HANDLE                Handle,
  IN  EFI_GUID                  *Protocol,
  OUT VOID                      **Interface OPTIONAL,
  IN  EFI_HANDLE                AgentHandle,
  IN  EFI_HANDLE                ControllerHandle,
  IN  UINT32                    Attributes
  )
{
  EFI_STATUS                 Status;
  UINTN                      Index;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  Status = mOpenProtocol (
    Handle,
    Protocol,
    Interface,
    AgentHandle,
    ControllerHandle,
    Attributes
    );

  //
  // On Apple EFI some paths may not be valid for our DevicePath library.
  // As a result we can end up in an infinite loop in GetImageNameFromHandle
  // calling "drivers" in the Shell. Workaround by discarding unsupported paths.
  //
  if (!EFI_ERROR (Status)
    && CompareGuid (Protocol, &gEfiLoadedImageProtocolGuid)
    && Interface != NULL) {
    LoadedImage = *Interface;

    if (LoadedImage->FilePath != NULL && !IsDevicePathValid (LoadedImage->FilePath, 0)) {
      LoadedImage->FilePath = NULL;
    }
  }

  if (Status != EFI_UNSUPPORTED || Handle != gImageHandle) {
    return Status;
  }

  if (Interface == NULL && Attributes != EFI_OPEN_PROTOCOL_TEST_PROTOCOL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Process all protocols as overrides are not specific to handle.
  //
  for (Index = 0; Index < mRegisteredProtocolCount; ++Index) {
    if (CompareGuid (mRegisteredProtocols[Index].ProtocolGuid, Protocol)) {
      if (Interface != NULL) {
        *Interface = mRegisteredProtocols[Index].ProtocolInstance;
      }
      return EFI_SUCCESS;
    }
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OcLocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN     EFI_GUID                     *Protocol      OPTIONAL,
  IN     VOID                         *SearchKey     OPTIONAL,
  IN OUT UINTN                        *NoHandles,
  OUT    EFI_HANDLE                   **Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  Status = mLocateHandleBuffer (
    SearchType,
    Protocol,
    SearchKey,
    NoHandles,
    Buffer
    );

  if (Status == EFI_NOT_FOUND && SearchType == ByProtocol) {
    //
    // Process all protocols as overrides are not specific to handle.
    //
    for (Index = 0; Index < mRegisteredProtocolCount; ++Index) {
      if (CompareGuid (mRegisteredProtocols[Index].ProtocolGuid, Protocol)) {
        Status = gBS->AllocatePool (
          EfiBootServicesData,
          sizeof (**Buffer),
          (VOID **) Buffer
          );
        if (!EFI_ERROR (Status)) {
          *NoHandles = 1;
          **Buffer   = gImageHandle;
          return EFI_SUCCESS;
        }

        return EFI_NOT_FOUND;
      }
    }
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OcLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if (Protocol == NULL || Interface == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Process overriding protocols first.
  //
  for (Index = 0; Index < mRegisteredProtocolCount; ++Index) {
    if (mRegisteredProtocols[Index].Override
      && CompareGuid (mRegisteredProtocols[Index].ProtocolGuid, Protocol)) {
      *Interface = mRegisteredProtocols[Index].ProtocolInstance;
      return EFI_SUCCESS;
    }
  }

  //
  // Call the original function second.
  //
  Status = mLocateProtocol (
    Protocol,
    Registration,
    Interface
    );

  //
  // Process fallback protocols third.
  //
  if (EFI_ERROR (Status)) {
    for (Index = 0; Index < mRegisteredProtocolCount; ++Index) {
      if (!mRegisteredProtocols[Index].Override
        && CompareGuid (mRegisteredProtocols[Index].ProtocolGuid, Protocol)) {
        *Interface = mRegisteredProtocols[Index].ProtocolInstance;
        return EFI_SUCCESS;
      }
    }
  }

  return Status;
}

EFI_STATUS
OcRegisterBootServicesProtocol (
  IN EFI_GUID   *ProtocolGuid,
  IN VOID       *ProtocolInstance,
  IN BOOLEAN    Override
  )
{
  if (mRegisteredProtocolCount == ARRAY_SIZE (mRegisteredProtocols)) {
    return EFI_OUT_OF_RESOURCES;
  }

  mRegisteredProtocols[mRegisteredProtocolCount].ProtocolGuid     = ProtocolGuid;
  mRegisteredProtocols[mRegisteredProtocolCount].ProtocolInstance = ProtocolInstance;
  mRegisteredProtocols[mRegisteredProtocolCount].Override         = Override;

  ++mRegisteredProtocolCount;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcBootServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Assert on invalid startup arguments.
  //
  ASSERT (ImageHandle != NULL);
  ASSERT (SystemTable != NULL);
  ASSERT (SystemTable->BootServices != NULL);

  //
  // Cache the Image Handle.
  //
  gImageHandle = ImageHandle;

  //
  // Cache EFI System Table.
  // Note, we cannot override it as it may be used for ConOut
  // spoofing to redirect output for tools.
  //
  gST = SystemTable;

  //
  // Allocate a copy of EFI Boot Services Table.
  //
  Status = SystemTable->BootServices->AllocatePool (
    EfiBootServicesData,
    SystemTable->BootServices->Hdr.HeaderSize,
    (VOID **) &gBS
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Copy the original EFI Boot Services Table into our custom table.
  //
  CopyMem (
    gBS,
    SystemTable->BootServices,
    SystemTable->BootServices->Hdr.HeaderSize
    );

  //
  // Override boot services functions and rehash.
  //
  mConnectController      = gBS->ConnectController;
  mOpenProtocol           = gBS->OpenProtocol;
  mLocateHandleBuffer     = gBS->LocateHandleBuffer;
  mLocateProtocol         = gBS->LocateProtocol;
  gBS->ConnectController  = OcConnectController;
  gBS->OpenProtocol       = OcOpenProtocol;
  gBS->LocateHandleBuffer = OcLocateHandleBuffer;
  gBS->LocateProtocol     = OcLocateProtocol;
  gBS->Hdr.CRC32          = 0;
  SystemTable->BootServices->CalculateCrc32 (
    gBS,
    gBS->Hdr.HeaderSize,
    &gBS->Hdr.CRC32
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcBootServicesTableLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Free memory for table copies.
  //
  SystemTable->BootServices->FreePool (gBS);
  return EFI_SUCCESS;
}

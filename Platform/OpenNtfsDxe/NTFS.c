/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Helper.h"

#define LANGUAGE_CODE_ENGLISH  "eng"

CHAR16  *gNTFSDriverName = L"NTFS Driver";

EFI_COMPONENT_NAME_PROTOCOL
  gNTFSDriverNames = {
  .GetDriverName      = NTFSCtlDriverName,
  .GetControllerName  = NTFSCtlGetControllerName,
  .SupportedLanguages = LANGUAGE_CODE_ENGLISH
};

EFI_DRIVER_BINDING_PROTOCOL
  gNTFSDriverBinding = {
  .Supported           = NTFSSupported,
  .Start               = NTFSStart,
  .Stop                = NTFSStop,
  .Version             = 0x10U,
  .ImageHandle         = NULL,
  .DriverBindingHandle = NULL
};

EFI_STATUS
EFIAPI
NTFSEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not open loaded image protocol - %r\n", Status));
    return Status;
  }

  gNTFSDriverBinding.ImageHandle         = ImageHandle;
  gNTFSDriverBinding.DriverBindingHandle = ImageHandle;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gNTFSDriverBinding,
                  &gEfiComponentNameProtocolGuid,
                  &gNTFSDriverNames,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not bind driver - %r\n", Status));
    return Status;
  }

  LoadedImage->Unload = UnloadNTFSDriver;

  return Status;
}

EFI_STATUS
EFIAPI
UnloadNTFSDriver (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_HANDLE  *Buffer;
  UINTN       NumOfHandles;
  UINTN       Index;
  EFI_STATUS  Status;

  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &NumOfHandles,
                  &Buffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; ++Index) {
    gBS->DisconnectController (Buffer[Index], ImageHandle, NULL);
  }

  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gNTFSDriverBinding,
                  &gEfiComponentNameProtocolGuid,
                  &gNTFSDriverNames,
                  NULL
                  );

  return Status;
}

EFI_STATUS
EFIAPI
NTFSSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_FS      *Instance;

  Instance = AllocateZeroPool (sizeof (EFI_FS));
  if (Instance == NULL) {
    DEBUG ((DEBUG_INFO, "NTFS: Out of memory.\n"));
    return EFI_UNSUPPORTED;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&Instance->BlockIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&Instance->DiskIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  Status = NtfsMount (Instance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: This is not NTFS Volume.\n"));
    Status = EFI_UNSUPPORTED;
  } else {
    FreeAttr (&Instance->RootIndex->Attr);
    FreeAttr (&Instance->MftStart->Attr);
    FreePool (Instance->RootIndex->FileRecord);
    FreePool (Instance->MftStart->FileRecord);
    FreePool (Instance->RootIndex->File);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDiskIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (Instance);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL               **Root
  )
{
  EFI_FS  *Instance;

  ASSERT (This != NULL);
  ASSERT (Root != NULL);

  Instance = (EFI_FS *)This;

  *Root = (EFI_FILE_PROTOCOL *)Instance->RootIndex->File;

  return EFI_SUCCESS;
}

/**
   Installs Simple File System Protocol
**/
EFI_STATUS
EFIAPI
NTFSStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_FS      *Instance;

  Instance = AllocateZeroPool (sizeof (EFI_FS));
  if (Instance == NULL) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not allocate a new file system instance\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->FileIoInterface.Revision   = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Instance->FileIoInterface.OpenVolume = OpenVolume;

  Instance->DevicePath = DevicePathFromHandle (Controller);
  if (Instance->DevicePath == NULL) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not get Device Path\n"));
    FreePool (Instance);
    return EFI_NO_MAPPING;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&Instance->BlockIo,
                  This->DriverBindingHandle,
                  Controller,

                  /*
                   * EFI_OPEN_PROTOCOL_BY_DRIVER would return Access Denied here,
                   * because the disk driver has that protocol already open. So use
                   * EFI_OPEN_PROTOCOL_GET_PROTOCOL (which doesn't require us to close it).
                   */
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not access BlockIO protocol - %r\n", Status));
    FreePool (Instance);
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&Instance->DiskIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not access DiskIO protocol - %r\n", Status));
    FreePool (Instance);
    return Status;
  }

  Instance->EfiFile.Revision    = EFI_FILE_PROTOCOL_REVISION2;
  Instance->EfiFile.Open        = FileOpen;
  Instance->EfiFile.Close       = FileClose;
  Instance->EfiFile.Delete      = FileDelete;
  Instance->EfiFile.Read        = FileRead;
  Instance->EfiFile.Write       = FileWrite;
  Instance->EfiFile.GetPosition = FileGetPosition;
  Instance->EfiFile.SetPosition = FileSetPosition;
  Instance->EfiFile.GetInfo     = FileGetInfo;
  Instance->EfiFile.SetInfo     = FileSetInfo;
  Instance->EfiFile.Flush       = FileFlush;

  Status = NtfsMount (Instance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not mount file system.\n"));
    FreePool (Instance);
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Instance->FileIoInterface,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not install simple file system protocol - %r\n", Status));
    gBS->CloseProtocol (
           Controller,
           &gEfiDiskIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    FreeAttr (&Instance->RootIndex->Attr);
    FreeAttr (&Instance->MftStart->Attr);
    FreePool (Instance->RootIndex->FileRecord);
    FreePool (Instance->MftStart->FileRecord);
    FreePool (Instance->RootIndex->File);

    FreePool (Instance);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
   Uninstall Simple File system Protocol
**/
EFI_STATUS
EFIAPI
NTFSStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *NTFS;
  EFI_FS                           *Instance;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&NTFS,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not locate our instance - %r\n", Status));
    return Status;
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NTFS,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close DISK_IO Protocol
  //
  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiDiskIoProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Instance = (EFI_FS *)NTFS;

  FreeAttr (&Instance->RootIndex->Attr);
  FreeAttr (&Instance->MftStart->Attr);
  FreePool (Instance->RootIndex->FileRecord);
  FreePool (Instance->MftStart->FileRecord);
  FreePool (Instance->RootIndex->File);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NTFSCtlDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN CHAR8                        *Language,
  OUT CHAR16                      **DriverName
  )
{
  *DriverName = gNTFSDriverName;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NTFSCtlGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_HANDLE                   ChildHandle      OPTIONAL,
  IN CHAR8                        *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

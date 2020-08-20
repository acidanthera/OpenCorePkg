/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BootManagementInternal.h"

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#include <Guid/AppleVariable.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/OcVariable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleSecureBootLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

STATIC EFI_IMAGE_LOAD mOriginalEfiLoadImage;

STATIC
EFI_STATUS
InternalEfiLoadImageFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT UINTN                     *FileSize,
  OUT VOID                      **FileBuffer
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *File;
  VOID               *Buffer;
  UINT32             Size;

  Status = OcOpenFileByDevicePath (
    &DevicePath,
    &File,
    EFI_FILE_MODE_READ,
    0
    );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = GetFileSize (
    File,
    &Size
    );
  if (EFI_ERROR (Status) || Size == 0) {
    File->Close (File);
    return EFI_UNSUPPORTED;
  }

  Buffer = AllocatePool (Size);
  if (Buffer == NULL) {
    File->Close (File);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetFileData (
    File,
    0,
    Size,
    Buffer
    );
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    File->Close (File);
    return EFI_DEVICE_ERROR;
  }

  *FileBuffer = Buffer;
  *FileSize   = Size;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalEfiLoadImageProtocol (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  BOOLEAN                   UseLoadImage2,
  OUT UINTN                     *FileSize,
  OUT VOID                      **FileBuffer
  )
{
  //
  // TODO: Implement image load protocol if necessary.
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
InternalUpdateLoadedImage (
  IN EFI_HANDLE                ImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 DeviceHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath;

  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RemainingDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &RemainingDevicePath, &DeviceHandle);
  if (EFI_ERROR (Status)) {
    //
    // TODO: Handle load protocol if necessary.
    //
    return Status;
  }

  if (LoadedImage->DeviceHandle != DeviceHandle) {
    LoadedImage->DeviceHandle = DeviceHandle;
    LoadedImage->FilePath     = DuplicateDevicePath (RemainingDevicePath);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
InternalEfiLoadImage (
  IN  BOOLEAN                      BootPolicy,
  IN  EFI_HANDLE                   ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath,
  IN  VOID                         *SourceBuffer OPTIONAL,
  IN  UINTN                        SourceSize,
  OUT EFI_HANDLE                   *ImageHandle
  )
{
  EFI_STATUS                 SecureBootStatus;
  EFI_STATUS                 Status;
  VOID                       *AllocatedBuffer;
  UINT32                     RealSize;

  if (ParentImageHandle == NULL || ImageHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (SourceBuffer == NULL && DevicePath == NULL) {
    return EFI_NOT_FOUND;
  }

  if (SourceBuffer != NULL && SourceSize == 0) {
    return EFI_UNSUPPORTED;
  }

  AllocatedBuffer = NULL;
  if (SourceBuffer == NULL) {
    Status = InternalEfiLoadImageFile (
      DevicePath,
      &SourceSize,
      &SourceBuffer
      );
    if (EFI_ERROR (Status)) {
      Status = InternalEfiLoadImageProtocol (
        DevicePath,
        BootPolicy == FALSE,
        &SourceSize,
        &SourceBuffer
        );
    }

    if (!EFI_ERROR (Status)) {
      AllocatedBuffer = SourceBuffer;
    }
  }

  if (DevicePath != NULL && SourceBuffer != NULL) {
    SecureBootStatus = OcAppleSecureBootVerify (
      DevicePath,
      SourceBuffer,
      SourceSize
      );
  } else {
    SecureBootStatus = EFI_UNSUPPORTED;
  }

  //
  // A security violation means we should just die.
  //
  if (SecureBootStatus == EFI_SECURITY_VIOLATION) {
    DEBUG ((
      DEBUG_WARN,
      "OCB: Apple Secure Boot prohibits this boot entry, enforcing!\n"
      ));
    return EFI_SECURITY_VIOLATION;
  }

  if (SourceBuffer != NULL) {
    RealSize = (UINT32) SourceSize;
#ifdef MDE_CPU_IA32
    Status = FatFilterArchitecture32 ((UINT8 **) &SourceBuffer, &RealSize);
#else
    Status = FatFilterArchitecture64 ((UINT8 **) &SourceBuffer, &RealSize);
#endif
    if (!EFI_ERROR (Status)) {
      SourceSize = RealSize;
    } else if (AllocatedBuffer != NULL) {
      SourceBuffer = NULL;
      SourceSize   = 0;
    }
  }

  if (SecureBootStatus == EFI_SUCCESS) {
    //
    // TODO: Here we should use a custom COFF loader!
    //
  }

  Status = mOriginalEfiLoadImage (
    BootPolicy,
    ParentImageHandle,
    DevicePath,
    SourceBuffer,
    SourceSize,
    ImageHandle
    );

  if (AllocatedBuffer != NULL) {
    FreePool (AllocatedBuffer);
  }

  //
  // Some firmwares may not update loaded image protocol fields correctly
  // when loading via source buffer. Do it here.
  //
  if (!EFI_ERROR (Status) && SourceBuffer != NULL && DevicePath != NULL) {
    InternalUpdateLoadedImage (*ImageHandle, DevicePath);
  }

  return Status;
}

EFI_STATUS
InternalInitImageLoader (
  VOID
  )
{
  mOriginalEfiLoadImage = gBS->LoadImage;
  gBS->LoadImage = InternalEfiLoadImage;

  gBS->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);
  return EFI_SUCCESS;
}

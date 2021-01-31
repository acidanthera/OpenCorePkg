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
#include <Protocol/LoadedImage.h>
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
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcPeCoffLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

#if defined(MDE_CPU_IA32)
  #define OC_IMAGE_FILE_MACHINE  IMAGE_FILE_MACHINE_I386
#elif defined(MDE_CPU_X64)
  #define OC_IMAGE_FILE_MACHINE  IMAGE_FILE_MACHINE_X64
#else
  #error Unsupported architecture.
#endif

STATIC EFI_GUID mOcLoadedImageProtocolGuid = {
  0x1f3c963d, 0xf9dc, 0x4537, { 0xbb, 0x06, 0xd8, 0x08, 0x46, 0x4a, 0x85, 0x2e }
};

typedef struct {
  EFI_IMAGE_ENTRY_POINT      EntryPoint;
  EFI_PHYSICAL_ADDRESS       ImageArea;
  UINTN                      PageCount;
  EFI_STATUS                 Status;
  VOID                       *JumpBuffer;
  BASE_LIBRARY_JUMP_BUFFER   *JumpContext;
  CHAR16                     *ExitData;
  UINTN                      ExitDataSize;
  UINT16                     Subsystem;
  BOOLEAN                    Started;
  EFI_LOADED_IMAGE_PROTOCOL  LoadedImage;
} OC_LOADED_IMAGE_PROTOCOL;

STATIC EFI_IMAGE_LOAD   mOriginalEfiLoadImage;
STATIC EFI_IMAGE_START  mOriginalEfiStartImage;
STATIC EFI_IMAGE_UNLOAD mOriginalEfiUnloadImage;
STATIC EFI_EXIT         mOriginalEfiExit;
STATIC EFI_HANDLE       mCurrentImageHandle;

STATIC OC_IMAGE_LOADER_PATCH     mImageLoaderPatch;
STATIC OC_IMAGE_LOADER_CONFIGURE mImageLoaderConfigure;
STATIC UINT32                    mImageLoaderCaps;
STATIC BOOLEAN                   mImageLoaderEnabled;

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

EFI_STATUS
EFIAPI
OcImageLoaderLoad (
  IN  BOOLEAN                  BootPolicy,
  IN  EFI_HANDLE               ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN  VOID                     *SourceBuffer OPTIONAL,
  IN  UINTN                    SourceSize,
  OUT EFI_HANDLE               *ImageHandle
  )
{
  EFI_STATUS                   Status;
  EFI_STATUS                   ImageStatus;
  PE_COFF_IMAGE_CONTEXT        ImageContext;
  EFI_PHYSICAL_ADDRESS         DestinationArea;
  VOID                         *DestinationBuffer;
  OC_LOADED_IMAGE_PROTOCOL     *OcLoadedImage;
  EFI_LOADED_IMAGE_PROTOCOL    *LoadedImage;

  ASSERT (SourceBuffer != NULL);

  //
  // Initialize the image context.
  //
  ImageStatus = PeCoffInitializeContext (
    &ImageContext,
    SourceBuffer,
    SourceSize
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff init failure - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }
  //
  // Reject images that are not meant for the platform's architecture.
  //
  if (ImageContext.Machine != OC_IMAGE_FILE_MACHINE) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff wrong machine - %x\n", ImageContext.Machine));
    return EFI_UNSUPPORTED;
  }
  //
  // Reject RT drivers for the moment.
  //
  if (ImageContext.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff no support for RT drivers\n"));
    return EFI_UNSUPPORTED;
  }
  //
  // Allocate the image destination memory.
  // FIXME: RT drivers require EfiRuntimeServicesCode.
  //
  Status = gBS->AllocatePages (
    AllocateAnyPages,
    ImageContext.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION
      ? EfiLoaderCode : EfiBootServicesCode,
    EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage),
    &DestinationArea
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DestinationBuffer = (VOID *)(UINTN) DestinationArea;

  //
  // Load SourceBuffer into DestinationBuffer.
  //
  ImageStatus = PeCoffLoadImage (
    &ImageContext,
    DestinationBuffer,
    ImageContext.SizeOfImage
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff load image error - %r\n", ImageStatus));
    FreePages (DestinationBuffer, EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage));
    return EFI_UNSUPPORTED;
  }
  //
  // Relocate the loaded image to the destination address.
  //
  ImageStatus = PeCoffRelocateImage (
    &ImageContext,
    (UINTN) DestinationBuffer,
    NULL,
    0
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff relocate image error - %r\n", ImageStatus));
    FreePages (DestinationBuffer, EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage));
    return EFI_UNSUPPORTED;
  }
  //
  // Construct a LoadedImage protocol for the image.
  //
  OcLoadedImage = AllocateZeroPool (sizeof (*OcLoadedImage));
  if (OcLoadedImage == NULL) {
    FreePages (DestinationBuffer, EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage));
    return EFI_OUT_OF_RESOURCES;
  }

  OcLoadedImage->EntryPoint = (EFI_IMAGE_ENTRY_POINT) ((UINTN) DestinationBuffer + ImageContext.AddressOfEntryPoint);
  OcLoadedImage->ImageArea  = DestinationArea;
  OcLoadedImage->PageCount  = EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage);
  OcLoadedImage->Subsystem  = ImageContext.Subsystem;

  LoadedImage = &OcLoadedImage->LoadedImage;

  LoadedImage->Revision        = EFI_LOADED_IMAGE_INFORMATION_REVISION;
  LoadedImage->ParentHandle    = ParentImageHandle;
  LoadedImage->SystemTable     = gST;
  LoadedImage->ImageBase       = DestinationBuffer;
  LoadedImage->ImageSize       = ImageContext.SizeOfImage;
  //
  // FIXME: Support RT drivers.
  //
  if (ImageContext.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
    LoadedImage->ImageCodeType   = EfiLoaderCode;
    LoadedImage->ImageDataType   = EfiLoaderData;
  } else {
    LoadedImage->ImageCodeType   = EfiBootServicesCode;
    LoadedImage->ImageDataType   = EfiBootServicesData;
  }
  //
  // Install LoadedImage and the image's entry point.
  //
  *ImageHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    LoadedImage,
    &mOcLoadedImageProtocolGuid,
    OcLoadedImage,
    NULL
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff proto install error - %r\n", Status));
    FreePool (OcLoadedImage);
    FreePages (DestinationBuffer, EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "OCB: Loaded image at %p\n", *ImageHandle));

  return EFI_SUCCESS;
}

/**
  Unload image routine for OcImageLoaderLoad.

  @param[in]  OcLoadedImage     Our loaded image instance.
  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
STATIC
EFI_STATUS
InternalDirectUnloadImage (
  IN  OC_LOADED_IMAGE_PROTOCOL  *OcLoadedImage,
  IN  EFI_HANDLE                ImageHandle
  )
{
  EFI_STATUS                Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

  LoadedImage = &OcLoadedImage->LoadedImage;
  if (LoadedImage->Unload != NULL) {
    Status = LoadedImage->Unload (ImageHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Do not allow to execute Unload multiple times.
    //
    LoadedImage->Unload = NULL;
  } else if (OcLoadedImage->Started) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    LoadedImage,
    &mOcLoadedImageProtocolGuid,
    OcLoadedImage,
    NULL
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->FreePages (OcLoadedImage->ImageArea, OcLoadedImage->PageCount);
  FreePool (OcLoadedImage);
  //
  // NOTE: Avoid EFI 1.10 extension of closing opened protocols.
  //
  return EFI_SUCCESS;
}

/**
  Unload image routine for OcImageLoaderLoad.

  @param[in]  OcLoadedImage     Our loaded image instance.
  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.
  @param[in]  ExitStatus        The image's exit code.
  @param[in]  ExitDataSize      The size, in bytes, of ExitData. Ignored if ExitStatus is EFI_SUCCESS.
  @param[in]  ExitData          The pointer to a data buffer that includes a Null-terminated string,
                                optionally followed by additional binary data. The string is a
                                description that the caller may use to further indicate the reason
                                for the image's exit. ExitData is only valid if ExitStatus
                                is something other than EFI_SUCCESS. The ExitData buffer
                                must be allocated by calling AllocatePool().

  @retval EFI_SUCCESS           The image has been unloaded.
**/
STATIC
EFI_STATUS
InternalDirectExit (
  IN  OC_LOADED_IMAGE_PROTOCOL     *OcLoadedImage,
  IN  EFI_HANDLE                   ImageHandle,
  IN  EFI_STATUS                   ExitStatus,
  IN  UINTN                        ExitDataSize,
  IN  CHAR16                       *ExitData     OPTIONAL
  )
{
  EFI_TPL                    OldTpl;

  DEBUG ((
    DEBUG_VERBOSE, "OCB: Exit %p %p (%d) - %r\n",
    ImageHandle,
    mCurrentImageHandle,
    OcLoadedImage->Started,
    ExitStatus
    ));

  //
  // Prevent possible reentrance to this function for the same ImageHandle.
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // If the image has not been started just free its resources.
  // Should not happen normally.
  //
  if (!OcLoadedImage->Started) {
    InternalDirectUnloadImage (OcLoadedImage, ImageHandle);
    gBS->RestoreTPL (OldTpl);
    return EFI_SUCCESS;
  }

  //
  // If the image has been started, verify this image can exit.
  //
  if (ImageHandle != mCurrentImageHandle) {
    DEBUG ((DEBUG_LOAD|DEBUG_ERROR, "Exit: Image is not exitable image\n"));
    gBS->RestoreTPL (OldTpl);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set the return status.
  //
  OcLoadedImage->Status = ExitStatus;

  //
  // If there's ExitData info provide it.
  //
  if (ExitData != NULL) {
    OcLoadedImage->ExitDataSize = ExitDataSize;
    OcLoadedImage->ExitData = AllocatePool (OcLoadedImage->ExitDataSize);
    if (OcLoadedImage->ExitData != NULL) {
      CopyMem (OcLoadedImage->ExitData, ExitData, OcLoadedImage->ExitDataSize);
    } else {
      OcLoadedImage->ExitDataSize = 0;
    }
  }

  //
  // return to StartImage
  //
  gBS->RestoreTPL (OldTpl);
  LongJump (OcLoadedImage->JumpContext, (UINTN)-1);

  //
  // If we return from LongJump, then it is an error
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
  return EFI_ACCESS_DENIED;
}


/**
  Simplified start image routine for OcImageLoaderLoad.

  @param[in]   OcLoadedImage     Our loaded image instance.
  @param[in]   ImageHandle       Handle of image to be started.
  @param[out]  ExitDataSize      The pointer to the size, in bytes, of ExitData.
  @param[out]  ExitData          The pointer to a pointer to a data buffer that includes a Null-terminated
                                 string, optionally followed by additional binary data.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
InternalDirectStartImage (
  IN  OC_LOADED_IMAGE_PROTOCOL  *OcLoadedImage,
  IN  EFI_HANDLE                ImageHandle,
  OUT UINTN                     *ExitDataSize,
  OUT CHAR16                    **ExitData OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  LastImage;
  UINTN       SetJumpFlag;

  //
  // Push the current image.
  //
  LastImage = mCurrentImageHandle;
  mCurrentImageHandle = ImageHandle;

  //
  // Set long jump for Exit() support
  // JumpContext must be aligned on a CPU specific boundary.
  // Overallocate the buffer and force the required alignment
  //
  OcLoadedImage->JumpBuffer = AllocatePool (
    sizeof (BASE_LIBRARY_JUMP_BUFFER) + BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT
    );
  if (OcLoadedImage->JumpBuffer == NULL) {
    //
    // Pop the current start image context
    //
    mCurrentImageHandle = LastImage;
    return EFI_OUT_OF_RESOURCES;
  }

  OcLoadedImage->JumpContext = ALIGN_POINTER (
    OcLoadedImage->JumpBuffer, BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT
    );

  SetJumpFlag = SetJump (OcLoadedImage->JumpContext);
  //
  // The initial call to SetJump() must always return 0.
  // Subsequent calls to LongJump() cause a non-zero value to be returned by SetJump().
  //
  if (SetJumpFlag == 0) {
    //
    // Invoke the manually loaded image entry point.
    //
    DEBUG ((DEBUG_VERBOSE, "OCB: Starting image %p\n", ImageHandle));
    OcLoadedImage->Started = TRUE;
    OcLoadedImage->Status = OcLoadedImage->EntryPoint (
      ImageHandle,
      OcLoadedImage->LoadedImage.SystemTable
      );
    //
    // If the image returns, exit it through Exit()
    //
    InternalDirectExit (OcLoadedImage, ImageHandle, OcLoadedImage->Status, 0, NULL);
  }

  FreePool (OcLoadedImage->JumpBuffer);

  //
  // Pop the current image.
  //
  mCurrentImageHandle = LastImage;

  //
  // NOTE: EFI 1.10 is not supported, refer to
  // https://github.com/tianocore/edk2/blob/d8dd54f071cfd60a2dcf5426764a89cd91213420/MdeModulePkg/Core/Dxe/Image/Image.c#L1686-L1697
  //

  //
  //  Return the exit data to the caller
  //
  if (ExitData != NULL && ExitDataSize != NULL) {
    *ExitDataSize = OcLoadedImage->ExitDataSize;
    *ExitData     = OcLoadedImage->ExitData;
  } else if (OcLoadedImage->ExitData != NULL) {
    //
    // Caller doesn't want the exit data, free it
    //
    FreePool (OcLoadedImage->ExitData);
    OcLoadedImage->ExitData = NULL;
  }

  //
  // Save the Status because Image will get destroyed if it is unloaded.
  //
  Status = OcLoadedImage->Status;

  //
  // If the image returned an error, or if the image is an application
  // unload it
  //
  if (EFI_ERROR (OcLoadedImage->Status)
    || OcLoadedImage->Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
    InternalDirectUnloadImage (OcLoadedImage, ImageHandle);
  }

  return Status;
}

/**
  Detect kernel capabilities from EfiBoot image.

  @param[in] SourceBuffer  Buffer containing EfiBoot.
  @param[in] SourceSize    Size of EfiBoot buffer.

  @returns OC_KERN_CAPABILITY bitmask.
**/
STATIC
UINT32
DetectCapabilities (
  IN  VOID                         *SourceBuffer,
  IN  UINT32                       SourceSize
  )
{
  INT32  Result;

  //
  // Find Mac OS X version pattern.
  // This pattern started to appear with 10.7.
  //
  Result = FindPattern (
    (CONST UINT8 *)"Mac OS X 10.",
    NULL,
    L_STR_LEN ("Mac OS X 10."),
    SourceBuffer,
    SourceSize - sizeof (UINT32),
    0
    );

#ifdef MDE_CPU_IA32
  //
  // For IA32 mode the only question is whether we support K32_64.
  // This starts with 10.7, and in theory is valid for some early
  // developer preview 10.8 images, so simply decide on Mac OS X
  // version pattern presence.
  //
  if (Result >= 0) {
    return OC_KERN_CAPABILITY_K32_U64;
  }
  return OC_KERN_CAPABILITY_K32_U32 | OC_KERN_CAPABILITY_K32_U64;
#else
  //
  // For X64 mode, when the pattern is found, this can be 10.7 or 10.8+.
  // 10.7 supports K32_64 and K64, while newer versions have only K64.
  //
  if (Result >= 0) {
    if (((UINT8 *)SourceBuffer)[Result + L_STR_LEN ("Mac OS X 10.")] == '7') {
      return OC_KERN_CAPABILITY_K32_U64 | OC_KERN_CAPABILITY_K64_U64;
    }
    return OC_KERN_CAPABILITY_K64_U64;
  }

  //
  // The pattern is not found. This can be 10.6 or 10.4~10.5.
  // 10.6 supports K32 and K64, while older versions have only K32.
  // Detect 10.6 by x86_64 pattern presence.
  //
  Result = FindPattern (
    (CONST UINT8 *)"x86_64",
    NULL,
    L_STR_SIZE ("x86_64"),
    SourceBuffer,
    SourceSize - sizeof (UINT32),
    (INT32) (SourceSize / 2)
    );
  if (Result >= 0) {
    return OC_KERN_CAPABILITY_K32_U32 | OC_KERN_CAPABILITY_K32_U64 | OC_KERN_CAPABILITY_K64_U64;
  }
  return OC_KERN_CAPABILITY_K32_U32 | OC_KERN_CAPABILITY_K32_U64;
#endif
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

  if (DevicePath != NULL && SourceBuffer != NULL && mImageLoaderEnabled) {
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

  //
  // By default assume target default.
  //
#ifdef MDE_CPU_IA32
  mImageLoaderCaps = OC_KERN_CAPABILITY_K32_U32 | OC_KERN_CAPABILITY_K32_U64;
#else
  mImageLoaderCaps = OC_KERN_CAPABILITY_K64_U64;
#endif

  if (SourceBuffer != NULL) {
    RealSize = (UINT32) SourceSize;
#ifdef MDE_CPU_IA32
    Status = FatFilterArchitecture32 ((UINT8 **) &SourceBuffer, &RealSize);
#else
    Status = FatFilterArchitecture64 ((UINT8 **) &SourceBuffer, &RealSize);
#endif

    //
    // This is FAT image.
    // Determine its capabilities.
    //
    if (!EFI_ERROR (Status) && RealSize != SourceSize && RealSize >= EFI_PAGE_SIZE) {
      mImageLoaderCaps = DetectCapabilities (SourceBuffer, RealSize);
    }

    DEBUG ((
      DEBUG_INFO,
      "OCB: Arch filtering %p(%u)->%p(%u) caps %u - %r\n",
      AllocatedBuffer,
      (UINT32) SourceSize,
      SourceBuffer,
      RealSize,
      mImageLoaderCaps,
      Status
      ));

    if (!EFI_ERROR (Status)) {
      SourceSize = RealSize;
    } else if (AllocatedBuffer != NULL) {
      SourceBuffer = NULL;
      SourceSize   = 0;
    }
  }

  if (SourceBuffer != NULL && mImageLoaderPatch != NULL) {
    mImageLoaderPatch (
      DevicePath,
      SourceBuffer,
      SourceSize
      );
  }

  //
  // Load the image ourselves in secure boot mode.
  //
  if (SecureBootStatus == EFI_SUCCESS) {
    if (SourceBuffer != NULL) {
      Status = OcImageLoaderLoad (
        FALSE,
        ParentImageHandle,
        DevicePath,
        SourceBuffer,
        SourceSize,
        ImageHandle
        );
    } else {
      //
      // We verified the image, but contained garbage.
      // This should not happen, just abort.
      //
      Status = EFI_UNSUPPORTED;
    }
  } else {
    Status = mOriginalEfiLoadImage (
      BootPolicy,
      ParentImageHandle,
      DevicePath,
      SourceBuffer,
      SourceSize,
      ImageHandle
      );
  }

  if (AllocatedBuffer != NULL) {
    FreePool (AllocatedBuffer);
  }

  //
  // Some types of firmware may not update loaded image protocol fields correctly
  // when loading via source buffer. Do it here.
  //
  if (!EFI_ERROR (Status) && SourceBuffer != NULL && DevicePath != NULL) {
    InternalUpdateLoadedImage (*ImageHandle, DevicePath);
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
InternalEfiStartImage (
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData OPTIONAL
  )
{
  EFI_STATUS                 Status;
  OC_LOADED_IMAGE_PROTOCOL   *OcLoadedImage;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  //
  // If we loaded the image, invoke the entry point manually.
  //
  Status = gBS->HandleProtocol (
    ImageHandle,
    &mOcLoadedImageProtocolGuid,
    (VOID **) &OcLoadedImage
    );
  if (!EFI_ERROR (Status)) {
    //
    // Call configure update for our images.
    //
    if (mImageLoaderConfigure != NULL) {
      mImageLoaderConfigure (
        &OcLoadedImage->LoadedImage,
        mImageLoaderCaps
        );
    }

    return InternalDirectStartImage (
      OcLoadedImage,
      ImageHandle,
      ExitDataSize,
      ExitData
      );
  }

  //
  // Call configure update for generic images too.
  //
  if (mImageLoaderConfigure != NULL) {
    Status = gBS->HandleProtocol (
      ImageHandle,
      &gEfiLoadedImageProtocolGuid,
      (VOID **) &LoadedImage
      );
    if (!EFI_ERROR (Status)) {
      mImageLoaderConfigure (
        LoadedImage,
        mImageLoaderCaps
        );
    }
  }

  return mOriginalEfiStartImage (ImageHandle, ExitDataSize, ExitData);
}

STATIC
EFI_STATUS
EFIAPI
InternalEfiUnloadImage (
  IN  EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                Status;
  OC_LOADED_IMAGE_PROTOCOL  *OcLoadedImage;

  //
  // If we loaded the image, do the unloading manually.
  //
  Status = gBS->HandleProtocol (
    ImageHandle,
    &mOcLoadedImageProtocolGuid,
    (VOID **) &OcLoadedImage
    );
  if (!EFI_ERROR (Status)) {
    return InternalDirectUnloadImage (
      OcLoadedImage,
      ImageHandle
      );
  }

  return mOriginalEfiUnloadImage (ImageHandle);
}

STATIC
EFI_STATUS
EFIAPI
InternalEfiExit (
  IN  EFI_HANDLE                   ImageHandle,
  IN  EFI_STATUS                   ExitStatus,
  IN  UINTN                        ExitDataSize,
  IN  CHAR16                       *ExitData     OPTIONAL
  )
{
  EFI_STATUS                Status;
  OC_LOADED_IMAGE_PROTOCOL  *OcLoadedImage;

  //
  // If we loaded the image, do the exit manually.
  //
  Status = gBS->HandleProtocol (
    ImageHandle,
    &mOcLoadedImageProtocolGuid,
    (VOID **) &OcLoadedImage
    );

  DEBUG ((DEBUG_VERBOSE, "OCB: InternalEfiExit %p - %r / %r\n", ImageHandle, ExitStatus, Status));

  if (!EFI_ERROR (Status)) {
    return InternalDirectExit (
      OcLoadedImage,
      ImageHandle,
      ExitStatus,
      ExitDataSize,
      ExitData
      );
  }

  return mOriginalEfiExit (ImageHandle, ExitStatus, ExitDataSize, ExitData);
}

VOID
OcImageLoaderInit (
  VOID
  )
{
  mOriginalEfiLoadImage   = gBS->LoadImage;
  mOriginalEfiStartImage  = gBS->StartImage;
  mOriginalEfiUnloadImage = gBS->UnloadImage;
  mOriginalEfiExit        = gBS->Exit;

  gBS->LoadImage   = InternalEfiLoadImage;
  gBS->StartImage  = InternalEfiStartImage;
  gBS->UnloadImage = InternalEfiUnloadImage;
  gBS->Exit        = InternalEfiExit;

  gBS->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);
}

VOID
OcImageLoaderActivate (
  VOID
  )
{
  mImageLoaderEnabled = TRUE;
}

VOID
OcImageLoaderRegisterPatch (
  IN OC_IMAGE_LOADER_PATCH  Patch      OPTIONAL
  )
{
  mImageLoaderPatch  = Patch;
}

VOID
OcImageLoaderRegisterConfigure (
  IN OC_IMAGE_LOADER_CONFIGURE  Configure  OPTIONAL
  )
{
  mImageLoaderConfigure = Configure;
}

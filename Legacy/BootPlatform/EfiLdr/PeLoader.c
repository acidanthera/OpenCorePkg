/*++
Copyright (c) 2023, Mikhail Krichanov. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  PeLoader.c

Abstract:

Revision History:

--*/
#include "EfiLdr.h"
#include "Support.h"

#include <Library/UefiImageLib.h>

EFI_STATUS
EfiLdrLoadImage (
  IN VOID                   *FHand,
  IN UINT32                 BufferSize,
  IN EFILDR_LOADED_IMAGE    *Image,
  IN UINTN                  *NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR  *EfiMemoryDescriptor
  )
{
  EFI_STATUS                       Status;
  UEFI_IMAGE_LOADER_IMAGE_CONTEXT  ImageContext;

  Status = UefiImageInitializeContext (
             &ImageContext,
             FHand,
             BufferSize,
             UEFI_IMAGE_SOURCE_FV
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set the image subsystem type
  //
  Image->Type = UefiImageGetSubsystem (&ImageContext);

  switch (Image->Type) {
    case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
      Image->Info.ImageCodeType = EfiLoaderCode;
      Image->Info.ImageDataType = EfiLoaderData;
      break;

    case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
      Image->Info.ImageCodeType = EfiBootServicesCode;
      Image->Info.ImageDataType = EfiBootServicesData;
      break;

    case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
      Image->Info.ImageCodeType = EfiRuntimeServicesCode;
      Image->Info.ImageDataType = EfiRuntimeServicesData;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  Image->NoPages = EFI_SIZE_TO_PAGES (BufferSize);

  //
  // Compute the amount of memory needed to load the image and
  // allocate it.  Memory starts off as data.
  //
  Image->ImageBasePage = (EFI_PHYSICAL_ADDRESS)(UINTN)FindSpace (
                                                        Image->NoPages,
                                                        NumberOfMemoryMapEntries,
                                                        EfiMemoryDescriptor,
                                                        EfiRuntimeServicesCode,
                                                        EFI_MEMORY_WB
                                                        );
  if (Image->ImageBasePage == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  Image->Info.ImageBase = (VOID *)(UINTN)Image->ImageBasePage;
  Image->Info.ImageSize = (Image->NoPages << EFI_PAGE_SHIFT) - 1;
  Image->ImageBase      = (UINT8 *)(UINTN)Image->ImageBasePage;
  Image->ImageEof       = Image->ImageBase + Image->Info.ImageSize;
  Image->ImageAdjust    = Image->ImageBase;

  //
  // Load and relocate image
  //
  Status = UefiImageLoadImageForExecution (
             &ImageContext,
             Image->ImageBase,
             (UINT32)EFI_PAGES_TO_SIZE (Image->NoPages),
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Image->EntryPoint = (EFI_IMAGE_ENTRY_POINT)UefiImageLoaderGetImageEntryPoint (&ImageContext);

  return Status;
}

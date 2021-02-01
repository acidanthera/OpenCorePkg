/** @file
  Copyright (c) 2018, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "../Include/Uefi.h"

#include <Library/OcPeCoffLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <stdio.h>
#include <string.h>

#include <UserFile.h>

EFI_STATUS
TestImageLoad (
  IN VOID   *SourceBuffer,
  IN UINTN  SourceSize
  )
{
  EFI_STATUS                   Status;
  EFI_STATUS                   ImageStatus;
  PE_COFF_IMAGE_CONTEXT        ImageContext;      
  EFI_PHYSICAL_ADDRESS         DestinationArea;
  VOID                         *DestinationBuffer;
 

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
  if (ImageContext.Machine != IMAGE_FILE_MACHINE_X64) {
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
    EfiBootServicesCode,
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

  FreePages (DestinationBuffer, EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage));

  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff relocate image error - %d\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

int ENTRY_POINT (int argc, char *argv[]) {
  if (argc < 2) {
    printf ("Please provide a valid PE image path\n");
    return -1;
  }

  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  uint8_t *Image;
  uint32_t ImageSize;

  if ((Image = UserReadFile (argv[1], &ImageSize)) == NULL) {
    printf ("Read fail\n");
    return 1;
  }

  EFI_STATUS Status = TestImageLoad (Image, ImageSize);
  free(Image);
  if (EFI_ERROR (Status)) {
    return 1;
  }

  return 0;
}

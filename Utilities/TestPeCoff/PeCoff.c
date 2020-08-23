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

#include <File.h>

/**

clang -g -fsanitize=undefined,address -Wno-incompatible-pointer-types-discards-qualifiers -fshort-wchar -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h DiskImage.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLibInternal.c ../../Library/OcMiscLib/DataPatcher.c ../../Library/OcCompressionLib/zlib/zlib_uefi.c ../../Library/OcCompressionLib/zlib/adler32.c ../../Library/OcCompressionLib/zlib/deflate.c ../../Library/OcCompressionLib/zlib/crc32.c  ../../Library/OcCompressionLib/zlib/compress.c ../../Library/OcCompressionLib/zlib/infback.c ../../Library/OcCompressionLib/zlib/inffast.c  ../../Library/OcCompressionLib/zlib/inflate.c  ../../Library/OcCompressionLib/zlib/inftrees.c ../../Library/OcCompressionLib/zlib/trees.c ../../Library/OcCompressionLib/zlib/uncompr.c ../../Library/OcCryptoLib/Sha256.c  ../../Library/OcCryptoLib/Rsa2048Sha256.c ../../Library/OcAppleKeysLib/OcAppleKeysLib.c ../../Library/OcAppleChunklistLib/OcAppleChunklistLib.c ../../Library/OcAppleRamDiskLib/OcAppleRamDiskLib.c ../../Library/OcFileLib/ReadFile.c ../../Library/OcFileLib/FileProtocol.c -o DiskImage

clang-mp-7.0 -DFUZZING_TEST=1 -g -fsanitize=undefined,address,fuzzer -Wno-incompatible-pointer-types-discards-qualifiers -fshort-wchar -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h DiskImage.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLibInternal.c ../../Library/OcMiscLib/DataPatcher.c ../../Library/OcCompressionLib/zlib/zlib_uefi.c ../../Library/OcCompressionLib/zlib/adler32.c ../../Library/OcCompressionLib/zlib/deflate.c ../../Library/OcCompressionLib/zlib/crc32.c  ../../Library/OcCompressionLib/zlib/compress.c ../../Library/OcCompressionLib/zlib/infback.c ../../Library/OcCompressionLib/zlib/inffast.c  ../../Library/OcCompressionLib/zlib/inflate.c  ../../Library/OcCompressionLib/zlib/inftrees.c ../../Library/OcCompressionLib/zlib/trees.c ../../Library/OcCompressionLib/zlib/uncompr.c ../../Library/OcCryptoLib/Sha256.c  ../../Library/OcCryptoLib/Rsa2048Sha256.c ../../Library/OcAppleKeysLib/OcAppleKeysLib.c ../../Library/OcAppleChunklistLib/OcAppleChunklistLib.c ../../Library/OcAppleRamDiskLib/OcAppleRamDiskLib.c../../Library/OcFileLib/ReadFile.c ../../Library/OcFileLib/FileProtocol.c -o DiskImage
rm -rf DICT fuzz*.log ; mkdir DICT ; UBSAN_OPTIONS='halt_on_error=1' ./DiskImage -jobs=4 DICT -rss_limit_mb=4096

**/

#ifdef FUZZING_TEST
#define main no_main
#include <sanitizer/asan_interface.h>
#else
#define ASAN_POISON_MEMORY_REGION(addr, size)
#define ASAN_UNPOISON_MEMORY_REGION(addr, size)
#endif

#if 0
#include <CommonCrypto/CommonDigest.h>
#endif

EFI_STATUS
TestImageLoad (
  IN VOID   *SourceBuffer,
  IN UINTN  SourceSize
  )
{
  EFI_STATUS                   Status;
  IMAGE_STATUS                 ImageStatus;
  PE_COFF_LOADER_IMAGE_CONTEXT ImageContext;      
  EFI_PHYSICAL_ADDRESS         DestinationArea;
  VOID                         *DestinationBuffer;
 

  //
  // Initialize the image context.
  //
  ImageStatus = OcPeCoffLoaderInitializeContext (
    &ImageContext,
    SourceBuffer,
    SourceSize
    );
  if (ImageStatus != IMAGE_ERROR_SUCCESS) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff init failure - %d\n", ImageStatus));
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
  ImageStatus = OcPeCoffLoaderLoadImage (
    &ImageContext,
    DestinationBuffer,
    ImageContext.SizeOfImage
    );
  if (ImageStatus != IMAGE_ERROR_SUCCESS) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff load image error - %d\n", ImageStatus));
    FreePages (DestinationBuffer, EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage));
    return EFI_UNSUPPORTED;
  }
  //
  // Relocate the loaded image to the destination address.
  //
  ImageStatus = OcPeCoffLoaderRelocateImage (
    &ImageContext,
    (UINTN) DestinationBuffer
    );

  FreePages (DestinationBuffer, EFI_SIZE_TO_PAGES (ImageContext.SizeOfImage));

  if (ImageStatus != IMAGE_ERROR_SUCCESS) {
    DEBUG ((DEBUG_INFO, "OCB: PeCoff relocate image error - %d\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

int main (int argc, char *argv[]) {
  if (argc < 2) {
    printf ("Please provide a valid PE image path\n");
    return -1;
  }

  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  uint8_t *Image;
  uint32_t ImageSize;

  if ((Image = readFile (argv[1], &ImageSize)) == NULL) {
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

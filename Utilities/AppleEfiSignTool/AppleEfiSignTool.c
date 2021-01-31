/** @file

AppleEfiSignTool – Tool for signing and verifying Apple EFI binaries.

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <UserFile.h>
#include <Library/OcAppleImageVerificationLib.h>
#include <Library/OcMachoLib.h>
#include <Library/DebugLib.h>

static char UsageBanner[] = "AppleEfiSignTool v1.0 – Tool for verifying Apple EFI binaries\n"
                            "It supports PE and Fat binaries.\n"
                            "Usage:\n"
                            "  -i : input file\n"
                            "  -h : show this text\n"
                            "Example: ./AppleEfiSignTool -i apfs.efi\n";


int main(int argc, char *argv[]) {
  int Opt;

  if (argc < 2) {
    puts(UsageBanner);
    exit(EXIT_FAILURE);
  }

  uint8_t *Image      = NULL;
  uint32_t ImageSize  = 0;

  while ((Opt = getopt (argc, argv, "i:vh")) != -1) {
    switch (Opt) {
      case 'i': {
        //
        // Open input file
        //
        Image = UserReadFile(optarg, &ImageSize);
        if (Image == NULL) {
          printf("Cannot read: %s\n", optarg);
          exit(EXIT_FAILURE);
        }
        break;
      }
      case 'h': {
        puts(UsageBanner);
        free(Image);
        exit(0);
        break;
      }
      default:
        puts(UsageBanner);
        free(Image);
        exit(EXIT_FAILURE);
    }
  }

  if (Image == NULL) {
    puts(UsageBanner);
    exit(EXIT_FAILURE);
  }

  uint32_t OrgImageSize = ImageSize;
  uint8_t *OrgImage = Image;
  bool HasFat = false;

  int code = EXIT_SUCCESS;

  EFI_STATUS Status = FatFilterArchitecture32 (&Image, &ImageSize);
  if (!EFI_ERROR (Status) && OrgImageSize != ImageSize) {
    DEBUG ((DEBUG_ERROR, "SIGN: Discovered 32-bit slice\n"));
    uint32_t BaseImageSize = ImageSize;
    Status = VerifyApplePeImageSignature (
      Image,
      &ImageSize
      );
    DEBUG ((DEBUG_ERROR, "SIGN: Signature check - %r (%u -> %u)\n", Status, BaseImageSize, ImageSize));
    HasFat = true;
    if (EFI_ERROR (Status)) {
      code = EXIT_FAILURE;
    }
  }

  ImageSize = OrgImageSize;
  Image = OrgImage;
  Status = FatFilterArchitecture64 (&Image, &ImageSize);
  if (!EFI_ERROR (Status) && OrgImageSize != ImageSize) {
    DEBUG ((DEBUG_ERROR, "SIGN: Discovered 64-bit slice\n"));
    uint32_t BaseImageSize = ImageSize;
    Status = VerifyApplePeImageSignature (
      Image,
      &ImageSize
      );
    DEBUG ((DEBUG_ERROR, "SIGN: Signature check - %r (%u -> %u)\n", Status, BaseImageSize, ImageSize));
    HasFat = true;
    if (EFI_ERROR (Status)) {
      code = EXIT_FAILURE;
    }
  }

  ImageSize = OrgImageSize;
  Image = OrgImage;
  if (!HasFat) {
    DEBUG ((DEBUG_ERROR, "SIGN: Discovered slim slice\n"));
    uint32_t BaseImageSize = ImageSize;
    Status = VerifyApplePeImageSignature (
      Image,
      &ImageSize
      );
    DEBUG ((DEBUG_ERROR, "SIGN: Signature check - %r (%u -> %u)\n", Status, BaseImageSize, ImageSize));
    if (EFI_ERROR (Status)) {
      code = EXIT_FAILURE;
    }
  }

  free(OrgImage);

  return code;
}

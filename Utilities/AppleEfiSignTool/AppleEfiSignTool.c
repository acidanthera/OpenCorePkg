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
#include <string.h>
#include <UserFile.h>
#include <Library/OcPeCoffExtLib.h>
#include <Library/OcMachoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

static char UsageBanner[] = "AppleEfiSignTool v1.0 – Tool for verifying Apple EFI binaries\n"
                            "It supports PE and Fat binaries.\n"
                            "Usage:\n"
                            "  -i : input file\n"
                            "  -h : show this text\n"
                            "Example: ./AppleEfiSignTool -i apfs.efi\n";

enum {
  PE_ARCH_32,
  PE_ARCH_64,
  PE_ARCH_ANY
};

static int VerifySignature(uint8_t *Image, uint32_t ImageSize, bool *IsFat, int arch) {
  EFI_STATUS Status = EFI_SUCCESS;
  uint32_t OrgImageSize = ImageSize;
  const char *Slice;
  if (arch == PE_ARCH_32) {
    Status = FatFilterArchitecture32 (&Image, &ImageSize);
    Slice = "32-bit";
  } else if (arch == PE_ARCH_64) {
    Status = FatFilterArchitecture64 (&Image, &ImageSize);
    Slice = "64-bit";
  } else {
    Slice = "raw";
  }

  if (EFI_ERROR (Status)) {
    return 0;
  }

  if (OrgImageSize == ImageSize && arch != PE_ARCH_ANY) {
    return 0;
  }

  if (OrgImageSize != ImageSize) {
    *IsFat = true;
  }

  DEBUG ((DEBUG_ERROR, "SIGN: Discovered %a slice\n", Slice));
  OrgImageSize = ImageSize;
  Status = PeCoffVerifyAppleSignature (
    Image,
    &ImageSize
    );
  DEBUG ((
    DEBUG_ERROR,
    "SIGN: Signature check - %r (%u -> %u)\n",
    Status,
    OrgImageSize,
    ImageSize
    ));
  if (EFI_ERROR (Status)) {
    return EXIT_FAILURE;
  }
  return 0;
}

int ENTRY_POINT(int argc, char *argv[]) {
  int Opt;

  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

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

  bool HasFat = false;
  int code = EXIT_SUCCESS;
  code |= VerifySignature(Image, ImageSize, &HasFat, PE_ARCH_32);
  code |= VerifySignature(Image, ImageSize, &HasFat, PE_ARCH_64);
  if (!HasFat) {
    code |= VerifySignature(Image, ImageSize, &HasFat, PE_ARCH_ANY);
  }

  APFS_DRIVER_VERSION *DriverVersion;
  EFI_STATUS Status = PeCoffGetApfsDriverVersion (Image, ImageSize, &DriverVersion);
  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SIGN: Got APFS %Lu (%-16a %-16a)\n",
      DriverVersion->Version,
      DriverVersion->Date,
      DriverVersion->Time
      ));
  }

  free(Image);

  return code;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
#if 0
  APFS_DRIVER_VERSION *DriverVersion;
  EFI_STATUS Status = PeCoffGetApfsDriverVersion ((UINT8*)Data, (UINT32)Size, &DriverVersion);
  if (!EFI_ERROR (Status)) {
    volatile size_t p = 0;
    for (size_t i = 0; i < sizeof (*DriverVersion); i++) {
      p += ((uint8_t *)DriverVersion)[i];
    }
  }
#endif

  if (Size > 0 && Size <= 1024*1024*1024) {
    void *Copy = malloc(Size);
    if (Copy == NULL) {
      abort();
    }
    memcpy(Copy, Data, Size);
    UINT32  NewSize = (UINT32) Size;
    PeCoffVerifyAppleSignature(Copy, &NewSize);
    free(Copy);
  }

  return 0;
}

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

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcPeCoffExtLib.h>
#include <Library/PcdLib.h>

#include <UserFile.h>

#define  APPLE_EFI_SIGN_TOOL_VERSION  "1.0"

typedef enum {
  PE_ARCH_32,
  PE_ARCH_64,
  PE_ARCH_ANY
} PE_IMAGE_ARCH;

STATIC
VOID
PrintHelp (
  VOID
  )
{
  DEBUG ((
    DEBUG_ERROR,
    "AppleEfiSignTool v%a – Tool for verifying Apple EFI binaries\n",
    APPLE_EFI_SIGN_TOOL_VERSION
    ));
  DEBUG ((DEBUG_ERROR, "It supports PE and Fat binaries.\n"));

  DEBUG ((DEBUG_ERROR, "Usage: ./AppleEfiSignTool <path/to/image>\n"));
  DEBUG ((DEBUG_ERROR, "Example: ./AppleEfiSignTool path/to/apfs.efi\n"));
}

/**
  Verify PE image signature.

  @param[in,out]  Image      A pointer to image file buffer.
  @param[in]      ImageSize  Size of Image.
  @param[out]     IsFat      Will be set to TRUE if Image is a FAT binary.
  @param[in]      Arch       Image architechure.

  @return  0 if Image is successfully verified, otherwise EXIT_FAILURE.
**/
STATIC
INT32
VerifySignature (
  IN  OUT  UINT8          *Image,
  IN       UINT32         ImageSize,
  OUT  BOOLEAN            *IsFat,
  IN       PE_IMAGE_ARCH  Arch
  )
{
  EFI_STATUS   Status;
  UINT32       OrgImageSize;
  CONST CHAR8  *Slice;

  Status       = EFI_SUCCESS;
  OrgImageSize = ImageSize;

  if (Arch == PE_ARCH_32) {
    Status = FatFilterArchitecture32 (&Image, &ImageSize);
    Slice  = "32-bit";
  } else if (Arch == PE_ARCH_64) {
    Status = FatFilterArchitecture64 (&Image, &ImageSize);
    Slice  = "64-bit";
  } else {
    Slice = "raw";
  }

  if (EFI_ERROR (Status)) {
    return 0;
  }

  if ((OrgImageSize == ImageSize) && (Arch != PE_ARCH_ANY)) {
    return 0;
  }

  if (OrgImageSize != ImageSize) {
    *IsFat = TRUE;
  }

  DEBUG ((DEBUG_ERROR, "SIGN: Discovered %a slice\n", Slice));
  OrgImageSize = ImageSize;
  Status       = PeCoffVerifyAppleSignature (
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

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  CONST CHAR8          *ImageFileName;
  UINT32               ImageSize;
  UINT8                *ImageFileBuffer;
  BOOLEAN              IsFat;
  INT32                RetVal;
  APFS_DRIVER_VERSION  *DriverVersion;
  EFI_STATUS           Status;

  //
  // Enable PCD debug logging.
  //
  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  //
  // Print usage.
  //
  if (argc != 2) {
    PrintHelp ();
    return EXIT_FAILURE;
  }

  ImageFileName   = argv[1];
  ImageFileBuffer = UserReadFile (ImageFileName, &ImageSize);
  if (ImageFileBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to read %a\n", ImageFileName));
    return EXIT_FAILURE;
  }

  IsFat   = FALSE;
  RetVal  = EXIT_SUCCESS;
  RetVal |= VerifySignature (ImageFileBuffer, ImageSize, &IsFat, PE_ARCH_32);
  RetVal |= VerifySignature (ImageFileBuffer, ImageSize, &IsFat, PE_ARCH_64);
  if (!IsFat) {
    RetVal |= VerifySignature (ImageFileBuffer, ImageSize, &IsFat, PE_ARCH_ANY);
  }

  Status = PeCoffGetApfsDriverVersion (ImageFileBuffer, ImageSize, &DriverVersion);
  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SIGN: Got APFS %Lu (%-16a %-16a)\n",
      DriverVersion->Version,
      DriverVersion->Date,
      DriverVersion->Time
      ));
  }

  FreePool (ImageFileBuffer);

  return RetVal;
}

int
LLVMFuzzerTestOneInput (
  const uint8_t  *Data,
  size_t         Size
  )
{
 #if 0
  APFS_DRIVER_VERSION  *DriverVersion;
  EFI_STATUS           Status;
  volatile UINTN       Walker;
  UINTN                Index;

  Status = PeCoffGetApfsDriverVersion ((UINT8 *)Data, (UINT32)Size, &DriverVersion);
  if (!EFI_ERROR (Status)) {
    Walker = 0;
    for (Index = 0; Index < sizeof (*DriverVersion); ++Index) {
      Walker += ((UINT8 *)DriverVersion)[Index];
    }
  }

 #endif

  VOID    *Copy;
  UINT32  NewSize;

  if ((Size > 0) && (Size <= 1024*1024*1024)) {
    Copy = AllocatePool (Size);
    if (Copy != NULL) {
      CopyMem (Copy, Data, Size);

      NewSize = (UINT32)Size;
      PeCoffVerifyAppleSignature (Copy, &NewSize);
      FreePool (Copy);
    }
  }

  return 0;
}

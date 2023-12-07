/** @file

AppleEfiSignTool – Tool for signing and verifying Apple EFI binaries.

Copyright (c) 2018-2023, savvas, PMheart, vit9696, mikebeaton

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
#include <Library/OcStringLib.h>
#include <Library/PcdLib.h>

#include <UserFile.h>

#define  APPLE_EFI_SIGN_TOOL_VERSION  "1.1"

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
  DEBUG ((DEBUG_ERROR, "Supports PE and Fat binaries.\n"));
  DEBUG ((DEBUG_ERROR, "Displays signing info. Displays APFS info if image is APFS driver.\n"));
  DEBUG ((DEBUG_ERROR, "Also shows PE COFF image fixups when -f is specified.\n"));

  DEBUG ((DEBUG_ERROR, "\nUsage: ./AppleEfiSignTool [-f] <path/to/image>\n"));
  DEBUG ((DEBUG_ERROR, " -f force FixupAppleEfiImages quirk\n"));

  DEBUG ((DEBUG_ERROR, "\nExample: ./AppleEfiSignTool path/to/apfs.efi\n"));
}

/**
  Verify PE image signature. Report APFS info if present.

  @param[in,out]  Image      A pointer to image file buffer.
  @param[in]      ImageSize  Size of Image.
  @param[out]     IsFat      Will be set to TRUE if Image is a FAT binary, or left unchanged otherwise.
  @param[in]      Arch       Image architechure.

  @return  0 if Image is successfully verified, otherwise EXIT_FAILURE.
**/
STATIC
INT32
VerifySignatureAndApfs (
  IN  OUT  UINT8          *Image,
  IN       UINT32         ImageSize,
  IN OUT   BOOLEAN        *IsFat,
  IN       BOOLEAN        ForceFixup,
  IN       PE_IMAGE_ARCH  Arch
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    ApfsStatus;
  UINT32                        OrgImageSize;
  CONST CHAR8                   *Slice;
  PE_COFF_LOADER_IMAGE_CONTEXT  Context;
  RETURN_STATUS                 ContextStatus;
  APFS_DRIVER_VERSION           *DriverVersion;

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

  if (ForceFixup) {
    ContextStatus = RETURN_VOLUME_CORRUPTED;
  } else {
    ContextStatus = PeCoffInitializeContext (
                      &Context,
                      Image,
                      ImageSize,
                      UefiImageOriginUserImage
                      );
  }

  if (ContextStatus == RETURN_VOLUME_CORRUPTED) {
    DEBUG ((DEBUG_ERROR, "SIGN: Trying legacy fixup...\n", ContextStatus));
    ContextStatus = OcPeCoffFixupInitializeContext (
                      &Context,
                      Image,
                      ImageSize,
                      FALSE
                      );
  }

  if (EFI_ERROR (ContextStatus)) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = InternalPeCoffVerifyAppleSignatureFromContext (
               &Context,
               &ImageSize
               );
  }

  DEBUG ((
    DEBUG_ERROR,
    "SIGN: Signature check (%u -> %u) - %r\n",
    OrgImageSize,
    ImageSize,
    Status
    ));

  if (!EFI_ERROR (ContextStatus)) {
    ApfsStatus = InternalPeCoffGetApfsDriverVersionFromContext (&Context, ImageSize, &DriverVersion);

    if (!EFI_ERROR (ApfsStatus)) {
      DEBUG ((
        DEBUG_ERROR,
        "SIGN: Got APFS %Lu (%-16a %-16a)\n",
        DriverVersion->Version,
        DriverVersion->Date,
        DriverVersion->Time
        ));
    }
  }

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
  CONST CHAR8  *ImageFileName;
  UINT32       ImageSize;
  UINT8        *ImageFileBuffer;
  BOOLEAN      IsFat;
  BOOLEAN      ForceFixup;
  INT32        RetVal;

  //
  // Enable PCD debug logging.
  //
  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  //
  // Match PcdImageProtectionPolicy defined in OpenCore.dsc.
  // Apple images (supplied by user) may be not 4KBi aligned.
  //
  PcdGet32 (PcdImageProtectionPolicy) = 0x03;

  //
  // Process args or print usage.
  //
  if (argc == 2) {
    ForceFixup    = FALSE;
    ImageFileName = argv[1];
  } else if ((argc == 3) && (AsciiStrCmp ("-f", argv[1]) == 0)) {
    ForceFixup    = TRUE;
    ImageFileName = argv[2];
  } else {
    PrintHelp ();
    return EXIT_FAILURE;
  }

  ImageFileBuffer = UserReadFile (ImageFileName, &ImageSize);
  if (ImageFileBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to read %a\n", ImageFileName));
    return EXIT_FAILURE;
  }

  IsFat   = FALSE;
  RetVal  = EXIT_SUCCESS;
  RetVal |= VerifySignatureAndApfs (ImageFileBuffer, ImageSize, &IsFat, ForceFixup, PE_ARCH_32);
  if (IsFat) {
    DEBUG ((DEBUG_ERROR, "\n"));
  }

  RetVal |= VerifySignatureAndApfs (ImageFileBuffer, ImageSize, &IsFat, ForceFixup, PE_ARCH_64);
  if (!IsFat) {
    RetVal |= VerifySignatureAndApfs (ImageFileBuffer, ImageSize, &IsFat, ForceFixup, PE_ARCH_ANY);
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

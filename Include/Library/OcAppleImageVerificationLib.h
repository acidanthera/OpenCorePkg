/** @file

OcAppleImageVerificationLib

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DXE_IMAGE_VERIFICATION_H
#define APPLE_DXE_IMAGE_VERIFICATION_H

#include <IndustryStandard/PeImage.h>

#define APPLE_SIGNATURE_SECENTRY_SIZE 8

typedef struct APPLE_PE_COFF_LOADER_IMAGE_CONTEXT_ {
  UINT64                           ImageAddress;
  UINT64                           ImageSize;
  UINT64                           EntryPoint;
  UINT64                           SizeOfHeaders;
  UINT16                           ImageType;
  UINT16                           NumberOfSections;
  UINT32                           *OptHdrChecksum;
  UINT32                           SizeOfOptionalHeader;
  UINT64                           SumOfSectionBytes;
  EFI_IMAGE_SECTION_HEADER         *FirstSection;
  EFI_IMAGE_DATA_DIRECTORY         *RelocDir;
  EFI_IMAGE_DATA_DIRECTORY         *SecDir;
  UINT64                           NumberOfRvaAndSizes;
  UINT16                           PeHdrMagic;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr;
  UINT8                            PeImageHash[32];
} APPLE_PE_COFF_LOADER_IMAGE_CONTEXT;

//
// Signature context
//
typedef struct APPLE_SIGNATURE_CONTEXT_ {
  UINT8                            PublicKey[256];
  UINT8                            PublicKeyHash[32];
  UINT8                            Signature[256];
} APPLE_SIGNATURE_CONTEXT;

//
// Function prototypes
//
EFI_STATUS
BuildPeContext (
  VOID                                *Image,
  UINTN                               ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  );

VOID
SanitizeApplePeImage (
  VOID                                *Image,
  UINTN                               *RealImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  );

EFI_STATUS
GetApplePeImageSignature (
  VOID                                *Image,
  UINTN                               ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  APPLE_SIGNATURE_CONTEXT             *SignatureContext
  );

EFI_STATUS
GetApplePeImageSha256 (
  VOID                                *Image,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  );

EFI_STATUS
VerifyApplePeImageSignature (
  IN OUT VOID                                *PeImage,
  IN OUT UINTN                               *ImageSize
  );

#endif //APPLE_DXE_IMAGE_VERIFICATION_H

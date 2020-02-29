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

#ifndef APPLE_EFI_PE_IMAGE_H
#define APPLE_EFI_PE_IMAGE_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <Library/OcCryptoLib.h>
#include "Edk2PeImage.h"

//
// The context structure used while PE/COFF image is being loaded and relocated.
//
typedef struct APPLE_PE_COFF_LOADER_IMAGE_CONTEXT_ {
    uint64_t                         ImageAddress;
    uint64_t                         ImageSize;
    uint64_t                         EntryPoint;
    uint64_t                         SizeOfHeaders;
    uint16_t                         ImageType;
    uint16_t                         NumberOfSections;
    uint32_t                         *OptHdrChecksum;
    uint32_t                         SizeOfOptionalHeader;
    uint64_t                         SumOfSectionBytes;
    EFI_IMAGE_SECTION_HEADER         *FirstSection;
    EFI_IMAGE_DATA_DIRECTORY         *RelocDir;
    EFI_IMAGE_DATA_DIRECTORY         *SecDir;
    uint64_t                         NumberOfRvaAndSizes;
    uint16_t                         PeHdrMagic;
    EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr;
} APPLE_PE_COFF_LOADER_IMAGE_CONTEXT;

typedef struct APPLE_SIGNATURE_DIRECTORY_ {
    uint32_t  ImageSize;
    uint32_t  SignatureDirectorySize;
    uint32_t  SignatureSize;
    uint16_t  CompressionType;
    uint16_t  EfiSignature;
    EFI_GUID  UnknownGuid;
    EFI_GUID  CertType;
    uint8_t   PublicKey[256];
    uint8_t   Signature[256];
}  APPLE_SIGNATURE_DIRECTORY;

#define APPLE_SIGNATURE_SECENTRY_SIZE 8

//
// Function prototypes
//
int
BuildPeContext (
  void                               *Image,
  uint32_t                           *RealImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT *Context
  );

int
GetApplePeImageSignature (
  void                               *Image,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT *Context,
  uint8_t                            *PkLe,
  uint8_t                            *PkBe,
  uint8_t                            *SigLe,
  uint8_t                            *SigBe
  );

int
GetApplePeImageSha256 (
  void                                *Image,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  uint8_t                             *CalcucatedHash
  );

int
VerifyApplePeImageSignature (
  void     *PeImage,
  uint32_t ImageSize
  );

int
VerifyAppleImageSignature (
  uint8_t  *Image,
  uint32_t ImageSize
  );


#endif //APPLE_EFI_PE_IMAGE_H

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

#ifndef APPLE_EFI_FAT_BINARY_H
#define APPLE_EFI_FAT_BINARY_H

#include <stdio.h>
#include <stdint.h>

#define EFI_FAT_MAGIC  0x0ef1fab9
#define CPU_ARCH_ABI64 0x01000000
#define CPU_TYPE_X86 7
#define CPU_TYPE_X86_64 (CPU_TYPE_X86 | CPU_ARCH_ABI64)

typedef struct {
    //
    // Probably 0x07 (CPU_TYPE_X86) or 0x01000007 (CPU_TYPE_X86_64)
    //
    uint32_t CpuType;
    //
    // Probably 3 (CPU_SUBTYPE_I386_ALL)
    //
    uint32_t CpuSubtype;
    //
    // Offset to beginning of architecture section
    //
    uint32_t Offset;
    //
    // Size of arch section
    //
    uint32_t Size;
    //
    // Alignment
    //
    uint32_t Align;
} EFIFatArchHeader;

typedef struct {
    //
    // Apple EFI fat binary magic number (0x0ef1fab9)
    //
    uint32_t Magic;
    //
    // Number of architectures
    //
    uint32_t NumArchs;
    //
    // Architecture headers
    //
    EFIFatArchHeader Archs[];
} EFIFatHeader;

//
// Functions prototypes
//
int
VerifyAppleImageSignature (
  uint8_t  *Image,
  uint32_t ImageSize
  );

#endif //APPLE_EFI_FAT_BINARY_H

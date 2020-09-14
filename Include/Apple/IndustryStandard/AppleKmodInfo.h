/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_KMOD_INFO_H
#define APPLE_KMOD_INFO_H

///
/// kmod structure definitions.
/// Keep in sync with XNU osfmk/mach/kmod.h.
/// Last sync time: 4903.221.2.
///

#define KMOD_INFO_VERSION     1
#define KMOD_MAX_NAME         64
#define KMOD_RETURN_SUCCESS    0
#define KMOD_RETURN_FAILURE    5

#pragma pack(4)
//
// A compatibility definition of kmod_info_t for 32-bit kexts.
//
typedef struct KMOD_INFO_32_V1_ {
    UINT32            NextAddr;
    INT32             InfoVersion;
    UINT32            Id;
    UINT8             Name[KMOD_MAX_NAME];
    UINT8             Version[KMOD_MAX_NAME];
    INT32             ReferenceCount;
    UINT32            ReferenceListAddr;
    UINT32            Address;
    UINT32            Size;
    UINT32            HdrSize;
    UINT32            StartAddr;
    UINT32            StopAddr;
} KMOD_INFO_32_V1;

//
// A compatibility definition of kmod_info_t for 64-bit kexts.
//
typedef struct KMOD_INFO_64_V1 {
    UINT64            NextAddr;
    INT32             InfoVersion;
    UINT32            Id;
    UINT8             Name[KMOD_MAX_NAME];
    UINT8             Version[KMOD_MAX_NAME];
    INT32             ReferenceCount;
    UINT64            ReferenceListAddr;
    UINT64            Address;
    UINT64            Size;
    UINT64            HdrSize;
    UINT64            StartAddr;
    UINT64            StopAddr;
} KMOD_INFO_64_V1;

#pragma pack()

typedef union {
  KMOD_INFO_32_V1 Kmod32;
  KMOD_INFO_64_V1 Kmod64;
} KMOD_INFO_ANY;

#endif // APPLE_KMOD_INFO_H

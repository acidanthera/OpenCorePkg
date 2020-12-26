/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef LZVN_H
#define LZVN_H

#include <Library/BaseMemoryLib.h>
#include <Library/OcCompressionLib.h>

#define lzvn_decode_buffer DecompressLZVN

#ifdef EFIUSER
#include <stdint.h>
#include <stddef.h>
#include <string.h>

//
// Ugly workaround for size_t incompatibility with UINTN.
//
#ifdef size_t
#undef size_t
#endif
#define size_t UINTN

#else
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;

typedef UINTN size_t;
typedef UINTN uintmax_t;

#ifdef memset
#undef memset
#endif

#ifdef memcpy
#undef memcpy
#endif

#define memset(Dst, Value, Size) SetMem ((Dst), (Size), (UINT8)(Value))
#define memcpy(Dst, Src, Size) CopyMem ((Dst), (Src), (Size))

#endif

#endif /* LZVN_H */

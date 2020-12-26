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

#ifndef LZSS_H
#define LZSS_H

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCompressionLib.h>

#define compress_lzss CompressLZSS
#define decompress_lzss DecompressLZSS

#ifdef EFIUSER
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#else

typedef INT8  int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;

#ifdef memset
#undef memset
#endif

#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

#define memset(Dst, Val, Size) SetMem ((Dst), (Size), (Val))
#define malloc(Size) AllocatePool (Size)
#define free(Ptr) FreePool (Ptr)
#endif

typedef UINT8  u_int8_t;
typedef UINT16 u_int16_t;
typedef UINT32 u_int32_t;

#ifdef bzero
#undef bzero
#endif
#define bzero(Dst, Size) ZeroMem ((Dst), (Size))

#endif // LZSS_H

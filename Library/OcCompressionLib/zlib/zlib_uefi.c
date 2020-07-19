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

#include "zutil.h"

#include <Library/MemoryAllocationLib.h>
#include <Library/OcCompressionLib.h>

voidpf ZLIB_INTERNAL zcalloc (opaque, items, size)
    voidpf opaque;
    unsigned items;
    unsigned size;
{
  (void) opaque;
  return (voidpf) AllocatePool (items * size);
}

void ZLIB_INTERNAL zcfree (opaque, ptr)
    voidpf opaque;
    voidpf ptr;
{
  (void) opaque;
  FreePool (ptr);
}

UINT8 *
CompressZLIB (
  OUT UINT8        *Dst,
  IN  UINT32       DstLen,
  IN  CONST UINT8  *Src,
  IN  UINT32       SrcLen
  )
{
  uLongf  ResultingLen = DstLen;

  if (SrcLen > OC_COMPRESSION_MAX_LENGTH || DstLen > OC_COMPRESSION_MAX_LENGTH) {
    return 0;
  }

  if (compress (Dst, &ResultingLen, Src, SrcLen) == Z_OK) {
    return Dst + ResultingLen;
  }

  return NULL;
}

UINTN
DecompressZLIB (
  OUT UINT8        *Dst,
  IN  UINTN        DstLen,
  IN  CONST UINT8  *Src,
  IN  UINTN        SrcLen
  )
{
  uLongf  ResultingLen = DstLen;

  if (SrcLen > OC_COMPRESSION_MAX_LENGTH || DstLen > OC_COMPRESSION_MAX_LENGTH) {
    return 0;
  }

  if (uncompress (Dst, &ResultingLen, Src, SrcLen) == Z_OK) {
    return ResultingLen;
  }

  return 0;
}

UINT32
Adler32 (
  IN CONST UINT8  *Buffer,
  IN UINT32       BufferLen
  )
{
  return adler32 (1, Buffer, BufferLen);
}
